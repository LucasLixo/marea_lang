# Step 3 of 6 - Choose the concurrency and launch the workers

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- One worktree per apply, one fresh `general-purpose` agent per slot, never two agents of one apply at once.
- Never pass `isolation` to the agent: it would give each part a new, empty worktree.
- Start the first slot of every apply of the round in the same turn; start the next slot of an apply
  when its previous one reports.
- If the first worker reports that worktrees do not build, start no other apply.

**Concurrency.** Check `nproc` and free memory, and weigh what the applies' `## Verification`
sections build. **3 applies at once** is a reasonable default; lower for build-heavy applies on a
small machine, higher only for light ones.

**Worktrees.** From the main checkout, for each apply: `git fetch`, then
`git worktree add --detach .claude/worktrees/<slug> origin/<its base>`. The base is `<base>`, or the
root branch of its handoff when that exists.

**Slots.** Take the apply's `## Parts` table without the LAST part. Each part that carries the
task's own Steps is one slot. The setup part joins the first of these slots and the open-pr part
joins the last one, so no agent starts only for a short part.

**The prompt of each slot**, with every `<...>` filled in:

"""
You run <the part files of this slot, absolute paths> of ONE apply of this repository, and no other
part. Do not open the next part: another agent runs it.
Your worktree is `<absolute worktree path>`. Start EVERY shell command with `cd <that path> &&` and
read and edit only files under it, except the ones under `temp/` named here. Read `AGENTS.md` of the
worktree, the apply's index `<absolute path>` (its Context, Conventions, Files and Do not touch) and
the skills your parts cite.
Earlier parts reported: <their reports, pasted; "nothing, you are first" for the first slot>.

Overrides:
(1) The worktree starts detached at `origin/<its base>`. Do not switch to or pull `<base>` or the root
    branch; make the issue branch with `git switch -c <label>/<n> origin/<its base>`.
(2) In the first slot only: `mise trust`, then `mise run setup:git-modules`, then the build of the
    apply's `## Verification` (by default `mise run dev:quiet-test`). If it will not build, STOP and
    report the exact error.
(3) <only for a handoff with no root yet, first slot of the one apply chosen to go first: "create the
    root issue and branch as your setup part says, from `origin/<base>`, and push it empty first">
(4) Do not edit anything under `temp/`. Report what you would have written there.
(5) Commit only through the `generate-commit` skill; leave the tree clean. Never merge.
(6) Follow the repository rules and the skills (TDD, ASCII, copyright header). Verify the apply's own
    claims against the real code, correct the minimum needed and report every deviation. Never
    declare success without having seen a command's real output.

Report at most 15 lines: root issue and branch if you created them; issue, branch and PR; commits
(hash and subject); each check with its real counts; deviations; what the next part needs.
"""

When a worker reports, check that `git -C <worktree> status --porcelain` is empty before you start
the next slot. Give the next one every earlier report of that apply.

## Done when

- `nproc` and free memory were checked and the concurrency was chosen.
- Each apply has its own worktree, and each of its slots ran in a fresh agent, in order.
- Every apply of the round either reported its PR or stopped with a reported reason.

Next: [steps/04-merge-phase.md](04-merge-phase.md)
