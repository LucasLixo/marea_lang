# Marea sema stage

The `sema` stage checks the tree of the `parse` stage. It reads a public tree and writes diagnostics.
It has no global state, never prints and never reads the disk: the caller gives it an arena, a
diagnostic sink and an import resolver. It makes no typed tree. The shape of that hand-off is decided
when the `ir` stage is built.

## Use

```c
MAREA_SemaImportResolver *resolver;
MAREA_SemaContext *sema;

MAREA_SemaImportResolverCreate(arena, my_resolve_function, my_data, &resolver);
MAREA_SemaContextCreate(arena, sink, resolver, &sema);
MAREA_SemaCheckCompilationUnit(sema, root);   /* once per file, dependencies first */
```

`MAREA_SemaCheckCompilationUnit` returns an error only for a null argument or an out of memory. A rule
violation is a diagnostic in the sink. Every sema diagnostic is an error, in the range 3001-3023.
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
mutable`. Any other expression is rejected. This is a simple first rule.

**Control flow.** A function with a return type ends every path with `return value;`, through `if`,
`elseif`, `else`, scope blocks and `switch`. A loop is never an ending path. `unreachable!()` and
`compile_error!()` end a path. A `switch` over an enum names every variant or has a `default`.

**Entry point.** In the namespace `main`, `function main` has exactly the parameter `arguments:
array<string>` with no word, and the result `integer32`. Sema never requires a `main`.

## Gaps

- `TYPE_MISMATCH` (3011) checks a literal against a primitive type only, never another expression. The use of an undeclared name in an expression is not checked.
  `UNDECLARED_NAME` is raised for a `where` clause only.
- The contract check only sees arguments whose type is cheap to know. Other arguments are not checked.
- `destroy`, `operator` and `export` are checked for shape only.
- The type of a call result, of a library type and of a generic result is not tracked, so a
  missing `copy` or `move` there is not reported.
