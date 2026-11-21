# Step 5 of 7 - Verify and sync the bookkeeping

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

1. **Verify** every check the chosen path names and keep the real output for the report.
2. **Sync the bookkeeping back** before removing the worktree: `temp/` is ignored by git and does
   not travel with a worktree. Append the apply's row (when one exists) to the main checkout's
   `temp/_LIST_APPLY.md`, a success entry for the task to its `temp/_LOG_APPLY_SUCCESS.md` always,
   and to its `temp/_LOG_APPLY_ERROR.md` only when something is left open - same heading format
   and split as [generate-apply](../../generate-apply/references/logs.md) defines.

## Done when

- Every check the chosen path names ran, and its real output is kept for the report.
- The apply row, the success entry and, if needed, the error entry were written under the main checkout's `temp/`.

Next: [steps/06-cleanup.md](06-cleanup.md)
