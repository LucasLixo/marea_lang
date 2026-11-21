# Step 3 of 7 - Create and set up the worktree

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- From here every action runs inside the worktree.

1. **Slug and baseline**: pick a lowercase kebab-case ASCII slug of 3 to 6 words from the
   request, as `generate-apply` does. Take a `git status` of the main checkout and keep it as the
   baseline of what was already dirty.
2. **Create the worktree** off the base branch (`develop` unless the request says otherwise):
   `git worktree add ../request-<slug> -b request/<slug> <base>`. The worktree sits next to the
   main checkout, so no ignore rule is needed. The pipeline of step 4 creates its own
   `<label>/<n>` branch inside the worktree from `request/<slug>`.
3. **Set up the worktree**: `mise trust`, then `git pull --rebase` (dirty-tree fallback:
   `git fetch` plus `git merge --ff-only origin/<base>`), then `mise run setup:git-modules`
   (it initializes `3rdparty/GoogleTest`; never a direct `git submodule update`). Check the build
   with `mise run config:cmake`. If the worktree will not build, STOP and report the exact error;
   do not improvise around it.

## Done when

- The slug is chosen and the `git status` baseline of the main checkout is saved.
- The worktree `../request-<slug>` exists on branch `request/<slug>`.
- `mise trust`, the pull, `mise run setup:git-modules` and `mise run config:cmake` ran in it.
- A worktree that does not build stopped the run with the exact error.

Next: [steps/04-deliver.md](04-deliver.md)
