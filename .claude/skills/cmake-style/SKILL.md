---
name: cmake-style
mode: agent
effort: medium
description: Marea CMake conventions - modern target-based CMake, MAREA_ variables and MAREA_OPTION_* options, keyword-argument functions, generator expressions versus if(), early checks, the static-library-per-stage target layout, and gersemi formatting. Use when editing any CMakeLists.txt or .cmake file, adding a dependency, an option or a target.
globs: "**/CMakeLists.txt,**/*.cmake"
---

# CMake style

## Context and Objective

The root `CMakeLists.txt` only wires the build; each responsibility lives in its own file under `cmake/`: `cmake/functions/` (`Basic.cmake` with `BasicParseArguments`, `BasicSet` and friends, `FindExecutable.cmake`) and `cmake/settings/` (one `Configure<Name>.cmake` macro per concern: folders, policies, compiler ID, build type, options, mise tools, third-party checks, library targets, precompiled header). `lib/` has one `CMakeLists.txt` per folder: `include/` (INTERFACE target `marea::include`), `internal/` (INTERFACE `marea::internal`, linked PRIVATE) and `source/` (the stage libraries, through `ConfigureLibTargets`). Match what they do. Governing rules: the project [AGENTS.md](../../../AGENTS.md) (it wins over this file); the design record is `temp/OBJETIVO.md` (objectives 4, 11, 37 and decision 38), which `temp/` keeps out of git. Build commands are `mise` tasks when a matching task exists (`config:cmake`, `build:marea`); never hand-roll the configure line.

A section marked "planned" describes work that is not in the repository yet. Do not invent its files; write them only when the work is requested, and then update the section.

## Strict Requirements

- **Modern CMake, target-based**: `target_include_directories`, `target_link_libraries`, `target_compile_options` with explicit `PUBLIC`/`PRIVATE`/`INTERFACE`. No directory-wide `include_directories`, `add_definitions` or `link_libraries`, and no hand-set `CMAKE_C_FLAGS`. Raise `cmake_minimum_required` only when a feature needs it (the repository uses 3.20).
- **Languages**: production code is C11 (`CMAKE_C_STANDARD 11`, `CMAKE_C_EXTENSIONS OFF`). C++17 exists only for the GoogleTest suite under `tests/`, see [cpp-testing](../cpp-testing/SKILL.md). Never add a C++ source to a production target.
- **Copyright header**: every new or substantially rewritten `CMakeLists.txt` or `.cmake` file opens with the header from [AGENTS.md](../../../AGENTS.md) in `#` comments, as its first lines.
- **ASCII only**, quoted paths, one-line comments (see `## Formatting`).
- **TDD**: a failing test first, then the minimum change. For a target or an option, the failing test is a GoogleTest case under `tests/` that needs the new target, or a configure or build that fails (`mise run config:cmake`, `mise run build:marea`). No script-mode CMake test framework exists yet; do not write one without a request.
- **Keep it green**: configure and build must pass after every increment, with `MAREA_OPTION_TARGET_TESTS` both ON and OFF.

## Variables

**Names are UPPER_SNAKE with the `MAREA_` prefix**, widest scope first (`MAREA_OPTION_TARGET_TESTS`, `MAREA_TESTS_SOURCES`). A value used in one scope and not needed after it is `unset()` at the end of a macro, where it would otherwise leak into the caller.

**An immutable cached value takes `CACHE <TYPE> "docstring"`** (`PATH` for a directory, `FILEPATH` for a file, `BOOL` for a flag, `STRING` for text, `INTERNAL` for a name that users never set). Add `FORCE` only to override a dependency's own cache entry before `add_subdirectory`.

An upstream option of a third-party project keeps the spelling that project defines.

## Options

- An option is `option(MAREA_OPTION_<NAME> "Description" ON|OFF)`, with a description that says what it builds or changes. A normally built target is ON.
- A rule between options uses `cmake_dependent_option` (`include(CMakeDependentOption)`), not a hand-written `if()` that forces a value. A message that explains a forced value, and a combination that has no valid fallback (`FATAL_ERROR`), stay outside it.
- Options that build a subset of the project are named `MAREA_OPTION_TARGET_<THING>` (today `MAREA_OPTION_TARGET_TESTS`, ON, guarding `enable_testing()` and `add_subdirectory(tests)`).
- Global defaults of the language come from CMake options and not from a `marea.toml`: `MAREA_DEFAULT_INLINE` and `CMAKE_BUILD_TYPE` (SIMD is chosen by the `backend` attribute of `marea.math`, not by a global option) (planned, not in the repository yet; objective 37).
- `MAREA_OPTION_SHARED_LIBS` selects a shared build, OFF by default (see `## Targets and linkage`; planned, not in the repository yet).

Declare an option in `cmake/settings/ConfigureOptions.cmake`, in its one block. A folder path is defined once in `cmake/settings/ConfigureFolders.cmake` as `MAREA_FOLDER_*`; read that variable and never rebuild the path from `CMAKE_SOURCE_DIR`. `ConfigureMiseTools` checks that every tool of `mise.toml` [tools] provides its executables; a new tool there needs an entry in that file.

The generated `MAREASettingsHeader.h` has its template and its generating script next to the public headers (`lib/include/marea/MAREASettingsHeader.h.in` and `.cmake`, included by `lib/include/CMakeLists.txt`); it reaches the compiler as a force-include on the command line (`BasicForceIncludeHeader`, INTERFACE on `marea::include`), never through an `#include` in a source or header.

Every file under `cmake/` starts with a guard that fails when it is included twice, and a settings file exposes one macro named like the file.

## Keyword arguments

A CMake function or macro that takes parameters takes keyword arguments, not positional ones: declare it without formal parameters and parse `${ARGN}` with `cmake_parse_arguments(PARSE_ARGV 0 PARAM "<flags>" "<one-value>" "<multi-value>")` in a `function()`. Check required keywords and fail early with `message(FATAL_ERROR "<function>: missing required argument <KEY>")`. The call site names every value.

Use `function()` when the work is self-contained. Use `macro()` only when the body must set variables or define targets in the caller scope. Parameters take the `PARAM_` prefix, scope-locals the `TEMP_` prefix.

Planned, not in the repository yet: `marea_add_library` and `marea_add_executable` (see `## Targets and linkage`). When written, they follow this section.

## Generator expressions versus if()

**A generator expression when the condition only picks a flag, a definition or a link library; `if()` when it decides what configure does.** A generator expression is evaluated per target, configuration and language at generate time, so it cannot choose a cache value, create a target, include a file or run a check.

```cmake
# Per compiler
"$<$<C_COMPILER_ID:Clang,GNU>:-Wall>"

# Per configuration and language together
"$<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:C>>:-flto>"

# A configure-time variable read at generate time
"$<$<BOOL:${MAREA_SOME_FLAG}>:MAREA_SOME_DEFINE>"
```

What stays an `if()`: target creation, `add_subdirectory`, option checks, and any value read at configure time. Flags that depend on the compiler or on the configuration go in a generator expression on the target, never in `CMAKE_C_FLAGS`.

The toolchain is clang/LLVM on every platform (`mise.toml` sets `CC=clang`); do not add a GCC-specific path.

## Targets and linkage

From the design record (objective 37 and decision 38); the repository today has one library, `marea`, and the test executable `marea_tests`.

- **One static library target per compiler stage** (`lex`, `parse`, `sema`, `ir`, `bytecode`, `vm`, `cgen`), plus a small shared base (arena, diagnostics, source position) that the stages link. A stage links only earlier stages, never a later one; the build graph must match the dependency order of the stages.
- **Libraries are static by default.** A shared build happens only when `MAREA_OPTION_SHARED_LIBS` is ON (OFF by default); the choice is made once, through `BUILD_SHARED_LIBS` or the library type, and never hard-coded per target. A shared build needs an export macro; keep the public header the only place that has it.
- **Targets and aliases**: each library has an `ALIAS` in the `marea::` namespace declared right after it (`marea::lex`), and other targets link the alias. Library targets follow the Mimico engine pattern, `Marea-<Stage>` (`Marea-Base`, `Marea-Lex`) with the alias `marea::<stage>`; the test executable stays `marea_tests`.
- **Public include directory**: `PUBLIC` is `lib/include`; internal headers (`lib/internal`) are `PRIVATE` and never reach a consumer. Sources are listed per target or globbed with `CONFIGURE_DEPENDS`, like `tests/CMakeLists.txt`.
- **`MAREA_OPTION_TARGET_*` options build a subset**, so a consumer can build only the lexer or only the interpreter without configuring the rest. Every option-guarded target works alone.
- **No own driver.** A small set of CMake functions, `marea_add_library` and `marea_add_executable`, will call `marea-cc` (C11 to machine code) and `marea-bc` (bytecode). Planned, not in the repository yet.
- **Release builds strip** the output (`--strip`, objective 11). Planned, not in the repository yet; set it on the target for the `Release` configuration through a generator expression.

## Early checks

Check a requirement at the top of configure and stop with one `message(FATAL_ERROR ...)` that names what is missing and the command that fixes it, instead of a failure deep inside a dependency. Examples that apply today:

- a missing 3rdparty checkout: `3rdparty/GoogleTest` is a git submodule; the hint is `mise run setup:git-modules`.
- a missing compiler or tool: the hint is `mise install`.

## Third-party code

`3rdparty/` holds only GoogleTest, a git submodule that `tests/CMakeLists.txt` adds with `add_subdirectory(... EXCLUDE_FROM_ALL)`; production targets never link it. A new dependency follows [add-dependency](../add-dependency/SKILL.md), which owns the license rules and the vendoring steps; this file adds only the CMake side: add it with `add_subdirectory` and `EXCLUDE_FROM_ALL`, link it to the one target that needs it, and set its upstream options (`CACHE ... FORCE`) before `add_subdirectory`. Patching a vendored source is not a supported workflow; ask first.

## Validation

- `mise run config:cmake` then `mise run build:marea` (both default to Debug; `-- --config Release` overrides), and the tests through `ctest --test-dir cmake-build-debug`, with `MAREA_OPTION_TARGET_TESTS` ON and OFF.
- `mise run format:check-sources` is the read-only formatting check; `mise run dev:ascii-check` checks ASCII.
- To prove "no behaviour change" in a refactor, compare a fresh configure (a reconfigure reuses the cache), such as the sorted `compile_commands.json`.

## Formatting

gersemi is the formatter and wins on whitespace and layout: indent, line width and list wrapping come from [.gersemirc](../../../.gersemirc) and are applied by `mise run format:cmake-sources`. What the formatter cannot decide stays a hand rule: paths are always quoted.

**A comment is one line**, saying what the block does and, where the reason is not obvious, why. No consecutive `#` comment lines; a section banner is one line.
