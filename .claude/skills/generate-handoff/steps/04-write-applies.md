# Step 4 of 6 - Write the applies

Part of the [generate-handoff](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Follow [generate-apply](../../generate-apply/SKILL.md): its steps 02, 03 and 04 own the format, the naming rule and the parts (index plus `_PART/<slug>/` folder).

Write each apply under `temp/_APPLY/<slug>.apply.md` plus its parts under `temp/_APPLY/_PART/<slug>/`,
following the generate-apply skill for the file format, the naming rule and the part breakdown.
Always use the **handoff-batch variant**, since every apply here carries `handoff:`. Then add one
row per apply to `temp/_LIST_APPLY.md`.

[generate-apply](../../generate-apply/SKILL.md) owns the apply format. **Do not restate its template here or invent a variant** - one owner is what keeps the two commands from drifting apart.

Every apply written by this skill carries `handoff: <the exact section title>` in its header. That field is what lets a cold session find its way back to `_LIST_HANDOFF.md` and `_LOG_HANDOFF.md`, and it is what tells `generate-apply`'s part template which variant applies - leaving it empty silently breaks the last part's merge-and-finish/bookkeeping content and points the batch at the standalone variant instead of the shared root branch.

## Done when

- Every apply's index file exists and carries `handoff: <the exact section title>`.
- Every apply has the handoff-batch four-part shape (`setup`, `implement`, `open-pr`, `merge-and-finish`) under `temp/_APPLY/_PART/<slug>/`.
- Every apply has a row in `temp/_LIST_APPLY.md`.
- No apply restates the generate-apply template under another form.

Next: [steps/05-write-handoff-files.md](05-write-handoff-files.md)
