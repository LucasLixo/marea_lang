# Step 3 of 8 - Stage 1A - Permission map sync

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Run this stage only when the diff touches public API. Otherwise say it was skipped and go on.



An existing group's public API change needs no map edit when its permission surface is unchanged. Adding or removing a public group, header or section still updates the map, including an audited negative answer. When `tools/marea-permissions/` is absent, report that once and do not run this stage.

## Done when

- A missing map update is reported as a finding and fixed in the same commit theme.

Next: [steps/04-platform-support.md](04-platform-support.md)
