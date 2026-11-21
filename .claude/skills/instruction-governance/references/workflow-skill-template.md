# Workflow skill template

The shape of a skill written in parts. The rules are in "Workflow skills" of [instruction-governance](../SKILL.md). No task checks them: use the manual checks of its "Validation" section.

## Folder

```
.claude/skills/<name>/
  SKILL.md
  steps/
    01-<first-step>.md
    02-<second-step>.md
  references/
    <shared-format>.md
```

## SKILL.md

```md
---
name: <name>
mode: edit
effort: medium
description: Use when <the request that starts this skill>. Run it only when the user or a rule names it.
argument-hint: "<arguments>"
---

# <Title>

## Context

What the skill delivers and for whom, in a few lines.

## Requirements

- The rules that bind every step. A rule used by one step goes into that step instead.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-<first-step>.md](steps/01-<first-step>.md) | <what it does> |
| 2 | [steps/02-<second-step>.md](steps/02-<second-step>.md) | <what it does> |
```

## A step file

```md
# Step 1 of 2 - <title>

Part of the [<name>](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Only the rules this step uses.

<The actions, concrete and in order.>

## Done when

- An observable condition that proves the step is finished.

Next: [steps/02-<second-step>.md](02-<second-step>.md)
```

The last step ends with `This is the last step. Do not start another task on your own.` in place of the `Next` line.
