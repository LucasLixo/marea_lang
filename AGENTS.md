---
name: marea-lang-agents
description: Global policy for agents working in the Marea Lang repository - scope, commits, TDD, ASCII, versioning, outcome logs - with detail delegated to the owning skills.
---

# Marea Lang

A statically-typed programming language of its own, in C11, on a modern CMake setup, with structure, enum, and
explicit function types checked once at compile/parse time and fully erased before code generation, compiled to
bytecode for a reference interpreter or to C11 from one IR (no JIT), with scoped memory and static ownership/borrow
rules, and one C
macro that registers a function for the language runtime and Rust alike, with no external generator and no hand-written glue on any side. The
project is independent: no tie to any other engine, language, or repository, with its own name, license, and
structure.

Comments and session chat according to the ASD-STE100 standard.

## Language boundary

The rule that shapes every decision here:

- **The compiler core is C11**, written from scratch - no legacy tree to reorganize, no inherited
  source to attribute.
- **One IR, two backends.** A bytecode backend with a small reference interpreter (portable, embedding)
  and a C11 backend built with clang or gcc (maximum performance). Both must give the same observable behavior,
  and a parity test runs every example through both. There is no JIT. The language has no standard library; only
  the namespace `c` is reserved. A host can replace a library or runtime function from C (never `c`), and
  reloads modules itself through a small C module API; hot reload is not a language feature.
- **Memory is scoped (regions) with static ownership and borrow rules.** Every non-primitive value has one owner. A
  primitive or a `#[value]` type passes by value; any other type is a read-only borrow, or a write borrow with
  `mutable`. A reference lives only for one call and is never stored or returned. Parallel
  code uses structured fork-join or a native thread pool, and the type system guarantees ownership and immutability
  across threads.
- **Typing is compile/parse-time only.** `structure`, `enum`, and explicit function types are checked statically and
  then fully erased before code generation - zero cost at runtime.
- **One binding declaration, two consumers.** A single C registration macro exposes a function to the language
  runtime and to Rust; there is no external binding generator and no per-language glue written by hand.
- **C++ exists only for tests** (GoogleTest, see [cpp-testing](.claude/skills/cpp-testing/SKILL.md)). Production
  code is C11 (or Rust where a binding needs it), never C++.

## Working model

- **Commit only through the `generate-commit` skill** ([.claude/skills/generate-commit/SKILL.md](.claude/skills/generate-commit/SKILL.md))
  - never a hand-written `git commit`, from the main session or from a subagent.
- **Independent tasks in different areas go through [parallelize](.claude/skills/parallelize/SKILL.md)**; it
  dispatches everything independent in parallel and never splits a task artificially.
- Senior pair programmer, not a code generator: challenge designs, propose simpler ones.
- **TDD is mandatory**: failing test first, minimum implementation, then refactor.
- Small compilable increments; keep the test suite green - fix broken tests before adding code.
- Docs are part of the change, not a follow-up.
- No Unicode characters in code, comments, docs, or commit messages - ASCII only, saved as UTF-8.
  Verify with `tools/marea-ascii-check` (`mise run dev:ascii-check`).
- **Copyright header.** Every new or substantially rewritten source file opens with this header as its first
  lines, above any include guard, in the file's own comment marker (`//`, `--`, or `#`):
  ```
  // Copyright (c) 2026 Paulo Lucas Alves Franca
  // Licensed under the MIT License. See LICENSE.txt for full terms.
  ```
  Skip this for third-party code that was not substantively rewritten - its original upstream notice stays as the
  only one, per [LICENSE.txt](LICENSE.txt).

## Reporting the outcome of any task

Every task ends with a record in the two logs [generate-apply](.claude/skills/generate-apply/references/logs.md)
defines, `temp/_LOG_APPLY_SUCCESS.md` and `temp/_LOG_APPLY_ERROR.md`, whether or not it was an apply: a fix asked
for in conversation, a refactor, a build repair, a dependency change. `temp/` is ignored by git, so a log is never
committed; create the file when it is missing.

- **`_LOG_APPLY_SUCCESS.md`** - what went right, always written: the checks that were actually run with their real
  counts or results, a deviation from the request that turned out harmless, and a fact later work can rely on
  without re-deriving it.
- **`_LOG_APPLY_ERROR.md`** - open points and inconveniences, written whenever any exist: a check that was not run
  or could not be run, a failure seen and whether it already existed without the change, a rough edge left on
  purpose, a decision waiting on the user. An entry stays until what it describes is resolved; it is never removed
  because the task is done.
- **Heading**: one `### <slug> (#<issue>, merged via PR #<n>)` per task, appended in the order tasks ran. A task
  with no issue or apply uses `### <slug> (no apply, commit <short hash>)`, and work that never landed says so
  plainly, for example `(NOT applied, no commit)`. The slug is lowercase kebab-case and names the work.
- **Never overstate**: an unverified claim goes to the error log as unverified, not to the success log as done.
  ASCII only.

## Versioning

SemVer, judged against the public API: break => major, additive => minor, no API impact => patch. A pure
structural refactor that keeps the public API byte-identical is **patch**, not major. There is no `metadata.yaml`
yet - create it, with a `build` field as a monotonic counter independent of the other three, before the first
version bump.

## Where the rules live

Detailed conventions are skills under `.claude/skills/`. Load the one that matches what you are touching; this
file keeps only the global policy.

| Working on | Skill |
|---|---|
| `.c` / `.h` | `c-style` |
| `.rs` | `rust-style` |
| `CMakeLists.txt` / `*.cmake` | `cmake-style` |
| Tests under `tests/` (GoogleTest, C++) | `cpp-testing` |
| Adding a 3rdparty library or a Cargo crate (licenses included) | `add-dependency` |
| Committing | `generate-commit` |
| Editing `AGENTS.md` or skills | `instruction-governance` |

Workflow skills under `.claude/skills/`. Each is a folder with a short `SKILL.md` and one prompt file per step
under `steps/`; run one step at a time, in order, and honour the `mode` and `effort` in the frontmatter. Run them
only when the user or a rule names them:

- `generate-issue` - issue => report.
- `generate-apply` - one self-contained APPLY under `temp/_APPLY/` that a cold session executes end to end.
- `generate-handoff` - a set of APPLYs plus the dependency graph that lets them run in parallel.
- `dispatch-applies` - several existing applies at once, one agent worktree each, this session orchestrating.
- `request-make-apply` - `<submodule path>` then `<request text>`: a generated apply executed at once in a
  worktree of the submodule; leaves it at the tip of `develop`/`main`/`master` and commits the pointer bump in the
  parent.
- `parallelize` - free text => tasks => dependency waves dispatched to subagents, in a git worktree only when
  write sets could collide.
- `reconcile-temp-logs` - prunes finished applies and merged log entries, backing them up first; never removes
  open error-log entries.
- `audit-code` - `<file>...`: the post-apply code audit; [the skill](.claude/skills/audit-code/SKILL.md) owns its
  stages and run conditions.
