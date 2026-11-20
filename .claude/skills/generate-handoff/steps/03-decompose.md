# Step 3 of 6 - Design the decomposition and get approval

Part of the [generate-handoff](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Stay in plan mode until the user approves the shape.

Design the decomposition in plan mode and close the open questions with the user. Then leave plan mode.

Each apply is one focused session of work. Size them by that, not by file count.

**Split** when any of these holds:

- the pieces carry **different `metadata.yaml` bumps** - one PR must not claim to be both a patch and a major
- one piece is a **decision gate** whose outcome changes whether the next is worth doing
- together they would produce a diff nobody can review as a unit

**Group** when a reviewer would want to see the pieces land together - an engine symbol and its only consumer, a function and the test that covers it. A change always travels with its test and its doc; splitting those apart is never the right cut.

### Dependencies

An edge means a **real** dependency: an apply needs a symbol, a file, or a decision that another one produces. Nothing else earns an edge.

Sharing a file is not a dependency by itself. When two applies edit the same file in different regions, say so in each one's `## Do not touch` instead of serialising them - a false edge costs parallelism for nothing.

Order the applies so every edge points backwards, and derive the waves from the edges: wave 1 is everything with no dependency, wave 2 is everything whose dependencies are all in wave 1, and so on.

The root issue and root branch (see the handoff-batch pipeline of [generate-apply](../../generate-apply/SKILL.md)) are shared setup, not a code dependency, so they never appear as an edge - but they still gate every batch in practice, since none of them has anything to branch from until the first one creates the root branch. Report this plainly when announcing which applies can start now: wave 1 is parallel once the root branch exists, not from the first instant.

## Done when

- The user approved the split.
- Every apply is one focused session, travels with its test and doc, and every edge is a real dependency pointing backwards.
- The waves are derived from the edges.
- Plan mode is left.

Next: [steps/04-write-applies.md](04-write-applies.md)
