# temp/_LOG_APPLY_SUCCESS.md and temp/_LOG_APPLY_ERROR.md

The prose siblings of the list above, created the first time any apply needs one. Anything that
does not fit a table cell - why a split happened, what a real file survey found that the plan
missed, an environment quirk, a verification command that had to change - goes into one of these
two, never inline in the list, split by what kind of thing it is:

- **`_LOG_APPLY_SUCCESS.md`**: what went right. A verification result with real counts, a harmless
  deviation from the plan, a fact the survey established that a later apply can now rely on
  without re-deriving it. Everything here held up.
- **`_LOG_APPLY_ERROR.md`**: open points and inconveniences, not only outright errors. A check that
  was not run and should be before anyone relies on this, a known pre-existing failure confirmed
  unchanged, a stale comment or a rough edge left on purpose, a decision still waiting on the user.
  Anything here is unfinished business, not history.

One `### <slug> (#<issue>, merged via PR #<n>)` heading per apply, appended in the order the
applies ran. Every completed task writes a success entry, even one that ran exactly as planned;
an error entry is written only when open work exists. A task with no issue or apply uses
`### <slug> (no apply, commit <short hash>)`. A finding that never became a merged apply at all (a worker
stopped short, a hypothesis ruled out) still gets a heading in `_LOG_APPLY_ERROR.md`, worded
plainly instead of the merged-PR parenthetical, e.g. `### <slug> (root #<n>, NOT applied, no PR)`.

`_LOG_APPLY_ERROR.md` is a live checklist, not a diary: an entry stays until whatever it describes
is actually resolved (fixed, or replaced with a line naming the issue that now tracks it) - never
removed just because the apply it came from is done, cleaned up, or its handoff closed. The only
exception is the verified workflow of `reconcile-temp-logs`, which retires an error entry
only after confirming that what it describes is resolved. It retires a `_LOG_APPLY_SUCCESS.md`
entry once the apply is done and cleaned, and never touches `_LOG_APPLY_ERROR.md` on that
trigger alone.
