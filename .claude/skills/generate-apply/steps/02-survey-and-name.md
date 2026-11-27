# Step 2 of 5 - Survey the code and pick the slug

Part of the [generate-apply](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Reading with `git`, `gh` and the file tools is expected. Writing is not allowed yet.

2. Survey the affected code before writing - the apply must name real files, functions, and tests. **A placeholder in an apply is a bug**: a cold session cannot resolve `<the relevant header>` into a path.
3. Check `temp/_LOG_APPLY_OPEN_POINTS.md` and `temp/_LOG_APPLY_ERROR.md` ([references/logs.md](../references/logs.md)) for an existing entry relevant to what this apply is about to build. Build on a still-open entry instead of re-discovering it, and name it in the apply's `## Context` when one applies.
4. Pick the slug and check `ls temp/_APPLY/` for a collision.

### Naming

`temp/_APPLY/<slug>.apply.md`, where the slug is lowercase kebab-case ASCII, 3 to 6 words, named after **what the task is** and specific enough to tell it apart from every sibling: `add-lexer-token-kinds`, `cover-diagnostic-sink-accessors`, `document-arena-api` - not `lexer-work`, `refactor`, `apply-2`.

Never a timestamp, never a number. On a collision that is genuinely a different task, sharpen the slug rather than appending a digit.

## Done when

- Every file, function and test the apply will name exists, and you saw it in the repository.
- No placeholder such as `<the relevant header>` remains in your notes.
- Both outcome logs were checked for a relevant still-open entry, and one was named in the apply's context when found.
- The slug is lowercase kebab-case ASCII, 3 to 6 words, and `ls temp/_APPLY/` shows no collision.

Next: [steps/03-write-apply-file.md](03-write-apply-file.md)
