# temp/_LOG_HANDOFF.md and a handoff's outcome

`temp/_LOG_HANDOFF.md` ([steps/05-write-handoff-files.md](../steps/05-write-handoff-files.md)) holds only a
handoff's **plan**: the goal, the decomposition rationale, out-of-scope notes, the `Root issue`/`Root branch`
lines and the `Parallel waves` summary. It is settled once at write time and is only ever updated for the
`Root issue`/`Root branch` lines (replaced by whichever batch's setup runs first) and `Parallel waves`
(recomputed when an edge changes). This file is unaffected by anything below - it is not an outcome log and it
is not removed.

A handoff has no outcome log of its own. What a handoff produced - what landed, what is still open - is
recorded through the **closing apply's own entries** in `temp/_LOG_APPLY_ERROR.md` (a confirmed bug or broken
check) and `temp/_LOG_APPLY_OPEN_POINTS.md` (anything else still open), exactly as
[generate-apply](../../generate-apply/SKILL.md)'s [logs reference](../../generate-apply/references/logs.md)
defines for any other apply. The closing apply - the one whose last part runs the handoff-closeout action
([apply-part-template.md](../../generate-apply/references/apply-part-template.md)'s handoff-batch variant) -
writes that record the moment it closes the handoff out, naming the handoff the same way every batch already
does.

`temp/_LOG_HANDOFF_SUCCESS.md` and `temp/_LOG_HANDOFF_ERROR.md` no longer exist as a concept: no skill reads or
writes them any more. A per-apply deviation or open point belongs in `temp/_LOG_APPLY_ERROR.md`/
`temp/_LOG_APPLY_OPEN_POINTS.md` instead, cross-referenced by slug - never duplicated into more than one file.
