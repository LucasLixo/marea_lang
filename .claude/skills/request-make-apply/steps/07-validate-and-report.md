# Step 7 of 7 - Validate and report

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

- A large request is not a reason to stop: it takes the handoff path. Stop only when the request
  is too unclear to size, and say what is missing.
- Done means: PR merged (or handoff closed out), worktree removed, main checkout at the base
  tip (or left alone with a reason), logs written.

### Final report

Short and factual: the slug; the size verdict; issue and PR numbers with merge state; the
worktree path used and removed; every verification's result with real counts (passed,
failed, not run); every deviation from the generated apply; and confirmation that the
bookkeeping was synced to the main checkout's `temp/`. Then stop - never chain a second
request on your own.

## Done when

- The PR is merged (or the handoff is closed out), the worktree is removed, the logs are written.
- The final report is given and you stopped. No second request starts on its own.

This is the last step. Do not start another task on your own.
