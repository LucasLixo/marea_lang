# Step 4 of 8 - Stage 1B - Platform support

Part of the [audit-code](../SKILL.md) skill. This file is a complete prompt for this step alone: do what it says, check "Done when", then open the next step.

## Rules for this step

- Run this stage when the diff adds or changes behaviour that a user can reach (a feature, an API, a package). Skip it for a pure refactor, a test-only or a doc-only change and say so.
- Judge from evidence: the code gates, the build gates, the tests and the cross checks. Never assume a platform works because the code compiles there.

For every audited feature, decide per platform whether it works. The platforms are Windows, Linux, Android and Emscripten (web).

1. Find the gates in the diff and in the code it calls: `#if defined(MAREA_PLATFORM_*)` and `MAREA_API_*` in C, `cfg(target_os = ...)` and `api-*` features in Rust, `MAREA_PLATFORM_*` and `MAREA_OPTION_API_*` in CMake.
2. Run the platform tests that exist for the platforms the diff can reach, and keep the real output. Use the mise tasks, never a hand-rolled command:

   | Platform | Task | What it proves | Limit |
   | --- | --- | --- | --- |
   | Linux x86_64 | `mise run test:engine-cpp-headless` | the engine suite with a dummy video driver | no real GPU |
   | Web | `mise run test:engine-cpp-web` (and `test:engine-cpp-web-fetch` for the fetch API) | the suite under Node with Emscripten | no window, GPU or pthreads: UI tests skip with a reason |
   | Linux aarch64, i686, armv7 | `mise run test:cross-engine -- --target <target>` | the CPU subset (`ctest -L cpu`) under qemu-user or natively | armv7 is reported as not run when the tools are missing; i686 skips tests that need a 32-bit Vulkan |
   | Android x86_64, in an Activity | `mise run test:cross-engine -- --target android-x86_64-app --label all` | the suites inside the tests/android application with a real Java VM, built with Gradle, one Activity launch per suite; a failing, crashing or hanging suite fails the run | x86_64 only, a software GPU in the emulator, one window only (a suite that opens a second window crashes), minutes long and never part of `all` |
   | Android x86_64, from the shell | `mise run test:cross-engine -- --target android-x86_64 --label all` | the engine test executable run on the KVM emulator, one suite at a time; suites that need a Java VM are reported as not verified | x86_64 only: the arm ABIs cannot build the engine; faster, but SDL's JNI code cannot run |
   | Android CPU and ABI | `mise run test:cross -- --target <android target>` | libm, inline and SIMD checks of `marea-platform-check` on the emulator or qemu | CPU and ABI only, never a feature |
   | Windows | the `windows-*` jobs of `.github/workflows/ci.yml` | MSVC build of the crate, the C checks and the engine | CI only. Wine (`test:cross -- --target windows-x86_64-gnu`) is a hint, not evidence: it ships its own libm, so a green wine run does not prove the Windows result |

   Run only the rows the diff can reach, and run the cheap one first. A task that cannot run here (no emulator, no qemu, no Node) is reported as not run, and its platform stays `not verified`. A test that skips on a platform is not evidence for that platform: read the skip reason.
3. Give each platform one status:
   - **supported**: the code path runs there and a test or a cross check proves it (`mise run test:cross`, the platform CI leg, the headless test).
   - **unsupported by design**: the platform cannot do it (for example a native file dialog on the web). The code answers with the package's own not-supported result. It never compiles into a silent no-op, never crashes, and never fails the build of that platform.
   - **not verified**: the code may work but nothing proved it on this platform.
4. Every `unsupported by design` platform needs all of these, or a finding:
   - an explicit runtime answer (an error or "not supported" result) that a caller can read;
   - the `<platforms>` element in `docs/{kind}/{Name}.xml`, listing the platforms where it works (see "Platform support" in [doxygen-documentation](../../doxygen-documentation/SKILL.md)), and a sentence in `<details>` saying what a caller gets on the other platforms. Never put Doxygen comments in the header;
   - the permission map `tools/marea-permissions/map.lua` is in step (see stage 1A);
   - the gate is as narrow as the real limit: not `#if !defined(MAREA_PLATFORM_WINDOWS)` when only Android lacks it.
5. Every `not verified` platform is an open point. Write it in `temp/_LOG_APPLY_ERROR.md` with the exact check that is missing. Do not report it as supported.
6. Look for hidden limits: a hard-coded path separator, a POSIX-only call on Windows, a thread, a file or a network call on the web, a permission on Android, a 32-bit assumption on i686 or armv7.
7. Make the table for the final report: one row per feature, one column per platform, each cell `supported`, `unsupported by design` or `not verified`, with the evidence.

## Done when

- The stage was skipped with a reason, or every audited feature has a status for Windows, Linux, Android and Emscripten.
- Every platform row the diff can reach was run, or the report says it was not run and why. Every `supported` cell names its evidence (the task and its real result). Every `not verified` cell is in the error log.
- Every `unsupported by design` platform has the runtime answer, the `<platforms>` list that matches the gates and an exact gate, or a reported finding.

Next: [steps/05-minimal-code.md](05-minimal-code.md)
