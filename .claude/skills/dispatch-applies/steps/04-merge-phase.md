# Step 4 of 6 - Merge the PRs in turn

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never merge before every worker of the round has reported.
- Never push to `main`. Never trigger a release workflow if one is ever added.
- Anything that is not an obvious parallel addition: stop and ask the user.

Never merge a PR before every worker of this round has reported - a later report might change how
an earlier one should be reconciled. Order: handoffs that already had a root before this round,
then the rest; within one handoff, by dependency order. One PR at a time:

1. `git fetch`. If the PR's branch has fallen behind its own base branch, update it first: on the
   PR's branch, `git merge <base of that PR>` and resolve.
2. `metadata.yaml` conflict (only when the file exists): take the base branch's value and reapply only this PR's own bump -
   `major` bump => major+1, minor=0, patch=0; `minor` => minor+1, patch=0; `patch` => patch+1;
   `build` = the base's build plus this PR's own commit count. A conflict in any OTHER file that is
   an obvious parallel addition (a source list, an allowlist, an include list, a list of tests) - keep
   BOTH additions. Anything else: **stop and ask the user**, do not guess at someone else's intent.
3. Before merging the NEXT PR, run the minimum verification the just-merged PR's own files call
   for (its apply's `## Verification` section is the source of truth - do not invent a heavier
   or lighter check). If it breaks, do not merge anything else in this round: report which PR
   broke it and why.
4. `gh pr merge <branch> --merge --delete-branch`; `gh issue close <n>` if it is still open; delete
   any local/remote branch that survived; `git switch` back to the branch this PR targeted, then
   `git fetch`.
5. If this PR's apply belongs to a handoff, check `temp/_LIST_HANDOFF.md`'s matching section: once
   every row is `[x]` or `[-]`, close that handoff out now (below) before moving to the next PR.

## Done when

- Each PR was merged one at a time, in the order the text gives.
- The verification of each merged PR ran before the next merge.
- A handoff whose rows are all `[x]` or `[-]` was closed out through step 05 before the next PR.

Next: [steps/05-close-handoff.md](05-close-handoff.md)
