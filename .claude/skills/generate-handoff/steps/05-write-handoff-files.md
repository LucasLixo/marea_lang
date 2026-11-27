# Step 5 of 6 - Write the handoff list and plan

Part of the [generate-handoff](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Edit the global files, never rewrite them. Write the plan, never an outcome.

Write this handoff's section in `temp/_LIST_HANDOFF.md` (title, legend, table only) and its matching section in `temp/_LOG_HANDOFF.md` (goal, decomposition rationale, out-of-scope notes, parallel waves).

### temp/_LIST_HANDOFF.md

One global file, one `##` section per handoff. Created on the first run and **edited** afterwards, never rewritten - earlier handoffs stay. Each section holds **only** the section title, legend, and table - no goal, no narrative, no parallel-wave summary; those live in the matching section of `_LOG_HANDOFF.md`.

```md
# Handoff list

## Add the lexer stage

Legend: `[ ]` pending  `[x]` done  `[-]` cancelled

| | Apply | Description | Depends on | Issue |
| - | ----- | ----------- | ---------- | ----- |
| [ ] | `_APPLY/add-lexer-token-kinds.apply.md` | Token kinds and the token type in lib/include/marea/lex | - | |
| [ ] | `_APPLY/cover-diagnostic-sink-accessors.apply.md` | GoogleTest for the diagnostic sink accessors | - | |
| [ ] | `_APPLY/add-lexer-scan-function.apply.md` | The scan function that fills a token list | add-lexer-token-kinds | |
```

`Depends on` holds apply slugs without the `.apply.md` suffix, `-` when there are none. There is deliberately **no second identifier scheme** - the slug is the name everywhere, so nothing has to be kept in step with anything.

The section title is what every apply of this handoff carries in its `handoff:` field, so it must be stable and unique across the file. Sharpen a colliding title rather than numbering it.

### temp/_LOG_HANDOFF.md

One global file, one `##` section per handoff, titled identically to its `_LIST_HANDOFF.md` counterpart - that title is the join key across `_LIST_HANDOFF.md`, this file, and the two logs below. Holds the handoff's **plan**, settled once and rarely touched again:

```md
# Handoff log

## Add the lexer stage

Goal: add the lex stage with its token kinds, diagnostics and tests, following the grammar in docs/grammar/marea.ebnf.

Root issue: TBD
Root branch: TBD

Parallel waves:
- wave 1: `add-lexer-token-kinds`, `cover-diagnostic-sink-accessors`
- wave 2: `add-lexer-scan-function`
```

The goal, why the decomposition looks the way it does, what is deliberately out of scope, any reference pattern or shared verification recipe every apply in the handoff follows, and the `Parallel waves` summary all go here at write time - write the `Root issue`/`Root branch` lines as shown, literally `TBD`. `TBD` means absent: the first batch to execute replaces both values during its first-batch setup, per step 0 of `generate-apply`'s handoff-batch pipeline, and every later batch reads them from here. Recompute `Parallel waves` whenever an edge in the matching `_LIST_HANDOFF.md` table changes.

This file never holds a deviation entry or a closing note - those are outcomes, not plan. A handoff's outcome,
once it closes, is recorded through the closing apply's own `_LOG_APPLY_ERROR.md`/`_LOG_APPLY_OPEN_POINTS.md`
entries instead (see [references/outcome-logs.md](../references/outcome-logs.md)), cross-referenced by slug -
never duplicated into more than one file.

## Done when

- `_LIST_HANDOFF.md` has the section with the table only.
- `_LOG_HANDOFF.md` has the matching section with `Root issue: TBD`, `Root branch: TBD` and `Parallel waves`.
- Both sections carry the identical title, which every apply carries in `handoff:`.

Next: [steps/06-validate-and-report.md](06-validate-and-report.md)
