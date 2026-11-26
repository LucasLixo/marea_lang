---
name: dispatch-applies
mode: edit
effort: high
description: Use when the user asks to run several existing applies at once, one per argument, each in its own worktree with one fresh agent per part while this session only orchestrates, merges every PR in turn, reconciles metadata.yaml when it exists and closes out a finished handoff. Run it only when the user or a rule names it.
argument-hint: "<apply-slug> [<apply-slug> ...]"
---

# Dispatch Apply Round

## Context

**Run this in edit mode.** Never enter plan mode. Execute the actions below immediately - do not
read this as documentation, do not ask for confirmation except where a rule below says to stop and
ask. Effort stays at `high`: a wrong merge or a mis-reconciled version file is expensive to
undo once pushed, the same reasoning [generate-handoff](../generate-handoff/SKILL.md) gives
for a wrong decomposition.

You are the **orchestrator** for one **round**: the set of applies named by this skill's own
arguments, dispatched together. "Round" is this skill's own word for that set - it is not the
same thing as a handoff's "batch" (one apply's own PR, [generate-apply](../generate-apply/SKILL.md)'s
vocabulary) or a handoff's "wave" ([generate-handoff](../generate-handoff/SKILL.md)'s dependency
layer); a round can mix applies from several different handoffs, or none. You never write apply
code yourself: each apply gets one worktree, and its parts up to, but never including, the LAST
part run there one after another, each in a fresh **worker** agent, so no agent carries the context
of the parts before it (commits go through the `generate-commit` skill) - the last part always merges, closes the
issue, cleans up branches and does the bookkeeping, and is always the orchestrator's, never a
worker's ([generate-apply](../generate-apply/references/apply-part-template.md) fixes that
contract). You do everything the last part would have done - merges, conflict resolution,
verification, bookkeeping, and closing a handoff out once every row is done.
Workers get the code audit through their apply's own open-pr part ([audit-code](../audit-code/SKILL.md)).

## Requirements

- Wait until every apply of the round has finished its last worker before merging anything.
- If a worker fails or stops short, start no further part of its apply and do not merge its PR - report
  why and continue with the rest of the round.
- Nothing reaches `<base>` until the handoff it belongs to is fully done, or, for a standalone
  apply, until its own PR is ready per its own Pipeline.
- **Never push to `main`.** The repository has no release workflow today; if one is added under
  `.github/workflows/`, never trigger it from here.
- Commits are made by workers through the `generate-commit` skill
  ([SKILL.md](../generate-commit/SKILL.md)), never a hand-written `git commit`. The
  orchestrator only merges: it resolves merge conflicts and, when `metadata.yaml` exists,
  reconciles it, and does not otherwise edit the code an apply produced.
- Never stage what was already dirty before you touched anything (see Startup).
- This skill never invents a plan inline - dispatching and deciding what to build are different
  jobs, and an apply is by definition a plan already settled.
- ASCII only, no emojis, in code, docs, commits, and everything you write under `temp/`.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-arguments-and-startup.md](steps/01-arguments-and-startup.md) | Resolve the arguments and start up |
| 2 | [steps/02-sanity-check-round.md](steps/02-sanity-check-round.md) | Sanity-check the round |
| 3 | [steps/03-launch-workers.md](steps/03-launch-workers.md) | Create the worktrees and run the parts, one worker each |
| 4 | [steps/04-merge-phase.md](steps/04-merge-phase.md) | Merge the PRs in turn |
| 5 | [steps/05-close-handoff.md](steps/05-close-handoff.md) | Close a handoff out |
| 6 | [steps/06-bookkeeping-and-report.md](steps/06-bookkeeping-and-report.md) | Bookkeeping, validation and final report |
