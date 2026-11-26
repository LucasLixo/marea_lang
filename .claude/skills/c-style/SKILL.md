---
name: c-style
mode: agent
effort: medium
description: "Marea C11 conventions - C11 headers and sources only, the MAREA_ type and macro system from lib/include/marea, opaque structs with accessor functions, per-module Except enums, the rules that bind a compiler stage (no global state, no printing, caller-owned arena), expression parenthesization, restrict, and the one-line \"// TAG: description\" comment format (WARNING/WARN/ERROR/INFO/TODO/FIXME/BUG). Use when writing or reviewing any .h or .c file under lib/."
globs: lib/**/*.h, lib/**/*.c
---

# C style

## Context and Objective

Conventions for every `.h` and `.c` under `lib/`. The policy is in [AGENTS.md](../../../AGENTS.md) and wins over this skill; the design record behind it is `temp/OBJETIVO.md` (objective 37, decision 38). Non-compliance is an error.

A rule marked "not in this repo yet" names a file, macro or task that does not exist today. Do not use it and do not invent the file; when the file appears, the rule becomes active and the mark is removed.

## Strict Requirements

- TDD is mandatory: write the failing test first ([cpp-testing](../cpp-testing/SKILL.md)).
- The copyright header, the ASCII-only rule and the ASD-STE100 comment standard are owned by [AGENTS.md](../../../AGENTS.md); this skill does not repeat them.
- Rust conventions live in [rust-style](../rust-style/SKILL.md). There is no `lib/rust` crate in this repo yet.

## Language and stage rules

- **C11 only, no extensions.** CMake sets `CMAKE_C_STANDARD 11` with extensions off. A header must also compile as C++, because the tests are C++ (GoogleTest) and include it. Production code is never C++.
- **One library per stage** (`base`, `lex`, later `parse`, `sema`, `ir`, `bytecode`, `vm`, `cgen`). A stage never includes the header of a later stage.
- **No global state.** A stage keeps nothing in a mutable file-scope variable. Everything it needs comes in an explicit context: an arena that the caller owns and a diagnostic sink.
- **A stage never prints.** No `printf`, `fprintf` or `puts` under `lib/`. A stage returns diagnostics and the caller decides.
- **Results live in the caller's arena.** The caller frees everything by destroying the arena. A function that allocates takes the arena as a parameter and never frees what it hands out.
- **Opaque by default.** A public header declares `typedef struct MAREA_Name MAREA_Name;` and exposes accessor functions. A struct is opened to the public only where it is needed, and the doc comment of that struct records the reason (see `MAREA_SourcePosition` in `lib/include/marea/base/MAREASourcePosition.h`).
- **Static libraries by default.** A shared build is an explicit CMake option; the export and import macros `MAREA_ENABLE_EXPORT` and `MAREA_ENABLE_IMPORT` (see `MAREA_DECL`) are for that case.

Not in this repo yet: the registration macro that exposes a function to the language runtime and to Rust, and any Rust boundary rule.

## File layers

| Ext | Layer | Location | Includes |
|-----|-------|----------|----------|
| `.h` | Public C API | `lib/include/marea/` (flat core headers) and `lib/include/marea/<stage>/` | `marea/MAREACTypes.h`, `marea/MAREAMacros.h`, then the exact headers it uses |
| `.h` | Internal headers | `lib/internal/marea/internal/` (empty today) | the exact headers it uses |
| `.c` | C implementation | `lib/source/marea/` and `lib/source/marea/<stage>/` | its own header first, then the exact headers it uses |

- Never expose an internal path in a public header.
- A `.c` or `.h` includes exactly the headers whose symbols it uses and nothing else. Never include a header only to reach another one transitively. There is no umbrella header.
- Include order: engine headers (core headers first, then the other `marea/...` headers), then standard library headers. System headers that need warning control go between `MAREA_BEGIN_IGNORE_PRAGMA_PACK` and `MAREA_END_IGNORE_PRAGMA`.
- Not in this repo yet: a force-included settings header, a `marea/Marea.h` umbrella and an automatic include checker.

## Include guards

`MAREA_INCLUDE_<FOLDER>_<FILE>_H` for `lib/include`, `MAREA_INTERNAL_<FOLDER>_<FILE>_H` for `lib/internal`. The `marea` path component is dropped and the file name keeps its `MAREA` prefix, upper-cased: `marea/base/MAREAArena.h` -> `MAREA_INCLUDE_BASE_MAREAARENA_H`, `marea/MAREAMacros.h` -> `MAREA_INCLUDE_MAREAMACROS_H`. Use `#if !defined(...)` / `#define` / `#endif /* NAME */`, never `#pragma once`. The copyright header comes first, above the guard.

## Types and literals

Never use a plain C type name (`int`, `long`, `char`, `float`, `double`, `bool`, `size_t`) in a declaration when an alias exists in [`MAREACTypes.h`](../../../lib/include/marea/MAREACTypes.h). That file is the one list of aliases; this skill does not copy it. The families are: `MAREA_Boolean`, `MAREA_Sinteger<N>` and `MAREA_Uinteger<N>` (also `Least` and `Fast`), `MAREA_Byte`, `MAREA_Word`, `MAREA_DoubleWord`, `MAREA_IntegerSize`, `MAREA_Float32/64/80`, `MAREA_FloatSize`, `MAREA_Null` and `MAREA_Character8/Wide/16/32`.

- **Literals:** `MAREA_SINTEGER_<N>(v)`, `MAREA_UINTEGER_<N>(v)`, `MAREA_FLOAT_<N>(v)`, `MAREA_INTEGER_SIZE(v)`, `MAREA_HEXADECIMAL(FF)`, `MAREA_BINARY(1010)`, `MAREA_BYTE(FF)`, `MAREA_CHARACTER_8/WIDE/16/32(text)`.
- **Limits:** `MAREA_SINTEGER_<N>_MAXIMUM` / `_MINIMUM`, and the same for the other families.
- **Null:** `MAREA_NULLPTR` for a null pointer value. A pointer parameter that receives an untyped block is `MAREA_Null`.
- **Booleans:** `MAREA_BOOLEAN_TRUE` / `MAREA_BOOLEAN_FALSE`, never raw `true` / `false`.
- **Printf-style formats:** `MAREA_PRINT_*` macros (for example `MAREA_PRINT_DECIMAL_SINTEGER_32`), never raw `PRId32`. Library code does not print, so this applies to tests and tools only.
- **Initializers:** C11 only. Use `=` for scalars and designated initializers `{ .field = v }` for aggregates. A compound literal `(T){ 0 }` zeroes an object. Brace-init `T x{ v }`, value-init `T{}` and member default initializers are C++ and forbidden. Never initialize a struct positionally: the field order is a size order, not an API order.

## Macros that exist (`marea/MAREAMacros.h`)

Never use a raw keyword or compiler built-in when a `MAREA_` macro for it exists.

- **Linkage and export:** `MAREA_DECL`, `MAREA_EXTERN`, `MAREA_EXTERN_C`, `MAREA_EXTERN_DECL`, `MAREA_EXTERN_DECL_C`, `MAREA_BEGIN_EXTERN_C`, `MAREA_END_EXTERN_C`.
- **Inline:** `MAREA_INLINE`, `MAREA_FORCE_INLINE` (already `static`), `MAREA_INLINE_EXPORTED`, `MAREA_FORCE_CONST`.
- **Attributes:** `MAREA_NODISCARD`, `MAREA_UNUSED`, `MAREA_DEPRECATED_ATTRIBUTE`, `MAREA_CALL`.
- **Checks:** `MAREA_STATIC_ASSERT(cond, msg)`.
- **Alignment and packing:** `MAREA_ALIGNED(x)`, `MAREA_ALIGNOF(x)`, `MAREA_PACKED_START` / `MAREA_PACKED_END`, only when needed.
- **Aliasing:** `MAREA_RESTRICT`, see "restrict" below.
- **Thread-local:** `MAREA_THREAD_LOCAL`. A stage has no global state, so this is for tests and tools.
- **Warnings:** `MAREA_BEGIN_IGNORE_PRAGMA_PACK`, `MAREA_BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS`, closed by `MAREA_END_IGNORE_PRAGMA`.
- **Misc:** `MAREA_IGNORE(expr)`, `MAREA_ARRAY_SIZE(arr)`, `MAREA_STRINGIFY(x)`, `MAREA_CONCATENATE(x,y)`, `MAREA_BUILD_DEBUG` (0 or 1). `MAREA_SHORT_LIFE_BLOCK` is in `marea/MAREACTypes.h`.

Not in this repo yet: cast macros (use a plain C cast), `MAREA_CONSTEXPR`, `MAREA_NOEXCEPT`, `MAREA_SPECIALIZATION_INLINE`, `MAREA_EXTERN_MAIN`, platform macros `MAREA_PLATFORM_*`, and every `SETTING_*` source: the headers test `SETTING_BUILD_DEBUG`, `SETTING_USE_STRUCTS_PACKED` and `SETTING_USE_SHORT_LIFE_BLOCK`, but nothing defines them yet, so the default is off. Raw platform defines (`_WIN32`, `__linux__`) stay inside `MAREAMacros.h` until `MAREA_PLATFORM_*` exists.

## Mandatory macros on declarations

| Context | Required |
|---------|----------|
| Non-inline function | `MAREA_EXTERN_DECL` + `MAREA_CALL` |
| Function with a return value | + `MAREA_NODISCARD` in front |
| Inline function | `MAREA_FORCE_INLINE` (or `MAREA_INLINE`) + `MAREA_CALL` |

`MAREA_NODISCARD` goes on value-returning **functions**, never on a `struct` or `enum` type: it expands to a function attribute and is invalid on a type. A `void` function does not get it. An accessor that returns a value takes it too.

```c
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_ArenaExcept MAREA_CALL MAREA_ArenaCreate(MAREA_IntegerSize chunkSize, MAREA_Arena **outArena);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_ArenaDestroy(MAREA_Arena *arena);
```

Public headers wrap their declarations in `MAREA_BEGIN_EXTERN_C` / `MAREA_END_EXTERN_C`. Headers have no namespaces, so every public name carries the `MAREA_` prefix.

## Types and enums

Every `enum`, `struct` and `union` is wrapped in a `typedef`. Headers carry plain C structs: no access labels, no `final`.

An opaque type is `typedef struct MAREA_Name MAREA_Name;` in the public header, with the body in a `.c` file or an internal header, and the lifecycle and accessor functions in the public header. Do not publish a struct whose only payload is `void *data`.

An enum is plain C: no fixed underlying type. C has no enum scope, so a value is `UPPER_CASE` prefixed with the type name: `MAREA_ArenaExcept` -> `MAREA_ARENA_EXCEPT_SUCCESS`. Values use `MAREA_UINTEGER_8(n)`.

## Function shape

A module declares one `MAREA_<Module>Except` enum with `SUCCESS` = 0 first, then `ERROR_NULL_POINTER`, `ERROR_OUT_OF_MEMORY` and the other errors the module needs (see `MAREA_ArenaExcept` and `MAREA_DiagnosticExcept`). Every fallible function returns it and hands results back through `out`-prefixed pointer parameters.

- The body opens with one null check per pointer argument, a blank line between each, then the work, then `return ..._EXCEPT_SUCCESS`.
- Use a plain `if` that returns the Except value. The `MAREA_CHECK_*` guard macros are not in this repo yet; when a shared guard header is added, it replaces the plain `if` and this rule changes with it.
- An accessor on an opaque type accepts null and returns a documented neutral value (0, null or a zeroed struct), as the declarations in `lib/include/marea/base/MAREADiagnostic.h` do.
- A function that allocates takes the arena from the caller. No function allocates with `malloc` on behalf of the caller.

```c
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_ArenaExcept MAREA_CALL MAREA_ArenaAllocate(MAREA_Arena *arena, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null *outMemory)
{
    if (arena == MAREA_NULLPTR)
    {
        return MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outMemory == MAREA_NULLPTR)
    {
        return MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER;
    }

    // ... allocate from the arena ...

    return MAREA_ARENA_EXCEPT_SUCCESS;
}
```

## Struct layout

Every struct a public header opens has a layout that the compiler does not choose alone. It must be the same on 32-bit and 64-bit targets.

1. **Fields in descending size.** 64-bit scalars first, then 4-, 2- and 1-byte fields. A pointer is 4 bytes on 32-bit, so put a 64-bit scalar before a pointer. Fields of the same size keep their logical order.
2. **No implicit padding.** Only the tail needs padding, and it is one array field named `padding`: `MAREA_Uinteger8 padding[3];`. Never several one-slot padding fields.
3. **Pointer width.** When the tail depends on the pointer width, declare it with `MAREA_PADDING(bytes64, bytes32)`, `MAREA_PADDING_64(bytes)` or `MAREA_PADDING_32(bytes)` (in `marea/MAREACTypes.h`).
4. **Explicit alignment.** `typedef struct MAREA_ALIGNED(N) Name`, N the largest member alignment (8 for a struct with a pointer or a 64-bit scalar). Never rely on implicit alignment, and never add a larger N for style alone. `MAREA_SourcePosition` is the model.
5. **Verified.** Each opened struct has a test that checks `sizeof` and the `offsetof` of every field with plain GoogleTest assertions. A layout helper (`MAREA_ExpectExplicitLayout`, `MAREALayout.hpp`) is not in this repo yet.
6. **Opaque types are exempt** (no visible layout), but their body follows the same rules.

## restrict

`restrict` promises that, inside the function, the memory reached through that pointer is accessed through no other pointer. Breaking the promise is undefined behavior.

- Spell it `MAREA_RESTRICT`, never `restrict` or `__restrict`. The qualifier goes after the star: `const void *MAREA_RESTRICT src`.
- Put it on the written pointer when a function has a second pointer, and on both sides of a copy-like pair (source and destination). Two read-only inputs do not get it, because they may be the same buffer.
- Never use it on a move-like or in-place API (destination may equal source), on a callback or user data pointer, on an opaque handle paired with data it may own, or where `out` may be the input variable.
- The doc comment of the function names which parameters must not alias.
- Not in this repo yet: the debug alias asserts (`MAREA_ASSERT_NO_ALIAS`, `MAREA_ASSERT_NO_OVERLAP`) and the overlap predicate. Until they exist, a `MAREA_RESTRICT` function gets a test that passes distinct buffers and states the rule in its comment.

## Expressions

Every sub-expression of an arithmetic (`+ - * / %`) or bitwise (`& | ^ ~ << >>`) operation is parenthesized, whatever the precedence.

```c
MAREA_Sinteger32 result = ((a + b) * (c - d));
MAREA_Uinteger32 flags = ((mask & value) | (other << MAREA_UINTEGER_32(2)));
```

## Macros

A new macro takes the `MAREA_` prefix and a guard:

```c
#if !defined(MAREA_MY_MACRO)
    #define MAREA_MY_MACRO ...
#endif
```

## Translation-unit-local symbols

A symbol used only inside one `.c` file gets internal linkage with `static` and a `G` prefix. No `.c` file in the repo uses this yet, so the rule is a convention to apply from the first one.

| Kind (file-local only) | Rule | Example |
|------------------------|------|---------|
| `const` variable, function, `struct`, `enum`, `typedef` | `static` + `MAREA_G_` + PascalCase | `static void MAREA_CALL MAREA_G_ExampleHelper(...)` |
| Mutable variable | Not allowed in a stage (no global state). In a test helper or tool: `static` + `marea_g_` + camelBack | `static MAREA_Sinteger32 marea_g_counter` |

Function-body locals keep ordinary names. `MAREA_FORCE_INLINE` is already `static`, so add no second `static`.

## Documentation

Every `struct`, `enum`, `typedef` and function declared in a header carries a Doxygen-style `/** ... */` block directly above it, as the headers in `lib/include/marea/base/` do. This is a repo convention; no documentation generator task exists in this repo yet, so nothing is generated from it.

- `@brief` - what the symbol does.
- `@param` - one per parameter in declaration order; say which pointers must not be null and which must not alias.
- `@return` - the success value and every error value, with the condition that gives each one.
- The block is added with the symbol, changed in the same change as the contract, and removed with it.

## Comments

**Every non-doc comment is exactly one line, shaped `// TAG: description.`** Never a `/* ... */` block; the `/* NAME */` after an `#endif` is the one exception. If the text does not fit on one line, move it to the Doxygen block, express it as code, or split the code. The wording follows ASD-STE100 (see [AGENTS.md](../../../AGENTS.md)).

| Tag | Use for |
|---|---|
| `WARNING` / `WARN` | A non-obvious hazard that a later edit could trigger. `WARN` is the short form. |
| `ERROR` | Why an error path returns what it returns, when the guard does not show it. |
| `INFO` | Rationale or context: the "why", never the "what". |
| `TODO` | Deferred work with an issue number: `// TODO(#123): description.` A TODO with no issue is not committed. |
| `FIXME` | A known defect in this code, with an issue number: `// FIXME(#123): description.` |
| `BUG` | A workaround for a defect in code this repo does not own: name the defect and the condition to remove the workaround. |

A comment explains why, never what. Prefer enforcing a requirement in code over describing it. This applies to new and touched comments; do not rewrite untouched comments on sight.

## Formatting (clang-format)

clang-format is the formatter and wins on whitespace and layout: [`.clang-format`](../../../.clang-format) decides indent, brace placement, pointer binding, include order and preprocessor indentation, and this skill restates none of it. Includes are never sorted. After editing, run `mise run format:cpp-sources` (all tracked C and C++ files); `mise run format:check-sources` is the read-only check. A direct, scoped `clang-format -style=file -i <path>` on the files you touched is also allowed.

## Validation

- `mise run format:check-sources` reports nothing.
- `mise run dev:ascii-check` passes. Untracked new files are not covered by it: run `lua tools/marea-ascii-check/main.lua <file>...` on them.
- The library and the tests build and pass: `mise run dev:quiet-test` ([cpp-testing](../cpp-testing/SKILL.md), "Running tests").
- `grep -R "printf\|fprintf\|puts" lib` prints nothing.
