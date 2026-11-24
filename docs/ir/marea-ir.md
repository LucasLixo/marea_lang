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
an error, and only three codes exist, 4004, 4005 and 4006 (4001 to 4003 are retired, see Diagnostics). The tree stays
valid for the lifetime of the arena.

## Node model

There is one opaque type, `MAREA_IrNode`, for every kind. A node holds a kind, a source position,
an optional text, an optional operator (`MAREA_TokenKind`), a flags word, an optional type name, an optional
resolved type and an ordered list of children. The accessors mirror `MAREAAst.h`:

| Accessor | Result |
|---|---|
| `MAREA_IrNodeGetKind` | The kind. Null returns `MAREA_IR_KIND_COUNT`. |
| `MAREA_IrNodeGetPosition` | The position, by value. Null returns a zeroed position. |
| `MAREA_IrNodeGetText`, `MAREA_IrNodeGetTextLength` | The text, not NUL terminated. Null or none gives null and 0. |
| `MAREA_IrNodeGetOperator` | The operator, or `MAREA_TOKEN_KIND_ERROR` when none. |
| `MAREA_IrNodeGetFlags` | The flags word. Null returns 0. |
| `MAREA_IrNodeGetChildCount`, `MAREA_IrNodeGetChildAt` | The children. An index out of range returns `MAREA_IR_NODE_EXCEPT_ERROR_OUT_OF_RANGE`. |
| `MAREA_IrNodeGetTypeName`, `MAREA_IrNodeGetTypeNameLength` | The type name, not NUL terminated. Null or none gives null and 0. |
| `MAREA_IrNodeGetResolvedType`, `MAREA_IrNodeGetResolvedTypeLength` | The full resolved type, not NUL terminated. Null or none gives null and 0. |

`MAREA_IrKindGetName` returns the stable upper-case kind name, or `UNKNOWN` for a value out of range.

The type name is the head of the written type and stays as it was: `array` for `array<sinteger32,4>`. The resolved
type is the full canonical text that `MAREA_SemaTypedNodeGetResolvedType` gives (see the sema doc, "Resolved type"),
with its type arguments (`array<sinteger32,4>`), so a later stage never has to guess a width or a kind. It is a copy in
the arena of the ir context, made at node creation. Two groups of node carry it: every expression node that comes
from the source (`IDENTIFIER`, the five literals, `BINARY_EXPRESSION`, `UNARY_EXPRESSION`, `MEMBER_ACCESS`,
`INDEX_ACCESS`, `CALL_EXPRESSION` or the `CONSTRUCT_CALL` it became, `LITERAL_VALUE`) and every `PARAMETER`,
`LOCAL_DECLARATION`, `FIELD_DECLARATION`, `VARIANT_FIELD`, `CONSTANT_DECLARATION` and `FUNCTION_DECLARATION` with a
written type. A name that only qualifies a member (`Planet` in `Planet.Earth`) has none and is not a gap. The nodes
that the ir invents carry one too: the hidden locals
`$lastN` (the type of the last bound), `$switch` (the type of the scrutinee) and `$return` (the type of the returned
value) have the type they hold, and every `IDENTIFIER` that reads one shares it; the loop variable of a for-range loop
has the type sema gave its name (else the type of the first value), and the step literal `1` its type; a
`FOR_EACH_STATEMENT` has the type sema gave its value name, and its key the type sema gave it (see For-each
lowering); the tests the desugaring builds (`<=`, `==` and `or`) are `boolean`. The `ASSIGNMENT` of the step and every `DESTROY_CALL` stay
untyped: they are statements, not values.

Some expressions stay untyped on purpose, because sema leaves them untyped (the closed list in the sema doc, "Gaps").
They stay in the tree with no resolved type and raise nothing: an `IDENTIFIER` (a loop name, a function or built-in
name), the callee of a call, a `CALL_EXPRESSION` whose callee has no type (a call of an `EXTERNAL` function with no
signature), an operation with an untyped operand, and a `spawn` of a call typed `void`. Any other expression that has
no type raises 4006 (see Diagnostics).

A node never owns its text or type name: both point into the source buffer of the lexer, so that
buffer must outlive the tree. The one exception is the name of an `implementation` member, which is
joined as `Type.name` and copied into the arena.

Flags reuse the AST flag bits. `MAREA_AST_FLAG_MUTABLE` is kept on a `PARAMETER`, on a `PARTITION_STATEMENT`,
on a `FOR_EACH_STATEMENT` and on the hidden loop variable of a for-range loop. A
node made by the named-node path (declarations, fields, parameters, locals) drops every flag but `MUTABLE`.
A `PARAMETER` no longer carries `MUTABLE` on its own raw AST node (the retired `mutable`/`reference`
parameter keywords used to set it there); it is set on the `PARAMETER` IR node instead from the
ownership mode sema already computed for it (`MAREA_SemaTypedNodeGetOwnershipMode`), which is
`MUTABLE_BORROW` exactly when the parameter's effective `semantics` attribute is
`reference_mutable` written on the parameter; a parameter with no attribute is a read-only borrow and leaves the flag clear. The other modes
(`VALUE`, `BORROW`) both leave the flag clear, so the ir does not tell a by-value parameter from a read-only borrow:
a backend reads that from the type, and there is no other flag for it (see Gaps).

The type name is the head name of the written type, with no type arguments (`array<sinteger32>` gives
`array`; a dotted name is kept whole). A `FUNCTION_TYPE` gives none. It is set on: `CONSTANT_DECLARATION`,
`FIELD_DECLARATION`, `VARIANT_FIELD`, `PARAMETER`, `LOCAL_DECLARATION` and `FUNCTION_DECLARATION`
(its return type), read from the AST type child; on a struct `LITERAL_VALUE`, from its type child;
and on a non-generic `CALL_EXPRESSION`, from the type name the typed tree carries for the call result,
when it has one. A `CALL_EXPRESSION` of a built-in or of a static call on a written type (see Type operands)
follows the same rule, and a `TYPE_NAME` or a `BUILTIN_NAME` has no type name: it holds its name as text.
The loop variable and `$lastN` of a for-range loop, a `FOR_EACH_STATEMENT` (its value) and the key of a for-each loop
have no written type, so their type name is the head of their resolved type, the text before the first type
argument (see For-range desugaring and For-each lowering); a function type has none. The other hidden locals
(`$switch`, `$return`) keep no type name.
An `ENUM_DECLARATION` also carries a type name, its tag type: the written tag when the source wrote one
(`enum Name : uinteger8 ... end` gives `uinteger8`), or the literal `sinteger32` when it wrote none, the
default the language spec sets for a tag with no written type.

## Kinds and what is erased

The kind set keeps the name and the child layout of the AST kind it comes from, except `MODULE` (the
root), `UNREACHABLE_STATEMENT`, `TYPE_NAME` and `BUILTIN_NAME`, and the name of `PARTITION_STATEMENT`, `WITH_STATEMENT`
and `FOR_EACH_STATEMENT`, which is the text of the node (see Fork-join lowering, For-each lowering and Type
operands). The kinds are: `MODULE`, `CONSTANT_DECLARATION`,
`STRUCTURE_DECLARATION`, `FIELD_DECLARATION`, `ENUM_DECLARATION`, `VARIANT_DECLARATION`,
`VARIANT_FIELD`, `FUNCTION_DECLARATION`, `PARAMETER_LIST`, `PARAMETER`, `BLOCK`, `LOCAL_DECLARATION`,
`ASSIGNMENT`, `EXPRESSION_STATEMENT`, `IF_STATEMENT`, `ELSEIF_CLAUSE`, `ELSE_CLAUSE`,
`WHILE_STATEMENT`, `RETURN_STATEMENT`, `BREAK_STATEMENT`, `CONTINUE_STATEMENT`, `SCOPE_BLOCK`,
`IDENTIFIER`, `INTEGER_LITERAL`, `FLOAT_LITERAL`, `STRING_LITERAL`, `BOOLEAN_LITERAL`,
`BINARY_EXPRESSION`, `UNARY_EXPRESSION`, `MEMBER_ACCESS`, `INDEX_ACCESS`, `CALL_EXPRESSION`,
`ARGUMENT_LIST`, `LITERAL_VALUE`, `FIELD_ENTRY`, `INDEX_ENTRY`, `VALUE_ENTRY`, `QUALIFIED_NAME`,
`UNREACHABLE_STATEMENT`, `FORK_JOIN_BLOCK`, `PARTITION_STATEMENT`, `WITH_STATEMENT`, `FOR_EACH_STATEMENT`,
`TYPE_NAME`, `BUILTIN_NAME`, `CHARACTER_LITERAL` (the text is the literal as written, with its single quotes),
`CONSTRUCT_CALL` and `DESTROY_CALL` (see Construct and destroy lowering). The kinds from `UNREACHABLE_STATEMENT` on come
after every older kind, so no value of an older kind moved. One value is retired: the one right after `CHARACTER_LITERAL`, which was `ZERO_VALUE`
(see Zero-default creation). It is not an enumerator any more, and no value moved to fill it: `CONSTRUCT_CALL` is
declared as `MAREA_IR_KIND_CHARACTER_LITERAL + 2`, `DESTROY_CALL` follows it, and `MAREA_IR_KIND_COUNT` is unchanged.
`MAREA_IrKindGetName` gives `UNKNOWN` for the retired value, as for a value out of range, so a loop over the values
below `MAREA_IR_KIND_COUNT` meets one value that names no kind.

The module keeps its declarations in source order. The typing is erased before the IR, so these make
no node:

- `namespace`, `import`, file attributes, attributes, type aliases and `contract` declarations. Their aliases are
  lowered into the names that remain (see Aliases).
- Type nodes (`NAMED_TYPE`, `FUNCTION_TYPE`) and `TYPE_ARGUMENTS`. Only the type name above survives, with one
  exception: a type that is the operand of a call keeps its name and its type arguments as a `TYPE_NAME` (see Type
  operands).
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
- A constant, variant field or local with an initializer holds the lowered expression as its only child. A
  `CONSTANT_DECLARATION`, a `VARIANT_FIELD` and a `LOCAL_DECLARATION` with no initializer keep the no-child shape.
  Sema rejects a local with no initializer (3065), so the ir sees that shape only from an AST sema refused, and it
  adds no value for it (see Zero-default creation).
- A `PARAMETER` with a default value holds the lowered default as its only child, the same shape a
  constant or a variant field uses for its own initializer. A `PARAMETER` with no default keeps the no-child
  shape: it is a required argument, not a zero (see Zero-default creation).
- A `CALL_EXPRESSION` whose callee is the bare name of a function of the same unit fills every
  parameter the call omitted: the `ARGUMENT_LIST` holds the lowered written arguments first, then, for
  each trailing parameter the call did not supply, the lowered default read fresh from that parameter in
  the callee's own typed parameter list. The ir materializes every default, so a backend never looks at
  the callee's declaration to find one (see Gaps for what callee shape this does not cover). A parameter
  or a local of the enclosing function that hides the module name (`MAREA_IrFunctionDeclares`, the same
  check an import alias uses) is never matched to the module function, so a call of the local fills
  nothing.

## Zero-default creation

The language spec's zero-default rule (decided 2026-11-22: no value or field is left unset at creation) covers six
creation paths in all: a literal with an omitted field, a `local` with no initializer, `Type(args)` before
`construct`, `array<T, N>` elements, `owned<T>` storage and a `convert` result that exposes padding. The ir lowers
none of them to a zero node, and it has no such kind. It had one, `ZERO_VALUE`, for the first two. It is removed
(the value it held is retired, see Kinds and what is erased): sema now rejects both paths, so no accepted program
reaches them.

- **A `local` with no initializer** is rejected by sema (`LOCAL_WITHOUT_INITIAL_VALUE`, 3065).
- **A struct literal that omits a field** is rejected by sema (`MISSING_FIELD`, 3077, for `Point{ x = 1 }` and
  `Point{}`, reported at the first missing field). `MAREA_SemaBuildTypedAst`, the only input of the ir, refuses a
  tree with a sema error (`ERROR_NOT_CHECKED`), so the ir never sees such a literal, and a `LITERAL_VALUE` holds
  exactly the `FIELD_ENTRY` items the source wrote. A test checks it.
- The rule stops there. A `PARAMETER` with no default, a `CONSTANT_DECLARATION` (the grammar always requires its
  initializer), a `VARIANT_FIELD` with no written value and a `LOCAL_DECLARATION` with no initializer (sema rejects
  it) all keep their own no-child shape: a missing parameter default is a required argument, and a missing variant
  field default is a required argument of the variant, neither one a zero.
- **The other four paths are a gap**, recorded in Gaps: `Type(args)` before `construct`, `array<T, N>` elements,
  `owned<T>` storage and a `convert` result that exposes padding. They need `cgen` or `vm` to exist first, and a
  backend zero-fills them from the type. `cgen` is expected to emit `= {0}` for a type of constant size, or a
  `memset` otherwise; the `vm` is expected to zero the slot on allocation. A parity test is expected to read the
  zeroed value on both backends and expect 0. None of this exists yet: the ir places no marker for it, and a
  `CONSTRUCT_CALL` or a `LOCAL_DECLARATION` carries the type name and the resolved type a backend sizes the zero
  from.

## Construct and destroy lowering

The language spec makes construction and destruction automatic. The ir makes both explicit, so a backend never
looks for a `construct` or a `destroy` by itself. Two kinds, appended after the retired value that `ZERO_VALUE` held, carry them. A new kind,
not a flag on `CALL_EXPRESSION`, because a flag would print in the dump as a raw bit mask.

- **`CONSTRUCT_CALL`** is `Type(args)` and, when the type has a `construct`, `Type{...}`. Its type name is the written name of the structure or enum that is
  constructed. Its one child is an `ARGUMENT_LIST`: the written arguments first, then the lowered default of each
  trailing parameter the call omitted, read from the parameter list of the type's own `construct` operator, the way
  an ordinary call fills its defaults. A type with no `construct` gets an empty `ARGUMENT_LIST`. The ir finds the
  structure or enum by name among the declarations of the same unit; a `Type(args)` of a type it cannot find there
  stays an ordinary `CALL_EXPRESSION` (see Gaps). The ir does not place a zero fill in front of the call: the
  language spec zeroes the value before `construct` runs, and that stays the backend's job, read from the type name
  (see Zero-default creation). A local or a parameter
  of the enclosing function that hides the type name stops the match, as for an ordinary call. `Point(1)` with
  `construct(x: sinteger32 = 0, y: sinteger32 = 0)` lowers to `(CONSTRUCT_CALL :Point (ARGUMENT_LIST (INTEGER_LITERAL 1)
  (INTEGER_LITERAL 0)))`.

  `Type{...}` sets the fields (named, in order, or none) and then runs `construct` with its defaults. A literal of a
  non-generic structure of the same unit that has a `construct` operator lowers to a `CONSTRUCT_CALL` with two
  children: the `ARGUMENT_LIST` of the defaults of `construct` (no argument is written, so every parameter is
  filled), then the `LITERAL_VALUE` itself, unchanged (sema requires every field to be named), as the initial value the
  backend runs `construct` on once the fields are set. `Point{ x = 1, y = 2 }` with
  `construct(x: sinteger32 = 7, y: sinteger32 = 8)` lowers to `(CONSTRUCT_CALL :Point (ARGUMENT_LIST (INTEGER_LITERAL
  7) (INTEGER_LITERAL 8)) (LITERAL_VALUE :Point (FIELD_ENTRY x (INTEGER_LITERAL 1)) (FIELD_ENTRY y (INTEGER_LITERAL 2))))`.
  A literal of a type with no `construct`, or of a type the unit does not declare, stays a plain `LITERAL_VALUE`. So
  the optional second child is the only difference between the two shapes: `Type(args)` has one child and the
  backend zero-fills; `Type{...}` has two and the fields come from the literal.
- **`DESTROY_CALL`** is the cleanup of one local. Its text is the qualified destroy function (`Box.drop`), and its
  one child is an `IDENTIFIER` that names the local, for a backend to bind as the receiver. The node has no other
  argument and no result, because sema rejects a `destroy` that takes a parameter or returns a value
  (`LIFECYCLE_OPERATOR_BAD_SHAPE`, 3029) and a direct call of `construct` or `destroy`
  (`LIFECYCLE_OPERATOR_CALLED_DIRECTLY`, 3027). The ir does not check either again, so these two kinds are the only
  place where a lifecycle function is called.

A local is destructible when the type name it was declared with is the type of an `implementation` that holds a
function with `#[operator = "destroy"]`. Every `BLOCK` that declares such a local directly (a function body, a
loop body, a branch, an `if`, a `do ... end` scope) gets the destroys of its own locals, and only of its own:

- **Order.** Reverse declaration order: the local declared last is destroyed first. A nested block destroys its own
  locals before the block that holds it does.
- **Fall through.** The destroys are appended at the end of the block, unless the block already ends in a `return`,
  `break` or `continue` (a jump leaves them in front of the jump instead). A loop body therefore destroys its
  locals once per iteration.
- **Early exit.** A `return`, `break` or `continue` that leaves the block is replaced by `BLOCK(destroy..., jump)`,
  with a fresh `DESTROY_CALL` set for each exit (a node is never shared between two places). A `return` leaves every
  enclosing block, so it also leaves the inner blocks of a nested loop; a `break` or `continue` is wrapped only up to
  the nearest `WHILE_STATEMENT` or `FOR_EACH_STATEMENT`, which it targets, so it never destroys a local of a block
  outside its own loop. The inner block wraps first, so the destroys of the inner locals run before those of the
  outer ones. For a for-range `continue` the destroys stand in front of the step.
- **Value of a return.** A `return` with a value is wrapped as `BLOCK(LOCAL_DECLARATION $return (value), destroy...,
  RETURN_STATEMENT(IDENTIFIER $return))`, so the value is read before any destroy runs and `return b.size;` never
  reads a destroyed `b`. The hidden local `$return` has no type name and no flag, like `$switch`, and its resolved type
  is the resolved type of the value it holds. A `return` through two destructible scopes hoists once: the outer wrap
  sees the value `IDENTIFIER $return` and does not declare it again, so the outer destroys stand between the inner
  destroys and the `return`. A `return` with no value, a `break` and a `continue` are wrapped as before, with no
  hidden local.
- **Moved-from locals.** A local that `move x` names anywhere in its own block is never destroyed at any exit of that
  block. The check is syntactic: the language has no lifetime analysis, so a move on one branch still skips the
  destroy on every branch (see Gaps).

```
local a: Box = Box(); local b: Box = Box();
(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST)))
       (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST)))
       (DESTROY_CALL Box.drop (IDENTIFIER b)) (DESTROY_CALL Box.drop (IDENTIFIER a)))
```

```
local a: Box = Box(); return a.x;
(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST)))
       (BLOCK (LOCAL_DECLARATION $return (MEMBER_ACCESS (IDENTIFIER a) (IDENTIFIER x)))
              (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT (IDENTIFIER $return))))
```

## For-range desugaring

`for name in first .. last do body end` has no IR kind of its own. It lowers to:

```
BLOCK
  LOCAL_DECLARATION "name" (flags MUTABLE)   -- child: first
  LOCAL_DECLARATION "$last0"                 -- child: last
  WHILE_STATEMENT
    BINARY_EXPRESSION op=LESS_EQUAL          -- children: IDENTIFIER name, IDENTIFIER "$last0"
    BLOCK                                    -- the lowered body, then the step
      ...
      IF_STATEMENT                           -- the step, part 1: leave at the last value
        BINARY_EXPRESSION op=EQUAL_EQUAL     -- children: IDENTIFIER name, IDENTIFIER "$last0"
        BLOCK
          BREAK_STATEMENT
      ASSIGNMENT op=PLUS_EQUAL               -- the step, part 2: children IDENTIFIER name, INTEGER_LITERAL "1"
```

The bound is inclusive (`<=`), because the grammar says a range includes both ends. The loop variable and
`$last0` carry a type name and a resolved type, as a local of the source does: `$last0` the type of `last`, the
variable the type sema gave its name (else the type of `first`). The resolved type is read with
`MAREA_SemaTypedNodeGetResolvedType` on child 0 of the typed `FOR_RANGE_STATEMENT` (the counter; the bounds are
children 1 and 2), and the type name is its head, the text before the first type argument, so a bound of type
`uinteger8` gives `type=uinteger8 resolved=uinteger8`. Each read of the two, the step literal `1` and the two tests
are typed too (the reads and the literal have a resolved type only, the tests are `boolean`). When sema gives the
counter no type, the variable takes the type of `first`, and a bound with no type leaves its local with neither,
never a guess. The `last` expression is lowered once, into `$last0`, which is set
before the loop; the loop never evaluates it again. The name `$last0` cannot be a source identifier,
so it never clashes with a user name. The hidden local carries no flag.

The name is `$last` and the number of the loop in its function, counted in source order from zero, the outer loop
before the loops in its body (`$last0`, `$last1`, ...). So two for-range loops of one function never share the
name of their hidden bound, whether one stands in the other or they follow each other, and a backend that keys the
locals of a function by name alone keeps them apart. The count starts again at zero in each function, so a function
lowers the same wherever it stands in the unit. The step of a loop, also the one a `continue` gets, reads the bound of
its own loop. Only the hidden bound of a for-range loop is numbered: `$switch` and `$return` are not (see Gaps).

The step leaves the loop at the last value before it adds one. So the increment never runs past `last`,
and a loop whose `last` is the maximum of its integer type still ends. A range with `first` greater
than `last` runs zero times, because the condition fails at once.

A `continue` that targets the loop would skip the step at the end of the body. So each such
`continue` becomes `BLOCK(step, CONTINUE_STATEMENT)`, where the step is the same `IF_STATEMENT` and
`ASSIGNMENT` pair. A `continue` inside a nested `while`, `FOR_EACH_STATEMENT` or for-range loop targets that
loop and stays as it is (the nested for-range loop adds its own step and its own `$lastN`).

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
hidden local has no type name and no flag, but its resolved type is that of the scrutinee, shared by every read of
it; each case test is `boolean`. An arm does not fall through, so an if chain is exact. The
tests run in source order and the first one that holds runs its block.

- A `default` arm is the `ELSE_CLAUSE`, wherever it stands among the arms. A `switch` with a `default` and
  no `case` has no test: the lowered `default` block follows the hidden local. A `switch` with no arm keeps
  only the hidden local, so its scrutinee is still evaluated.
- **A `switch` with no `default` (decided).** The missing else is explicit only when the `switch` is proven
  exhaustive, that is, over an enum (a type alias of the enum counts, as sema sees through it). It ends with an
  `ELSE_CLAUSE` that holds a `BLOCK` with one `UNREACHABLE_STATEMENT`, the node that `unreachable!()` lowers to.
  Sema reports `NON_EXHAUSTIVE_SWITCH` for such a `switch` that misses a variant, so a unit that reached the IR
  covers them all, and the IR does not count the variants again. A backend that checks that every path of a
  function with a result type returns reads the chain like any `if`: each arm must end the path, and the
  `UNREACHABLE_STATEMENT` ends the else, as `unreachable!()` ends a path in sema. A `switch` over any other type
  (an integer, a `boolean`, a type of an `EXTERNAL` library that sema does not know) with no `default` has no
  `ELSE_CLAUSE`: it can fall through when no case matches. Sema does not take an integer or `boolean` switch for an
  ending path (it accepts only a `default` or an enum; for a scrutinee of a type it does not know it gives the
  benefit of the doubt, which is no proof). An explicit `UNREACHABLE_STATEMENT` there would turn a legal
  fall-through into a trap, and would let a check of the returns pass where sema's rule fails. The absence of the
  else is the explicit form of that path, and a backend reads it as a path that continues after the `switch`.
- The scrutinee and each case value are lowered like any expression, so a case value that is an enum
  variant stays a `MEMBER_ACCESS`. The synthetic nodes take only a position from the source: the comparison
  is positioned at its case value, the `IF_STATEMENT` or `ELSEIF_CLAUSE` at its arm.
- A `break` or `continue` in an arm targets the enclosing loop, as it does in an `if`. A `continue` that
  targets a for-range loop still gets the step in front of it, also inside an arm. A nested `switch` has its
  own `BLOCK` and its own `$switch`, so two never mix.
- A gap inside the scrutinee, a case value or an arm drops the declaration whole, as everywhere else.

## For-each lowering

`for each key, value in collection do body end` keeps its shape in the IR, in one kind, `FOR_EACH_STATEMENT`. The
key is optional. It lowers to:

```
FOR_EACH_STATEMENT "value" (flags MUTABLE)   -- the text is the name of the value variable
  IDENTIFIER "key"                           -- only with two names, always the first child
  <collection>                               -- the lowered collection expression
  BLOCK                                      -- the lowered body, always the last child
```

A loop with one name has two children and a loop with two names has three, so a backend reads the body as the
last child, the collection as the one before it and the key as an optional first child. The one name of a loop
with one name is the value. With two names the first is the key and the second is the value.

- The node says that every element of the collection is visited once, in the order the collection defines. How
  a backend walks it, and what the key is (a position for an array or a slice, a key for a table), follows the
  type of the collection, which the backend knows and the IR does not name. So the IR holds no hidden local, no
  hidden call and no step.
- The collection is evaluated once, by the node, before the first pass. It is a borrow in the language, not a
  copy, and so is the value variable.
- The flag `MUTABLE` is kept on the node when the source wrote `mutable`: the body then has write access to
  each element. It is the one flag the node keeps. No local carries it for the loop, so on a
  `LOCAL_DECLARATION` it only marks the hidden loop variable of a for-range loop.
- The names are the text of the node (the value) and the text of the `IDENTIFIER` child (the key), not locals,
  because they declare names and do not read them; the key is a child because a node has one text.
- **Types of the two names.** Sema gives both names a type (`MAREA_SemaTypedNodeGetResolvedType`, see the sema doc,
  "Loop names and collection"): the value is the element type of the collection, and the key, when written, is
  `uinteger64`. The `FOR_EACH_STATEMENT` carries the type of its value, as a resolved type and as a type name, the
  same way a `LOCAL_DECLARATION` carries the type of its name. The key child carries the type of the key the same
  way. The type name is the head of the resolved type, the text before the first type argument (a dotted name is
  kept whole), and the resolved type keeps the type arguments: for `rows: array<array<sinteger32,4>,2>` the value is
  `type=array resolved=array<sinteger32,4>`. A function type as the element type has a resolved type and no type name,
  as a written function type has none. When sema gives a name no type (a collection of a library it does not know,
  such as `list`), the node or the key has neither, never a guess. The collection child is a lowered expression, so it
  has its own resolved type (`array<float32,4>`), and a backend reads the element type from the value instead of
  inferring it from the collection.
- **No hidden locals.** The IR adds no `$each`, `$count` or `$index` local for the walk, and no step: the node is
  the loop, and the types above are what a backend needs to give the value and the key a slot. A desugaring to
  hidden locals would fix one way to walk the collection in the IR, and the way depends on the type of the
  collection (a position for an array, a key for a table), so it stays a decision of the backend (see Gaps).
- A `continue` or `break` in the body targets the `FOR_EACH_STATEMENT`, so it stays as it is. The node is a
  loop, so a `continue` of an enclosing for-range loop still gets its step, and the walk that puts the step
  there stops at a nested `FOR_EACH_STATEMENT`, as it stops at a nested `while`.
- The node takes its position from the `for` statement and the key takes the position of its name.
- A gap in the collection drops the declaration and is reported once, and the body is not lowered after it. A
  gap in the body drops the declaration whole, as everywhere else.

## Fork-join lowering

`fork_join`, `partition`, `with` and `spawn` keep their shape in the IR. Three kinds hold what no existing
kind can say: where the join is, that the parts are disjoint, and where a scoped value closes. The IR has no
thread model, no runtime call and no scheduler. Each node says what must hold, and each backend chooses how.

```
FORK_JOIN_BLOCK                              -- one child: BLOCK
PARTITION_STATEMENT "part" (flags MUTABLE)   -- children: the value, the part count, BLOCK
WITH_STATEMENT "name"                        -- children: the value, BLOCK
UNARY_EXPRESSION op=KEYWORD_SPAWN            -- child: the call; the one form of spawn, no kind of its own
```

- `FORK_JOIN_BLOCK` is the join point. Every worker that a `spawn` starts inside the block ends before the node
  ends. A task kept in a local that the function declared before the block is read only after the node.
- `PARTITION_STATEMENT` cuts the value in disjoint parts, as many as the second child says, and runs the body
  once for each part. The text of the node is the name of the part, which the body reads as an `IDENTIFIER`. The
  flag `MUTABLE` is kept when the source wrote `mutable`: each body then has exclusive write access to its
  part. The count is an expression, lowered like any other, so a constant stays an `IDENTIFIER`. The IR does not
  say how many threads run, or in which order the parts run.
- `WITH_STATEMENT` opens the value before the body and closes it at the end of the body. The text of the node
  is the name that the body reads. The name cannot leave the block, which sema checks.
- `spawn` is a `UNARY_EXPRESSION` with the operator `KEYWORD_SPAWN`, as `copy` and `move` keep their operator.
  It has no kind of its own, because the operator already tells it from every other unary form. It starts one
  worker and gives a `task<T>`, so it may be a statement, the initializer of a local or an argument of a call.
  Sema accepts a spawn only inside a `fork_join`; the IR does not check it again.
- The name is the text of the node, not an `IDENTIFIER` child, because it declares a name and does not read
  one; a `LOCAL_DECLARATION` and a `PARAMETER` work the same way. So a backend never mistakes the binder for the
  value. The part name and the `with` name have no type name, because sema types neither.
- A `continue` or `break` inside the body of a `with` that targets an enclosing loop is lowered as in any other
  block, and a `continue` that targets a for-range loop still gets the step in front of it.
- A gap inside any child drops the declaration whole and is reported once, as everywhere else.

## Aliases

The IR lowers every alias to the original name, so a backend sees no alias. `MAREA_IrLowerCompilationUnit` builds
one alias table from the unit before it lowers a declaration:

- A whole-path alias (`import a.b.c as d;`) stands for the last segment of its path (`c`).
- A member alias (`import a.b { f as g, Color.red as Crimson };`) stands for the member path as written (`f`,
  `Color.red`). A two-part path is joined with a dot in the arena. A member without `as` is not an alias.
- A local type alias without generic parameters (`type Name = Other;`, `type Name = Other<sinteger32>;`) stands for
  the written head type name; the type arguments are not kept, as on any written type. An alias of a function type
  (`type Callback = function(v: sinteger32): boolean;`) lowers to no type name, as the written function type does.
  A chain of type aliases is followed up to 8 steps; an import alias ends a chain. A generic alias is not in the
  table and keeps raising 4004. A `TYPE_NAME` operand (a built-in or static call argument) named by an alias that
  carries type arguments or names a function type raises 4004, because the lowered name would lose them.

The rules:

- An `IDENTIFIER` that is read as a value (a call, a constant, a variant, a name in an expression) and is an import
  alias lowers to an `IDENTIFIER` with the original text. The head of a member chain or of a `QUALIFIED_NAME`
  expression is such a name (`lst.push` becomes `list.push`). The name after the dot, a field entry name and a
  declaration name are never rewritten.
- A written type name is the original in every place the IR keeps one: the type name of a field, a parameter, a
  local, a constant, a function, a struct literal, a call result, the `TYPE_NAME` operand of a built-in or of a
  static call, and the qualifier of the methods of an `implementation`. The head of a dotted type name that is a
  namespace alias is replaced (`lst.Item` becomes `list.Item`).
- A parameter or a local of the function that is lowered, with the name of an alias, hides the alias in that
  function, from its own declaration onward: a reference before the declaration still lowers to the original
  (see Gaps).
- The built-in lookup (`size_of`, `align_of`, `convert`) counts the local names of a member list: an alias or a
  member with the built-in name binds it, the original of an alias does not.
- `import` and `type` declarations still make no node, so a dump holds no alias text.

## Type operands

Two forms of call have a type as an operand, and the IR cannot erase it: `size_of<T>()` has no other input, and
`array<sinteger32>.filled(8, 0)` names the type whose function it calls. The IR holds the written type, as a name and
a tree of its type arguments, and nothing else. It adds no type system, no check and no runtime call: it does not
look up the type, does not compute a size and does not choose a conversion. Each backend does that.

```
CALL_EXPRESSION                              -- a built-in call: size_of<Light>(), convert<sinteger64>(count)
  BUILTIN_NAME "convert"                     -- the text is the name; the children are the type arguments
    TYPE_NAME "sinteger64"
  ARGUMENT_LIST
    IDENTIFIER "count"

CALL_EXPRESSION                              -- a static call on a written type: array<float32, 16>.filled(0.0)
  MEMBER_ACCESS
    TYPE_NAME "array"                        -- the text is the name; the children are the type arguments
      TYPE_NAME "float32"
      INTEGER_LITERAL "16"
    IDENTIFIER "filled"
  ARGUMENT_LIST
    FLOAT_LITERAL "0.0"
```

- `TYPE_NAME` is a written type. Its text is the name, kept whole when dotted (`collections.array`), as a type
  name is elsewhere. Its children are its type arguments, in the order written. Each is a `TYPE_NAME` or, for a
  value parameter such as the `16` of `array<float32, 16>`, an `INTEGER_LITERAL`. A type with no argument has no
  child. The node has no type name and no flag.
- `BUILTIN_NAME` is the callee of a built-in call. Its text is the name and its children are the type arguments,
  each a `TYPE_NAME` or an `INTEGER_LITERAL`. The IR knows three built-ins: `size_of`, `align_of` and `convert`.
  The IR holds no result type for it: a backend knows the built-in by its name.
- Both forms keep the layout of every call: the callee first, the `ARGUMENT_LIST` last. A backend finds a built-in
  call by a `BUILTIN_NAME` callee, and a static call by a `MEMBER_ACCESS` callee whose first child is a `TYPE_NAME`.
  Two kinds, and not one, because the type argument belongs to the function in the first form and to the type in
  the second, and a backend must not tell them apart by where the node stands.
- A call is a built-in call when its callee is the bare name `size_of`, `align_of` or `convert`, it names type
  arguments, and the unit binds no name of its own with that spelling. A function declared in the unit, an import
  of that last segment, an import alias, or a member of an import list (its alias, or its last segment) with that
  name is a user function, so its call stays a user generic. A call of the same name with no type argument is a
  plain call, as before. A qualified callee (`array.convert<sinteger32>(1)`) is not a built-in.
- A call is a static call on a written type when its callee is a `MEMBER_ACCESS` on a type with type arguments
  (`array<sinteger32>.filled`) and the call itself names no type argument. A type that the unit declares with
  generic parameters, as a structure or a type alias, is a user generic and keeps raising 4004: its declaration
  is left out of the module, so the call would point at nothing. Any other head, a built-in type such as `vector`
  or a type of another namespace such as `array`, lowers. A head with a dot is a type of another namespace.
- A type argument that is a function type is not lowered, because a `TYPE_NAME` cannot hold its parameters and its
  result. It raises 4004 once, and the declaration that holds it is left out, as for any gap.
- A gap in an argument of the call, or in a type argument, drops the declaration whole and is reported once.
- The nodes take their position from what they were written from: the `TYPE_NAME` from the written type, the
  `BUILTIN_NAME` from the callee name.

## Text dump

`MAREA_IrModuleDump` writes the tree as stable text into the arena, one node per line, in the same
shape as `MAREA_AstDump`. A line is the kind name indented two spaces per depth, then the text in
double quotes when present, then `type=NAME` when the node has a type name, then `resolved=TEXT` when it has a resolved type (the text has no
space in its canonical spelling, so it needs no quoting), then `op=OPERATOR` when it has an operator, then `flags=NUMBER` when the flags are not zero. In the text, `"` and `\` get a
backslash and a byte below 0x20 or above 0x7E is written as `\xHH`. The result is NUL terminated;
`outLength` does not count the NUL. It returns `ERROR_NULL_POINTER` for a null argument and
`ERROR_OUT_OF_MEMORY` when the arena cannot grow. The walk uses an explicit stack held in the arena, so
call-stack use does not depend on the tree depth; a deep tree costs arena memory only.

```
MODULE
  STRUCTURE_DECLARATION "Point"
    FIELD_DECLARATION "x" type=float64 resolved=float64
  FUNCTION_DECLARATION "f" type=sinteger32 resolved=sinteger32
    PARAMETER_LIST
      PARAMETER "a" type=sinteger32 resolved=sinteger32
    BLOCK
      LOCAL_DECLARATION "b" type=sinteger32 resolved=sinteger32
        BINARY_EXPRESSION resolved=sinteger32 op=PLUS
          IDENTIFIER "a" resolved=sinteger32
          INTEGER_LITERAL "1" resolved=sinteger32
      RETURN_STATEMENT
        IDENTIFIER "b" resolved=sinteger32
```

Lowering the same unit twice gives byte-identical text.

## Constructs the IR cannot lower

Three constructs raise an error diagnostic and make no node: a generic (except the calls of Type operands), a macro
other than a statement `unreachable!()`, and an expression that sema gave no type and that is not untyped on purpose
(see Node model). The declaration that holds the gap is left
out of the module whole, never in part: a gap in a function body drops that function, a gap in a
variant default drops the enum, a gap in a constant initializer drops the constant. Lowering then
continues with the next declaration, so the other declarations stay in the module. A gap inside a
declaration is reported once: lowering of that declaration stops at the first one.

## Example sweep

`tests/source/ir/MAREAIrExamples.cpp` lowers all 23 programs under `examples/` through `lex`, `parse`,
`sema` and `ir`. Each one checks with zero errors, lowers to a `MODULE`, dumps as text and raises
exactly the codes in its row. The 20 programs not listed raise none.

| Program | Codes |
|---|---|
| `interop/c_library.marea` | 4005 |
| `macros/builtin.marea` | 4005 |
| `types/generics.marea` | 4004 |

The programs `attributes/layout.marea`, `basics/blocks.marea`, `basics/primitive_types.marea` and
`interop/gpu_handles.marea` raised 4004 for a built-in call or a static call on a written type. They raise none
now (see Type operands).

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 4004 | `UNINSTANTIATED_GENERIC` | a generic function, structure or `implementation`; a call with explicit type arguments that is not a built-in call (`identity<sinteger32>(5)`); a call whose result type is an unbound generic; a `GENERIC_NAME` that is not the receiver of a call, or whose head is a generic type of the unit; a function type as the type argument of a built-in or a static call; a call whose callee is the bare name of a function of the unit that was left out of the module, also when the callee is declared after the caller (the call and its top-level declaration are dropped, and the callers of a dropped function raise in turn). `size_of<T>()`, `align_of<T>()`, `convert<T>(x)` and `array<sinteger32>.filled(8, 0)` raise nothing. |
| 4005 | `UNSUPPORTED_MACRO` | `print!("hi");`, a module-level `static_assert!(true);`, a macro call inside an expression. `unreachable!();` as a statement is lowered and raises nothing. |
| 4006 | `UNTYPED_EXPRESSION` | an expression for which `MAREA_SemaTypedNodeGetResolvedType` gives no type and that is not untyped on purpose (see Node model): `stdio.value` read as a value (not called) from an `EXTERNAL` namespace. It is raised once, at the position of the expression, and the declaration that holds it is left out whole. A name that only qualifies a member raises nothing. |

The codes 4001, 4002 and 4003 are retired: `fork_join`, `switch` and `for each` are lowered. The numbers have no
name in the public header and are never reused.

The message of 4005 and of 4004 on a declaration names the macro or the declaration in single quotes.
Each diagnostic carries the position of the construct that raised it. The message of 4006 does not name the expression.

## Gaps

- The resolved type is read from sema as it is, so it is as complete as sema makes it. In the 23 examples, 45 expression
  nodes of the lowered trees have no resolved type, all of them untyped on purpose (calls of `EXTERNAL` functions
  and library members with no signature, their callees, and the operations and `spawn` derived from them); the
  sweep test `MAREAIrResolvedType.ExamplesTypeEveryLoweredExpression` lists none other. The `ir` cannot tell a
  name sema left untyped on purpose from one it missed, so an `IDENTIFIER` with no type never raises 4006. The
  nodes that the ir invents are typed, except the step `ASSIGNMENT` and the `DESTROY_CALL`, which are statements.
- The zero-default rule (see Zero-default creation) has no node in the ir: a `LOCAL_DECLARATION` with no written
  initializer is rejected by sema (3065) and a struct literal that omits a field is rejected by sema (3077). Four
  creation paths the language spec names are not lowered yet, because they need a backend to exist first: the zero fill of `Type(args)` before `construct` (the call itself is a `CONSTRUCT_CALL` now), `array<T, N>`
  elements, `owned<T>` storage and a `convert` result that exposes padding. A `PARAMETER`, a `CONSTANT_DECLARATION`
  and a `VARIANT_FIELD` with no value are outside the rule on purpose (see Zero-default creation), not an
  oversight.
- A `FOR_EACH_STATEMENT` does not say how its collection is walked, or what the key is for it beyond its type. The
  collection has its own resolved type, and the value and the key carry theirs when sema gives one (see For-each
  lowering), but sema types them only for `array`, `vector` and `slice` (also under `c.`). For any other collection
  (`list`, a user type) the value and the key have no type, and a backend must find the type itself, and must
  reject, or handle in its own way, a collection it cannot walk. The IR does not check that a loop with a key walks
  a collection that has one.
- A for-each loop has no hidden locals (`$each`, `$count`, `$index`) and no step: the walk is one node. A backend
  that wants a counted loop builds it from the type of the collection. A desugaring to hidden locals would be a new
  IR form, a decision not taken yet.
- The hidden bound of a for-range loop is numbered, but `$switch` and `$return` are not: two `switch` statements
  that nest, or a `return` hoisted in an outer scope after an inner one, declare the same name in nested `BLOCK`
  scopes of one function. A backend that keys the locals of a function by name alone must still scope them by
  `BLOCK`, or the ir must number these names the same way. No backend needs it yet.
- `MAREA_G_LowerSwitch` keeps only the last `default` arm and drops the others with no diagnostic. This
  is defensive code that no accepted program reaches. Sema rejects a second `default` arm, and a repeated `case` value, with
  `DUPLICATE_SWITCH_ARM` (3025, issue #50, PR #51), and `MAREA_SemaBuildTypedAst`, the only input of the IR,
  refuses a tree with a sema error (`ERROR_NOT_CHECKED`). No caller that goes through sema can reach either
  shape (a test checks it). The IR adds no diagnostic for it.
- Only a call whose callee is a bare name is checked against the functions left out of the module. A dropped
  function that is used as a value, not called, or reached through a qualified name, still lowers to a name that
  points at nothing. A parameter or a local with the same name as a dropped function makes a call of it raise
  too, because the IR holds no scope.
- A `TYPE_NAME` is a written name, not a resolved type. The IR does not say whether `Light` in `size_of<Light>()` is
  a structure, a built-in or a generic parameter, and does not check that a built-in gets the number of type
  arguments it needs. A backend must look the name up and reject what it cannot handle.
- The built-ins are a fixed list in the IR: `size_of`, `align_of` and `convert`. A new built-in is a change of the
  IR. The unit is checked for a function or an import with the same name, but not for a parameter or a local, and
  the IR cannot see what another file declares: an imported function that shares a built-in name is taken as the
  built-in when the import does not bind that name.
- A function type as a type argument of a built-in or of a static call is not lowered and raises 4004, because a
  `TYPE_NAME` has no place for its parameters and its result. A `GENERIC_NAME` that is not the receiver of a call
  (`array<sinteger32>.LIMIT`) also raises 4004.
- A static call on a generic type of another namespace lowers, but a call of a generic function of another
  namespace with explicit type arguments (`other.maximum<sinteger32>(1, 2)`) still raises 4004: only the three
  built-ins have a call form with a type argument on the function.
- `LOCAL_DECLARATION` never carries the `MUTABLE` flag, except the hidden loop variable of a for-range loop: the
  AST has no write-target flag for a local.
- The sema typed tree gives a type name only for a call result, a struct literal and a few expression
  shapes, so the type name of a declaration is the head name written in the source. A type argument,
  and any `FUNCTION_TYPE`, is lost on a declaration; only the operand of a built-in or of a static call keeps its
  type arguments.
- There is no linking between modules: an imported name is lowered to its original name, but the IR does not look
  at the namespace that holds it.
- A local hides an import alias from its declaration to the end of the function, also after the end of the nested
  block that holds it, because the check compares source offsets and does not track block scope. Sema does not
  reject such a shadow.
- Two members with the same original name in two namespaces keep the same lowered name; the IR does not add the
  namespace. A backend that needs it must read the import itself.
- The tree builder and the lowering of a nested type argument are recursive. A very deep tree needs a large stack;
  no test goes deeper than the example programs. The dump is not part of this gap: it walks an explicit stack.
- `MAREA_IrLowerCompilationUnit` reports a root that is not a `COMPILATION_UNIT` with
  `MAREA_IR_EXCEPT_ERROR_NULL_POINTER`, because the except set has no other fitting value.
- A call fills an omitted defaulted argument only when its callee is a bare `IDENTIFIER` naming a
  `FUNCTION_DECLARATION` of the same unit, the same limit the built-in lookup already has. A call
  through a member access (an `implementation` method), an import or a qualified name does not fill
  one, even when sema accepted the same omission: the IR holds no scope or cross-module link to find
  that callee's parameter list. Such a call keeps the arguments the source wrote, with no gap and no
  diagnostic, so it lowers with fewer arguments than the callee's parameter list names.
