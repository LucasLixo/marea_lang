---
name: add-dependency
mode: edit
effort: high
description: Use when adding, replacing or upgrading a third-party dependency - a vendored 3rdparty library (git submodule plus CMake) or a Cargo crate in lib/rust/Cargo.toml. Enforces permissive licenses only (MIT, Apache-2.0 and the like), the most permissive choice when several are offered, no copyleft anywhere in the dependency tree, and a license_embed_add registration that is always inside an if. Run it before any edit to 3rdparty/ or to the dependencies of Cargo.toml.
argument-hint: "<name> [3rdparty|cargo]"
---

# Add dependency

## Context

A dependency is code we do not own, shipped inside a proprietary engine. Each one costs a license obligation, a build cost and an update cost. This skill is the canonical owner of how a new dependency enters the repository, for the two kinds it has: a **3rdparty library** (a git submodule built by CMake) and a **Cargo crate** of `lib/rust`. The rule that decides whether to add one at all is the "Source independence" order of [c-style](../c-style/SKILL.md) and [rust-style](../rust-style/SKILL.md): a dependency is a last resort, after SDL3, the standard library and what the repository already has.

Other documents link here and never restate the license rules.

## Requirements

- **Permissive licenses only.** Accepted: MIT, Apache-2.0, and the other permissive licenses this repository already ships (BSD-2-Clause, BSD-3-Clause, ISC, Zlib, Unlicense, 0BSD, CC0-1.0). Anything else is refused until the user clears it.
- **Restrictive licenses are prohibited**: GPL, LGPL, AGPL, MPL, EPL, SSPL, BUSL, Commons Clause and any license that forces the project to publish its own source. This holds for the dependency AND for every one of its own dependencies that is compiled in, at any depth. A permissive top-level license with a restrictive dependency below it is refused.
- **When a dependency offers several licenses, take the most permissive one** (the order is in step 02) and say which one was taken.
- **No duplicate crate versions in the binary.** A crate that is compiled twice in two versions costs binary size and build time. Step 05 scans for duplicates before and after, and step 07 reports them. A new crate never adds a duplicate; an upgrade moves the direct dependencies to the version the rest of the tree already uses.
- **When two candidates differ in weight, ask the user.** One candidate may be heavier or more complete and still cost less, because every crate it needs is already in `Cargo.lock`. Step 01 puts that choice to the user with numbers; the skill never decides it alone.
- **Every `license_embed_add` is inside an `if`**, built from the condition under which the dependency is really linked: a platform, a build type, or an API package option. Never an unconditional registration. Step 03 explains the choice.
- Never guess a license from the reputation of a project. Read the license file and the metadata.
- Stop and ask the user when a license is not clearly permissive, when a dependency of the tree is restrictive, or when no honest condition for the `if` exists.
- Commit only through the `generate-commit` skill. TDD applies to the CMake change (a failing script test first). ASCII only.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text: step 04 is for a 3rdparty library, step 05 is for a crate, and a dependency that is both goes through both.
4. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-justify-and-classify.md](steps/01-justify-and-classify.md) | Justify and classify the dependency |
| 2 | [steps/02-license-check.md](steps/02-license-check.md) | Check the license of the dependency and of its whole tree |
| 3 | [steps/03-choose-the-condition.md](steps/03-choose-the-condition.md) | Choose the condition of the license registration |
| 4 | [steps/04-add-3rdparty.md](steps/04-add-3rdparty.md) | Add a 3rdparty library (skip for a crate) |
| 5 | [steps/05-add-cargo.md](steps/05-add-cargo.md) | Add a Cargo crate (skip for a 3rdparty library) |
| 6 | [steps/06-register-license.md](steps/06-register-license.md) | Register the license, test first |
| 7 | [steps/07-verify-and-report.md](steps/07-verify-and-report.md) | Verify and report |

## Related skills

- [cmake-style](../cmake-style/SKILL.md): CMake module shape, `if()` versus generator expressions, the `Configure<Name>` modules and the TDD rule for CMake.
- [rust-style](../rust-style/SKILL.md): crate layout, feature gating and the `Cargo.toml` conventions.
- [patch-vendored-source](../patch-vendored-source/SKILL.md): a change to a file of a library that is already vendored is a patch, never a copy.
- [engine-build](../engine-build/SKILL.md): the mise tasks that configure, build and test.
- [generate-commit](../generate-commit/SKILL.md): the only way to commit.
