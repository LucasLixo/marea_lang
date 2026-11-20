# Step 4 of 5 - Write the parts

Part of the [generate-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never mix the two pipeline variants. The `handoff:` header field of the index file chooses the variant.
- Follow [references/apply-part-template.md](../references/apply-part-template.md) for the folder, the part-file shape, and the default part breakdown. Do not invent a variant here.

Open the index file from step 03. Decide the part split the same way a skill is split (one part
per gated, ordered unit, never past about 80 lines - see the reference's "Workflow skills" link),
write each part file under `temp/_APPLY/_PART/<slug>/`, and fill the index's `## Parts` table to
match.

A single-file typo, a one-line correction, a doc wording fix needs none of the pipeline parts and
commits on the current branch - use the reference's two-part shape (`implement`, `finish`).
Anything that ships behaviour goes through the full four-part shape (`setup`, `implement`,
`open-pr`, `merge-and-finish`), choosing the standalone or handoff-batch variant by whether this
apply's `handoff:` field is filled. **Never mix them** - a handoff batch never merges into
`<base>` directly, and a standalone apply has no root branch to answer to. `generate-handoff`
always uses the handoff-batch variant, since every apply it writes carries `handoff:`.

The LAST part is always the one that merges, closes the issue, cleans up branches and does the
"On finish" bookkeeping - this is the contract [dispatch-applies](../../dispatch-applies/steps/03-launch-workers.md)
relies on to know which part a worker must never open. Keep that true regardless of how many
parts precede it.

## Done when

- `temp/_APPLY/_PART/<slug>/` holds one part file per row of the index's `## Parts` table, numbered consecutively from `01`.
- The index's `## Parts` table lists every part file that exists, in order, and no other.
- `<base>` is the real base branch name (`develop` unless the request says otherwise) in every part that names it.
- The four points of the reference's "Four points every part..." list are honoured by the parts that carry pipeline content, or the pipeline parts are absent for a single simple edit.
- `<root-n>` and `<root-branch>` remain literal in every handoff-batch part.
- The LAST part carries the merge/cleanup actions (when the pipeline parts exist) and the "On finish" bookkeeping.

Next: [steps/05-register-and-report.md](05-register-and-report.md)
