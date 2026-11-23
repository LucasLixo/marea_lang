# Step 2 of 6 - Phase 0 - Back up, then remove

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never remove text that is not confirmed present in the backup.

4. Back up, then remove. If nothing was collected here, skip the backup.
   a. Take the timestamp once with `date +%Y-%m-%d_%H%M%S` and create
      `temp/_LOG_APPLY_{DATE}_{TIME}.md` (one file per run; if the name exists, wait a second and
      take a fresh timestamp).
   b. Write a `# Apply log backup` title, one line naming the run timestamp, then the collected
      blocks verbatim under `## From _LOG_HANDOFF.md`, `## From _LOG_APPLY_SUCCESS.md`,
      `## From _LOG_HANDOFF_SUCCESS.md` (omit an empty group). Phase 1-3 backups of error entries
      are appended to this same file under `## From _LOG_APPLY_ERROR.md` and
      `## From _LOG_HANDOFF_ERROR.md`, so create the file when the first block of the run is
      backed up, whichever phase it comes from.
   c. Read the backup back and confirm every block is present, then remove the collected blocks
      from their source logs.

## Done when

- The backup file exists, was read back, and holds every collected block verbatim.
- Only then were the collected blocks removed from their source logs. If nothing was collected, no backup was made.

Next: [steps/03-inventory.md](03-inventory.md)
