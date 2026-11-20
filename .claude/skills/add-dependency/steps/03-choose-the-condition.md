# Step 3 of 7 - Choose the condition of the license registration

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never write an unconditional `license_embed_add`.
- The condition must be true exactly when the dependency is linked. A condition that is too wide ships a license that is not used. A condition that is too narrow hides one that is.

Every `license_embed_add` call goes inside an `if`. The condition says when the dependency is really linked, so a build that does not link it does not ship its license. Take the conditions from step 01 and write one `if` per license row of the table from step 02.

### The three kinds of condition

| Kind | When it applies | Variable | Existing example in `ConfigureLicenseEmbedCMake.cmake` |
| --- | --- | --- | --- |
| Platform | linked on some platforms only | `MAREA_PLATFORM_WINDOWS`, `MAREA_PLATFORM_ANDROID`, `MAREA_PLATFORM_LINUX`, `MAREA_PLATFORM_WEB` | the Windows-only crates (`windows-sys`, `windows-link`) |
| Build type | linked in Debug or in Release only | `CMAKE_BUILD_TYPE` in an `if()`. It decides what configure does, so it is not a generator expression (see "Generator expressions versus if()" in [cmake-style](../../cmake-style/SKILL.md)) | none yet: add the case to the test |

Conditions may be combined with `AND` and `OR`.

### For a Cargo crate

- Make one `MAREA_LICENSE_ROOT_<NAME>` variable for the cargo dependency, set from the option that turns its cargo feature on, in the block at the top of `ConfigureLicenseEmbedCMake`. The block mirrors the cargo features that `ConfigureRust` turns on.
- A crate that other crates bring in is registered under the `if` of every root that brings it, joined with `OR`.
- A target-specific crate (a Windows-only crate) uses the platform variable.

### For a 3rdparty library

- Use the condition that the library's own `Configure<Name>.cmake` uses to link it. Use the same variable, not a copy of its logic.
- A nested vendored library compiled in only with one option takes that option too.

### No honest condition

If the dependency is linked in every build and no variable describes it, do not write a bare `license_embed_add`. Stop and ask the user which `if` to use. The older unconditional entries (the SDL family and similar) stay as they are: this skill does not touch them.

## Done when

- Every license row has an `if` condition written down, with the variable names.
- For a crate, the `MAREA_LICENSE_ROOT_<NAME>` variable and the option that sets it are named.
- A case with no honest condition was sent to the user.

Next: [steps/04-add-3rdparty.md](04-add-3rdparty.md)
