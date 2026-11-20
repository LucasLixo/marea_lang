---
name: instruction-governance
mode: agent
effort: high
description: Mandatory governance for any request that creates, edits, renames, moves or deletes a skill (SKILL.md), an AGENTS.md (engine or Creator), or a workflow skill (a skill with steps/ prompt files). Compares the affected rules before any edit, consolidates compatible duplication through links, stops and asks the user when two rules conflict, then records the decision and fixes every cross-reference. Load it first, automatically, for every such request, including a delegated worker's.
globs: "**/AGENTS.md,**/SKILL.md,**/.agents/skills/**"
argument-hint: "<target file(s)> <requested change>"
---

# Instruction Governance

## Context and Objective

Marea keeps agent instructions in two `AGENTS.md` files and a set of skills. A workflow skill (the former commands) is a short `SKILL.md` plus one self-contained prompt file per step under `steps/`, and shared formats under `references/`. Each subject has one canonical owner; every other document links to it. This skill keeps that true whenever any of them changes, so rules never drift or silently contradict each other.

The owner of each subject is listed in [the canonical document map](references/canonical-documents.md). Keep that map current as part of every change.

## Strict Requirements

- Never edit a skill, an `AGENTS.md` or a workflow skill before the Workflow's read and comparison steps are done.
- A rule lives in exactly one canonical owner. Compatible duplication is replaced by a relative Markdown link to the owner, keeping only a skill-specific exception when one exists.
- Never guess which of two conflicting rules prevails. Report the conflict and WAIT for the user.
- The Creator `.agents/` entries are symlink aliases into the engine submodule. Edit the engine original only, never an alias copy.
- Use relative Markdown links between documents. English, ASCII only.
- Frontmatter: skills carry `name`, `mode`, `effort`, `description`, plus `globs` and `argument-hint` where meaningful; a workflow skill always carries `mode`, `effort` and `argument-hint`, and its `description` starts with "Use when"; a step file under `steps/` carries no frontmatter, has a "Done when" list and names the next step; an `AGENTS.md` carries only `name` and `description`.
- Instructions never tell an agent to author tutorial or report `.md` files.
- A skill whose work is a sequence of actions is written in parts, as a workflow skill (see "Workflow skills" below). Never write a long ordered procedure as one block of prose.

## Workflow skills

A long prompt read at once loses its middle: the agent skips a step or forgets a rule. A workflow skill gives the agent one short prompt at a time, so each step is read in full and checked before the next one starts.

**When to write a skill in parts.** Split it when any of these holds:

- the work is an ordered sequence that a cold session must not reorder or skip;
- a check must pass before the next action may start (a gate);
- the instructions would pass about 80 lines in one file.


**Layout.**

- `SKILL.md`: the frontmatter, a short Context, the Requirements that bind every step, "How to run" and the Steps table. It holds no step body.
- `steps/NN-name.md`: one self-contained prompt per step, numbered from 01 with no gap. A step repeats only the rules it uses; a rule used by several steps lives once in `SKILL.md` or in a reference.
- `references/`: a format or a table that several steps or several skills use. The owner keeps it and every other file links to it.

**Rules of a step file.** The first line is `# Step N of M - title`, with N the file number and M the number of steps. The file has no frontmatter, a `## Rules for this step` section when the step has rules of its own, a `## Done when` list with at least one item, and a last line that names the next step (`Next: [steps/NN-name.md](NN-name.md)`) or says `This is the last step.` A step that may not apply says so in its own text.

**Frontmatter of the skill.** `name` equal to the folder name, `mode`, `effort`, `argument-hint` (an empty text when the skill takes no argument) and a `description` that starts with "Use when". A skill that must run only when the user asks adds "Run it only when the user or a rule names it".

The file shapes are in the [workflow skill template](references/workflow-skill-template.md). `mise run test:skill-check` checks every rule of this section on all skills.

## Workflow

1. **Read before editing**, in full:
   - the engine [AGENTS.md](../../../AGENTS.md) and the Creator [AGENTS.md](../../../../AGENTS.md) (the Creator file sits at the Creator repository root; in a worktree open the worktree copy of both);
   - the target file;
   - every canonical owner of a subject the change touches (see the [map](references/canonical-documents.md));
   - the map itself.
2. **Emit an exact rule comparison** before any edit: for each affected rule quote it exactly with its source path and heading, and label the pair `identical`, `compatible overlap`, `exception` or `conflict`.
3. **Compatible duplication**: keep the text in the canonical owner, replace the other copies with a link, and keep only what is genuinely skill- or area-specific.
4. **Semantic conflict**: stop. Report, for each side, the source path, the heading, the effect of the rule, and the choices (keep A, keep B, a merged wording, scope each to its area). Make no edit that depends on the answer and WAIT for the user.
5. **After the answer**: apply the decision, record it (the temp logs defined by [generate-apply](../generate-apply/references/logs.md) and a one-line note in the owning document where useful), and update every affected cross-reference: links, the canonical map, skill descriptions, skill argument hints and any `AGENTS.md` mention.
6. **Delegation**: every delegated worker prompt carries clickable relative links to both AGENTS files, this skill, the target, and every related canonical owner, plus the workers' file ownership. Independent cross-area work is dispatched through [parallelize](../parallelize/SKILL.md).
7. **Commit** through the [generate-commit](../generate-commit/SKILL.md) skill only.

## Validation

- `git diff --check` is clean and every changed file is ASCII.
- Every relative link in the changed files and in the map resolves.
- Each changed rule still has one canonical owner, and no sibling restates it.
- The map lists every skill that exists, and no removed one.
- `mise run test:skill-check` passes: frontmatter keys, ASCII, resolving links, and for a workflow skill the "Use when" description, the `argument-hint`, the step table, the `Step N of M` heading, the `Done when` list and the `Next` line of every step.
- Report anything unverified plainly; never claim a check that was not run.

## References

- [Canonical document map](references/canonical-documents.md)
- [Workflow skill template](references/workflow-skill-template.md)
- [Engine AGENTS.md](../../../AGENTS.md)
- [Creator AGENTS.md](../../../../AGENTS.md)
