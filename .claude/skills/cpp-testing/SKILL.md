---
name: cpp-testing
mode: agent
effort: medium
description: Marea Lang GoogleTest conventions - tests live in tests/source/<module>/<MAREA_Filename>.cpp, start with "tests/dependencies.hpp", and use the always-available GTEST_COUT macro. TDD is mandatory, so no symbol ships without a test. Use when writing or changing any test under tests/, or before implementing a C/C++ symbol.
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

Every test file begins with this include first:

```cpp
#include "tests/dependencies.hpp"

TEST(SuiteName, TestName) { ... }
```

Add another direct include only when the symbol under test is unavailable through `tests/dependencies.hpp`.

`GTEST_COUT` is always available - never define or redefine it.

- **Source:** `tests/source/<module>/<MAREA_Filename>.cpp` - **Headers:** `tests/include/tests/`.
- CMake auto-globs all `.cpp` under `tests/source/` as target sources; each `TEST(...)` inside
  them is separately registered as a `ctest` test by `gtest_discover_tests()` after a build - no
  CMake edit needed for either.

### Running tests

Run the project's test task once the build is set up (see [cmake-style](../cmake-style/SKILL.md) for the CMake
side); a filtered `ctest` invocation run directly is fine for debugging one test or suite.

### Naming `TEST(SuiteName, TestName)`

No fixture class by default.

- **SuiteName:** source file name in CamelCase, prefixed `MAREA` (not `MAREA_`). Always starts with `MAREA` -
  mandatory.
- **TestName:** function, or `ClassName` + `FunctionName`, in CamelCase, `MAREA_` prefix stripped.

| File | Symbol | `TEST(...)` |
|------|--------|-------------|
| `MAREACore.cpp` | `MAREA_GetVersion()` | `TEST(MAREACore, GetVersion)` |
| `MAREAMath.cpp` | `MAREA_Clamp()` | `TEST(MAREAMath, Clamp)` |

### Assertions

- `ASSERT_*` = preconditions (stop on failure). `EXPECT_*` = feature checks (continue).
- Every assertion carries a `<<` message - never bare. One blank line after each assertion.

```cpp
ASSERT_NE(result, MAREA_SINTEGER_32(0)) << "Result must not be zero.";

EXPECT_GT(result, MAREA_SINTEGER_32(0)) << "Result must be a positive integer.";
```

### No comments

No `//` or `/* */` anywhere. Use `GTEST_COUT` for output - at least one emission per `TEST` block:

```cpp
GTEST_COUT << "Validating MAREA_GetVersion returns a positive value." << std::endl;
```

### Unused return values

Wrap intentionally-ignored returns with `MAREA_IGNORE()`. Never call a `MAREA_NODISCARD` function without storing
or ignoring its result.

```cpp
MAREA_IGNORE(MAREA_DequeCreate(&deque));
MAREA_IGNORE(MAREA_DequePushBack(deque, &value));
```

### Naming conventions (clang-tidy)

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

1. Write the failing test in `tests/source/<module>/<MAREA_Filename>.cpp`.
2. Build and run it; confirm it fails for the right reason.
3. Implement the minimum, then refactor with the suite green.

## Validation

- The new `TEST` is discovered and passes; the rest of the test suite stays green.
- Every assertion has a `<<` message, every `TEST` emits `GTEST_COUT`, and the file has no comments.
