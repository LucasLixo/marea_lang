# Step 3 of 6 - Stage 2 - Minimal code

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Public API is frozen: only report a change to a symbol declared under `lib/include/marea/`.
- Never remove a test, security check, bound check or required header or doc to save lines.

With the tests green, remove dead code, duplicated branches and needless indirection, and apply the clean-function principles to non-exported code:

- **SRP**: a function does one thing; if describing it needs "and", split it.
- **SLAP**: one abstraction level per function; delegate low-level detail.
- **Command-Query Separation**: a function either changes state or returns information, never both. The project's own conventions (for example a function that reports an error through an out-parameter and returns a status) are not violations.
- **Pure functions** wherever possible: the result depends only on the inputs, with no external side effects, so a test needs no setup.
- **Reuse**: in non-exported C code, replace a hand-written routine that duplicates the C standard library or a function the repository already has. An exported function is only reported. Where [c-style](../../c-style/SKILL.md) states a rule for this, it wins.

Each change is behaviour-preserving, small, and followed by the covering tests. Delete or adapt tests of deleted code per the deletion rule.

## Done when

- Dead code was removed with the tests whose only subject it was.
- Every hand-written routine that duplicates the C standard library or an existing function was replaced, or reported when it is exported.
- Each change is behaviour-preserving and the covering tests are green after it.

Next: [steps/04-bug-hunt.md](04-bug-hunt.md)
