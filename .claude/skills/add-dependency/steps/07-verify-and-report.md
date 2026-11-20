# Step 7 of 7 - Verify and report

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never claim a check that did not run.
- Do not push. Pushing the engine and bumping the submodule pointer follows the repository rules.

Run the checks and report. Every check is a mise task: use it and never hand-roll the command.

1. `mise run test:license-check`: runs the tool's own tests, then checks the real tree. Every `Cargo.lock` crate must have a registered, non-empty, permissive license file. Fix every line it prints.
2. For a crate, also check the SPDX expressions: `cargo metadata --format-version 1 --locked` from `lib/rust/` into a file, then `lua tools/marea-license-check/main.lua --metadata <file>`. It fails when a selected `TYPE` is not an alternative of the crate's expression, and when no alternative is permissive.
3. `mise run test:cmake` and `mise run test:cmake-guards`.
4. `mise run format:check-sources`.
5. Configure once with the condition on and once with it off (for example the API option on and off, or Windows and Linux) and check that the license appears only when it must: `mise run config:cmake` with the option, then the embedded license list. Say plainly if a platform could not be configured here.
6. For a crate: `mise run build:rust-crate` and `mise run test:rust-crate` for the feature on and off, and run the duplicate scan of step 05 once more: `cargo tree -d --all-features --target all -e normal,build` from `lib/rust/`.
7. ASCII check: `lua tools/marea-ascii-check/main.lua <file>...` over every file you changed.
8. Commit through the `generate-commit` skill, one theme per commit: the 3rdparty pointer and its rows, the manifest and lockfile, the license files, the registration with its test.

### Report

Short and factual: the dependency, the kind, the upstream and version; the license table of step 02 (offered, selected, why); the `if` condition of each registration; each check with its real result and count (passed, failed, not run); the duplicate crates before and after, each remaining one with its cause (pinned by another crate, or build-only); and anything left open. A check that did not run goes to `temp/_LOG_APPLY_ERROR.md`, never to the success log.

## Done when

- `test:license-check`, `test:cmake`, `test:cmake-guards` and `format:check-sources` pass, or each failure is reported with its output.
- For a crate, the duplicate list after the change adds nothing new, or the report names the cause of each new entry.
- The license shows in a build where the condition is on and does not show where it is off, or the report says it was not checked.
- The commits exist, one theme each, through `generate-commit`.
- The report and the log entries are written.

This is the last step. Do not start another task on your own.
