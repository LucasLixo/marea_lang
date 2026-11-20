---
name: audit-code
mode: edit
effort: high
description: Use when the user or a rule asks to audit the code files given (or the branch diff) in five stages - style conformance, permission-map sync, minimal code, a memory and logic bug hunt and the formatter - committing what changes through generate-commit. It runs at the end of every apply and after any large change.
argument-hint: "<file>..."
---

# Audit Code

## Context

**Run this in edit mode.** Execute the workflow immediately; ask for confirmation only where a rule below says to stop. This skill is the canonical owner of the post-apply code audit: skill conformance, permission-map sync, minimal code, the bug hunt and formatting. Other documents link here and never restate the rules or the threshold.

Arguments: one or more code file paths. With none, audit the files the current branch changed against its base (`git diff --name-only <base>...HEAD`, where `<base>` is the branch the PR targets: the root branch of a handoff batch, else `develop`, `main` or `master`), minus `3rdparty/`, generated output and `docs/`.

### When it runs

- **After an apply**: at the end of every apply's work, before the push (step 4 of both pipeline variants in [generate-apply](../generate-apply/SKILL.md)), over the files the work changed.
- **After a large change**: after any other task that changed 5 or more source files or 200 or more lines (`git diff --stat <base>`), tests and docs excluded from the count.
- **Where it runs: always in isolated agents.** An automatic audit is never done in the session that triggered it. The triggering session dispatches it through [parallelize](../parallelize/SKILL.md) as one or more agents, each in its own git worktree (`isolation: "worktree"`) and each started cold: its prompt holds only the rules to apply (this skill, the matching style skills, `AGENTS.md`), the exact file list it owns and the checks to run, and none of the apply's reasoning or earlier conversation. That keeps the audit judging the code against the rules instead of against the story of how it was written, and spends the agent's tokens on the files.
- **How many agents.** A small set (fewer than about 10 source files in one area) is one agent. A larger set, or files in several areas, is split by area so each agent owns a disjoint file set (for example `lib/rust/marea/<area>` with its mirror `lib/rust/tests/<area>`, `lib/framework`, `tools`, the C sources of one module) and the agents run in parallel waves as [parallelize](../parallelize/SKILL.md) defines. Each agent runs the whole workflow below over its files, commits on its worktree branch only through `generate-commit`, and reports; isolation, waves and the merge of the branches are owned by [parallelize](../parallelize/SKILL.md), and the triggering session then runs the Close verification once on the merged result. A person who runs the command by hand on a few files may still run it in place.
- It never triggers itself, and a clean pass produces no commits.

## Requirements

- Startup, before anything else: `mise trust`, then `git pull --rebase` (dirty tree: `git fetch` plus `git merge --ff-only origin/<branch>`), as the repository `AGENTS.md` states. Never stage what was dirty before the audit started.
- Commit only through the `generate-commit` skill ([../generate-commit/SKILL.md](../generate-commit/SKILL.md)), one theme per commit and one commit per bug; never a hand-written `git commit`. Never push: the pipeline that called this skill owns the push.
- **Precedence**: a style skill's rule beats "fewest lines". Never strip a blank line a skill requires, an LDoc header, a copyright header, a `SAFETY` comment or a doc that a skill requires.
- **Bugs are fixed test-first**: every memory or logic bug gets a failing test, then the minimum fix. Simplification is behaviour-preserving: the covering tests are green before AND after.
- **Deletion rule**: code proven dead (no caller in the repository, confirmed by `git grep` and the build) is deleted together with the tests whose only subject it was (for Rust, in the mirror test file, never in the source). A test that also covers live code is adapted, never dropped. No test, security check, bound check or performance-relevant structure is removed to save lines.
- Never add or change a formatter, a config or a mise task here; this skill only calls the existing ones.
- Report what ran and what could not run; never claim a check that did not run. There is no `.clang-tidy` and no clippy task, so none is run.

Step map: Stage 0 is step 01, Stage 1 is step 02, Stage 1A is step 03, Stage 1B is step 04, Stage 2 is step 05, Stage 3 is step 06, Stage 4 is step 07, and Close is step 08.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-scope-and-baseline.md](steps/01-scope-and-baseline.md) | Stage 0 - Scope and baseline |
| 2 | [steps/02-conformance.md](steps/02-conformance.md) | Stage 1 - Conformance |
| 3 | [steps/03-permission-map-sync.md](steps/03-permission-map-sync.md) | Stage 1A - Permission map sync |
| 4 | [steps/04-platform-support.md](steps/04-platform-support.md) | Stage 1B - Platform support |
| 5 | [steps/05-minimal-code.md](steps/05-minimal-code.md) | Stage 2 - Minimal code |
| 6 | [steps/06-bug-hunt.md](steps/06-bug-hunt.md) | Stage 3 - Bug hunt |
| 7 | [steps/07-formatter.md](steps/07-formatter.md) | Stage 4 - Formatter |
| 8 | [steps/08-close-and-report.md](steps/08-close-and-report.md) | Close, validate and report |
