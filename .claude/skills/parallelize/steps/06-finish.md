# Step 6 of 6 - Finish, validate and report

Part of the [parallelize](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never push unless the user asked. Never describe an unrun check as done.

Run the verification the request implies on the final tree (after the last wave), then follow the
end-of-task rules from `AGENTS.md`: the success-log entry for the task and, if anything is open,
the error-log entry, and a commit through the `generate-commit` skill when the repository's rules
make one part of every task.

When the merged result qualifies as a large change, run [audit-code](../../audit-code/SKILL.md) on it first;
the command owns the threshold.

### Validation

- Every task has an owner list, and no two tasks of a wave write the same file.
- No mutating wave ran on `main` or another protected branch.
- Every worktree worker that committed did so through `generate-commit`; nothing was pushed unless
  the user asked.
- Every verification in the final report was actually run, with real counts.

### Final report

A table (task, wave, isolation, agent result, merged or not), the order the waves ran in, the
wall-clock win you expect versus running serially (one line), every verification with real counts
(what passed, failed, or did not run), every deviation from the plan, and anything left open.

## Done when

- The final verification ran with real counts.
- The success-log entry is written, and the error-log entry when something is open.
- The final report has the table, the wave order, the verifications and what is left open.

This is the last step. Do not start another task on your own.
