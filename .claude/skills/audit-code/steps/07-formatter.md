# Step 7 of 8 - Stage 4 - Formatter

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never add or change a formatter, a config or a mise task. Only call the existing ones.

Run the language task: `mise run format:cpp-sources` (C/C++; on a dirty tree prefer the scoped `clang-format -style=file -i <files>` exception in [c-style](../../c-style/SKILL.md)), `format:rust-sources`, `format:tool-scripts`, `format:cmake-sources`, or `format:all-sources`. The tree is already formatted, so a diff after the run is only what the audit itself changed. Confirm with `mise run format:check-sources`, then re-run the covering tests.

## Done when

- The language format task ran.
- `mise run format:check-sources` exits 0 and the covering tests are green again.

Next: [steps/08-close-and-report.md](08-close-and-report.md)
