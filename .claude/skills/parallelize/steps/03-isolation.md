# Step 3 of 6 - Choose the isolation

Part of the [parallelize](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

Per task, choose the cheapest isolation that is safe:

- **No worktree (default)**: read-only research, review, analysis, or a task whose write set is
  disjoint from every other task in its wave. Subagents share the working tree; the disjoint
  write sets are what keep it safe, so state the owned files in the prompt and forbid touching
  anything else.
- **Git worktree** (`isolation: "worktree"`): use it when two parallel tasks must edit the same
  file or overlapping areas, when a task runs a build or test that rewrites generated files, when
  a task is risky or exploratory and its result may be discarded, or when the working tree is
  dirty with drift the task could trip over. A worktree task's result is a branch you merge back
  in Step 5.
- **A worktree is never required.** Do not pay its cost (submodule init, a fresh build tree) for
  a task that does not need it. In a repository that builds heavily, prefer fewer worktrees and
  more waves over many cold builds at once.
- In a fresh worktree, run the repository's own setup first: `mise trust`, then
  `mise run setup:git-modules` (never a hand-rolled `git submodule update`). If it will not build,
  the subagent stops and reports the exact error instead of improvising.

## Done when

- Each task has the cheapest safe isolation chosen.
- A worktree task has its setup (`mise trust`, `mise run setup:git-modules`) in its prompt.

Next: [steps/04-dispatch.md](04-dispatch.md)
