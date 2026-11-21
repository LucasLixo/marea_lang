---
name: generate-issue
mode: edit
effort: low
description: Use when the user asks to create a GitHub issue from the matching .github/ISSUE_TEMPLATE template and write its report file. Branches, worktrees, PRs and cleanup belong to the apply and dispatch skills. Run it only when the user or a rule names it.
argument-hint: "[issue title or a short description of the work]"
---

# Issue Workflow

## Context

Create one GitHub issue from the matching issue template and record a report for it under `temp/_ISSUE/`. Nothing else: this skill creates no branch, worktree or PR and cleans up nothing. [generate-apply](../generate-apply/SKILL.md), [dispatch-applies](../dispatch-applies/SKILL.md) and [request-make-apply](../request-make-apply/SKILL.md) own the branch, the worktree, the Pull Request and the cleanup.

## Requirements

- **Run in edit mode.** Never enter plan mode, and survey no code beyond what the template fields need - filling a template and calling `gh` carries almost no decision, which is why effort stays at `low`.
- Execute the workflow immediately; ask for confirmation only where stated.
- Startup, before anything else: `mise trust`, then `git pull --rebase` (dirty tree: `git fetch` plus `git merge --ff-only origin/<branch>`). The default base branch is `develop`.
- English. At least one label; the first is primary (`bug` `feature` `documentation` `refactor` `tests` `audit`).
- `--body` is plain Markdown: real newlines and spaces, no URL-encoding.
- Populate the matching template `.github/ISSUE_TEMPLATE/<primary-label>.md`, replacing every placeholder (`<!-- ... -->`) with real details. The templates are `audit`, `bug`, `documentation`, `feature`, `refactor` and `tests`; the issue goes to the GitHub repository of `origin` through `gh`.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-gather-info.md](steps/01-gather-info.md) | Gather the information |
| 2 | [steps/02-create-issue.md](steps/02-create-issue.md) | Create the issue |
| 3 | [steps/03-write-report.md](steps/03-write-report.md) | Write the report |
