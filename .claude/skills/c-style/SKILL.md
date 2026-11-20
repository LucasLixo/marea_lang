---
name: c-style
mode: agent
effort: medium
description: "Marea C conventions - C17 headers and sources only, the MAREA_ type and macro system, per-module Except enums with MAREA_CHECK guards, MAREA_API package gating, the boundary where a module stops being C and becomes Rust, and the one-line \"// TAG: description\" comment format (WARNING/WARN/ERROR/INFO/TODO/FIXME/BUG). Use when writing or reviewing any .h or .c file under lib/."
globs: lib/**/*.h, lib/**/*.c
---

# C style

## Context and Objective

Conventions for every `.h` and `.c` under `lib/`, derived from `docs/CODE_OF_CONDUCT_CPP.md`. Non-compliance is an error.


## Strict Requirements

- TDD is mandatory: write the failing test first ([cpp-testing](../cpp-testing/SKILL.md)).
- Every header symbol carries a Doxygen-style comment block in the header itself, kept in sync with the symbol's lifecycle - see "Documentation" below.
- Rust conventions live in [rust-style](../rust-style/SKILL.md); build and test tasks in [engine-build](../engine-build/SKILL.md).

## Language boundary

**Headers are C.** Every `.h` under `lib/include/marea/` and `lib/internal/marea/internal/` is C17, consumable from C, C++, and bindgen.

**`.c` is for simple code and SDL3 wrappers** - argument checks, a call into `SDL_*` or the C standard library, an error code back. Nothing that owns a non-trivial data structure.

**Complex code is Rust.** Owning state, containers, parsers, algorithms, anything with real invariants goes to `lib/rust/` (`rust-style`).

No new production `.cpp` or `.hpp` is written under `lib/`. The ones left under `lib/source/marea/scripting/register/` and `lib/internal/` are legacy being ported to Rust under issue #85. Tests under `tests/` are C++ (GoogleTest) and are allowed ([cpp-testing](../cpp-testing/SKILL.md)).

## Rust boundary

The header does not say which language implements it. A Rust implementation declares the same C ABI and exports the declared name:

```rust
#[export_name = "MAREA_DequeCreate"]
pub extern "C" fn marea_deque_create(out_deque: *mut *mut MareaDeque) -> MareaDequeExcept
```

The header then needs only an entry on the bindgen allowlist in `lib/rust/build/marea.rs` (the bindgen wrapper header is generated at build time from the public headers, never edited by hand), so the Rust side mirrors the same types. A module moving from `.c` to Rust deletes the `.c` and changes nothing in the `.h`.

## File layers

| Ext | Layer | Location | Top include |
|-----|-------|----------|-------------|
| `.h` | Public C API | `lib/include/marea/` | `marea/Marea.h` |
| `.h` | Internal C headers | `lib/internal/marea/internal/` | the exact headers it uses |
| `.c` | C implementation | `lib/source/marea/` | the exact headers it uses |

- Never expose internal paths publicly.
- After adding a `.h` under `lib/include/`, add its `#include` to `lib/include/marea/Marea.h`.
- A `.c` (and an internal `.h`) includes exactly the headers whose symbols it uses - one `#include` per header that declares something the file references, and nothing it does not use. There is no internal umbrella header: never include a header just to reach another one transitively, and never re-create an all-in-one internal header. `SDL_*` symbols come from `marea/internal/system/MAREASdl.h`. `mise run dev:check-includes` (`lua tools/marea-api-catalog/main.lua check`) reports missing and unused includes and runs on every pull request; keep it at zero. The one exception is `PrecompileHeader.h`, which uses no symbol and lists its headers with angle brackets so the check (quoted includes only) skips them.
- The public `marea/Marea.h` stays the umbrella for consumers (apps, tests, examples), never for engine sources.

## Include guards

`MAREA_INCLUDE_<FOLDER>_<FILE>_H` for `lib/include`, `MAREA_INTERNAL_<FOLDER>_<FILE>_H` for `lib/internal`. The `marea` path component is dropped: `base/MAREAMemory.h` -> `MAREA_INCLUDE_BASE_MAREAMEMORY_H`. Use `#if !defined(...)` / `#define` / `#endif`, never `#pragma once`, and carry the name on the `#endif`.

## Include order

1. `SettingsHeader.h` - CMake force-includes it; never add it manually.
2. Top engine header.
3. Internal engine headers.
4. Third-party headers, wrapped in `BEGIN_IGNORE_PRAGMA_PACK` / `END_IGNORE_PRAGMA` when needed.
5. Standard library headers, same guard when needed.

## Standards and types

C17, no extensions. Brace-init (`T x{ v }`), value-init (`T{}`), and struct member default initializers are C++ and forbidden. Use `=` for scalars, designated initializers `= { .field = v }` for aggregates, and compound literals `(T){ ... }` - `*ptr = (T){ 0 };` zeroes an object.

| Alias | Underlying |
|-------|-----------|
| `MAREA_Boolean` | `bool` |
| `MAREA_Sinteger8/16/32/64` - `MAREA_Uinteger8/16/32/64` | `int*_t` - `uint*_t` |
| `MAREA_SintegerLeast8...Fast64` - `MAREA_UintegerLeast8...Fast64` | `int_least*/fast*_t` - `uint_least*/fast*_t` |
| `MAREA_IntegerSize` | `size_t` |
| `MAREA_Byte/Word/DoubleWord/QuadWord` | `uint8/16/32/64_t` |
| `MAREA_Float32/64/80/Size` | `float` / `double` / `long double` / platform |
| `MAREA_Null` | `void *` |
| `MAREA_Character8/Wide/16/32` | `char` / `wchar_t` / `uint16_t` / `uint32_t` |

- **Literals:** `MAREA_SINTEGER_8(v)`, `MAREA_UINTEGER_32(v)`, `MAREA_FLOAT_32/64/80/SIZE(v)`, `MAREA_HEXADECIMAL(FF)`, `MAREA_BINARY(1010)`, `MAREA_BYTE(FF)`, `MAREA_INTEGER_SIZE(v)`, `MAREA_CHARACTER_8/WIDE/16/32(text)`.
- **Null:** `MAREA_NULL` (0), `MAREA_NULLPTR` (NULL), or typed `MAREA_Null`.
- **Booleans:** always `MAREA_BOOLEAN_TRUE` / `MAREA_BOOLEAN_FALSE`, never raw `true` / `false`.
- **Printf:** `MAREA_PRINT_DECIMAL_SINTEGER_<N>`, `MAREA_PRINT_UNSIGNED_UINTEGER_<N>`, `MAREA_PRINT_HEXADECIMAL[_UPPER]_[S|U]INTEGER_<N>` with `<N>` in {8,16,32,64}; `MAREA_PRINT_PREFIX_MAX` for size-based types. Never raw `PRId32`.

## Cross-platform macros (`marea/base/MAREAMacros.h`)

Never use a raw keyword or compiler built-in when an `MAREA_` macro exists.

- **Visibility:** `MAREA_DECL`, `MAREA_EXTERN[_C]`, `MAREA_EXTERN_DECL[_C]`, `MAREA_BEGIN_EXTERN_C`, `MAREA_END_EXTERN_C`, `MAREA_EXTERN_MAIN`.
- **Inline:** `MAREA_INLINE`, `MAREA_FORCE_INLINE`, `MAREA_SPECIALIZATION_INLINE`, `MAREA_FORCE_CONST`.
- **Compile-time:** `MAREA_CONSTEXPR`, `MAREA_NOEXCEPT`, `MAREA_NODISCARD`, `MAREA_STATIC_ASSERT`. The first two expand to nothing in C; they carry meaning only for the C++ consumers of a header.
- **Casts:** `MAREA_STATIC_CAST(t,v)`, `MAREA_REINTERPRET_CAST(t,v)`, `MAREA_CONST_CAST(t,v)`.
- **Alignment:** `MAREA_ALIGNED(x)`, `MAREA_ALIGNOF(x)`. **Packing:** `MAREA_PACKED_START ... MAREA_PACKED_END;`, only when needed. **Aliasing:** `MAREA_RESTRICT`, see "restrict" below.
- **Warnings:** wrap system and third-party headers in `BEGIN_IGNORE_PRAGMA_PACK` / `END_IGNORE_PRAGMA` or `BEGIN_IGNORE_UNUSED_AND_INIT_WARNINGS` / `END_IGNORE_PRAGMA`.
- **Misc:** `MAREA_CALL`, `MAREA_IGNORE(expr)`, `MAREA_ARRAY_SIZE(arr)`, `MAREA_STRINGIFY(x)`, `MAREA_CONCATENATE(x,y)`. `MAREA_SHORT_LIFE_BLOCK` lives in `marea/base/MAREACTypes.h`.

## Mandatory macros on declarations

| Context | Required |
|---------|----------|
| Non-inline function | `MAREA_EXTERN_DECL` + `MAREA_CALL` |
| Function with a return value | + `MAREA_NODISCARD` |
| Inline function | `MAREA_INLINE` / `MAREA_FORCE_INLINE` / `MAREA_SPECIALIZATION_INLINE` + `MAREA_CALL` |
| Exported symbol | `MAREA_DECL`, through `MAREA_EXTERN_DECL[_C]` |

`MAREA_NODISCARD` goes on value-returning **functions**, never on a `struct` or `enum` type - in C17 it expands to a function attribute and is invalid on a type.

```c
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Sinteger32 MAREA_CALL MAREA_MyFunction(MAREA_Sinteger32 a);
MAREA_NODISCARD MAREA_FORCE_INLINE MAREA_Sinteger32 MAREA_CALL MAREA_MyInline(MAREA_Sinteger32 a);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MyVoid(void);
```

## Types and enums

Every `enum`, `struct`, and `union` is wrapped in a `typedef`. Headers carry plain C structs: no access labels, no `final`, no member default initializers - initialize at the use site.

**Opaque pointer wrappers stay opaque.** Do not publish a struct whose only payload is
`void *data` (including the `MAREA_Null data` spelling). It exposes no usable value
semantics and leaks an implementation detail. Publish `typedef struct MAREA_Name MAREA_Name;`
instead, keep the body in an internal header or Rust, and expose the required lifecycle
and operation functions through the C API.
`mise run dev:check-opaque-handles` machine-checks this rule across public headers.

Enums are plain C: no fixed underlying type (`: MAREA_Uinteger8` is unavailable in C17), so the enum is `int`-sized. C has no enum scope, so values are `UPPER_CASE` prefixed with the type name - `MAREA_ComponentExcept` -> `MAREA_COMPONENT_EXCEPT_SUCCESS`.

```c
typedef enum MAREA_ComponentExcept
{
    MAREA_COMPONENT_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
    MAREA_COMPONENT_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
} MAREA_ComponentExcept;
```

## Struct layout

Every `struct` in a header has a layout the compiler never decides on its own. It is the same on x86_64, x86 (i686), armv7 and aarch64:

1. **Fields in descending size.** 64-bit scalars and pointers first, then 4-, 2- and 1-byte fields. Arrays and nested structs go by their alignment, not their size. Fields of the same size keep their logical order. Put a 64-bit scalar before a pointer: a pointer is 4 bytes on 32-bit, so only the 64-bit scalar keeps its 8-byte alignment with no hole on both widths.
2. **No implicit padding.** A hole in the middle cannot happen once the fields are ordered, so only the tail needs padding, and it is **one array field** named `padding`: `MAREA_Uinteger8 padding[3];` (`MAREA_Float32 padding[2];` inside a float vector). Never `paddingZ`/`paddingW`, never a scalar `padding`, never several one-slot fields. A struct with several separate holes names them by position, `padding0`, `padding1`.
3. **Pointer width.** A pointer, a function pointer or an `MAREA_IntegerSize` is 8 bytes on 64-bit and 4 on 32-bit, so the tail of a struct that holds one can differ per width. Declare the array with `MAREA_PADDING(bytes64, bytes32)`, or `MAREA_PADDING_64(bytes)` / `MAREA_PADDING_32(bytes)` when only one width needs it (all three in `marea/base/MAREACTypes.h`; each expands to the single `padding` field, or to nothing).
4. **Explicit alignment.** `typedef struct MAREA_ALIGNED(N) MAREA_NODISCARD Name`, N the largest member alignment: 16 for SIMD-loaded types and any struct with a 16-byte member, 8 for a struct with a 64-bit scalar or a pointer (an `i386` `double` is 4-aligned and a 32-bit pointer 4-aligned, so write the 8 explicitly), 4 for plain 32-bit scalars, 2 or 1 as the fields say. Never rely on the compiler's implicit alignment, and never add 16 for style alone.
5. **Verified.** `tests/source/<module>/<MAREA_Filename>.cpp` carries one `<Struct>Layout` test per struct, built on `MAREA_ExpectExplicitLayout` and `MAREA_LAYOUT_FIELD` (`tests/include/tests/MAREALayout.hpp`): it checks that each field starts where the previous one ends, that alignment never increases along the fields, that the fields (padding included) add up to `sizeof` and that `sizeof` is a multiple of the struct alignment. A size that does not depend on the pointer width may also carry an `MAREA_STATIC_ASSERT` on `sizeof`. The four layouts are surveyed with libclang for the four targets before a layout change merges.
6. **Opaque handles are exempt.** A struct that a public header only declares (`typedef struct MAREA_Name MAREA_Name;`) has no visible layout; its definition in an internal header or in Rust follows the same rule anyway.

```c
// Before: a hole after the bool, a hole at the tail, implicit alignment.
typedef struct MAREA_NODISCARD MAREA_WidgetButton
{
    MAREA_String *label;
    MAREA_Vec2F32 size;
    MAREA_Boolean sameLine;
    MAREA_Float32 sameLineOffset;
    MAREA_Float32 sameLineSpacing;
} MAREA_WidgetButton;

// After: descending alignment, explicit alignment, one explicit padding array.
typedef struct MAREA_ALIGNED(16) MAREA_NODISCARD MAREA_WidgetButton
{
    MAREA_Vec2F32 size;
    MAREA_String *label;
    MAREA_Float32 sameLineOffset;
    MAREA_Float32 sameLineSpacing;
    MAREA_Boolean sameLine;
    MAREA_PADDING(15, 3)
} MAREA_WidgetButton;
```

A struct is initialized with designated initializers (`{ .size = ..., .label = ... }`), never positionally: the field order is a size order, not an API order, and a positional initializer silently breaks when a field moves. The padding array needs no initializer.

## Function shape

A module declares one `MAREA_<Module>Except` enum; every fallible function returns it and hands results back through `out`-prefixed pointer parameters. The body opens with one `MAREA_CHECK_NULL` per pointer argument, blank line between each, then the primitive call, then success.

```c
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_MemoryExcept MAREA_CALL MAREA_MemoryAllocate(MAREA_IntegerSize size, MAREA_Null *outMemory)
{
    MAREA_CHECK_NULL(outMemory, MAREA_MEMORY_EXCEPT_ERROR_NULL_POINTER);

    *outMemory = SDL_malloc(size);

    MAREA_CHECK_ALLOC(*outMemory, MAREA_MEMORY_EXCEPT_ERROR_OUT_OF_MEMORY);

    return MAREA_MEMORY_EXCEPT_SUCCESS;
}
```

The `MAREA_CHECK_*` family - `MAREA_CHECK_NULL`, `MAREA_CHECK_NULL_VOID`, `MAREA_CHECK_ALLOC`, `MAREA_CHECK_STATE`, `MAREA_CHECK_RESULT`, `MAREA_CHECK_EMPTY` - lives in `marea/internal/base/MAREACheck.h`; each logs and returns the given result.

## restrict

`restrict` promises the compiler that, inside the function, the memory reached through that pointer is accessed through no other pointer. It buys fewer reloads and room to vectorize; breaking the promise is undefined behavior, so every use is paired with a debug check.

- **Spell it `MAREA_RESTRICT`, never `restrict` or `__restrict`.** The macro (`marea/base/MAREAMacros.h`) is `restrict` in C99 and later, `__restrict` on GCC, Clang and MSVC for C++ consumers of the headers, and nothing elsewhere. The qualifier goes after the star: `const void *MAREA_RESTRICT src`, `MAREA_Null MAREA_RESTRICT dst`, `MAREA_Document **MAREA_RESTRICT outDocument`.
- **Put it on the written pointer.** Every `out` parameter of a function with a second pointer is `MAREA_RESTRICT`, and so are the source and destination of a copy-like pair (`MAREA_MemoryCopy`, `MAREA_NetAddressFormat`, `MAREA_AabbFromPoints`). Two read-only inputs do not get it, because they may legitimately be the same buffer (`MAREA_MemoryCompare`'s `s1` and `s2`, the centers and radii of a sphere batch).
- **Never on move-like or in-place-capable APIs** (`MAREA_MemoryMove`, anything where destination may equal source), **never on an opaque handle paired with data it may own** (`MAREA_StringSet(string, value)`, the `MAREA_NetBuffer*` read and write functions), **never on callbacks and `userdata`**, and never where `out` may be the input variable (`MAREA_MemoryReallocate(p, n, &p)`). Grep the call sites in `lib/`, `tests/` and `example/` before adding it.
- **Assert it in debug builds.** Right after the null guards, a restrict function calls `MAREA_ASSERT_NO_ALIAS(a, b)` for two single objects or two base pointers, or `MAREA_ASSERT_NO_OVERLAP(dst, dstSize, src, srcSize)` for ranges (`marea/internal/base/MAREARestrict.h`). Distinct pointers are not enough for ranges: a copy whose source starts inside the destination violates the promise although the pointers differ. Both macros expand to `MAREA_DIAGNOSTICS_ASSERT` when `MAREA_BUILD_DEBUG` is set and to `((void)0)` otherwise; the pure predicate `MAREA_RangesOverlap` is overflow-safe and tested without aborting the process. A Rust body mirrors them with `marea_debug_assert_no_alias!` and `marea_debug_assert_no_overlap!` (`rust-style`).
- **Document it.** The function's header comment (see "Documentation" below) names which parameters must not alias in its `@param` lines, and a death test (`EXPECT_DEATH`, debug builds only) shows the assert firing.

The effect, from `clang -O2` on x86_64 with vectorization off so the load stays visible:

```c
int ex1(int *a, int *b) { *a = 10; *b = 20; return *a; }
//   movl $10, (%rdi) ; movl $20, (%rsi) ; movl (%rdi), %eax          - *a reloaded, b may alias it
int ex1(int *MAREA_RESTRICT a, int *MAREA_RESTRICT b) { *a = 10; *b = 20; return *a; }
//   movl $10, (%rdi) ; movl $20, (%rsi) ; movl $10, %eax             - reload gone, constant returned

void scale(float *a, const float *fator, long n) { for (long i = 0; i < n; i++) a[i] *= *fator; }
//   movss (%rsi), %xmm0 ; mulss (%rdi,%rcx,4), %xmm0 ; ...           - *fator loaded again on every iteration
void scale(float *MAREA_RESTRICT a, const float *MAREA_RESTRICT fator, long n) { ...same body... }
//   movss (%rsi), %xmm0 once before the loop, then mulss %xmm0, ...  - *fator hoisted out of the loop
```

## Expressions

Every sub-expression of an arithmetic (`+ - * / %`) or bitwise (`& | ^ ~ << >>`) operation is parenthesized, whatever the precedence.

```c
MAREA_Sinteger32 result = ((a + b) * (c - d));
MAREA_Uinteger32 flags = ((mask & value) | (other << MAREA_UINTEGER_32(2)));
```

## Macros

New macros take the `MAREA_` prefix and a guard:

```c
#if !defined(MAREA_MY_MACRO)
    #define MAREA_MY_MACRO ...
#endif
```

Headers have no namespaces - use `MAREA_BEGIN_EXTERN_C` / `MAREA_END_EXTERN_C`.

## Translation-unit-local symbols

Any symbol used only inside one `.c` gets internal linkage with `static` and a prefix marking it a translation-unit **G**lobal.

| Kind (file-local only) | Rule | Example |
|------------------------|------|---------|
| `const` variable, `struct`, `enum`, `typedef`, function | `static` + `MAREA_G_` + PascalCase | `static const MAREA_Sinteger32 MAREA_G_ExampleTable[]`, `static void MAREA_CALL MAREA_G_ExampleHelper(...)` |
| Mutable variable | `static` + `marea_g_` + camelBack | `static MAREA_AppContext *marea_g_currentContext` |

File-scope only - function-body locals, including function-local `static`, keep ordinary names. `MAREA_FORCE_INLINE` already expands to `static MAREA_INLINE`, so do not add a second `static`; just apply the `MAREA_G_` name.

## Platform guards

Always `MAREA_PLATFORM_*`. Raw platform defines (`_WIN32`, `__linux__`, `ANDROID`, `__EMSCRIPTEN__`) appear only in the platform-detection normalization code that derives the `MAREA_PLATFORM_*` macros; everywhere else they are forbidden.

```c
#if defined(MAREA_PLATFORM_WINDOWS)
#elif defined(MAREA_PLATFORM_LINUX)
#elif defined(MAREA_PLATFORM_ANDROID)
#elif defined(MAREA_PLATFORM_WEB)
#endif
```

## Settings flags

Tested with `#if defined(SETTING_...)`, emitted into the force-included `SettingsHeader.h`.

| Define | Purpose |
|--------|---------|
| `SETTING_ENABLE_ASSERT` / `SETTING_ENABLE_LOGGER` | Assertions / logger |
| `SETTING_PRECOMPILE_HEADER` | PCH active |
| `SETTING_USE_STRUCTS_PACKED` | `MAREA_PACKED_*` enabled |
| `SETTING_USE_SHORT_LIFE_BLOCK` | short-life-block pattern |
| `SETTING_BUILD_DEBUG` | Debug configure (`MAREA_BUILD_DEBUG` is its 0/1 form) |
| `SETTING_SIMD_ASM_DISABLE` | all inline SIMD (asm, intrinsics, vector extensions) off, scalar C bodies only |
| `SETTING_ENGINE_TITLE/LABEL/PACKAGE/VERSION*` | engine metadata |

## API packages

Each API is a package compiled out by `#if defined(MAREA_API_<PKG>)`. CMake owns the polarity: every package is ON by default, `-DMAREA_OPTION_API_<PKG>=OFF` compiles it out, and the define arrives through the force-included `SettingsHeader.h` - there is no header to include.

A single-file API is `<folder>.<file-without-MAREA>` -> `MAREA_API_<FOLDER>_<FILE>` (`base/MAREAStr.h` -> `MAREA_API_BASE_STR`). A cohesive multi-file folder is named by the folder (`input/` -> `MAREA_API_INPUT`). The core packages `base.macros`, `base.ctypes`, and `base.enum` are never gated.

Converting a package touches two places together:

1. **Public header** - wrap the declaration block in `#if defined(MAREA_API_<PKG>)`. A disabled package leaves its symbols undeclared, so using one is a compile error; never `#error` on include, since the umbrella includes every header.
2. **Implementation** - wrap the whole `.c` body in the same guard, so a disabled TU compiles to an empty object and no CMake edit is needed.

Then register it: add the package to `MAREA_API_PACKAGES` in `lib/include/marea/SettingsHeader.cmake`, add the `#cmakedefine MAREA_API_<PKG>` line and any dependency `#error` in `lib/include/marea/SettingsHeader.h.in`, and add its gated name entry to `lib/source/marea/MAREAApi.c` for runtime introspection.

Disabling a package (`MAREA_OPTION_API_<PKG>=OFF`) removes its exported symbols from the engine, Rust side included: the crate's module for the package is compiled out by the matching `api-<id>` feature, so no `#[export_name]` of it survives in the binary. The header's `MAREA_API_<PKG>` gate and the Rust feature must name the same package, and deleting a declaration from a header deletes its Rust definition in the same change set. `mise run test:exports-match` enforces both (see `rust-style`, "Feature gating").

## Thread primitives

New thread primitives belong in `marea/base/MAREAThread.h`, never as an ad-hoc primitive in a module. Choose the smallest family that matches the shared state:

| Shared state | Primitive | Decision rule |
|---|---|---|
| One word with no neighboring invariant | Atomic | Use an atomic operation. |
| Several fields that must change together | Mutex | Protect the invariant with a mutex. |
| Initialization that any thread may request | Once | Run the initialization through MAREA_ThreadOnce. |
| Per-thread scratch storage | Thread-local | Use MAREA_THREAD_LOCAL from `marea/base/MAREAMacros.h`. |

## Source independence

In an implementation, reach for the lowest available primitive, in order: **SDL3** (`SDL_*`), then the **C standard library**, then an **existing engine function**, and only then a **new engine function**.

**The one exception is numeric math in Rust.** In `lib/rust`, the SDL math wrappers (`sdl_sqrtf`, `sdl_sinf`, ... in `system/marea_sdl.rs`) are FFI calls that can never inline, so a hot path pays the call for a single instruction. The rule here is where a gain is measured, not a blanket preference for native: `sqrt` and `abs` (`f32` and `f64`) are always native Rust, because they are IEEE-exact and identical on every platform; of the transcendentals only `sinf`, `cosf` and `atan2f` are native, because they measured at least 1.1x faster (sinf 1.27x, cosf 1.42x, atan2f 1.11x) and are bit-identical to libm. `tanf`, `expf`, `logf`, `powf`, `asinf`, `acosf`, `atanf` and `atan2` on `f64` stay on SDL: they measured 1.10x or less, so a swap buys nothing. Any further swap needs both a measured gain of at least 1.1x on the benchmark (`mise run bench:math`) and a bit-comparison test against SDL (`marea_math_bits.rs`) that passes on every platform, because libm results can differ per platform; `mise run test:cross -- --target all --check libm --strict` runs that comparison on Linux x86_64 and aarch64, Android x86_64 (emulator) and aarch64, armv7 and i686 (user-mode qemu, static bionic), Emscripten under node (with and without simd128) and Windows MinGW under wine, and the CI workflow runs it for Windows MSVC and clang-cl. Nothing else changes: C sources keep calling `SDL_*` for math, every non-math SDL call stays an `SDL_*` call, and `SDL_min`, `SDL_max` and `SDL_clamp` stay ternaries.

Never call a sibling `MAREA_*` wrapper that merely forwards to a call you can make directly - write `SDL_free(p)`, not `MAREA_MemoryFree(p)`. The public `MAREA_*` wrappers are the API for consumers; engine internals bottom out at the primitive.

## Documentation

A symbol's documentation lives in the header, next to the declaration it documents - never in a separate file or a generated tree. Every `struct`, `enum`, `typedef`, and function declared in a public or internal header carries a Doxygen-style `/** ... */` block directly above it:

```c
/**
 * @brief Allocates a block of memory of the given size.
 * @param size Number of bytes to allocate.
 * @param outMemory Receives the allocated pointer. Must not alias another output of this call.
 * @return MAREA_MEMORY_EXCEPT_SUCCESS, or MAREA_MEMORY_EXCEPT_ERROR_OUT_OF_MEMORY on allocation failure.
 */
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_MemoryExcept MAREA_CALL MAREA_MemoryAllocate(MAREA_IntegerSize size, MAREA_Null *outMemory);
```

- `@brief` - one line, what the symbol does.
- `@param` - one per parameter, in declaration order; states which pointer parameters must not alias (see "restrict" above) and which may be null.
- `@return` - the success value and every error value the function can return, with the condition that produces each one.
- The block is added with the symbol, updated in the same change that changes the symbol's contract, and deleted with it. It is the only copy of that documentation - nothing downstream regenerates or mirrors it.

## Comments

**Every non-doc comment is exactly one line, shaped `// TAG: description.`** - never a `/* ... */` block (the single-line `/* name */` carried on an `#endif` is the one exception). If an explanation does not fit on one line, that is a signal to move it into the symbol's Doxygen comment block (see "Documentation" above), express it as a `MAREA_CHECK_*` guard or an `MAREA_` macro instead, or split the code - not to wrap the comment.

The tag vocabulary (same as `rust-style`, minus `SAFETY`, which is Rust-only):

| Tag | Use for |
|---|---|
| `WARNING` / `WARN` | A non-obvious hazard: a trap a future edit could walk into without seeing it coming. `WARN` is the accepted shorthand of `WARNING`. |
| `ERROR` | Why a specific error path returns what it returns, when that is not obvious from the guard alone. |
| `INFO` | Rationale, cross-reference, or context that does not fit `WARNING`/`ERROR` - the general "why", never the "what". |
| `TODO` | Deferred work, always with an issue number: `// TODO(#123): description.` A TODO with no issue has no owner and does not get committed. |
| `FIXME` | Known defect in this code - incorrect or broken-by-design behavior that must be corrected, always with an issue number: `// FIXME(#123): description.` |
| `BUG` | A workaround for a bug in code this repository does not own (a vendored library, a platform or driver API): names the upstream defect and the condition under which the workaround can be removed. |

A comment explains *why*, never *what* - `index += 1` needs no comment; skipping the increment on a specific error path to keep a caller-visible prefix unchanged does. Prefer enforcing a requirement in a `MAREA_CHECK_*` guard over describing it in a comment; a comment is not a proof.

This applies to new and touched comments; it is not a mandate to rewrite untouched comment blocks elsewhere on sight.

## Formatting (clang-format)

clang-format is the formatter and wins on whitespace and layout: [`.clang-format`](../../../.clang-format) decides indent, column limit, brace placement, pointer binding, include order and preprocessor indentation, and this skill restates none of it. Two consequences worth knowing: includes are never sorted, and `*BEGIN*` / `*END*` macros delimit blocks and indent their body, which is why `MAREA_BEGIN_EXTERN_C` bodies are indented.

After editing, run `mise run format:cpp-sources` (all tracked C/C++); `mise run format:check-sources` is the read-only check across every language. As an explicit exception to the mise-only rule, a direct, scoped `clang-format -style=file -i <path>` on the files you touched is allowed.

## Validation

- `mise run dev:check-includes` and `mise run dev:check-opaque-handles` report zero.
- `mise run test:exports-match` passes when a header declaration or `MAREA_API_<PKG>` gate changed.
- Formatting applied as above; `Marea-Tests` stays green.
