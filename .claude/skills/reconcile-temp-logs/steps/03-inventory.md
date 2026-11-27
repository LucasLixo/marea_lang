# Step 3 of 6 - Phase 1 - Inventory the error and open-points logs

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Change nothing in this step.

List every `### <slug> ...` heading in `temp/_LOG_APPLY_ERROR.md` and `temp/_LOG_APPLY_OPEN_POINTS.md`,
with the file and the line range of each. Either file may be missing (create neither here - a
missing file simply contributes no headings). This list is the work queue; nothing is changed in
this phase.

## Done when

- Every heading of `temp/_LOG_APPLY_ERROR.md` and `temp/_LOG_APPLY_OPEN_POINTS.md` is listed with its file and line range.

Next: [steps/04-verify.md](04-verify.md)
