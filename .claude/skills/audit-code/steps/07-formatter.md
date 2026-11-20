# Step 7 of 8 - Stage 4 - Formatter

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never add or change a formatter, a config or a mise task. Only call the existing ones.


## Done when

- The language format task ran.
- `mise run format:check-sources` exits 0 and the covering tests are green again.

Next: [steps/08-close-and-report.md](08-close-and-report.md)
