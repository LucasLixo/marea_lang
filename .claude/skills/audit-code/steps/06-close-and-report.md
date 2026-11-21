# Step 6 of 6 - Close, validate and report

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never push. The pipeline that called this skill owns the push. A clean pass creates no commit.

1. Run the verification the files imply: `mise run build:marea`, the build of `marea_tests` and `ctest --test-dir cmake-build-debug --output-on-failure` (see step 01) for C and tests, `lua tools/marea-ascii-check/tests/run.lua` when the ASCII checker changed, and `mise run dev:ascii-check` for every file.
2. Commit each theme through `generate-commit`.
3. Write the success entry in `temp/_LOG_APPLY_SUCCESS.md` and an error entry in `temp/_LOG_APPLY_ERROR.md` for every finding left open (reported public-API issues, unrunnable checks, skipped formatting), in the format of [the logs reference of generate-apply](../../generate-apply/references/logs.md). A worker with no `temp/` puts them in its final report instead.

- The baseline was green before stage 2, and every covering test is green after each stage.
- `mise run format:check-sources` exits 0.
- Every bug fixed has a test that failed first.
- No test, security check, bound check or required header or doc was removed.
- A clean pass created no commit.

A table per file: skill checked, violations fixed and reported, lines removed and added, tests deleted or adapted, bugs found with their tests, formatter result. Then the checks actually run with real counts, and the checks that could not run, plainly.

## Done when

- The verification the files imply ran with real counts.
- Each theme is committed through generate-commit.
- The success entry and an error entry for every open finding are written (or put in the final report when there is no `temp/`).
- The final report has a table per file, the checks run with real counts and the checks that could not run.

This is the last step. Do not start another task on your own.
