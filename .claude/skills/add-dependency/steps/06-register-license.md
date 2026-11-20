# Step 6 of 7 - Register the license, test first

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- TDD: the test fails first, then the registration.
- Never add a `license_embed_add` outside an `if`.
- Alphabetical order inside the block. `TYPE` is one id.

Write the registration in `cmake/dependencies/ConfigureLicenseEmbedCMake.cmake`, test first.

### 1. The failing test

`cmake/tests/test_license_embed_gating.cmake` runs the module with every variable off except the ones a scenario names, and records which names `license_embed_add` registered.

1. Add every new variable of step 03 (the new `MAREA_OPTION_*` or platform variable) to its `MAREA_TEST_VARIABLES` list.
2. Add two scenarios: one where the condition is on, with `marea_test_expect_present` for the new names; one where the condition is off, with `marea_test_expect_absent`. For a build-type condition, add a scenario for each build type.
3. Run `mise run test:cmake` and see the new scenario fail. Only then write the registration.

### 2. The registration

Add one `license_embed_add(NAME "<Name>" TYPE "<SPDX-id>" PATH "...")` call per license row of step 02, **inside the `if` of step 03**:

```cmake
if(MAREA_LICENSE_ROOT_EXAMPLE)
    license_embed_add(NAME "example" TYPE "MIT" PATH "${MAREA_FOLDER_LIB_RESOURCES}/licenses/Example.txt")
endif()
```

- `TYPE` is the single selected SPDX id of step 02, never an `OR` expression.
- `PATH` points at the real license file: the file of step 05 for a crate, the dependency's own `LICENSE` or `COPYING` for a 3rdparty library (`${MAREA_FOLDER_3RDPARTY}/<Name>/LICENSE`).
- A **crate** goes in the block that starts with the comment `# Rust dependency licenses`, alphabetical by crate name. Keep the block without blank lines: `tools/marea-license-check` reads it up to the first blank line.
- A **3rdparty library** goes in the 3rdparty block, alphabetical by `NAME`.
- A nested library that the build compiles in gets its own call with its own `if`.
- Run `mise run format:cmake-sources` (the gersemi layout) after the edit.

### 3. The test goes green

Run `mise run test:cmake` again. Both scenarios pass.

## Done when

- The new scenarios failed before the registration and pass after it.
- Every license row is registered inside an `if` with the condition of step 03.
- Crate entries are inside the `# Rust dependency licenses` block with no blank line.
- `mise run format:cmake-sources` ran.

Next: [steps/07-verify-and-report.md](07-verify-and-report.md)
