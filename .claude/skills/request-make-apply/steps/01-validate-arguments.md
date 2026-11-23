# Step 1 of 7 - Validate the argument

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never invent the request.

The only argument is `<request text>`: free text, the task to deliver.

Validation order, before anything else: (1) the current directory must be inside a git checkout
of this repository and contain `AGENTS.md` and `.claude/skills/generate-apply/SKILL.md`; (2) the
request text must be non-empty. On any failure **stop and say so**.

## Done when

- The current directory is a checkout that holds `AGENTS.md` and `.claude/skills/generate-apply/SKILL.md`.
- The request text is not empty.
- On any failure you stopped and said so.

Next: [steps/02-size-triage.md](02-size-triage.md)
