# Step 6 of 7 - Remove the worktree and commit the pointer

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- The parent is touched exactly once: the pointer bump, through generate-commit. The skill never pushes.

7. **Remove the worktree**: `git -C <submodule> worktree remove .worktrees/request-<slug>`
   once the PR is merged and step 6 is done. Branch cleanup already happened in the pipeline.
8. **Leave the submodule at the tip of its base branch**: `git -C <submodule> fetch`, then
   fast-forward `develop` when it exists, otherwise `main` or `master` (none of the three:
   stop and report) with `git switch <base>` then `git pull --rebase`, or
   `git merge --ff-only origin/<base>` when the tree is dirty.
9. **Commit the pointer bump in the parent**: back in the invocation directory, stage only
   the submodule path and commit through the [generate-commit](../../generate-commit/SKILL.md)
   skill - a single-concern commit, never bundled, never alongside paths dirty in the step 1
   baseline. When the pointer did not move, say so and skip the commit. The skill never
   pushes; whether the parent pushes is the user's call.

## Done when

- The worktree is removed.
- The submodule is at the tip of its base branch.
- The pointer commit exists, staged alone, with nothing from the baseline. Or you said that the pointer did not move.

Next: [steps/07-validate-and-report.md](07-validate-and-report.md)
