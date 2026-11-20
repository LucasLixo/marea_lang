---
name: cmake-style
mode: agent
effort: medium
description: Marea CMake conventions - the cmake/ layout, the GLOBAL-property error guard and macro skeleton of every module, keyword-argument helpers (BasicParseArguments), MAREA_ variables, generator expressions versus if(), options, patching 3rdparty sources (the patch-vendored-source skill) and vendoring a new one (the add-dependency skill), early checks, target naming, and the TDD rule for CMake (script-mode tests under cmake/tests/). Use when editing any CMakeLists.txt or .cmake file, adding a dependency, an option or a mise tool, or wiring a new target.
globs: "**/CMakeLists.txt,**/*.cmake"
---

# CMake style

## Context and Objective

Every file under `cmake/` and every `CMakeLists.txt` has the same shape: a copyright header, an error guard, one macro or function, keyword arguments. Match it. Governing rules: the engine [AGENTS.md](../../../AGENTS.md); building and running tasks is owned by [engine-build](../engine-build/SKILL.md). Build commands are `mise` tasks (mandatory when a matching task exists); never hand-roll the configure line.

## Strict Requirements

- **TDD for CMake**: behavior-changing CMake work (a helper, a guard, a check, an option rule, `ApplyPatch`) starts with a failing script-mode test under `cmake/tests/` (`test_*.cmake`), then the minimum change, then refactor, then `mise run test:cmake` (and `test:cmake-guards` when a module or `CMakeLists.txt` is added). Markdown-only guidance changes are exempt from synthetic tests.
- Source files are globbed (`CONFIGURE_DEPENDS`), so adding or removing a file normally needs no reconfigure; run `mise run config:cmake-reconfigure` only when a file is reported "not found" or a deleted one is still compiled.
- Copyright header, ASCII only, one-line comments, gersemi layout (see `## Formatting`).
- The rest of this document is the reference for each rule.

## File organization

| Directory | Holds | Shape |
|---|---|---|
| `cmake/functions/` | reusable helpers | `Basic.cmake`, `FindExecutable.cmake`, `YamlParser.cmake`, `CopyRuntimeDependencies.cmake` |
| `cmake/settings/` | project configuration only: folders, options, languages, LLVM, early checks, the engine's own targets | one `Configure<Thing>.cmake` defining `macro(Configure<Thing>)` |
| `cmake/dependencies/` | how each vendored 3rdparty library is configured and linked | one `Configure<Thing>.cmake` per library (`ConfigureSDL`, `ConfigureDearImgui`, ...) |
| `cmake/scripts/` | `cmake -P` script-mode tools | `GenerateDocsGroups.cmake`, driven by `-DPARAM_*` |
| `cmake/toolchain/` | `<platform>.toolchain.cmake` | `android`, `linux`, `web`, `windows` |
| `cmake/tests/` | `cmake -P` checks over the modules | `test_arguments.cmake`, `check_guards.cmake`, `RunTests.cmake` |

A module that generates a specific file lives next to it instead - `lib/include/marea/SettingsHeader.cmake`, `lib/internal/marea/internal/PrecompileHeader.cmake`.

A `CMakeLists.txt` stays thin: guard, `project()`, and one `include(...)` plus one call per module (`lib/CMakeLists.txt` is six of them). New logic goes in a `Configure<Thing>` module, never inline. Two kinds of names live in one place: every folder variable in `ConfigureFolders` (`cmake/settings/ConfigureFolders.cmake`, called once from the root) and every engine library target name and alias in `ConfigureMareaLibNames` - add new ones there, not at the point of use.

A new file gets the copyright header from `CLAUDE.md` in `#` comments as its first lines, above the guard.

## Vendoring a new 3rdparty dependency

Owned by the [add-dependency](../add-dependency/SKILL.md) skill: the five vendoring steps, the permissive-license rule, the `license_embed_add` registration (always inside an `if`) and the Cargo crate path. This file keeps no copy of it. The CMake side of a new dependency still follows this skill: the `Configure<Name>` module shape, `if()` versus generator expressions and the TDD rule.

## Patching vendored sources

Owned by the [patch-vendored-source](../patch-vendored-source/SKILL.md) skill: where a patch lives, its format, the `ApplyPatch` call, the option per patch, the checks and the rebase when upstream moves. This file keeps only the `ApplyPatch` row of the helper table and the option rule below.

## Module skeleton

Every module opens with the header, the version floor, the GLOBAL-property error guard, and one macro named after the file.

```cmake
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

get_property(TEMP_GUARD_SET GLOBAL PROPERTY ConfigureSDLGuard SET)
if(TEMP_GUARD_SET)
    message(FATAL_ERROR "ConfigureSDL.cmake was included twice")
endif()
set_property(GLOBAL PROPERTY ConfigureSDLGuard ON)
unset(TEMP_GUARD_SET)

# Macro
macro(ConfigureSDL)
    ...
endmacro()
```

The guard is a GLOBAL property, so a second `include()` from any directory scope aborts configure instead of silently returning. **A file is included exactly once per configure; a later scope (an application, lib, tests) calls the macro the first include defined.** The property is `<FileName>Guard`, the message `<FileName>.cmake was included twice`.

- Exempt: `cmake/toolchain/*` (try_compile re-runs them) and `cmake/scripts/*` (they run under `cmake -P`).
- `ConfigurePolicies.cmake` has no `cmake_minimum_required`, which would reset the policies in the including scope, and is included with `NO_POLICY_SCOPE` (root `CMakeLists.txt`, and an application that is its own configure root, such as the Creator).
- Every `CMakeLists.txt` reached by `add_subdirectory()` (and an application's root) carries the same guard, named after the project and its folder: `MareaRootGuard`, `MareaLibGuard`, `MareaLibIncludeGuard`, `MareaLibInternalGuard`, `MareaLibSourceGuard`, `MareaTestsGuard`, `MareaDocsGuard`, `MareaTypingsGuard`, `MareaExampleDemoGpuGuard`, `MareaExampleDemoSpritesGuard`. The message is `"The root CMakeLists.txt was included twice"` (root) or `"<path>/CMakeLists.txt was included twice"` (`lib/CMakeLists.txt`, `tests/CMakeLists.txt`, ...).
- `mise run test:cmake-guards` fails listing any included `.cmake` or `CMakeLists.txt` that lacks its guard; a new `CMakeLists.txt` needs a row in the `MAREA_CHECK_LISTS` table of `cmake/tests/check_guards.cmake`.

The caller includes the file by its folder variable and invokes the macro on the next line:

```cmake
include("${MAREA_FOLDER_CMAKE_DEPENDENCIES}/ConfigureSDL.cmake")
ConfigureSDL()
```

**State that CMake keeps per directory** (languages, `add_compile_options`, `add_link_options`, compiler launchers, job pool variables, link directories) is not inherited by a scope that is not a child of the caller: when an application is the configure root (the Creator) this root runs in a child scope of it, so the application's `CMakeLists.txt` calls `ConfigureLanguages()`, `ConfigureLinuxLinkDirectories()` and `ConfigureLlvm()` again after its own `project()`. What must happen once per configure (defining Ninja job pools) is gated on a GLOBAL property inside the macro (`ConfigureLlvmApplied` in `cmake/settings/ConfigureLlvm.cmake`).

**A value a parent scope needs goes up through one `foreach`** with the unquoted `${${VAR}}` form (root `CMakeLists.txt`):

```cmake
if(MAREA_SUBDIRECTORY)
    foreach(TEMP_VAR MAREA_COMPILER_IS_GCC_LIKE MAREA_COMPILER_IS_MSVC_LIKE MAREA_COMPILER_IS_INTEL_LIKE ...)
        set(${TEMP_VAR} ${${TEMP_VAR}} PARENT_SCOPE)
    endforeach()
    unset(TEMP_VAR)
endif()
```

Never one `set(NAME ${NAME} PARENT_SCOPE)` block per variable copied by hand: a missing `${}` silently sets the variable to its own name.

## Root order

The root `CMakeLists.txt` runs, in this order: policies (top-level only), the labelled block of built-in modules, the function files, `ConfigureFolders`, metadata (`YamlParser`), `project(... LANGUAGES NONE)`, `ConfigureLanguages`, `ConfigureCompilerID`, `ConfigurePlatform`, `ConfigureBuildType`, `ConfigureLlvm`, `ConfigureLinuxLinkDirectories`, `ConfigureOptions`, `ConfigureMiseTools`, `ConfigureThirdpartyProjects`, then the `add_subdirectory()` calls. A module that reads a variable must sit after the module that sets it: the mise tools check needs the options and `ConfigurePlatform`, the 3rdparty check needs the final `MAREA_OPTION_JIT`.

## macro or function

Use `macro` when the body must set variables or define targets in the caller's scope - that is every `Configure*` module. Use `function` when the work is self-contained and only reports back: `BasicParseArguments`, `BasicExecuteCommand`, `YamlParser`, `FindExecutable`, `ConfigureLicenseEmbedCMake`.

A macro leaks every variable it touches into the caller, which is what makes the `unset` rule below load-bearing.

## Variables

**Names are UPPER_SNAKE with the `MAREA_` prefix and read hierarchically**, widest scope first: `MAREA_FOLDER_CMAKE_BINARY_3RDPARTY_SDL`, `MAREA_LIB_SHARED_ALIAS`. Parameters take `PARAM_`, scope-locals take `TEMP_`.

**An immutable value takes `CACHE <TYPE> "docstring"`.** Pick the type by what it holds:

| Type | For |
|---|---|
| `PATH` | a directory |
| `FILEPATH` | a single file |
| `BOOL` | a flag |
| `STRING` | text or a version |
| `INTERNAL` | a name other scopes read but users never set, such as a target name |

```cmake
set(MAREA_FOLDER_LIB "${MAREA_FOLDER}/lib" CACHE PATH "Source directory for Marea Core Library")
set(MAREA_LIB_INCLUDE "${MAREA_METADATA_label}-Lib-Include" CACHE INTERNAL "Public include interface target")
```

Add `FORCE` only to override a dependency's own cache entry before `add_subdirectory`, or to keep a cached mirror in step with what CMake resolved:

```cmake
set(SDL_TESTS OFF CACHE BOOL "Build SDL test programs" FORCE)
set(MAREA_LANGUAGE_${TEMP_LANGUAGE}_COMPILER
    "${CMAKE_${TEMP_LANGUAGE}_COMPILER}"
    CACHE FILEPATH
    "${TEMP_LANGUAGE} compiler used to build Marea"
    FORCE
)
```

**A value used in one scope is `TEMP_`-prefixed and `unset()` at the end**, under a `# Unset` comment. Mandatory in a macro, where the variable would otherwise reach the caller.

```cmake
macro(BasicCapitalize)
    BasicParseArguments(BasicCapitalize PARAM "" "INPUT;OUTPUT" ""
        REQUIRED INPUT OUTPUT
        TYPES INPUT:STRING OUTPUT:NAME
        ARGS ${ARGN}
    )
    string(TOLOWER "${PARAM_INPUT}" TEMP_LOWER_STRING)
    string(SUBSTRING "${TEMP_LOWER_STRING}" 0 1 TEMP_FIRST_CHAR)

    ...

    # Unset
    unset(PARAM_INPUT)
    unset(PARAM_OUTPUT)
    unset(TEMP_LOWER_STRING)
    unset(TEMP_FIRST_CHAR)
endmacro()
```

**Exceptions to UPPER_SNAKE**, and only these: the PascalCase `<Module>Guard` GLOBAL property, which mirrors the file it protects; `MAREA_METADATA_<yaml_key>`, which keeps the YAML key's own case; and an upstream option, spelled exactly as the third party defines it (`SDL_SHARED`, `gtest_force_shared_crt`, `Rust_CARGO_TARGET`).

A script-mode file under `cmake/scripts/` or `cmake/tests/` has no caller to leak into, so its file-scope variables are module variables named `MAREA_<TOPIC>_*` rather than `TEMP_`, and they need no `unset`.

## Helpers and keyword arguments

Reuse `cmake/functions/` rather than rewriting its work.

| Helper | Keywords (`*` required) | Does |
|---|---|---|
| `BasicParseArguments(<owner> <prefix> "<flags>" "<one>" "<multi>" REQUIRED ... TYPES ... ARGS ${ARGN})` | - | validates keyword arguments and returns them as `<prefix>_<KEY>` |
| `BasicSet` | `NAME*` `VALUE*` `PARENT_SCOPE` | sets, optionally propagates to `PARENT_SCOPE`, and logs the assignment |
| `BasicCapitalize`, `BasicCapitalizeHyphenated` | `INPUT*` `OUTPUT*` | derive a target name (`marea-lib-shared` -> `Marea-Lib-Shared`) |
| `BasicGetDirectoryGenerate` | `OUTPUT*` `TARGET*` `SUFFIX` | resolve and create a target's generated-output directory |
| `BasicForceIncludeHeader` | `TARGET*` `VISIBILITY*` `HEADER*` | apply the per-compiler force-include flag to a target |
| `BasicExecuteCommand` | `COMMAND*` (list) `ERROR_MESSAGE*` | run a command and fail with `FATAL_ERROR` on a non-zero result |
| `FindExecutable` | `NAME*` `OUTPUT*` `REQUIRED` | tool lookup, caches the path |
| `ApplyPatch` | `SOURCE*` `PATCH*` `OUTPUT*` `RELATIVE*` `ENABLED` | writes `OUTPUT` as `SOURCE` with a unified diff applied through git (see `## Patching vendored sources`) |
| `YamlParser` | `FILE*` `PREFIX*` | metadata.yaml reader |
| `CopyRuntimeDependencies` | `TARGET_NAME*` `COMPONENT*` | install a target and its resolved runtime libraries under a CPack component |
| `ConfigureBuildType` | `CURRENT*` `AVAILABLE*` | name the variables that receive the current build type (`$<CONFIG>` for multi-config generators) and the list of types |
| `ConfigureMarkupAssets` | `TARGET*` `COMPONENT*` plus source directories | stage markup for a target, shared under `cmake/settings/` so any project root that `add_subdirectory()`s the repo root can call it |

`BasicSet` is the default for a mutable value that subdirectories need; a raw `set` with `CACHE` is for an immutable one.

**Every parameterized helper or macro takes keyword arguments.** It is declared without formal parameters and opens with one `BasicParseArguments` call using the prefix `PARAM` (`cmake/functions/FindExecutable.cmake`):

```cmake
function(FindExecutable)
    BasicParseArguments(FindExecutable PARAM "REQUIRED" "NAME;OUTPUT" ""
        REQUIRED NAME OUTPUT
        TYPES NAME:STRING OUTPUT:NAME
        ARGS ${ARGN}
    )
```

The call site names every value: `FindExecutable(NAME "doxygen" OUTPUT MAREA_EXECUTABLE_DOXYGEN REQUIRED)`. `BasicParseArguments` is itself a function taking the caller's `${ARGN}` after the `ARGS` marker, so a macro's body reads `PARAM_<KEY>` and unsets each key at the end. An unset optional key is unset in the caller too.

Types, one per `TYPES <KEY>:<TYPE>[:EXISTS]` entry:

| Type | Accepts |
|---|---|
| `STRING` | any non-empty value |
| `PATH`, `FILEPATH` | non-empty; `:EXISTS` also requires the path to exist |
| `BOOL` | `ON` `OFF` `TRUE` `FALSE` `YES` `NO` `1` `0`, any case |
| `TARGET` | non-empty; `:EXISTS` also requires `TARGET <name>` |
| `LIST` | non-empty list |
| `NAME` | a variable name (`^[A-Za-z_][A-Za-z0-9_]*$`), for `OUTPUT` keys |

A bad call stops configure with one of these, naming the owner:

- `<Owner>: missing required argument <KEY>`
- `<Owner>: unexpected argument(s): <tokens>`
- `<Owner>: keyword <KEY> requires a value` (an empty value forwarded through a macro's `${ARGN}` is dropped by CMake and reports this too)
- `<Owner>: <KEY> expects <TYPE>, got '<value>'` (`existing PATH` or `existing TARGET` for `:EXISTS`)
- `<Owner>: unknown type <TYPE> for <KEY>`, `<Owner>: unknown type modifier <M> for <KEY>`

A new helper gets its error cases in `cmake/tests/test_arguments.cmake`; `mise run test:cmake` runs every `cmake/tests/test_*.cmake` in its own `cmake -P` process with no configure needed.

## Generator expressions versus if()

**A generator expression when the condition only picks a flag, a definition or a link library; `if()` when it decides what configure does.** A generator expression is evaluated per target, configuration and language at generate time, so it cannot choose a cache option before `add_subdirectory`, whether to create a target, which tool or build step to use, or a property that does not evaluate them (`SUFFIX`).

The idioms in use:

```cmake
# Compiler ID: the list is defined once (TEMP_IDS_GCC_LIKE, mirrors MAREA_COMPILER_IS_GCC_LIKE)
"$<$<C_COMPILER_ID:${TEMP_IDS_GCC_LIKE}>:SHELL:-finput-charset=UTF-8>"

# Platform, for defines and link libraries
"$<$<PLATFORM_ID:Linux>:MAREA_PLATFORM_LINUX>"

# Configuration and language together
"$<$<AND:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,$<COMPILE_LANGUAGE:C,CXX>>:${TEMP_LLVM_LTO_FLAG}>"

# A configure-time variable read at generate time
"$<$<BOOL:${MAREA_COMPILER_IS_GCC_LIKE}>:-Wl,--start-group>"
```

The first three are from `cmake/settings/ConfigureMareaLibCompiler.cmake`, `ConfigureMareaLibPlatform.cmake` and `ConfigureLlvm.cmake`; the last from `ConfigureMareaLibLinking.cmake`.

- The lib modules prefix each flag with `SHELL:` (`SHELL:-Wa,-mbig-obj`) so CMake keeps it as written; a `>` inside a header name in a generator expression is written `$<ANGLE-R>`, as in the precompiled header lists.
- Use `-Wl,...` rather than `LINKER:...` where the command line must stay identical (`ConfigurePageSize.cmake`): Clang renders `LINKER:` as `-Xlinker` pairs.
- `PLATFORM_ID` reads `CMAKE_SYSTEM_NAME`, which `cmake/toolchain/windows.toolchain.cmake` does not set: for a Linux-hosted MinGW cross build `$<PLATFORM_ID:Windows>` reads Linux. Fix the toolchain before relying on it there.
- What stays an `if()`: `if(MINGW)` (a configure variable, `-Wa,-mbig-obj` in `ConfigureMareaLibCompiler.cmake` and the root), target creation (`add_library` versus `add_executable` on Android), `SUFFIX ".html"` for Web, options read at configure time, and any branch that selects a whole configure-time strategy. The `MAREA_PLATFORM_*` and `MAREA_COMPILER_IS_*` variables remain for those.

## Options

**All project options are declared in `cmake/settings/ConfigureOptions.cmake`**, never in the root or a project `CMakeLists.txt`. An independent one is `option(MAREA_OPTION_<NAME> "Description" ON|OFF)`, ON for anything normally built. **A rule between options is `cmake_dependent_option`, never a business `if`**: while its condition is false the option is forced to the fallback value and hidden from the cache UI.

```cmake
cmake_dependent_option(
    MAREA_OPTION_TARGET_TESTS
    "Enable target tests"
    ON
    "MAREA_OPTION_TARGET_LIB"
    OFF
)
cmake_dependent_option(
    MAREA_OPTION_STATIC_RUNTIME
    "Build the runtime library STATIC"
    ON
    "NOT MAREA_OPTION_SHARED_RUNTIME"
    OFF
)
cmake_dependent_option(
    MAREA_OPTION_BACKEND_LLVM
    "Enable the LLVM code generation backend"
    ON
    "NOT MAREA_COMPILER_IS_INTEL_LIKE;NOT MAREA_PLATFORM_WEB"
    OFF
)
```

`cmake_dependent_option` forces silently and works on BOOL only, so what stays outside it is the explanation of a forced value (the `message(WARNING ...)` for an unsupported platform/compiler) and the one combination that has no valid fallback, both the static and shared runtime OFF (`FATAL_ERROR`).

**Each patch under `patches/` has its own `MAREA_OPTION_PATCH_<PROJECT>_<NAME>` option**, an independent `option(...)` ON by default, grouped under a `# One option per patch` comment in `ConfigureOptions`; it feeds the `ENABLED` argument of `ApplyPatch` ([patch-vendored-source](../patch-vendored-source/SKILL.md)).

**The `MAREA_LLVM_*` options are declared in `cmake/settings/ConfigureLlvm.cmake`**, the exception: they are STRING-valued, validated with `FATAL_ERROR`, and named after the LLVM CMake variable they mirror. All default off or empty.

| Option | Values | Maps to |
|---|---|---|
| `MAREA_LLVM_CCACHE_BUILD` | `ON`/`OFF` | `CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER` set to ccache (Ninja or Makefile generators) |
| `MAREA_LLVM_USE_LINKER` | `""`, `lld`, `gold`, `bfd` | `-fuse-ld=<linker>` on the link line |
| `MAREA_LLVM_ENABLE_LTO` | `Off`, `Thin`, `Full` | `-flto=thin` (Clang only) or `-flto` for Release, RelWithDebInfo and MinSizeRel; Clang on Linux needs `MAREA_LLVM_USE_LINKER=lld` |
| `MAREA_LLVM_USE_SANITIZER` | `""` or a `;` list of `Address`, `Undefined`, `Thread` | `-fsanitize=...` and `-fno-omit-frame-pointer`, plus `-shared-libsan` on Clang |
| `MAREA_LLVM_PARALLEL_COMPILE_JOBS`, `MAREA_LLVM_PARALLEL_LINK_JOBS` | `""` or a positive integer | Ninja job pools `marea_compile` and `marea_link` |

The `MAREA_API_PACKAGES` list in `lib/include/marea/SettingsHeader.cmake` drives the per-package `MAREA_API_<PKG>` defines; `c-style` owns what the C side does with them.

## Built-in module includes

A CMake built-in module (`include(CheckLanguage)`) is included only in the labelled block of the project root that uses it, never inside a macro or module:

```cmake
# ==================================================
# CMake built-in modules used by the root project
# ==================================================
include(CheckLanguage)
include(CMakeDependentOption)
include(CheckIPOSupported)
include(GoogleTest)
```

An application's `CMakeLists.txt` (the Creator's) has the same block for `InstallRequiredSystemLibraries`, placed after `ConfigureLanguages()` because it needs the compiler ID. The one exception is `include(CPack)`, which stays right after `ConfigurePackaging()` (the root's, at the end of the top-level `CMakeLists.txt`, or the application's own): it reads the `CPACK_*` variables at include time. `CMakeParseArguments` is not included (built in since CMake 3.5).

## Early checks

Three modules verify a requirement up front, collect every failure, and stop configure with one `FATAL_ERROR` and one hint, instead of a failure deep inside a dependency.

| Module | Checks | Hint in the message |
|---|---|---|
| `ConfigureLanguages` | ASM, C and CXX compilers, through `check_language`; caches each as `MAREA_LANGUAGE_<LANG>_COMPILER` and enables it once | `mise install` |
| `ConfigureMiseTools` | every key of the `[tools]` table of `mise.toml` provides the executables the current options and platform need | `run: mise install` |
| `ConfigureThirdpartyProjects` | every 3rdparty project the current options configure is checked out, nested submodules included | `run: mise run setup:git-modules` |

`project()` is always `LANGUAGES NONE`; `ConfigureLanguages` enables the languages, and toolchain files keep choosing cross compilers. `ConfigureMiseTools` and `ConfigureThirdpartyProjects` run after `ConfigureOptions` and before the first `add_subdirectory()`.

**A new key in the `[tools]` table of `mise.toml` needs a branch in `ConfigureMiseTools`**, or configure fails with `unknown tool '<key>' in mise.toml: add it to ConfigureMiseTools`. **A new 3rdparty dependency, or a change to which externals SDL_image, SDL_ttf or SDL_mixer build, needs a row in `ConfigureThirdpartyProjects`** (`"cmake|<path>"` when the project has a `CMakeLists.txt`, `"content|<path>|<marker file>"` otherwise), gated on the option that configures it:

```cmake
if(MAREA_OPTION_TARGET_DOCS)
    list(APPEND TEMP_ROWS "content|DoxygenAwesomeCss|doxygen-awesome.css")
endif()
```

## Targets

Named `<Label>-<Role>[-<Variant>]` (`Marea-Lib-Shared`, `Marea-Tests`), with the name held in an `INTERNAL` cache variable (`ConfigureMareaLibNames`) or derived through `BasicCapitalizeHyphenated`, and a `::`-separated `ALIAS` declared right after. Library targets carry `EXCLUDE_FROM_ALL`, and creation is announced first (`cmake/settings/ConfigureMareaLibTargets.cmake`):

```cmake
message(STATUS "Add target: ${MAREA_LIB_SHARED}")
add_library(${MAREA_LIB_SHARED} SHARED EXCLUDE_FROM_ALL)
add_library(${MAREA_LIB_SHARED_ALIAS} ALIAS ${MAREA_LIB_SHARED})
```

Sources are globbed, so adding a file needs no CMake edit (`lib/source/CMakeLists.txt`):

```cmake
file(GLOB_RECURSE MAREA_PRIVATE_SOURCES CONFIGURE_DEPENDS "*.c" "*.cpp" "**/*.c" "**/*.cpp")
```

## Build-tree directories

Every repository-owned directory CMake creates under the build root follows the established Case-Case convention. This covers `add_subdirectory()` binary directories, target output directories, generated staging directories and ad-hoc repository-owned subdirectories. Reuse `BasicGetDirectoryTarget`, which preserves existing hyphens and separates compound words without changing the target name; generated target directories go through `BasicGetDirectoryGenerate` so they cannot drift. For example, `Marea-Docs` remains `Marea-Docs`, `DearImguiImpl` resolves to `Dear-Imgui-Impl`, and `Marea-ApiIndex` resolves to `Marea-Api-Index`.

Every binary-producing repository target calls `BasicSetTargetOutputDirectory` immediately after creation. It assigns its `RUNTIME_OUTPUT_DIRECTORY`, `ARCHIVE_OUTPUT_DIRECTORY` and `LIBRARY_OUTPUT_DIRECTORY` to one target-private directory. OBJECT, INTERFACE, IMPORTED and stamp-only custom targets have no output directory; leave a one-line `no output dir - <reason>` comment at their creation site instead. Do not set the global `CMAKE_*_OUTPUT_DIRECTORY` variables, which would redirect vendor outputs and reintroduce shared dumping grounds.

Vendored and external-tooling internals remain out of scope: Corrosion and cargo state, LicenseEmbed's license and SPDX folders, vendored GoogleTest internals, and a vendored tool's required working directory. Record the exception beside the creation site. Existing build trees are invalid after a directory rename; configure a fresh tree rather than moving stale directories.

## Validation

Two script-mode tasks, neither needs a configure:

- `mise run test:cmake` - every `cmake/tests/test_*.cmake` in its own `cmake -P` process (the keyword-argument validator, `ApplyPatch`, error messages).
- `mise run test:cmake-guards` - every included `.cmake` module and the listed `CMakeLists.txt` carry the guard.

Anything else is proven by configure and build results through the mise tasks ([engine-build](../engine-build/SKILL.md)). To prove "no behaviour change", compare only **fresh** configures (a re-configure reuses the cache): the sorted `compile_commands.json`, `ninja -t commands`, `cmake -L -N` and `cmake_install.cmake` before and after, after normalizing the path-derived `xxd_resources_<hash>` target name. A new check module must also be shown to fail with its message on a deliberately broken input.

## Formatting

gersemi is the formatter and wins on whitespace and layout: indent, line width and how argument lists wrap are decided by [`.gersemirc`](../../../.gersemirc) and applied by `mise run format:cmake-sources`; `mise run format:check-sources` is the read-only check. This skill restates none of those settings. A project function or macro that gersemi cannot resolve is declared in the `definitions` the config lists (`cmake/`, `.gersemi/Definitions.cmake`), or gersemi lowercases it.

What the formatter cannot decide stays a hand rule: paths are always quoted.

**A comment is one line**, saying what the block does and, where the reason is not obvious, why; a second sentence goes on the same line after a `;`. No consecutive `#` comment lines. A file-level comment goes after the copyright header, above `cmake_minimum_required` (`ConfigureMiseTools.cmake`). Only a major section gets a banner:

```cmake
# ==================================================
# Options and early checks
# ==================================================
```

## Directory and file links

`file(CREATE_LINK ... SYMBOLIC)` works on POSIX but cannot make NTFS junctions on Windows (`JUNCTION` is not a valid argument). When two include paths must share device/inode IDs so Clang's `#pragma once` deduplicates (a `file(COPY)` breaks that), link instead of copying: `file(REMOVE_RECURSE)` the destination, then on a Windows host run PowerShell `New-Item -ItemType Junction -Path '<link>' -Target '<target>' -Force` through `execute_process` (backslash paths, `FATAL_ERROR` on a non-zero result, `unset` the `TEMP_` variables), else `file(CREATE_LINK <target> <link> SYMBOLIC)`. Prefer PowerShell over `cmd /c mklink /J` (older cmd rejects forward slashes). Junctions need no elevation; directory and file symlinks on Windows do.
