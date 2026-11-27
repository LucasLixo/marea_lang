# temp/_LOG_APPLY_ERROR.md and temp/_LOG_APPLY_OPEN_POINTS.md

The two outcome logs that [AGENTS.md](../../../../AGENTS.md) ("Reporting the outcome of any task") requires
whenever a task leaves a confirmed bug or an open point behind, and the prose siblings of the apply list
([list-apply.md](list-apply.md)). Create each file the first time a task needs it; `temp/` is ignored by git, so
a log is never committed. Anything that does not fit a table cell - why a split happened, what a real file survey
found that the plan missed, an environment quirk, a verification command that had to change - goes into one of
these two, never inline in the list, split by what kind of thing it is:

- **`_LOG_APPLY_ERROR.md`**: confirmed bugs, broken checks and real failures seen during a task only. A known
  pre-existing failure still belongs here, noted as pre-existing.
- **`_LOG_APPLY_OPEN_POINTS.md`**: everything else by design, never a bug: a check that was not run or could not
  be run, a deferred feature, a known gap, a decision still waiting on the user, a harmless deviation from the
  plan, a fact the survey established that a later apply can rely on but has not yet acted on.

One `### <slug> (#<issue>, merged via PR #<n>)` heading per apply, appended in the order the applies ran, in
whichever of the two logs fits. Neither log is written automatically: a task that leaves no confirmed bug and
nothing open writes to neither. A task with no issue or apply uses `### <slug> (no apply, commit <short hash>)`.
A finding that never became a merged apply at all (a worker stopped short, a hypothesis ruled out) still gets a
heading, worded plainly instead of the merged-PR parenthetical, e.g. `### <slug> (root #<n>, NOT applied, no
PR)`.

Both logs are live checklists, not a diary: an entry stays until whatever it describes is actually resolved
(fixed, or replaced with a line naming the issue that now tracks it) - never removed just because the apply it
came from is done, cleaned up, or its handoff closed. The two ways an entry is removed:

- **`reconcile-temp-logs`** retires an entry from either log only after its own verified workflow confirms what
  the entry describes is resolved.
- **An apply's own closing step** retires an entry when that apply itself is what resolves it - see "Resolving
  an entry when finishing an apply" below. Either way, a mention goes away only once the thing it describes is
  actually resolved; that single rule, not a new exception, covers both.

## Reading the logs when writing a new apply

Before a new apply is written ([steps/02-survey-and-name.md](../steps/02-survey-and-name.md) and onward), check
both `_LOG_APPLY_OPEN_POINTS.md` and `_LOG_APPLY_ERROR.md` for an existing entry relevant to what the apply is
about to build. Pick up or build on a still-open entry instead of re-discovering it, and name the entry (if one
applies) in the apply's `## Context`.

## Resolving an entry when finishing an apply

An apply's own closing or merge step, when it resolves something an earlier task logged, removes that entry's
mention from `_LOG_APPLY_OPEN_POINTS.md`/`_LOG_APPLY_ERROR.md` as part of what finishing the apply means - see
the "On finish" bookkeeping of [apply-part-template.md](apply-part-template.md). This is not a new exception to
the retention rule above: a mention is removed only once the thing it describes is actually resolved; it is
simply the closing part of an apply doing that removal directly, instead of waiting for a later
`reconcile-temp-logs` run to confirm it.
