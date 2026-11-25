# Marea cgen stage

## Contract

The `cgen` stage is the second backend of the one IR: it reads a `MAREA_IrNode` tree rooted at
`MAREA_IR_KIND_MODULE` (the output of `marea::ir`) and writes C11 source text into a caller-owned
arena. It has no global state, never prints and never reads or writes the disk: the caller gives it
an arena and a diagnostic sink through a `MAREA_CgenContext`, and the written text is a plain
`MAREA_Character8` buffer the caller hands to its own C compiler or build step. The stage links only
`marea::ir` (and, through it, the earlier stages); it never reads `marea::bytecode` or `marea::vm`,
so the two backends of the one IR stay independent, as the project policy requires. A construct this
stage cannot generate yet is not an exception: it is a diagnostic in the sink, and
`MAREA_CgenGenerateModule` still returns `MAREA_CGEN_EXCEPT_SUCCESS` (the function that could not be
written is simply absent from the output).

## Use

```c
MAREA_CgenContext *context;
const MAREA_Character8 *text;
MAREA_IntegerSize length;

MAREA_CgenContextCreate(arena, sink, &context);
MAREA_CgenGenerateModule(context, irModule, &text, &length);

/* text is NUL-terminated C11 source; hand it to a C compiler found by CMake, as the tests do. */
```

## What this apply covers

Scalar functions only: their parameters, their locals, their expressions, their control flow (`if`,
`while`, the `while true` a `do ... while` lowers to, the `if` chain a `switch` lowers to, the `while`
a for-range loop lowers to) and a plain call of another function of the same module. A structure, an
enum, a lifecycle call (`construct`/`destroy`), a host call and `fork_join` are the next cgen apply
(`cgen-composites-and-parity`); a `FUNCTION_DECLARATION` that is not purely scalar, or a call this
stage does not resolve, is reported through the sink (`MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE`,
`_UNSUPPORTED_CONSTRUCT` or `_UNRESOLVED_NAME`) and left out of the generated text.

## Scalar kinds and name mangling

The ten scalar kinds the `ir` carries a resolved type for map to a fixed C11 type: `sinteger8..64` to
`int8_t..int64_t`, `uinteger8..64` to `uint8_t..uint64_t`, `float32`/`float64` to `float`/`double`, and
`boolean` to `bool` (`<stdbool.h>`). A function with no result type maps to `void`.

Every name cgen writes is mangled: a function gets the prefix `f_` and a local or a parameter gets
the prefix `v_`, each followed by its source text with every byte that is not a letter, a digit or an
underscore (so a hidden local such as `$return0` or `$last0`) replaced by `_`. The prefix is the only
guarantee two different source names before it never collide; the mangling is not meant to be
reversible. A function keeps the bare name sema and the `ir` leave it with inside its own module (see
the ir doc, "Aliases"): `main.add` called from inside `main` stays `add`, so cgen mangles `add` to
`f_add`, never reading the module's own namespace for this.

## Defined arithmetic, never C undefined behavior

Every generated module opens with a small preamble of `static inline` helper functions, one per
integer kind, that give the exact outcome `docs/vm/marea-vm.md` already defines for the reference
interpreter, so the two backends agree observably even though this apply ships no parity test yet
(that is `cgen-composites-and-parity`):

- `marea_rt_add_*`, `_sub_*`, `_mul_*`, `_neg_*`, `_not_*` compute through the kind's own unsigned
  type and cast back, the portable form of C11's own fixed-width wraparound rule, so `+ - * -(unary)
  ~` never overflow signed integer arithmetic.
- `marea_rt_div_*` and `_rem_*` take a `marea_rt_trap *` out parameter: a zero divisor sets
  `MAREA_RT_TRAP_DIVISION_BY_ZERO` and returns 0; a signed divisor of -1 is computed as an unsigned
  negation (never the one C expression, `INT_MIN / -1`, that would overflow) and a signed remainder
  by -1 is 0, both without consulting the dividend.
- `marea_rt_shl_*` and `_shr_*` also take a trap pointer: a count not less than the kind's own width
  sets `MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE` and returns 0, so the shift count is always in range by the
  time the C `<<`/`>>` operator runs. A signed right shift still leans on clang's and gcc's own
  implementation-defined (arithmetic, two's complement) choice for a negative left operand once the
  count is in range; it is not re-derived through an unsigned trick (see Gaps).
- Float arithmetic (`+ - * /`) is plain C, since IEEE 754 already defines every one of its outcomes
  (including a division by zero, which gives an infinity or a NaN, never a trap); `%` on a float is
  not generated (see Gaps).

Division, remainder, shift and a plain call all take (or receive) a `marea_rt_trap *marea_trap`, the
one hidden last parameter of every generated function. A trap is checked once per statement, right
after a statement that could have reached a trapping operation (a local's initializer, an
assignment's value, an expression statement, a `return` with a value): `if (*marea_trap) { return
<zero of the result kind>; }`. This is the statement-granularity stand-in for the vm's own rule that a
trapped callee never executes another instruction; it is coarser than the vm's own per-instruction
check (see Gaps).

## Control flow

`if`/`elseif`/`else`, `while` and `break`/`continue` lower to the same C11 keywords, because the IR
already desugars `do ... while`, a for-range loop and `switch` into exactly this shape (see the ir
doc, "For-range desugaring", "Switch desugaring" and "Do-while lowering") before cgen ever sees them;
cgen itself has no do-while, for-range or switch code of its own. A `BLOCK` and a `SCOPE_BLOCK` open a
fresh C11 `{ ... }` scope.

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 7001 | UNSUPPORTED_CONSTRUCT | A statement or an expression this apply does not generate yet (a structure, a method call, a macro call). |
| 7002 | UNSUPPORTED_TYPE | A parameter, a local or a result whose type is not one of the ten scalar kinds. |
| 7003 | UNRESOLVED_NAME | A name that is not a local, a parameter or a function of the same module. |

## Gaps

- Composites (a `structure` or an `enum`), lifecycle calls (`construct`/`destroy`), a host call and
  `fork_join` are not generated yet; a `FUNCTION_DECLARATION` that needs one is skipped with a
  diagnostic (`cgen-composites-and-parity`, the next apply).
- No parity test against the vm exists yet: "the same observable behavior in both backends" is
  checked only by this stage's own tests today, as the ir doc already notes for the vm.
- A trap is checked once per statement, not once per instruction as the vm does; once a function
  traps it still finishes evaluating the rest of the statement that trapped (with whatever value the
  trapping operation itself returned, always 0) before the next statement's guard stops it. The value
  observed after a trap is still 0/0.0/false either way, so no test distinguishes the two
  granularities, but a future apply that chains several traps in one statement should re-check this.
- A signed right shift in range relies on clang's and gcc's own implementation-defined (not
  undefined) arithmetic-shift behavior for a negative left operand; it is not computed through an
  explicit unsigned trick the way division and negation are.
- `%` (remainder) on a float operand is not generated (`UNSUPPORTED_CONSTRUCT`); the language's own
  float remainder semantics were not part of this apply's scope.
- Only a plain call of a function identified by a bare `IDENTIFIER` callee is generated; a static
  call, a method call, a built-in call (`size_of`, `align_of`, `convert`) and a macro call all raise
  `UNSUPPORTED_CONSTRUCT` today.
- The acceptance list asks for a sanitizer run where available; this apply's own verification ran
  `-std=c11 -Wall -Wextra -Werror` under every C compiler the build found, but did not additionally
  run `-fsanitize=undefined`/`-fsanitize=address` over the generated programs (see
  `temp/_LOG_APPLY_ERROR.md`).
