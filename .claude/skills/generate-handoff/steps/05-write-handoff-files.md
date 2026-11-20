# Step 5 of 6 - Write the handoff list and plan

Part of the [generate-handoff](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Edit the global files, never rewrite them. Write the plan, never an outcome.

Write this handoff's section in `temp/_LIST_HANDOFF.md` (title, legend, table only) and its matching section in `temp/_LOG_HANDOFF.md` (goal, decomposition rationale, out-of-scope notes, parallel waves).

### temp/_LIST_HANDOFF.md

One global file, one `##` section per handoff. Created on the first run and **edited** afterwards, never rewritten - earlier handoffs stay. Each section holds **only** the section title, legend, and table - no goal, no narrative, no parallel-wave summary; those live in the matching section of `_LOG_HANDOFF.md`.

```md
# Handoff list

## Port the audio device to Rust

Legend: `[ ]` pending  `[x]` done  `[-]` cancelled

| | Apply | Description | Depends on | Issue |
| - | ----- | ----------- | ---------- | ----- |
| [ ] | `_APPLY/port-audio-device-to-rust.apply.md` | Device handle into lib/rust | - | |
| [ ] | `_APPLY/cover-device-enumeration.apply.md` | GoogleTest for device enumeration | - | |
| [ ] | `_APPLY/document-marea-audio-open.apply.md` | docs/functions/ entry for the new signature | port-audio-device-to-rust | |
```

`Depends on` holds apply slugs without the `.apply.md` suffix, `-` when there are none. There is deliberately **no second identifier scheme** - the slug is the name everywhere, so nothing has to be kept in step with anything.

The section title is what every apply of this handoff carries in its `handoff:` field, so it must be stable and unique across the file. Sharpen a colliding title rather than numbering it.

### temp/_LOG_HANDOFF.md

One global file, one `##` section per handoff, titled identically to its `_LIST_HANDOFF.md` counterpart - that title is the join key across `_LIST_HANDOFF.md`, this file, and the two logs below. Holds the handoff's **plan**, settled once and rarely touched again:

```md
# Handoff log

## Port the audio device to Rust

Goal: move device ownership out of C into lib/rust with the C ABI unchanged.

Root issue: TBD
Root branch: TBD

Parallel waves:
- wave 1: `port-audio-device-to-rust`, `cover-device-enumeration`
- wave 2: `document-marea-audio-open`
```

The goal, why the decomposition looks the way it does, what is deliberately out of scope, any reference pattern or shared verification recipe every apply in the handoff follows, and the `Parallel waves` summary all go here at write time - write the `Root issue`/`Root branch` lines as shown, literally `TBD`. `TBD` means absent: the first batch to execute replaces both values during its first-batch setup, per step 0 of `generate-apply`'s handoff-batch pipeline, and every later batch reads them from here. Recompute `Parallel waves` whenever an edge in the matching `_LIST_HANDOFF.md` table changes.

This file never holds a deviation entry or a closing note - those are outcomes, not plan, and belong in `_LOG_HANDOFF_SUCCESS.md`/`_LOG_HANDOFF_ERROR.md` below. A per-apply deviation belongs in `_LOG_APPLY_SUCCESS.md`/`_LOG_APPLY_ERROR.md` instead, cross-referenced by slug - never duplicated into more than one file.

## Done when

- `_LIST_HANDOFF.md` has the section with the table only.
- `_LOG_HANDOFF.md` has the matching section with `Root issue: TBD`, `Root branch: TBD` and `Parallel waves`.
- Both sections carry the identical title, which every apply carries in `handoff:`.

Next: [steps/06-validate-and-report.md](06-validate-and-report.md)
