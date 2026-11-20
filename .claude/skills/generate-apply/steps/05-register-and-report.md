# Step 5 of 5 - Register the apply and report

Part of the [generate-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Edit `temp/_LIST_APPLY.md`, never rewrite it. It holds only the table.

Create or edit `temp/_LIST_APPLY.md`. Add one row with this apply's `mode`, `effort` and `label` copied in next to its description. The table format and the mutability rule are in [references/list-apply.md](../references/list-apply.md). The log formats are in [references/logs.md](../references/logs.md).

Report the index path written, how many parts it has, and whether they are the two-part shape
(a single simple edit) or the four-part shape with pipeline parts (setup, implement, open-pr,
merge-and-finish).

## Done when

- The row exists in `temp/_LIST_APPLY.md` with Mode, Effort and Label filled and Issue empty.
- The report names the index path, the part count, and which shape (two-part or four-part) was used.

This is the last step. Do not start another task on your own.
