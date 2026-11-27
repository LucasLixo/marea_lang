# Step 6 of 6 - Validate and report

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

- Verify the copied blocks with `lua tools/marea-ascii-check/main.lua temp/_BACKUP/{DATE}_{TIME}/LOG_BLOCKS.md` (`temp/` is untracked, so `mise run dev:ascii-check` does not cover it).
- Every removed entry has a verbatim copy in the backup and recorded evidence.
- Every kept entry is classified still open, large gap in flight, or unverified.
- This run's own outcome record, per [AGENTS.md](../../../../AGENTS.md)'s "Reporting the outcome of
  any task", only when something qualifies: a confirmed bug or broken check this run found (not
  fixed inline) gets a `### reconcile-temp-logs (no apply, commit <short hash>)` heading (or
  `(no apply, no commit)` when nothing was committed) in `temp/_LOG_APPLY_ERROR.md`; anything not
  run or not provable gets the same heading in `temp/_LOG_APPLY_OPEN_POINTS.md`. A run that
  resolved everything it touched cleanly needs no such heading - its evidence goes in the report
  below instead.

### Report

Summarize briefly: `.apply.md` files and their `_PART/<slug>/` folders moved, rows backfilled,
handoff sections and success entries removed (named), the backup folder path (or that none was
created), and per error heading the classification, the evidence, and what was removed, fixed,
requested, or left open.

## Done when

- The backup file passes the ASCII check.
- Every removed entry has a verbatim copy and recorded evidence.
- This run's outcome record, if anything qualifies, is appended to `temp/_LOG_APPLY_ERROR.md` and/or `temp/_LOG_APPLY_OPEN_POINTS.md` under its heading.
- The report lists files moved, rows backfilled, sections and entries removed, the backup folder and each heading's outcome.

This is the last step. Do not start another task on your own.
