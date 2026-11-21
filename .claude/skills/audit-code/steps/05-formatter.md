# Step 5 of 6 - Stage 4 - Formatter

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never add or change a formatter, a config or a mise task. Only call the existing ones.

Run the task for each language the audit touched: `mise run format:cpp-sources` (C and C++; on a dirty tree prefer the scoped `clang-format -style=file -i <files>` so that files outside the audit stay untouched), `format:tool-scripts` (Lua under `tools/`) or `format:cmake-sources`. The tree is already formatted, so a diff after the run is only what the audit itself changed. Confirm with `mise run format:check-sources`, then re-run the covering tests of step 01.

## Done when

- The format task of each touched language ran.
- `mise run format:check-sources` exits 0 and the covering tests are green again.

Next: [steps/06-close-and-report.md](06-close-and-report.md)
