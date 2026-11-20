# Step 2 of 6 - Sanity-check the round

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Reject a bad round and say why. Never drop an argument silently.

Before launching anything, read every named apply's `## Files`, `## Do not touch`, and `handoff:`
header field, and reject the round (report why, do not silently drop an argument) if:

- Two arguments are both the **first apply of the same handoff** (both would race to create the
  same root issue and root branch). At most one "first of its handoff" per round.
- Two arguments belong to the **same handoff** and their own `## Files`/`## Do not touch` do not
  actually partition - the apply files should already say this, but you confirm it, since a wrong
  apply is worse than a slow round.
- An apply depends (per its `handoff:` section in `_LIST_HANDOFF.md`) on another apply that is not
  yet `[x]` and is not itself in this round. Dependencies resolve in an earlier round, never inside
  the same one - a round has no ordering between its own workers.
- An apply needs something this environment plainly lacks (docker, a display, sudo, a Windows
  toolchain) that the apply itself does not already route around. Skip it and say why; never
  guess that it will work.

Two applies from **different** handoffs, or a handoff apply next to a standalone one, are always
fine together.

## Done when

- No two applies are the first of the same handoff.
- Applies of one handoff partition their files.
- No apply depends on one that is neither `[x]` nor in this round.
- No apply needs something this environment lacks.

Next: [steps/03-launch-workers.md](03-launch-workers.md)
