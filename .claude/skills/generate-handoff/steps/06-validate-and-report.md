# Step 6 of 6 - Validate and report

Part of the [generate-handoff](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Outcome logs of a handoff follow [references/outcome-logs.md](../references/outcome-logs.md).
- ASCII only, no emojis, in everything you write.

Report which applies can start in parallel right now. Then, only if this planning task itself leaves a confirmed
bug or something open, append this task's entry to `temp/_LOG_APPLY_ERROR.md` or
`temp/_LOG_APPLY_OPEN_POINTS.md`, as [generate-apply logs](../../generate-apply/references/logs.md) defines.

Before reporting: every apply has a row in both lists, every edge points backwards, `Parallel waves` matches the table, and each file is ASCII only.

#### Keeping the four files in step

Every apply of a handoff has a row in **both** `_LIST_APPLY.md` and its `_LIST_HANDOFF.md` section. `_LIST_APPLY.md` is the flat view of all work; the handoff section is the same rows plus their edges.

Both lists follow the same mutability rule: a row may be reworded, split, added, or cancelled, always with a short parenthetical reason pointing at the fuller account in the matching log, and **never deleted**. A table change is made in both lists in the same breath. Cancelling an apply that others depend on cancels them too, or the reason must say what replaces it. A handoff whose every apply is cancelled is cancelled itself, with the reason on the section - in `_LOG_HANDOFF.md`, since the list itself carries no prose.

## Done when

- Every apply has a row in both lists, every edge points backwards, and `Parallel waves` matches the table.
- Each file is ASCII only.
- The report says wave 1 is parallel once the root branch exists, not from the first instant.
- An error-log entry is written when a real problem was found, and an open-points entry when something is left
  open; neither when there is nothing to report.

This is the last step. Do not start another task on your own.
