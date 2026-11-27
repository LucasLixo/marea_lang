# Step 4 of 4 - Verify and report

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never claim a check that did not run.
- Do not push.

Run the checks and report. Use the mise task when one exists.

1. `mise run dev:quiet-test` ([cpp-testing](../../cpp-testing/SKILL.md), "Running tests"). Record the real counts. A fresh configure must pass: remove the build folder you made for this check, not another one.
2. `mise run format:check-sources`.
3. `mise run dev:ascii-check`. The vendored submodule is a separate repository, so `git ls-files` does not list its files and they are not checked.
4. Check that no production target links the dependency when it is test-only: `git grep -n "<Name>" -- CMakeLists.txt lib` shows no link from `marea` or a stage library.
5. Commit through the `generate-commit` skill, one theme per commit: the submodule pin with `.gitmodules`, then the CMake wiring with its test.

### Report

Short and factual: the dependency, the upstream and version; the license table of step 02 (offered, selected, why); the target that links it and with which visibility; each check with its real result and count (passed, failed, not run); and anything left open. A check that did not run goes to `temp/_LOG_APPLY_OPEN_POINTS.md`; a confirmed bug or broken check goes to `temp/_LOG_APPLY_ERROR.md`.

## Done when

- The build, the tests and `format:check-sources` pass, or each failure is reported with its output.
- `dev:ascii-check` passes.
- The commits exist, one theme each, through `generate-commit`.
- The report and the log entries are written.

This is the last step. Do not start another task on your own.
