# Step 6 of 6 - Bookkeeping, validation and final report

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Only the orchestrator edits `temp/`. The log formats are in [generate-apply logs](../../generate-apply/references/logs.md) and the handoff logs in [outcome logs](../../generate-handoff/references/outcome-logs.md).
- ASCII only, no emojis, in everything you write.

For each merged apply: tick its row `[x]` in both `_LIST_APPLY.md` and, if it belongs to a
handoff, the matching `_LIST_HANDOFF.md` section, filling the Issue column in both.

For every apply of the round, write a `_LOG_APPLY_ERROR.md` entry when a confirmed bug or broken check was seen,
and a `_LOG_APPLY_OPEN_POINTS.md` entry for anything else left open, wrong, or inconvenient (a harmless
deviation, a fact the next apply can rely on but has not yet acted on) - neither when the apply ran exactly as
planned with nothing open. The heading format and the error/open-points split are owned by
[generate-apply](../../generate-apply/SKILL.md); follow it, do not restate it here. When this round resolves a
mention an earlier apply left in either log, remove it as part of finishing that apply, per the same reference.

If this round created a handoff's root issue/branch, record "Root issue: #<n>" and "Root branch:
`<label>/<n>`" in `_LOG_HANDOFF.md`'s matching section, replacing its `TBD` lines. If this round
closed a handoff out, its outcome is recorded through the closing apply's own entries above - a
handoff has no outcome log of its own, per
[generate-handoff](../../generate-handoff/references/outcome-logs.md). `_LOG_APPLY_ERROR.md`/
`_LOG_APPLY_OPEN_POINTS.md` entries are a live checklist: never remove one just because its apply
or handoff is now done - only when it is actually resolved.

- Every merged PR passed its apply's `## Verification` before the next merge, and a closed handoff
  passed its full recipe on the merged root.
- Every confirmed bug of the round has an error-log entry; every open point has an open-points-log entry.
- `git status` shows nothing staged that was dirty at Startup.

### Final report

A table (apply, handoff, issue, PR, merge state, bump applied, when `metadata.yaml` exists), the final `metadata.yaml` on every
branch this round touched (when it exists), every verification's result with real counts (what passed, failed, or
did not run), every deviation, each touched handoff's row completion (`x` of `y`), and a proposed
next round. Then **stop and wait** - never start another round on your own.

## Done when

- Every merged apply is ticked in `_LIST_APPLY.md` and, if it belongs to a handoff, in `_LIST_HANDOFF.md`, with the Issue column filled.
- Every confirmed bug of the round has an error-log entry. Every open point has an open-points-log entry.
- `git status` shows nothing staged that was dirty at startup.
- The final report was given and you stopped. No new round starts on its own.

This is the last step. Do not start another task on your own.
