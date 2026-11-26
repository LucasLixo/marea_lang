---
name: generate-commit
mode: edit
effort: medium
description: Split the working tree into focused, atomic local commits, bumping metadata.yaml and carrying the issue number from the branch name; never pushes. Triggers automatically for any prompt that contains the word "commit" or asks for local commit work (commit my changes, make commits, group and commit, save this as commits).
argument-hint: "[optional path or scope to restrict the commits to]"
---

# Generate Commit

## Context and Objective

Group the working-tree changes by theme and create the commits locally. This skill is the only way to commit in this repository (see `AGENTS.md`): never a hand-written `git commit`, from the main session or from a subagent. The attribution policy (trailers, footers) is whatever the repository `AGENTS.md` and the system prompt of the session say; this skill adds no rule of its own about it.

Run in edit mode, never in plan mode. Read nothing beyond the `git` output named below - the diffs are the whole input. Effort stays at `medium`: grouping themes and choosing the `metadata.yaml` bump need judgement, the rest is mechanical.

## Strict Requirements

- Allowed operations only: `git status`, `git branch --show-current`, `git diff`, `git log`, and the `commit` command of [marea-ship](../../../tools/marea-ship/main.lua), which stages and commits. Never type `git add` plus `git commit` by hand. Never push, amend, rebase, reset, or create a branch or tag.
- If an argument was given, restrict everything below to the paths it names.
- Never stage what was already dirty and unrelated to the request.
- Messages: ASCII only, English, never the `type: description` format. A full sentence (or a few), first letter capitalized, describing what changed and why rather than what the file is.
- When the issue number `N` is set, close every message with ` #N` (no trailing period before it); never invent a number.
- Attribution follows the repository `AGENTS.md` and the session instructions; do not add or strip trailers on your own initiative.

## Grouping

One theme per commit. A single commit carrying unrelated changes is a bad commit - split it. Two changes share a theme only when they serve the same purpose; touching the same file does not make them one, and spanning several files does not make them two. A change travels with the test or doc that covers it.

Name the paths of one theme in each `commit` call. The command commits exactly those paths and leaves everything else as it was, staged or not.

## Versioning

[metadata.yaml](../../../metadata.yaml) holds `major`, `minor`, `patch` and `build`. Never edit these fields by hand: `commit --bump <level>` writes them into the commit it makes. You choose the level:

- **build** (default): `build` goes up by one. Every level does this, so three commits move it three times.
- **major** breaks the public API (the public headers under `lib/include/`), **minor** adds to it, **patch** leaves it byte-identical (a pure structural refactor is patch). The SemVer rule is in [AGENTS.md](../../../AGENTS.md) ("Versioning").
- Use `major`, `minor` or `patch` at most once per run, on the commit that causes it, and say so in the report. When one change set mixes impacts, take the highest one (major over minor over patch).
- If the repository has no `metadata.yaml`, the command skips versioning.

## Workflow

1. `git status --porcelain=v1` and `git diff --stat`. Read the full diff only of a file whose change you do not already know - the message must describe the real change.
2. For each theme, in order, make one `commit` call. It reads the issue number from the branch name (the first run of digits: `refactor/94` -> `94`, `develop` -> none) and closes the subject line with ` #N`.
3. Report the line each call printed, and `git status --short` for whatever was deliberately left uncommitted.

## Commit command form

Run from the root of the checkout or worktree that holds the change. The message comes from a quoted heredoc: `<<'EOF'` stops the shell from expanding `$`, backticks and `!`. A blank line separates subject from body.

```bash
lua tools/marea-ship/main.lua commit --bump build lib/include/marea/base/MAREADiagnostic.h tests/source/marea/base/MAREADiagnostic.cpp <<'EOF'
Add an accessor for the severity of a diagnostic and cover it with a test

A consumer of the opaque diagnostic could not read the severity without opening the
struct. The accessor keeps the struct closed and the test proves it is enough.
EOF
```

The command refuses a message that is empty, not ASCII or in the `type: description` format, and a `metadata.yaml` whose version somebody already edited. A refused commit changes nothing in `metadata.yaml`. If the call itself is denied or the tool is missing, stop and report it.

## Delegation

A main session with a large context may hand a whole run to the [shipper](../../agents/shipper.md) agent, which runs on a small model. The prompt names the paths, says in one or two sentences what changed and why, and gives the bump level. A session that is already a subagent runs the skill itself.

## Validation

- Every commit has one theme, ends with ` #N` when the branch has a number, and is ASCII.
- `build` went up by exactly one per commit, and major/minor/patch moved at most once, to the highest impact.
- Nothing was pushed; `git status` shows only what was left out on purpose.

## References

- [AGENTS.md](../../../AGENTS.md) - versioning and attribution policy.
