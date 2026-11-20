# Step 5 of 7 - Add a Cargo crate (skip for a 3rdparty library)

Part of the [add-dependency](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Never hand-edit `Cargo.lock`.
- Never enable a crate feature that was not part of the step 02 license check.

Run this step only for a Cargo crate. For a 3rdparty library only, say "not applicable" and go to step 06.

1. **`lib/rust/Cargo.toml`**: add the crate under `[dependencies]`:
   - with `default-features = false` and the exact list of features this project uses, so the license check of step 02 matches the real build;
   - `optional = true` when only some API packages use it, and a feature in `[features]` that names it with `dep:<crate>`, mirroring the `api-*` features. A feature is the same switch that `ConfigureRust` turns on from the `MAREA_OPTION_API_<PKG>` option;
   - a comment above the line with the purpose, the license under which the crate is taken, the path of its license file under `lib/resources/licenses/`, and which module is the only one that touches it. Follow the comments of the existing entries (`getrandom`, `rustls`, `lopdf`).
2. **Duplicate scan**, before and after the manifest edit, from `lib/rust/`:
   - `cargo tree -d --all-features --target all -e normal,build` lists every crate that is built in more than one version. Save the list before the edit and again after it.
   - The edit must not add a crate to the list. When it does, find the cause with `cargo tree -i <crate>@<version> --all-features --target all -e normal,build`. Fix it in this order: align the version requirement of the direct dependency (a requirement such as `"0.8"` instead of `"0.8.44"` unifies compatible versions); move the other direct dependencies of the same family to the version that the rest of the tree uses, as the move of the crypto crates to the `lopdf` generation did; pick another crate; or stop and ask the user.
   - Features are additive: `default-features = false` in our manifest does not remove a feature that another crate asks for. Read what the other crates request before you count on a feature being off.
   - A duplicate that cannot be removed is allowed only when the report names its cause. Two causes are known: a crate that pins an older version in a release we cannot change (`ring` pins `getrandom` 0.2), and crates that appear only through build dependencies such as `bindgen` (`nom`, `shlex`, `syn`) and never reach the binary.
   - The `Cargo.lock` diff must show only the crates this change needs. Restore the old lockfile and let cargo resolve from the new manifest instead of running a full `cargo update`.
3. **Cargo.lock**: let cargo update the lockfile (`mise run build:rust-crate` or the mise task that builds the crate) and commit the lockfile with the manifest. Never edit it by hand.
4. **`ConfigureRust.cmake`**: when the crate has its own feature, check that `ConfigureRust` turns the feature on from the option of the package. Add the mapping only if the option-to-feature mirror does not already cover it.
5. **The license files**: every new package of the lockfile, the crate and the crates it brings in, needs one file `lib/resources/licenses/<PascalName>.txt`:
   - the file name is the crate name in PascalCase (`webpki-roots` becomes `WebpkiRoots.txt`);
   - when the crate offers `A OR B`, the file starts with the line `Selected license: <id> (crate offers <expression>)` and then the full text of the selected license with the copyright line of the crate, as `Rustls.txt` shows;
   - an `A AND B` crate ships every text;
   - the file is at least 100 bytes and has no `TODO` or `PLACEHOLDER`, because `marea-license-check` rejects those.
6. Use the crate from one module only when you can. Say in the `Cargo.toml` comment which module it is.

Registration of each license in `ConfigureLicenseEmbedCMake.cmake` is step 06.

## Done when

- The crate is in `Cargo.toml` with `default-features = false`, the list of features, a comment with license and purpose, and `optional = true` with a `dep:` feature when only some packages use it.
- The duplicate list after the edit has no crate that the list before did not have, or each new entry has a named cause the user accepted.
- `Cargo.lock` was updated by cargo and its diff holds only what this change needs.
- Every new package has its license file under `lib/resources/licenses/`, with the `Selected license:` line when the crate offers a choice.
- The feature is turned on by `ConfigureRust` from the right option.

Next: [steps/06-register-license.md](06-register-license.md)
