# Step 5 of 7 - Verify and sync the bookkeeping

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

1. **Verify** every check the chosen path names and keep the real output for the report.
2. **Sync the bookkeeping back** before removing the worktree: `temp/` is ignored by git and does
   not travel with a worktree. Append the apply's row (when one exists) to the main checkout's
   `temp/_LIST_APPLY.md`, an error entry to its `temp/_LOG_APPLY_ERROR.md` when a confirmed bug or
   broken check was seen, and an open-points entry to its `temp/_LOG_APPLY_OPEN_POINTS.md` when
   anything else is left open - same heading format and split as
   [generate-apply](../../generate-apply/references/logs.md) defines. Neither when nothing
   qualifies.

## Done when

- Every check the chosen path names ran, and its real output is kept for the report.
- The apply row was written, and the error entry and/or open-points entry when something qualifies, under the main checkout's `temp/`.

Next: [steps/06-cleanup.md](06-cleanup.md)
