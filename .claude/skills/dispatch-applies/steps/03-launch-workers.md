# Step 3 of 6 - Choose the concurrency and launch the workers

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Send all Agent calls of the round in the same turn, each with `isolation: "worktree"`.
- If the first worker reports that worktrees do not build, do not launch the rest.

No fixed number is correct for every machine or every apply - a round of full CMake
rebuilds and a round of doc-only edits do not cost the same. Before picking how many workers run
at once, check `nproc` and free memory, and weigh what the named applies' own `## Verification`
sections actually build. A commonly reasonable default is **3 concurrent worktree-isolated
workers**; go lower if the applies are build-heavy and the machine is small, higher only if they
are light (docs, single-file edits) and the machine has room. Never launch more workers than there
are arguments. If the FIRST worker's report says worktrees do not build at all on this machine, do
not launch the rest - report the error verbatim instead of guessing around it.

Launch one isolated agent per apply (parallel calls in the same turn), each with `isolation:
"worktree"` and this prompt, `<slug>` filled in:

"""
You execute ONE apply of this repository. Read `<absolute path to AGENTS.md>` and
`<absolute path to temp/_APPLY/<slug>.apply.md>` (the index - follow its own "## How to run") and
the skills it cites under `<absolute path to .claude/skills/>`. The index points at ordered part
files under `<absolute path to temp/_APPLY/_PART/<slug>/>`; open and run them one at a time, in
order, exactly as the index says. You run in an ISOLATED WORKTREE.

Pipeline overrides:
(1) First `mise trust`, then `mise run setup:git-modules` to initialize the submodules (today
    only `3rdparty/GoogleTest`; never hand-roll `git submodule update`). Then build as the apply's
    `## Verification` says (`mise run config:cmake`, then `cmake --build cmake-build-debug
    --target marea marea_tests`). If the worktree will not build, STOP and report the exact
    error - do not improvise around it.
(2) If this apply's `handoff:` field is empty: run its parts up through the one that opens the PR
    against `<base>` (the standalone variant's `open-pr` part) - but never the LAST part. If it is
    filled: <either "a root issue/branch already exist - #<root-n> / `<root-branch>`, pushed; do
    not create them, branch from `<root-branch>` and open your PR against it" or, for the one
    worker chosen to go first, "no root exists yet for this handoff - create it per your apply's
    own `setup` part, from `<base>`, and push it empty before doing anything else">. Either way,
    run the parts only through the one that opens the PR - do NOT open the apply's LAST part: it
    is the one that merges, closes your issue, and cleans up branches, and it is always the
    orchestrator's, never yours.
(3) Do not edit anything under `temp/` - it does not exist in your worktree, and several workers
    editing it at once would corrupt it. Everything you would have written there goes in your
    final report instead.
(4) Commit only through the `generate-commit` skill, never a hand-written `git commit`. When
    `metadata.yaml` exists, bump it on your own branch exactly as the apply's
    `## Conventions`/pipeline text says, before your first build; today it does not exist, so skip
    that. Its grouping rule already keeps out any path that was dirty in the main checkout before
    you started (a submodule pointer, a build output); do not fight it.
(5) Follow the repository rules and the skills the apply cites (TDD, ASCII, the copyright header
    on a new file). Verify the apply's own claims against the real code; correct the minimum
    needed and report every deviation. Never declare success without having seen a command's
    real output.

Final report, SHORT: root issue/branch if you created them; your own issue, branch, and PR (with
its state); every commit (hash and subject); the bump kind and commit count and the final
`metadata.yaml` line (both only when the file exists); each verification's result with real counts (say plainly what failed or did
not run); every deviation from the plan; and anything the next apply in this handoff needs to
know.
"""

## Done when

- `nproc` and free memory were checked and the concurrency was chosen.
- One worker per apply was launched with the worker prompt, `<slug>` filled in.
- Every worker of the round has reported.

Next: [steps/04-merge-phase.md](04-merge-phase.md)
