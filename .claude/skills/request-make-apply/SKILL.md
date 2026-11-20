---
name: request-make-apply
mode: edit
effort: high
description: Use when the user asks to turn a text request into delivered work inside an isolated worktree of a given submodule path, sized as large (handoff), medium (generated apply) or small (direct delivery), so the submodule's own skills and instructions govern it. Run it only when the user or a rule names it.
argument-hint: "<submodule path> <request text>"
---

# Request Make Apply

## Context

Run this in edit mode: survey, size, deliver. Do not read it as documentation and do not
ask for confirmation except where a rule below says to stop. Effort stays at `high`: the
command drives a full pipeline (issue, branch, commits, PR, merge) and a misstep after a
push is expensive to undo, the same reasoning [dispatch-applies](../dispatch-applies/SKILL.md)
gives for its merges.

It closes the loop for an engine gap of any size: instead of requesting the work from the
submodule's project and waiting, it sizes the gap and delivers it in the same run. The
session works inside a worktree of the submodule, so the submodule's `AGENTS.md`/`CLAUDE.md`
and `.agents/skills/` govern the work, not the parent repository's.
The small direct pipeline runs [audit-code](../audit-code/SKILL.md) before the push; the medium and large paths get it through their applies' pipelines.

## Requirements

- The parent repository is touched exactly once: the submodule pointer bump of step 9,
  committed through the [generate-commit](../generate-commit/SKILL.md) skill.
  Nothing else in the parent checkout is staged or edited.
- Never push to `main`/`master`, and never trigger a release workflow.
- A bare `git rebase` is denied in `.claude/settings.json`; `git pull --rebase` is fine.
- The apply's own `## Do not touch` section binds this session like any executing session.
- ASCII only, no emojis, in code, docs, commits and everything written under `temp/`.
- Never declare success without having seen a command's real output.
- Every completed task writes a success-log entry (see step 6 and
  [generate-apply](../generate-apply/SKILL.md)); open points go to the error log.
- Independent tasks in different areas go through [parallelize](../parallelize/SKILL.md).

Step map: the workflow steps of the text map to step files as follows. Workflow steps 1 to 3 are step file 03, workflow step 4 is step file 04, workflow steps 5 and 6 are step file 05, workflow steps 7 to 9 are step file 06.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-validate-arguments.md](steps/01-validate-arguments.md) | Validate the arguments |
| 2 | [steps/02-size-triage.md](steps/02-size-triage.md) | Size the gap |
| 3 | [steps/03-worktree-setup.md](steps/03-worktree-setup.md) | Create and set up the worktree |
| 4 | [steps/04-deliver.md](steps/04-deliver.md) | Deliver by size |
| 5 | [steps/05-verify-and-sync.md](steps/05-verify-and-sync.md) | Verify and sync the bookkeeping |
| 6 | [steps/06-cleanup-and-pointer.md](steps/06-cleanup-and-pointer.md) | Remove the worktree and commit the pointer |
| 7 | [steps/07-validate-and-report.md](steps/07-validate-and-report.md) | Validate and report |
