---
name: generate-apply
mode: edit
effort: medium
description: Use when the user asks to turn a request into one self-contained APPLY under temp/_APPLY/, split into an index plus ordered parts under temp/_APPLY/_PART/ that a cold session reads and runs one at a time by pasting the index's path, and to register it in the temp/_LIST_APPLY.md checklist. Run it only when the user or a rule names it.
argument-hint: "[issue number or a description of the task to apply]"
---

# Apply Generator

## Context

**Run this in edit mode** - survey the code, write the file, stop. Escalate to plan mode only under the rule below. Effort stays at `medium`: the output is a prompt another session runs blind, so it has to be right, but this skill builds one apply and not a whole decomposition.

An **apply** is the unit of executable work: an index file plus the ordered parts under
`temp/_APPLY/_PART/<slug>/` it points at. Pasting `temp/_APPLY/<slug>.apply.md` into an empty
prompt must be enough to carry the task from nothing to a merged PR, one part file at a time.
Write it for a reader who never saw this conversation, will not open another apply, and will not
ask you anything.

**Plan only.** Write no production code, run no build, make no commit, and touch nothing outside `temp/`. Reading the repository with `git`, `gh`, and the file tools is expected.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-read-and-size.md](steps/01-read-and-size.md) | Read the request and decide the mode |
| 2 | [steps/02-survey-and-name.md](steps/02-survey-and-name.md) | Survey the code and pick the slug |
| 3 | [steps/03-write-apply-file.md](steps/03-write-apply-file.md) | Write the apply's index file |
| 4 | [steps/04-write-parts.md](steps/04-write-parts.md) | Write the parts |
| 5 | [steps/05-register-and-report.md](steps/05-register-and-report.md) | Register the apply and report |

## References

- [references/list-apply.md](references/list-apply.md) - the apply list table format and the rule for changing a row
- [references/logs.md](references/logs.md) - the two outcome logs, their heading format and the live-checklist rule
- [references/apply-part-template.md](references/apply-part-template.md) - the parts folder, the index's `## How to run`/`## Parts` table, the part-file shape, and the default part breakdown
