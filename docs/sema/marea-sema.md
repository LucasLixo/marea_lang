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
Every sema diagnostic is an error, in the range 3001-3025.
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
is one diagnostic, at the repeated item. Two values are the same when they are built from the same literals,
names and unary operators with the same text (`1` and `1`, `-1` and `-1`, `"a"` and `"a"`, `a.LIMIT` and
`a.LIMIT`). Over an enum a bare and a qualified name of one variant (`red`, `Color.red`) are the same
variant, as in the coverage check. Two broken (`ERROR_NODE`) values are never compared. The check runs for
every scrutinee type, and it does not replace `NON_EXHAUSTIVE_SWITCH`: a `switch` over an enum with a repeat
and a missing variant raises both. The rule follows the grammar (`switch_statement` has one optional
`default_arm`) and the arms of a `switch` do not fall through, so a repeated value is an arm that never runs
and a second `default` is an arm that the IR would drop. The place of the `default` is not checked: the
grammar puts it last, but the parser accepts it anywhere and the IR lowers it as the last test, so a
`default` before a `case` raises nothing here.

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

**Call results.** A non-generic call expression's typed node carries the callee's real, unsubstituted
return type: `USER`, `EXTERNAL` or a built-in class, exactly as the function declares it. Generic-
result substitution is not performed: when the return type names an unbound generic parameter (for
example `Pair<T>.first(): T` called without `ir`-level instantiation), the typed node is tagged
`GENERIC` and the raw, unsubstituted type name is kept - never a guess, never a crash. Resolving the
real, instantiated result type is deferred to when the `ir` stage needs one for real.

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 3001 | `UNRESOLVED_IMPORT` | `import a.b.c;` when the resolver finds neither `a.b.c` nor a public `c` in `a.b` |
| 3002 | `NAMESPACE_PATH_MISMATCH` | `namespace a.b.c;` in the file `a/b.marea` |
| 3003 | `NAME_ALREADY_DECLARED` | `structure Item end function Item() end`, or an import and a declaration of one local name |
| 3004 | `UNDECLARED_NAME` | `where Item: Missing`, or a constraint on a name that is not a generic parameter |
| 3005 | `UNKNOWN_ATTRIBUTE_KEY` | `#[frobnicate = true]` |
| 3006 | `INVALID_ATTRIBUTE_VALUE` | `#[inline = maybe]`, `#[build = 1]`, `#[simd = force]` |
| 3007 | `CONFLICTING_ATTRIBUTES` | `#[public, private]`, `#[pack = 1, align = 16]`, a key given twice |
| 3008 | `INVALID_ALIGNMENT` | `#[align = 15]` |
| 3009 | `UNKNOWN_TYPE` | `a: Missing;` in a structure |
| 3010 | `ARGUMENT_COUNT_MISMATCH` | `add(1)` when `add` takes two parameters |
| 3011 | `TYPE_MISMATCH` | `f("text")` when the parameter is `integer32`, `local x: boolean = 1;` |
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

## Gaps

- `TYPE_MISMATCH` (3011) checks a literal against a primitive type only, never another expression. The use of an undeclared name in an expression is not checked.
  `UNDECLARED_NAME` is raised for a `where` clause only.
- The contract check only sees arguments whose type is cheap to know. Other arguments are not checked.
- `destroy`, `operator` and `export` are checked for shape only.
- The type of a call result is closed for non-generic calls via the typed-AST hand-off; generic-
  result substitution still open. The type of a library type is not tracked, so a missing `copy` or
  `move` there is not reported.
- A `default` arm that is not the last arm of a `switch` is accepted. The grammar (`switch_statement`) puts the
  `default_arm` last, but the parser accepts it anywhere, and sema leaves the form as it is: the place of an
  arm is a syntax rule, so it belongs to the parser, and the IR lowers a `default` that comes first as the last
  test.
- `DUPLICATE_SWITCH_ARM` compares the text of a value, not its number: `1` and `0x1`, or `1.0` and `1.00`, are
  taken for different values. A constant that is named in one arm and written as its literal in another is not
  matched either, and a name of an enum is matched by the variant name only, so `Color.red` and `Other.red`
  count as one. A case value that is a call or another computed expression is never compared.
- A `break` or `continue` that is not inside any loop is not reported.
- A `with` statement is not an ending path for `MISSING_RETURN`, even when its body returns.
