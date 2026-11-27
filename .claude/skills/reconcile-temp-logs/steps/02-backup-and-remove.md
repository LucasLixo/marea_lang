# Step 2 of 6 - Phase 0 - Back up, then remove

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never remove text that is not confirmed present in the backup.

4. Back up, then remove. If nothing was collected here, skip this step.
   a. Create `LOG_BLOCKS.md` in this run's backup folder (see the Requirements of
      [SKILL.md](../SKILL.md)), one file per run.
   b. Write a `# Apply log backup` title, one line naming the run timestamp, then the collected
      blocks verbatim under `## From _LOG_HANDOFF.md` (omit if empty). Phase 1-3 copies of
      resolved entries are appended to this same file under `## From _LOG_APPLY_ERROR.md` and
      `## From _LOG_APPLY_OPEN_POINTS.md`, so create the file when the first block of the run is
      copied, whichever phase it comes from.
   c. Read the file back and confirm every block is present, then remove the collected blocks
      from their source logs.

## Done when

- `LOG_BLOCKS.md` exists in the backup folder, was read back, and holds every collected block verbatim.
- Only then were the collected blocks removed from their source logs. If nothing was collected, no block was copied.

Next: [steps/03-inventory.md](03-inventory.md)
