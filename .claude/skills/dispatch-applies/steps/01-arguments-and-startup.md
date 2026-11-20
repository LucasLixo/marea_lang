# Step 1 of 6 - Resolve the arguments and start up

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never enter plan mode. Never stage what was already dirty.

Each argument names one apply that already exists: a bare slug (`add-cmake-folder-settings`), a
slug with the suffix (`add-cmake-folder-settings.apply.md`), or a path
(`temp/_APPLY/add-cmake-folder-settings.apply.md`) - resolve all three the same way, against
`temp/_APPLY/`. Given none, or one that does not resolve, **stop and say so**: name which slugs
were not found, and point at `temp/_LIST_HANDOFF.md`/`temp/_LIST_APPLY.md` for a reader choosing
what to run next, or [generate-apply](../../generate-apply/SKILL.md)/[generate-handoff](../../generate-handoff/SKILL.md)
for a task with no apply file yet.

#### Startup

1. `mise trust`, then `git pull --rebase`, and resolve what it reports first. If the tree is dirty
   (pre-existing drift such as a dirty submodule pointer or a generated doc that drifts on every
   build) and the pull refuses, use `git fetch` plus `git merge --ff-only origin/<branch>`
   instead. A bare `git rebase` is denied in `.claude/settings.json`.
2. `git status` is your baseline for what NOT to `git add`.
3. Read `AGENTS.md`/`CLAUDE.md`, the skills each named apply cites, `temp/_LIST_APPLY.md`,
   `temp/_LIST_HANDOFF.md`, `temp/_LOG_HANDOFF.md`, and, for every apply's slug, its existing
   entries (if any) in `temp/_LOG_APPLY_SUCCESS.md`/`temp/_LOG_APPLY_ERROR.md`.
4. Verify the **real** state before choosing anything - do not trust memory or an earlier report:
   `git branch --show-current`, `git worktree list`, `gh pr list --state open`,
   `gh issue list --state open --limit 50`. If a previous round left something unfinished - an
   open PR from a stale worktree, a worker that stopped mid-run, a root branch pushed but never
   merged into `<base>` - handle that FIRST: merge it if it is ready, or report it and stop
   before starting this round on top of it.
5. Ignore worktrees that are not yours from a previous, still-running session.

## Done when

- Every argument resolved to an existing apply file, or you stopped and named the slugs not found.
- `git status` is saved as the baseline of what not to `git add`.
- The real state was verified, and anything a previous round left unfinished is handled first.

Next: [steps/02-sanity-check-round.md](02-sanity-check-round.md)
