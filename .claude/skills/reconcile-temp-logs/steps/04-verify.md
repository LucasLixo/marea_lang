# Step 4 of 6 - Phase 2 - Read-only verification

Part of the [reconcile-temp-logs](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Verification tasks are read-only. At most 3 tasks per wave, one task per error heading.
- Dispatch through [parallelize](../../parallelize/SKILL.md).

Run [parallelize](../../parallelize/SKILL.md) over the queue: one task per error heading, in
waves of at most 3 tasks. Each task is read-only (no source edit, no log edit, no commit) and must:

1. Read its entry and find the code, test, or check it names.
2. Re-run the cheapest check that decides it (the smallest relevant test, a grep, a file existence
   check), or state why the check cannot be run.
3. Report evidence (command and real result) and classify the entry as exactly one of:
   - **resolved** - the described error no longer exists.
   - **small gap** - it exists and fits a few commits on the current branch.
   - **large gap** - it exists and needs many modifications across distinct APIs.
   - **still open** - it exists and waits on a decision, an unavailable tool, or cannot be verified.

## Done when

- Every heading has a classification (resolved, small gap, large gap or still open) with the command and its real result as evidence.

Next: [steps/05-act.md](05-act.md)
