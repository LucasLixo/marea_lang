# Step 7 of 7 - Validate and report

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

- Large gap is not a reason to stop: it takes the handoff path. Stop only when the request
  is too unclear to size, and say what is missing.
- Done means: PR merged (or handoff closed out), worktree removed, submodule at the base
  tip, logs written, pointer commit made or declared unmoved.

### Final report

Short and factual: the slug; the size verdict; issue and PR numbers with merge state; the
worktree path used and removed; every verification's result with real counts (passed,
failed, not run); every deviation from the generated apply; confirmation that the
bookkeeping was synced to the main submodule checkout; and the parent's pointer commit hash
or a plain statement that the pointer did not move. Then stop - never chain a second
request on your own.

## Done when

- The PR is merged (or the handoff is closed out), the worktree is removed, the submodule is at the base tip, the logs are written and the pointer commit is made or declared unmoved.
- The final report is given and you stopped. No second request starts on its own.

This is the last step. Do not start another task on your own.
