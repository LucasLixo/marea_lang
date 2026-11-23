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

- Allowed Git operations only: `git status`, `git branch --show-current`, `git diff`, `git log`, `git add`, `git commit`. Never push, amend, rebase, reset, or create a branch or tag.
- If an argument was given, restrict everything below to the paths it names.
- Never stage what was already dirty and unrelated to the request.
- Messages: ASCII only, English, never the `type: description` format. A full sentence (or a few), first letter capitalized, describing what changed and why rather than what the file is.
- When the issue number `N` is set, close every message with ` #N` (no trailing period before it); never invent a number.
- Attribution follows the repository `AGENTS.md` and the session instructions; do not add or strip trailers on your own initiative.

## Grouping

One theme per commit. A single commit carrying unrelated changes is a bad commit - split it. Two changes share a theme only when they serve the same purpose; touching the same file does not make them one, and spanning several files does not make them two. A change travels with the test or doc that covers it.

Stage with `git add <specific-path>`; `git add .` only when every remaining file belongs to that theme.

## Versioning

No `metadata.yaml` exists yet, so today this section does not apply and versioning is skipped. [AGENTS.md](../../../AGENTS.md) ("Versioning") says to create it, with a `build` field, before the first version bump; that creation is its own commit. Once the file exists (`version: major.minor.patch.build`), keeping it in step is part of this skill: edit it as a normal file, staged into the commit it describes.

- **build**: `+1` on every committed change set, independent of the other fields - three commits bump it three times.
- **major** breaks the public API (the public headers under `lib/include/`), **minor** adds to it, **patch** leaves it byte-identical (a pure structural refactor is patch).
- Bump `major`/`minor`/`patch` at most once per run, on the commit that causes it, and say so in the report. When one change set mixes impacts, take the highest-impact one (major over minor over patch), consistent with the SemVer rule in `AGENTS.md`.
- If the repository has no `metadata.yaml`, skip versioning entirely.

## Workflow

1. `git status --porcelain=v1`, plus `git diff` on anything unclear - the message must describe the real change.
2. `git branch --show-current` -> `N` is the first run of digits in it: `refactor/94` -> `94`, `tests-11` -> `11`, `feature/7-foo` -> `7`, `develop` -> unset.
3. For each theme, in order: update `metadata.yaml` if it exists, `git add` the theme's paths (plus `metadata.yaml` when it changed), then commit.
4. Report `git log --oneline -<count>` and `git status` for whatever was deliberately left uncommitted.

## Commit command form

Pass the message through a quoted heredoc: `<<'EOF'` stops the shell from expanding `$`, backticks and `!`, so identifiers and apostrophes survive verbatim. The closing `EOF` and `)"` each stay on their own line; a blank line separates subject from body.

```bash
git add lib/include/marea/base/MAREADiagnostic.h tests/source/marea/base/MAREADiagnostic.cpp
git commit -m "$(cat <<'EOF'
Add an accessor for the severity of a diagnostic and cover it with a test #94

A consumer of the opaque diagnostic could not read the severity without opening the
struct. The accessor keeps the struct closed and the test proves it is enough.
EOF
)"
```

This is POSIX shell (Linux, macOS, Windows through Git Bash or WSL). Only when committing by hand from PowerShell, use the `git commit -m @'` ... `'@` here-string.

## Validation

- Every commit has one theme, ends with ` #N` when `N` is set, and is ASCII.
- When `metadata.yaml` exists: build went up by exactly one per commit, and major/minor/patch moved at most once, to the highest impact.
- Nothing was pushed; `git status` shows only what was left out on purpose.

## References

- [AGENTS.md](../../../AGENTS.md) - versioning and attribution policy.
