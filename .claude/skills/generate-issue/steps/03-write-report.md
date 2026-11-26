# Step 3 of 3 - Write the report

Part of the [generate-issue](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- This skill creates no branch, worktree or PR and cleans up nothing.

Write the report `temp/_ISSUE/<issue-number>-<label>.md` with the content shown below.

### Report template

The command list is a reference for the apply and dispatch commands that carry the issue forward; this skill runs none of it.

> **Why a PR, not a local merge:** GitHub scans a commit for `#N` references only on its first push, and links it only if that push lands on the default branch. A `feature/N` branch pushed first consumes the scan, so the issue never shows the commits. A Pull Request with `Resolves #<n>` links the commits, fills the issue's Development section, and auto-closes it on merge.

~~~md
# Report: <label>/<issue-number>

## Commands

```sh
# Creates a new local branch based on develop or your current branch, named after the issue label and number.
git branch <label>/<issue-number> [develop|current]

# Creates a separate physical directory (worktree) to work on your new branch without changing or disturbing your current directory.
git worktree add "../<label>-<issue-number>" <label>/<issue-number>

# Pushes the local branch to the remote repository (GitHub) for the first time and links it so future pushes require only git push.
git push -u origin <label>/<issue-number>

# Pushes the branch if needed and opens the Pull Request (PR) into the base branch with "Resolves #<issue-number>".
lua tools/marea-ship/main.lua open-pr --base <base> --title "<issue title>"

# Merges the PR, closes the issue, returns to the base branch and deletes the branch, local and remote.
lua tools/marea-ship/main.lua merge --base <base>

# Forces the removal of the isolated worktree directory from your machine after you are done with the task.
git worktree remove ../<label>-<issue-number> --force

# Fetches the latest updates from the remote repository and reapplies your local commits on top of them, keeping a clean and linear project history.
git pull --rebase
```
~~~

## Done when

- The report exists under `temp/_ISSUE/`, its name carries the number the URL printed, and it is ASCII only.

This is the last step. Do not start another task on your own.
