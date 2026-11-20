# Step 4 of 6 - Dispatch the wave

Part of the [parallelize](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Send all Agent calls of the wave in the same turn. Never poll in a loop.

Check `nproc` and free memory before sizing a wave: a wave of build-heavy tasks and a wave of
doc edits do not cost the same. A reasonable default is **up to 3 concurrent build-heavy agents,
up to 6 light ones**; if a wave is larger, split it into sub-batches in order, and never launch
more agents than tasks.

For each wave, send all of its Agent calls **in the same turn** (that is what makes them run in
parallel), then wait for every report before building the next wave. Never poll in a loop; a
background agent's completion notifies you. Pick the agent type per task: a read-only search or
audit task takes `Explore`; a design question takes `Plan`; any task that edits takes
`general-purpose`.

Every subagent prompt is complete on its own and carries:

0. First line of every prompt: the startup rule - `mise trust`, then `git pull --rebase` (in a
   dirty tree or a worktree that refuses it: `git fetch` plus `git merge --ff-only origin/<branch>`).
   You run it yourself before Step 1.
1. The overall request in one sentence, then ITS task: goal, acceptance, how to verify.
2. The exact files it owns (may write) and the ones it must not touch, naming the other tasks of
   its wave so it knows what is being edited beside it.
3. For a task with `needs:`, the relevant results of those tasks pasted in (paths, signatures,
   decisions) - not "see the earlier agent".
4. Absolute paths to `AGENTS.md` and the skills that apply, and the rule that the repository's
   conventions (TDD where it is mandatory, ASCII only, the file copyright header, language style
   skills) govern its work.
5. Do not push, do not edit `temp/` (several agents at once would corrupt it). Commit only if this
   task is explicitly the committing one, and then only through the `generate-commit` skill.
6. A **short final report** in a fixed shape: files changed, what was done, each verification run
   with its real output counts, anything it could not do or deviated from, and whatever the next
   wave must know. Never claim a result it did not see in real command output.

## Done when

- `nproc` and free memory were checked and the wave was sized.
- Each subagent prompt carries all seven items of this step.
- Every report of the wave is in.

Next: [steps/05-integrate.md](05-integrate.md)
