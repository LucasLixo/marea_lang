# Step 6 of 7 - Remove the worktree and update the base branch

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never push. Never stage anything from the `git status` baseline of step 3.

1. **Remove the worktree** once the PR is merged and step 5 is done:
   `git worktree remove ../request-<slug>`, then `git branch -D request/<slug>`. Branch cleanup of
   the `<label>/<n>` branch already happened in the pipeline.
2. **Bring the main checkout to the tip of the base branch**: `git fetch`, then
   `git switch <base>` and `git pull --rebase`, or `git merge --ff-only origin/<base>` when the
   tree is dirty. Do this only when the main checkout is on a clean tree or already on `<base>`;
   otherwise leave it and say so in the report.

## Done when

- The worktree and the `request/<slug>` branch are removed.
- The main checkout is at the tip of `<base>`, or you said why it was left alone.

Next: [steps/07-validate-and-report.md](07-validate-and-report.md)
