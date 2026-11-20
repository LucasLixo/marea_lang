# Step 4 of 7 - Add a 3rdparty library (skip for a crate)

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Commit only through `generate-commit`. Never push the engine before the `marea_3rdparty` commit exists.
- Never add a submodule path that is not PascalCase.

Run this step only for a 3rdparty library. For a Cargo crate only, say "not applicable" and go to step 05.

`3rdparty/` is itself a submodule: the `marea_3rdparty` repository (`https://github.com/LucasLixo/marea_3rdparty.git`), which owns every third-party submodule, `README.md`, `CREDITS.txt`, and `IconFontCppHeaders`. Adding a library is five steps, always in the same order - a dependency added without all five is incomplete, not merely undocumented. Steps 1 to 3 are commits in `marea_3rdparty` (`git submodule add <url> <Name>` at its root); after pushing it, commit the bumped `3rdparty` pointer here together with steps 4 and 5:

2. **`.gitmodules` always pins an explicit `branch =`**, even when the dependency has no tagged releases - name its real default branch by hand (`git remote show origin` on a fresh clone of it reports `HEAD branch: <name>`, e.g. `master` for `mozilla/nestegg`), never leave the field off to fall back on an implicit default. The gitlink pinned in `marea_3rdparty` is what a fresh `mise run setup:git-modules` checks out; `branch =` records where to update it from, so a later push upstream never changes a checkout on its own.
3. **One row in `marea_3rdparty`'s `README.md` table (`3rdparty/README.md` here) and one entry in its `CREDITS.txt`**, alphabetical by name, `Version` copied verbatim from the `.gitmodules` `branch` value, `License` from the dependency's own declared license.
4. **One row in `ConfigureThirdpartyProjects`** using the same PascalCase path ("Early checks" in [cmake-style](../../cmake-style/SKILL.md)) - configure never gets to see a missing or incomplete checkout without one.

The registration of the license (item 5 of the old list) is step 06 of this skill, with the `if` of step 03.

Also check that the library's own `Configure<Name>.cmake` links it only under the condition of step 03, and that the nested sub-dependencies of the library have the same README row when the build compiles them in. A nested directory that a vendor's own CMake pulls in with `add_subdirectory` is not exempt: "repositories inside repositories" need the same README row and the same registration.

If the new library needs a change in one of its own files (an Android fix, an exported symbol), that change is a patch: use [patch-vendored-source](../../patch-vendored-source/SKILL.md) after this step. Never copy the vendor file.

Work from the `marea_3rdparty` repository for items 1 to 3. Commit them there, then commit the bumped `3rdparty` pointer here with item 4 and step 06.

## Done when

- The submodule is added with a PascalCase path and an explicit `branch =` in `.gitmodules`.
- The `README.md` row and the `CREDITS.txt` entry exist in `marea_3rdparty`, alphabetical, with the version copied from `branch`.
- The `ConfigureThirdpartyProjects` row exists.
- `cmake/dependencies/Configure<Name>.cmake` links the library only under the condition of step 03.

Next: [steps/05-add-cargo.md](05-add-cargo.md)
