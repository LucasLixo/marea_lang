# Step 5 of 6 - Integrate the wave

Part of the [parallelize](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Only the orchestrator merges. Anything that is not an obvious parallel addition: stop and ask the user.

After each wave, before the next:

1. Read every report. A failed or stopped task: do not start a wave that `needs:` it; report why
   and continue only with what does not depend on it.
2. Merge each worktree branch back (`git merge` into the feature branch you are on, one at a time;
   only you merge). Resolve conflicts that are an obvious parallel addition (a list, an include, a
   registration) by keeping both sides; anything else: **stop and ask the user**, do not guess at
   intent.
3. Re-run the cheapest verification that covers the merged result (the tests named in the reports, for instance `ctest --test-dir cmake-build-debug -R <suite>` after building `marea_tests`,
   not a full rebuild unless the repository's rules demand one). A combined tree can break in
   ways no single task did; if it does, fix the seam yourself or send a follow-up agent - do not
   start the next wave on a broken tree.
4. Remove worktrees and branches you created once merged (`git worktree remove`, `git branch -d`);
   leave alone any worktree that is not yours.

## Done when

- Every report was read. A failed task blocked the waves that need it.
- Each worktree branch is merged, one at a time.
- The cheapest verification that covers the merged result ran, with real output.
- Your worktrees and branches are removed. Others are untouched.
- If tasks remain, go back to step 04 for the next wave.

Next: [steps/06-finish.md](06-finish.md)
