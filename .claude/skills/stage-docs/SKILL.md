---
name: stage-docs
mode: agent
effort: medium
description: Marea Lang's one reference doc per compiler stage under docs/ - the fixed section pattern (title, Contract, Use, the stage's own sections, Diagnostics, Gaps), how Gaps differs from the generate-apply outcome logs, and when to update it. Use when writing or changing any file under docs/, or when a stage gains, closes or finds a limitation.
globs: docs/**/*.md
argument-hint: "[stage name, e.g. parse, sema]"
---

# Stage docs

## Context and Objective

A compiler stage that has public documentation (today: `parse` and `sema`; `lex`, `ir`, `bytecode`,
`vm` and `cgen` will join as they are built) gets exactly one reference doc under
`docs/<stage>/marea-<stage>.md`, named after the stage as [c-style](../c-style/SKILL.md)'s "one
library per stage" names it (`base`, `lex`, `parse`, `sema`, `ir`, `bytecode`, `vm`, `cgen`). One
doc per stage: never split one stage across two files, never merge two stages into one doc.

`docs/ast/marea-ast.md` documents the `parse` stage under a historical folder and file name from
before this skill existed. This skill does not rename it - that is a path change with its own
blast radius, left for a change that asks for it. Until then, its title line names the stage it
really documents, same as every other stage doc.

## Strict Requirements

- A stage doc is a living reference, not a point-in-time report. [instruction-governance](../instruction-governance/SKILL.md)
  and [audit-code](../audit-code/SKILL.md) forbid authoring a tutorial or report `.md` file; a stage
  doc is the opposite of that - it is the one canonical description of a stage's current public
  contract, updated in the same change that changes the contract, never generated once and left to
  rot. See the note each of those skills carries at that rule.
- ASCII only, English (owned by [AGENTS.md](../../../AGENTS.md); this skill does not repeat the
  rule, only applies it here).
- "Docs are part of the change, not a follow-up" ([AGENTS.md](../../../AGENTS.md), "Working model")
  applies in full: a change to a stage's public behavior and its doc land in the same commit.

## Section pattern

Fixed order. A stage-specific section is inserted only where marked; nothing else moves.

1. `# Marea <stage> stage` - `<stage>` is the exact `lib/` stage name, never a different label.
2. `## Contract` - one paragraph: what the stage reads, what it writes, and the boundary invariant
   worth restating for this stage (global state, printing, the arena, disk access). State only what
   is specific to this stage or worth the reminder; do not copy c-style's rules in full.
3. `## Use` - the smallest code snippet that calls the stage's public entry point end to end.
4. **Stage-specific sections**, zero or more, each its own `##` heading, for whatever only this
   stage needs: `sema` has "Import resolver" and "Typed AST hand-off"; `parse` has "Text dump",
   "Node kinds and children", "Precedence", "Lookahead rules" and "Recovery". A future stage adds
   its own; this skill does not enumerate what they must be.
5. `## Diagnostics` - a table (`Code`, `Name`, `Example`) of every diagnostic the stage raises.
   Omitted entirely for a stage that raises none.
6. `## Gaps` - every known limitation of the stage's current, shipped behavior, as plain bullets.
   No numbering scheme: a decision log's own identifiers (for example `temp/OBJETIVO.md`'s `D`
   numbers) are that log's business, not this doc's.

## Gaps versus the outcome logs

`## Gaps` is the curated, de-duplicated, reader-facing statement of what a stage cannot do yet.
`temp/_LOG_APPLY_OPEN_POINTS.md` (owned by [generate-apply](../generate-apply/SKILL.md)) is the
chronological, per-task record of everything every apply left open, and is never pruned while an
item stays open. The two are compatible, not duplicates: a limitation is usually logged first, by
the apply that found it, in `temp/_LOG_APPLY_OPEN_POINTS.md`; the stage doc then folds it into
`## Gaps` as the lasting statement a cold reader sees without reading history.

- Closing a gap: rewrite or remove its `## Gaps` bullet in the same change, and mark the matching
  `temp/_LOG_APPLY_OPEN_POINTS.md` entry resolved - never delete that entry ([generate-apply](../generate-apply/SKILL.md)'s
  own retention rule).
- Finding a new one: add a `## Gaps` bullet and, if the finding came from an apply or an audit,
  record it in `temp/_LOG_APPLY_OPEN_POINTS.md` the same way every other open point is recorded.

## Validation

- `mise run dev:ascii-check` passes on the changed doc (or `lua tools/marea-ascii-check/main.lua <file>`
  for an untracked one).
- Every relative link in the changed doc resolves.
- The section order matches the pattern above; `## Gaps` is last and `## Diagnostics` (when present)
  is the section right before it.
