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
an error, in the range 4001-4005 (the codes 4002 and 4003 are no longer raised, see Diagnostics). The tree stays
valid for the lifetime of the arena.

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

Flags reuse the AST flag bits. `MAREA_AST_FLAG_MUTABLE` is kept on a `PARAMETER`, on the hidden
loop variable of a for-range loop and on some hidden and loop locals of a for-each loop (see below). A
node made by the named-node path (declarations, fields, parameters, locals) drops every flag but `MUTABLE`.

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

## Switch desugaring

`switch value case a then A case b, c then B default then C end` has no IR kind of its own. It lowers to:

```
BLOCK
  LOCAL_DECLARATION "$switch"                -- child: value
  IF_STATEMENT                               -- the first case
    BINARY_EXPRESSION op=EQUAL_EQUAL         -- children: IDENTIFIER "$switch", the lowered a
    BLOCK                                    -- the lowered A
    ELSEIF_CLAUSE                            -- each case after the first
      BINARY_EXPRESSION op=KEYWORD_OR        -- two values: the two tests joined, grouped to the left
        BINARY_EXPRESSION op=EQUAL_EQUAL     -- IDENTIFIER "$switch", the lowered b
        BINARY_EXPRESSION op=EQUAL_EQUAL     -- IDENTIFIER "$switch", the lowered c
      BLOCK                                  -- the lowered B
    ELSE_CLAUSE                              -- the default arm
      BLOCK                                  -- the lowered C
```

The scrutinee is lowered once, into `$switch`, which is set before the first test; no case evaluates it
again. The name `$switch` cannot be a source identifier, so it never clashes with a user name, and the
hidden local has no type name and no flag. An arm does not fall through, so an if chain is exact. The
tests run in source order and the first one that holds runs its block.

- A `default` arm is the `ELSE_CLAUSE`, wherever it stands among the arms. A `switch` with a `default` and
  no `case` has no test: the lowered `default` block follows the hidden local. A `switch` with no arm keeps
  only the hidden local, so its scrutinee is still evaluated.
- A `switch` with no `default` has no `ELSE_CLAUSE`, also when sema has proved that its cases cover every
  variant of an enum. Nothing in the IR says that the missing branch cannot run (see Gaps).
- The scrutinee and each case value are lowered like any expression, so a case value that is an enum
  variant stays a `MEMBER_ACCESS`. The synthetic nodes take only a position from the source: the comparison
  is positioned at its case value, the `IF_STATEMENT` or `ELSEIF_CLAUSE` at its arm.
- A `break` or `continue` in an arm targets the enclosing loop, as it does in an `if`. A `continue` that
  targets a for-range loop still gets the step in front of it, also inside an arm. A nested `switch` has its
  own `BLOCK` and its own `$switch`, so two never mix.
- A gap inside the scrutinee, a case value or an arm drops the declaration whole, as everywhere else.

## For-each desugaring

`for each key, value in collection do body end` has no IR kind of its own. The key is optional. It lowers to:

```
BLOCK
  LOCAL_DECLARATION "$each"                  -- child: collection (flags MUTABLE when written with mutable)
  LOCAL_DECLARATION "$count"                 -- child: CALL_EXPRESSION, children IDENTIFIER "$length", ARGUMENT_LIST(IDENTIFIER "$each")
  LOCAL_DECLARATION "$index" (flags MUTABLE) -- child: INTEGER_LITERAL "0"
  WHILE_STATEMENT
    BINARY_EXPRESSION op=LESS                -- children: IDENTIFIER "$index", IDENTIFIER "$count"
    BLOCK                                    -- one pass
      LOCAL_DECLARATION "key"                -- only with two names; child: IDENTIFIER "$index"
      LOCAL_DECLARATION "value"              -- child: INDEX_ACCESS(IDENTIFIER "$each", IDENTIFIER "$index")
      BLOCK                                  -- the lowered body
      ASSIGNMENT op=PLUS_EQUAL               -- the step: children IDENTIFIER "$index", INTEGER_LITERAL "1"
```

The collection is lowered once, into `$each`, and its length once, into `$count`; the loop never evaluates
either again. The names `$each`, `$count`, `$index` and `$length` cannot be source identifiers, so they never
clash with a user name. A nested for-each has its own `BLOCK` and its own hidden locals, so two never mix.
The one name of a loop with one name is the value. With two names the first is the key and the second is the
value.

- The collection is a borrow in the language, not a copy, and so is the loop variable. In a loop written with
  `mutable`, `$each` and the value local carry `MUTABLE`, as a write-borrow `PARAMETER` does. The key never
  does. The value local with `MUTABLE` is a write borrow of the element, not a mutable copy: a backend tells it
  from the loop variable of a for-range loop by its initializer, an `INDEX_ACCESS` of `$each`. `$index` carries
  `MUTABLE` because the step writes it, as the variable of a for-range loop does. No other node carries a flag.
- `$length` is a call to a hidden name that no declaration has. It stands for the number of elements of
  `$each`, and a backend resolves it from the type of the collection. No node in the IR names that type.
- The loop ends at `$index < $count`, so the step needs no leave at the bound: `$index` never passes `$count`.
  A collection with no element runs zero times.
- A `continue` that targets the loop would skip the step at the end of the pass. So each such `continue`
  becomes `BLOCK(step, CONTINUE_STATEMENT)`, with the same step as above, also inside a nested `if` or
  `switch` arm. A `break` stays as it is. A `continue` in a nested `while`, for-range or for-each loop targets
  that loop and is left alone. The body keeps its own `BLOCK`, so a local of the body never shares a scope with
  the loop variables.
- The hidden locals take their position from the `for` statement. The key and the value local take the
  position of their name. The hidden locals and the loop variables have no type name.
- A gap in the collection drops the declaration and is reported once, and the body is not lowered after it. A
  gap in the body drops the declaration whole, as everywhere else.

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

Three constructs raise an error diagnostic and make no node. The declaration that holds the gap is left
out of the module whole, never in part: a gap in a function body drops that function, a gap in a
variant default drops the enum, a gap in a constant initializer drops the constant. Lowering then
continues with the next declaration, so the other declarations stay in the module. A gap inside a
declaration is reported once: lowering of that declaration stops at the first one.

## Example sweep

`tests/source/ir/MAREAIrExamples.cpp` lowers all 23 programs under `examples/` through `lex`, `parse`,
`sema` and `ir`. Each one checks with zero errors, lowers to a `MODULE`, dumps as text and raises
exactly the codes in its row. The 15 programs not listed raise none.

| Program | Codes |
|---|---|
| `attributes/layout.marea` | 4004 |
| `basics/blocks.marea` | 4004 |
| `basics/primitive_types.marea` | 4004 |
| `interop/c_library.marea` | 4005 |
| `interop/gpu_handles.marea` | 4001, 4004 |
| `macros/builtin.marea` | 4005 |
| `parallel/scaling.marea` | 4001 |
| `types/generics.marea` | 4004 |

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 4001 | `UNSUPPORTED_FORK_JOIN` | a `fork_join` block, a `partition` or `with` statement, or a `spawn` expression |
| 4002 | `UNSUPPORTED_SWITCH` | no longer raised: a `switch` statement is lowered. The code and its name stay in the public header, so a value is never reused. |
| 4003 | `UNSUPPORTED_FOR_EACH` | no longer raised: a `for each` statement is lowered. The code and its name stay in the public header, so a value is never reused. |
| 4004 | `UNINSTANTIATED_GENERIC` | a generic function, structure or `implementation`; a call with explicit type arguments (`identity<integer32>(5)`); a call whose result type is an unbound generic; a `GENERIC_NAME` outside a call |
| 4005 | `UNSUPPORTED_MACRO` | `print!("hi");`, a module-level `static_assert!(true);`, a macro call inside an expression. `unreachable!();` as a statement is lowered and raises nothing. |

The message of 4005 and of 4004 on a declaration names the macro or the declaration in single quotes.
Each diagnostic carries the position of the construct that raised it.

## Gaps

- The three constructs above are not lowered: `fork_join` (with `partition`, `with` and `spawn`),
  generics, and every macro other than a statement `unreachable!()`. The matching
  declaration is left out of the module, and only its first gap is reported.
- A for-each loop reads its collection by position: `$each[$index]` and `$length($each)`. Sema gives no type for
  the collection, so the IR cannot tell an array or a slice from a collection that is not walked by position,
  such as a table with a key. A backend must infer the type of `$each`, `$count`, the loop variables and the
  result of `$length` from the collection, and must reject, or handle in its own way, a collection that has no
  position. With two names the key is always the position. A dedicated IR kind for the loop would let each
  backend choose how to walk the collection; it needs a public header change and was not taken.
- The hidden `$each`, `$count` and `$index` locals have no type name, like the for-range `$last`. The value local
  of a for-each loop has none either, because sema types no loop variable.
- A `switch` with no `default` lowers to an if chain with no else, also when the cases cover every variant
  of an enum. A function that ends with such a `switch` and returns in every arm has no `RETURN_STATEMENT`
  after the chain, so a backend that checks that every path returns must treat the missing branch as
  unreachable. The IR does not mark it.
- Sema accepts a `switch` with more than one `default` arm. The IR keeps the last one and drops the others,
  without a diagnostic. A defect of an earlier stage, not fixed here.
- The hidden `$switch` local has no type name, like the for-range `$last`: the IR does not copy one from the
  scrutinee. A backend must infer it from the scrutinee.
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
