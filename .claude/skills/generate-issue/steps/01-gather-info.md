# Step 1 of 3 - Gather the information

Part of the [generate-issue](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Single message, single reply, no follow-up questions. Never enter plan mode.

Run `mise trust`, then `git pull --rebase` (dirty tree: `git fetch` plus `git merge --ff-only origin/<branch>`).

Gather in one message: the issue title; the primary label; the template-specific fields (load the template first to derive them); the branch base (`develop`, or the current branch), recorded in the report.

## Done when

- The startup rule ran.
- You have the title, the primary label, the template fields and the base.

Next: [steps/02-create-issue.md](02-create-issue.md)
