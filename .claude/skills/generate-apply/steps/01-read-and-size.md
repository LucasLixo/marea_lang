# Step 1 of 5 - Read the request and decide the mode

Part of the [generate-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Plan only: write no production code, run no build, make no commit, and touch nothing outside `temp/`.

Read the request. When the argument is an issue number, run `gh issue view <n>`.

### Escalation

Design in **plan mode** when the request is large or complex - several areas touched, an unclear approach, a decision the user has to make. Enter it, research read-only, settle the open questions with the user, get the approach approved, leave, and only then write the file, since plan mode itself may not write to `temp/`.

A small, unambiguous request skips this and goes straight to writing.

When the decomposition is what is unclear - the work is plainly several sessions rather than one - this is the wrong command. Say so and point at [generate-handoff](../../generate-handoff/SKILL.md).

## Done when

- The request is understood in one sentence.
- You chose: design in plan mode first, or go straight to writing. A work that is plainly several sessions was sent to generate-handoff and this skill stopped.

Next: [steps/02-survey-and-name.md](02-survey-and-name.md)
