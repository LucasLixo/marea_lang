---
name: marea-lang-agents
description: Global policy for agents working in the Marea Lang repository - scope, commits, TDD, ASCII, versioning, outcome logs - with detail delegated to the owning skills.
---

# Marea Lang

A statically-typed, high-performance scripting language of its own, for general use together with C. It is written in
C11 on a modern CMake setup, with explicit fixed-width types, `structure`, `enum` and explicit
function types checked once at compile time and fully erased before code generation, compiled from one IR to bytecode
for a reference interpreter or to C11, with scoped memory and static ownership rules, and one C macro that registers
a function for the language runtime, with no external generator and no hand-written glue on any side.
The project is independent: no tie to any other engine, language, or repository, with its own name, license, and
structure.

Comments and session chat according to the ASD-STE100 standard.

## Specifications

The compiler and language specifications are not restated here: they live under `docs/`, one
reference per area, each kept in sync with the shipped code in the same change that changes its
behavior ([stage-docs](.claude/skills/stage-docs/SKILL.md) owns the per-stage pattern). The example
programs are in [examples](examples). In pipeline order:

| Doc | Covers |
| --- | --- |
| [docs/grammar/marea.ebnf](docs/grammar/marea.ebnf) | The formal syntax grammar and the language's own main rules (statement and body endings, braces, identifier case) |
| [docs/lex/marea-lex.md](docs/lex/marea-lex.md) | The `lex` stage |
| [docs/ast/marea-ast.md](docs/ast/marea-ast.md) | The `parse` stage |
| [docs/sema/marea-sema.md](docs/sema/marea-sema.md) | The `sema` stage: typing, ownership and borrow checking, the library registry and the C registration macro |
| [docs/ir/marea-ir.md](docs/ir/marea-ir.md) | The `ir` stage |
| [docs/bytecode/marea-bytecode.md](docs/bytecode/marea-bytecode.md) | The `bytecode` stage |
| [docs/vm/marea-vm.md](docs/vm/marea-vm.md) | The `vm` stage: the bytecode reference interpreter |
| [docs/cgen/marea-cgen.md](docs/cgen/marea-cgen.md) | The `cgen` stage: the C11 backend |
| [docs/tools/marea-cc-and-bc.md](docs/tools/marea-cc-and-bc.md) | The `marea-cc` and `marea-bc` command-line tools |
| [docs/language/marea-language.md](docs/language/marea-language.md) | The language catalogue: what is accepted, what is rejected and why |

**C++ exists only for tests** (GoogleTest, see [cpp-testing](.claude/skills/cpp-testing/SKILL.md)). Production
code is C11, never C++.

## Working model

- **The main branch is `develop`.** It is the default branch; every branch starts from it and every PR targets it.
  There is no `main` branch.
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

## Context economy

Every turn reads the whole context again, so keep it small, in the main session and in every subagent.

- Read code by symbol, not by file. Find the line with `grep -n`, then use the `marea-docs` MCP (`search_symbols` and
  `get_symbol` for headers, `get_function_body` for a `.c` function; load them with ToolSearch) or a Read with `offset`
  and `limit`. Do not `cat` a source file.
- `marea-docs` reads the checkout that started the session. In another worktree, use `grep -n` and a ranged Read.
- Build and test only with `mise run dev:quiet-build` and `mise run dev:quiet-test`
  ([cpp-testing](.claude/skills/cpp-testing/SKILL.md), "Running tests"). For any other long command, send the output to
  a file under `tmp/` and print only the lines that decide the result.
- Do not read a whole outcome log: `grep -n '^### '` it, then Read the range you need.

## Reporting the outcome of any task

Every task that leaves a confirmed bug or an open point behind ends with a record in the two logs
[generate-apply](.claude/skills/generate-apply/references/logs.md) defines, `temp/_LOG_APPLY_ERROR.md` and
`temp/_LOG_APPLY_OPEN_POINTS.md`, whether or not it was an apply: a fix asked for in conversation, a refactor, a
build repair, a dependency change. `temp/` is ignored by git, so a log is never committed; create the file when
it is missing. A task that leaves nothing wrong and nothing open writes to neither.

- **`_LOG_APPLY_ERROR.md`** - confirmed bugs, broken checks and real failures seen during the task only, and
  whether each already existed without the change.
- **`_LOG_APPLY_OPEN_POINTS.md`** - everything else by design: a deferred feature, a known gap, a decision
  waiting on the user, a harmless deviation, a check that was not run or could not be run. An entry stays until
  what it describes is resolved; it is never removed because the task is done.
- A new apply, before it is written, checks both logs for an existing entry relevant to what it is about to
  build, so it can pick up or build on a still-open point instead of re-discovering it.
- Finishing an apply - its own closing or merge step - that resolves something an earlier task logged removes
  that entry's mention from `_LOG_APPLY_OPEN_POINTS.md`/`_LOG_APPLY_ERROR.md` as part of what finishing means.
  This is not a new exception to "never removed because the task is done": a mention goes away only once the
  thing it describes is actually resolved, the same retention rule, now stated as something the closing part of
  an apply does rather than something that happens by chance.
- **Heading**: one `### <slug> (#<issue>, merged via PR #<n>)` per task, appended in the order tasks ran. A task
  with no issue or apply uses `### <slug> (no apply, commit <short hash>)`, and work that never landed says so
  plainly, for example `(NOT applied, no commit)`. The slug is lowercase kebab-case and names the work.
- **Never overstate**: an unverified claim goes to the error log (a real failure) or the open-points log
  (anything else still open) as unverified or open, never claimed as resolved anywhere - there is no success log
  to wrongly mark it done in. ASCII only.

## Versioning

SemVer, judged against the public API: break => major, additive => minor, no API impact => patch. A pure
structural refactor that keeps the public API byte-identical is **patch**, not major. The version lives in
[metadata.yaml](metadata.yaml); its `build` field is a monotonic counter independent of the other three, bumped once
per commit by `generate-commit`.

## Where the rules live

Detailed conventions are skills under `.claude/skills/`. Load the one that matches what you are touching; this
file keeps only the global policy.

| Working on | Skill |
|---|---|
| `.c` / `.h` | `c-style` |
| `CMakeLists.txt` / `*.cmake` | `cmake-style` |
| Tests under `tests/` (GoogleTest, C++) | `cpp-testing` |
| Adding a 3rdparty library (licenses included) | `add-dependency` |
| `docs/<stage>/*.md` | `stage-docs` |
| Committing | `generate-commit` |
| Editing `AGENTS.md` or skills | `instruction-governance` |

Workflow skills under `.claude/skills/`. Each is a folder with a short `SKILL.md` and one prompt file per step
under `steps/`; run one step at a time, in order, and honour the `mode` and `effort` in the frontmatter. Run them
only when the user or a rule names them:

- `generate-issue` - issue => report.
- `generate-apply` - one self-contained APPLY under `temp/_APPLY/` that a cold session executes end to end.
- `generate-handoff` - a set of APPLYs plus the dependency graph that lets them run in parallel.
- `dispatch-applies` - several existing applies at once, one worktree each and one fresh agent per part, this session orchestrating.
- `request-make-apply` - `<request text>`: sizes the request, then delivers it at once in an isolated git worktree of
  this repository, carried to a merged PR in the same run.
- `parallelize` - free text => tasks => dependency waves dispatched to subagents, in a git worktree only when
  write sets could collide.
- `reconcile-temp-logs` - moves finished applies and closed handoff sections to `temp/_BACKUP/`, deleting
  nothing; never removes an open error-log or open-points-log entry.
- `audit-code` - `<file>...`: the post-apply code audit; [the skill](.claude/skills/audit-code/SKILL.md) owns its
  stages and run conditions.
