# Step 1 of 6 - Phase 0 - Collect what can be pruned

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- No log block is removed in this step. Do not touch any row's checkbox, description or issue number.

1. Read `temp/_LIST_APPLY.md` in full. For every row marked `[x]`:
   a. If the row has no `Mode`/`Effort`/`Label` cells and `temp/_APPLY/<slug>.apply.md` still exists,
      read that file's header and backfill the cells first; this is the row's only remaining record.
   b. Move `temp/_APPLY/<slug>.apply.md` if it exists, and `temp/_APPLY/_PART/<slug>/` (its parts
      folder) if it exists, into this run's backup folder. The row itself stays unchanged.
   c. Leave every `[ ]` and `[-]` row's file and parts folder untouched; a cancelled apply's
      reasoning may live only there and in the logs.
2. Read `temp/_LIST_HANDOFF.md` in full. A `##` section whose rows are all `[x]` or `[-]` is a closed
   handoff. Confirm each of its rows either had its file moved by step 1 or never had one.
3. Read `temp/_LOG_HANDOFF.md` in full and collect (do not remove yet) every `##` section for a
   closed handoff. There is no success log and no handoff outcome log to collect entries from any
   more: a closed handoff's own outcome now lives in the closing apply's
   `temp/_LOG_APPLY_ERROR.md`/`temp/_LOG_APPLY_OPEN_POINTS.md` entries, which Phase 1-3 (steps 03
   to 05) handles by verification, not this step.

## Done when

- Every `[x]` row's apply file and parts folder were moved to the backup folder, after its Mode, Effort and Label cells were backfilled when missing.
- The closed handoff sections to prune are collected, and no log block is removed yet.

Next: [steps/02-backup-and-remove.md](02-backup-and-remove.md)
