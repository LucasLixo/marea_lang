# Step 1 of 8 - Stage 0 - Scope and baseline

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- A red baseline stops the audit and is reported, never fixed blindly.

### Which skill checks which file

| File | Skill (stage 1) |
| --- | --- |
| `.c` / `.h` under `lib/` | [c-style](../../c-style/SKILL.md) |
| `.rs` under `lib/rust/` | [rust-style](../../rust-style/SKILL.md) |
| `CMakeLists.txt` / `*.cmake` | [cmake-style](../../cmake-style/SKILL.md) |
| `mise.toml` | [mise-style](../../mise-style/SKILL.md) |

A file with no matching skill skips stage 1 and the report says so.

1. Resolve the argument list (or the branch diff), drop `3rdparty/`, generated files and `docs/`, and map every file to its skill with the table above.
3. A Rust source with an inline `mod tests {` or other test code under `#[cfg(test)]` is a conformance finding, except under `lib/rust/build/` and except the seams the `rust-style` section [Where tests live](../../rust-style/SKILL.md#where-tests-live) lists as allowed (logging seams, host gates, the crate root's two declarations, modules with no test module). Those seams are not findings; do not restate the list here.

## Done when

- The file list is resolved and `3rdparty/`, generated files and `docs/` are dropped.
- Every file is mapped to its skill, or marked as having none.
- The covering tests of each file are recorded and ran, and the baseline result is noted with real counts.

Next: [steps/02-conformance.md](02-conformance.md)
