# Canonical Documents Map

Link map of every canonical agent-instruction document and the one subject each owns. Update it in the same change that adds, renames or removes a document. Paths are relative to this file.

`CLAUDE.md` at the repository root is a symlink to `AGENTS.md`, and the entries of `.github/skills/` are symlinks to `.claude/skills/<name>/`. They are never edited as copies; edit the target.

## AGENTS file

| Document | Owned subject |
| --- | --- |
| [AGENTS.md](../../../../AGENTS.md) | Project description, a pointer to the compiler and language specifications under `docs/` (the specifications themselves are owned by those docs, not by this file), working model (commits through generate-commit, TDD, ASCII, copyright header), context economy, the two outcome logs, versioning, the table of skills |

## Skills

| Skill | Owned subject |
| --- | --- |
| [add-dependency](../../add-dependency/SKILL.md) | Adding a 3rdparty library or a Cargo crate: permissive-license rule, restrictive-license ban across the tree, most permissive choice, the vendoring steps |
| [audit-code](../../audit-code/SKILL.md) | Post-apply code audit and the threshold for running it after a large change |
| [c-style](../../c-style/SKILL.md) | C11 conventions, the MAREA_ type and macro system, opaque structs with accessors, the C and Rust boundary |
| [cmake-style](../../cmake-style/SKILL.md) | CMake layout, variables, options, targets, 3rdparty wiring and formatting |
| [cpp-testing](../../cpp-testing/SKILL.md) | GoogleTest conventions under `tests/`, the quiet build and test commands, and TDD for C symbols |
| [dispatch-applies](../../dispatch-applies/SKILL.md) | Running several existing applies in parallel worktrees, one fresh agent per part, and merging their PRs |
| [generate-apply](../../generate-apply/SKILL.md) | Apply files and parts, how a part is delegated and delivered (push, PR, merge), the apply list, the outcome logs (`temp/_LOG_APPLY_ERROR.md`, `temp/_LOG_APPLY_OPEN_POINTS.md`), headings and retention |
| [generate-commit](../../generate-commit/SKILL.md) | Commit mechanics, atomic grouping, the `metadata.yaml` bump |
| [generate-handoff](../../generate-handoff/SKILL.md) | Handoff dependency graph and the handoff plan log (`temp/_LOG_HANDOFF.md`) |
| [generate-issue](../../generate-issue/SKILL.md) | Creating one GitHub issue from `.github/ISSUE_TEMPLATE` and its report |
| [instruction-governance](../SKILL.md) | Changes to skills (workflow skills included) and to AGENTS.md: comparison, conflict handling, cross-references, when a skill is written in parts, and the manual validation checks |
| [parallelize](../../parallelize/SKILL.md) | Ephemeral delegation, waves and isolation |
| [reconcile-temp-logs](../../reconcile-temp-logs/SKILL.md) | Verified reconciliation of apply files and temp logs, and the `temp/_BACKUP/` folder |
| [request-make-apply](../../request-make-apply/SKILL.md) | Sizing a request (large, medium, small) and delivering it in an isolated worktree of this repository |
| [rust-style](../../rust-style/SKILL.md) | Rust binding crate conventions (no Rust code exists yet) |
| [stage-docs](../../stage-docs/SKILL.md) | The one reference doc per compiler stage under `docs/`: its fixed section pattern and how `## Gaps` relates to the outcome logs |
