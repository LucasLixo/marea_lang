# Step 1 of 7 - Validate the arguments

Part of the [request-make-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never invent the target repository.

- **Argument 1 - `<submodule path>`**: the checkout the work belongs to, e.g. `marea/`
  (relative to the current directory or absolute).
- **Argument 2 - `<request text>`**: free text, the task to deliver.

Validation order, before anything else: (1) the submodule path must exist, be a git
checkout, and contain `.agents/skills/generate-apply/SKILL.md` and `CLAUDE.md` or
`AGENTS.md`; (2) the request text must be non-empty. On any failure **stop and say so** -
this skill never invents the target repository.

## Done when

- The submodule path exists, is a git checkout, and holds `.agents/skills/generate-apply/SKILL.md` and `CLAUDE.md` or `AGENTS.md`.
- The request text is not empty.
- On any failure you stopped and said so.

Next: [steps/02-size-triage.md](02-size-triage.md)
