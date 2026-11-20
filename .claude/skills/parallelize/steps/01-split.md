# Step 1 of 6 - Before you start, then split the request into tasks

Part of the [parallelize](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never enter plan mode. Never stage or revert what was already dirty.

Run `mise trust`, then `git pull --rebase`, and resolve what it reports first (dirty tree or worktree that refuses: `git fetch` plus `git merge --ff-only origin/<branch>`).

Read `AGENTS.md` and the skills the request touches (`.agents/skills/`), then verify the real
state: `git status`, `git branch --show-current`, `git worktree list`. The tree may already carry
drift that is nobody's doing this run (a dirty submodule pointer, generated docs); `git status` at
the start is your baseline for what NOT to stage or revert.

### Split

Read enough code to split on real seams, not guesses (a quick search, not a full audit). Break the
request into **tasks**, each one:

- **Self-contained**: a cold subagent can do it from its prompt alone - goal, exact files or
  directories it owns, the rules that apply, what "done" means and how to verify it.
- **Small enough to finish in one agent run**, and **big enough to be worth an agent** - merge
  trivial one-line edits into a neighbouring task instead of dispatching them alone.
- **Owned**: every task lists the files it may write. Two tasks that write the same file are
  either merged into one, or ordered (one depends on the other).

For each task record: id (`T1`, `T2`, ...), title, files it writes, files it only reads, and
`needs:` - the ids whose RESULT it requires (a function it will call, a file it will edit after
them, a decision it builds on). Sharing a read-only dependency is not a `needs:`.

If the request is really one indivisible task, run it as one task and say that parallelism does
not apply - never split artificially.

## Done when

- The startup rule ran, `AGENTS.md` and the skills the request touches were read.
- `git status`, `git branch --show-current` and `git worktree list` were checked and the baseline is saved.
- If a wave will mutate files, you are on a non-protected feature branch.
- Every task has an id, a title, files it writes, files it reads and `needs:`. No two tasks write the same file without an order between them.
- An indivisible request is one task and you said that parallelism does not apply.

Next: [steps/02-waves.md](02-waves.md)
