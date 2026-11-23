# Step 4 of 6 - Stage 3 - Bug hunt

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Every bug gets a failing test first, then the minimum fix, one commit per bug through generate-commit. A bug in a `tools/` script gets a failing `test_*.lua` in that tool's `tests/` folder when the tool has one; when it has none, report the bug and the missing test, and fix it only with a test written first.

Read each C file for memory bugs:

- null checks and unchecked results of allocation or of a call that can fail;
- ownership: double free, use after free, a leak on an error path, an arena pointer kept after the arena is reset or freed;
- buffer bounds and off-by-one in loops and in string or token handling (a missing terminating zero, a read past the end of a source buffer);
- integer overflow in size or index math, signed overflow, a conversion between signed and unsigned or between widths that loses a value;
- uninitialized reads, and a struct returned or copied with an unset member;
- an opaque struct whose accessor exposes a pointer into storage that a later call can move or free.

And for logic bugs: inverted conditions, a wrong early return, state not reset between calls, ignored error codes, a diagnostic that is dropped, a `switch` without a `default` or without every enum member handled.

Read each script under `tools/` (Lua, shell and PowerShell):

- **Lua**: nil access, an accidental global, an unchecked fallible result (`io.open`, `io.popen`, `tonumber`, `pcall`), an off-by-one, a file or process handle that is not closed, a path built with a hard-coded separator.
- **Shell and PowerShell** - read for silent errors: a `mktemp` or temp-folder result that is never checked, or a temp folder with no `trap` (sh) or `finally` (PowerShell) that removes it; `set -e` without `pipefail`, or a `cd` without a check; unquoted variables, and an empty glob that expands to the literal pattern; the exit code of an external tool that is never checked (`$?` in sh, `$LASTEXITCODE` in PowerShell); a tool that exits 0 and leaves an empty or partial output file; an output directory that does not exist or cannot be created; a `|| true` that hides a failure.

No sanitizer or Valgrind task exists. Run one (for example a clang `-fsanitize=address,undefined` build in a scratch build folder) only when asked, and say in the report that it was not run otherwise. Never add a sanitizer to the repository build from this skill.

Each confirmed bug: write the failing test first, then the minimum fix, one commit per bug through `generate-commit`.

## Done when

- Each C file was read for the memory bugs listed above and for logic bugs.
- Each confirmed bug has a test that failed first, a fix and one commit.
- Each `tools/` script was read for the errors listed for its language.
- The report says plainly that no sanitizer ran, or names the one that ran and its result.

Next: [steps/05-formatter.md](05-formatter.md)
