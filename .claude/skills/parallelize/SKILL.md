---
name: parallelize
mode: edit
effort: high
description: Use when the user asks to split a text request into tasks, build the dependency graph and dispatch subagents in waves, with a git worktree only for tasks whose writes could collide. Also the owner of ephemeral delegation and isolation that other documents link to. Run it only when the user or a rule names it.
argument-hint: "<request text>"
---

# Parallelize

## Context

**Run this in edit mode.** Never enter plan mode. Execute the steps below immediately - do not read
this as documentation, and do not ask for confirmation except where a rule below says to stop. You
are the **orchestrator**: you decompose, dispatch, wait, integrate and report. Whatever a task
produces, a subagent produces; you only touch files to integrate (merge, resolve conflicts, fix
the seams between two results).

This skill is the canonical owner of ephemeral delegation and isolation: free text in, a split
decided in memory, run now, nothing left under `temp/_APPLY/`. For large planned work that needs
durable APPLY files, issues and PRs, use [generate-handoff](../generate-handoff/SKILL.md) plus
[dispatch-applies](../dispatch-applies/SKILL.md) instead, and say so rather than running here.
Other documents that need delegation or isolation link here instead of restating it.

## Requirements

- The whole argument is the request text. Given none, **stop and say so**. Never invent work the
  text does not ask for.
- **Startup**: run `mise trust`, then `git pull --rebase`, and resolve what it reports first. In a
  dirty tree or a worktree that refuses the pull, use `git fetch` plus
  `git merge --ff-only origin/<branch>`, and never stage what was already dirty.
- **Read-only work is allowed on `main`.** Any wave that mutates files requires a non-protected
  feature branch first; if you are on `main` (or another protected branch), create or switch to a
  feature branch before dispatching that wave, or stop and ask. Never push, never trigger release
  workflows.
- **Commits (worktree workers included)**: a worker in a worktree may commit only through the
  `generate-commit` skill ([SKILL.md](../generate-commit/SKILL.md)), and only when its task
  says to commit. Only the orchestrator merges and pushes (push only when the user asked for it).
  Never a hand-written `git commit`.
- Every task ends with an entry in the success log, and in the error log when anything is open; the
  rule and heading format are in `AGENTS.md` ("Reporting the outcome of any task"), not repeated here.
- Parallelism is the goal only where it is safe: a correct slower run beats a fast conflicting one.
- Never stage or revert what was already dirty before you started.
- If something fails, report it plainly with the real output; never describe unrun checks as done.
- ASCII only, no emojis, in everything you write: code, docs, commits, prompts to subagents.

Step map: the step files are numbered like the `Step N` names in the text: step 01 holds the preparation and Step 1 (Split), step 02 is Step 2, and so on to step 06 (Finish).

## How to run

Run the steps in order. Each step file under `steps/` is a complete prompt for that step alone.

1. Open ONE step file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next step. Never read ahead, never merge steps and never skip one. A step that does not apply says so in its own text.
4. A step names the rules it needs. The rules of this file and the rules of the step bind the work: when in doubt, read them again.

## Steps

| Step | File | Purpose |
| --- | --- | --- |
| 1 | [steps/01-split.md](steps/01-split.md) | Before you start, then split the request into tasks |
| 2 | [steps/02-waves.md](steps/02-waves.md) | Build the waves |
| 3 | [steps/03-isolation.md](steps/03-isolation.md) | Choose the isolation |
| 4 | [steps/04-dispatch.md](steps/04-dispatch.md) | Dispatch the wave |
| 5 | [steps/05-integrate.md](steps/05-integrate.md) | Integrate the wave |
| 6 | [steps/06-finish.md](steps/06-finish.md) | Finish, validate and report |
