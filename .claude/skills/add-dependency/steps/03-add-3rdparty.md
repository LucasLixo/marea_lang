# Step 3 of 4 - Add the submodule and wire it in CMake

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Commit only through `generate-commit`. Never push.
- Never copy vendor files into the tree: a submodule only. A change to a vendor file is a patch kept outside the submodule, and needs the user first.
- Follow [cmake-style](../../cmake-style/SKILL.md) for the CMake edit.

`3rdparty/` is a plain folder of git submodules of this repository. `3rdparty/GoogleTest` is the model: `.gitmodules` holds its `path` and `url`, the submodule is pinned to the tag `v1.18.0`, and `tests/CMakeLists.txt` adds it with `add_subdirectory(... EXCLUDE_FROM_ALL)` and links `GTest::gtest_main` to `marea_tests`.

1. **Directory name is PascalCase** (`3rdparty/<Name>`), also when upstream's own name is lowercase.
2. **Add the submodule** at the version of step 01: `git submodule add <url> 3rdparty/<Name>`, then check out the tag inside it (`git -C 3rdparty/<Name> checkout <tag>`) so the pinned commit is the release. A tag pin has no `branch =` line in `.gitmodules`: `setup:git-modules-update` only moves submodules that follow a branch, never a tag. A dependency with no tag follows a branch: name its real default branch by hand with `branch = <name>` and say so in the report.
3. **Check the nested submodules** of the library: `mise run setup:git-modules` initializes them, and the license table of step 02 must cover each one the build compiles.
4. **Wire it in CMake**, test first when the dependency is code under test (a failing build or test that needs it, then the wiring):
   - add it with `add_subdirectory` from the `CMakeLists.txt` of the target that needs it, with a binary directory under `${CMAKE_BINARY_DIR}`, and `EXCLUDE_FROM_ALL` so only linked targets build it;
   - set the dependency's own options (tests, install, examples off) with the cache pattern of cmake-style before `add_subdirectory`;
   - link it with `target_link_libraries` and the right visibility (`PRIVATE` for the test target or an implementation detail of a stage; `PUBLIC` only when its types appear in a public header, which the opaque-struct rule of `AGENTS.md` makes rare). Prefer a static library.
5. Run `mise run format:cmake-sources`, then `mise run setup:git-modules` on a fresh state if you can, to prove the pin checks out.

## Done when

- `3rdparty/<Name>` exists with a PascalCase path, pinned to the tag of step 01, with the `.gitmodules` entry (`path`, `url`, and `branch` only when no tag exists).
- Nested submodules are initialized and covered by the license table.
- The CMake wiring links the dependency only to the targets that need it, with `EXCLUDE_FROM_ALL` on the `add_subdirectory`.
- `mise run format:cmake-sources` ran.

Next: [steps/04-verify-and-report.md](04-verify-and-report.md)
