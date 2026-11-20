# Step 1 of 7 - Justify and classify the dependency

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Do not edit any file in this step.
- The dependency is a last resort: if SDL3, the standard library or an existing dependency does the job, stop and say so.

Decide whether the dependency is needed and what kind it is.

1. State in one sentence what the dependency does for the engine and which package or feature uses it.
2. Walk the "Source independence" order ([c-style](../../c-style/SKILL.md) and [rust-style](../../rust-style/SKILL.md)): SDL3, then the standard library, then a crate or engine function that already exists, and only then new code. Search the repository first: `lib/rust/Cargo.toml`, `lib/rust/Cargo.lock`, `3rdparty/` and `.gitmodules`. A dependency that already exists is reused, not added a second time.
3. Name the kind:
   - **3rdparty library**: C or C++ code that CMake builds from a git submodule under `3rdparty/`.
   - **Cargo crate**: a crate of `lib/rust/Cargo.toml`.
   - Both: a crate that wraps a C library goes through both paths, and each path is checked on its own.
4. When more than one candidate does the job, compare them before you choose, with numbers and not with reputation. For each Cargo candidate run `cargo tree -e normal,build --prefix none` on a scratch copy, or read its manifest, and count: the crates it adds that are not in `Cargo.lock` yet, the crates it needs in a version that differs from the one already locked (these become duplicates), and the features it forces on. One candidate can be heavier or more complete than another and still be the cheaper choice, because everything it needs is already in the project. **Do not decide that alone: stop and ask the user** with `AskUserQuestion`, one option per candidate, each with its new-crate count, its duplicate count and what it offers more or less. Put your recommendation first. Go on only with the answer.
5. Name the exact upstream (URL) and the exact version or branch you propose.
6. Say where it is linked: which platforms (Windows, Linux, Android, Emscripten), which build types (Debug, Release), and which API package or option brings it in. Step 03 needs this.

## Done when

- You wrote the purpose, the kind, the upstream and the version or branch.
- With more than one candidate, you compared the new crates and the duplicates of each and the user chose.
- You searched the repository and the dependency does not exist yet, or you stopped.
- You know the platforms, the build types and the API option where it is linked.

Next: [steps/02-license-check.md](02-license-check.md)
