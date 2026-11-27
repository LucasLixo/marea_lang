---
name: reconcile-temp-logs
mode: edit
effort: low
description: Use when the user asks to reconcile temp/ bookkeeping - move apply files already marked done into a backup folder, prune closed handoff sections with a verbatim copy there first, and verify every open error-log and open-points-log entry. Run it only when the user or a rule names it.
argument-hint: ""
---

# Reconcile Temp Logs

## Context

`temp/` is gitignored. Once an apply is done, its `.apply.md` file is no longer active: the merged PR
and its issue are the permanent record, and the file moves out of the way. Closed handoff sections
age out the same way. `temp/_LOG_APPLY_ERROR.md` and `temp/_LOG_APPLY_OPEN_POINTS.md`
([generate-apply logs](../generate-apply/references/logs.md)) do not age out on that trigger: an
entry stays until what it describes is proven resolved. This skill replaces the former
`cleanup-temp` and keeps its safe behaviors, then adds a verification pass over those two logs.

`temp/_LOG_APPLY.md` is a frozen pre-split archive and is out of scope. `temp/_LOG_APPLY_SUCCESS.md`,
`temp/_LOG_HANDOFF_SUCCESS.md` and `temp/_LOG_HANDOFF_ERROR.md` are retired logs that no skill writes
to any more (per [generate-apply logs](../generate-apply/references/logs.md) and
[generate-handoff's outcome-logs reference](../generate-handoff/references/outcome-logs.md)): this
skill never reads, prunes or edits them. Nothing under `temp/_BACKUP/`, and no older
`temp/_LOG_APPLY_{DATE}_{TIME}.md` backup, is read, edited, pruned, or deleted by a later run.

## Requirements

- Startup, before anything else: run `mise trust`, then `git pull --rebase`, and resolve what it
  reports first. If the tree is dirty and the pull refuses, use `git fetch` plus
  `git merge --ff-only origin/<branch>` instead, and never stage what was already dirty.
- Delete nothing. A file or folder is moved, and a log block is copied before it leaves its log, always
  into this run's backup folder `temp/_BACKUP/{DATE}_{TIME}/`.
- The backup folder: take the timestamp once with `date +%Y-%m-%d_%H%M%S` and create the folder at the
  first move or copy of the run (if the name exists, wait a second and take a fresh timestamp). A run
  with nothing to back up creates none.
- A moved file keeps its path below `temp/`: `temp/_APPLY/<slug>.apply.md` goes to
  `temp/_BACKUP/{DATE}_{TIME}/_APPLY/<slug>.apply.md`. Move with `mv -n` and check that the source is
  gone and the destination exists.
- Copy every log block verbatim (heading included, no rewording) into `LOG_BLOCKS.md` of that folder
  before removing it. Never remove text that is not confirmed present there.
- Never remove an error-log or open-points-log entry unless verification proves it is resolved. An
  unverified or still-open entry stays.
- Commit only through the `generate-commit` skill ([../generate-commit/SKILL.md](../generate-commit/SKILL.md)); never a hand-written `git commit`.
- Verification tasks are read-only and go through [parallelize](../parallelize/SKILL.md), at
  most 3 tasks per wave, one task per error heading.
- Do not touch any row's checkbox, description, or issue number in `temp/_LIST_APPLY.md` or
  `temp/_LIST_HANDOFF.md`, anything in the retired logs named above, or anything outside `temp/`
  except a small gap fix (Phase 3).
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
| 3 | [steps/03-inventory.md](steps/03-inventory.md) | Phase 1 - Inventory the error and open-points logs |
| 4 | [steps/04-verify.md](steps/04-verify.md) | Phase 2 - Read-only verification |
| 5 | [steps/05-act.md](steps/05-act.md) | Phase 3 - Act on the classification |
| 6 | [steps/06-validate-and-report.md](steps/06-validate-and-report.md) | Validate and report |
