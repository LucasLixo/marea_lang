# Step 6 of 6 - Validate and report

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

- Verify the backup file with `lua tools/marea-ascii-check/main.lua temp/_LOG_APPLY_{DATE}_{TIME}.md`.
- Every removed error entry has a verbatim copy in the backup and recorded evidence.
- Every kept error entry is classified still open, large gap in flight, or unverified.
- Append this run to the two logs under a `### reconcile-temp-logs (no apply, commit <short hash>)`
  heading (or `(no apply, no commit)` when nothing was committed): the real counts in the success
  log, anything not run or not provable in the error log.

### Report

Summarize briefly: `.apply.md` files and their `_PART/<slug>/` folders deleted, rows backfilled,
handoff sections and success entries removed (named), the backup file path (or that none was
created), and per error heading the classification, the evidence, and what was removed, fixed,
requested, or left open.

## Done when

- The backup file passes the ASCII check.
- Every removed error entry has a verbatim copy and recorded evidence.
- This run is appended to both logs under its heading.
- The report lists files deleted, rows backfilled, sections and entries removed, the backup path and each error heading's outcome.

This is the last step. Do not start another task on your own.
