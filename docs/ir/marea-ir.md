# Marea ir stage

## Contract

The `ir` stage lowers the typed tree of the `sema` stage into one IR tree. It reads a
`MAREA_SemaTypedNode` tree (from `MAREA_SemaBuildTypedAst`) and writes a tree of `MAREA_IrNode`
rooted at `MAREA_IR_KIND_MODULE`, plus diagnostics for the constructs it cannot lower yet. It has no
global state, never prints and never reads the disk: the caller gives it an arena (which owns the
whole tree) and a diagnostic sink. It is a pure lowering pass: one call per compilation unit, no
linking between modules. The stage links only `marea::sema` (and through it the earlier stages).
The CMake target is `Marea-Ir` (alias `marea::ir`), built when `MAREA_OPTION_TARGET_IR` is on. The
option is on by default and depends on `MAREA_OPTION_TARGET_SEMA`.

## Use

```c
MAREA_IrContext *ir;
const MAREA_IrNode *module;
const MAREA_SemaTypedNode *typed;

MAREA_SemaCheckCompilationUnit(sema, unit);               /* must report zero errors first */
MAREA_SemaBuildTypedAst(sema, unit, &typed);
MAREA_IrContextCreate(arena, sink, &ir);
MAREA_IrLowerCompilationUnit(ir, typed, &module);
```

`MAREA_IrLowerCompilationUnit` returns `MAREA_IR_EXCEPT_ERROR_NULL_POINTER` for a null argument or for
a typed root whose AST node is not a `COMPILATION_UNIT`, and `MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY`
when the arena cannot grow. A construct that cannot be lowered is not an error code: it is a
diagnostic in the sink, and the call still returns `MAREA_IR_EXCEPT_SUCCESS`. Every ir diagnostic is
an error, in the range 4001-4005. The tree stays valid for the lifetime of the arena.

## Node model

There is one opaque type, `MAREA_IrNode`, for every kind. A node holds a kind, a source position,
an optional text, an optional operator (`MAREA_TokenKind`), a flags word, an optional type name and
an ordered list of children. The accessors mirror `MAREAAst.h`:

| Accessor | Result |
|---|---|
| `MAREA_IrNodeGetKind` | The kind. Null returns `MAREA_IR_KIND_COUNT`. |
| `MAREA_IrNodeGetPosition` | The position, by value. Null returns a zeroed position. |
| `MAREA_IrNodeGetText`, `MAREA_IrNodeGetTextLength` | The text, not NUL terminated. Null or none gives null and 0. |
| `MAREA_IrNodeGetOperator` | The operator, or `MAREA_TOKEN_KIND_ERROR` when none. |
| `MAREA_IrNodeGetFlags` | The flags word. Null returns 0. |
| `MAREA_IrNodeGetChildCount`, `MAREA_IrNodeGetChildAt` | The children. An index out of range returns `MAREA_IR_NODE_EXCEPT_ERROR_OUT_OF_RANGE`. |
| `MAREA_IrNodeGetTypeName`, `MAREA_IrNodeGetTypeNameLength` | The type name, not NUL terminated. Null or none gives null and 0. |

`MAREA_IrKindGetName` returns the stable upper-case kind name, or `UNKNOWN` for a value out of range.

A node never owns its text or type name: both point into the source buffer of the lexer, so that
buffer must outlive the tree. The one exception is the name of an `implementation` member, which is
joined as `Type.name` and copied into the arena.

Flags reuse the AST flag bits. `MAREA_AST_FLAG_MUTABLE` is kept on a `PARAMETER` and on the hidden
loop variable of a for-range loop (see below). A node made by the named-node path (declarations,
fields, parameters, locals) drops every flag but `MUTABLE`.

The type name is the head name of the written type, with no type arguments (`array<integer32>` gives
`array`; a dotted name is kept whole). A `FUNCTION_TYPE` gives none. It is set on: `CONSTANT_DECLARATION`,
`FIELD_DECLARATION`, `VARIANT_FIELD`, `PARAMETER`, `LOCAL_DECLARATION` and `FUNCTION_DECLARATION`
(its return type), read from the AST type child; on a struct `LITERAL_VALUE`, from its type child;
and on a non-generic `CALL_EXPRESSION`, from the type name the typed tree carries for the call result,
when it has one.

## Kinds and what is erased

The kind set keeps the name and the child layout of the AST kind it comes from, except `MODULE` (the
root) and `UNREACHABLE_STATEMENT`. The kinds are: `MODULE`, `CONSTANT_DECLARATION`,
`STRUCTURE_DECLARATION`, `FIELD_DECLARATION`, `ENUM_DECLARATION`, `VARIANT_DECLARATION`,
`VARIANT_FIELD`, `FUNCTION_DECLARATION`, `PARAMETER_LIST`, `PARAMETER`, `BLOCK`, `LOCAL_DECLARATION`,
`ASSIGNMENT`, `EXPRESSION_STATEMENT`, `IF_STATEMENT`, `ELSEIF_CLAUSE`, `ELSE_CLAUSE`,
`WHILE_STATEMENT`, `RETURN_STATEMENT`, `BREAK_STATEMENT`, `CONTINUE_STATEMENT`, `SCOPE_BLOCK`,
`IDENTIFIER`, `INTEGER_LITERAL`, `FLOAT_LITERAL`, `STRING_LITERAL`, `BOOLEAN_LITERAL`,
`BINARY_EXPRESSION`, `UNARY_EXPRESSION`, `MEMBER_ACCESS`, `INDEX_ACCESS`, `CALL_EXPRESSION`,
`ARGUMENT_LIST`, `LITERAL_VALUE`, `FIELD_ENTRY`, `INDEX_ENTRY`, `VALUE_ENTRY`, `QUALIFIED_NAME` and
`UNREACHABLE_STATEMENT`.

The module keeps its declarations in source order. The typing is erased before the IR, so these make
no node:

- `namespace`, `import`, file attributes, attributes, type aliases and `contract` declarations.
- Type nodes (`NAMED_TYPE`, `FUNCTION_TYPE`) and `TYPE_ARGUMENTS`. Only the type name above survives.
- Parenthesized expressions: the inner expression is lowered in place.
- `unsafe` blocks: the plain `BLOCK` inside is lowered.
- Nodes that the parser could not build (`ERROR_NODE`).

Other shapes:

- An `opaque` declaration becomes a `STRUCTURE_DECLARATION` with zero fields.
- An `implementation` makes no node. Its functions and constants become module-level nodes named
  `Type.name` (the type name written after `implementation`), in the module where the block stands.
- A `do ... end` scope stays a `SCOPE_BLOCK` with a `BLOCK` child.
- A `FIELD_ENTRY` of a literal is named after its field and holds the lowered value as its only child.
- `unreachable!()` as a statement becomes `UNREACHABLE_STATEMENT`.
- A constant, variant field or local with an initializer holds the lowered expression as its only child.

## For-range desugaring

`for name in first .. last do body end` has no IR kind of its own. It lowers to:

```
BLOCK
  LOCAL_DECLARATION "name" (flags MUTABLE)   -- child: first
  LOCAL_DECLARATION "$last"                  -- child: last
  WHILE_STATEMENT
    BINARY_EXPRESSION op=LESS_EQUAL          -- children: IDENTIFIER name, IDENTIFIER "$last"
    BLOCK                                    -- the lowered body, then the step
      ...
      IF_STATEMENT                           -- the step, part 1: leave at the last value
        BINARY_EXPRESSION op=EQUAL_EQUAL     -- children: IDENTIFIER name, IDENTIFIER "$last"
        BLOCK
          BREAK_STATEMENT
      ASSIGNMENT op=PLUS_EQUAL               -- the step, part 2: children IDENTIFIER name, INTEGER_LITERAL "1"
```

The bound is inclusive (`<=`), because the grammar says a range includes both ends. The hidden loop
variable and `$last` have no type name. The `last` expression is lowered once, into `$last`, which is set
before the loop; the loop never evaluates it again. The name `$last` cannot be a source identifier,
so it never clashes with a user name. The hidden local carries no flag.

The step leaves the loop at the last value before it adds one. So the increment never runs past `last`,
and a loop whose `last` is the maximum of its integer type still ends. A range with `first` greater
than `last` runs zero times, because the condition fails at once.

A `continue` that targets the loop would skip the step at the end of the body. So each such
`continue` becomes `BLOCK(step, CONTINUE_STATEMENT)`, where the step is the same `IF_STATEMENT` and
`ASSIGNMENT` pair. A `continue` inside a nested `while` or for-range loop targets that loop and stays as
it is (the nested for-range loop adds its own step and its own `$last`).

## Text dump

`MAREA_IrModuleDump` writes the tree as stable text into the arena, one node per line, in the same
shape as `MAREA_AstDump`. A line is the kind name indented two spaces per depth, then the text in
double quotes when present, then `type=NAME` when the node has a type name, then `op=OPERATOR` when it
has an operator, then `flags=NUMBER` when the flags are not zero. In the text, `"` and `\` get a
backslash and a byte below 0x20 or above 0x7E is written as `\xHH`. The result is NUL terminated;
`outLength` does not count the NUL. It returns `ERROR_NULL_POINTER` for a null argument and
`ERROR_OUT_OF_MEMORY` when the arena cannot grow.

```
MODULE
  STRUCTURE_DECLARATION "Point"
    FIELD_DECLARATION "x" type=float64
  FUNCTION_DECLARATION "f" type=integer32
    PARAMETER_LIST
      PARAMETER "a" type=integer32
    BLOCK
      LOCAL_DECLARATION "b" type=integer32
        BINARY_EXPRESSION op=PLUS
          IDENTIFIER "a"
          INTEGER_LITERAL "1"
      RETURN_STATEMENT
        IDENTIFIER "b"
```

Lowering the same unit twice gives byte-identical text.

## Constructs the IR cannot lower

Five constructs raise an error diagnostic and make no node. The declaration that holds the gap is left
out of the module whole, never in part: a gap in a function body drops that function, a gap in a
variant default drops the enum, a gap in a constant initializer drops the constant. Lowering then
continues with the next declaration, so the other declarations stay in the module. A gap inside a
declaration is reported once: lowering of that declaration stops at the first one.

## Example sweep

`tests/source/ir/MAREAIrExamples.cpp` lowers all 23 programs under `examples/` through `lex`, `parse`,
`sema` and `ir`. Each one checks with zero errors, lowers to a `MODULE`, dumps as text and raises
exactly the codes in its row. The 13 programs not listed raise none.

| Program | Codes |
|---|---|
| `attributes/layout.marea` | 4004 |
| `basics/blocks.marea` | 4004 |
| `basics/control_flow.marea` | 4002, 4003 |
| `basics/primitive_types.marea` | 4004 |
| `interop/c_library.marea` | 4005 |
| `interop/gpu_handles.marea` | 4001, 4004 |
| `macros/builtin.marea` | 4005 |
| `parallel/scaling.marea` | 4001, 4003 |
| `types/enum_methods.marea` | 4002 |
| `types/generics.marea` | 4004 |

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 4001 | `UNSUPPORTED_FORK_JOIN` | a `fork_join` block, a `partition` or `with` statement, or a `spawn` expression |
| 4002 | `UNSUPPORTED_SWITCH` | a `switch` statement |
| 4003 | `UNSUPPORTED_FOR_EACH` | `for each value in values do end` |
| 4004 | `UNINSTANTIATED_GENERIC` | a generic function, structure or `implementation`; a call with explicit type arguments (`identity<integer32>(5)`); a call whose result type is an unbound generic; a `GENERIC_NAME` outside a call |
| 4005 | `UNSUPPORTED_MACRO` | `print!("hi");`, a module-level `static_assert!(true);`, a macro call inside an expression. `unreachable!();` as a statement is lowered and raises nothing. |

The message of 4005 and of 4004 on a declaration names the macro or the declaration in single quotes.
Each diagnostic carries the position of the construct that raised it.

## Gaps

- The five constructs above are not lowered: `fork_join` (with `partition`, `with` and `spawn`),
  `switch`, `for each`, generics, and every macro other than a statement `unreachable!()`. The matching
  declaration is left out of the module, and only its first gap is reported.
- A call to a function that was left out of the module still lowers to a plain name that points at
  nothing.
- Built-ins with explicit type arguments (`size_of<T>()`, `align_of<T>()`, `convert<T>(x)`) and a
  type-qualified static call (`array<integer32>.filled(8, 0)`) are not user generics, but raise 4004,
  because the IR erases type arguments and a call that keeps only its name would be wrong. They need an
  IR form or a diagnostic code of their own.
- The hidden loop variable of a for-range loop has no type name (sema gives none for a range bound). A
  backend must infer it from the first bound.
- `LOCAL_DECLARATION` never carries the `MUTABLE` flag, except the hidden loop variable: the AST has no
  write-target flag for a local.
- The sema typed tree gives a type name only for a call result, a struct literal and a few expression
  shapes, so the type name of a declaration is the head name written in the source. A type argument,
  and any `FUNCTION_TYPE`, is lost.
- There is no linking between modules: an imported name stays a plain name.
- The tree builder and the dump are recursive. A very deep tree needs a large stack; no test goes
  deeper than the example programs.
- `MAREA_IrLowerCompilationUnit` reports a root that is not a `COMPILATION_UNIT` with
  `MAREA_IR_EXCEPT_ERROR_NULL_POINTER`, because the except set has no other fitting value.
