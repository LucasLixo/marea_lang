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
Every sema diagnostic is an error, in the range 3001-3069. Not every number of the range is a live code: the
Diagnostics table lists the live ones.
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

`import a.b.c;` asks for `a.b.c`. On `NOT_FOUND` sema asks for `a.b` and looks for a top-level symbol `c` that is not `#[visibility = private]`
there. Both answers `NOT_FOUND` is `UNRESOLVED_IMPORT`. A name from an
`EXTERNAL` namespace is opaque: it meets every contract, takes any arguments and gives no diagnostic.
The local name of an import is its alias, or its last segment. A type name that no local rule knows
is given to the resolver as a last step.

`import a.b { f as g, T as U, Color.red as Crimson, MAX };` asks the resolver for `a.b` once, then
binds one local name per member: its alias, or the last segment of its path. A member path has one
or two segments; a longer path is `UNRESOLVED_IMPORT`. For a `NOT_FOUND` namespace every member is
unresolved. For `EXTERNAL` every member of one or two segments is accepted. For `FOUND` the first
segment must be a top-level symbol of the tree that is not `#[visibility = private]`, and a two-segment member needs that symbol
to be an `enum` with that variant. An unresolved member still binds its local name, so one error
does not start a chain of `UNDECLARED_NAME`. A member alias that is a parse error (`f as }`) is
skipped without a diagnostic.

An import of a non-private `enum` from a `FOUND` namespace (a one-segment member, or a whole path whose
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

An import alias (either form) named like one of the 13 primitive types (`boolean`, `sinteger8` to `sinteger64`,
`uinteger8` to `uinteger64`, `float32`, `float64`, `character`, `string`) is `NAME_ALREADY_DECLARED` with the
message `name is a primitive type: 'sinteger32'`, and the alias is not bound. A library name such as `array` stays a
legal alias. A top-level function, constant, `structure`, `enum` or `type`, a local, or an import alias named like one
of the built-in macros (`assert`, `debug_assert`, `unreachable`, `compile_error`, `static_assert`, `include_bytes`,
`include_text`, `format`, `print`, `log`, `line`, `file`; `namespace` is a keyword) is `RESERVED_MACRO_NAME` (3056)
with the message `name is a built-in macro: 'print'`. A declaration or a local that is refused is still bound, so its
uses raise nothing more; an alias that is refused is not bound, so its uses raise `UNDECLARED_NAME`. A name that only
contains a macro name (`printer`) is accepted. A parameter, a `for`, `for each`, `partition` or `with` name and an
alias without `as` are not checked for a macro name.

### Signature of an EXTERNAL member

`MAREA_SemaImportResolverSetSignatureFunction` adds an OPTIONAL second callback to a resolver created
with `MAREA_SemaImportResolverCreate` (null by default, so every existing caller is unaffected):

```c
void signature(void *userData, const char *namespaceText, size_t namespaceLength,
                const char *memberText, size_t memberLength,
                MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature);
```

Sema calls it only for a call whose callee names a member of a namespace the resolve function answers
`EXTERNAL` for - never for a `FOUND` or `NOT_FOUND` namespace, and never when no signature function is
set. The member is read the same way for a member access (`stdio.puts(text)`: namespace `c.stdio`,
member `puts`) and for a bare name an import bound to one member (`import c.math.pow as power;` then
`power(...)`: namespace `c.math`, member `pow`, the import's original name, not its alias). A 3-segment
`import a.b.c;` into an `EXTERNAL` namespace never resolves its own full joined path to `NOT_FOUND` (every
depth answers `EXTERNAL`), so its local name stays bound as a namespace with the member kept as its
`original` text, rather than becoming an `IMPORTED_SYMBOL` the way a deeper member of a `FOUND` namespace
does; sema reads both shapes the same way here.

`outResult` answers `NOT_FOUND` when the host does not know the member (the call stays exactly as opaque
as with no signature function at all: no diagnostic, no type) or `FOUND` with `*outSignature` filled
(`EXTERNAL` is never a meaningful answer here). `MAREA_SemaSignature` is a plain struct, not opaque,
because the host builds and owns it: an array of `MAREA_SemaSignatureParameter` (each a canonical type
spelling such as `float64` or `string`, not NUL terminated) with its count, and a canonical result type
spelling. The host keeps every text and the parameter array valid for as long as the resolver may be
asked again; sema only reads them during one call, never copies or frees them.

A call to a member with a signature is checked like a call of a file function: the argument count must
equal `parameterCount` exactly (the host describes no default) or `ARGUMENT_COUNT_MISMATCH` (3010), and
each argument must fit its parameter's primitive spelling or `TYPE_MISMATCH` (3011), by the same literal
and primitive fit rule as `MAREA_SemaCheckExpressionFit`. The call is then typed with the signature's
result spelling. The member or name itself (`puts` before the call, `power` used with no call) stays
untyped: only the call gets the signature's result.

### Members of a library type

The same signature function also types a call on a member of a library type: array, vector, slice, pointer, owned,
task and table, written bare or under the reserved `c`. Sema keeps one fixed shape per library (the canonical
namespace `c.<name>` and its generic parameter names) and asks the function with that namespace and the member
name, never with the namespace an import gave the name. Two call shapes are read: a static call on the type
itself (`array<float32, 16>.filled(0.0)`, `c.array<sinteger32>.filled(8, 0)`) and an instance call on a value whose
type peels down to one of these types (`tasks.append(x)`, `item.result()`, `product.sum()`), following a generic
parameter a frame fills, `Self`, a local alias and `owned<T>`. A result spelled `Self` is the receiver's own
type. A result spelled with a generic parameter of the library (array: `Item`, `Count`; table: `Key`, `Value`;
the others: `Item`) is the receiver's type argument at that place, read in the receiver's own frame; a size
argument is not a type, so `Count` leaves the call untyped. Any other spelling is the literal result, as for an
`EXTERNAL` call. The arguments of such a call are not checked against the signature. The index of a `table<K, V>`
is typed with `V` by the structure alone (no signature is asked), as the element of an array is. With no signature
function, a receiver that is not one of these types or a member the function does not know, nothing changes: the
call stays untyped and no diagnostic is raised.

## Rules

**Integer names.** The primitive integers are `sinteger8`, `sinteger16`, `sinteger32`, `sinteger64`, `uinteger8`,
`uinteger16`, `uinteger32` and `uinteger64`. The older names `integerN` and `unsignedN` are retired: they are
unknown types and raise `UNKNOWN_TYPE`.

**One name, one scope.** A name that is declared twice in one scope raises `NAME_ALREADY_DECLARED` (3003) at the
second one, or `DUPLICATE_MEMBER` (3012) for a member of a structure, an enum or a contract. The parameters of a
function share the scope of the top-level statements of its body. A name in an inner block that hides a name of an
outer block is accepted, and so is a `for`, `for each`, `partition` or `with` name.

**Attributes.** A fixed key table: `visibility` (`public`, `private`), `inline` (`force`, `never`,
`auto`), `build` (`debug`, `release`), `layout` (`c`, `marea`), `align` (a power of two, not with
`pack`), `pack` (an integer), `move_only` (a boolean), `operator` (a string of a fixed list),
`export` (a string, the label at the C border), `semantics` (`value_read_only`,
`reference_read_only`, `reference_mutable` - a parameter's passing mode, see "Ownership" below). A
key of an enum needs one of its members, and `export`
and `operator` need a string, so a bare key of those is `INVALID_ATTRIBUTE_VALUE`. The strings of `operator`
are the operator tokens of the grammar (`+ - * / % **`, the wrapping and saturating forms, `~ & | ^ << >>`, the
comparisons, the assignment forms), `"[]"`, `"()"`, `"convert"`, `"construct"` and `"destroy"`; any other
string or a name is `INVALID_ATTRIBUTE_VALUE`. A key not in the table, such as the
retired `public`, `private`, `simd`, `destroy` and `value`, is `UNKNOWN_ATTRIBUTE_KEY`. The deeper meaning of
`operator` (the signature, the placement) and `export` is not checked. A flag key with no value is true. File attributes `#![...]` use
the same table; the attribute of the symbol wins over the file attribute, so `#![visibility = public]`
with `#[visibility = private]` on one symbol is not a conflict. `semantics` is meaningful only on a
`PARAMETER`, but is validated by the same generic, node-shape-agnostic attribute check as everywhere
else; it is adopted as a `PARAMETER` child last, after the name, type and optional default, so every
other reader of those fixed positions keeps working unchanged - unlike every other declaration kind,
whose attributes lead.

**Types.** The built-in names are `boolean`, `sinteger8` to `sinteger64`, `uinteger8` to `uinteger64`,
`float32`, `float64`, `character`, `string`, `array`, `vector`, `pointer`, `owned`, `slice` and `task`. A type name is also
`Self` inside an `implementation` or a `contract`, a generic parameter in scope, a structure, enum,
opaque type or alias of the file, or an import. `array` takes two arguments (a type and a size), the others one, and a
user generic takes as many arguments as it has parameters. A number fills a value parameter only.

**Contracts.** `Copyable` is met by a primitive, an `array` whose element is `Copyable`, a `pointer`, `slice` or function type and an
enum; the enum case is a documented, intentional exception to "a non-primitive is never a copy contract" (no attribute exempts any
other user type - the retired `#[value]` key does not exist). A `vector` is never `Copyable`. `Movable` is met by `owned`, `task`, `vector`, a `#[move_only]` type and a plain
user type. Any other contract is met when the `implementation` blocks of the type hold every contract
function with the same parameters and result, `Self` replaced by the type. A generic parameter meets a
contract only when its own `where` clause lists it. The check runs at a call of a function of the file,
with the types written in `f<T>(...)` or taken from the arguments (a name with a declared type, or a
struct literal). A built-in type is not searched for contract functions.

**Default parameter values.** A parameter may end with `= value`. A call of a function of the file passes
from the number of parameters up to the last one with no default, to the total number of parameters; any other
count is `ARGUMENT_COUNT_MISMATCH` (3010). Each default is checked like a local initializer: a name in it must
be declared (`UNDECLARED_NAME`) and it must fit the parameter type (`TYPE_MISMATCH`). A defaulted parameter
that has a required parameter after it is `DEFAULT_PARAMETER_BEFORE_REQUIRED` (3030), once per defaulted
parameter. Sema keeps the default in the tree; it does not copy it into a call.

**Construct and destroy.** A function with `#[operator = "construct"]` or `#[operator = "destroy"]` stands only
in an `implementation` that has no generic parameter, of a structure or an enum that the file declares (a type
that the file does not declare is accepted, as its own rules report it); the function has no generic parameter
either. Any other place (a top-level function, a generic implementation or function, an implementation of an alias
or another type) is `LIFECYCLE_OPERATOR_NEEDS_CONCRETE_TYPE` (3028). A `destroy` has no parameter and no return
type, and a `construct` has no return type and a default on every parameter; any other shape is
`LIFECYCLE_OPERATOR_BAD_SHAPE` (3029), once per function. A call of such a function is
`LIFECYCLE_OPERATOR_CALLED_DIRECTLY` (3027): by a bare name that no local and no top-level function of another
meaning hides, by the static form `Type.name(...)` where `Type` is the implemented type, and by a call on a value
(`point.drop()`), decided to be rejected the same way (see Gaps). The automatic call
at creation and destruction is lowered in the ir, not here.

**Type construction call.** A call whose callee is a structure or an enum name of the file, not hidden by a local,
resolves to that type's construct operator instead of an ordinary call: `Point(1.0, 2.0)` is checked against
`construct`'s own parameters, defaults allowed, the same rule a function call follows ("Default parameter values"
above). A type with no `construct` accepts only a call with no argument. A wrong count or a wrong argument type
reuses `ARGUMENT_COUNT_MISMATCH` (3010) and `TYPE_MISMATCH` (3011), never a new code. A plain function sharing a
type's name never reaches this rule: the name table rejects the second declaration as `NAME_ALREADY_DECLARED`
(3003) before any call is checked, so a call site sees one declaration only, the type or the function, never
both. `Type{...}` (the aggregate literal) is unaffected and keeps working on its own rule, as every `construct`
parameter is defaulted.

**Function type value.** A declared type that is a function type, written or reached through an alias of the file,
gets three checks. A literal (integer, float, string, character or boolean) for it is `TYPE_MISMATCH` (3011). A
bare name of a function of the file, not hidden by a local, for it is `FUNCTION_TYPE_MISMATCH` (3053) when its
parameter count, the passing mode or type of a parameter, or its result type differ from the function type; the
check runs wherever a value meets a declared type (a local initializer, an argument, a return, an assignment, a
constant). A call through a parameter or a local of a function type is checked against the parameters of that
function type: a wrong count is `ARGUMENT_COUNT_MISMATCH` (3010) and a wrong argument type is 3011, the rule of an
ordinary call.

**Enum tag type.** `enum A : uinteger8` names the integer type of the discriminant. With no tag it is
`sinteger32`. A tag must be one bare integer primitive (`sinteger8` to `sinteger64`, `uinteger8` to
`uinteger64`); any other tag (`float32`, `string`, `boolean`, a structure, a generic or an alias) is
`INVALID_ENUM_TAG_TYPE` (3031), once per enum. A tag name that is not a type at all also raises
`UNKNOWN_TYPE`. Sema does not use the tag for the layout of an enum: `size_of` of an enum is unchanged.

**Ownership.** A parameter's passing mode is its `semantics` attribute - `value_read_only` (a
copy), `reference_read_only` (a read-only borrow) or `reference_mutable` (a write borrow, the
`EXCLUSIVE_BORROW_CONFLICT`/`INVALID_SPAWN_ARGUMENT` axis); `MAREA_SemaGetParameterSemantics` is
the one place the rest of sema reads it. With no attribute written, a primitive defaults to
`value_read_only`, `owned<T>`, `task<T>` and `vector<T>` default to `reference_mutable` (they move
in and are never a borrow), and any other type defaults to `reference_read_only` - settled rule 2. A
write borrow needs the explicit `reference_mutable`. The former `mutable`/`reference` keywords are
retired, `mutable` stays reserved only for `for each` and `partition`.
A second table, `readOnly`, is built like the references table: `MAREA_SemaBindReadOnly` pushes one entry per binder
when a scope is entered and `MAREA_SemaScopeLeave` pops them. A function parameter is read-only exactly when its
effective passing mode is `reference_read_only`. A `for each` variable and a `partition` part are writable only when
the statement says `mutable` and its source is not read-only. A `with` name is read-only when the source is an
existing value (a name, a field or an element: a borrowed view) and writable when the source is a call. A new local
that reuses a read-only name is a writable shadow. The root of an expression is the name at the bottom of its field and
element chain; a root that is not in the table (a plain local, a by-value parameter, a file name) is writable, so sema
rounds toward valid. `WRITE_THROUGH_READ_ONLY_BORROW` (3069) is raised in four shapes: an assignment of any operator
whose target is rooted in a read-only borrow; `for each ... in mutable` or `partition mutable` over a read-only source,
at the source; a read-only borrow given to a write-borrow parameter; and a call result or a literal given to a
write-borrow parameter (a temporary has no storage to write back). A move-in parameter (`owned`, `task`, `vector`) is
not a write borrow of a place and takes a call result. The rule runs in `MAREA_SemaCheckOwnership` and
`MAREA_SemaCheckWriteArguments`; the typed AST pass only rebuilds the table and reports nothing.
`value_read_only` is legal only on a primitive; writing it on anything else is `INVALID_BORROW`
(3017). `owned<T>` moves in regardless of `semantics`. A primitive, an `array`, `pointer`, `slice`
and an enum is by value for the *ownership mode* (`MAREA_SemaOwnershipMode`) no matter what
`semantics` says - checked before `semantics` is even read, the one place `array`/`pointer`/
`slice` differ from every other non-primitive: the ownership mode is always `VALUE`, so a
`semantics` attribute on one of them only ever affects whether it counts as a borrow for scope
escape (below), never its mode. The enum case is the by-value exception among user types (no
attribute exempts any other user type - the retired `#[value]` key does not exist, so a structure,
move_only or plain, is never by value by itself). A library storage type written exactly `c.array<T>`, `c.vector<T>`, `c.table<K, V>`, `c.owned<T>` or `c.task<T>` is a move type too, so `=` of one needs `copy` or `move`; `c.slice<T>`, `c.pointer<T>` and any other type that sema cannot see stay by value and are never guessed. A `vector` is a move type, so `=` of a `vector`
name needs `copy x` or `move x`. `=` of a name, field or element of any other type needs
`copy x` or `move x`, in an assignment and in a local initializer. A call or a literal needs no word.
A borrow parameter (`reference_read_only`, explicit or by default, or an explicit `reference_mutable`), a
`for each` variable, a `partition` part and a `with` name cannot be returned, nor be the value of
a field. A new local of the same name ends the reference. A file constant passed as the argument of
an explicit `reference_mutable` (write-borrow) parameter has no writable storage to
back the borrow, so it is rejected with `CANNOT_BORROW_CONSTANT_AS_MUTABLE` (3032); a variable
passed to the same parameter is unaffected. An assignment (`=` or a compound form) whose target is a file constant, or a field or element path
rooted in one (`ORIGIN.x = 1;`, `TABLE[0] = 1;`), is `ASSIGNMENT_TO_CONSTANT` (3040), raised at the target; a local,
parameter or `for` variable of the same name hides the constant and may be assigned. Tracking of a moved local and a live borrow across
statements is a design decision recorded in the last bullet of `## Gaps`.

**Integer literal range.** An integer literal (decimal, `0x` or `0b`, with `_` between digits, with one
leading minus and any parentheses) must fit the integer primitive it meets: `sinteger8` to `sinteger64`, `uinteger8`
to `uinteger64`. A literal that does not fit is rejected with `LITERAL_OUT_OF_RANGE` (3033) and is never truncated.
A negated literal never fits an unsigned type, except `-0`. The check runs beside the `TYPE_MISMATCH` check, at
the same sites: a local initializer, a constant, a parameter default, a return value, an assignment, a call
argument (a `spawn` call too, and a call of an `EXTERNAL` member), and a field of a struct literal. A float or any
non-integer target is never range checked, so a node gets 3011 or 3033, never both. A literal whose target has no
known primitive type is checked only against the widest range: `uinteger64`, or `sinteger64` when negated.

**Signed boundary.** Plain `+`, `-` and `*` wrap in two's complement, so only two shapes have no defined result. A unary minus over an
unsigned value is rejected with `NEGATE_UNSIGNED` (3036); a negated literal is the literal range rule, not this one. A `/` or `%` whose
left side folds to the lowest value of a signed type and whose right side folds to -1 is rejected with `SIGNED_DIVISION_OVERFLOW` (3037).
The folder reads an integer literal, one unary minus and parentheses. The signed type is the primitive of an operand, else the declared type
of the `local`, `constant`, `return` or assignment that holds the division.

**Spawn.** In `fork_join`, each argument of `spawn f(...)` is a `partition` part, a literal, a file
constant, or a name that is not a `reference_mutable` (write-borrow) parameter and not the source of
an active `partition mutable`, or a field access (`a.b`, nested too) whose base meets the same rule. Any other expression
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
when `constant MAX: sinteger32 = 100;`. A local or a parameter with the name of a constant hides it. Other
literals, names and unary operators are compared by text (`"a"` and `"a"`, `a.LIMIT` and `a.LIMIT`). Over an enum a bare and a qualified name of one variant (`red`, `Color.red`) are the same
variant, as in the coverage check. Two broken (`ERROR_NODE`) values are never compared. The check runs for
every scrutinee type, and it does not replace `NON_EXHAUSTIVE_SWITCH`: a `switch` over an enum with a repeat
and a missing variant raises both. The rule follows the grammar (`switch_statement` has one optional
`default_arm`) and the arms of a `switch` do not fall through, so a repeated value is an arm that never runs
and a second `default` is an arm that the IR would drop. The place of the `default` is not checked by
sema itself: the grammar puts `default_arm` last, and the parser now enforces that placement
(`DEFAULT_ARM_NOT_LAST`, 2010), so a `default` that is not the last arm never reaches sema through a
program the parser accepts.

**Opaque structures.** An `opaque structure` has no known layout, so sema refuses to build it, to read a field of it,
to apply an operator to it and to hold it by value in a structure. A struct literal of an opaque type, a field read
and a binary operation whose operand is an opaque name raise `OPAQUE_TYPE_MISUSE` (3057), and a `FIELD_DECLARATION`
whose type is the bare name of an opaque structure raises `OPAQUE_FIELD_BY_VALUE` (3058). `owned<T>`,
`pointer<T>`, a borrow and a C call stay legal.

**Expressions.** Sema gives an expression a type only when it is cheap to know, and never guesses. A
literal has its own type. A name has the declared type of its local, parameter or file constant. A field, or an
element of an `array`, `slice`, `vector` or `pointer`, has the type sema already reads for the ownership check
(a primitive only). A built-in `convert<T>(x)` has the type `T`. A `-`, `~`, `copy` or `move` keeps the type of its operand. `not`, `and`, `or` and a
comparison give `boolean`. A shift gives the type of its left side. Any other binary operator, `**` too, gives the
type of both sides when they are the same primitive, or when one side is a number literal and the other a
primitive of its family; it gives no type when they differ. A call of a plain function of the file (not
generic, and not hidden by a local of the same name) has the declared result type. A call whose callee is a
structure or an enum name of the file has that type, as construction ("Type construction call" above). Everything else has an
unknown type and is accepted. The type of an expression is compared with the declared type when that type
is a primitive, at a call argument of a function of the file, a local initializer, a plain `=` assignment, a
`return` value and a constant initializer: a conversion is always written, so two primitives are the same
only by name (`sinteger32` does not fit `sinteger64`), a number literal fits any number type, a fraction literal
any float type, a character literal fits only `character` (a number is not a character, and a character is not a number: it has no arithmetic), and a structure fits no primitive. An alias of a primitive is the primitive. The mismatch is
`TYPE_MISMATCH` (3011) on the expression. A name of an enum, of a library type or of a generic parameter, and an `owned`,
`task` or `vector` value, and an `array` value, are classified as their own shapes but are never compared as a
whole; only their type arguments are (next paragraph).

The type arguments of a generic type are compared with the declared type before the primitive check, at the same
places. When the declared type and the type of the value (a name, a field, an index, a struct literal, with one
`copy` or `move` and any parentheses looked through) have the same head name and the same number of type arguments,
each pair must agree: two primitives by name, two value arguments by text, two nested generic types by this rule. A
difference is `GENERIC_ARGUMENT_MISMATCH` (3014) on the value. The variable of a `for each` has the element type of
the collection: one `owned<...>` is peeled, then the first type argument of `vector`, `array` or `slice`; with two
variables the second is the element and the first, the index, stays untyped. A field read through it is compared
like a field read through a parameter (3011).

**Operands of a binary operator.** Every binary expression in a checked slot (the same slots as the names, and
the initializer of a constant) has its two operands compared, whatever the declared type: `+`, `-`, `*`, `/`,
`%`, `**` and their wrapping and saturating forms, the six comparisons, `&`, `|`, `^`, `and` and `or`. Two primitives
agree only by exact name (`sinteger32` and `float64` do not, and a conversion is always written). A number literal
agrees with a primitive as it fits a declared type, so `a + 1` is accepted for a `float64` and `a + 1.5` is not
for an `sinteger32`. A mismatch is `TYPE_MISMATCH` (3011) on the binary expression, once per expression, and the
expression then has no type, so the declared type of a local or a `return` is not reported a second time. An
operand that is a structure, an enum, a library type, a generic result or any other unknown type is accepted.
A shift is not compared here, as its right side may have another type; it has its own rule below.

**What an operand may be.** A bitwise operator (`&`, `|`, `^`, `~` and `&=`, `|=`, `^=`) with an operand that is a
float (a `float32` or `float64` name or a float literal), a `boolean` or a `string` is `BITWISE_OPERAND_NOT_INTEGER`
(3038); `and`, `or` and `not` with an operand of a known type that is not `boolean`, or a number, string or character
literal, is `LOGICAL_OPERAND_NOT_BOOLEAN` (3039). Each is raised once on the expression, and only when the two operands
already agree in type: a pair that raises `TYPE_MISMATCH` (3011) raises no second code. An operand of an unknown type is
accepted, and a `character` operand of a bitwise operator is accepted.

**Raw pointer.** A `local` whose type is `pointer<T>` is `RAW_POINTER_OUTSIDE_UNSAFE` (3041) unless an `unsafe ... end` block
encloses it. A block nested in the `unsafe` block is enclosed too; the block ends the cover at its `end`. The context
keeps an `unsafeDepth` counter beside `loopDepth`, and the scope mark restores it.

**Local initial value.** A `local` needs a written value: `local x: sinteger32;` is `LOCAL_WITHOUT_INITIAL_VALUE` (3065),
raised at the local, once per local. The rule is syntactic and needs no flow analysis, so a later assignment in a branch
does not change it. The grammar still lets the parser accept the form (the initializer is optional there); sema rejects it.
A local with an `ERROR_NODE` as its name or type is not checked.

**Constant cycle.** A file constant whose initial value reaches the constant itself, directly or through other file
constants, is `CYCLIC_CONSTANT` (3055), raised once for each cycle at the constant of the cycle that comes first in the
file. A name counts as an edge only where it stands as a value: a member name, a field key, a type and a call target are
not followed. A constant that only uses a cycle without being on it is not reported. The search runs once, after the
declaration checks and before the visit, with a visit state kept in the arena and an explicit stack, so a long chain
needs no recursion.

**Compile-time macros.** A `compile_error!(...)` call always raises `COMPILE_TIME_ASSERTION_FAILED` (3059) at the
call, in a function body and at the top level, whatever its arguments are (a wrong argument shape adds 3061 before it). A `static_assert!(condition, "text")` call
with two arguments raises 3059 when its condition is a constant that evaluates to false. A small evaluator reads the
condition: boolean and integer literals (decimal, `0x`, `0b`, `_`), a file constant (read through its initial value,
with a depth limit of 32, so a cyclic constant ends as unknown), parentheses, unary `not` and unary minus, `and`, `or`,
the comparisons `==`, `!=`, `<`, `<=`, `>`, `>=` and `+ - * / %` over 64-bit integers. It answers true, false or
unknown, and an unknown condition is accepted. Overflow, a division by zero, a call, `size_of`, `align_of`, a name that
is not a file constant (a local or a parameter of the same name hides the constant) and every other node are unknown:
the rules of constant arithmetic are other checks, never reported here. A `static_assert!` with another argument count
is skipped. The message holds the text of the string literal argument without its quotes, as
`compile_error: <text>` or `static assertion failed: <text>`, and the prefix alone when there is none.

**Visibility across namespaces.** The default visibility is public: only an explicit `#[visibility = private]` hides a
symbol, a type or a field from another file (a file attribute `#![visibility = private]` hides every symbol of the file,
but never a field, which reads only its own attribute). A whole-namespace import (`import a.b;`, or `import a.b as x;`)
brings no names, so three uses reach into the tree that the resolver answers `FOUND` for. `b.hidden()` or `b.LIMIT`,
where the member is a top-level declaration that is private, is `PRIVATE_SYMBOL_ACCESS` (3066) at the member. A type
written `b.Secret`, in a parameter, local, field, return, generic argument or struct literal, where the structure,
enum, opaque or alias is private, is `PRIVATE_TYPE_ACCESS` (3067) at the type. A private field of a structure that is
not private, read or written as `box.value` where `box` is a local or parameter written `b.Box`, or set in a literal
`b.Box{ value = 1 }`, is `PRIVATE_FIELD_ACCESS` (3068) at the field name; a private structure is 3067 at its written
type and never also 3068. A local or a generic parameter that hides the namespace name is no namespace access. A
namespace that the resolver answers `EXTERNAL` or `NOT_FOUND` is never checked, and neither is a field of the same
file. An import of one symbol or a member list is checked the same way: a private symbol is `UNRESOLVED_IMPORT`.

**Built-in macro.** `MAREA_SemaCheckMacro` (`MAREASemaMacro.c`) checks every `MACRO_CALL` against one table of the 13
built-in macros, the table of `examples/macros/builtin.marea`. A name that is not in the table is `UNKNOWN_MACRO` (3060),
raised at the call. A call with the wrong arguments is `INVALID_MACRO_ARGUMENT` (3061), raised at the offending argument,
or at the argument list when the count is wrong; at most one diagnostic is raised for a call, and a call with an
`ERROR_NODE` among its arguments raises nothing. The message names the macro.

| Macro | Arguments | Value |
|---|---|---|
| `assert`, `debug_assert` | exactly 2: a condition, a message that is a string literal | none |
| `unreachable` | exactly 0 | none |
| `compile_error` | exactly 1: a string literal | none |
| `static_assert` | exactly 2: a condition, a string literal | none |
| `include_bytes`, `include_text` | exactly 1: a string literal | `c.array<uinteger8>` (not typed here), `string` |
| `format` | 1 or more: a string literal, then the values | `string` |
| `print` | 1 or more: a string literal, then the values | none |
| `log` | 2 or more: a level name, a string literal, then the values | none |
| `line`, `file`, `namespace` | exactly 0 | not typed (`line`), `string` |

Four decisions bind the table. (1) The log levels are the five names `trace`, `debug`, `info`, `warning` and `error`
(a bare name, not a string); the language docs did not list them, so the list is an assumption that the user must
confirm. A level that is not a bare name of the list is rejected, a literal included. (2) A condition or a text argument
is rejected only when it is a literal of the wrong kind: an integer, float, boolean or character literal for a text; an
integer, float, string or character literal for a condition. Any other expression has an unknown type and is accepted.
(3) The `{}` check runs when the format text is a string literal: it counts the occurrences of the two characters `{}`
and compares the count with the number of value arguments. A lone `{` or `}` is skipped. (4) `line!()` is not typed;
`file!()` and `namespace!()` are `string`.

The value of a macro is part of the type check. A macro with no value has a shape that fits no type, so it is
`TYPE_MISMATCH` (3011) where a primitive is expected: the initial value of a `local` (`local n: sinteger32 = print!("x");`),
a returned value, an argument of a call or an operand. A macro that gives a `string` fits a `string` and is a 3011 for
any other primitive (`return format!("x");` in a function that returns `sinteger32`). A macro statement is not a value
use and raises nothing, and the typed tree is not changed.

**Condition type.** The condition of an `if`, an `elseif` or a `while` must be a `boolean`: a condition with a known
primitive type other than `boolean`, or a number, string or character literal, is `NON_BOOLEAN_CONDITION` (3043), raised
at the condition once. `true`, `false`, a comparison, `and`, `or`, `not` and a `boolean` name pass. A `switch` subject and
an expression statement are not conditions and are not checked.

**No implicit numeric conversion (decision of the user).** A calculation never converts a
number by itself: the two operands of an arithmetic or a comparison operator have the same primitive type, and
`**` follows the same rule as `+`. An integer base with a float exponent is `TYPE_MISMATCH`, whatever the declared type of the result, and the result of a power has the type of its operands.

**Shifts (decision of the user).** The count of `<<` and `>>` may be any integer primitive of
any width (`sinteger8` to `sinteger64`, `uinteger8` to `uinteger64`, or an integer literal), and the result has the
type of the left operand. A count or a left operand that is a known type and not an integer (a float, a
`boolean`, a `string`, a fraction literal) is `TYPE_MISMATCH` (3011) on the shift, once, and the shift then has
no type. An operand of an unknown type is accepted.

**Compound assignments (decision of the user).** `+=`, `-=`, `*=`, `/=`, `%=`, `**=`, `&=`,
`|=` and `^=` need the same type on both sides, as `a = a + b` does, with the same literal rule as a binary
operator. `<<=` and `>>=` follow the shift rule. A mismatch is `TYPE_MISMATCH` (3011) on the assignment, once.
A side of an unknown type is accepted.

**Conversion `convert<T>(x)` (decision of the user).** A conversion is always written and
never implicit. The call is the built-in when it names exactly one type argument and has exactly one argument and
the unit binds no name `convert` of its own (a function, an import, a parameter or a local with that name makes
it a plain call, as the IR reads it). Its result type is `T`, even when the conversion itself is rejected, so it
feeds every other check of this section (a local, a parameter, a `return`, a constant, an operand, a compound
assignment). Allowed:

- a number primitive to a number primitive of any width and sign: integer to integer, integer to float, float to
  integer and float to float (`sinteger8` to `uinteger64`, `float64` to `sinteger32`). A number literal and a
  calculation of numbers are number operands. `boolean` and `string` are not numbers;
- the same primitive to itself (`convert<string>(text)`, `convert<boolean>(flag)`);
- a `structure` to another `structure` that has the same layout (below), and a structure to itself;
- an operand or a target of a type sema does not know (an enum, a library type, a generic parameter, a `pointer`,
  a field or a call that sema cannot type). An unknown type stays accepted.

Everything else is `TYPE_MISMATCH` (3011) on the call, once: a `string` or a `boolean` to a number or the other
way, a structure to a primitive, a primitive to a structure, and a structure to a structure with another layout.

Two structures have the same layout when every field has the same primitive type, by exact name, or a structure of
the same layout (below), at the same place (an alias of a primitive is the primitive, the names of the fields do not
count), the same field `align`, they have the same alignment and, when both are `#[layout = c]`, the same size.
Sema computes the alignment and the size by the rules of C: each field at the next multiple of its own alignment
(the size of a primitive, the alignment of a nested structure), `pack = N` caps that alignment, a field
`align = N` raises the alignment of the field, `align = N` on the structure raises the alignment of the structure,
and the size is rounded up to the alignment. The layout may come from a file attribute. A structure held by value
is measured too, up to 8 levels deep, and only inside a `c` structure when it is `c` itself. Two `marea` structures
(the default layout) with the same fields, `pack` and `align` have the same layout, because the backend gives the
same input the same layout. A pair that sema cannot measure is a `TYPE_MISMATCH` (3011): a generic structure, a
field that is not a fixed-width primitive (a `character` is one byte) or a structure of the file (`string`, `pointer`, `array`, `vector`, an enum, a
library type), a `pack` or `align` value that is not an integer from 1 to 4294967296, a `marea` structure inside a
`c` structure, a `c` structure against a `marea` one, a chain of structures deeper than 8 levels (or one that holds
itself). A structure converted to itself needs no measure and is accepted.

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
A member name, a field key of a literal value, a type and a macro call are not checked. The bare name that
starts a member chain (`stdio.puts(text)`) is checked like any other name, so a namespace that no import binds,
or that only the original name of an alias reaches (`import c.stdio as output;` then `stdio.puts(text)`), raises
`UNDECLARED_NAME` (3004). The local that a
`for`, `for each`, `partition` or `with` name hides is never used to type the name.
A function is declared before its use: a bare function name inside a function body that stands before the declaration
of that function in the file raises `UNDECLARED_NAME` (3004) with the message "function is used before its declaration".
There is no forward declaration. A recursive call is fine, a local of the same name hides the function, and a
constant initializer is not checked for it.

**Entry point.** Sema treats `main` as an ordinary function: any signature is accepted and no diagnostic is
raised. Sema never requires a `main`. The code 3023 (`INVALID_MAIN_SIGNATURE`) is retired and stays reserved, never
reused.

**Constant operands.** `/`, `%`, `/=` and `%=` with a divisor that is a constant zero raise `DIVISION_BY_ZERO` (3034)
when the left side is an integer (a primitive integer type or an integer literal). A shift or compound shift whose
count is a constant that is negative or not below the width in bits of the left primitive integer type (`value << 32`
for `uinteger32`) raises `SHIFT_COUNT_TOO_LARGE` (3035). A constant is an integer literal (decimal, `0x`, `0b`, `_`
digits), one leading minus on a constant, or a parenthesized constant. A float divisor, a left side with no known
primitive type (a literal) for a shift, and a divisor or count that is not a constant raise nothing.

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
same rules "Ownership" above already states: `VALUE` (a primitive, an `array`/`pointer`/`slice`
core type - checked before `semantics`, so no attribute changes it - or an enum, the documented,
intentional exception; no attribute exempts any other user type), `BORROW` (a parameter whose
effective `semantics` is `reference_read_only`, a `for each` variable, a `partition` part or a
`with` name; this is also a parameter with no attribute, whose type is not by value and does not
move in), `MUTABLE_BORROW` (a parameter whose effective `semantics` is an explicit
`reference_mutable`), `MOVE` (an `owned<T>` or `task<T>` parameter or value), `COPY` (a `copy x`
expression). A node this cannot classify defaults to `VALUE`.

**Call results.** A call expression's typed node carries the type the call returns. A call of a function of the
file carries the callee's declared return type: `USER`, `EXTERNAL` or a built-in class. A construction call, whose
callee names a structure or an enum of the file, carries that type itself: `USER` with the structure or enum
declaration, never the `construct` function that checked its arguments. A built-in `convert<T>(x)`
(with no name `convert` bound in the unit) carries the type `T`, read in the scope of the caller. When the callee is
generic and the call gives explicit type arguments, a return type that is one generic parameter (`Item`) is
replaced by the type argument at the same place (`same<sinteger32>(5)` is `sinteger32`). When there are no type
arguments, or the return type is a compound of a parameter (`c.array<Item>`), the typed node keeps the raw type: a
bare parameter is tagged `GENERIC` with its name - never a guess, never a crash. Resolving a result from inferred
type arguments is deferred to when the `ir` stage needs one for real.

**Resolved type.** `MAREA_SemaTypedNodeGetResolvedType(node, &text, &length)` gives the canonical full text of the
type that a node declares in the source, with its type arguments, so a later stage never sees only a head name. The
text is built while the typed tree is built, lives in the arena and is not NUL terminated. Six node kinds
declare a type: `PARAMETER`, `LOCAL_DECLARATION`, `FIELD_DECLARATION`, `VARIANT_FIELD`, `CONSTANT_DECLARATION`
and `FUNCTION_DECLARATION` (its result type). A literal has a type too, from its context (see "Resolved type of a
literal" below), and an expression has one by the rules of "Resolved type of an expression". Any other node, a
function with no result type and a written type that sema cannot spell return
`MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE` and clear both outputs; the accessor never
guesses. A null node, text or length is `ERROR_NULL_POINTER`. The accessor is read-only and gives the same text on
every run. The canonical spelling:

- Type arguments follow the name in angle brackets, joined by a comma with no space: `array<sinteger32,4>`,
  `vector<array<sinteger32,4>>`, `vector<Point>`. An integer argument is its text.
- A function type is `function(` the parameter types joined by a comma `)`, then `:` and the result type when there
  is one. A parameter name is never part of it: `function(sinteger32,sinteger32):sinteger32`, `function(string)`.
- A dotted name is kept whole: `c.array<string>`.
- An import alias is replaced the way the `ir` does it (see [the IR doc](../ir/marea-ir.md), "Aliases"): a whole-path
  alias by the last segment of its path, a member alias by the member path as written, and the head of a dotted name
  that is an import alias by that original, with the tail kept. Written type arguments are kept after it: with
  `import c.collections.list as lst;`, `lst<sinteger32>` is `list<sinteger32>`. An import without `as` stands for itself.
- A local type alias without generic parameters, written with no type arguments, is replaced by the type it names, and
  a chain is followed up to 8 steps, as in the `ir`. Unlike the `ir`, which keeps only the head name, the arguments of
  the target are kept: with `type Grid = array<Point, 4>;`, `Grid` is `array<Point,4>`. A generic alias is kept as
  written.
- A generic parameter in scope is its own name: `Item`.
- `void` is the text of the type of a call that returns nothing (see the call row of "Resolved type of an expression"). It is
  only a spelling: it is not a type of the language, no declaration can write it, and no declared node carries it.

**Resolved type of an expression.** The same accessor gives the type of an expression, in the same spelling. The text
is made when the typed tree is built, after the typed children, so it uses what the children carry. A name that is
not read as a value (a declared name, including the name that a loop or a scope declares, a member name, a field key) has no
type. The rules:

| Expression | Resolved type |
|---|---|
| arithmetic, bitwise and power operation | the type of the operands. An operand that is a number literal (alone, in parentheses, with a sign, or in an operation of such numbers) never decides: the first other operand with a type does |
| comparison, `and`, `or`, `not` | `boolean` |
| shift | the type of the left operand |
| `-` and `~` | the type of the operand |
| `copy x`, `move x`, `(x)` | the type of `x`, with its type arguments |
| `spawn call` | `task<T>`, where `T` is the type of the call |
| identifier | the type of its declaration: a parameter or a local, a constant of the file or of the implementation block, a value parameter such as `Count` in `<constant Count: sinteger32>`, or a function name as its function type |
| name of a loop or a scope | read as a value, it has the type its statement gives it. `for i in a .. b`: the type of the bounds, as for an operation (a number literal never decides, two literals give the type of the first). `for each key, value in c`: the value is the element type of `array`, `vector` or `slice` (also under `c.`), the key is `uinteger64`. `partition v into n as part`: `slice<T>`, where `T` is the element type of `v`. `with e as name`: the type of `e`. The name has no type when the type that decides it has none |
| member access | the declared type of the field. On a value of a generic structure, the type arguments of the value replace the parameters of the structure. An enum variant (`Color.Red`) is the enum. A field of a variant of an enum is read on the enum value. `Type.NAME` is a constant of an implementation block. A method or a static function, named with no call, is its function type with the receiver first: `function(Vector2,float64)` |
| index access | the first type argument of `array`, `slice`, `vector` or `pointer` (also under `c.`) |
| call | the declared result type of the callee. `convert<T>(x)` is `T`. A call on the name of a structure or enum of the file is that type. A call of a method, a static function, a function value or a local alias of a function type is the result of that function. A call of one of these with no result type is `void`, and `spawn` of such a call keeps no type. `size_of<T>()` and `align_of<T>()` (with no name of the same text bound in the unit) are `uinteger64`; the names `size_of`, `align_of` and `convert` as a callee keep no type of their own. A call of a generic function of the file, by its bare name and with no written type arguments, has its type arguments inferred (see "Inferred type arguments" below); a method call on a receiver whose type is a generic parameter in scope has the result of the contract function its where clause lists for it (see "A method on a generic receiver") |
| structure, array or indexed literal | the written type (`Point`, `array<float32,4>`) |

A value that has `owned<T>` as type gives its members and elements as `T` does. A type written for another declaration
is read with the type arguments that fill it: a generic function takes the explicit type arguments of the call
(`make<sinteger32, float32>(x, y)` gives `Pair<float32,sinteger32>` for a result written `Pair<Second, First>`), a
generic structure takes the type arguments of the value, and `Self` is the implemented type. When a parameter has no
argument to fill it, the node has no type: sema never guesses a type argument, except for the one inference below. A
node with no type returns `MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE` and clears both outputs, as for a declaration.
The literals have their own rule, below. An
operation takes its type from the types its children have at the end of the build, so `local x: sinteger64 = 1 + 2;`
gives the binary expression `sinteger64` and not the default `sinteger32` of its literals: when a context changes the
type of the literals below an operation, the binary, unary and parenthesized expressions below that context are typed
again.

**Nodes that stay without a type.** An expression node with no resolved type is in one of nine closed classes (a macro
call and what it holds, a written generic type that qualifies a member, the built-in callee names, a function name or
member that has no function type, a call of an unknown function, the name of a `with` or `for` statement that an
untyped expression decides, an operation with an untyped operand, and `spawn` of a void call). "Gaps" lists the exact
classes and their counts on the 23 examples, and the sweep test fails for any other untyped expression node. A literal
is never in the list.

**Qualifier names.** An `IDENTIFIER` that is the left side of a member access and only qualifies the member is not a
value. `MAREA_SemaTypedNodeIsQualifier` is true for it, and it has no resolved type, so it is not an expression that
misses a type. No local, parameter or generic parameter may have its name, and it is one of these: the name of a
structure, an enum or an opaque type of the file (`Planet` in `Planet.Earth`, `Vector2` in `Vector2.origin()`); a
namespace import or its alias, whatever the resolver answered (`stdio`, `lib`); an imported enum, or a type that a
`FOUND` namespace declares and the file imports by name; a name with no declaration at all, which sema takes for a
namespace (`stdio.puts` with no `import`). A function or a constant imported by name is a value, not a qualifier. Only
an `IDENTIFIER` is flagged: a dotted type name such as `c.array` is not an expression. The member after a qualifier is
typed by the rules above (an enum variant, a constant or a static function of the file) or by the rule below.

**Members of a FOUND namespace.** When the name on the left of a member access stands for a namespace that the
resolver answers `FOUND` (an import with or without an alias), the member is typed from the tree that the resolver gave.
Only a function that is not `#[visibility = private]` (its function type, so a call gives the declared result) and a
constant (its declared type) are typed. A bare name that an import bound to one such symbol (`import a.b.c as d;`,
`import a.b { c };`) is typed the same way. A type written in that other file is read in that file, never in this one.
A structure, an enum, an opaque type or an alias that the file declares is spelled with the last segment of its
namespace (`shapes.Box`, as a file writes `structures.Shape`), a local alias is followed to its target, and any other
name (a built-in) is kept as written. For a generic function the type arguments written on the call fill the
parameters, and a call with none has no type. A field or a method that is read on such a result has no type, because
the name would be looked up in this file. A type, an enum variant and a symbol that is private have no type here,
and neither has a member of an `EXTERNAL` namespace.

**Inferred type arguments.** A call of a generic function of the file, by its bare name, with no written type
arguments, unifies each parameter whose written type is a bare generic parameter (`Item`) or a compound of one
(`array<Item,N>`, `vector<Item>`) against the resolved type text of the typed argument at the same place: a bare
parameter binds the whole argument text, a compound parameter splits head and arguments and binds the generic part
that the argument text has at the same place, each one recursively. The first binding of a name wins; a second
binding of the same name that reads a different text makes the whole call keep no inferred type, the same neutral
outcome as before this inference existed - the checker already rejects the program where a mismatch matters, so this
never raises a diagnostic of its own. When the result type is itself one bound bare generic parameter, its binding's
text becomes the call's resolved type (`maximum(a, b)` with `sinteger32` arguments is `sinteger32`); any other result
shape (unbound, compound, or a call that already gives explicit type arguments, which substitutes as "Resolved type
of an expression" above already describes) is untouched. A call reached through a `FOUND` namespace never infers:
"Members of a FOUND namespace" above already fixes its own rule, a call with no type arguments has no type.

**A method on a generic receiver.** A member access whose base is a value of a generic parameter type in scope
(`left` of type `Item`, with no frame bound it to a concrete type) has no structure or enum to read a field or a
method on, so it is typed through the contracts its own where clause names for that parameter instead: the first
contract that declares a function of the member's name gives that function, read as the result of a call the same
way an implementation member is (`left.less(right)` with `where Item: Comparable` is `boolean`, the result
`Comparable.less` declares). The where clause searched is the one of the function or the implementation block the
expression is inside; a parameter with no such member in any of its contracts, or a receiver that is not a bare
generic parameter (a frame already bound it, or it names a structure, an enum or a built-in instead), falls back to
the existing rules above and is typed the usual way, or not at all.

**Resolved type of a literal.** An `INTEGER_LITERAL`, `FLOAT_LITERAL`, `STRING_LITERAL`, `CHARACTER_LITERAL` and
`BOOLEAN_LITERAL` has a resolved type too, and the accessor never gives `ERROR_NO_TYPE` for one. A string, a
character and a boolean literal are always `string`, `character` and `boolean`. A number literal takes the primitive
type its context fixes, spelled as written in the source (the name behind a local type alias, never the alias). The
context is read in this order, the nearest one wins, and a context fixes the literal only when it is a primitive that
the literal fits: an integer literal fits any number type, a float included, as `TYPE_MISMATCH` accepts it, and a float
literal fits `float32` and `float64` only.

- The declared type of the local, the constant or the variant field that the literal initializes.
- The type of the parameter that an argument is passed to, and of the parameter that a default value belongs to. A
  call of a structure or an enum uses the parameters of its `construct` operator.
- The result type of the function, for `return`.
- The target of an assignment, a compound assignment included, except the count of `<<=` and `>>=`.
- The other operand of an arithmetic, bitwise or compare expression, of the two bounds of a `for` range, and of a
  `case` value against the switched value, when sema knows its primitive type. The type of a context passes down
  through a parenthesis, a unary `-` or `~`, and an arithmetic or bitwise operator (the left side of a shift only),
  so `local x: sinteger8 = (1 + 2) * 3;` types all three literals `sinteger8`.
- The field type of an entry `name = value` of a structure literal, and the element type (the first type argument) of
  an entry of an `array`, `vector` or `slice` literal, for a plain entry and for the value of `[key] = value`.
- The target type `T` of `convert<T>(x)` for a literal `x`.

A number literal with no fixing context takes the default: `sinteger32` for an integer literal and `float64` for a
float literal. The repository named no default before this rule, so these two are the assumption of the plan, not a
language rule from the grammar. This holds for a literal that is not a value of an expression too: the size in
`array<sinteger32,4>` and the integer in an attribute are `sinteger32`. The key of `[key] = value` and the index of
`a[i]` take the default. The rule only records a type: it raises no diagnostic and changes nothing that sema accepts
or rejects.

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 3001 | `UNRESOLVED_IMPORT` | `import a.b.c;` when the resolver finds neither `a.b.c` nor a `c` that is not private in `a.b`; `import a.b { f };` when `a.b` is not found, `f` is a private symbol of it or is missing, a two-part member is not a variant of an enum that is not private, or the member path has more than two parts |
| 3002 | `NAMESPACE_PATH_MISMATCH` | `namespace a.b.c;` in the file `a/b.marea` |
| 3003 | `NAME_ALREADY_DECLARED` | `structure Item end function Item() end`, or an import (or an import alias, or a member alias) and a declaration or another import of one local name; an import alias or a `type` named `c` (message `name is reserved:`); an import alias named like a primitive type, `import c.foo as sinteger32;` (message `name is a primitive type:`); a generic parameter that repeats an earlier one of its list or names a top-level declaration, `pick<Item, Item>`; two parameters of one name; a body local that repeats a parameter or a local of the same block |
| 3004 | `UNDECLARED_NAME` | `where Item: Missing`, or a constraint on a name that is not a generic parameter; `local x: sinteger32 = missing;`, `missing(1);`, `missing.field`, `stdio.puts(x)` with no import of `stdio`; a bare function name that stands in a function body before the function is declared in the file |
| 3005 | `UNKNOWN_ATTRIBUTE_KEY` | `#[frobnicate = true]`, `#[simd]`, `#[public]` |
| 3006 | `INVALID_ATTRIBUTE_VALUE` | `#[inline = maybe]`, `#[build = 1]`, `#[visibility]`, `#[export]` |
| 3007 | `CONFLICTING_ATTRIBUTES` | `#[pack = 1, align = 16]`, a key given twice (`visibility` twice included) |
| 3008 | `INVALID_ALIGNMENT` | `#[align = 15]` |
| 3009 | `UNKNOWN_TYPE` | `a: Missing;` in a structure |
| 3010 | `ARGUMENT_COUNT_MISMATCH` | `add(1)` when `add` takes two parameters with no default; `f()` or `f(1, 2, 3)` when `f` has one required and one defaulted parameter; `Point(1.0, 2.0, 3.0)` when `construct` takes two; `Line(1.0)` when `Line` declares no `construct`; `power(1.0)` when the host's signature of `c.math.pow` takes two parameters |
| 3011 | `TYPE_MISMATCH` | `convert<sinteger32>(text)` with a `string` name, `convert<Point>(other)` for two structures with another layout, `f("text")` when the parameter is `sinteger32`, `local x: boolean = 1;`, `local n: sinteger32 = a < b;`, `f(s)` with a `string` name `s`, `return a + 1;` in a function that returns `boolean`, `a + b` with an `sinteger32` and a `float64`, `Point("x", 2.0)` when `construct`'s first parameter is `float32`; `power("two", 10.0)` when the host's signature of `c.math.pow` takes `float64, float64`; `local x: sinteger8 = item.x;` over a `for each` whose element field is `sinteger32` |
| 3012 | `DUPLICATE_MEMBER` | two `implementation` blocks of one type that both declare `area`; a structure field, an enum variant or a contract function that is declared twice in its declaration |
| 3013 | `CONTRACT_NOT_SATISFIED` | `maximum(a, b)` when the type of `a` has no `less` for `where Item: Comparable` |
| 3014 | `GENERIC_ARGUMENT_MISMATCH` | `Pair<sinteger32>` when `Pair` has two parameters, `Box<4>`, `local x: array<sinteger8, 4> = values;` when `values` is `array<sinteger32, 4>` |
| 3015 | `INSTANTIATION_TOO_DEEP` | a type argument nested 65 levels |
| 3016 | `ASSIGNMENT_NEEDS_COPY_OR_MOVE` | `a = b;` when the type is not a primitive, a copyable `array` or an enum; a library storage type (`c.array`, `c.vector`, `c.table`, `c.owned`, `c.task`) needs the word too |
| 3017 | `INVALID_BORROW` | `#[semantics = value_read_only] s: Shape` when `Shape` is not a primitive |
| 3018 | `REFERENCE_ESCAPES_SCOPE` | `return s;` when `s` is a borrow parameter |
| 3019 | `EXCLUSIVE_BORROW_CONFLICT` | `take(s, s)` when one parameter's effective `semantics` is `reference_mutable` |
| 3020 | `INVALID_SPAWN_ARGUMENT` | `spawn g(m)` in `fork_join` when `m`'s effective `semantics` is `reference_mutable` |
| 3021 | `NON_EXHAUSTIVE_SWITCH` | a `switch` over an enum that misses a variant and has no `default` |
| 3022 | `MISSING_RETURN` | `if a then return 1; end` as the whole body of a function with a return type |
| 3023 | retired (`INVALID_MAIN_SIGNATURE`) | reserved, never reused; sema no longer checks the signature of `main` |
| 3024 | `INVALID_PARALLEL_JUMP` | `continue;`, `break;` or `return;` in a `partition` body when no loop of the body is the target, or directly in a `fork_join` block, outside every `partition` body, when no loop of the block is the target |
| 3025 | `DUPLICATE_SWITCH_ARM` | a second `default` arm in a `switch`, or `case 1 then ... case 1 then ...`, `case 1, 1 then ...`, `case Color.red` twice |
| 3026 | `JUMP_OUTSIDE_LOOP` | `break;` or `continue;` with no enclosing loop and no enclosing `partition`/`fork_join` block |
| 3027 | `LIFECYCLE_OPERATOR_CALLED_DIRECTLY` | `Point.make()`, `make()` or `point.make()` when `make` carries `#[operator = "construct"]` or `"destroy"` |
| 3028 | `LIFECYCLE_OPERATOR_NEEDS_CONCRETE_TYPE` | a `construct` or `destroy` function at top level, in a generic implementation, with generic parameters, or in an implementation of a type that is not a structure or an enum |
| 3029 | `LIFECYCLE_OPERATOR_BAD_SHAPE` | a `destroy` with a parameter or a return type; a `construct` with a return type or a parameter that has no default |
| 3030 | `DEFAULT_PARAMETER_BEFORE_REQUIRED` | `function f(a: sinteger32 = 0, b: sinteger32)`: a parameter with a default value before one with none |
| 3031 | `INVALID_ENUM_TAG_TYPE` | `enum A : float32`, `: string`, `: SomeStruct`: a tag that is not an integer primitive |
| 3032 | `CANNOT_BORROW_CONSTANT_AS_MUTABLE` | `take(ORIGIN)` when `take`'s parameter's effective `semantics` is `reference_mutable` and `ORIGIN` is a file constant |
| 3033 | `LITERAL_OUT_OF_RANGE` | `local a: uinteger8 = 256;`, `local b: sinteger8 = -129;`, `constant BIG: uinteger8 = 300;`, `local c: uinteger32 = -1;`: an integer literal whose value does not fit the integer type it is bound to |
| 3034 | `DIVISION_BY_ZERO` | `x / 0`, `x % 0`, `x /= 0`, `x %= 0` for an integer: the divisor is a constant zero |
| 3035 | `SHIFT_COUNT_TOO_LARGE` | `x << 32` for `uinteger32`, `x >>= 64` for `sinteger64`, `x << -1`: a constant shift count that is negative or not below the width of the left integer type |
| 3036 | `NEGATE_UNSIGNED` | `-value`, `-(a + b)` or `-point.count` when the operand has a known unsigned primitive type |
| 3037 | `SIGNED_DIVISION_OVERFLOW` | `-2147483648 / -1` or `-128 % -1` when the left side folds to the lowest value of the signed type and the right side folds to -1 |
| 3038 | `BITWISE_OPERAND_NOT_INTEGER` | `a & b`, `a \| b`, `a ^ b`, `~a`, `a &= b` with a `float32` or `float64`, a `boolean` or a `string` operand, or a float literal |
| 3039 | `LOGICAL_OPERAND_NOT_BOOLEAN` | `a and b`, `a or b`, `not a` with an operand whose known type is not `boolean`, or a number, string or character literal |
| 3040 | `ASSIGNMENT_TO_CONSTANT` | `LIMIT = 11;`, `LIMIT += 1;`, `ORIGIN.x = 1;`, `TABLE[0] = 1;` when the root name is a file constant and no local hides it |
| 3041 | `RAW_POINTER_OUTSIDE_UNSAFE` | `local raw: pointer<sinteger32> = f();` with no `unsafe ... end` block around it, at any depth of nesting |
| 3043 | `NON_BOOLEAN_CONDITION` | `if n then`, `elseif s then`, `while 1 do` when the condition has a known primitive type that is not `boolean`, or is a number, string or character literal |
| 3053 | `FUNCTION_TYPE_MISMATCH` | `apply(negate)` when `negate` takes one parameter and the function type takes two |
| 3055 | `CYCLIC_CONSTANT` | `constant A: sinteger32 = B;` with `constant B: sinteger32 = A;`, or `constant A: sinteger32 = A;`, once for each cycle, at the first constant of it in the file |
| 3056 | `RESERVED_MACRO_NAME` | `function print() end`, `constant format: sinteger32 = 1;`, `local log: sinteger32 = 1;` or `import c.stdio as print;`: a name that is a built-in macro |
| 3057 | `OPAQUE_TYPE_MISUSE` | `Texture{ }`, `texture.width` or `texture + other` when `texture` is a local or a parameter of an `opaque structure` of the same file |
| 3058 | `OPAQUE_FIELD_BY_VALUE` | `structure Holder texture: Texture; end` with `Texture` an `opaque structure` of the same file |
| 3059 | `COMPILE_TIME_ASSERTION_FAILED` | `compile_error!("stop");` with any arguments, at the top level or in a function body; `static_assert!(1 == 2, "never");` or `static_assert!(N == 5, "n");` when the condition is a constant that is false |
| 3060 | `UNKNOWN_MACRO` | `shout!("hello");` when `shout` is none of the 13 built-in macros, at the call |
| 3061 | `INVALID_MACRO_ARGUMENT` | `print!();`, `print!(1, 2);`, `print!("{} {}", 1);`, `assert!(1, "x");`, `unreachable!(1);`, `log!(loud, "x");`, `include_text!(1);`: the wrong count of arguments, a literal of the wrong kind, a log level that is not one of the five, or a count of `{}` that differs from the count of values, at the offending argument |
| 3065 | `LOCAL_WITHOUT_INITIAL_VALUE` | `local x: sinteger32;` with no `= value`, for a primitive, a structure, a `pointer` or an `array`, at any depth of nesting |
| 3066 | `PRIVATE_SYMBOL_ACCESS` | `b.hidden()` or `b.LIMIT` after `import a.b;` (or an alias) when the member is a top-level declaration of a `FOUND` namespace with `#[visibility = private]` |
| 3067 | `PRIVATE_TYPE_ACCESS` | `s: b.Secret`, `b.Secret{ }` or any written `b.Secret` when the structure, enum, opaque or alias of a `FOUND` namespace is private |
| 3068 | `PRIVATE_FIELD_ACCESS` | `box.value`, `box.value = 1` or `b.Box{ value = 1 }` when `box` is written `b.Box` and the field has `#[visibility = private]` |
| 3069 | `WRITE_THROUGH_READ_ONLY_BORROW` | `function f(p: Point) p.x = 1; end`, `for each v in mutable xs do end` with `xs` read-only, `write_point(p)` with a read-only `p`, `write_point(make_point())` or `set(1)` for a write-borrow parameter |

## Gaps

- A write through a method call on a read-only receiver, and a library call that mutates (`values.append(x)`), are
  not seen: sema does not know which calls mutate. The rule 3069 reads only a written assignment, a mutable loop or
  partition and a write-borrow argument.
- A `with` over a call is writable and a `with` over an existing value is read-only, whatever the type of the value:
  the rule reads the shape of the source, not what the call returns.
- The visibility check (3066 to 3068) reads only a bare namespace name and a bare name with a written type `ns.T`. It does
  not check a member that the tree does not declare, a field of a value that is not a plain `ns.T` name (a call result,
  an index, a chain such as `a.b.c`), a field of a type that an imported symbol gives (`import a.b.Box;`), or a private
  symbol that a generic or an alias leads to.

- Two functions that call each other cannot be written, because a function must be declared before its use and the
  grammar has no forward declaration.
- The name of a `for`, `for each`, `partition` or `with` statement is not checked against the names of its own
  scope: it can repeat a local of the same block, and a local of its body can repeat it.
- No check of the signature of `main` exists any more, and a driver-level replacement does not exist yet.
- `array` and `vector` are built-in names now, so a library that exports a type named `array` is shadowed by the
  built-in (an import never wins over a built-in name). A growable library type is spelled under the `c.{lib}`
  namespace for now. A `vector` is still indexable in the type inference, though it is a move type.
- Renaming on use inside one file (without an import) is not offered: a local declaration has its own name.
- `TYPE_MISMATCH` (3011) sorts the type of an expression into a fixed set of shapes: a literal, a primitive, a
  structure, and, known but never compared, an enum, a library type, a generic parameter and an `owned`, `task`, `vector` or `array` value. Still fully unknown: a `pointer`, a `slice`, the type of a field or a file
  constant that is not a primitive, and the result of a generic call (see below).
- The type arguments of a generic type are compared (3014) only for two types of one head name and one argument
  count. A pair that mentions a generic parameter in scope, an alias, an import or an unknown type, a different head
  (`owned<vector<T>>` into `vector<T>` is accepted), and a `for each` over a collection of unknown type, are skipped.
- A function type is compared (3011, 3053) only for a literal and for a bare name of a function of the file. It is
  still unknown for a field, a call result and any other expression of a function type. A generic function, and a
  function type that mentions a generic parameter in scope, are never compared.
- A binary operator is checked against what its operands may be only for a bitwise operator on a float, a boolean
  or a string (3038) and for `and`, `or` and `not` on a known type that is not `boolean` (3039). Every other operator
  (arithmetic on a `boolean` or a `string`, a comparison of two booleans) is not checked, a `character` operand of a
  bitwise operator is accepted, and an operator that is declared for a structure is not compared.
- `RESERVED_MACRO_NAME` (3056) is not raised for a parameter, a `for`, `for each`, `partition` or `with` name, or an
  import alias with no `as`, even when it is named like a built-in macro.
- The `for`, `for each`, `partition`
  and `with` header expressions and the initializer of a file constant are checked. The loop name is already in
  scope when its own header is checked, so `for i in 0 .. i` with no outer `i` raises nothing. A constant that has
  an `ERROR_NODE` below it has no name check, because the parser may leave its tokens as an expression.
- `OPAQUE_TYPE_MISUSE` (3057) sees only an operand or a base that is a bare name, or one in parentheses, whose
  written type is an opaque structure of the same file. A call result, an index, a field of another value and an
  opaque type of another file are not classified, and `OPAQUE_FIELD_BY_VALUE` (3058) likewise sees only a bare
  opaque name of the same file as a field type.
- `CANNOT_BORROW_CONSTANT_AS_MUTABLE` (3032) sees only a bare file-constant name as the argument. A field chain
  rooted in a constant (`ORIGIN.x`) passed to an explicit `reference_mutable` parameter is not rejected, and a test proves only the
  bare name.
- `ASSIGNMENT_TO_CONSTANT` (3040) sees only a constant that a file declares. A constant that an `implementation`
  declares is not rejected.
- `COMPILE_TIME_ASSERTION_FAILED` (3059) decides a `static_assert!` condition only when the evaluator can fold it. A
  condition with `size_of`, `align_of`, a call, a name that is not a file constant of this file (a constant of another
  namespace included), a float, a string, a division by zero or an overflow is unknown and accepted, so a false
  `static_assert!(size_of<Light>() == 9, "x");` raises nothing yet. The arithmetic is over 64-bit integers, not over the
  type of the constant, so a constant of a narrow type does not wrap.
- `CYCLIC_CONSTANT` (3055) sees only the constants that a file declares at the top level. A constant that an
  `implementation` declares is not part of the search. A name is matched by its text in the name table, so a
  qualified name of another namespace is not followed.
- `INVALID_MACRO_ARGUMENT` (3061) does not check the type of each value that a `{}` takes: `print!("{}", x)` is
  accepted for any `x`. A condition or a text that is not a literal is accepted whatever its type, and a format text
  that is not a string literal has no `{}` count. The five log levels are an assumption (see Built-in macro).
- `line!()` has no type in sema, and `include_bytes!` gives no type either (it should give `c.array<uinteger8>`), so
  both fit any target.
- `RAW_POINTER_OUTSIDE_UNSAFE` (3041) checks only a `local` whose type is spelled `pointer<T>`. A `pointer` in a
  parameter, a return type or a structure field is not checked (issue #190), and a type alias of a pointer is not resolved.
- `NON_BOOLEAN_CONDITION` (3043) judges only a literal and a primitive that sema knows. A condition of an unknown type
  (a call, a generic value, a field of an imported type) raises nothing.
- `LITERAL_OUT_OF_RANGE` (3033) reads only a written integer literal. A float literal, a literal in an operand
  (`1 + 300` bound to a `uinteger8`), a value folded from a named constant, and a literal whose target type is not a
  primitive sema knows (it is then checked against `uinteger64` only) are not range checked.
- `DIVISION_BY_ZERO` (3034) and `SHIFT_COUNT_TOO_LARGE` (3035) fold only an integer literal, one leading minus and
  parentheses. A named constant is not folded, so `x / ZERO` raises nothing. A shift whose left side is a literal has
  no width and raises nothing. A count above 64 bits is not folded.
- An alias of an `enum` is not resolved to the enum for any comparison, because sema never compares an enum type
  for `TYPE_MISMATCH` (see the `TYPE_MISMATCH` gap above); the aliased-enum case that a test covers is the
  `switch` coverage of an imported enum with an alias. No test covers `type A = Color;` of a same-file enum.
- `SIGNED_DIVISION_OVERFLOW` (3037) folds only an integer literal, one unary minus and parentheses. A hex or binary literal, a
  named constant, a nested expression such as `(-128 / -1) + 1`, and a compound `/=` or `%=` (whose left side is a name) are not folded.
  The folder duplicates the ones of the sibling literal, zero divisor and shift rules; merge them later.
- `INVALID_ENUM_TAG_TYPE` (3031) checks only that the tag is an integer primitive. Sema does not check that the tag
  is wide enough for the number of variants, and does not use it for the layout of the enum (`size_of` is
  unchanged).
- The contract check only sees arguments whose type is cheap to know. Other arguments are not checked.
- `operator` is checked against its vocabulary; only `construct` and `destroy` have a placement and a shape check
  (see "Construct and destroy"), the signature of any other operator is not checked; `export` is checked for shape
  only.
- `LIFECYCLE_OPERATOR_CALLED_DIRECTLY` (3027) today sees a bare name and the static form `Type.name(...)` only.
  Decision: a call on a value (`point.drop()`) of a lifecycle operator (`construct` or `destroy`) is also rejected
  with 3027, for consistency with those two forms; no new code. Sema does not yet implement it, as it does not
  know the type of the receiver. A bare name that no top-level function declares also raises `UNDECLARED_NAME`
  (3004), as a member is not a bare name.
- `MAREA_SemaTypedNodeGetResolvedType` covers the six declaring nodes, the literals and the expressions of "Resolved
  type of an expression". A `Self` type and a generic parameter are spelled by
  their written name, not by the type they stand for in one instantiation. A name of an `EXTERNAL` namespace is
  spelled by what is written and the import alias rule, with no qualifier added; a type that a `FOUND` namespace
  declares is spelled with the last segment of that namespace instead (see "Members of a FOUND namespace").
- An expression with no resolved type. The sweep of the 23 examples (866 expression nodes, 36 of them qualifier names
  that are not values and are counted on their own) finds 79 expression nodes with no type, with a fixture signature
  function that answers `filled`, `append`, `sum` and `result` (see "Members of a library type"), and every one of the
  79 is in one of the nine classes below. The list is closed: the sweep test
  (`tests/source/sema/MAREASemaTypedAstExamples.cpp`) fails with the file, the position and the name for any other
  untyped expression node, and it fails for a class that no example reaches any more, so a class that a fix empties
  leaves this list in the same change. A literal never has a place in the list: every literal has a type. The `ir`
  stage can treat every untyped expression outside these classes as `UNTYPED_EXPRESSION` (4006).
  - `MACRO_SUBTREE` (20): a `MACRO_CALL` (15) and every node below one (5: `info` of `log!`, the call `power(...)` of
    `print!`, `size_of` and `align_of` of `static_assert!`). The `ir` raises 4005 for a macro.
  - `QUALIFYING_TYPE_NAME` (2): a `GENERIC_NAME` on the left of a member access, a written type that qualifies a static
    member (`c.array<sinteger32>.filled(8, 0)`). It is not a value, and sema does not flag it as a qualifier name,
    which is only an `IDENTIFIER`.
  - `BUILTIN_CALLEE_NAME` (7): the `IDENTIFIER` `size_of`, `align_of` or `convert` that is the callee of a call. The
    call has the type; the name has none.
  - `UNTYPED_FUNCTION_NAME` (3): an `IDENTIFIER` that is the callee of a call and has no function type: `power` and
    `square_root`, imported by name from an `EXTERNAL` namespace that gives no signature, and `maximum`, a generic
    function of the file (a generic function name has no function type; the call of `maximum` has its inferred type).
  - `UNTYPED_FUNCTION_MEMBER` (23): a `MEMBER_ACCESS` that is the callee of a call, with no function type: a function
    or method of an `EXTERNAL` namespace or of a library type (`stdio.puts`, `vulkan.present`, `tasks.append`,
    `item.result`, `Arena.create`, `product.sum`), a static member of a written generic type, or a method on a
    receiver of a generic parameter type (`left.less(right)`, `winner.describe()`). The call may have a type of its
    own (`tasks.append(...)`, `left.less(right)`); the member never does.
  - `UNTYPED_CALL` (18): a `CALL_EXPRESSION` whose callee has no type and that no rule types: a call of an `EXTERNAL`
    function with no signature, or of a library member that the signature function does not know
    (`stdio.puts(text)`, `vulkan.present(swapchain, texture)`, `text.length(full)`, `slice.length(pixels)`,
    `table.length(scores)`, `square_root(x)`).
  - `UNTYPED_LOOP_NAME` (3): the `IDENTIFIER` `pixels` or `index` read as a value, the name of a `with` or a `for`
    statement whose deciding expression has no type (`with vulkan.map_texture(texture) as pixels`, the counter of the
    range `0 .. slice.length(pixels) - 1`).
  - `DERIVED_OPERATION` (2): a `BINARY_EXPRESSION` (`slice.length(pixels) - 1`) or an `INDEX_ACCESS` (`pixels[index]`)
    that has an operand with no type. The root cause is counted on its own.
  - `SPAWN_OF_VOID_CALL` (1): a `UNARY_EXPRESSION` `spawn` whose call is typed `void` (`spawn scale_slice(part,
    factor)`): the task of a call that returns nothing keeps no type.

  Outside these examples sema leaves a node untyped in the same cases: the name of a `for`, `for each`, `partition` or
  `with` statement when the type that decides it has none, and every expression made from it; the key of a `for each`
  over any collection but `array`, `vector` or `slice` (no library type of the repository has a key); a call, a member
  or a name from an `EXTERNAL` namespace with no signature known for it, or from a `FOUND` namespace that is not a
  public function or constant, or a generic one with no type arguments (see "Members of a FOUND namespace"); a method
  or a field read on a result typed from a `FOUND` namespace, because such a lookup stays in the file being built; a
  member of a library type that the signature function does not know, a static call whose receiver is a bare name and a
  size result; a method call on a generic-parameter receiver whose own where clause lists no contract that declares
  the member; a call of a generic function whose inferred type arguments find nothing to bind, disagree, or give a
  result that is not itself one bound bare parameter (see "Inferred type arguments"); the built-in `convert`, `size_of`
  and `align_of` as names; every `MACRO_CALL` and `GENERIC_NAME`.
- A binary operation has the type of its operands even when an operator function of the structure declares another
  result type: the result of `#[operator = "+"]` is not read. A field of a variant is read from the first variant that
  declares the name.
- The type of a literal is the default (`sinteger32`, `float64`) where the context is not a primitive that sema can
  read: a generic parameter type (`g(5)` with `a: Item`, also with explicit type arguments), an enum or a library
  type (an integer literal that `TYPE_MISMATCH` accepts for an enum is `sinteger32`, not the tag type), an entry of a
  library literal such as a table, the index of `a[i]`, the key of `[key] = value`, the count of a shift, a literal
  inside a macro call, and an operand whose type sema does not know (a call of a generic function, a member of a
  generic structure). The default itself is an assumption of the plan, not a rule of the grammar. The range of a
  value is not checked against the type that the context gave: `local x: uinteger8 = 256;` stays accepted, as before.
  The literal of `convert<T>(x)` takes `T`, so `convert<uinteger8>(300)` types `300` as `uinteger8`.
- The typed-AST hand-off substitutes a generic result for a bare parameter with explicit type arguments, and now also
  for a bare parameter bound through "Inferred type arguments" when the call gives none; a compound result (such as
  one written `c.array<Item>`) still keeps the raw type either way, since only a parameter, never a result, is
  unified against a compound shape.
- `DUPLICATE_SWITCH_ARM` compares a float by its text: `1.0` and `1.00` are taken for different values. A constant
  that sema cannot read (imported, or declared with a computed expression) is compared by its name only, and a
  case value that is a call or another computed expression is never compared.
- `convert<T>(x)` between two structures is measured in sema, as described under "Rules", for primitive fields, a
  field `align`, a structure held by value and two `marea` structures with the same fields. It is no longer accepted
  when sema cannot measure the pair: a generic structure, a `string`, `pointer`, `array`, `vector`, enum or library field, an
  `marea` structure inside a `c` structure, a `marea` against a `c` structure, and a chain deeper than 8 levels are
  an explicit `TYPE_MISMATCH` (3011), not a silent accept. These are rejections, not real sizes: a generic
  structure and a field of another type have no size in sema, and two `marea` structures are compared by their
  fields, not by a size, on the assumption that the backend lays out equal input equally. A field pair of two
  primitives with the same width but another name (`sinteger32` and `float32`) is a mismatch here, as the rule is
  by exact name; a looser rule is a later decision of the user. The numeric range of a conversion
  (`convert<sinteger8>(count)` with `count` above 127) is a run-time stop, not a check of sema.
- The parser's `<`-opens-type-arguments lookahead (see [the AST doc](../ast/marea-ast.md),
  "Lookahead rules") does not tolerate `#[...]` inside the parameter list of a function type used
  as a type argument (`f<function(#[semantics = reference_mutable] x: T): T>()`); that scan's token
  vocabulary was never widened for attributes, so such a type argument fails to parse as one. The
  same scan already had no tolerance for the retired `mutable`/`reference` parameter keywords it
  used to allow, so this is not a new restriction, only an unaddressed one.
- Move state and borrow liveness are not tracked across statements. Sema keeps no per-variable "moved" or
  "borrow ended" bit, and `MAREA_SemaCheckSpawn` (`MAREASemaParallel.c`) checks one `spawn` alone, so two
  sibling `spawn` statements of one `fork_join` are never compared. The settled design for every later
  check (use after move, partial move, borrow liveness, cross-spawn, `partition` body) is one rule set,
  recorded here so no check invents its own:
  - **State table.** One new per-scope table, modeled directly on `references`
    (`MAREA_SemaNameTable` in `MAREASemaInternal.h`): `MAREA_SemaBindReferences` (`MAREASemaOwnership.c`)
    pushes entries on scope enter, and they pop on scope leave. The new table is pushed on scope enter,
    popped on scope leave, and read and written inside the same `MAREA_SemaVisit` pass. There is no second
    traversal and no control-flow graph.
  - **Merge rule.** At a branch or loop boundary (`if`/`elseif`/`else`, the arms of a `switch`, a loop
    body) the merge follows `MAREA_G_Terminates` (`MAREASemaControlFlow.c`), which says a path terminates
    only when every arm agrees (`MAREA_G_IfTerminates`, `MAREA_G_SwitchTerminates`). In the same way, a
    local is "definitely moved" after the merge, or a borrow "definitely ended", only when every arm
    proves it on its own. A loop body is visited once, so an effect that can happen on a later iteration
    but not on the first is treated as "maybe already happened" once the walk is past the loop.
  - **Rounding rule.** When the effect of an arm is ambiguous, the merge always rounds toward "still
    valid" (not moved, still borrowed), never toward "reject". A missed catch is acceptable; a false
    rejection of valid code is not. Every later check that builds on this table must inherit this
    rounding and must not define its own.
