# Marea sema stage

## Contract

The `sema` stage checks the tree of the `parse` stage. It reads a public tree and writes diagnostics.
It has no global state, never prints and never reads the disk: the caller gives it an arena, a
diagnostic sink and an import resolver. `MAREA_SemaCheckCompilationUnit` itself returns no artifact
beyond diagnostics; a separate, additive API builds the typed shadow tree the `ir` stage will consume
- see [Typed AST hand-off](#typed-ast-hand-off).

## Use

```c
MAREA_SemaImportResolver *resolver;
MAREA_SemaContext *sema;

MAREA_SemaImportResolverCreate(arena, my_resolve_function, my_data, &resolver);
MAREA_SemaContextCreate(arena, sink, resolver, &sema);
MAREA_SemaCheckCompilationUnit(sema, root);   /* once per file, dependencies first */
```

`MAREA_SemaCheckCompilationUnit` returns an error only for a null argument or an out of memory. A rule
violation is a diagnostic in the sink. A size that does not fit in `MAREA_IntegerSize` (a diagnostic
message, or the growth of a name table) is also an out of memory: nothing is allocated or copied.
Every sema diagnostic is an error, in the range 3001-3026.
The lexer file name decides the path rule, so call `MAREA_LexContextSetFileName` with a project
relative path with forward slashes and the `.marea` extension before the lex step.

A node that the parser could not build (`ERROR_NODE`) is skipped: no check runs under it and no
diagnostic is placed on it.

## Import resolver

```c
void resolve(void *userData, const char *text, size_t length,
             MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot);
```

| Answer | Meaning |
|---|---|
| `NOT_FOUND` | No namespace has this name. |
| `FOUND` | The namespace has a tree. `*outRoot` is its `COMPILATION_UNIT`. |
| `EXTERNAL` | The namespace exists but sema has no tree for it (every `c.*` namespace, and the libraries that are not written yet). |

`import a.b.c;` asks for `a.b.c`. On `NOT_FOUND` sema asks for `a.b` and looks for a `#[public]`
top-level symbol `c` there. Both answers `NOT_FOUND` is `UNRESOLVED_IMPORT`. A name from an
`EXTERNAL` namespace is opaque: it meets every contract, takes any arguments and gives no diagnostic.
The local name of an import is its alias, or its last segment. A type name that no local rule knows
is given to the resolver as a last step.

`import a.b { f as g, T as U, Color.red as Crimson, MAX };` asks the resolver for `a.b` once, then
binds one local name per member: its alias, or the last segment of its path. A member path has one
or two segments; a longer path is `UNRESOLVED_IMPORT`. For a `NOT_FOUND` namespace every member is
unresolved. For `EXTERNAL` every member of one or two segments is accepted. For `FOUND` the first
segment must be a `#[public]` top-level symbol of the tree, and a two-segment member needs that symbol
to be an `enum` with that variant. An unresolved member still binds its local name, so one error
does not start a chain of `UNDECLARED_NAME`. A member alias that is a parse error (`f as }`) is
skipped without a diagnostic.

An import of a `#[public]` `enum` from a `FOUND` namespace (a one-segment member, or a whole path whose
parent is `FOUND`) also keeps that enum declaration with its local name, alias included. The type
class of that name is then `USER`, so a `switch` over it raises `NON_EXHAUSTIVE_SWITCH` like a
same-file enum. Any other import, and every `EXTERNAL` one, keeps no declaration and stays `EXTERNAL`.

Every import name keeps its original: the text the same import binds without `as` (the last segment
of a whole path, the member path as written for a member, for example `Color.red`). The internal name
table (`MAREA_SemaName`) holds it in `original` and `originalLength`, empty for a name that has none.
The IR lowers an alias to this text. A type written with an alias compares equal to the type
written with the original in a contract signature, and so does a local `type Name = Other;` (a chain
of aliases is followed up to 8 steps; an alias with generic parameters is not expanded).
An import name also keeps its qualifier, the namespace it came from (`qualifier` and `qualifierLength`),
and the comparison renders it with that namespace: `import a { T as X }; import b { T as Y };` make two
types, while two aliases of one member of one namespace stay one type.

The name `c` is the reserved namespace of C. An import alias named `c` (either form) or a `type`
named `c` is `NAME_ALREADY_DECLARED` with the message `name is reserved: 'c'`; an alias that is
refused is not bound. A plain declaration or a plain `import c.x;` is not affected.

## Rules

**Attributes.** A fixed key table: `public` and `private` (not both), `inline` (`force`, `never`,
`auto`), `build` (`debug`, `release`), `simd` (boolean), `layout` (`c`, `auto`), `align` (a power of
two, not with `pack`), `pack` (an integer), `value` and `move_only` (booleans), `destroy` (a name),
`operator` (a name or a string), `export` (a flag or a name). The deeper meaning of `destroy`,
`operator` and `export` is not checked. A key with no value is true. File attributes `#![...]` use
the same table; the attribute of the symbol wins over the file attribute.

**Types.** The built-in names are `boolean`, `integer8` to `integer64`, `unsigned8` to `unsigned64`,
`float32`, `float64`, `string`, `vector`, `pointer`, `owned`, `slice` and `task`. A type name is also
`Self` inside an `implementation` or a `contract`, a generic parameter in scope, a structure, enum,
opaque type or alias of the file, or an import. `vector` takes two arguments, the others one, and a
user generic takes as many arguments as it has parameters. A number fills a value parameter only.

**Contracts.** `Copyable` is met by a primitive, a `vector`, `pointer`, `slice` or function type, an
enum and a `#[value]` type. `Movable` is met by `owned`, `task`, a `#[move_only]` type and a plain
user type. Any other contract is met when the `implementation` blocks of the type hold every contract
function with the same parameters and result, `Self` replaced by the type. A generic parameter meets a
contract only when its own `where` clause lists it. The check runs at a call of a function of the file,
with the types written in `f<T>(...)` or taken from the arguments (a name with a declared type, or a
struct literal). A built-in type is not searched for contract functions.

**Ownership.** A parameter with no word is a read-only borrow, `mutable` is a write borrow,
`reference` forces a borrow of a by-value type, `owned<T>` moves in. A primitive, a `vector`, an
enum and a `#[value]` type is by value. `=` of a name, field or element of any other type needs
`copy x` or `move x`, in an assignment and in a local initializer. A call or a literal needs no word.
A borrow parameter, a `for each` variable, a `partition` part and a `with` name cannot be returned,
nor be the value of a field. A new local of the same name ends the reference.

**Spawn.** In `fork_join`, each argument of `spawn f(...)` is a `partition` part, a literal, a file
constant, or a name that is not a `mutable` parameter and not the source of an active `partition
mutable`, or a field access (`a.b`, nested too) whose base meets the same rule. Any other expression
(a call, an index, a computed value) is rejected. This is a simple first rule.

**Parallel bodies.** The body of a `partition` runs once for each part, so it is a worker body and ends
only at its own `end`. A `return`, a `break` or a `continue` in it that would leave the body is rejected
with `INVALID_PARALLEL_JUMP` (3024), one diagnostic for each jump. A `break` or `continue` that targets a
loop inside the body is accepted, also through `if`, `switch` and `with`. The nearest `partition` body
counts: in a nested `partition`, the inner body is the barrier. The `fork_join` block itself is the same
kind of barrier, one level up: a `return`, `break` or `continue` that stands directly in it, outside every
`partition` body (also directly around a `spawn`), is rejected with `INVALID_PARALLEL_JUMP` (3024) too,
unless a loop that is itself directly in the block catches the `break` or `continue` first - a loop outside
the whole `fork_join` block is never a valid target. A jump in a `with` body, in a loop or in a function body
outside every `fork_join` block is not changed. The rule follows the grammar: `return` leaves the function
(note 9), the body is run once for each part, and all spawned work ends before the `end` of the `fork_join`
- a jump that bypasses that `end`, from inside a `partition` body or directly in the block, would skip the
join of the spawned workers.

**Control flow.** A function with a return type ends every path with `return value;`, through `if`,
`elseif`, `else`, scope blocks and `switch`. A loop is never an ending path. `unreachable!()` and
`compile_error!()` end a path. A `switch` over an enum names every variant or has a `default`.

**Switch arms.** A `switch` has at most one `default` arm, and no `case` value appears twice. Each second
and later `default` arm is rejected with `DUPLICATE_SWITCH_ARM` (3025), and so is each `case` value that
appeared before, in one list (`case 1, 1`) or across arms. The first use is never reported, and each repeat
is one diagnostic, at the repeated item. Two values are the same when they hold one value. Integer literals
are compared by number in any base, with or without `_` separators (`1` and `0x1`, `5` and `0b101`, `-1` and
`-0x1`). A name of a constant of the file stands for the value it is declared with, and a constant that is
declared with another constant is followed (at most 16 steps, so a cycle ends), so `MAX` and `100` are the same
when `constant MAX: integer32 = 100;`. A local or a parameter with the name of a constant hides it. Other
literals, names and unary operators are compared by text (`"a"` and `"a"`, `a.LIMIT` and `a.LIMIT`). Over an enum a bare and a qualified name of one variant (`red`, `Color.red`) are the same
variant, as in the coverage check. Two broken (`ERROR_NODE`) values are never compared. The check runs for
every scrutinee type, and it does not replace `NON_EXHAUSTIVE_SWITCH`: a `switch` over an enum with a repeat
and a missing variant raises both. The rule follows the grammar (`switch_statement` has one optional
`default_arm`) and the arms of a `switch` do not fall through, so a repeated value is an arm that never runs
and a second `default` is an arm that the IR would drop. The place of the `default` is not checked by
sema itself: the grammar puts `default_arm` last, and the parser now enforces that placement
(`DEFAULT_ARM_NOT_LAST`, 2010), so a `default` that is not the last arm never reaches sema through a
program the parser accepts.

**Expressions.** Sema gives an expression a type only when it is cheap to know, and never guesses. A
literal has its own type. A name has the declared type of its local, parameter or file constant. A field, or an
element of an `array`, `slice`, `vector` or `pointer`, has the type sema already reads for the ownership check
(a primitive only). A built-in `convert<T>(x)` has the type `T`. A `-`, `~`, `copy` or `move` keeps the type of its operand. `not`, `and`, `or` and a
comparison give `boolean`. A shift gives the type of its left side. Any other binary operator, `**` too, gives the
type of both sides when they are the same primitive, or when one side is a number literal and the other a
primitive of its family; it gives no type when they differ. A call of a plain function of the file (not
generic, and not hidden by a local of the same name) has the declared result type. Everything else has an
unknown type and is accepted. The type of an expression is compared with the declared type when that type
is a primitive, at a call argument of a function of the file, a local initializer, a plain `=` assignment, a
`return` value and a constant initializer: a conversion is always written, so two primitives are the same
only by name (`integer32` does not fit `integer64`), a number literal fits any number type, a fraction literal
any float type, and a structure fits no primitive. An alias of a primitive is the primitive. The mismatch is
`TYPE_MISMATCH` (3011) on the expression. A name of an enum, of a library type or of a generic parameter, and an `owned`,
`task` or `vector` value, are classified as their own shapes but are never compared.

**Operands of a binary operator.** Every binary expression in a checked slot (the same slots as the names, and
the initializer of a constant) has its two operands compared, whatever the declared type: `+`, `-`, `*`, `/`,
`%`, `**` and their wrapping and saturating forms, the six comparisons, `&`, `|`, `^`, `and` and `or`. Two primitives
agree only by exact name (`integer32` and `float64` do not, and a conversion is always written). A number literal
agrees with a primitive as it fits a declared type, so `a + 1` is accepted for a `float64` and `a + 1.5` is not
for an `integer32`. A mismatch is `TYPE_MISMATCH` (3011) on the binary expression, once per expression, and the
expression then has no type, so the declared type of a local or a `return` is not reported a second time. An
operand that is a structure, an enum, a library type, a generic result or any other unknown type is accepted.
A shift is not compared here, as its right side may have another type; it has its own rule below.

**No implicit numeric conversion (decision of the user, by the rules of Rust).** A calculation never converts a
number by itself: the two operands of an arithmetic or a comparison operator have the same primitive type, and
`**` follows the same rule as `+`. An integer base with a float exponent is `TYPE_MISMATCH` (Rust `pow` against
`powf`), whatever the declared type of the result, and the result of a power has the type of its operands.

**Shifts (decision of the user, by the rules of Rust).** The count of `<<` and `>>` may be any integer primitive of
any width (`integer8` to `integer64`, `unsigned8` to `unsigned64`, or an integer literal), and the result has the
type of the left operand. A count or a left operand that is a known type and not an integer (a float, a
`boolean`, a `string`, a fraction literal) is `TYPE_MISMATCH` (3011) on the shift, once, and the shift then has
no type. An operand of an unknown type is accepted.

**Compound assignments (decision of the user, by the rules of Rust).** `+=`, `-=`, `*=`, `/=`, `%=`, `**=`, `&=`,
`|=` and `^=` need the same type on both sides, as `a = a + b` does, with the same literal rule as a binary
operator. `<<=` and `>>=` follow the shift rule. A mismatch is `TYPE_MISMATCH` (3011) on the assignment, once.
A side of an unknown type is accepted.

**Conversion `convert<T>(x)` (decision of the user, by the rules of Rust).** A conversion is always written and
never implicit. The call is the built-in when it names exactly one type argument and has exactly one argument and
the unit binds no name `convert` of its own (a function, an import, a parameter or a local with that name makes
it a plain call, as the IR reads it). Its result type is `T`, even when the conversion itself is rejected, so it
feeds every other check of this section (a local, a parameter, a `return`, a constant, an operand, a compound
assignment). Allowed:

- a number primitive to a number primitive of any width and sign: integer to integer, integer to float, float to
  integer and float to float (`integer8` to `unsigned64`, `float64` to `integer32`). A number literal and a
  calculation of numbers are number operands. `boolean` and `string` are not numbers;
- the same primitive to itself (`convert<string>(text)`, `convert<boolean>(flag)`);
- a `structure` to another `structure` that has the same layout (below), and a structure to itself;
- an operand or a target of a type sema does not know (an enum, a library type, a generic parameter, a `pointer`,
  a field or a call that sema cannot type). An unknown type stays accepted.

Everything else is `TYPE_MISMATCH` (3011) on the call, once: a `string` or a `boolean` to a number or the other
way, a structure to a primitive, a primitive to a structure, and a structure to a structure with another layout.

Two structures have the same layout when every field has the same primitive type, by exact name, at the same
place (an alias of a primitive is the primitive, the names of the fields do not count), they have the same
alignment and, when both are `#[layout = c]`, the same size. Sema computes the alignment and the size by the rules
of C: each field at the next multiple of its own alignment (the size of a primitive), `pack = N` caps that
alignment, `align = N` raises the alignment of the structure, and the size is rounded up to the alignment. The
layout may come from a file attribute. The pair is accepted, and the size check is a gap for the backend, when sema
cannot decide it: a generic structure, a field that is not a fixed-width primitive (`string`, `pointer`, `vector`,
a structure, an enum, a library type), a field with its own `align`, a `pack` or `align` value that is not an
integer from 1 to 4294967296, and two structures whose size depends on the backend (a layout that is not `c`,
and `auto` is the default). The alignment and the field types are still compared when only the size is open.

**Constants of an `implementation` block (decision of the user).** Inside an `implementation` block, a bare
constant of that block shadows a top-level constant of the same name: the inner scope wins, and the name has the
type that the block gives it. A parameter or a local still hides both. A constant of one block is not a name in
another block or outside the blocks.

**Names in expressions.** A bare name in the initializer of a local, either side of an assignment, an
expression statement, the condition of an `if`, `elseif` or `while`, a `return` value and a `switch` value (and
below them: operands, arguments, indexes, entries of a literal value) must be declared, or sema raises
`UNDECLARED_NAME` (3004) on it. A name is declared when it is a local or parameter in scope, the name of a
`for`, `for each`, `partition` or `with` statement (hidden from the end of that statement), a declaration or
import of the file, a generic parameter in scope, a constant of an `implementation` block inside that block only, the
first segment of the file namespace, or one of the built-ins `c`, `size_of`, `align_of`, `convert` and `Self`.
A member name, a field key of a literal value, a type, a macro call and the bare name that starts a member
chain (`stdio.puts(text)`, which may be a namespace the file never imports) are not checked. The local that a
`for`, `for each`, `partition` or `with` name hides is never used to type the name.

**Entry point.** In the namespace `main`, `function main` has exactly the parameter `arguments:
array<string>` with no word, and the result `integer32`. Sema never requires a `main`.

## Typed AST hand-off

`MAREA_SemaBuildTypedAst` (`marea/sema/MAREASemaTypedAst.h`) turns a compilation unit `sema` already
checked into a typed shadow tree: one `MAREA_SemaTypedNode` per original `MAREA_AstNode`, same order
and child count. It is purely additive - `MAREA_SemaCheckCompilationUnit`'s signature and behavior do
not change, and it never duplicates logic shared with the checker: a new, standalone inference path
resolves a call result's return type, kept fully independent from the existing `MAREA_SemaInferType`
so that function's already-shipped assignment fallback behavior is untouched.

```c
const MAREA_SemaTypedNode *root;

MAREA_SemaCheckCompilationUnit(sema, unit);                 /* must report zero errors first */
MAREA_SemaBuildTypedAst(sema, unit, &root);
```

`MAREA_SemaBuildTypedAst` returns `MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED` when `unit` was not
the last tree checked on `sema`, or when `MAREA_DiagnosticSinkGetErrorCount` on
`MAREA_SemaContextGetDiagnostics(sema)` reports an error - the hand-off only exists for a unit sema
already accepted. Otherwise it returns `MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS`,
`ERROR_NULL_POINTER` for a null argument, or `ERROR_OUT_OF_MEMORY` when the arena cannot grow.

**Type class.** `MAREA_SemaTypeClass` is the same enum the checker already uses internally, moved to
this public header: `PRIMITIVE`, `COPY_BUILTIN`, `MOVE_BUILTIN`, `GENERIC` (an unbound generic
parameter), `USER` (a structure or enum of this compilation, `MAREA_SemaTypedNodeGetTypeDeclaration`
gives its declaration) and `EXTERNAL` (a name the import resolver reported as `EXTERNAL` or that no
local rule knows; `MAREA_SemaTypedNodeGetTypeName` gives its text). A node whose type could not be
inferred keeps the neutral default `PRIMITIVE` with no declaration or name, the same best-effort
fallback `MAREA_SemaInferType` already uses for the checker.

**Ownership mode.** `MAREA_SemaOwnershipMode` names how a typed node carries its value, following the
same rules "Ownership" above already states: `VALUE` (a primitive, a `vector`/`pointer`/`slice`/
`owned`/`task` core type is a different case below, an enum or a `#[value]` type), `BORROW` (a
read-only parameter, a `for each` variable, a `partition` part or a `with` name), `MUTABLE_BORROW` (a
`mutable` parameter), `MOVE` (an `owned<T>` or `task<T>` parameter or value), `COPY` (a `copy x`
expression). A node this cannot classify defaults to `VALUE`.

**Call results.** A call expression's typed node carries the type the call returns. A call of a function of the
file carries the callee's declared return type: `USER`, `EXTERNAL` or a built-in class. A built-in `convert<T>(x)`
(with no name `convert` bound in the unit) carries the type `T`, read in the scope of the caller. When the callee is
generic and the call gives explicit type arguments, a return type that is one generic parameter (`Item`) is
replaced by the type argument at the same place (`same<integer32>(5)` is `integer32`). When there are no type
arguments, or the return type is a compound of a parameter (`array<Item>`), the typed node keeps the raw type: a
bare parameter is tagged `GENERIC` with its name - never a guess, never a crash. Resolving a result from inferred
type arguments is deferred to when the `ir` stage needs one for real.

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 3001 | `UNRESOLVED_IMPORT` | `import a.b.c;` when the resolver finds neither `a.b.c` nor a public `c` in `a.b`; `import a.b { f };` when `a.b` is not found, `f` is not a public symbol of it, a two-part member is not a variant of a public enum, or the member path has more than two parts |
| 3002 | `NAMESPACE_PATH_MISMATCH` | `namespace a.b.c;` in the file `a/b.marea` |
| 3003 | `NAME_ALREADY_DECLARED` | `structure Item end function Item() end`, or an import (or an import alias, or a member alias) and a declaration or another import of one local name; an import alias or a `type` named `c` (message `name is reserved:`) |
| 3004 | `UNDECLARED_NAME` | `where Item: Missing`, or a constraint on a name that is not a generic parameter; `local x: integer32 = missing;`, `missing(1);` |
| 3005 | `UNKNOWN_ATTRIBUTE_KEY` | `#[frobnicate = true]` |
| 3006 | `INVALID_ATTRIBUTE_VALUE` | `#[inline = maybe]`, `#[build = 1]`, `#[simd = force]` |
| 3007 | `CONFLICTING_ATTRIBUTES` | `#[public, private]`, `#[pack = 1, align = 16]`, a key given twice |
| 3008 | `INVALID_ALIGNMENT` | `#[align = 15]` |
| 3009 | `UNKNOWN_TYPE` | `a: Missing;` in a structure |
| 3010 | `ARGUMENT_COUNT_MISMATCH` | `add(1)` when `add` takes two parameters |
| 3011 | `TYPE_MISMATCH` | `convert<integer32>(text)` with a `string` name, `convert<Point>(other)` for two structures with another layout, `f("text")` when the parameter is `integer32`, `local x: boolean = 1;`, `local n: integer32 = a < b;`, `f(s)` with a `string` name `s`, `return a + 1;` in a function that returns `boolean`, `a + b` with an `integer32` and a `float64` |
| 3012 | `DUPLICATE_MEMBER` | two `implementation` blocks of one type that both declare `area` |
| 3013 | `CONTRACT_NOT_SATISFIED` | `maximum(a, b)` when the type of `a` has no `less` for `where Item: Comparable` |
| 3014 | `GENERIC_ARGUMENT_MISMATCH` | `Pair<integer32>` when `Pair` has two parameters, `Box<4>` |
| 3015 | `INSTANTIATION_TOO_DEEP` | a type argument nested 65 levels |
| 3016 | `ASSIGNMENT_NEEDS_COPY_OR_MOVE` | `a = b;` when the type is not a primitive or a `#[value]` type |
| 3017 | `INVALID_BORROW` | `s: reference Shape` when `Shape` is already a borrow |
| 3018 | `REFERENCE_ESCAPES_SCOPE` | `return s;` when `s` is a borrow parameter |
| 3019 | `EXCLUSIVE_BORROW_CONFLICT` | `take(s, s)` when one parameter is `mutable` |
| 3020 | `INVALID_SPAWN_ARGUMENT` | `spawn g(m)` in `fork_join` when `m` is a `mutable` parameter |
| 3021 | `NON_EXHAUSTIVE_SWITCH` | a `switch` over an enum that misses a variant and has no `default` |
| 3022 | `MISSING_RETURN` | `if a then return 1; end` as the whole body of a function with a return type |
| 3023 | `INVALID_MAIN_SIGNATURE` | `function main(): integer32` in the namespace `main` |
| 3024 | `INVALID_PARALLEL_JUMP` | `continue;`, `break;` or `return;` in a `partition` body when no loop of the body is the target, or directly in a `fork_join` block, outside every `partition` body, when no loop of the block is the target |
| 3025 | `DUPLICATE_SWITCH_ARM` | a second `default` arm in a `switch`, or `case 1 then ... case 1 then ...`, `case 1, 1 then ...`, `case Color.red` twice |
| 3026 | `JUMP_OUTSIDE_LOOP` | `break;` or `continue;` with no enclosing loop and no enclosing `partition`/`fork_join` block |

## Gaps

- Renaming on use inside one file (without an import) is not offered: a local declaration has its own name.
- `TYPE_MISMATCH` (3011) sorts the type of an expression into a fixed set of shapes: a literal, a primitive, a
  structure, and, known but never compared, an enum, a library type, a generic parameter and an `owned`, `task` or
  `vector` value. Still fully unknown: a `pointer`, a `slice`, a function type, the type of a field or a file
  constant that is not a primitive, and the result of a generic call (see below).
- A binary operator is not checked against what its operands may be (`a and b` with two integers raises nothing,
  as the two have the same name), and an operator that is declared for a structure is not compared.
- `UNDECLARED_NAME` (3004) is not raised for the bare name that starts a member chain, for the header of a `for`,
  `for each`, `partition` or `with` statement, or for a name in the initializer of a file constant. The bare
  chain head is skipped because the `basics/functions` example calls `stdio.puts(text)` with no `import`, so the
  name may be a namespace; a later step can check it once that example imports what it uses.
- The contract check only sees arguments whose type is cheap to know. Other arguments are not checked.
- `destroy`, `operator` and `export` are checked for shape only.
- The type of a library type is not tracked, so a missing `copy` or `move` there is not reported.
- The typed-AST hand-off substitutes a generic result only for a bare parameter with explicit type arguments; a
  result from inferred type arguments or a compound such as `array<Item>` keeps the raw type.
- `DUPLICATE_SWITCH_ARM` compares a float by its text: `1.0` and `1.00` are taken for different values. A constant
  that sema cannot read (imported, or declared with a computed expression) is compared by its name only, and a
  case value that is a call or another computed expression is never compared.
- `convert<T>(x)` between two structures is checked in sema only for the field types, the alignment and, for a `c`
  layout, the size, as described under "Rules". Where sema cannot size a structure (the default `auto` layout, a
  `string`, `pointer`, `vector`, structure or enum field, a generic structure, a field `align`), the pair is
  accepted and the backend must check the size and the alignment of the two types before it emits the conversion.
  A field pair of two primitives with the same width but another name (`integer32` and `float32`) is a mismatch
  here, as the rule is by exact name; a looser rule is a later decision of the user. The numeric range of a
  conversion (`convert<integer8>(count)` with `count` above 127) is a run-time stop, not a check of sema.
