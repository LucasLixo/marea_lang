# Step 6 of 8 - Stage 3 - Bug hunt

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Every bug gets a failing test first, then the minimum fix, one commit per bug through generate-commit.

Read for memory bugs:

- **C**: null checks, ownership and double free, use after free, leaks on error paths, buffer bounds, integer overflow in size math, uninitialized reads.
- **Rust**: every `unsafe` block has a `SAFETY` reason that holds, raw-pointer lifetimes, FFI ownership, and the raw read, write and reference rules of [rust-style](../../rust-style/SKILL.md) ("Raw pointer reads, writes and references"): the checks before an access, no long-lived `&mut` from an ABI pointer, `MaybeUninit`, and the tests for the null, misaligned, short and overlapping cases.
- **Tooling scripts (tools/)**: nil access, accidental globals, unchecked fallible results, off-by-one.

- **Shell and PowerShell scripts** - read for silent errors: a `mktemp` or temp-folder result that is never checked, or a temp folder with no `trap` (sh) or `finally` (PowerShell) that removes it; `set -e` without `pipefail`, or a `cd` without a check; unquoted variables, and an empty glob that expands to the literal pattern; the exit code of an external tool that is never checked (`$?` in sh, `$LASTEXITCODE` in PowerShell); a tool that exits 0 and leaves an empty or partial output file; an output directory that does not exist or cannot be created; a `|| true` that hides a failure.

And for logic bugs: inverted conditions, a wrong early return, state not reset, ignored error codes.

For Rust files with `unsafe`, run the narrow Miri check (`mise run setup:rust-miri` once, then `cargo +nightly miri test <filter>` from `lib/rust/`, as [rust-style](../../rust-style/SKILL.md) documents) when `rustup` has the nightly toolchain; otherwise say it was not run. Sanitizers are opt-in in `cmake/settings/ConfigureLlvm.cmake`; run one only when asked.

Each confirmed bug: write the failing test first, then the minimum fix, one commit per bug through `generate-commit`.

## Done when

- Each file was read for the memory bugs of its language and for logic bugs.
- Each confirmed bug has a test that failed first, a fix and one commit.
- Each shell or PowerShell script was read for the silent errors listed above.
- Miri ran for Rust files with `unsafe`, or the report says it did not run.

Next: [steps/07-formatter.md](07-formatter.md)
