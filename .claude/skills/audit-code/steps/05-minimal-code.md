# Step 5 of 8 - Stage 2 - Minimal code

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Public API is frozen: only report a change to an exported symbol.
- Never remove a test, security check, bound check or required header or doc to save lines.

With the tests green, remove dead code, duplicated branches and needless indirection, and apply the clean-function principles to non-exported code:

- **SRP**: a function does one thing; if describing it needs "and", split it.
- **SLAP**: one abstraction level per function; delegate low-level detail.
- **Command-Query Separation**: a function either changes state or returns information, never both. The framework's own conventions (`marea.core.result` returns, the MVU `update` returning the new model) are not violations.
- **Pure functions** wherever possible: the result depends only on the inputs, with no external side effects, so a test needs no setup.
- **Source independence**: in non-exported C and Rust code, replace a hand-written routine that duplicates a primitive higher in the order of the "Source independence" rule ([c-style](../../c-style/SKILL.md), [rust-style](../../rust-style/SKILL.md)): SDL3 first, then the standard library, then a crate or engine function that already exists, and only then new code. The numeric-math exception in those skills still holds. An exported function is only reported.

Each change is behaviour-preserving, small, and followed by the covering tests. Delete or adapt tests of deleted code per the deletion rule.

## Done when

- Dead code was removed with the tests whose only subject it was.
- Every hand-written routine that duplicates a higher primitive was replaced, or reported when it is exported.
- Each change is behaviour-preserving and the covering tests are green after it.

Next: [steps/06-bug-hunt.md](06-bug-hunt.md)
