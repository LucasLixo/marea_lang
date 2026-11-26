# Marea sema stage

## Contract

The `sema` stage checks the tree of the `parse` stage. It reads a public tree and writes diagnostics.
It has no global state, never prints and never reads the disk: the caller gives it an arena, a
diagnostic sink and an import resolver (and, when it has one, a second resolver for the names the host registered, a
type registry with the library types, a target description, the build settings and an include resolver).
`MAREA_SemaCheckCompilationUnit` itself returns no artifact
beyond diagnostics; a separate, additive API builds the typed shadow tree the `ir` stage will consume
- see [Typed AST hand-off](#typed-ast-hand-off). The typed node of a structure and of an enum also carries its size,
its alignment and the offset of each member, see [Layout of a structure and an enum](#layout-of-a-structure-and-an-enum).

## Use

```c
MAREA_SemaImportResolver *resolver;
MAREA_SemaContext *sema;

MAREA_SemaImportResolverCreate(arena, my_resolve_function, my_data, &resolver);
MAREA_SemaContextCreate(arena, sink, resolver, &sema);
MAREA_SemaContextSetRequireResolver(sema, registry);   /* optional, see "Two resolvers" */
MAREA_SemaContextSetTypeRegistry(sema, types);         /* optional, see "Descriptors and roles" */
MAREA_SemaContextSetTarget(sema, &target);             /* optional, see "Caller inputs" */
MAREA_SemaContextSetSettings(sema, &settings);         /* optional, see "Caller inputs" */
MAREA_SemaContextSetIncludeResolver(sema, &includes);  /* optional, see "Caller inputs" */
MAREA_SemaCheckCompilationUnit(sema, root);   /* once per file, dependencies first */
```

`MAREA_SemaCheckCompilationUnit` returns an error only for a null argument or an out of memory. A rule
violation is a diagnostic in the sink. A size that does not fit in `MAREA_IntegerSize` (a diagnostic
message, or the growth of a name table) is also an out of memory: nothing is allocated or copied.
Every sema diagnostic is an error, in the range 3001-3086. Not every number of the range is a live code: the
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
A type name that no local rule knows is given to the resolver as a last step.

### Full paths, covers and aliases

An import or a require binds no name. It makes its full path legal in the file: it is a cover, the path and every path below it.
`require marea.math;` makes `marea.math.Vector4Float32` and `marea.math.scalar.add(a, b)` legal, and `math.Vector4Float32`
is `UNDECLARED_NAME` (the last segment alone never resolves, and a path is never cut). A name of another namespace that no
cover reaches is `UNIMPORTED_NAMESPACE` (3078), in a value and in a type position too (`marea.array.Array<sinteger32, 4>` needs
`require marea.array;` or `require marea.*;`). The namespace of the file and its own declarations need no cover. `c` is
no built-in name any more: `c.stdio.puts(x)` needs `require c.stdio;`.

| Written | Covers | Binds |
|---|---|---|
| `import N;` or `require N;` | `N` and every path below it | nothing |
| `import N.*;` or `require N.*;` | the same, at any depth. `N` is asked of the resolver as a prefix | nothing |
| `import N as x;` | the same | `x`, which stands for the whole path `N` (`x.sub.f()` is `N.sub.f()`) |
| `import N.M.f as g;` | `N.M.f` | `g`, one symbol |
| `import N.M { f as g, T };` | `N.M.f` and `N.M.T` | `g`; a member with no `as` binds no name |

A wildcard cannot have an alias or a member list (a parse error). An alias named `c`, `marea`, a primitive type or a built-in
macro is refused. One full path covered by an import and by a require (one path equal to, or below, the other) is
`AMBIGUOUS_NAMESPACE` (3080), reported at the second declaration. A file of the project cannot declare a namespace that starts
with `c` or `marea`: `RESERVED_NAMESPACE_ROOT` (3081) is unconditional, and no registry changes it. The two roots belong to the
libraries the language ships, which reach the registry through its internal entry, never through a project file.

### Two resolvers

`import` brings a file of the project and is answered by the import resolver the context was created with. `require`
brings a name that is registered on the C side, by a shipped library (`c.*`, `marea.*`) or by the host (any other
root), and is answered by the registry that `MAREA_SemaContextSetRequireResolver` sets. A context with no registry
answers a require through the import resolver, so a host with one resolver needs no change. With a registry, an `import`
of a name that only the registry knows is `UNRESOLVED_IMPORT` with the hint "registered on the C side, use require", and
a `require` of a name that only
the import resolver knows is `UNRESOLVED_REQUIRE` (3079) with the hint "a project file, use import". A use of a path asks
the import resolver first, then the registry, and the signature function of the resolver that knows the namespace gives the
signature of a member. A host may not register a name under `c` or `marea`: the two roots belong to the libraries the
language ships, and the public entry of the registry rejects them (see Descriptors and roles).

`import a.b { f as g, T as U, Color.red as Crimson, MAX };` asks the resolver for `a.b` once, then
binds one local name per member that has an alias (a member with no alias only covers its full path). A member path has one
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

Every import alias keeps its original: the last segment of a whole path, or the member path as written for a member
(for example `Color.red`). The internal name
table (`MAREA_SemaName`) holds it in `original` and `originalLength`, empty for a name that has none.
The IR lowers an alias to this text. A type written with an alias compares equal to the type
written with the original in a contract signature, and so does a local `type Name = Other;` (a chain
of aliases is followed up to 8 steps; an alias with generic parameters is not expanded).
An import name also keeps its qualifier, the namespace it came from (`qualifier` and `qualifierLength`),
and the comparison renders it with that namespace: `import a { T as X }; import b { T as Y };` make two
types, while two aliases of one member of one namespace stay one type.

The names `c` and `marea` are the reserved first segments. An import alias named `c` or `marea` (either form) or a `type`
named `c` is `NAME_ALREADY_DECLARED` with the message `name is reserved: 'c'`; an alias that is
refused is not bound. A plain declaration or a plain `require c.x;` is not affected.

An import alias (either form) named like one of the 13 primitive types (`boolean`, `sinteger8` to `sinteger64`,
`uinteger8` to `uinteger64`, `float32`, `float64`, `character`, `string`) is `NAME_ALREADY_DECLARED` with the
message `name is a primitive type: 'sinteger32'`, and the alias is not bound. A library name such as `array` stays a
legal alias. A top-level function, constant, `structure`, `enum` or `type`, a local, or an import alias named like one
of the built-in macros (`assert`, `debug_assert`, `unreachable`, `compile_error`, `static_assert`, `include_bytes`,
`include_text`, `format`, `print`, `log`, `line`, `file`; `namespace` is a keyword) is `RESERVED_MACRO_NAME` (3056)
with the message `name is a built-in macro: 'print'`. A declaration or a local that is refused is still bound, so its
uses raise nothing more; an alias that is refused is not bound, so its uses raise `UNDECLARED_NAME`. A name that only
contains a macro name (`printer`) is accepted. A parameter and a `for`, `for each`, `partition` or `with` name are
refused like a local and are still bound. An import or a member with no `as` binds no name, so there is no name to refuse.

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
set. The member is read the same way for a member access with a full path (`c.stdio.puts(text)`: namespace `c.stdio`,
member `puts`), for one through an alias (`require c.stdio as io;` then `io.puts(text)`) and for a bare name an alias bound
to one member (`require c.math.pow as power;` then `power(...)`: namespace `c.math`, member `pow`, the original name, not
the alias). A 3-segment `import a.b.c as d;` into an `EXTERNAL` namespace never resolves its own full joined path to
`NOT_FOUND` (every depth answers `EXTERNAL`), so its local name stays bound as a namespace with the member kept as its
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
and primitive fit rule as `MAREA_SemaCheckExpressionFit`. A parameter may also name a `structure`: a value of the
structure of that name fits it, and a value of any other structure is `TYPE_MISMATCH`. A number literal fits a primitive
parameter only, and in the typed tree it takes the type of that parameter, as for a function of the file. The call is then typed with the signature's
result spelling. The member or name itself (`puts` before the call, `power` used with no call) stays
untyped: only the call gets the signature's result.

### Members of a library type

The same signature function also types a call on a member of a registered library type (see
[Descriptors and roles](#descriptors-and-roles)). Sema reads the descriptor of the receiver, never its name. The function
is asked with the full written path of the type (`marea.array.Array`) and the member name. Two call shapes are read: a static
call on the type itself (`marea.array.Array<float32, 16>.filled(0.0)`) and an instance call on a value whose type peels down
to a registered type (`tasks.append(x)`, `item.result()`, `product.sum()`), following a generic parameter a frame fills,
`Self`, a local alias and a type with the owner role. A result spelled `Self` is the receiver's own type. A result spelled with the
positional generic parameter names of the descriptor (`Item` for the first type argument, `Count` for the second) is the
receiver's type argument at that place, read in the receiver's own frame; a size argument is not a type, so `Count` leaves
the call untyped. Any other spelling is the literal result, as for an `EXTERNAL` call. The arguments of such a call are not
checked against the signature. The index of a registered type with two type parameters and no index protocol (a keyed table)
is typed with the second argument by the structure alone (no signature is asked), as the element of an array is. With no
signature function, a receiver that is not a registered type or a member the function does not know, nothing changes: the
call stays untyped and no diagnostic is raised.

### Caller inputs

Everything the caller must tell sema is a plain struct in `marea/sema/MAREASema.h`: caller owned, not opaque, and only
read during a check (sema never copies or frees it). They are plain because the caller fills every field (CMake, or a
host with no CMake) and no accessor would add a rule; each setter takes a pointer, and a null pointer clears the input.
A context with none of them behaves as the host-neutral one.

| Struct | Setter | What reads it | With none given |
|---|---|---|---|
| `MAREA_SemaTarget` | `MAREA_SemaContextSetTarget` | a `when(...)` predicate and the availability of a `backend` | every comparison is false, `auto` is `scalar`, an explicit `vector_extension` or `intrinsics` is `BACKEND_UNAVAILABLE` |
| `MAREA_SemaSettings` | `MAREA_SemaContextSetSettings` | `build_value!` | every `build_value!` call takes its default |
| `MAREA_SemaIncludeResolver` | `MAREA_SemaContextSetIncludeResolver` | a macro that a library registers and that reads a file | every file is missing, so the call is `INVALID_MACRO_ARGUMENT` |

`MAREA_SemaTarget` holds NUL terminated texts (`architecture`, `operatingSystem`, `compiler`, `endianness`), a list of
feature names and a `pointerWidth`. A null text, an empty list and a zero width are "not answered". `MAREA_SemaSettings` is a
list of `name`, `value` text pairs; a pair whose name no call asks for is ignored. `MAREA_SemaIncludeResolver` is one function
that gives the size of a file (`MAREA_MacroIncludeSizeFunction`, declared in the registry header, because a library expander
calls it) and the pointer that goes with it. Sema never reads the disk: this function is the only way it learns about a file.

### Descriptors and roles

Sema knows no library. A library registers type descriptors, and sema reads the roles and the ownership class of the
descriptor, never a name. The registry is `MAREA_TypeRegistry` (`marea/base/MAREATypeRegistry.h`, a base header, so no stage
includes a later one); `MAREA_SemaContextSetTypeRegistry` hands it to sema. A context with no registry knows no library type.
A type is registered by its namespace and name (`marea.array`, `Array`) and is found by the full path written in the source, so a
type is always written with a full path and its `require`. `string` stays a primitive. The bare names `array`, `vector`,
`pointer`, `owned`, `slice` and `task` are not types any more: written bare, each one is `UNKNOWN_TYPE`.

A `MAREA_TypeDescriptor` holds the namespace, the name, the count of type parameters, the roles, the ownership class and
whether the first type parameter is an element reached by index (`hasIndexProtocol`). The roles are bits:

| Role | What sema does with it |
|---|---|
| `PARTITION_PART` | the type of the part a `partition` gives: sema writes `<path><element>` from the registered type that holds the role |
| `SPAWN_HANDLE` | the type of a `spawn`, written the same way, and a move-in value |
| `RAW_POINTER` | rejected outside `unsafe` in every place a type is written (`RAW_POINTER_OUTSIDE_UNSAFE`, 3041); index-worthy |
| `OWNER` | peeled one level to read a member or an element, and a name given to such a parameter needs `copy` or `move` |
| `VIEW` | a borrow of elements (by value, never guessed to be a move type) |

The ownership class is `COPY` (by value) or `MOVE` (a name needs `copy` or `move`, and a parameter of the type moves in).
`hasIndexProtocol` makes `for each`, an index access, a literal value and `Copyable` (a `COPY` run with no role) read the first
type argument as the element. A descriptor with a `COPY` class, no role and the index protocol is an inline run: the layout record
measures it when its element is a primitive and its count is a literal (`marea.array.Array<sinteger32, 4>` is 16 bytes,
aligned to 4).

**Registry.** `MAREA_TypeRegistryCreate(arena, mask, &registry)` makes a registry. The mask is a plain input of the caller:
`MAREA_LibraryFlag` is a bit-flag enum, one bit per shipped library (`MAREA_LIBRARY_FLAG_MAREA_ARRAY`, `_MAREA_VECTOR`,
`_MAREA_TUPLE`, `_C_MEMORY`, `_C_THREADS`), combined with bitwise or; `MAREA_LIBRARY_MASK_DEFAULT` turns every one on.
A library whose bit is off registers nothing, so every name of it is unknown to sema and a `require` of it is
`UNRESOLVED_REQUIRE`. A library declares the libraries it depends on (`MAREA_TypeRegistryAddDependency`); a library that is on
with a dependency that is off is a configuration error, `MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY`, never a silent
pass. A macro that returns a library type belongs to the library that owns the type (decision 53 of the 2026-11-24 plan):
`include_bytes!` returns `marea.array.Array<uinteger8, N>`, so it is a macro of `marea.array`, called as
`array.include_bytes!("file")` after `require marea.array as array;` or as `marea.array.include_bytes!("file")`. With
`marea.array` off the name is unknown. The real macro is not written yet (see Gaps); a fixture library proves the mechanism.

The registry has two entries. `MAREA_TypeRegistryRegisterInternal` (and the function form, the registration macro) is used only
by the libraries the language ships and may register under `c.*` and `marea.*`. `MAREA_TypeRegistryRegisterPublic` is the entry of
the user and the host: it rejects any name whose first segment is `c` or `marea` with
`MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT`. A host may register any other root and may replace a runtime
function (`alloc`, `free`) through `MAREA_TypeRegistryReplaceRuntimeFunction`; a name outside the allow list is
`ERROR_UNKNOWN_RUNTIME_FUNCTION`. The C registration macro `MAREA_REGISTER_FUNCTION` registers the wrapper that the binding
declaration `MAREA_HOST_FUNCTION` made for one typed C function (see the vm doc, "Host calls"), through the internal entry
for a shipped library and through the public entry for a user or a host. `MAREA_TypeRegistryFindByRole` gives the registered
type that holds a role.

**Macros of a library.** `MAREA_TypeRegistryRegisterInternalMacro` registers one compile-time macro for a shipped library,
under the same mask and dependency rules as a type. A `MAREA_MacroDescriptor` holds the namespace and the name, the kind of
each argument (only `MAREA_MACRO_ARGUMENT_KIND_STRING_LITERAL` exists), the result type (a registered type descriptor, the
name of its element type and the count that the expander gives) and the expander, a C11 function. The expander gets the text
of the string argument and the include size function of the caller (`MAREA_MacroExpandRequest`) and answers with a count, or
false when the call is not valid, for example for a file that does not exist. There is no public entry: a user or a host never
registers a macro, so the reserved roots `c` and `marea` stay the only owners of one and the rule "macros are never written by a
user" holds. `MAREA_TypeRegistryFindMacro` finds one by namespace and name.

**Two rules of the border.** A registered type that holds its first type argument by value (an index protocol) needs a
type with a measured layout there: a function type is `ELEMENT_WITHOUT_MEASURED_LAYOUT` (3082), at the element. A type sema
cannot see is not guessed here (its own check reports an unknown type). A raw pointer in a parameter, a result, a field, an
alias or another type's argument is `RAW_POINTER_OUTSIDE_UNSAFE` (3041) too, not only a local (decision 30 of the plan, issue #190).

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
`reference_read_only`, `reference_mutable` - a parameter's passing mode, see "Ownership" below), `backend` (`auto`,
`scalar`, `vector_extension`, `intrinsics`, see "Backend" below). A
key of an enum needs one of its members, and `export`
and `operator` need a string, so a bare key of those is `INVALID_ATTRIBUTE_VALUE`. The strings of `operator`
are the operator tokens of the grammar (`+ - * / % **`, the wrapping and saturating forms, `~ & | ^ << >>`, the
comparisons, the assignment forms), `"[]"`, `"()"`, `"convert"`, `"construct"` and `"destroy"`; any other
string or a name is `INVALID_ATTRIBUTE_VALUE`. A key not in the table, such as the
retired `public`, `private`, `simd`, `destroy` and `value`, is `UNKNOWN_ATTRIBUTE_KEY`. The deeper meaning of
`operator` is the signature of an operator function, `OPERATOR_SIGNATURE_MISMATCH` (3050), see Rules; its place is the placement matrix below. A flag key with no value is true. File attributes `#![...]` use
the same table; the attribute of the symbol wins over the file attribute, so `#![visibility = public]`
with `#[visibility = private]` on one symbol is not a conflict. `semantics` is meaningful only on a
`PARAMETER`, but is validated by the same generic, node-shape-agnostic attribute check as everywhere
else; it is adopted as a `PARAMETER` child last, after the name, type and optional default, so every
other reader of those fixed positions keeps working unchanged - unlike every other declaration kind,
whose attributes lead.

**Placement of an attribute.** A key stands only where the placement matrix of the attribute map allows it, otherwise it is
`MISPLACED_ATTRIBUTE` (3085), once per item, at the item. The grammar gives an attribute list to the places in the first
column; `no` cells are the 3085 cases.

| Place | visibility | inline | build | layout | align | pack | move_only | operator | export | semantics | backend |
|---|---|---|---|---|---|---|---|---|---|---|---|
| file `#![...]` | yes | yes | yes | yes | no | no | no | no | no | no | yes |
| top-level function | yes | yes | yes | no | no | no | no | no | yes | no | yes |
| top-level constant | yes | no | no | no | no | no | no | no | no | no | no |
| `structure` | yes | no | no | yes | yes | yes | yes | no | no | no | no |
| `opaque structure` | yes | no | no | no | no | no | no | no | no | no | no |
| `enum` | yes | no | no | yes | yes | yes | yes | no | no | no | no |
| `type` alias | yes | no | no | no | no | no | no | no | no | no | no |
| `contract` | yes | no | no | no | no | no | no | no | no | no | no |
| `implementation` block | yes | no | no | no | no | no | no | no | no | no | no |
| field of a `structure` | yes | no | no | no | yes | no | no | no | no | no | no |
| function in an `implementation` | yes | yes | yes | no | no | no | no | yes | yes | no | yes |
| constant in an `implementation` | yes | no | no | no | no | no | no | no | no | no | no |
| parameter | no | no | no | no | no | no | no | no | no | yes | no |
| macro statement | no | no | no | no | no | no | no | no | no | no | no |

`align` and `pack` have no file form. An `export` on a function with a generic parameter of its own, or in a generic
`implementation`, is also 3085 (C has no generics), and its label must be a valid C identifier, otherwise
`INVALID_ATTRIBUTE_VALUE`. One item gives one diagnostic: the first step that fails (key, value, namespace limit, place) ends it.

**The `when` item.** `when(predicate, key = value, ...)` gives the listed attributes a condition. The predicates are
`architecture`, `operating_system`, `feature`, `compiler` (a string), `pointer_width` (an integer) and `endianness` (a
string), joined by `all`, `any` and `not`. A predicate name that is not one of the six, or a value of the wrong kind, is
`INVALID_ATTRIBUTE_VALUE`, once per such predicate; the values themselves are an open set and are not checked. A comparison
is true when the target description (`MAREA_SemaTarget`) answers it and the answer matches; with no target, or a field the
caller left empty, it is false (so `not(...)` of it is true). The listed attributes are checked for key, value and place
whether the predicate holds or not. A `when` that holds replaces the plain value of the same key, and two that hold and give
one key are `CONFLICTING_ATTRIBUTES` (3007), so the order never decides. A `when` works in any attribute list, the file list
included. The check is the only reader of a `when`: the other readers of an attribute (visibility, layout, ...) still see only
the plain items (see Gaps).

**Backend.** The `backend` key stands only in a file whose namespace is `marea.math` or starts with `marea.math.`, on the file
or on a function; anywhere else it is `ATTRIBUTE_NOT_ALLOWED_HERE` (3084) (and a structure of `marea.math` is 3085). The
backend that decides is the one a holding `when` gives, else the plain one. `scalar` and `auto` are always available. An
explicit `vector_extension` needs a compiler `gcc` or `clang` and the architecture `x86_64` with the feature `sse2`, or
`aarch64`; `intrinsics` needs the same with `neon` on `aarch64`. A backend the target cannot give is
`BACKEND_UNAVAILABLE` (3083), with no silent fallback, naming the backend, the architecture and the compiler. `auto` has only
its start rule (the best backend the target gives, `scalar` at least); the table of which backend is best per operation does
not exist yet.

**Types.** The built-in names are `boolean`, `sinteger8` to `sinteger64`, `uinteger8` to `uinteger64`,
`float32`, `float64`, `character` and `string`. A type name is also
`Self` inside an `implementation` or a `contract`, a generic parameter in scope, a structure, enum,
opaque type or alias of the file, an import, or a library type a registry knows, written with its full path (see
[Descriptors and roles](#descriptors-and-roles)). A registered type takes at most the type arguments its descriptor declares, and
a user generic takes as many arguments as it has parameters. A number fills a value parameter only.

**Contracts.** `Copyable` is met by a primitive, an inline run (an index protocol, a copy class, no role) whose element is `Copyable`, any other
registered type of the copy class, a function type and an enum; the enum case is a documented, intentional exception to "a non-primitive is never a copy contract" (no attribute exempts any
other user type - the retired `#[value]` key does not exist). A type of the move class is never `Copyable`. `Movable` is met by every type of the move class, a `#[move_only]` type and a plain
user type. Any other contract is met when the `implementation` blocks of the type hold every contract
function with the same parameters and result, `Self` replaced by the type. A generic parameter meets a
contract only when its own `where` clause lists it. The check runs at a call of a function of the file,
with the types written in `f<T>(...)` or taken from the arguments (a name with a declared type, or a
struct literal). A built-in type is not searched for contract functions.

**Definition rule and overlap rule.** The body of a generic function is checked against its bounds once, at the
definition. No contract has an operator, so a binary operator or a compound assignment with a generic operand is
`BOUND_OPERATION_NOT_GIVEN` (3051). A call `value.name(...)` on a local or parameter of a generic type is 3051 too,
unless a contract that the `where` clause lists for that parameter has a function `name`; a contract that the file
does not declare is external and gives the call, `Copyable` and `Movable` give no function. Store, pass, `copy`,
`move` and return are always allowed. Two `implementation` blocks of one type head overlap when they have the same
number of type arguments, every pair of arguments is equal text or has a bare generic parameter of its own block, and
both declare a function or constant of one name: `OVERLAPPING_IMPLEMENTATION` (3052), at the member of the later
block. Two blocks of the same written type stay with `DUPLICATE_MEMBER`.

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
(`point.drop()`) whose inferred type is one plain type name of the file. The automatic call
at creation and destruction is lowered in the ir, not here.

**Operator signature.** A function with `#[operator = "<token>"]` in an `implementation` block is checked against its
token, once per function, as `OPERATOR_SIGNATURE_MISMATCH` (3050), reported at the function. Only the parameter
count, the presence of a result, the type of the first parameter and a boolean result are fixed:

| Token | Parameters | Result | First parameter |
|---|---|---|---|
| `+ - * / % ** +% -% *% +\| -\| *\| & \| ^ << >>` | exactly 2 (`-` may take 1) | required | the implemented type or `Self` |
| `== != < <= > >=` | exactly 2 | `boolean` | the implemented type or `Self` |
| `~` | exactly 1 | required | free |
| `= += -= *= /= %= **= <<= >>= &= \|= ^=` | exactly 2 | free | free |
| `"[]"` | 2 or more | required | free |
| `"()"` | 1 or more | free | free |
| `"convert"` | exactly 1 | required | free |

The other parameter types and the result of an arithmetic token stay free. A first parameter whose type is not one
plain name is not judged. `construct` and `destroy` keep their own shape rule (3029). A top-level function with an
operator attribute is not checked.

**Type function call.** A call `value.name(...)` or `Type.name(...)` is `UNKNOWN_TYPE_FUNCTION` (3049), reported at the
call, when the receiver is a structure or an enum declared in the file (a bare type name that no local hides, or a
local or parameter of that type, an alias followed) and the type has no function `name` in any of its
`implementation` blocks, no field `name` (a callback) and, for an enum, no variant `name` (`Shape.Circle(1.0)`).

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
is compared wherever a value meets it (a local initializer, an argument, a return, an assignment, a constant). A bare
name of a function of the file, not hidden by a local, and any expression whose written type is a function type (a
local, a field, an element, a constant, a call result) must have the same parameter count, passing mode and type of
each parameter, and result type, else `FUNCTION_TYPE_MISMATCH` (3053); an alias equals its original. A value that is
surely not a function (a literal, a primitive, a structure, an enum, a view) is `TYPE_MISMATCH` (3011). A call through a parameter or a local of a function type is checked against the parameters of that
function type: a wrong count is `ARGUMENT_COUNT_MISMATCH` (3010) and a wrong argument type is 3011, the rule of an
ordinary call.

**Enum tag type.** `enum A : uinteger8` names the integer type of the discriminant. With no tag it is
`sinteger32`. A tag must be one bare integer primitive (`sinteger8` to `sinteger64`, `uinteger8` to
`uinteger64`); any other tag (`float32`, `string`, `boolean`, a structure, a generic or an alias) is
`INVALID_ENUM_TAG_TYPE` (3031), once per enum. A tag name that is not a type at all also raises
`UNKNOWN_TYPE`. The check of `size_of` and `convert` does not use the tag of an enum, but the layout record does: its
width is the size of the tag (see [Layout of a structure and an enum](#layout-of-a-structure-and-an-enum)).

**Enum variants.** An enum that declares no variant is `EMPTY_ENUM` (3047), once at the enum. `Enum.Name` is
`UNKNOWN_ENUM_VARIANT` (3048) when `Enum` is an enum of this file, no local hides its name, and `Name` is no variant
and no function or constant of an `implementation` block of this file for that enum. The callee of a call
(`Color.make()`) is not checked here (see `UNKNOWN_TYPE_FUNCTION`, 3049).

**Ownership.** A parameter's passing mode is its `semantics` attribute - `value_read_only` (a
copy), `reference_read_only` (a read-only borrow) or `reference_mutable` (a write borrow, the
`EXCLUSIVE_BORROW_CONFLICT`/`INVALID_SPAWN_ARGUMENT` axis); `MAREA_SemaGetParameterSemantics` is
the one place the rest of sema reads it. With no attribute written, a primitive defaults to
`value_read_only`, a type of the move class (an owner, a spawn handle, a growable collection) defaults to `reference_mutable` (they move
in and are never a borrow), and any other type defaults to `reference_read_only` - settled rule 2. A
write borrow needs the explicit `reference_mutable`. The former `mutable`/`reference` keywords are
retired, `mutable` stays reserved only for `for each` and `partition`.
A second table, `readOnly`, is built like the references table: `MAREA_SemaBindReadOnly` pushes one entry per binder
when a scope is entered and `MAREA_SemaScopeLeave` pops them. A function parameter is read-only exactly when its
effective passing mode is `reference_read_only`. A `for each` variable and a `partition` part are writable only when
the statement says `mutable` and its source is not read-only. A `with` name follows the source: a name, a field or
an element has the mode of that place (a source rooted in a name that is no local, such as a file constant, is
read-only), and a call gives a new value, so the name is writable. A new local
that reuses a read-only name is a writable shadow. The root of an expression is the name at the bottom of its field and
element chain; a root that is not in the table (a plain local, a by-value parameter, a file name) is writable, so sema
rounds toward valid. `WRITE_THROUGH_READ_ONLY_BORROW` (3069) is raised in four shapes: an assignment of any operator
whose target is rooted in a read-only borrow; `for each ... in mutable` or `partition mutable` over a read-only source,
at the source; a read-only borrow given to a write-borrow parameter; and a call result or a literal given to a
write-borrow parameter (a temporary has no storage to write back). A move-in parameter (a type of the move class) is
not a write borrow of a place and takes a call result. The rule runs in `MAREA_SemaCheckOwnership` and
`MAREA_SemaCheckWriteArguments`; the typed AST pass only rebuilds the table and reports nothing.
`value_read_only` is legal only on a primitive; writing it on anything else is `INVALID_BORROW`
(3017). A type of the move class moves in regardless of `semantics`, and a name, a field or an element given to a
parameter whose type has the owner role needs `copy` or `move` like an assignment (3016, at the argument). A primitive, a type of
the copy class (an inline run, a raw pointer, a view) and an enum is by value for the *ownership mode*
(`MAREA_SemaOwnershipMode`) no matter what `semantics` says - checked before `semantics` is even read, the one place a
copy-class library type differs from every other non-primitive: the ownership mode is always `VALUE`, so a
`semantics` attribute on one of them only ever affects whether it counts as a borrow for scope
escape (below), never its mode. The enum case is the by-value exception among user types (no
attribute exempts any other user type - the retired `#[value]` key does not exist, so a structure,
move_only or plain, is never by value by itself). A registered type of the move class is a move type, so `=` of one needs `copy x` or `move x`; a type that sema cannot see
(not registered, not declared) stays by value and is never guessed. `=` of a name, field or element of any other type needs
`copy x` or `move x`, in an assignment and in a local initializer. A call or a literal needs no word.
A borrow parameter (`reference_read_only`, explicit or by default, or an explicit `reference_mutable`), a
`for each` variable, a `partition` part and a `with` name cannot be returned, nor be the value of
a field. A new local of the same name ends the reference. A file constant, or a field or element chain
rooted in one (`ORIGIN.inner`, `ORIGINS[0]`), passed as the argument of
an explicit `reference_mutable` (write-borrow) parameter has no writable storage to
back the borrow, so it is rejected with `CANNOT_BORROW_CONSTANT_AS_MUTABLE` (3032); a variable
passed to the same parameter is unaffected. An assignment (`=` or a compound form) whose target is a file constant, or a field or element path
rooted in one (`ORIGIN.x = 1;`, `TABLE[0] = 1;`), is `ASSIGNMENT_TO_CONSTANT` (3040), raised at the target; a local,
parameter or `for` variable of the same name hides the constant and may be assigned. Tracking of a live borrow across
statements is a design decision recorded in the last bullet of `## Gaps`.

**Borrowed source.** While the body of a `for each` or a `with` block runs, its source is borrowed. `MAREA_SemaCheckBorrowedSource`
(`MAREASemaBorrowLive.c`) walks the body once, when the visit reaches the statement, and reports `VALUE_CHANGED_WHILE_BORROWED` (3073) at a
write whose target overlaps the source, at a `move` of a place that overlaps it, and at a call of `append`, `insert`, `remove`, `clear`,
`pop`, `push` or `resize` on it. Two places overlap when they share a root name and one path starts with the other: a value and a field of
it overlap, two fields do not, and two elements overlap unless both indices are integer literals of two values. A `with` over a call, and
a write through the loop name or the `with` name, are not a source use. The borrow ends with the statement, so a change after it is
accepted. This check keeps no table of its own: the scan of the body is the whole state, and every arm of a branch inside the body
counts, which is the conservative direction for a loop that runs once. 3019 uses the same overlap rule for the arguments of one call; a
`for each` name stands for its source there, and a `copy x` argument is a new value, not a use of `x`.

**Move state.** `move x` of a whole local or parameter whose type is not by value (not a primitive, an enum, a copy-contract library
type, or a type that sema cannot see) marks that local moved. A later read of it is `USE_OF_MOVED_VALUE` (3070), a later `move` of it is
`VALUE_MOVED_AGAIN` (3071), both raised at the name. A read is a name in a place that takes a value (an operand, an argument, the root
of a member or index chain, a method receiver, a `spawn` argument, a `return`, a condition, a loop bound or collection); a declared name,
a member name, a field key and a case label are not reads. A plain assignment `x = value;` of the whole local makes it live again, after
its right side was read; `x.field = value;` and `x += 1;` are reads. A new local of the same name is a new, live local. The moved bit is
a field of the entry of the `locals` table (`MAREA_SemaName`), so it pushes and pops with the scope of the local, and it is read and
written in the one `MAREA_SemaVisit` pass (`MAREASemaMoveState.c`). A branch follows the merge rule of the design in `## Gaps`: every
arm of an `if`/`elseif`/`else` or `switch` starts from the state the statement started with, an arm that ends the function
(`MAREA_SemaBlockTerminates`), or has a `break` among its own statements, takes no part, a local is moved after the statement when any arm that
reaches the end moved it (OR over the arms), or when it was moved before a statement that has no `else` or `default`, and a local that every
arm of a statement with an `else` or `default` made live again is live; a `do` block is not a branch. The body of a loop (`while`, `for`, `for each`,
`partition`) starts from the state the loop started with, and nothing it moves counts after the loop, because it may run zero times. A
body that moves a local declared outside the loop, and has no `break` or `return` among its own statements, would move it again on
the next turn: `VALUE_MOVED_AGAIN` at the `move`. A move of a field or an element (`move x.inner`, `move xs[0]`) of a local that is not a borrow, when the part is not by value and sema can
see its type, marks that part: the entry keeps the chain of the first part moved out (`partAt`), and it saves, loads and merges with the
`moved` bit in the frame of a branch, but a part counts after a branch only when every arm that reaches the end moved the same part, so a partial move still rounds toward valid. A
later use of a path that holds the moved part (`x`, passed or moved whole, or a method called on it) or lies inside it (`x.inner.y`, the
part itself again) is `USE_OF_PARTIALLY_MOVED_VALUE` (3072), raised at the access; a path that differs from it at a field name or a
literal index (`x.count`, `xs[1]`) is not a use. An index that is not an integer literal is never equal to another index, so it is never
compared. A plain assignment to the part, or to a path that holds it, or to the whole local, makes the part live again. A `move` of a
`for each` loop name, whose collection is a `vector`, `array` or `slice` (also inside `owned`) of a type that is not by value, is also
3072 at the name, because the loop reads the collection again on the next turn; a loop with a key and a value is not read.

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

**Scope of a parallel construct.** A `spawn` and a `partition` need an enclosing `fork_join` block in the same function.
Without one, sema reports `PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN` (3062) at the construct and runs no other check on it,
so the argument rules below run only inside a block. The count of blocks is per function: a block of an outer function
never covers a `spawn` of another one.

**Spawn.** In `fork_join`, each argument of `spawn f(...)` is a `partition` part, a literal, a file
constant, or a name that is not a `reference_mutable` (write-borrow) parameter and not the source of
an active `partition mutable`, or a field access (`a.b`, nested too) whose base meets the same rule. Any other expression
(a call, an index, a computed value) is rejected. This is a simple first rule.

**Spawn and partition shape.** After the scope check, the operand of a `spawn` must be a call: any other operand (a name,
a literal, a macro call such as `print!("x")`) is rejected with `SPAWN_NEEDS_CALL` (3063) at the operand. The source of a
`partition` is rejected with `PARTITION_SOURCE_NOT_ARRAY` (3064) at the source when sema knows it is not an array: a number
literal, a primitive (a `string` too), a structure, an enum, a function value, or a registered type with no index, no
partition part role and no owner role (a raw pointer, a spawn handle, a keyed table). A call result is judged by its type. An
owner and a type that sema cannot trace to a registered descriptor are accepted. A construct that fails the scope check raises no shape code.

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

**Shared access in one fork_join block.** `MAREASemaCapture.c` keeps one table of what the workers of the `fork_join`
blocks in progress took. A valid argument of a `spawn` is recorded as a write when the parameter of the callee (a function of
the file, found by name) is `reference_mutable`, as a read when it is a read-only borrow, and not at all for a by-value
parameter, a move-in parameter or a callee that sema cannot see. A `partition` source is recorded as a write for
`partition mutable` and as a read otherwise. Before a new entry is added, it is compared with every earlier entry of the
block and of the blocks around it: the same value (a part and a loop name are known by the node that declared them, a local by
its type node) and overlapping places, where one of the two is a write, is `CONFLICTING_PARALLEL_ACCESS` (3074) at the
argument or the source. Two places overlap when the path of one (the name, then the field names; an index access ends the
path) is the start of the path of the other, so a value and a field of it overlap and two fields do not. Two arguments of one
`spawn` are not compared with each other (the call check owns that). A write in a `spawn` that repeats, inside a loop or a
`partition` body, of a value declared before its `fork_join` block, is also 3074, because each turn is a new writer. A
statement that stands directly in the block (the block body, outside every `spawn`, `partition` and nested `fork_join`) is
scanned when the visit reaches it: a use of a value that an earlier worker writes, or a write of a value that an earlier
worker reads, is 3074 at the use. The table is cut back when the `fork_join` block ends, so two blocks in a row do not meet.
An argument that is already `INVALID_SPAWN_ARGUMENT` is not recorded, so one mistake gives one diagnostic.

**Partition body.** After the scope and shape checks, `MAREA_SemaCheckPartition` scans the body of the `partition` once, in
source order. An assignment (also a compound one) whose target root name (the name at the base of a name, a `MEMBER_ACCESS`
chain or an `INDEX_ACCESS` chain) is a local or a parameter that was declared before the partition is
`PARTITION_BODY_WRITES_OUTER_NAME` (3075) at the target, because every worker runs the body. A name that the body declares
before the write (a local, a loop variable, a `with` name, a nested part), the part of this partition, a file constant and the
source of a `partition mutable` are not reported by this rule. In a `partition mutable`, a use of the name of its source
(a read, a write, a `for each` collection) is `PARTITION_SOURCE_USED_IN_BODY` (3076) at each use, except in a `spawn`, which
rule 3020 owns. A nested partition has its own scan, so a write in its body to a local of the outer body is 3075 once.

**Control flow.** A function with a return type ends every path with `return value;`, through `if`,
`elseif`, `else`, scope blocks and `switch`. A loop is never an ending path. `unreachable!()` and
`compile_error!()` end a path. A `switch` over an enum names every variant or has a `default`.

**Result use.** A function with no result type has no value to give. `return value;` in it is `INVALID_RESULT_USE`
(3054), reported at the statement. A call of a plain function of the file with no result type, by a bare name that no
local hides, is 3054 too when it is used as a value: a local initializer, the right side of an assignment, a returned
value, a call argument or an operand of a binary or unary operator (not of `spawn`). A call statement and a bare
`return;` are valid. A call through a local or parameter whose declared type is a primitive, a structure or an enum
is `TYPE_MISMATCH` (3011), as that value is not a function.

**Case values and range bounds.** When the switch value is a primitive or an enum, each value of each `case` must fit
its type, else `TYPE_MISMATCH` (3011) at the value: a literal fits as it fits a declared type, and `Enum.Variant` of an
enum of the file is a value of that enum only. A value of an unknown type fits. The two bounds of a `for` range must be
numbers of one type (a literal fits the other bound), else 3011 at the wrong bound.

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
whose type is the bare name of an opaque structure raises `OPAQUE_FIELD_BY_VALUE` (3058). a type
with the owner role, a raw pointer, a borrow and a C call stay legal.

**Expressions.** Sema gives an expression a type only when it is cheap to know, and never guesses. A
literal has its own type. A name has the declared type of its local, parameter or constant (a constant of the file, of
the implementation block being visited, or `Type.NAME`). A field has the type its structure declares for it, and a
field of a generic structure takes the type arguments of the value (`b.value` is `float32` when `b` is `Box<float32>`);
`Self` is the implemented type. An element of an `array`, `slice`, `vector` or `pointer` has the first type argument. A
built-in `convert<T>(x)` has the type `T`. A `-`, `~`, `copy` or `move` keeps the type of its operand. `not`, `and`,
`or` and a comparison give `boolean`. A shift gives the type of its left side. Any other binary operator, `**` too,
gives the type of both sides when they are the same primitive, or when one side is a number literal and the other a
primitive of its family; it gives no type when they differ. A call has the declared result of its callee: a plain
function of the file (not hidden by a local of the same name), a method or a static function of an
`implementation` of a structure or an enum of the file, or a value of a function type. A generic callee
is read after its arguments are bound: the written type arguments first, then the type of each argument that sema knows,
the first binding of a name wins. A bound name stands for its type in the result and in each parameter, so an argument
is compared with the bound parameter type (3011, 3014, 3053); a name that nothing binds is not compared. A call whose
callee is a
structure or an enum name of the file has that type, as construction ("Type construction call" above). Everything else
has an unknown type and is accepted. The type of an expression is compared with the declared type when that type
is a primitive, at a call argument of a function of the file, a local initializer, a plain `=` assignment, a
`return` value and a constant initializer: a conversion is always written, so two primitives are the same
only by name (`sinteger32` does not fit `sinteger64`), a number literal fits any number type, a fraction literal
any float type, a character literal fits only `character` (a number is not a character, and a character is not a number: it has no arithmetic), and a structure fits no primitive. An alias of a primitive is the primitive. A value of a registered type with
the view role or the raw pointer role fits no primitive either, and a declared type of one of these two roles is compared too: a literal, a
primitive, a structure or an enum does not fit it (its element type is the first type argument, compared as below). A `convert` of a view or
of a raw pointer is not judged, and a view or raw pointer value fits a name that a host signature gives. The mismatch is
`TYPE_MISMATCH` (3011) on the expression. A name of an enum, of a library type or of a generic parameter, and a value of a registered type of the move class
or an inline run, are classified as their own shapes but are never compared as a
whole; only their type arguments are (next paragraph).

The type arguments of a generic type are compared with the declared type before the primitive check, at the same
places. The declared type and the type of the value (any expression that has a written type, with `copy`, `move` and
parentheses looked through) must both have type arguments. Their heads are compared first: a head is a registered
type, a primitive, or a structure or an enum of the file, and two different heads (`c.memory.Owned<marea.vector.Vector<T>>`
for `marea.vector.Vector<T>`) are `GENERIC_ARGUMENT_MISMATCH` (3014) on the value. With one head, each pair of
arguments must agree: two value arguments by text and two types by this rule again. The variable of a `for each` has the element type of
the collection: one type with the owner role is peeled, then the first type argument of a registered type with the index protocol; with two
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
float (a `float32` or `float64` name or a float literal), a `boolean`, a `string` or a `character` is
`BITWISE_OPERAND_NOT_INTEGER` (3038); `and`, `or` and `not` with an operand of a known type that is not `boolean`, or a
number, string or character literal, is `LOGICAL_OPERAND_NOT_BOOLEAN` (3039). An arithmetic operator (`+`, `-`, `*`,
`/`, `%`, `**`, their wrapping, saturating and compound forms and unary `-`) with a `boolean` or `string` operand, and a
comparison between two `boolean` operands (`==` and `!=` too), is `TYPE_MISMATCH` (3011). Each is raised once on the
expression, and only when the two operands already agree in type: a pair that raises 3011 raises no second code. An
operand of an unknown type is accepted, and so is an expression that holds a node the parser could not build.

**A declared operator.** When the left side of a binary operator is a structure or an enum of the file and exactly one
function of its implementations has `#[operator = "token"]` for the operator, the right side must fit the second
parameter of that function (`Self` is the type of the left side), else `TYPE_MISMATCH` (3011) at the expression. A value
of an unknown type fits. The built-in operand rules above do not apply to such an expression.

**Raw pointer.** A written type whose descriptor has the raw pointer role (a local, a parameter, a result, a field, an alias or a type
argument) is `RAW_POINTER_OUTSIDE_UNSAFE` (3041) unless an `unsafe ... end` block encloses it. A block nested in the `unsafe` block is enclosed too; the block ends the cover at its `end`. The context
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
condition: boolean, integer (decimal, `0x`, `0b`, `_`), float and string literals, a file constant (read through its
initial value in its declared type, with a depth limit of 32, so a cyclic constant ends as unknown), `size_of<T>()` and
`align_of<T>()` of a primitive with a width, an inline run, or a structure or enum of the file (the layout record, as a
`uinteger64`), parentheses, unary `not` and unary minus, `and`, `or`, the comparisons `==`, `!=`, `<`, `<=`, `>`, `>=`
and `+ - * / %`. Integer arithmetic on a typed value of 8, 16 or 32 bits wraps in that width; literals alone and 64-bit
types are computed in 64 bits. A `float32` constant is rounded to its width. Two strings fold for `==` and `!=` only,
and two different texts that hold an escape are unknown. It answers true, false or unknown, and an unknown condition is
accepted. An overflow of 64 bits, a division by zero, any other call, a character, a name that is not a file constant (a
local or a parameter of the same name hides the constant) and every other node are unknown: the rules of constant
arithmetic are other checks, never reported here. A `static_assert!` with another argument count
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

**Built-in macro.** `MAREA_SemaCheckMacro` (`MAREASemaMacro.c`) checks every `MACRO_CALL` against one table of the 14
built-in macros (the table of `examples/macros/builtin.marea` and `build_value`), and a call with a qualified name against the
macros that a shipped library registered. A name that is not in the table is `UNKNOWN_MACRO` (3060),
raised at the call. A call with the wrong arguments is `INVALID_MACRO_ARGUMENT` (3061), raised at the offending argument,
or at the argument list when the count is wrong; at most one diagnostic is raised for a call, and a call with an
`ERROR_NODE` among its arguments raises nothing. The message names the macro.

| Macro | Arguments | Value |
|---|---|---|
| `assert`, `debug_assert` | exactly 2: a condition, a message that is a string literal | none |
| `unreachable` | exactly 0 | none |
| `compile_error` | exactly 1: a string literal | none |
| `static_assert` | exactly 2: a condition, a string literal | none |
| `include_bytes`, `include_text` | exactly 1: a string literal | `marea.array.Array<uinteger8, N>` (see "Macros of a library"), `string` |
| `format` | 1 or more: a string literal, then the values | `string` |
| `print` | 1 or more: a string literal, then the values | none |
| `log` | 2 or more: a level name, a string literal, then the values | none |
| `line`, `file`, `namespace` | exactly 0 | `uinteger32` (`line`), `string` |
| `build_value` | exactly 3: a string literal (the name), a type name, a default | the type of the second argument |

Four decisions bind the table. (1) The log levels are the five names `trace`, `debug`, `info`, `warning` and `error`
(a bare name, not a string); the language docs did not list them, so the list is an assumption that the user must
confirm. A level that is not a bare name of the list is rejected, a literal included. (2) A text argument (a message, a format text, a path) must be a string literal. A condition is rejected when its type
is known and is not `boolean` (see "Condition type"); a condition of an unknown type is accepted. (3) The count of the two
characters `{}` in the format text must equal the number of value arguments (a lone `{` or `}` is skipped), and each value
must be printable: a value whose type is known to be a structure, an enum, a view, a raw pointer, an array, a function
type or no value is rejected, while a primitive, a literal and a value of an unknown, generic or library type pass. (4) `line!()` is `uinteger32`;
`file!()` and `namespace!()` are `string`.

The value of a macro is part of the type check. A macro with no value has a shape that fits no type, so it is
`TYPE_MISMATCH` (3011) where a primitive is expected: the initial value of a `local` (`local n: sinteger32 = print!("x");`),
a returned value, an argument of a call or an operand. A macro that gives a `string` fits a `string` and is a 3011 for
any other primitive (`return format!("x");` in a function that returns `sinteger32`). A macro statement is not a value
use and raises nothing. In the typed tree only a bare `line!()` and `include_bytes!` have a resolved type
(`uinteger32`, `marea.array.Array<uinteger8,N>`).

**`build_value!`.** `build_value!("NAME", Type, default)` reads a value that the build gave (see "Caller inputs"). `Type` is
written bare: a primitive (`boolean`, an integer, `float32`, `float64`, `character`, `string`) or an enum declared in the file;
a qualified name is accepted and not checked, and any other name is `INVALID_MACRO_ARGUMENT`. An enum is usable when every
field of every variant has a default (decision 53), otherwise `INVALID_MACRO_ARGUMENT` at the type argument. A variant with
data is built by its name, like one with none. The default must fit the type, otherwise `TYPE_MISMATCH` (an enum default
is a member, so a literal is a mismatch) or `LITERAL_OUT_OF_RANGE`. When a pair of the settings holds the name, its text must
convert: `true` or `false`; an integer is decimal or `0x`, with an optional `-`, and in the range of the type; a float is a
decimal number with an optional fraction and exponent; a character is one character or one between single quotes; a string is
any text; an enum is a variant name (never its tag number). A text that does not convert is `INVALID_BUILD_VALUE` (3086),
never replaced by the default, and the message names the setting, the text and the type. The call has the type of its second
argument, so `constant X: string = build_value!("X", sinteger32, 1);` is `TYPE_MISMATCH`. A `static_assert!` reads the value of
a boolean or integer `build_value!` call (the given text, or the default when none is given); a value of another type is
unknown to it. `build_value` is a reserved macro name.

**Macros of a library.** A call whose name is a qualified name (`array.include_bytes!("file")`,
`marea.array.include_bytes!("file")`) is read as a full path, like any other: an alias expands to its namespace, and a
namespace that no import or require covers is `UNIMPORTED_NAMESPACE` (3078). The last name is looked up in the registry under
that namespace; a name that no registered macro has, a namespace that no resolver knows and a library that is off are
`UNKNOWN_MACRO` (3060). The arguments must be the kinds the descriptor gives, otherwise `INVALID_MACRO_ARGUMENT`, and the
expander must accept the call, otherwise `INVALID_MACRO_ARGUMENT` too (for example a file that the include resolver does not
know). The value has the result type of the descriptor, a library type, so it fits no primitive (`TYPE_MISMATCH`), and a declared
type that is written `Library.Type<Element, Count>` must be that type with the element and the count the expander gave
(`GENERIC_ARGUMENT_MISMATCH` otherwise). The one macro of a library that its bare name reaches is `include_bytes!`, which
stands for the macro of that name that `marea.array` registers: it has the same result type, so
`constant LOGO: marea.array.Array<uinteger8, 12> = include_bytes!("logo.bin");` needs a file of 12 bytes in the include
resolver. With `marea.array` off there is no such macro and the bare call has no type. The bare name stays in the table of
the language macros; it leaves with the real macro (see Gaps).

**Condition type.** The condition of an `if`, an `elseif`, a `while` or a `do ... while` must be a `boolean`: a condition
with a known type other than `boolean` (a primitive, a structure, an enum, a view, a raw pointer, a function type or an
array), or a number, string or character literal, is `NON_BOOLEAN_CONDITION` (3043), raised at the condition once. `true`,
`false`, a comparison, `and`, `or`, `not` and a `boolean` name pass. A value of an unknown type, of a generic parameter or
of a library type passes too. A `switch` subject and
an expression statement are not conditions and are not checked.

**Do-while.** `do body while condition end` checks its condition like a `while` (`NON_BOOLEAN_CONDITION` and the names of the
expression), and its body is a loop body: `break` and `continue` are valid in it (`JUMP_OUTSIDE_LOOP` ends with the statement),
a name that the body declares ends at the `while`, so the condition cannot read it (`UNDECLARED_NAME`; decision 4), and a moved
value is judged as in any loop. The loop does not end a path by itself, so a function with a result still needs its `return`
after it.

**Recursive type and cyclic alias.** `MAREA_SemaCheckDeclarations` runs two passes over the top-level declarations,
the alias pass first. A `type` alias that leads back to itself through other aliases is `CYCLIC_TYPE_ALIAS` (3046),
reported once for each alias on the cycle, at the alias. A structure or enum that holds itself by value, directly or
through other structures, enums, aliases and the element of an inline run, is `RECURSIVE_TYPE` (3045), reported once for
each such declaration, at the field or variant field that holds the path back. `owned`, `vector`, `pointer`, `slice`,
`task` and a function type are indirect and raise nothing. Both walks read the name table only, so a type of another
file is skipped, and both are bounded by the number of declarations of the file.

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
first segment of the file namespace, or one of the built-ins `size_of`, `align_of`, `convert` and `Self`.
A member name, a field key of a literal value, a type and a macro call are not checked. The bare name that
starts a member chain is checked like any other name, with one more rule: a chain of identifiers whose first name nothing
declares is read as a full path. When a resolver knows a prefix of it as a namespace, a cover must reach the path (a
cover: the chain is accepted whole; none: `UNIMPORTED_NAMESPACE`, 3078). When no resolver knows any prefix, the first name
is checked like any other name, so a short name that no alias binds (`require c.stdio;` then `stdio.puts(text)`), or that
only the original name of an alias reaches, raises `UNDECLARED_NAME` (3004). The local that a
`for`, `for each`, `partition` or `with` name hides is never used to type the name.
A function is declared before its use: a bare function name inside a function body that stands before the declaration
of that function in the file raises `UNDECLARED_NAME` (3004) with the message "function is used before its declaration".
There is no forward declaration. A recursive call is fine, a local of the same name hides the function, and a
constant initializer is not checked for it.

**Fields of a structure.** For a structure that this file declares (an alias is followed), a literal value is checked
against the fields: a key that no field has is `UNKNOWN_FIELD` (3044), a key given twice is `DUPLICATE_MEMBER` (3012) and
the value of a field must fit the declared type by the rule of `TYPE_MISMATCH` (3011), so a primitive field is compared
and any other field type is skipped. A field read or a field write (`point.z`, `point.z = 1.0;`) whose base has a known
structure type and names no field is `UNKNOWN_FIELD` (3044). Each defect is reported once, at the entry or the access.
A structure of another namespace, an enum, a library type and a generic parameter are skipped, and so is a
`MEMBER_ACCESS` that is the callee of a call, which is a method. Every field of the structure must be written in a
literal of it: after the entries, the first field in declaration order that no entry names is `MISSING_FIELD` (3077),
reported once per literal at the literal, so a literal that misses two fields shows the second one after the first is
written. A structure with no field, an opaque or library type, an enum and a generic structure (its fields are not seen
through the type arguments) raise nothing.

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
`reference_mutable`), `MOVE` (an `c.memory.Owned<T>` or `c.threads.Task<T>` parameter or value), `COPY` (a `copy x`
expression). A node this cannot classify defaults to `VALUE`.

**Call results.** A call expression's typed node carries the type the call returns. A call of a function of the
file carries the callee's declared return type: `USER`, `EXTERNAL` or a built-in class. A construction call, whose
callee names a structure or an enum of the file, carries that type itself: `USER` with the structure or enum
declaration, never the `construct` function that checked its arguments. A built-in `convert<T>(x)`
(with no name `convert` bound in the unit) carries the type `T`, read in the scope of the caller. When the callee is
generic and the call gives explicit type arguments, a return type that is one generic parameter (`Item`) is
replaced by the type argument at the same place (`same<sinteger32>(5)` is `sinteger32`). When there are no type
arguments, or the return type is a compound of a parameter (`marea.array.Array<Item>`), the typed node keeps the raw type: a
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

- Type arguments follow the name in angle brackets, joined by a comma with no space: `marea.array.Array<sinteger32,4>`,
  `marea.vector.Vector<marea.array.Array<sinteger32,4>>`, `marea.vector.Vector<Point>`. An integer argument is its text.
- A function type is `function(` the parameter types joined by a comma `)`, then `:` and the result type when there
  is one. A parameter name is never part of it: `function(sinteger32,sinteger32):sinteger32`, `function(string)`.
- A dotted name is kept whole: `marea.array.Array<string>`.
- An import alias is replaced the way the `ir` does it (see [the IR doc](../ir/marea-ir.md), "Aliases"): a whole-path
  alias by the last segment of its path, a member alias by the member path as written, and the head of a dotted name
  that is an import alias by that original, with the tail kept. Written type arguments are kept after it: with
  `import c.collections.list as lst;`, `lst<sinteger32>` is `list<sinteger32>`. A full path written in the source is kept whole.
- A local type alias without generic parameters, written with no type arguments, is replaced by the type it names, and
  a chain is followed up to 8 steps, as in the `ir`. Unlike the `ir`, which keeps only the head name, the arguments of
  the target are kept: with `type Grid = marea.array.Array<Point, 4>;`, `Grid` is `marea.array.Array<Point,4>`. A generic alias is kept as
  written.
- A generic parameter in scope is its own name: `Item`.
- `void` is the text of the type of a call that returns nothing (see the call row of "Resolved type of an expression"). It is
  only a spelling: it is not a type of the language, no declaration can write it, and no declared node carries it.

**Resolved type of an expression.** The same accessor gives the type of an expression, in the same spelling. The text
is made when the typed tree is built, after the typed children, so it uses what the children carry. A name that is
not read as a value (a member name, a field key, the declared name of a `partition` or a `with`) has no
type. The declared name of a `for` and of a `for each` is the exception, see "Loop names and collection". The rules:

| Expression | Resolved type |
|---|---|
| arithmetic, bitwise and power operation | the type of the operands. An operand that is a number literal (alone, in parentheses, with a sign, or in an operation of such numbers) never decides: the first other operand with a type does |
| comparison, `and`, `or`, `not` | `boolean` |
| shift | the type of the left operand |
| `-` and `~` | the type of the operand |
| `copy x`, `move x`, `(x)` | the type of `x`, with its type arguments |
| `spawn call` | the registered type with the spawn handle role around `T` (`c.threads.Task<float32>`), where `T` is the type of the call; no type when no registered type holds the role |
| identifier | the type of its declaration: a parameter or a local, a constant of the file or of the implementation block, a value parameter such as `Count` in `<constant Count: sinteger32>`, or a function name as its function type |
| name of a loop or a scope | read as a value, it has the type its statement gives it. `for i in a .. b`: the type of the bounds, as for an operation (a number literal never decides, two literals give the type of the first). `for each key, value in c`: the value is the element type of `array`, `vector` or `slice` (also under `c.`), the key is `uinteger64`. `partition v into n as part`: `c.memory.Slice<T>`, where `T` is the element type of `v`. `with e as name`: the type of `e`. The name has no type when the type that decides it has none |
| member access | the declared type of the field. On a value of a generic structure, the type arguments of the value replace the parameters of the structure. An enum variant (`Color.Red`) is the enum. A field of a variant of an enum is read on the enum value. `Type.NAME` is a constant of an implementation block. A method or a static function, named with no call, is its function type with the receiver first: `function(Vector2,float64)` |
| index access | the first type argument of `array`, `slice`, `vector` or `pointer` (also under `c.`) |
| call | the declared result type of the callee. `convert<T>(x)` is `T`. A call on the name of a structure or enum of the file is that type. A call of a method, a static function, a function value or a local alias of a function type is the result of that function. A call of one of these with no result type is `void`, and `spawn` of such a call keeps no type. `size_of<T>()` and `align_of<T>()` (with no name of the same text bound in the unit) are `uinteger64`; the names `size_of`, `align_of` and `convert` as a callee keep no type of their own. A call of a generic function of the file, by its bare name and with no written type arguments, has its type arguments inferred (see "Inferred type arguments" below); a method call on a receiver whose type is a generic parameter in scope has the result of the contract function its where clause lists for it (see "A method on a generic receiver") |
| structure, array or indexed literal | the written type (`Point`, `marea.array.Array<float32,4>`) |

A value that has `c.memory.Owned<T>` as type gives its members and elements as `T` does. A type written for another declaration
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

**Loop names and collection.** The typed tree gives a type to the three parts of a loop header that a backend would
otherwise infer, and the same accessor, `MAREA_SemaTypedNodeGetResolvedType`, reads each of them on the typed child of the
statement (children in the shape of the AST, see "Typed AST hand-off"):

| Statement | Typed child | Resolved type |
|---|---|---|
| `FOR_RANGE_STATEMENT` | child 0, the declared counter | the type of the bounds, the same text that a read of the counter has |
| `FOR_RANGE_STATEMENT` | child 1 and child 2, the bounds | the type of each bound, as of any expression |
| `FOR_EACH_STATEMENT` with one name | child 0, the value | the element type of the collection |
| `FOR_EACH_STATEMENT` with two names | child 0, the key; child 1, the value | `uinteger64`; the element type of the collection |
| `FOR_EACH_STATEMENT` | the child before the last, the collection | the type of the collection expression (`marea.array.Array<float32,4>`, `marea.vector.Vector<uinteger8>`, `c.memory.Slice<float32>`) |

The names come first, then the collection, then the body, so a `for each` has 3 children with one name and 4 with two:
the collection is child `childCount - 2`, and the value is the last name, child `childCount - 3`. The collection of
`for each x in mutable v` is the plain `v` (the flag `MUTABLE` is on the statement). The declared name carries the same
type as a read of it, and a type that sema does not know leaves the name and the collection with no type
(`ERROR_NO_TYPE`), never a guess. The element and the key follow the rule of "name of a loop or a scope" above: the
element type of `array`, `vector` or `slice` (also under `c.`), and the key `uinteger64` for any of them.

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
alias of a namespace, whatever the resolver answered (`io`, `lib`); a chain that a cover reaches, which is the full path of a
namespace or of a type of one (`c.stdio` in `c.stdio.puts`, flagged on the `MEMBER_ACCESS` node); an imported enum, or a
type that a `FOUND` namespace declares and the file imports by name; a name with no declaration at all, which sema takes for a
namespace. A function or a constant imported by name is a value, not a qualifier. Only
an `IDENTIFIER` is flagged: a dotted type name such as `c.array` is not an expression. The member after a qualifier is
typed by the rules above (an enum variant, a constant or a static function of the file) or by the rule below.

**External calls.** `MAREA_SemaTypedNodeIsExternalCall` is true for a `CALL_EXPRESSION` of a name that the host or a
library registered: its callee names a member of a namespace that a resolver answers `EXTERNAL`. The callee is read in
the scope of the call, through parentheses: a full path that a cover reaches (`c.math.pow`), a path through an alias of
an import or a require (`io.puts`) and a bare name that an alias bound to one member (`power`). A call into a tree of
the project (a `FOUND` namespace), through `Self`, through a local type alias, of a name of the file or of its own
namespace, or of a local or a parameter (also one that hides an alias) is not one. The ir keeps the answer on the call,
and a backend sends only such a call to a C function.

**Members of a FOUND namespace.** When the name on the left of a member access stands for a namespace that the
resolver answers `FOUND` (an alias, or a full path that a cover reaches), the member is typed from the tree that the resolver gave.
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
(`marea.array.Array<Item,N>`, `marea.vector.Vector<Item>`) against the resolved type text of the typed argument at the same place: a bare
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
`marea.array.Array<sinteger32,4>` and the integer in an attribute are `sinteger32`. The key of `[key] = value` and the index of
`a[i]` take the default. The rule only records a type: it raises no diagnostic and changes nothing that sema accepts
or rejects.

## Layout of a structure and an enum

The typed node of a `STRUCTURE_DECLARATION` and of an `ENUM_DECLARATION` carries an opaque layout record,
`MAREA_SemaTypeLayout` (`marea/sema/MAREASemaLayout.h`), made by `MAREA_SemaBuildTypedAst` and valid for the arena
lifetime. It is the one place where sema says how big a type is and where its members sit, so a later stage never
measures a type again.

```c
const MAREA_SemaTypeLayout *layout;
MAREA_Uinteger64 size, alignment, offset;

MAREA_SemaTypedNodeGetLayout(node, &layout);               /* node: a typed structure or enum declaration */
if (MAREA_SemaTypeLayoutGetVerdict(layout) == MAREA_SEMA_MEASURE_VERDICT_MEASURED) {
    MAREA_SemaTypeLayoutGetSize(layout, &size);
    MAREA_SemaTypeLayoutGetAlignment(layout, &alignment);
    MAREA_SemaTypeLayoutGetMemberOffset(layout, 0, &offset); /* field 0, or the payload of variant 0 */
}
```

| Accessor | Gives |
|---|---|
| `MAREA_SemaTypedNodeGetBackend` | the backend of a typed function, resolved by the build of the typed tree against the target of the context (the function attribute over the file attribute, a holding `when` over a plain item, `auto` replaced by `intrinsics`, then `vector_extension`, then `scalar`), a NUL terminated member name. `ERROR_NO_BACKEND` for any node that is not a function of a file in `marea.math`, `ERROR_NULL_POINTER` for a null argument |
| `MAREA_SemaTypedNodeGetLayout` | the layout of a typed structure or enum declaration. `ERROR_NO_LAYOUT` for any other node (an opaque structure included), `ERROR_NULL_POINTER` for a null argument |
| `MAREA_SemaTypeLayoutGetVerdict` | `MAREA_SEMA_MEASURE_VERDICT_MEASURED` or `MAREA_SEMA_MEASURE_VERDICT_UNDECIDED` |
| `MAREA_SemaTypeLayoutGetReason` | `MAREA_SEMA_MEASURE_REASON_NONE` when measured, else the reason (below) |
| `MAREA_SemaTypeLayoutGetSize`, `GetAlignment`, `GetTagSize` | the size with its tail padding, the alignment and the width of the tag of an enum (0 for a structure), in bytes. `ERROR_UNDECIDED` when the layout is undecided |
| `MAREA_SemaTypeLayoutGetMemberCount` | the number of fields of a structure or of variants of an enum. Known for an undecided layout too |
| `MAREA_SemaTypeLayoutGetMemberOffset` | the offset of a field, or the offset of the payload of a variant. `ERROR_UNDECIDED` when undecided, `ERROR_NULL_POINTER` for an index that is out of range |
| `MAREA_SemaTypeLayoutGetVariantFieldCount`, `GetVariantFieldOffset` | the field count of one variant and the offset of one of its fields |

Every offset counts from the start of the value of the structure or of the enum.

**The rules.** Both values of the `layout` key give the same result, and so does a structure with no key: the fields
stay in the order written, each at the next multiple of its own alignment (the size of a primitive, the alignment of a
nested structure or enum), and the size is rounded up to the largest alignment. `pack = N` caps the alignment of a
member, a field `align = N` raises the alignment of that field, and `align = N` on the type raises the alignment of the
type. The layout may come from a file attribute. The value `marea` of the key stays reserved: a later change may
reorder the fields for it, and then only the offsets change, not the format. The widths are `boolean`, `character`,
`sinteger8` and `uinteger8` 1, `sinteger16` and `uinteger16` 2, `sinteger32`, `uinteger32` and `float32` 4, and
`sinteger64`, `uinteger64` and `float64` 8, each aligned to its size. A structure with no field has size 0 and
alignment 1. A type alias of a primitive is the primitive.

An enum is its tag at offset 0, then its data as a union. The tag is the written integer type, `sinteger32` when none is
written, and its width is the size of the type. Each variant is laid out as a structure from the start of its payload.
The payload is as large as the largest variant and starts at the first offset after the tag that fits the largest
alignment of a variant, so every variant has the same payload offset. The alignment of the enum is the largest one
among the tag and the variants, and the size is rounded up to it. An enum with no data is its tag. `pack` and `align`
on an enum act as they do on a structure. For `enum Shape : uinteger8 Circle(radius: float32); Rect(w: float64, h:
float64); Empty; end` the size is 24, the alignment 8, the tag 1, the payload offset 8 and the fields of `Rect` sit at
8 and 16.

**Undecided on purpose.** A type that sema does not measure has the verdict `UNDECIDED` and a reason, and no size, no
alignment and no offset. The record has the same format for a measured layout, so a later change can measure a type
that is undecided today without a change of format. The reasons are:

- `LIBRARY_TYPE`: a field or a variant field of `string`, `array`, `vector`, `pointer`, `owned`, `slice`, `task`, or
  any type of a library under `c.{lib}` or `marea.{lib}`, or a name that sema cannot see. The layout of such a type
  comes from its library later.
- `GENERIC`: a generic structure, or a field whose type is a generic parameter.
- `FUNCTION_TYPE`: a field of a function type. Its width is a choice of the backend.
- `OPAQUE`: an opaque structure, which has no known layout.
- `INVALID_ATTRIBUTE`: a `pack` or an `align` that sema cannot use (zero, or above 4294967296).
- `TOO_DEEP`: a chain of types held by value deeper than 8 levels, a type that holds itself, or a type whose
  measure visits more than 1048576 declarations.
- `UNRESOLVED`: a type that sema cannot trace to a declaration (an alias loop), an enum bound by an import (it belongs
  to another file, whose names and file attributes this run cannot see), and an enum tag that is not an integer
  primitive, which `INVALID_ENUM_TAG_TYPE` (3031) rejects already.

A structure or an enum that holds an undecided type is undecided too, with the reason of that type.

The layout does not change what sema accepts or rejects. The convert check of `convert<T>(x)` between two structures
keeps its own rule (see "Conversion" under "Rules") and reads the same measure: a structure that holds an enum is
still not compared, and a `marea` structure inside a `c` structure is still not compared.

## Diagnostics

| Code | Name | Example |
|---|---|---|
| 3001 | `UNRESOLVED_IMPORT` | `import a.b.*;` when `a.b` is not found; `import a.b.c;` when the resolver finds neither `a.b.c` nor a `c` that is not private in `a.b`; `import a.b { f };` when `a.b` is not found, `f` is a private symbol of it or is missing, a two-part member is not a variant of an enum that is not private, or the member path has more than two parts; with a registry, an `import` of a name that only the registry knows |
| 3002 | `NAMESPACE_PATH_MISMATCH` | `namespace a.b.c;` in the file `a/b.marea` |
| 3003 | `NAME_ALREADY_DECLARED` | `structure Item end function Item() end`, or an import (or an import alias, or a member alias) and a declaration or another import of one local name; an import alias or a `type` named `c` (message `name is reserved:`); an import alias named like a primitive type, `import c.foo as sinteger32;` (message `name is a primitive type:`); a generic parameter that repeats an earlier one of its list or names a top-level declaration, `pick<Item, Item>`; two parameters of one name; a body local that repeats a parameter or a local of the same block |
| 3004 | `UNDECLARED_NAME` | `where Item: Missing`, or a constraint on a name that is not a generic parameter; `local x: sinteger32 = missing;`, `missing(1);`, `missing.field`, `stdio.puts(x)` when no alias binds `stdio` and no resolver knows it, `math.sqrt(x)` after `require marea.math;`; a bare function name that stands in a function body before the function is declared in the file |
| 3005 | `UNKNOWN_ATTRIBUTE_KEY` | `#[frobnicate = true]`, `#[simd]`, `#[public]`, also inside `when(...)` |
| 3006 | `INVALID_ATTRIBUTE_VALUE` | `#[inline = maybe]`, `#[build = 1]`, `#[visibility]`, `#[export]`, `#[export = "run it"]`, `#[backend = fast]`, `when(colour = "red", ...)`, `when(pointer_width = "64", ...)` |
| 3007 | `CONFLICTING_ATTRIBUTES` | `#[pack = 1, align = 16]`, a key given twice (`visibility` twice included), two `when` items that hold and give one key |
| 3008 | `INVALID_ALIGNMENT` | `#[align = 15]` |
| 3009 | `UNKNOWN_TYPE` | `a: Missing;` in a structure |
| 3010 | `ARGUMENT_COUNT_MISMATCH` | `add(1)` when `add` takes two parameters with no default; `f()` or `f(1, 2, 3)` when `f` has one required and one defaulted parameter; `Point(1.0, 2.0, 3.0)` when `construct` takes two; `Line(1.0)` when `Line` declares no `construct`; `power(1.0)` when the host's signature of `c.math.pow` takes two parameters |
| 3011 | `TYPE_MISMATCH` | `convert<sinteger32>(text)` with a `string` name, `convert<Point>(other)` for two structures with another layout, `f("text")` when the parameter is `sinteger32`, `local x: boolean = 1;`, `local n: sinteger32 = a < b;`, `f(s)` with a `string` name `s`, `return a + 1;` in a function that returns `boolean`, `return same(text);` in a function that returns `sinteger32` when `same<Item>` returns `Item`, `a + b` with an `sinteger32` and a `float64`, `count(2)` when `count` is an `sinteger32` local, `Point("x", 2.0)` when `construct`'s first parameter is `float32`; `power("two", 10.0)` when the host's signature of `c.math.pow` takes `float64, float64`; `local x: sinteger8 = item.x;` over a `for each` whose element field is `sinteger32`, `local first: float32 = values;` when `values` is a `demo.slice<float32>`, `f("text")` when the parameter is a slice or a raw pointer; `s + s` with two strings, `-p` with a `boolean`, `p == q` with two booleans, `point + 1` when the `+` function of `Point` takes a `Point`; `case "one" then` over an integer, `case Shape.Circle then` over a `Color`, `for index in 0 .. true do` |
| 3012 | `DUPLICATE_MEMBER` | two `implementation` blocks of one type that both declare `area`; a structure field, an enum variant or a contract function that is declared twice in its declaration |
| 3013 | `CONTRACT_NOT_SATISFIED` | `maximum(a, b)` when the type of `a` has no `less` for `where Item: Comparable` |
| 3014 | `GENERIC_ARGUMENT_MISMATCH` | `Pair<sinteger32>` when `Pair` has two parameters, `Box<4>`, a registered type written with more type arguments than its descriptor declares, `local x: marea.array.Array<sinteger8, 4> = values;` when `values` is `marea.array.Array<sinteger32, 4>`, an `owned<vector<T>>` given where a `vector<T>` is declared |
| 3015 | `INSTANTIATION_TOO_DEEP` | a type argument nested 65 levels |
| 3016 | `ASSIGNMENT_NEEDS_COPY_OR_MOVE` | `a = b;` when the type is not a primitive, a copyable `array` or an enum; a registered type of the move class needs the word too; so does a name, a field or an element given to a parameter with the owner role, reported at the argument (`MAREA_SemaCheckOwnedArguments`; a `vector` or `task` parameter is not checked, and an argument of unknown type is accepted) |
| 3017 | `INVALID_BORROW` | `#[semantics = value_read_only] s: Shape` when `Shape` is not a primitive |
| 3018 | `REFERENCE_ESCAPES_SCOPE` | `return s;` when `s` is a borrow parameter; `return s.inner;` when the field is not a value (also in an arm of `switch`); `shapes.append(s);` (the one storing library call that sema knows by name); `kept = part;` when `kept` is not declared in the `partition` body |
| 3019 | `EXCLUSIVE_BORROW_CONFLICT` | `take(s, s)` when one parameter's effective `semantics` is `reference_mutable`; also two arguments that name overlapping places when one is a write borrow or a `move` (`two_write(b.inner, b.inner)`, `two_write(v[0], v[0])`, `wr(b.inner, b)`, `wt(p, move p)`, `pair(item, v)` in a `for each item in mutable v`; `MAREA_SemaCheckAliasedArguments`) |
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
| 3032 | `CANNOT_BORROW_CONSTANT_AS_MUTABLE` | `take(ORIGIN)` or `take(ORIGIN.inner)` or `take(ORIGINS[0])` when `take`'s parameter's effective `semantics` is `reference_mutable` and the root of the argument is a file constant |
| 3033 | `LITERAL_OUT_OF_RANGE` | `local a: uinteger8 = 256;`, `local b: sinteger8 = -129;`, `constant BIG: uinteger8 = 300;`, `local c: uinteger32 = -1;`: an integer literal whose value does not fit the integer type it is bound to |
| 3034 | `DIVISION_BY_ZERO` | `x / 0`, `x % 0`, `x /= 0`, `x %= 0` for an integer: the divisor is a constant zero |
| 3035 | `SHIFT_COUNT_TOO_LARGE` | `x << 32` for `uinteger32`, `x >>= 64` for `sinteger64`, `x << -1`: a constant shift count that is negative or not below the width of the left integer type |
| 3036 | `NEGATE_UNSIGNED` | `-value`, `-(a + b)` or `-point.count` when the operand has a known unsigned primitive type |
| 3037 | `SIGNED_DIVISION_OVERFLOW` | `-2147483648 / -1` or `-128 % -1` when the left side folds to the lowest value of the signed type and the right side folds to -1 |
| 3038 | `BITWISE_OPERAND_NOT_INTEGER` | `a & b`, `a \| b`, `a ^ b`, `~a`, `a &= b` with a `float32` or `float64`, a `boolean`, a `string` or a `character` operand, or a float literal |
| 3039 | `LOGICAL_OPERAND_NOT_BOOLEAN` | `a and b`, `a or b`, `not a` with an operand whose known type is not `boolean`, or a number, string or character literal |
| 3040 | `ASSIGNMENT_TO_CONSTANT` | `LIMIT = 11;`, `LIMIT += 1;`, `ORIGIN.x = 1;`, `TABLE[0] = 1;` when the root name is a file constant and no local hides it |
| 3041 | `RAW_POINTER_OUTSIDE_UNSAFE` | a written type with the raw pointer role (`local raw: c.memory.Pointer<sinteger32> = f();`, a parameter, a result, a field, an alias) with no `unsafe ... end` block around it, at any depth of nesting |
| 3043 | `NON_BOOLEAN_CONDITION` | `if n then`, `elseif s then`, `while 1 do`, `do ... while 1 end` when the condition has a known type that is not `boolean` (a primitive, a structure, an enum, a view, an array), or is a number, string or character literal |
| 3044 | `UNKNOWN_FIELD` | `Point{ z = 1.0 }`, `point.z` or `point.z = 1.0;` when `Point` has no field `z` |
| 3045 | `RECURSIVE_TYPE` | `structure Node next: Node; end` |
| 3046 | `CYCLIC_TYPE_ALIAS` | `type First = Second; type Second = First;` |
| 3047 | `EMPTY_ENUM` | `enum Empty end` |
| 3048 | `UNKNOWN_ENUM_VARIANT` | `Color.Purple` when `Color` has no variant, function or constant `Purple` |
| 3049 | `UNKNOWN_TYPE_FUNCTION` | `point.length()` or `Point.origin()` when no `implementation` of `Point` declares it |
| 3050 | `OPERATOR_SIGNATURE_MISMATCH` | `#[operator = "+"] function add(self: Point): sinteger32` |
| 3051 | `BOUND_OPERATION_NOT_GIVEN` | `left + right` or `left.size()` on a generic `Item` that no `where` bound gives |
| 3052 | `OVERLAPPING_IMPLEMENTATION` | `implementation<Item> Box<Item>` and `implementation Box<sinteger32>` that both declare `same` |
| 3053 | `FUNCTION_TYPE_MISMATCH` | `apply(negate)` when `negate` takes one parameter and the function type takes two, `apply(holder.run)` or `apply(make())` when the field or the result has another function type |
| 3054 | `INVALID_RESULT_USE` | `return 1;` in a function with no result, or `local v: sinteger32 = log_line("x");` when `log_line` has no result |
| 3055 | `CYCLIC_CONSTANT` | `constant A: sinteger32 = B;` with `constant B: sinteger32 = A;`, or `constant A: sinteger32 = A;`, once for each cycle, at the first constant of it in the file |
| 3056 | `RESERVED_MACRO_NAME` | `function print() end`, `constant format: sinteger32 = 1;`, `local log: sinteger32 = 1;` or `import c.stdio as print;`, `for line in 0 .. 3 do end` or `with v as format do end`: a name that is a built-in macro |
| 3057 | `OPAQUE_TYPE_MISUSE` | `Texture{ }`, `texture.width` or `texture + other` when `texture` is a local or a parameter of an `opaque structure` of the same file |
| 3058 | `OPAQUE_FIELD_BY_VALUE` | `structure Holder texture: Texture; end` with `Texture` an `opaque structure` of the same file |
| 3059 | `COMPILE_TIME_ASSERTION_FAILED` | `compile_error!("stop");` with any arguments, at the top level or in a function body; `static_assert!(1 == 2, "never");` or `static_assert!(N == 5, "n");` when the condition is a constant that is false |
| 3060 | `UNKNOWN_MACRO` | `shout!("hello");` when `shout` is none of the 14 built-in macros, `marea.array.nothing!("x");` or `array.print!("x");` when the library registers no such macro, or a macro of a library that is off, at the call |
| 3061 | `INVALID_MACRO_ARGUMENT` | `print!();`, `print!(1, 2);`, `print!("{} {}", 1);`, `assert!(1, "x");`, `unreachable!(1);`, `log!(loud, "x");`, `include_text!(1);`: the wrong count of arguments, a literal of the wrong kind, a log level that is not one of the five, a text that is not a string literal, a condition that is not a `boolean`, a count of `{}` that differs from the count of values, or a `{}` value that is not printable (`print!("{}", point)`), at the offending argument; `build_value!("X", sinteger32)`, a first argument that is not a string literal, a type argument that is no primitive and no enum of the file, an enum with a field that has no default; a call of a macro of a library with the wrong arguments or that the expander refuses (`marea.array.include_bytes!("missing.bin");`) |
| 3062 | `PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN` | `function f(a: sinteger32) spawn g(a); end` or `partition values into 2 as part do end` with no `fork_join` block around it, also a `spawn` after the `end` of a block: raised at the `spawn` or `partition` and nothing else for that construct |
| 3063 | `SPAWN_NEEDS_CALL` | `fork_join spawn point; end` or `spawn print!("x");`: the operand of a `spawn` is not a call, at the operand |
| 3064 | `PARTITION_SOURCE_NOT_ARRAY` | `partition count into 2 as part do end` when `count` is a primitive, a structure, an enum, a number literal, a function value or a registered type with no array shape, at the source |
| 3065 | `LOCAL_WITHOUT_INITIAL_VALUE` | `local x: sinteger32;` with no `= value`, for a primitive, a structure, a `pointer` or an `array`, at any depth of nesting |
| 3066 | `PRIVATE_SYMBOL_ACCESS` | `b.hidden()` or `b.LIMIT` after `import a.b;` (or an alias) when the member is a top-level declaration of a `FOUND` namespace with `#[visibility = private]` |
| 3067 | `PRIVATE_TYPE_ACCESS` | `s: b.Secret`, `b.Secret{ }` or any written `b.Secret` when the structure, enum, opaque or alias of a `FOUND` namespace is private |
| 3068 | `PRIVATE_FIELD_ACCESS` | `box.value`, `box.value = 1` or `b.Box{ value = 1 }` when `box` is written `b.Box` and the field has `#[visibility = private]` |
| 3069 | `WRITE_THROUGH_READ_ONLY_BORROW` | `function f(p: Point) p.x = 1; end`, `for each v in mutable xs do end` with `xs` read-only, `write_point(p)` with a read-only `p`, `write_point(make_point())` or `set(1)` for a write-borrow parameter |
| 3070 | `USE_OF_MOVED_VALUE` | `local b: Point = move a; local c: sinteger32 = a.x;`, `take(move a); read(a);`, `spawn read(a);` or `a.sum()` after `a` was moved, also after a `do` block or after a branch where any arm that reaches the end moved it |
| 3071 | `VALUE_MOVED_AGAIN` | `move a;` twice, or `move a` of a local declared outside the loop in a loop body that has no `break` or `return` of its own, at the name or at the `move` |
| 3072 | `USE_OF_PARTIALLY_MOVED_VALUE` | `local s: Shape = move o.inner; local x: float32 = o.inner.x;`, `move o.inner;` then `take(move o);`, `o.total()` after `move o.inner`, `move v[0]; ... v[0].x`, or `move item` of a `for each` element of a collection of a type that is not by value, at the access or at the name |
| 3073 | `VALUE_CHANGED_WHILE_BORROWED` | `for each x in v do v[0] = ...; end`, `for each x in v do local o: marea.vector.Vector<P> = move v; end`, `for each x in mutable v do v.append(...); end`, `with p as view do p.x = 2; end` or `with p as view do local o: P = move p; end`: the source of a `for each` or a `with` block is written, moved or changed by a library call (`append`, `insert`, `remove`, `clear`, `pop`, `push`, `resize`) in the body |
| 3074 | `CONFLICTING_PARALLEL_ACCESS` | `spawn write(a); spawn write(a);`, `spawn write(a); spawn read(a);`, `spawn write(a); a.y = 5;` or a `spawn write(a);` in a loop in a `fork_join` block when `a` is declared before the block, and `partition xs ...` with a `spawn write(xs)` in the same block |
| 3075 | `PARTITION_BODY_WRITES_OUTER_NAME` | `local c: sinteger32 = 0; fork_join partition values into 2 as part do c += 1; end end` (also `point.x = 2;` and a parameter) | `a partition body cannot write an outer name: 'c'` |
| 3076 | `PARTITION_SOURCE_USED_IN_BODY` | `partition mutable values into 2 as part do values[0] = 1.0; end` (a read or a `for each` over the source too, except in a `spawn`, which is 3020) | `the source of a partition mutable cannot be used in the body: 'values'` |
| 3077 | `MISSING_FIELD` | `Point{ x = 1.0 }` or `Point{}` when `Point` also has the field `y` (the first missing field in declaration order, once per literal) | `struct literal misses a field 'y'` |
| 3078 | `UNIMPORTED_NAMESPACE` | `c.stdio.puts(x);` with no `require` that reaches `c.stdio`; `function f(values: marea.array.Array<sinteger32, 4>) end` with no `require marea.array;` |
| 3079 | `UNRESOLVED_REQUIRE` | `require nowhere.library;`, `require nowhere.*;`, or a `require` of a project file that only the import resolver knows (with a registry) |
| 3080 | `AMBIGUOUS_NAMESPACE` | `import demo.io; require demo.*;`: one full path under both keywords |
| 3081 | `RESERVED_NAMESPACE_ROOT` | `namespace marea.tools;` or `namespace c.tools;` in a project file, with no exception |
| 3082 | `ELEMENT_WITHOUT_MEASURED_LAYOUT` | `marea.array.Array<function(): sinteger32, 2>`: an element type with no measured layout held by value, at the element |
| 3083 | `BACKEND_UNAVAILABLE` | `#![backend = intrinsics]` in `marea.math` with no target, or on `riscv64`, or with `msvc`; `#[when(architecture = "riscv64", backend = intrinsics)]` on `riscv64` |
| 3084 | `ATTRIBUTE_NOT_ALLOWED_HERE` | `#![backend = auto]` or `#[backend = scalar]` in a file whose namespace is not `marea.math` or below |
| 3085 | `MISPLACED_ATTRIBUTE` | `#[align = 16]` on a function, `#[inline = force]` on a structure or a constant, `#[operator = "+"]` on a free function, `#[visibility = public]` on a parameter or a macro statement, `#[export = "label"]` on a generic function, `#[backend = auto]` on a structure of `marea.math`: any `no` cell of the placement matrix, also inside `when(...)`, at the item |
| 3086 | `INVALID_BUILD_VALUE` | a setting `LOG_LEVEL` with the text `high` for `build_value!("LOG_LEVEL", sinteger32, 2)`, `256` for a `uinteger8`, `yes` for a `boolean`, `Slow` for an enum with no such variant, at the call |

## Gaps

- `UNKNOWN_ENUM_VARIANT` (3048) checks a member access of an enum of this file only. A `case Color.Purple then` label
  is not a checked slot, so a label that is not a variant is not seen. An enum of another namespace or an imported
  enum is not checked, because its `implementation` blocks are in another file. The call form `Color.make()` is the
  rule of 3049.
- The array rule of `partition` (3064) does not judge a source that sema cannot trace to a type: a type that no
  descriptor describes, a call result that sema does not type, and an owner (the value that it holds is not read).
- A write through a method call on a read-only receiver, and a library call that mutates (`values.append(x)`), are
  not seen: sema does not know which calls mutate. The rule 3069 reads only a written assignment, a mutable loop or
  partition and a write-borrow argument.
- The visibility check (3066 to 3068) reads only a bare namespace name and a bare name with a written type `ns.T`. It does
  not check a member that the tree does not declare, a field of a value that is not a plain `ns.T` name (a call result,
  an index, a chain such as `a.b.c`), a field of a type that an imported symbol gives (`import a.b.Box;`), or a private
  symbol that a generic or an alias leads to.

- The full-path rules read the member chain of a value or a call and the qualified name of a type. A full path to a type of a
  `FOUND` namespace (`a.b.Item`, with no alias) is checked for its cover and its existence as a namespace prefix only: the
  visibility checks of a type and of a field (3067, 3068), the layout of such a field and the typed spelling of the type
  read the alias form (`import a.b as b;` then `b.Item`). A full path to an enum of a `FOUND` namespace with a variant
  (`a.b.Color.Red`) is covered but not read for its variants.
- A private member reached by the full path of a namespace is `PRIVATE_SYMBOL_ACCESS` (3066) as before; a cover that is
  an unresolved declaration still covers its path, so the use raises nothing beyond the unresolved report.
- The IR does not lower a full path yet: a chain before a member is only flagged as a qualifier, so the unit lowers like one
  with an alias.
- Two functions that call each other cannot be written, because a function must be declared before its use and the
  grammar has no forward declaration.
- The name of a `for`, `for each`, `partition` or `with` statement is not checked against the names of its own
  scope: it can repeat a local of the same block, and a local of its body can repeat it.
- No check of the signature of `main` exists any more, and a driver-level replacement does not exist yet.
- No real library exists yet: the types of `marea.array`, `marea.vector`, `marea.tuple`, `c.memory` and `c.threads` are registered
  by fixtures in the tests (under the neutral root `demo` and under the planned full paths), so the roles are proven but no shipped
  descriptor set is. A registered type is checked for too many type arguments only, never too few (the examples write a growable
  `marea.array.Array<T>`); a `vector` is still indexable in the type inference, though it is a move type.
- `include_bytes!` is not a macro of `marea.array` yet: the real macro is written with the library (apply `lib-array-vector-tuple`),
  and the fixture libraries of the tests prove the mechanism. The bare `include_bytes!` stays in the table of the language macros
  as a placeholder, and `examples/macros/builtin.marea` still writes it; both leave with the real macro. A bare call
  whose file the include resolver does not know is not reported (only the call by a qualified name is).
- The `when` item is read only by the attribute check. Another reader (`visibility`, `layout`, `align`, `pack`, `operator`,
  `export`, `inline`, `build`) sees the plain items, so a `when` that gives such a key has no effect on that reader yet. The
  `backend` rule reads it, since it is the only check that is about the target.
- A `when` predicate value is not checked against a list (`architecture = "riscv64"` is accepted for any text), because the
  sets are open; only the name and the kind of the value are checked.
- `build_value!` does not resolve a qualified type argument (`other.Mode`): it is accepted and nothing about it is checked,
  because sema has no way to read the declaration of an enum of another namespace from an expression. A float, a string, a
  character and an enum from the build are converted for the check but never folded for `static_assert!`.
- The `auto` backend has the start rule only (decision 18): the table of the best backend per operation does not exist.
  Backend rows that need a library that does not exist yet (`marea.math`) are proven by files written in the tests under that
  namespace, and a project file under the root `marea` is always `RESERVED_NAMESPACE_ROOT`, so a shipped `marea.math` file
  cannot pass the checks of sema clean until a shipped-file entry exists.
- The IR lowers `build_value!`, a macro with a qualified name and `do ... while` now (see the ir doc, "Macros" and "Do-while
  lowering"). A macro call that the IR lowers (`build_value!`, a qualified name) has no resolved type in the typed tree, so the IR
  reads the type from the declaration around it.
- A type that holds a table (two type parameters, no index protocol) is found by that shape, not by a role: a keyed collection
  has no role of its own in the roles table yet.
- Renaming on use inside one file (without an import) is not offered: a local declaration has its own name.
- `TYPE_MISMATCH` (3011) sorts the type of an expression into a fixed set of shapes: a literal, a primitive, a
  structure, a view, a raw pointer, and, known but never compared, an enum, a library type, a generic parameter and a registered type of the move class or an inline run. Still fully unknown: any other registered type and the result of a call of an imported function or of an `EXTERNAL` member (its host signature is read
  for the arguments only).
- A struct literal with a missing field is rejected (3077, one report per literal, so a second missing field shows only after the first is written). The ir has no `ZERO_VALUE` any more (removed in PR #355): no accepted program has an omitted field, so the ir has nothing to fill.
- A call of a method or of an imported function that has no result, used as a value, is not seen: sema cannot see its
  result (`INVALID_RESULT_USE`, 3054).
- Inside the body of a generic declaration a generic parameter in scope is never compared with a concrete type, and the
  field values of a literal of a generic structure are not compared with the type arguments of the literal. The
  arguments of a construction call of a generic structure are not compared after binding either.
- A pair of types is not compared (3014) when a head is a generic parameter in scope, an alias, an import or an
  unknown type, when only one side has type arguments, or for a `for each` over a collection of unknown type.
- A function type that mentions a generic parameter in scope, and a generic function given as a value, are never
  compared (3011, 3053). A bare name of an imported function, and a value that sema does not type, are accepted.
- A declared operator is read at its use only on a binary expression: a compound assignment and a unary operator do not
  read it, its result type is not read, and a type with two functions for one operator is not judged.
- `UNKNOWN_TYPE_FUNCTION` (3049) skips a receiver of another namespace, a library type, a generic parameter and an
  unknown type, because sema cannot see their functions. Only a bare name or a local that is a bare name is a receiver:
  a call on a field chain or on a call result is not checked. A function of a contract on a generic receiver is not checked.
- The `for`, `for each`, `partition`
  and `with` header expressions and the initializer of a file constant are checked. The loop name is already in
  scope when its own header is checked, so `for i in 0 .. i` with no outer `i` raises nothing. A constant that has
  an `ERROR_NODE` below it has no name check, because the parser may leave its tokens as an expression.
- `OPAQUE_TYPE_MISUSE` (3057) sees only an operand or a base that is a bare name, or one in parentheses, whose
  written type is an opaque structure of the same file. A call result, an index, a field of another value and an
  opaque type of another file are not classified, and `OPAQUE_FIELD_BY_VALUE` (3058) likewise sees only a bare
  opaque name of the same file as a field type.
- `ASSIGNMENT_TO_CONSTANT` (3040) sees only a constant that a file declares. A constant that an `implementation`
  declares is not rejected.
- `COMPILE_TIME_ASSERTION_FAILED` (3059) does not fold a call other than `size_of` and `align_of` (`convert` included),
  a character, a constant of another namespace or the size of an imported or library type: such a condition is accepted.
- `CYCLIC_CONSTANT` (3055) sees only the constants that a file declares at the top level. A constant that an
  `implementation` declares is not part of the search. A name is matched by its text in the name table, so a
  qualified name of another namespace is not followed.
- `INVALID_MACRO_ARGUMENT` (3061) accepts a `{}` value or a condition of an unknown type, such as an enum variant written
  `Color.Red` or a call that sema does not type. The names inside a macro argument are not checked. The five log levels
  are an assumption (see Built-in macro).
- `RAW_POINTER_OUTSIDE_UNSAFE` (3041) reads the role of the written type in every place, but a type alias of a raw pointer is
  not followed through its use: the alias declaration itself is the place that is rejected.
- `RECURSIVE_TYPE` (3045) does not follow a user generic structure given as `Box<Node>`: its field types are written in
  its own scope, so a recursion through it is not seen.
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
  is wide enough for the number of variants. The tag gives the width of the enum in its layout record, and nothing
  else reads it.
- The contract check only sees arguments whose type is cheap to know. Other arguments are not checked.
- The bound check of 3051 reads the `where` clause of the function only. A generic parameter of an `implementation` block has no bound there, so a call on a value of it is 3051.
- `operator` is checked against its vocabulary; only `construct` and `destroy` have a placement and a shape check
  (see "Construct and destroy"), and any other operator has a signature check in an `implementation` only (3050, see
  "Operator signature"); `export` is checked for shape only.
- `LIFECYCLE_OPERATOR_CALLED_DIRECTLY` (3027) on a call on a value judges only a receiver whose inferred type is one
  plain type name of the file. A receiver of unknown type, of another file or a library, a call result that sema does
  not type, and a receiver typed `Self` (a recorded Gap: `Self` is not resolved to the implemented type here) raise nothing (round toward valid). A bare name that no
  top-level function declares also raises `UNDECLARED_NAME` (3004), as a member is not a bare name.
- `MAREA_SemaTypedNodeGetResolvedType` covers the six declaring nodes, the literals and the expressions of "Resolved
  type of an expression". A `Self` type and a generic parameter are spelled by
  their written name, not by the type they stand for in one instantiation. A name of an `EXTERNAL` namespace is
  spelled by what is written and the import alias rule, with no qualifier added; a type that a `FOUND` namespace
  declares is spelled with the last segment of that namespace instead (see "Members of a FOUND namespace").
- The declared name of a `partition` (the part) and of a `with` carries no resolved type on its own typed node, though a read of the
  name does: only the declared names of a `for` and a `for each` carry one (see "Loop names and collection"). Neither has a
  consumer that needs it yet.
- An expression with no resolved type. The sweep of the 23 examples (838 expression nodes, 54 of them qualifier names
  that are not values and are counted on their own) finds 75 expression nodes with no type, with a fixture signature
  function that answers `filled`, `append`, `sum` and `result` (see "Members of a library type"), and every one of the
  75 is in one of the nine classes below. The list is closed: the sweep test
  (`tests/source/sema/MAREASemaTypedAstExamples.cpp`) fails with the file, the position and the name for any other
  untyped expression node, and it fails for a class that no example reaches any more, so a class that a fix empties
  leaves this list in the same change. A literal never has a place in the list: every literal has a type. The `ir`
  stage can treat every untyped expression outside these classes as `UNTYPED_EXPRESSION` (4006).
  - `MACRO_SUBTREE` (19): a `MACRO_CALL` and every node below one (`info` of `log!`, the call `power(...)` of
    `print!`, `size_of` and `align_of` of `static_assert!`), `line!()` and `include_bytes!` excepted, which have a type.
    The `ir` raises 4005 for a macro.
  - `QUALIFYING_TYPE_NAME` (2): a `GENERIC_NAME` on the left of a member access, a written type that qualifies a static
    member (`marea.array.Array<sinteger32, 4>.filled(8, 0)`). It is not a value, and sema does not flag it as a qualifier name,
    which is only an `IDENTIFIER`.
  - `BUILTIN_CALLEE_NAME` (7): the `IDENTIFIER` `size_of`, `align_of` or `convert` that is the callee of a call. The
    call has the type; the name has none.
  - `UNTYPED_FUNCTION_NAME` (3): an `IDENTIFIER` that is the callee of a call and has no function type: `power` and
    `square_root`, imported by name from an `EXTERNAL` namespace that gives no signature, and `maximum`, a generic
    function of the file (a generic function name has no function type; the call of `maximum` has its inferred type).
  - `UNTYPED_FUNCTION_MEMBER` (19): a `MEMBER_ACCESS` that is the callee of a call, with no function type: a function
    or method of an `EXTERNAL` namespace or of a library type (`stdio.puts`, `vulkan.present`, `tasks.append`,
    `item.result`, `Arena.create`, `product.sum`), a static member of a written generic type, or a method on a
    receiver of a generic parameter type (`left.less(right)`, `winner.describe()`). The call may have a type of its
    own (`tasks.append(...)`, `left.less(right)`); the member never does.
  - `UNTYPED_CALL` (19): a `CALL_EXPRESSION` whose callee has no type and that no rule types: a call of an `EXTERNAL`
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
  and `align_of` as names; every `GENERIC_NAME` and every `MACRO_CALL` but a bare `line!()` and `include_bytes!`.
- A binary operation has the type of its operands even when an operator function of the structure declares another
  result type: the result of `#[operator = "+"]` is not read. A field of a variant is read from the first variant that
  declares the name.
- The type of a literal is the default (`sinteger32`, `float64`) where the context is not a primitive that sema can
  read: a generic parameter type (`g(5)` with `a: Item`, also with explicit type arguments), an enum or a library
  type (an integer literal that `TYPE_MISMATCH` accepts for an enum is `sinteger32`, not the tag type), an entry of a
  library literal such as a table, the index of `a[i]`, the key of `[key] = value`, the count of a shift, a literal
  inside a macro call, and an operand whose type sema does not know (a call of a generic function that nothing
  binds). The default itself is an assumption of the plan, not a rule of the grammar. The range of a
  value is not checked against the type that the context gave: `local x: uinteger8 = 256;` stays accepted, as before.
  The literal of `convert<T>(x)` takes `T`, so `convert<uinteger8>(300)` types `300` as `uinteger8`.
- The typed-AST hand-off substitutes a generic result for a bare parameter with explicit type arguments, and now also
  for a bare parameter bound through "Inferred type arguments" when the call gives none; a compound result (such as
  one written `marea.array.Array<Item>`) still keeps the raw type either way, since only a parameter, never a result, is
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
- The layout record (see "Layout of a structure and an enum") leaves a library type undecided on purpose: a field of
  `string`, `array`, `vector`, `pointer`, `owned`, `slice`, `task` or any `c.{lib}` or `marea.{lib}` type makes its
  structure or enum undecided until the library gives the layout. A function type field is undecided too. The
  `marea` value of the `layout` key is measured like `c` and stays reserved so a later change can reorder the fields.
  An enum that is imported from another file is undecided (`UNRESOLVED`) too, because the names and the file
  attributes of its own file are not visible to this run. The convert check still does not compare a structure that
  holds an enum, though the layout of that enum is known now.
- The parser's `<`-opens-type-arguments lookahead (see [the AST doc](../ast/marea-ast.md),
  "Lookahead rules") does not tolerate `#[...]` inside the parameter list of a function type used
  as a type argument (`f<function(#[semantics = reference_mutable] x: T): T>()`); that scan's token
  vocabulary was never widened for attributes, so such a type argument fails to parse as one. The
  same scan already had no tolerance for the retired `mutable`/`reference` parameter keywords it
  used to allow, so this is not a new restriction, only an unaddressed one.
- Borrow liveness is tracked for the body of a `for each` and a `with` block only (3073, see "Borrowed source"), by a scan of the body, not by
  the table below. A borrow parameter is still not tracked across statements, and sema keeps no per-variable "borrow ended" bit. The settled design for every later
  check (use after move, partial move, borrow liveness, cross-spawn, `partition` body) is one rule set,
  recorded here so no check invents its own:
  - **State table.** One new per-scope table, modeled directly on `references`
    (`MAREA_SemaNameTable` in `MAREASemaInternal.h`): `MAREA_SemaBindReferences` (`MAREASemaOwnership.c`)
    pushes entries on scope enter, and they pop on scope leave. The new table is pushed on scope enter,
    popped on scope leave, and read and written inside the same `MAREA_SemaVisit` pass. There is no second
    traversal and no control-flow graph.
  - **Merge rule.** At a branch or loop boundary (`if`/`elseif`/`else`, the arms of a `switch`, a loop
    body) the merge follows `MAREA_G_Terminates` (`MAREASemaControlFlow.c`), which says a path terminates
    only when every arm agrees (`MAREA_G_IfTerminates`, `MAREA_G_SwitchTerminates`). The moved bit of a
    local is a "maybe moved" join: OR over the arms that reach the end, and OR with the base state when the
    statement has no `else` or `default`. A borrow "definitely ended" still needs every arm to prove it. A loop body is visited once, so an effect that can happen on a later iteration
    but not on the first is treated as "maybe already happened" once the walk is past the loop.
  - **Rounding rule.** The moved part (3072) and a borrow state, when the effect of an arm is ambiguous, round
    toward "still valid" (no part moved, still borrowed), never toward "reject": a part counts after a branch
    only when every arm that reaches the end moved the same part. The whole-local moved bit is the exception, a
    maybe-moved join by OR (issue #198, decision of the user), because a move on one path is a real move.
- The borrowed-source check (3073) is a scan, and it has these limits. A library call that changes its receiver is seen only by the names
  `append`, `insert`, `remove`, `clear`, `pop`, `push` and `resize`. A change made inside a called function, or through a second name for the
  source (a `with` name that is a view of it), is not seen. A write or a move in a nested function-like body is not a case, because the language
  has none. Two elements of one vector overlap in a call unless both indices are integer literals of two values, so `two_write(v[i], v[j])` is
  rejected even when `i` and `j` differ at run time.
- The move state (3070, 3071) is the first check built on that design. It covers a whole local or parameter only; the moved part
  (3072) is a second, narrower state on the same entry. A moved `with` name is not tracked. The moved part keeps one chain per local
  (the first), so a second moved part, a part moved with a computed index, a part whose type sema cannot see (a library type, a
  generic), a part moved in a loop body (a second turn is not reported) and a part reached through a borrow are not tracked, and a use
  of one of them is accepted by the rounding rule. A move in some arms of an `if` or `switch` counts after the statement
  (items `L-OWNERSHIP-042`, `L-OWNERSHIP-044` and `L-ADVERSARIAL-003` are reject items); a move in a loop body never counts after the loop.
  A loop body that has a `break` or a `return` among its own statements is never reported, even if the move comes after a conditional
  `break`. An arm with a `break` among its own statements leaves the loop, so it takes no part in the merge of the branch; a `continue`
  inside an arm does not end the arm for the merge. A name read through a function-type local is a read
  like any other. Move state is not read across a function call: a callee that takes an `owned` parameter is seen only through the
  `move` written at the call.
- The shared access check (3074) is conservative. It sees only a callee that is a function of the same file, found by a bare
  name, so a worker that calls a method, a function of another namespace or a function value records nothing. A statement of
  the block body is compared with the workers that started in earlier statements only: a worker that starts inside the same
  statement (a loop around a `spawn`) or later is not compared with it, and a body statement before a `spawn` in a loop is not
  compared with the next turn. A local declared inside a nested block of the body statement is not in the scope table when the
  statement is scanned, so a name it hides is read as the outer one. A write in a repeating `spawn` is caught only for a value
  declared before the `fork_join` block. These misses are accepted, never rejected.
- The partition body scan (3075, 3076) does not see a call that passes an outer local to a function that writes it (the write-borrow
  side, issues #204 and #210). A name declared in any block of the body is the body's own for the rest of the scan (no block-end
  tracking), so a write after the block that declared it, to an outer name of the same spelling, is accepted. A loop variable or a
  `with` name that hides the source of a `partition mutable` makes later uses of that spelling accepted.
