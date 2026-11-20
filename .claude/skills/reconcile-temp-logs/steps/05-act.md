# Step 5 of 6 - Phase 3 - Act on the classification

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never remove an error entry unless verification proved the error is gone.

Apply results serially unless stated otherwise.

- **resolved**: back up the entry verbatim (backup file of step 02), confirm the backup, then remove
  the entry. Record the verification evidence in `temp/_LOG_APPLY_SUCCESS.md` under the heading of
  this run.
- **small gap**: run the smallest relevant test first and watch it fail, fix on the current branch,
  validate with the checks the owning skill names, commit through the `generate-commit` skill, then
  back up and remove the entry with the evidence (test, result, commit short hash) recorded in the
  success log. Small fixes may run in parallel (at most 3) only when their write sets are disjoint;
  otherwise serialize them.
- **large gap**: do not fix inline. Run
  [request-make-apply](../../request-make-apply/SKILL.md) with `<submodule path>` BEFORE
  `<request text>` (the path first, then the request text). Keep the entry until that workflow has
  landed and the acceptance checks prove the error resolved; a later run then handles it as
  **resolved**.
- **still open** and unverified entries: leave untouched.

## Done when

- Each resolved entry was backed up (as in step 02) and removed, with its evidence in the success log.
- Each small gap was fixed test-first and committed through generate-commit, then backed up and removed with its evidence.
- Each large gap was sent to request-make-apply and its entry was kept.
- Every still-open entry is untouched.

Next: [steps/06-validate-and-report.md](06-validate-and-report.md)
