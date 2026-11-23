# temp/_LIST_APPLY.md and mutability

One global, accumulating table. Created on the first run and **edited** afterwards, never rewritten - the rows of earlier tasks are the point of the file. It holds **only** the table: no notes, no deviations, no prose.

```md
# Apply list

Legend: `[ ]` pending  `[x]` done  `[-]` cancelled

| | Apply | Description | Mode | Effort | Label | Issue |
| - | ----- | ----------- | ---- | ------ | ----- | ----- |
| [x] | `_APPLY/cleanup-stale-test-helpers.apply.md` | Delete unused test helpers left behind by a prior refactor | edit | low | refactor | #255 |
| [ ] | `_APPLY/add-lexer-token-kinds.apply.md` | Add the token kinds of the grammar to the lexer | edit | high | feature | |
| [-] | `_APPLY/add-bytecode-jit.apply.md` | JIT backend (cancelled: the language has no JIT) | edit | medium | feature | |
```

`Description` is one short line, the same substance as the apply's `description:` field. `Mode`,
`Effort`, and `Label` mirror the apply's own `mode:`/`effort:`/`label:` header fields, copied into
the row the moment it is added - this is what lets the row survive as the only remaining record of
them once a finished apply's own file is deleted ([reconcile-temp-logs](../../reconcile-temp-logs/SKILL.md) does exactly that once a
row is `[x]`). `Issue` stays empty until the issue exists, then carries `#123`.

A row written before these three columns existed has no `Mode`/`Effort`/`Label` entry - the file
this section describes is edited, never rewritten, so older rows are never backfilled purely to
match a later column; `reconcile-temp-logs` backfills one only at the moment it would otherwise
lose that row's last copy of the information, deleting the apply file right after.

## Mutability

The list is the single source of truth for progress - an apply file never keeps its own status. A session that finds a better approach may reword, split, add, or cancel a row, and updates the matching apply file to match, always with a short parenthetical reason so the next session sees why the plan moved, e.g. `(split - see ../_LOG_APPLY_SUCCESS.md)`. **Never delete a row to make it disappear; cancel it.** Anything longer than a short parenthetical belongs in `_LOG_APPLY_SUCCESS.md` or `_LOG_APPLY_ERROR.md`, not in the row.
