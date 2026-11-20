# Step 2 of 7 - Size the gap

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- When two sizes fit, take the smaller.

Size the gap by how many distinct APIs it touches, before step 1, and say the verdict in
one line. Setup (steps 1 to 3) and closing (steps 6 to 9) are the same for every size;
only step 4 changes, and it runs after setup, inside the worktree:

- **Large** - many modifications to distinct APIs toward a single purpose: run
  [generate-handoff](../../generate-handoff/SKILL.md) with the request, then execute the
  resulting applies through [dispatch-applies](../../dispatch-applies/SKILL.md), in
  dependency order, from the worktree.
- **Medium** - modifications involving the same or similar APIs: generate one apply with
  [generate-apply](../../generate-apply/SKILL.md) and carry it to a merged PR.
- **Small** - fits in a few commits, even across distinct APIs: skip the apply file and
  deliver directly in the worktree, using the [generate-commit](../../generate-commit/SKILL.md)
  skill at every compilable increment, through the same issue, branch, PR and merge pipeline.

When two sizes fit, take the smaller. When the gap is unclear, follow
`generate-apply`'s own escalation to plan mode before sizing.

Parallel worktrees are allowed: independent applies may each get
`<submodule>/.worktrees/request-<slug>` and run at once, orchestrated exactly as
[dispatch-applies](../../dispatch-applies/SKILL.md) defines (workers stop at an opened
PR; this session merges in turn, reconciles `metadata.yaml`, closes out the handoff). Only
applies with no dependency edge run together. Do not restate that command - follow it.

## Done when

- The verdict (large, medium or small) is said in one line.
- An unclear gap went through plan mode first, or you stopped and said what is missing.

Next: [steps/03-worktree-setup.md](03-worktree-setup.md)
