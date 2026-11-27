# Marea vm stage

## Contract

The `vm` stage is the reference interpreter: it reads a `MAREA_BytecodeModule` (`vm` never reads the IR, never
reads the disk and has no global state) and runs one function of it to completion, giving either a value or a
trap. A `MAREA_VmContext` (an arena, a diagnostic sink and an optional type registry, created once and reused across
calls) is the only state a caller keeps; the interpreter itself keeps nothing between calls. The registry
(`marea::base`) gives it the C functions of the host and the replacements of the runtime hooks. The memory that a
program allocates comes from the allocator hooks, never from the arena of the context. A register is an untagged 64-bit value,
read the way the bytecode stage already normalizes it (sign extended for a signed integer, zero extended for an
unsigned one, a float32 as the float64 bit pattern that holds it). A fault the language itself defines (division
by zero, a shift out of range, a negative exponent, a convert that does not fit, `unreachable!()`, a missing
return, a call chain deeper than the vm allows) is a `MAREA_VmResult` trap, never a crash and never a diagnostic:
the vm never prints and never raises anything through the sink. A fault that is not a language trap (an unknown
function, the wrong argument count, a register or jump target a malformed module put out of range) is a
`MAREA_VmExcept` instead, because it is a misuse of the stage or a module that never passed
`MAREA_BytecodeDeserialize` or `MAREA_BytecodeAssemble`, not an outcome of the program that was run. The CMake
target is `Marea-Vm` (alias `marea::vm`, plus a shared target `Marea-Vm-Shared` when `MAREA_OPTION_SHARED_LIBS`
is on), built when `MAREA_OPTION_TARGET_VM` is on. The option is on by default and depends on
`MAREA_OPTION_TARGET_BYTECODE`.

## Use

```c
MAREA_VmContext *context;
MAREA_VmContextCreate(arena, sink, &context);

MAREA_Uinteger64 arguments[2] = { 2, 3 };
MAREA_VmResult result;

MAREA_VmCallByName(context, module, "main.add", 8, arguments, 2, &result);

if (result.trapped) {
    /* result.trap is a MAREA_BytecodeTrapCode: a defined fault, not a crash */
} else {
    /* result.value is the raw register bits of the result, read per the function's own result type */
}
```

A program that calls C needs a registry. A C function keeps its typed signature; one file-scope binding declaration
(`MAREA_HOST_FUNCTION`) makes the wrapper that the vm calls, and `MAREA_REGISTER_FUNCTION` registers it:

```c
static MAREA_Sinteger32 MAREA_CALL host_add(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return (left + right);
}

MAREA_HOST_FUNCTION(host_add, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_VALUE(sinteger32), MAREA_HOST_VALUE(sinteger32));

MAREA_TypeRegistry *registry;
MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, &registry);
MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_NONE, "demo.host", 9, "add", 3, host_add);
MAREA_VmContextSetTypeRegistry(context, registry);
```

`MAREA_VmFindFunction` finds a function by its full name alone (the namespace and the name,
`MAREA_BytecodeModuleGetString` of a function's own `name` field), and `MAREA_VmCall` runs a function already
found by index; `MAREA_VmCallByName` is the two together. The argument count must equal the function's own
`parameterCount` (`MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT` otherwise); every argument is one register, in parameter
order, read the same way `MAREA_BytecodeModuleGetParameter` and `MAREA_BytecodeModuleGetType` say to.

## The interpreter

- **One frame per call, no persistent stack.** `MAREA_VmRun` is the recursive core: a call instruction runs the
  callee through another call of itself, one native C stack frame per bytecode call. `MAREA_VM_CALL_DEPTH_LIMIT`
  (512) bounds that recursion; past it, the result is the trap `MAREA_BYTECODE_TRAP_CODE_STACK_OVERFLOW`, a
  defined outcome, never a native stack overflow. `MAREA_VM_FRAME_REGISTER_LIMIT` (256) bounds how many
  registers one frame keeps on the native stack; a function that asks for more is
  `MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`, a vm limit rather than a language trap, because the fixed-size frame is
  this reference interpreter's own implementation choice, not a rule the bytecode format states.
- **Arithmetic wraps; the saturating family clamps.** `add`, `sub`, `mul`, `neg`, `bit_not` and the non-saturating
  `pow` are computed with unsigned 64-bit wraparound arithmetic and then normalized to the operand's width, so
  there is never undefined behavior and the result always matches C11's own fixed-width wraparound rule. The
  saturating opcodes clamp to the kind's own minimum and maximum instead; at `sinteger64` and `uinteger64` width,
  where a plain 64-bit multiply could itself overflow, the clamp is decided from a portable, allocation-free
  64x64 to 128-bit unsigned multiply (schoolbook, two 32-bit halves) corrected for a negative operand, never a
  compiler extension and never a signed-overflow expression.
- **Division, remainder and shift.** Division and remainder trap on a zero divisor
  (`MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO`). A signed division or remainder by -1 never reaches the one C
  expression that could overflow (the lowest value of the width divided by -1): it is computed directly as an
  unsigned negation, which wraps back to the same lowest value, exactly as the design states. A shift traps
  (`MAREA_BYTECODE_TRAP_CODE_SHIFT_OUT_OF_RANGE`) when the count, read as unsigned, is not less than the width of
  the kind; a right shift is arithmetic for a signed kind and logical for an unsigned one.
- **Convert checks the range.** `convert` traps (`MAREA_BYTECODE_TRAP_CODE_CONVERT_OUT_OF_RANGE`) when the source
  value, read as the source kind, does not fit the target kind exactly: a narrowing integer convert whose value
  is outside the target's minimum and maximum, or a float convert whose value is `NaN` or outside the target
  integer's range. A same-kind or widening convert, and a float-to-float convert, never traps.
  `float32` and `float64` share one 64-bit register representation (a float64 bit pattern that a float32 value
  fits inside), so a float opcode always reads and writes a C `double`, rounding through `float` only when the
  kind is `float32`.
- **Calls.** The call opcode's window (one result register, then one register per argument, right above the
  caller's own registers in use) is read exactly as `MAREA_BytecodeGenerate` built it: the arguments become the
  callee's own first registers, and the callee's return value lands back in the window's first register. A
  trapped callee propagates immediately: the caller never executes another instruction once a trap is raised at
  any depth of the call chain.
- **Defensive bounds, not trusted input.** Every register index the interpreter reads is checked against the
  frame's own limit before use (an opcode's `b` or `c` means a register only for the opcodes that define it so;
  elsewhere it is an offset, a kind, a data or function index, or an immediate half, and is never checked as one).
  A jump target, a function index and a data blob index are checked through the accessor they are read with. A
  module that reaches one of these checks is treated as malformed (`MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`), not
  trusted to have passed `MAREA_BytecodeValidate`-style checks first.
- **A composite value lives in the frame's own memory, addressed by a pointer.** Beside its register file, every
  frame keeps `MAREA_VM_FRAME_MEMORY_LIMIT` (4096) bytes of its own, zero filled at the start of the call, the
  same as the register file. `addr_of` turns a byte offset of that area into an address (the bit pattern of a
  real `MAREA_Byte *`, decoded and re-encoded with `memcpy`, never a cast that assumes a representation);
  `field` adds a constant byte offset to an address already held in a register; `load` and `store` read or write
  one scalar there, normalized the same way a typed arithmetic opcode is; `zero` clears a byte range at an
  address, or the register itself when the byte count is 0; `copy_bytes` duplicates a byte range between two
  addresses (`memmove`, so the ranges may overlap); `load_data_block` copies one blob of the read-only data
  section to an address, as many bytes as the blob holds. The area starts at a 16-byte address
  (`MAREA_VM_FRAME_MEMORY_ALIGNMENT`). None of these opcodes reads the module's type table:
  the byte offsets and the counts are baked into the instructions by `MAREA_BytecodeGenerate`, so the vm only
  ever moves bytes it is told to, never a type it looks up itself. A composite address stays valid for as long
  as its frame is on the call chain (including every call that frame makes), because the memory is the C stack
  frame of the recursive `MAREA_VmRun` that owns it, exactly like the register file; passing one to another
  function (a borrowed structure's own `self`, for instance) is an ordinary 64-bit argument, needing no change to
  the call opcode. A null address (only a malformed module can produce one; `MAREA_BytecodeGenerate` never does)
  is `MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`, never a dereference. There is no dedicated index opcode: a
  bounds-checked element access is `lt_u` against the known length, `jump_if_not` to a `trap`
  (`MAREA_BYTECODE_TRAP_CODE_INDEX_OUT_OF_BOUNDS`), the same pattern a generated `if` already uses, proven directly
  in `tests/source/vm/MAREAVm.cpp` against a hand-built module (`IndexOutOfRangeTrapsOnABoundsCheckedArray`).

## Host calls

A `call` whose function index is an import (`MAREA_BYTECODE_FUNCTION_FLAG_HOST`) goes to C. The vm splits the name of
the import at its last dot and asks the registry of the context for that namespace and name
(`MAREA_TypeRegistryFindFunction`), at each call. Nothing registered, or a context with no registry, is the trap
`HOST_FUNCTION_MISSING`. The registry rejects a registration of a user or a host under `c` or `marea`, so only a
shipped library, through the internal entry, answers there.

### The binding of a C function

`MAREA_HOST_FUNCTION(function, result, parameters...)` (`marea/base/MAREAHostFunction.h`) is written at file scope,
after the declaration of the function and in the file that registers it. It describes the signature, and the C
function must have exactly that signature, with `MAREA_CALL`:

| In the binding | The C function |
|---|---|
| `MAREA_HOST_VALUE(scalar)` | a parameter by value; `scalar` is `boolean`, `sinteger8` to `sinteger64`, `uinteger8` to `uinteger64`, `float32` or `float64`, and the C type is `MAREA_Boolean`, `MAREA_Sinteger8`, ... `MAREA_Float64` |
| `MAREA_HOST_BORROW(Type)` | `const Type *` |
| `MAREA_HOST_BORROW_MUTABLE(Type)` | `Type *` |
| `MAREA_HOST_CONTEXT` | `MAREA_HostContext *`, not a parameter of the program; only a function that lists it gets it |
| `MAREA_HOST_RETURNS_NOTHING` | `void` |
| `MAREA_HOST_RETURNS(scalar)` | the C type of `scalar`, returned by value |
| `MAREA_HOST_RETURNS_STRUCTURE(Type)` | `void`, with a last parameter `Type *` for the result |

A signature that differs (a type, a missing or extra parameter, a context that the function does not take) and a
function with more than `MAREA_HOST_FUNCTION_PARAMETER_LIMIT` (32) parameters are compile errors in C and in C++,
with any warning settings. `MAREA_REGISTER_FUNCTION` takes the name of the function, so a function with no binding
does not compile. The wrapper is for the vm; a C caller calls the typed function itself.

### The shape the vm calls

The binding makes a function of the one shape `MAREA_HostFunction`:
`void f(MAREA_HostContext *context, const MAREA_Null *arguments, MAREA_Null result)`.

| Value | Pointer that the wrapper gets |
|---|---|
| A borrowed structure | the bytes of the value itself, with its layout; a write through it changes the value of the program |
| A scalar argument | 8 bytes in the register form: an integer sign or zero extended to 64 bits, a float32 or float64 as a float64, a boolean as the integer 0 or 1 |
| A structure result | the bytes of the result, cleared by the bytecode before the call |
| A scalar result | 8 zeroed bytes; the vm then normalizes them to the declared type (`sinteger8` 200 is -56, a `float32` is rounded, a `boolean` is 0 or 1) |
| No result | a null pointer |

A call whose result nobody wants (no result in the import, for a call that sema could not type) still runs: the
wrapper drops a scalar result and gives a structure result a zeroed local of its own. The vm cannot check that the
types of an import match what the function reads and writes: the binding, the signature given to sema and the
registration must agree.

### The host context

Every call gets a new `MAREA_HostContext` (`marea/base/MAREAHostContext.h`), so the vm keeps no state between calls.
`MAREA_HostContextAllocate` gives a zeroed block from the replaced `alloc` hook, or from the allocator of the vm, and
`MAREA_HostContextFree` gives it back. A failed allocation is a result of the call, not a trap: the function raises
`MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY)` and returns. A raised trap stops the run when
the function returns, drops its result and is the trap `OUT_OF_MEMORY` of the run; nothing after the call runs. With
one replaced hook only, a function that allocates or frees makes the call
`MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR`, as `alloc` and `free` do. A host function adds no frame to the call depth.

A host function takes at most `MAREA_VM_HOST_PARAMETER_LIMIT` (32) parameters; an import with more is
`MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`. `MAREA_VmFindFunction` does not find an import, and `MAREA_VmCall` of one is
`MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION`.

## Allocation

`alloc` and `free` use the runtime hooks of the registry (`MAREA_TypeRegistryReplaceRuntimeFunction` with `"alloc"`
of the shape `MAREA_RuntimeAllocateFunction` and `"free"` of the shape `MAREA_RuntimeFreeFunction`), else the
allocator of the vm, which is `malloc` and `free` of the C library with the requested alignment. A host replaces both
or neither: with one only, the first `alloc` or `free` is `MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR`.

- The vm clears every new block, also a block of a host allocator.
- A request of 0 bytes asks for 1 byte, so it still gives an address that `free` takes.
- A null block, or a size that the machine cannot hold, is the trap `OUT_OF_MEMORY`.
- An alignment that is not a power of two is a malformed module (`MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`).
- `free` of the null address does nothing. A second `free` of a block, or an address that `alloc` did not give, is
  undefined. A block that is still held when a run ends or traps is never freed by the vm.

## fork_join

The vm has nothing for `fork_join`: the bytecode stage lowers it to the plain calls and loops of one thread, so a trap
in one worker ends the run and no later worker starts.

## Diagnostics

The vm raises no diagnostic: a fault the opcode defines is a `MAREA_VmResult` trap (the `MAREA_BytecodeTrapCode`
of `docs/bytecode/marea-bytecode.md`), and every other fault is a `MAREA_VmExcept` the caller reads directly
(`MAREA_VM_EXCEPT_ERROR_NULL_POINTER`, `MAREA_VM_EXCEPT_ERROR_OUT_OF_MEMORY`, `MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`,
`MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION`, `MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT`,
`MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR`).

## Gaps

- Every opcode the format header defines today is interpreted; a later opcode needs a matching case added here.
- The vm does not check the binding of a host function against the import of the module: a binding and a sema
  signature that differ are undefined.
- A binding has no form for a `character`, a `string`, an `enum` or a library type, as the bytecode stage cannot pass
  them yet, and a scalar write borrow does not exist.
- The traps that a host function can raise are `MAREA_HostTrap`, which holds `OUT_OF_MEMORY` only.
- A typed function reads a borrowed structure in the frame memory of the vm through a pointer of its own type, which
  relies on the vm being compiled apart from the host functions.
- `fork_join` is sequential: there is no thread and no atomic in the vm.
- Nothing traps on a double `free` and nothing tracks the blocks of a run, so a run that traps leaks them.
- A structure with an alignment above 16 may reach a C function misaligned: the frame memory area is aligned to 16 only.
- An address of a host allocator that does not meet the requested alignment is not detected.
- The composite opcodes move bytes at the address they are given with no bounds check of their own (the generator
  is trusted to have reserved enough frame memory and to have computed every offset correctly); only a
  dynamically indexed element, built from `lt_u` and a trap, is checked at run time. A malformed module that
  gives `addr_of` an offset past `MAREA_VM_FRAME_MEMORY_LIMIT`, or a `field`, `copy_bytes` or `load_data_block`
  a byte count that runs past the end of the area it was given, is undefined, the one place this interpreter
  still trusts the module rather than checking it; `MAREA_BytecodeGenerate` itself never emits such an offset.
  A tighter check (the address plus the count against the owning frame's own range) is future work.
- `MAREA_BytecodeGenerate` does not yet lower a structure as a parameter or a result of a function of the unit, a
  user `construct` or `destroy`, an `enum` with data, a `vector`, a `tuple` or a `string` (see the Gaps of
  `docs/bytecode/marea-bytecode.md`), so this interpreter has not run any of them through a generated program
  yet; its own mechanics for a tagged union are proven directly against hand-built
  modules instead (`tests/source/vm/MAREAVm.cpp`).
- The frame is a fixed-size native array (`MAREA_VM_FRAME_REGISTER_LIMIT`, 256 registers, and
  `MAREA_VM_FRAME_MEMORY_LIMIT`, 4096 bytes) on the C call stack, not an arena allocation, so a function that
  needs more registers or more composite storage than that is a vm limit (`MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`,
  or `OPERAND_OVERFLOW` at generate time for the memory area) rather than a language-defined trap. Raising either
  limit, or moving the frame to the context's own arena, is future work, not needed by any test or example today.
- A call is native C recursion, one stack frame per bytecode call, bounded by `MAREA_VM_CALL_DEPTH_LIMIT` (512).
  A host that embeds the vm on a thread with a small stack may need to lower this limit; it is a compile-time
  constant, not yet read from a build setting.
- The parity test with `cgen` (see [marea-cgen.md](../cgen/marea-cgen.md), "Parity with the vm") compares only
  what this interpreter runs, so each construct that it cannot run is outside the comparison.
- A saturating multiply of two large `uinteger32` values gives 0 instead of the maximum, since the product is
  computed in a signed 64-bit integer.
- A float of exactly 2^63 or 2^64 passes the range check of a conversion to a 64-bit integer, which then gives a
  value instead of `CONVERT_OUT_OF_RANGE`.
- `MAREA_VmCall` trusts the function index it is given once it is in range; it does not call
  `MAREA_BytecodeValidate` itself first. A host that deserializes or assembles a module already gets that check
  for free (both readers validate before handing back a module), but a module built by hand with the add calls
  of `docs/bytecode/marea-bytecode.md` and handed to the vm directly skips it.
