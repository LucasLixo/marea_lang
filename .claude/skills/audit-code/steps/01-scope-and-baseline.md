# Step 1 of 6 - Stage 0 - Scope and baseline

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- A red baseline stops the audit and is reported, never fixed blindly.

### Which skill checks which file

| File | Skill (stage 1) |
| --- | --- |
| `.c` / `.h` under `lib/` | [c-style](../../c-style/SKILL.md) |
| `.rs` (no Rust crate exists yet) | [rust-style](../../rust-style/SKILL.md) |
| `CMakeLists.txt` / `*.cmake` | [cmake-style](../../cmake-style/SKILL.md) |
| anything under `tests/` | [cpp-testing](../../cpp-testing/SKILL.md) |
| scripts under `tools/` (`.lua`, `.sh`, `.ps1`), `mise.toml` | no style skill: stage 1 only checks the ASCII rule and the copyright header; stage 3 reads them |

A file with no matching skill skips the style part of stage 1 and the report says so.

1. Resolve the argument list (or the branch diff), drop `3rdparty/`, generated files and `docs/`, and map every file to its skill with the table above.
2. Record the tests covering each file: C tests are GoogleTest files under `tests/source/<module>/`; the tests of a `tools/` script are the `test_*.lua` files under that tool's `tests/` folder, when it has one.
3. Run the covering tests first and record the baseline:
   - C and tests: `mise run build:marea`, then build the test target with `cmake --build cmake-build-debug --target marea_tests` (the mise task builds only the `marea` target), then `ctest --test-dir cmake-build-debug --output-on-failure`. Use the folder of the configuration you built (`cmake-build-<config>`).
   - Tools: `lua tools/marea-ascii-check/tests/run.lua` for the ASCII checker. A tool with no tests has no baseline: say so.
4. A red baseline stops the audit and is reported, never "fixed" blindly.

## Done when

- The file list is resolved and `3rdparty/`, generated files and `docs/` are dropped.
- Every file is mapped to its skill, or marked as having none.
- The covering tests of each file are recorded and ran, and the baseline result is noted with real counts.

Next: [steps/02-conformance.md](02-conformance.md)
