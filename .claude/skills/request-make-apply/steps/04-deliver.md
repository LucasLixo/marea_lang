# Step 4 of 7 - Deliver by size

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never dispatch the work to a later session.

Deliver by size, from the worktree:

- Large: `generate-handoff`, then `dispatch-applies`.
- Medium: `generate-apply` writes `temp/_APPLY/<slug>.apply.md` plus its parts under
  `temp/_APPLY/_PART/<slug>/`, and registers it in `temp/_LIST_APPLY.md`; open the index,
  keep its `## Acceptance` and `## Verification` in mind, and run its parts in order per its
  `## How to run` to carry it to a merged PR.
- Small: the direct pipeline (issue through [generate-issue](../../generate-issue/SKILL.md), branch
  `<label>/<n>`, work with `generate-commit`, audit, push, PR, merge, close, branch cleanup), no
  apply file.

Never dispatch the work to a later session; generation and execution happen in this run.

## Done when

- Large: the handoff was generated and its applies were dispatched in dependency order.
- Medium: one apply was generated and carried to a merged PR.
- Small: the direct pipeline ran (issue, branch, commits, audit, push, PR, merge, close, cleanup).

Next: [steps/05-verify-and-sync.md](05-verify-and-sync.md)
