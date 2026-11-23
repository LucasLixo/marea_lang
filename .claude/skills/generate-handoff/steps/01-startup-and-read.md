# Step 1 of 6 - Startup and read the request

Part of the [generate-handoff](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Plan only: no production code, no build, no commit, nothing outside `temp/`.

**Startup, before anything else**: run `mise trust`, then `git pull --rebase`, and resolve what it reports first. If the tree is dirty and the pull refuses, use `git fetch` plus `git merge --ff-only origin/<branch>` instead, and never stage what was already dirty.

Read the request. When the argument is an issue number, run `gh issue view <n>`.

## Done when

- `mise trust` and `git pull --rebase` ran, and what they reported is resolved.
- The request is understood in one sentence. If it is one focused session, this skill stopped and pointed at generate-apply.

Next: [steps/02-survey.md](02-survey.md)
