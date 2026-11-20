# Step 3 of 8 - Stage 1A - Permission map sync

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Run this stage only when the diff touches public API. Otherwise say it was skipped and go on.

Run this stage only when the audited diff touches public API: exported C symbols in `lib/include/`, or `marea.*` public functions. Map every touched symbol to its API group using the same grouping as `tools/marea-permissions/map.lua`; use `mise run docs:build-api-catalog` or the public header tree when that makes the ownership unclear.

For every touched group, read its entries in `tools/marea-permissions/map.lua` for all five platform targets: Android, Windows, Flatpak, Snap and web. When the change adds, removes or alters a capability with permission implications - network, clipboard, storage, file dialog, narration or semantics, renderer, audio, or video - the map must change in the same change set. Map edits follow the tool's own tests and TDD requirements. A missing map update is a reported finding like a stage 1 violation; its fix lands in the same commit theme.

An existing group's public API change needs no map edit when its permission surface is unchanged. Adding or removing a public group, header or section still updates the map, including an audited negative answer. When `tools/marea-permissions/` is absent, report that once and do not run this stage.

## Done when

- The stage was skipped with a reason, or every touched group was checked against the five platform targets of `tools/marea-permissions/map.lua`.
- A missing map update is reported as a finding and fixed in the same commit theme.

Next: [steps/04-platform-support.md](04-platform-support.md)
