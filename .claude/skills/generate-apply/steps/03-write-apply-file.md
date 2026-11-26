# Step 3 of 5 - Write the apply's index file

Part of the [generate-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- The header `mode` is always `edit`. Any decision the work still needs is settled before you write.
- Leave `issue:` empty. Leave `handoff:` empty unless generate-handoff is the caller.

Write `temp/_APPLY/<slug>.apply.md` from the template below. This is the **index**: read once and
kept in mind by the executing session, never the place the task's own actions live - those are
parts, written by step 04. Write every section except the `## Parts` table rows: step 04 writes
the part files and fills that table to match.

### The apply index file

```md
---
title: <the human title, the same one the GitHub issue will carry>
description: <one sentence: what this delivery changes>
mode: edit
effort: low | medium | high | max
label: bug | feature | documentation | refactor | tests | audit
issue:
handoff:
---

# <title>

This apply is split into parts under `_PART/<slug>/`. Read this index once, then run its parts
in order - one at a time, never two at once and never out of order. Execute immediately - this
is not documentation, do not ask for confirmation except where a part states otherwise.

## Objective

One sentence: what this session delivers.

## Context

What is needed to act - the shape of the existing code, the decision this follows
from, and what already landed that this builds on.

## Conventions

TDD first, no symbol ships without a test (`cpp-testing`). Docs are part of the change
(`AGENTS.md`). Style per language (`c-style`, `rust-style`, `cmake-style`). Build and
test with `mise run dev:quiet-build` and `mise run dev:quiet-test` (`cpp-testing`, "Running tests").
Format with `mise run format:cpp-sources` and check with
`mise run format:check-sources`. ASCII only, verified with `mise run dev:ascii-check` (tracked
files) or `lua tools/marea-ascii-check/main.lua <file>...`.

## Files

Exact paths to create, modify, or read as reference.

## Acceptance

Observable conditions that make this done.

## Verification

The commands that prove it, e.g. `mise run dev:quiet-test`.

## Do not touch

Paths and concerns owned by other work.

## How to run

<the "## How to run" block of references/apply-part-template.md, copied verbatim>

## Parts

| Part | File | Purpose |
| --- | --- | --- |
```

The header's `mode` and `effort` govern the **session that executes the apply**, and are independent of this skill's own. `mode` is always `edit` - an apply is by definition a plan already settled and ready to execute, never a design decision left to the executing session; any decision the work still needs gets resolved before the file is written, via this skill's own escalation to plan mode (see step 01). `effort` comes from how much judgement the parts need, not from how many files they touch.

Leave `issue:` empty - the executing session fills it. Leave `handoff:` empty too; only [generate-handoff](../../generate-handoff/SKILL.md) fills that field, with the exact title of the section it wrote.

Fill `## Conventions` down to what the task actually touches.

## Done when

- The file `temp/_APPLY/<slug>.apply.md` exists with the full header and every section down to `## How to run`.
- `## Conventions` names only the skills the task touches.
- The `## Parts` table has its header row only - step 04 fills it.
- No placeholder is left, except the ones the next step defines (`<base>` is replaced there).

Next: [steps/04-write-parts.md](04-write-parts.md)
