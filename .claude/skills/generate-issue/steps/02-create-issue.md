# Step 2 of 3 - Create the issue

Part of the [generate-issue](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- `--body` is plain Markdown with real newlines. English. At least one label.

Load the template `.github/ISSUE_TEMPLATE/<primary-label>.md`. Replace every placeholder (`<!-- ... -->`) with real details. Then run `gh issue create --title "..." --body "<populated>" --label "<primary-label>"`.

## Done when

- `gh issue create` printed an issue URL.
- No placeholder (`<!-- ... -->`) is left in the issue body.

Next: [steps/03-write-report.md](03-write-report.md)
