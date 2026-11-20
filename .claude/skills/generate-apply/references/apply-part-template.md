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
      `git merge --ff-only origin/<base>`).
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
   1. Run [audit-code](../../audit-code/SKILL.md) over the files this apply changed and commit its
      changes through `generate-commit`.
   2. `git push -u origin <label>/<n>`.
   3. `gh pr create --base <base> --head <label>/<n> --title "<title>" --body "Resolves #<n>"`.
   ```
   Ends exactly at `gh pr create` - nothing after it.
4. `04-merge-and-finish.md` (last part):
   ```
   1. `gh pr merge <label>/<n> --merge --delete-branch`.
   2. `gh issue close <n>` - a safety net, the merge normally closes it already.
   3. `git switch <base>` then `git pull --rebase`.
   4. `git branch -D <label>/<n>`, and `git push origin --delete <label>/<n>` if the remote
      branch survived the merge.
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
- `03-open-pr.md` targets `<root-branch>`: `gh pr create --base <root-branch> --head <label>/<n> ...`.
- `04-merge-and-finish.md` merges into `<root-branch>` (not `<base>`), switches back to
  `<root-branch>` and pulls it, then, only when `../../../_LIST_HANDOFF.md`'s "<exact handoff title>"
  section now has every row `[x]` or `[-]`, closes the handoff out: open
  `gh pr create --base <base> --head <root-branch> --title "<handoff title>" --body "Resolves #<root-n>"`,
  merge it, `git switch <base>` then `git pull --rebase`, delete `<root-branch>` (local and remote)
  the same way a batch branch is deleted, add a closing entry to `../../../_LOG_HANDOFF_SUCCESS.md`'s
  matching section (and a matching `../../../_LOG_HANDOFF_ERROR.md` entry if anything real is still
  open) - before the per-apply bookkeeping below.

`<root-n>` and `<root-branch>` stay literal placeholders in every handoff-batch part, resolved the
same way `<n>` already is: by whichever session executes `01-setup.md` first, and read back by
every session after.

**The contract `dispatch-applies` relies on**: whatever the split, the LAST part is always the one
that merges, closes the issue, cleans up branches and does the bookkeeping, and it is always the
only part a dispatch-applies worker must never open - see
[dispatch-applies](../../dispatch-applies/steps/03-launch-workers.md).

Four points every part that carries pipeline content must honour, because this is where the
surrounding rules meet:

- **The `generate-commit` skill never pushes and never creates a branch.** The `git`/`gh` actions
  above belong to the parts and are run directly - handing them to the commit command contradicts
  it.
- **Branch in place, not a worktree.** The report `generate-issue` writes suggests
  `git worktree add`; the apply's parts stay in the same tree with `git switch`. A part borrows
  only the issue creation from that command, not its command sheet.
- **`git rebase` is denied** in `.claude/settings.json`. `git pull --rebase` is a different command
  and is allowed; never emit a bare `git rebase`.
- **A handoff batch's target is the root branch, never `<base>` itself.** Every `gh pr create
  --base`, and the branch-point of every `git branch <label>/<n> ...`, is `<root-branch>` - only
  the handoff-closeout action of the last part ever touches `<base>` directly, and only once, for
  the whole handoff.

Substitute `<base>` for the real base branch (`develop` unless the request says otherwise) when
writing the parts - the executing session should find a branch name, not a variable.

## On finish

The tail of the LAST part, after whatever merge/cleanup actions the variant above adds:

1. Tick this apply's row in `../../../_LIST_APPLY.md` and fill its Issue column. Always record
   a success entry in `../../../_LOG_APPLY_SUCCESS.md` (verification counts, a harmless deviation,
   a fact the next apply can rely on) and, only when something is left open, wrong, or
   inconvenient, also an error entry in `../../../_LOG_APPLY_ERROR.md` - the list holds only the
   table, never prose.
2. When the `handoff:` header field is filled, tick this apply's row in that section of
   `../../../_LIST_HANDOFF.md` too and fill its Issue column. The two lists stay in step - an
   apply belonging to a handoff is never ticked in only one.
