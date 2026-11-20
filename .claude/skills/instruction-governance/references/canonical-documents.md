# Canonical Documents Map

Link map of every canonical agent-instruction document and the one subject each owns. Update it in the same change that adds, renames or removes a document. Paths are relative to this file.

The Creator `.agents/` entries (skills) are symlink aliases into the engine `marea/.agents/` folders. They are never edited as copies; edit the engine original.

## AGENTS files

| Document | Owned subject |
| --- | --- |
| [Engine AGENTS.md](../../../../AGENTS.md) | Engine startup, dirty baseline, ASCII, copyright, truthful reporting, language-boundary summary |

## Skills

| Skill | Owned subject |
| --- | --- |
| [add-dependency](../../add-dependency/SKILL.md) | Adding a 3rdparty library or a Cargo crate: permissive-license rule, restrictive-license ban across the tree, most permissive choice, the `license_embed_add` registration inside an `if`, the vendoring steps, the duplicate-crate scan of `Cargo.lock` before and after a Cargo change, and the user's choice between candidates of different weight |
| [audit-code](../../audit-code/SKILL.md) | Post-apply code audit: skill conformance, permission-map sync, minimal code, bug hunt, formatting; the threshold for running it after a large change |
| [c-style](../../c-style/SKILL.md) | C17 conventions, MAREA_ type system, Except enums, C/Rust boundary detail |
| [cmake-style](../../cmake-style/SKILL.md) | CMake layout, module skeleton, variables, targets |
| [cpp-testing](../../cpp-testing/SKILL.md) | GoogleTest conventions and C/C++ TDD |
| [dispatch-applies](../../dispatch-applies/SKILL.md) | Running several existing applies in parallel worktrees and merging their PRs |
| [doxygen-documentation](../../doxygen-documentation/SKILL.md) | C header XML authoring lifecycle (create, update, delete under docs/) |
| [engine-build](../../engine-build/SKILL.md) | Build, test and package invocation and recovery through mise tasks |
| [generate-apply](../../generate-apply/SKILL.md) | Apply files, apply lists, logs, headings, retention and pipeline schemas |
| [generate-commit](../../generate-commit/SKILL.md) | Commit mechanics, atomic grouping and metadata staging |
| [generate-handoff](../../generate-handoff/SKILL.md) | Handoff dependency graph and handoff logs |
| [generate-issue](../../generate-issue/SKILL.md) | Issue, branch and worktree creation and the PR flow |
| [instruction-governance](../SKILL.md) | Changes to skills (workflow skills included) and AGENTS files: comparison, conflict handling, cross-references, when and how a skill is written in parts, and the checks of `mise run test:skill-check` |
| [marea-docs-generator](../../marea-docs-generator/SKILL.md) | Header-to-XML parsing pipeline and its CLI |
| [marea-framework](../../marea-framework/SKILL.md) | The marea MVU framework, layer rule, screens and their lifecycle, panels, the lazy services aggregator and engine-facing services, element props |
| [mise-style](../../mise-style/SKILL.md) | `mise.toml` structure: task shape, naming, alphabetical order, descriptions, `dir`, usage flags, tools and env comments |
| [parallelize](../../parallelize/SKILL.md) | Ephemeral delegation, waves and isolation |
| [reconcile-temp-logs](../../reconcile-temp-logs/SKILL.md) | Verified reconciliation of apply files and temp logs (replaces cleanup-temp) |
| [request-make-apply](../../request-make-apply/SKILL.md) | Sizing an engine gap from the Creator and dispatching it (args: submodule path, request text) |
| [patch-vendored-source](../../patch-vendored-source/SKILL.md) | Vendor patches: `patches/<Project>/` layout and format, `ApplyPatch`, one `MAREA_OPTION_PATCH_*` option per patch, on/off verification, rebase on a pin bump |
| [rust-style](../../rust-style/SKILL.md) | Rust crate layout, ABI layout asserts, Rust-side language boundary, Rust test layout (mirrored files under lib/rust/tests) |
| [slang-shaders](../../slang-shaders/SKILL.md) | Slang shader authoring: the `<name>.<stage>.slang` convention, the stage by target matrix, `CompileShaders` skip rules, the SDL_GPU binding contract, embedding and per-platform format selection, the SPIR-V capability allow-list, verification on SwiftShader and lavapipe |
| [termux-copy](../../termux-copy/SKILL.md) | Detecting Tailscale/SSH/project facts and printing the Termux scp commands that copy named files to the phone (read-only) |
| [visual-verification](../../visual-verification/SKILL.md) | Building, running (headless or real display) and screenshotting a Marea GUI target to confirm a UI/layout/graphics change against a reference image |
