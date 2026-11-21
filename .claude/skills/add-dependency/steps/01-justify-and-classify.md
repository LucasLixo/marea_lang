# Step 1 of 4 - Justify the dependency and pick the upstream and version

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Do not edit any file in this step.
- The dependency is a last resort: if the C standard library or code that already exists in the repository does the job, stop and say so.

Decide whether the dependency is needed.

1. State in one sentence what the dependency does for the project and which library, tool or test target uses it.
2. Walk this order (see [c-style](../../c-style/SKILL.md)): the C standard library, then code that already exists here, and only then new code or a dependency. Search the repository first: `3rdparty/`, `.gitmodules` and `git grep` for the symbol. A dependency that already exists is reused, not added a second time.
3. When more than one candidate does the job, compare them with facts and not with reputation: license, size of the source tree, nested submodules, build options that turn on more code, and the date of the last release. Put the choice to the user, one option per candidate, with your recommendation first. Go on only with the answer.
4. Name the exact upstream URL and the exact version. Prefer a release tag. When upstream has no tag, name the branch.
5. Say which target links it (for example `marea_tests` only), and whether it is needed on every build or under an option.

## Done when

- You wrote the purpose, the upstream and the version (tag or branch).
- With more than one candidate, you compared them and the user chose.
- You searched the repository and the dependency does not exist yet, or you stopped.
- You know which target links it.

Next: [steps/02-license-check.md](02-license-check.md)
