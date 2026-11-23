---
name: reconcile-temp-logs
mode: edit
effort: low
description: Use when the user asks to reconcile temp/ bookkeeping - delete apply files already marked done, prune closed handoff sections and merged success-log entries with a verbatim backup first, and verify every open error-log entry. Run it only when the user or a rule names it.
argument-hint: ""
---

# Reconcile Temp Logs

## Context

`temp/` is gitignored. Once an apply is done, its `.apply.md` file has no further use: the merged PR
and its issue are the permanent record. Success-log prose for merged work and closed handoff sections
age out the same way. Error-log entries do not age out: an entry stays until what it describes is
proven resolved. This skill replaces the former `cleanup-temp` and keeps its safe behaviors, then
adds a verification pass over the two error logs.

`temp/_LOG_APPLY.md` is a frozen pre-split archive and is out of scope. Any existing
`temp/_LOG_APPLY_{DATE}_{TIME}.md` backup is never read, edited, pruned, or deleted by a later run.

## Requirements

- Startup, before anything else: run `mise trust`, then `git pull --rebase`, and resolve what it
  reports first. If the tree is dirty and the pull refuses, use `git fetch` plus
  `git merge --ff-only origin/<branch>` instead, and never stage what was already dirty.
- Back up every block verbatim (heading included, no rewording) to a timestamped file before removing
  it, from any log. Never remove text that is not confirmed present in the backup.
- Never remove an error-log entry unless verification proves the error no longer exists. An
  unverified or still-open entry stays.
- Commit only through the `generate-commit` skill ([../generate-commit/SKILL.md](../generate-commit/SKILL.md)); never a hand-written `git commit`.
- Verification tasks are read-only and go through [parallelize](../parallelize/SKILL.md), at
  most 3 tasks per wave, one task per error heading.
- Do not touch any row's checkbox, description, or issue number in `temp/_LIST_APPLY.md` or
  `temp/_LIST_HANDOFF.md`, the `## Notes` section of `_LOG_APPLY_SUCCESS.md`, or anything outside
  `temp/` except a small gap fix (Phase 3).
- ASCII only.

Step map: steps 01 and 02 are Phase 0, step 03 is Phase 1, step 04 is Phase 2 and step 05 is Phase 3. Step 06 validates and reports.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-collect-prunable.md](steps/01-collect-prunable.md) | Phase 0 - Collect what can be pruned |
| 2 | [steps/02-backup-and-remove.md](steps/02-backup-and-remove.md) | Phase 0 - Back up, then remove |
| 3 | [steps/03-inventory.md](steps/03-inventory.md) | Phase 1 - Inventory the error logs |
| 4 | [steps/04-verify.md](steps/04-verify.md) | Phase 2 - Read-only verification |
| 5 | [steps/05-act.md](steps/05-act.md) | Phase 3 - Act on the classification |
| 6 | [steps/06-validate-and-report.md](steps/06-validate-and-report.md) | Validate and report |
