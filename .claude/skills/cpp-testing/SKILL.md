---
name: cpp-testing
mode: agent
effort: medium
description: Marea Lang GoogleTest conventions - tests live in tests/source/ and mirror the public header path (tests/source/marea/MAREAVersion.cpp), start with "tests/dependencies.hpp", and use the GTEST_COUT macro it defines. Tests prefer the public accessor API of opaque structs and may also use the internal API (internal headers under lib/internal/) when the public API cannot reach the behavior or a defect. TDD is mandatory, so no symbol ships without a test. Use when writing or changing any test under tests/, or before implementing a C/C++ symbol.
globs: tests/**/*.cpp, tests/**/*.hpp
argument-hint: <module or symbol under test>
---

# CPP testing

## Context and Objective

GoogleTest suites for Marea Lang live under `tests/`. TDD is mandatory: a failing test comes first, then the
minimum implementation, then refactor. No symbol ships without a test.

New production `.cpp`/`.hpp` are never written (see [c-style](../c-style/SKILL.md)); GoogleTest sources under
`tests/` are the one permitted exception, because GoogleTest is a C++ framework and production code stays C11
(or Rust where a binding needs it).

## Strict Requirements

### File structure

Every new test file opens with the two-line copyright header of [AGENTS.md](../../../AGENTS.md), then this include first:

```cpp
// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

TEST(SuiteName, TestName) { ... }
```

Add another direct include only when the symbol under test is unavailable through `tests/dependencies.hpp`. Today that header includes `<gtest/gtest.h>` and `marea/MAREAVersion.h`, so a test of any other public header adds `#include "marea/<path>/<Header>.h"` on the next line. Add to `tests/dependencies.hpp` only what every test needs.

`GTEST_COUT` is defined in `tests/dependencies.hpp` - never define or redefine it in a test.

- **Source:** `tests/source/<path>/<HeaderName>.cpp`, where `<path>/<HeaderName>.h` is the public header under `lib/include/` that declares the symbol (`lib/include/marea/MAREAVersion.h` -> `tests/source/marea/MAREAVersion.cpp`, `lib/include/marea/base/MAREADiagnostic.h` -> `tests/source/marea/base/MAREADiagnostic.cpp`). **Support headers:** `tests/include/tests/`.
- `tests/CMakeLists.txt` globs every `.cpp` under `tests/source/` (`CONFIGURE_DEPENDS`) into the target `marea_tests`, which links `marea` and `GTest::gtest_main`. Each `TEST(...)` is registered as a `ctest` test by `gtest_discover_tests()` - no CMake edit is needed for a new file or test.
- The suite is built only when `MAREA_OPTION_TARGET_TESTS` is ON (the default). GoogleTest is the submodule `3rdparty/GoogleTest`; when it is missing, run `mise run setup:git-modules`.

### Tests and opaque structs

[AGENTS.md](../../../AGENTS.md) says a public header holds opaque structs with accessor functions. A test is first a consumer of the public API, so the public API is the default:

- Prefer the public functions (`MAREA_DiagnosticSinkCreate`, `MAREA_DiagnosticSinkGetCount`, ...) to create, read and change an opaque struct. Use them whenever they can reach the behavior under test.
- **Internal API allowed.** A test may include an internal header (`lib/internal/`) and call internal functions where the public API cannot reach the behavior or a defect, for example an arithmetic overflow on a size that no public caller can make large, or a helper with no public entry point. `marea_tests` has the internal include directory for this. A test that does so adds one line `//` after the copyright header that says why the public API is not enough (ASCII). Never cast to a private layout to read a field when an internal accessor exists.
  Decision of the user (issue #28, widened afterwards): see `tests/source/sema/MAREASemaInternal.cpp`.
- Every accessor is tested through the observable result of the call, so a test also proves the accessor is enough for a real consumer. If a test cannot reach a value without opening the struct, the API is missing an accessor: add it test-first, or open the struct only with the reason recorded, as AGENTS.md requires.
- Test the error path of each function that returns an `Except` enum (a null pointer, out of range, out of memory) as well as the success path.
- A struct that is opened to the public (fields visible) may be checked field by field, only where its header says it is open.

### Running tests

There is no `test:*` mise task. Configure and build the test target, then run `ctest`:

```sh
mise run config:cmake
cmake --build cmake-build-debug --target marea_tests --parallel
ctest --test-dir cmake-build-debug --output-on-failure
```

`mise run build:marea` builds only the `marea` library, not `marea_tests`. Debug is the default configuration; for another one, use `mise run config:cmake -- --config Release` and the matching `cmake-build-release` directory. A filtered run is fine for debugging, for example `ctest --test-dir cmake-build-debug -R MAREAVersion`. See [cmake-style](../cmake-style/SKILL.md) for the CMake side.

### Naming `TEST(SuiteName, TestName)`

No fixture class by default.

- **SuiteName:** the source file name without `.cpp` (it is the header name, which starts with `MAREA`, not `MAREA_`).
- **TestName:** function, or `ClassName` + `FunctionName`, in CamelCase, `MAREA_` prefix stripped.

| File | Symbol | `TEST(...)` |
|------|--------|-------------|
| `tests/source/marea/MAREAVersion.cpp` | `MAREA_GetVersion()` | `TEST(MAREAVersion, GetVersion)` |
| `tests/source/marea/base/MAREADiagnostic.cpp` | `MAREA_DiagnosticSinkCreate()` | `TEST(MAREADiagnostic, DiagnosticSinkCreate)` |

### Assertions

- `ASSERT_*` = preconditions (stop on failure). `EXPECT_*` = feature checks (continue).
- Every assertion carries a `<<` message - never bare. One blank line after each assertion.

```cpp
ASSERT_NE(version, MAREA_NULLPTR) << "MAREA_GetVersion must never return null.";

EXPECT_STREQ(version, MAREA_VERSION) << "MAREA_GetVersion must return the MAREA_VERSION string.";
```

### No comments

No `//` or `/* */` anywhere, except the two-line copyright header at the top of the file (AGENTS.md requires it) and the one-line reason comment of a test that uses the internal API (see "Tests and opaque structs"). Use `GTEST_COUT` for output - at least one emission per `TEST` block:

```cpp
GTEST_COUT << "Validating MAREA_GetVersion returns a positive value." << std::endl;
```

### Unused return values

Wrap intentionally-ignored returns with `MAREA_IGNORE()`. Never call a `MAREA_NODISCARD` function without storing
or ignoring its result.

```cpp
MAREA_IGNORE(MAREA_DiagnosticSinkAdd(sink, code, severity, position, "message"));
```

### Naming conventions (clang-tidy, see `.clangd`)

| Context | Case | Prefix |
|---------|------|--------|
| `class` / `struct` / `enum` / `typedef` | CamelCase | `MAREA_` |
| Function / method (any visibility) | camelBack | `MAREA_` |
| Macro | UPPER_CASE | `MAREA_` |
| Namespace | CamelCase | - |
| Local variable / parameter / public struct member | camelBack | - |
| Class member (private/protected) | camelBack | `_` |
| Constant / `constexpr` / enum constant | UPPER_CASE | - |

## Workflow

1. Write the failing test in `tests/source/<path>/<HeaderName>.cpp`.
2. Build `marea_tests` and run it with `ctest`; confirm it fails for the right reason (a test that cannot compile because the symbol is missing counts as the first failure).
3. Implement the minimum, then refactor with the suite green.
4. Run `mise run format:check-sources` and `mise run dev:ascii-check`.

## Validation

- The new `TEST` is discovered and passes; the rest of `ctest` stays green.
- The test touches an opaque struct through its public functions, or states in one line why it uses the internal API.
- Every assertion has a `<<` message, every `TEST` emits `GTEST_COUT`, and the file has no comments besides the copyright header.
