---
name: add-dependency
mode: edit
effort: high
description: Use when adding, replacing or upgrading a third-party dependency - a vendored 3rdparty library (git submodule plus CMake). Enforces permissive licenses only (MIT, Apache-2.0 and the like), the most permissive choice when several are offered, and no copyleft anywhere in the dependency tree. Run it before any edit to 3rdparty/ or .gitmodules.
argument-hint: "<name>"
---

# Add dependency

## Context

A dependency is code we do not own. Each one costs a license obligation, a build cost and an update cost. This skill is the canonical owner of how a new dependency enters the repository. Today the only kind is a **3rdparty library**: a git submodule under `3rdparty/` that CMake builds with `add_subdirectory`. `3rdparty/GoogleTest` is the one example; it is pinned to a tag and used only by the test suite. There is no Cargo crate and no Rust crate in the repository yet; when the first one is added, extend this skill and [rust-style](../rust-style/SKILL.md) together.

A dependency is a last resort: the compiler core is C11 written from scratch, so first use the C standard library and the code the repository already has (see [c-style](../c-style/SKILL.md)). Libraries are static by default and each compiler stage is its own library, so a dependency is linked only to the target that needs it.

Other documents link here and never restate the license rules.

## Requirements

- **Permissive licenses only.** Accepted: MIT, Apache-2.0, and the other permissive licenses: BSD-2-Clause, BSD-3-Clause, ISC, Zlib, Unlicense, 0BSD, CC0-1.0. Anything else is refused until the user clears it.
- **Restrictive licenses are prohibited**: GPL, LGPL, AGPL, MPL, EPL, SSPL, BUSL, Commons Clause and any license that forces the project to publish its own source or to relink. This holds for the dependency AND for every one of its own dependencies that is compiled in, at any depth. A permissive top-level license with a restrictive dependency below it is refused.
- **When a dependency offers several licenses, take the most permissive one** (the order is in step 02) and say which one was taken.
- Never guess a license from the reputation of a project. Read the license file.
- Stop and ask the user when a license is not clearly permissive, or when a dependency of the tree is restrictive.
- A dependency used only by tests (like GoogleTest) is added with `EXCLUDE_FROM_ALL` and linked only to the test target, so no production target ships it.
- Commit only through the `generate-commit` skill. ASCII only. The third-party license file stays inside the submodule as upstream wrote it; it is not rewritten and gets no Marea copyright header.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one.
4. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-justify-and-classify.md](steps/01-justify-and-classify.md) | Justify the dependency and pick the upstream and version |
| 2 | [steps/02-license-check.md](steps/02-license-check.md) | Check the license of the dependency and of its whole tree |
| 3 | [steps/03-add-3rdparty.md](steps/03-add-3rdparty.md) | Add the submodule and wire it in CMake |
| 4 | [steps/04-verify-and-report.md](steps/04-verify-and-report.md) | Verify and report |

## Related skills

- [cmake-style](../cmake-style/SKILL.md): CMake conventions, `if()` versus generator expressions and the TDD rule for CMake.
- [cpp-testing](../cpp-testing/SKILL.md): how the GoogleTest suite is written.
- [generate-commit](../generate-commit/SKILL.md): the only way to commit.
