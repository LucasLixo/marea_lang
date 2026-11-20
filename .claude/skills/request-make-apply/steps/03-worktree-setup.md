# Step 3 of 7 - Create and set up the worktree

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- From here every action runs inside the worktree.

1. **Slug and baseline**: pick a lowercase kebab-case ASCII slug of 3 to 6 words from the
   request, as `generate-apply` does. Take a `git status` of the parent repository
   and keep it as the baseline of what was already dirty - step 9 uses it.
2. **Create the worktree** off the submodule's current default branch:
   `git -C <submodule> worktree add .worktrees/request-<slug> -b request/<slug>`. With `-C`, git resolves the
   worktree path against `<submodule>`, so the path stays relative to it: a path that repeats
   `<submodule>` creates `<submodule>/<submodule>/.worktrees`.
   From here every action runs inside the worktree.
3. **Set up the worktree**: `mise trust`, then `git pull --rebase` (dirty-tree fallback:
   `git fetch` plus `git merge --ff-only origin/<branch>`), then `mise run setup:git-modules`
   - never a direct `git submodule update --reference`. Then move the worktree's own
   submodule pointers to the tip of their tracked branch with
   `mise run setup:git-modules-update -- --levels 1`, limited to one layer so nested
   submodules-of-submodules are left exactly where `setup:git-modules` pinned them; commit
   that pointer move on its own if anything moved, before step 4, through
   [generate-commit](../../generate-commit/SKILL.md). If the worktree will not build,
   STOP and report the exact error; do not improvise around it.

## Done when

- The slug is chosen and the parent `git status` baseline is saved.
- The worktree `<submodule>/.worktrees/request-<slug>` exists on branch `request/<slug>`.
- `mise trust`, the pull and `mise run setup:git-modules` ran. A pointer move was committed on its own through generate-commit.
- A worktree that does not build stopped the run with the exact error.

Next: [steps/04-deliver.md](04-deliver.md)
