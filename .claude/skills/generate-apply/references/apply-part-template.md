# Apply part template

An apply is split into parts so the executing session reads one part file at a time instead of
the whole task at once - the same rationale as a skill written in parts, owned by
[instruction-governance](../../instruction-governance/SKILL.md) ("Workflow skills"). This file
states only what is specific to an apply; it does not restate that rationale.

## Folder

```
temp/_APPLY/<slug>.apply.md
temp/_APPLY/_PART/<slug>/
  01-<first-part>.md
  02-<second-part>.md
  ...
  NN-<last-part>.md
```

## The index file (`<slug>.apply.md`)

Holds every section of [steps/03-write-apply-file.md](../steps/03-write-apply-file.md)'s template
except `## Steps` and `## Pipeline` - those move into parts - plus a `## How to run` and a
`## Parts` table. No part's body is restated here; the index is read once and kept in mind, the
parts are read one at a time.

```md
## How to run

Run the parts in order. Each part file under `_PART/<slug>/` is a complete prompt for that part
alone.

1. Open ONE part file, read it fully, and do exactly what it says.
2. Check its "Done when" list. Fix any item that fails before you move on.
3. Only then open the next part. Never read ahead, never merge parts and never skip one.
4. In an apply of four parts or more, run each part that carries the task's own Steps in a fresh
   `general-purpose` subagent, one at a time, in this same tree. Give it the paths of the index and
   of the part, and what the earlier parts reported. Take back at most 15 lines: commits, each check
   with its real counts, deviations, and what the next part needs. Run every other part yourself. A
   session that is itself a subagent runs every part itself.

## Parts

| Part | File | Purpose |
| --- | --- | --- |
| 1 | [_PART/<slug>/01-<first-part>.md](_PART/<slug>/01-<first-part>.md) | <what it does> |
```

## A part file

```md
# Part 1 of N - <title>

Part of the [<slug>](../../<slug>.apply.md) apply. This file is a complete prompt for this part
alone: do what it says, check "Done when", then open the next part.

## Rules for this part

- Only the rules this part uses.

<The actions, concrete and in order.>

## Done when

- An observable condition that proves the part is finished.

Next: [_PART/<slug>/02-<next-part>.md](02-<next-part>.md)
```

The last part ends with `This is the last part. Do not start another task on your own.` in place
of the `Next` line.

## Default part breakdown

Decide the split the same way a skill is split (see instruction-governance's "Workflow skills"):
one part per gated, ordered unit, never past about 80 lines. The common shapes:

**No `## Pipeline`** (a single simple edit, per [steps/04-write-parts.md](../steps/04-write-parts.md)'s rule):

1. `01-implement.md` - the task's own Steps, committed through `generate-commit`.
2. `02-finish.md` (last part) - the "On finish" bookkeeping below.

**Standalone apply (`handoff:` empty)**:

1. `01-setup.md`:
   ```
   1. `mise trust`, then `git pull --rebase` on `<base>` (dirty tree: `git fetch` plus
      `git merge --ff-only origin/<base>`). `git submodule status` must show GoogleTest
      initialized; if it does not, run `mise run setup:git-modules`.
   2. Open the issue with [generate-issue](../../generate-issue/SKILL.md), steps 1 to 3 only:
      populate `.github/ISSUE_TEMPLATE/<label>.md`, run `gh issue create`, write the report under
      `temp/_ISSUE/`. Record `<n>` in the index file's `issue:` field and in `../../../_LIST_APPLY.md`.
   3. `git branch <label>/<n> <base>` then `git switch <label>/<n>`.
   ```
2. `02-implement.md` (one part, or several when the task's own Steps are themselves multi-phase):
   the task's own Steps, calling `generate-commit` periodically - at every compilable increment,
   not once at the end.
3. `03-open-pr.md`:
   ```
   1. Run the checks of the index's `## Verification` once more on the final tree, run
      [audit-code](../../audit-code/SKILL.md) over the files this apply changed and commit its
      changes through `generate-commit`.
   2. `lua tools/marea-ship/main.lua open-pr --base <base> --title "<title>"` - pushes the branch and
      opens the pull request. Its description is the list of the branch's commits and `Resolves #<n>`,
      so write no `--body` unless the reviewer needs something the commit messages do not say.
   ```
   Ends exactly at `open-pr`, without `--merge` - nothing after it.
4. `04-merge-and-finish.md` (last part):
   ```
   1. `lua tools/marea-ship/main.lua merge --base <base>` - merges the pull request, closes the issue,
      switches to `<base>`, fast-forwards it and deletes the branch, local and remote. Do by hand
      what a `WARNING:` line says was not done.
   ```
   followed by the "On finish" bookkeeping below.

**Handoff batch (`handoff:` filled)**: the same four parts, with these differences:

- `01-setup.md` first reads `../../../_LOG_HANDOFF.md`'s "<exact handoff title>" section. If it
  already carries a "Root issue:" and "Root branch:" line (a `TBD` value counts as absent), read
  `<root-n>` and `<root-branch>` from them and skip to this batch's own issue and branch.
  Otherwise this is the first batch to run - create both now: `mise trust` and `git pull --rebase`
  on `<base>`; open the root issue with [generate-issue](../../generate-issue/SKILL.md), steps 1
  to 3 only, titled after the handoff itself; record its number as `<root-n>` on a new "Root
  issue: #<root-n>" line in `_LOG_HANDOFF.md`'s matching section; `git branch <label>/<root-n>
  <base>` then `git push -u origin <label>/<root-n>` (pushed immediately, empty, so later batches
  have a remote to track); record `<label>/<root-n>` as `<root-branch>` on a new "Root branch:
  `<label>/<root-n>`" line in the same section. Then `git pull --rebase` on `<root-branch>`, open
  this batch's issue with `--parent <root-n>` so GitHub files it as a sub-issue, and
  `git branch <label>/<n> <root-branch>` then `git switch <label>/<n>`.
- `03-open-pr.md` targets `<root-branch>`: `... open-pr --base <root-branch> --title "<title>"`.
- `04-merge-and-finish.md` runs `... merge --base <root-branch>`, which leaves the tree on
  `<root-branch>`. Then, only when `../../../_LIST_HANDOFF.md`'s "<exact handoff title>" section now
  has every row `[x]` or `[-]`, it closes the handoff out with
  `lua tools/marea-ship/main.lua open-pr --base <base> --title "<handoff title>" --issue <root-n> --merge`.
  A handoff has no outcome log of its own - this closing apply's own per-apply bookkeeping below
  (`../../../_LOG_APPLY_ERROR.md`/`../../../_LOG_APPLY_OPEN_POINTS.md`) is the handoff's outcome
  record too, per [generate-handoff](../../generate-handoff/references/outcome-logs.md).

`<root-n>` and `<root-branch>` stay literal placeholders in every handoff-batch part, resolved the
same way `<n>` already is: by whichever session executes `01-setup.md` first, and read back by
every session after.

**The contract `dispatch-applies` relies on**: whatever the split, the LAST part is always the one
that merges, closes the issue, cleans up branches and does the bookkeeping, and it is always the
only part a dispatch-applies worker must never open - see
[dispatch-applies](../../dispatch-applies/steps/03-launch-workers.md).

Four points every part that carries pipeline content must honour, because this is where the
surrounding rules meet:

- **The `generate-commit` skill never pushes and never creates a branch.** The branch is made with
  `git`, and the push, the pull request and the merge with
  [marea-ship](../../../../tools/marea-ship/main.lua), directly by the part - handing them to the
  commit skill contradicts it.
- **Branch in place, not a worktree.** The report `generate-issue` writes suggests
  `git worktree add`; the apply's parts stay in the same tree with `git switch`. A part borrows
  only the issue creation from that command, not its command sheet.
- **A bare `git rebase` is not on the allow list** of `.claude/settings.json`. `git pull --rebase` is
  a different command; never emit a bare `git rebase`.
- **A handoff batch's target is the root branch, never `<base>` itself.** Every `--base`, and the branch-point of every `git branch <label>/<n> ...`, is `<root-branch>` - only
  the handoff-closeout action of the last part ever touches `<base>` directly, and only once, for
  the whole handoff.

Substitute `<base>` for the real base branch (`develop` unless the request says otherwise) when
writing the parts - the executing session should find a branch name, not a variable.

## On finish

The tail of the LAST part, after whatever merge/cleanup actions the variant above adds:

1. Tick this apply's row in `../../../_LIST_APPLY.md` and fill its Issue column. Before writing
   anything new, check `../../../_LOG_APPLY_OPEN_POINTS.md` and `../../../_LOG_APPLY_ERROR.md`
   ([logs.md](logs.md)) for an entry this apply resolved, and remove that entry's mention as part
   of finishing - never because the apply is merely done, only because the thing it described is
   now actually resolved. Then, only when something qualifies, record an error entry in
   `../../../_LOG_APPLY_ERROR.md` (a confirmed bug or broken check seen during this apply) and an
   open-points entry in `../../../_LOG_APPLY_OPEN_POINTS.md` (a deferred feature, a known gap, a
   decision waiting on the user, a harmless deviation, a check not run) - the list holds only the
   table, never prose.
2. When the `handoff:` header field is filled, tick this apply's row in that section of
   `../../../_LIST_HANDOFF.md` too and fill its Issue column. The two lists stay in step - an
   apply belonging to a handoff is never ticked in only one.
