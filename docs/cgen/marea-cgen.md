# Marea cgen stage

## Contract

The `cgen` stage is the second backend of the one IR. It reads a `MAREA_IrNode` tree rooted at
`MAREA_IR_KIND_MODULE` (the output of `marea::ir`) and writes C11 source text into a caller-owned
arena. It also reads what the caller gives in a `MAREA_CgenContext`: the type registry of the library
types, a function that names the C functions of the host, the headers that declare them and the
allocator hooks. It has no global state, never prints and never reads or writes the disk. The stage
links only `marea::ir` (and, through it, the earlier stages); it never reads `marea::bytecode` or
`marea::vm`, so the two backends of the one IR stay independent.

A construct that this stage cannot write is not an exception: it is a diagnostic in the sink. The
function that holds it, and every function that calls that function, gets no declaration and no
definition, and `MAREA_CgenGenerateModule` still returns `MAREA_CGEN_EXCEPT_SUCCESS`. The text that
remains is always a complete translation unit.

The text is for clang or gcc. It uses `__attribute__` and compiler built-ins, and the tests compile
it with `-std=c11 -Wall -Wextra -Wpedantic -Werror`.

## Use

```c
MAREA_CgenContext *context;
const MAREA_Character8 *text;
MAREA_IntegerSize length;

MAREA_CgenContextCreate(arena, sink, &context);
MAREA_CgenContextSetTypeRegistry(context, registry);          /* the registry that sema got */
MAREA_CgenContextSetHostResolver(context, resolve, userData); /* names the C function of a name */
MAREA_CgenContextAddInclude(context, "host/functions.h", 16); /* declares those functions */
MAREA_CgenContextSetAllocator(context, "host_alloc", 10, "host_free", 9); /* optional */

MAREA_CgenGenerateModule(context, irModule, &text, &length);

/* text is NUL-terminated C11 source; hand it to a C compiler found by CMake, as the tests do. */
```

Each setter is optional. With no registry, no library type has a form. With no resolver, every call
of the host traps `HOST_FUNCTION_MISSING`.

## Generated functions and names

A function is `R f_name(parameters, marea_rt_trap *marea_trap)`. The caller sets the trap to
`MAREA_RT_TRAP_NONE`, calls and reads it; after a trap the result has no meaning (it is zero).

| Source | C name |
|---|---|
| function `name` | `f_name` |
| function `name` of a type `Type` | `f_<length of Type>_Type_name`, for example `f_5_Point_sum` |
| parameter or local `name` | `v_name` |
| hidden local `$name` of the ir | `h_name` |
| structure `Name`, enum `Name`, array | `marea_s_Name`, `marea_e_Name`, `marea_a<number>` |
| field `name` | `f_name` |
| temporary | `marea_t<number>` |

A byte that is not a letter, a digit or an underscore becomes an underscore.

A primitive and an enum pass by value. A structure passes by `const T *`, or by `T *` with a write
borrow (`#[semantics = reference_mutable]`). An array always passes by `T *`. A result comes back by
value.

## Traps

The codes of `marea_rt_trap` are the codes of the vm:

| Code | Name | Code | Name |
|---|---|---|---|
| 0 | NONE | 6 | MISSING_RETURN |
| 1 | DIVISION_BY_ZERO | 7 | STACK_OVERFLOW (never set by cgen) |
| 2 | SHIFT_OUT_OF_RANGE | 8 | INDEX_OUT_OF_BOUNDS |
| 3 | NEGATIVE_EXPONENT | 9 | OUT_OF_MEMORY |
| 4 | CONVERT_OUT_OF_RANGE | 10 | HOST_FUNCTION_MISSING |
| 5 | UNREACHABLE | 11 | INVALID_PARTITION_COUNT |

After each operation that can trap, and after each call, a function that sees a trap returns at once.
Nothing else runs and nothing is destroyed. `unreachable!()` sets UNREACHABLE and returns.

## Evaluation order

Operands and arguments run from left to right, and each operation goes into a temporary, so the order
of the source never depends on the C compiler. An assignment reaches its target first (with the check
of each index) and then runs the value; a compound assignment reads the target after the right side.
`and` and `or` run their right side only when the left one does not decide. The test of an `elseif`
runs only when it is reached. The value of a new local reads the outer name that the local hides.

## Arithmetic

No operation relies on C undefined behavior. Each one gives the outcome of the vm
([marea-vm.md](../vm/marea-vm.md)).

- `+ - * -(unary) ~ & | ^` on integers wrap, through `uint64_t`. The wrapping forms are the same.
- `/` and `%` trap on a zero divisor. A signed divisor of -1 negates with a wrap, or gives remainder 0.
- A shift reads its whole count as an unsigned 64-bit number, so a negative count is out of range. A
  count of the width of the type or more traps.
- `**` wraps. A negative signed exponent traps.
- A saturating form (`+| -| *|`) stays inside the type.
- Float `+ - * /` are plain C in the width of the type. A float `%` and `**` run `fmod` and `pow` in
  double. The saturating forms of floats are the plain ones.
- `convert` traps when the target cannot hold the value: an integer that does not fit, a float that is
  NaN or out of range, also exactly 2^63 or 2^64 for the 64-bit integers. A float is truncated. An
  integer becomes a float through double.

## Types and layout

The scalars are `int8_t` to `int64_t`, `uint8_t` to `uint64_t`, `float`, `double` and `bool`.

- **Structure.** The layout of sema, with explicit padding. `_Static_assert` checks the size, the
  alignment and every offset. A layout that C would not give is written `__packed__` or
  `__aligned__`. A structure whose layout sema does not measure (a string field, a generic type) has
  no form.
- **Enum.** With no data and the size of its tag, a `typedef` of the tag integer. Else a structure with
  `marea_tag` and a union `marea_data` of the data of each variant, at the offsets of sema, also
  asserted. `==` and `!=` compare the variant. A data field is read and written under the tag; the
  field of another variant is the trap UNREACHABLE.
- **Array.** A registered type with an index protocol, no role, a copy class and two type arguments,
  the element and a literal count, is a structure that holds `items[count]`. This is the shape that
  sema measures. An index is checked on each access; a negative index is out of bounds. The spelling
  of an element type is read through the aliases that the module shows. A count of 0 is a marked zero
  length array (an extension of the C compiler): its index always traps and its loops are not written.
  `size`, `max_size` and `empty` are constants, `front` and `back` copy an end, `fill`, `filled` and
  `swap` are loops or whole copies.
- **Included file.** `INCLUDED_BYTES` is one static array for each file of the module, named
  `marea_included_<hash of the namespace, the path and the bytes>`. File scope `asm` puts the file
  with `.incbin` into the read only section of ELF, Mach-O or COFF, and a `.if` stops the C build when
  the file no longer has the checked size. Emscripten has no `.incbin`: there a constructor loads the
  path from the virtual file system into a static buffer, and a file that is missing or has another
  size aborts. The build then has to put the file at that path (`--embed-file` or `--preload-file`).
  The C compiler must be able to open the path that the include resolver gave.
- A constant is built where it is read, so each reader gets a new value.
- `build_value!` is its default value.
- `copy` gives a new value. `move` of a composite value leaves zeros in the source.
- A borrow of storage inside a packed type passes an aligned copy, and a write borrow copies it back
  after the call.

## Lifecycle

A creation (`Type(args)`, or `Type{...}` of a type with `construct`) makes the zero value or the
literal first, then runs the arguments, then the construct operator. The ir does not name that
operator, so cgen takes the one function of the type with no result, no `self`, a default for every
parameter, the argument count of the call, that is not a destroy operator. With no argument, no literal
and no function of that shape, the value is only zeros. Any other case is reported, never guessed.

The destroy calls are the ones the ir inserts at each scope exit, in the reverse order of creation. A
trap runs none of them. A local array is destroyed by one call of the operator of its element for each
element.

**Vectors.** A registered growable run (see the sema doc) is `MAREA_VectorHeader` in C, so the program includes
`marea/base/MAREAVectorRuntime.h` and links `marea::base`. The storage changes only through that runtime, which the
`vm` calls too: capacities, the growth rule `max(4, 2 * capacity)` and `OUT_OF_MEMORY` are the same in both backends.
Each call of the runtime gets a `MAREA_HostContext` with the allocator hooks of the context. An element of no size is a
zeroed byte of the function. An index is compared as an unsigned value with the size.

A value that holds a vector has one owner. A name, a field or an element that is taken (an initializer, an assignment,
a `return`, a stored value, `move`) leaves zeros behind, and `copy` is a deep copy. A block destroys what it still owns,
and so does each `return`, `break`, `continue` and trap that leaves it; a temporary that is only borrowed is destroyed
with its block. A vector destroys its elements from the last to the first, with the destroy operator of an element, then
gives its storage back. A parameter that holds a vector passes by its address, so a callee that takes it over leaves
the value of the caller empty. A trap gives the storage back and runs no destroy operator. The value of an assignment to an element of a vector runs
before its place is reached,
since a call in the value can move the storage.

## Host calls

A call that `MAREA_IrNodeIsExternalCall` marks goes to the C function that the resolver gives for its
full path, for example `demo.host.add`. The generated text calls that typed function itself (not the
wrapper that `MAREA_HOST_FUNCTION` makes for the registry), and a header of the context declares it.

- A primitive passes by value and a composite value by address (`(void *)&x`). A structure result
  comes back through a zeroed value that is passed last. A primitive result is converted to the type
  that sema gave the call.
- A name with no answer runs its arguments and then traps `HOST_FUNCTION_MISSING`.
- A function that takes a `MAREA_HostContext *` (its `contextPosition`) gets a context that the
  generated code makes on its stack with `MAREA_HostContextInitialize`, with the allocator hooks of
  the context or with the allocator of the base library. After the call, a raised trap stops the run.
  Such a program includes `marea/base/MAREAHostContext.h` and links `marea::base`.

## fork_join and partition

A `fork_join` is a block and a `spawn` statement is its call, run where it stands, so every worker
ends before the next statement. A `partition` runs its body once for each part, from part 0; a part
count of zero or less traps `INVALID_PARTITION_COUNT`. The part is not a value yet.

## Parity with the vm

`tests/source/language/MAREALanguageParity.cpp` runs every accept item of `tests/language` that has a
function with a result through both backends. The ir of an item is lowered once; the vm runs its
bytecode, and the C compilers that the build found run the text of cgen (the first with address and
undefined behavior checks where it can link them, the second optimized). The compilers must print the
same lines, and those lines must match the vm.

The calling convention of an item: each function with a primitive result and only primitive
parameters is called with the same sample values, eight rows (one call when it takes no value). The
test compares the trap of each call, the value when there is no trap (two NaN values are equal) and
the log of the host functions.

Of 314 accept items, 235 have a function with a result: 93 run on both backends (375 calls) and 135
are excluded, each with a reason in the table of the test. The test checks every line both ways, so a
line that no longer holds fails. The 71 excluded items that cgen writes are still compiled. Five items
whose functions take only composites are compiled and not run. Two items of `marea.tuple` are ahead of
the compiler and fail until that library exists.

| Items | Reason | cgen writes it |
|---|---|---|
| 55 | the vm takes and returns no structure between functions | yes |
| 9 | the vm holds no enum value | yes |
| 6 | the vm runs no construct and no destroy operator | yes |
| 1 | the vm reads no constant of a structure type | yes |
| 37 | a library type that no registry describes (an owner, a pointer, a view, a task, a C library) | no |
| 11 | a string | no |
| 3 | a call of a C function that no signature types and no host registers | no |
| 2 | a character | no |
| 2 | a function type value | no |
| 4 | a `with` block | no |
| 1 | a generic structure, which the ir does not instantiate | no |
| 1 | an operator or a method of a library array | no |
| 2 | `size_of`, which neither backend lowers | no |
| 1 | a user operator and a C function that no signature types | no |

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 7001 | UNSUPPORTED_CONSTRUCT | A `with` block, the handle of a spawn, a construct operator that cannot be told apart, a macro of a library, a built-in other than `convert`, a call of a function that is not written. |
| 7002 | UNSUPPORTED_TYPE | A string or a character, a structure with no layout, a library type that is no registered array or vector, a partition part used as a value. |
| 7003 | UNRESOLVED_NAME | A call into another module, a name that is no local, parameter, constant or function. |
| 7004 | UNTYPED_OPERAND | A host call that no signature types used as a value, or an argument of a host call with no type. |

## Gaps

- Not written: strings, characters, function values, generic structures, library types other than a
  registered array or vector, operators and methods of a library, `with`, user operators (the ir calls no
  function for them), `size_of`, `align_of`, macros other than `build_value!`, the handle of a spawn,
  the value of a partition part and a call into another module.
- A `swap` or a `fill` of an array inside a packed structure writes through the place that cgen
  gives, with no test of its own. The Mach-O and COFF forms of an included file were not built.
- Vector: `data` is not written, and neither is a copy of an enum that holds a vector or an element type that holds
  itself through a vector. A trap gives back the storage of the values in scope, but not of a value that is being built;
  the `vm` gives no storage back after a trap, so a host that counts its allocator calls sees a difference.
- The ir does not name the construct operator. A type with two functions of that shape, or a construct
  with no parameter called as `Type()`, is reported.
- A returned local with a destroy operator is destroyed in the callee and again in the caller, since
  the ir inserts both calls.
- A write borrow of an enum or of a primitive passes a copy, since the ir marks such a parameter as a
  value, so the caller does not see the write. The vm does the same for a primitive.
- cgen sets no limit to the depth of calls; the vm traps `STACK_OVERFLOW` past 512 calls.
- cgen and the vm differ where the vm is wrong: the vm gives 0 for a saturating multiply of two large
  `uinteger32` values, and converts a float of exactly 2^63 or 2^64 to a 64-bit integer with no trap.
- A signed right shift relies on the arithmetic shift of clang and gcc.
- The C names of functions do not hold the namespace, so two generated modules cannot be linked into
  one program.
- cgen writes no declaration of a host function; a header of the context declares it. A C signature
  that differs from the signature of sema follows the conversion rules of C.
- The generated text needs clang or gcc.
