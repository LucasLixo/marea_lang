# Marea vm stage

## Contract

The `vm` stage is the reference interpreter: it reads a `MAREA_BytecodeModule` (`vm` never reads the IR, never
reads the disk and has no global state) and runs one function of it to completion, giving either a value or a
trap. A `MAREA_VmContext` (an arena and a diagnostic sink, created once and reused across calls) is the only
state a caller keeps; the interpreter itself keeps nothing between calls. A register is an untagged 64-bit value,
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

## Diagnostics

The vm raises no diagnostic: a fault the opcode defines is a `MAREA_VmResult` trap (the `MAREA_BytecodeTrapCode`
of `docs/bytecode/marea-bytecode.md`), and every other fault is a `MAREA_VmExcept` the caller reads directly
(`MAREA_VM_EXCEPT_ERROR_NULL_POINTER`, `MAREA_VM_EXCEPT_ERROR_OUT_OF_MEMORY`, `MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`,
`MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION`, `MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT`).

## Gaps

- Only the scalar opcodes of `MAREA_BytecodeGenerate`'s own current slice are interpreted (the stage covers
  every opcode the format header defines today, so this gap only matters once a later apply adds structures,
  arrays, enums, lifecycle, host calls or `fork_join` opcodes: this interpreter needs matching cases added then).
- The frame is a fixed-size native array (`MAREA_VM_FRAME_REGISTER_LIMIT`, 256 registers) on the C call stack, not
  an arena allocation, so a function that needs more registers than that is a vm limit
  (`MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE`) rather than a language-defined trap. Raising the limit, or moving the
  frame to the context's own arena, is future work, not needed by any test or example today.
- A call is native C recursion, one stack frame per bytecode call, bounded by `MAREA_VM_CALL_DEPTH_LIMIT` (512).
  A host that embeds the vm on a thread with a small stack may need to lower this limit; it is a compile-time
  constant, not yet read from a build setting.
- No parity test against `cgen` exists yet (`cgen` is not built): the "same observable behavior in both
  backends" goal of the design is checked only by this stage's own tests today.
- `MAREA_VmCall` trusts the function index it is given once it is in range; it does not call
  `MAREA_BytecodeValidate` itself first. A host that deserializes or assembles a module already gets that check
  for free (both readers validate before handing back a module), but a module built by hand with the add calls
  of `docs/bytecode/marea-bytecode.md` and handed to the vm directly skips it.
