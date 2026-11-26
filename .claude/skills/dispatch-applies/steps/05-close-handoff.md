# Step 5 of 6 - Close a handoff out

Part of the [dispatch-applies](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Run this step only when a handoff has every row `[x]` or `[-]`. Otherwise go to step 06.

`<base>` may have moved while the handoff's root branch lived, so merge `<base>` into the root
branch FIRST, resolving conflicts by the same rule as step 2 above (metadata.yaml, when it exists: base's value
plus every one of this handoff's own bumps in order, build = base's build plus the handoff's total
non-merge commit count; obvious parallel additions keep both sides; anything else, stop and ask).
Re-run the full verification recipe the handoff's own `_LOG_HANDOFF.md` section names (or, absent
one, what its applies' own `## Verification` sections together imply) on the MERGED root before
going further - a combined tree can break in ways no single PR did. Only then, from `<root-branch>`:
`lua tools/marea-ship/main.lua open-pr --base <base> --title "<handoff title>" --issue <root-n> --merge`,
and confirm that the root issue is closed.

## Done when

- `<base>` was merged into the root branch and conflicts were resolved by the rule.
- The full verification recipe ran on the merged root, with real counts.
- The handoff PR is merged, the root branch is deleted (local and remote) and the root issue is closed.

Next: [steps/06-bookkeeping-and-report.md](06-bookkeeping-and-report.md)
