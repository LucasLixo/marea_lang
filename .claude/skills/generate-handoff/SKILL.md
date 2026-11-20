---
name: generate-handoff
mode: plan
effort: high
description: Use when the user asks to decompose a complex request into a set of APPLY files under temp/_APPLY/ plus the dependency graph that says which can run in parallel, recorded in temp/_LIST_HANDOFF.md and temp/_LOG_HANDOFF.md. Run it only when the user or a rule names it.
argument-hint: "[issue number or a description of the complex task to decompose]"
---

# Handoff Generator

## Context

**Run this in plan mode.** A handoff is a decomposition, and a decomposition is a design decision - enter plan mode, research read-only, settle the open questions with the user, get the shape approved, then leave plan mode to write the files, since plan mode itself may not write to `temp/`. Effort stays at `high`: a wrong split costs every session that follows it.

A **handoff** is a set of APPLYs plus the dependency graph between them. It exists for work too large for one session, and its payoff is parallelism - two applies with no edge between them can run at the same time, in different sessions.

**Plan only.** Write no production code, run no build, make no commit, and touch nothing outside `temp/`. Reading the repository with `git`, `gh`, and the file tools is expected.

When the request turns out to be one focused session of work, this is the wrong command. Say so and point at [generate-apply](../generate-apply/SKILL.md) rather than splitting something that was never two things.

## Requirements

- Plan only, inside `temp/`; the applies follow [generate-apply](../generate-apply/SKILL.md) for format and pipeline.
- Every apply carries `handoff: <the exact section title>`; edges are real dependencies only.
- Any commit an apply asks for goes through the `generate-commit` skill ([SKILL.md](../generate-commit/SKILL.md)); this skill commits nothing.
- Outcome record: like every task, this one ends with an entry in `temp/_LOG_APPLY_SUCCESS.md` (and `temp/_LOG_APPLY_ERROR.md` when something is open), as [generate-apply](../generate-apply/SKILL.md) defines. A handoff's own outcomes go to the handoff logs below.

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-startup-and-read.md](steps/01-startup-and-read.md) | Startup and read the request |
| 2 | [steps/02-survey.md](steps/02-survey.md) | Survey the affected code |
| 3 | [steps/03-decompose.md](steps/03-decompose.md) | Design the decomposition and get approval |
| 4 | [steps/04-write-applies.md](steps/04-write-applies.md) | Write the applies |
| 5 | [steps/05-write-handoff-files.md](steps/05-write-handoff-files.md) | Write the handoff list and plan |
| 6 | [steps/06-validate-and-report.md](steps/06-validate-and-report.md) | Validate and report |

## References

- [references/outcome-logs.md](references/outcome-logs.md) - the outcome logs of a handoff and when each entry is written
