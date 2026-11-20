---
name: rust-style
mode: agent
effort: medium
description: Marea Rust conventions for the lib/rust crate - the module layout mirroring the C packages, mandatory size_of/align_of/offset_of layout asserts on every ABI type, hand-written C headers matched by #[export_name], consuming external C only through wrapper/*.h and the build/*.rs allowlists, the marea_check guard macros, the one-line "// TAG: description" comment format (SAFETY/WARNING/WARN/ERROR/INFO/TODO/FIXME/BUG), the raw pointer read, write and reference rules (untrusted ABI pointers, validate before access, no long-lived &mut from a raw pointer, MaybeUninit), and unwrap/expect/debug_assert/checked-arithmetic discipline, and the test layout (no inline tests; mirrored files under lib/rust/tests loaded through #[path], including the nested-module form and the allowed cfg(test) seams). Use when writing or reviewing any .rs file, porting a module to Rust, or adding a symbol from an external C library.
globs: lib/rust/**/*.rs, lib/rust/Cargo.toml
---

# Rust style

## Context and objective

### Why the crate exists

The engine is consumed as pure C headers. Rust replaced C++, not C: `.h` public headers stay C17, `.c` keeps simple code and SDL3 wrappers, and anything with owning state, a non-trivial data structure, or a real algorithm is Rust (`c-style` owns the C side of that boundary).

The crate is one half of a C ABI. Everything below follows from that: the layout asserts, the hand-written headers, and the single audited FFI funnel.

## ABI layout asserts

**Alignment agreement with C is the most sensitive property in the crate.** A wrong `align` is not a compile error on either side - it silently corrupts every value that crosses the boundary.

An ABI type is not declared in Rust. It comes from bindgen through `marea_abi::raw`, re-exported under its `Marea*` name - so its `#[repr(C)]` and its layout are whatever the real header says, for the real target. What you write by hand is the **check**: a `const _: () = { ... };` block asserting **`size_of` and `align_of` for the type, and `offset_of` for every field**. These are `const` asserts, so a mismatch fails the build, not a test.

```rust
const _: () = {
    assert!(core::mem::size_of::<MareaVec2F32>() == 16 && core::mem::align_of::<MareaVec2F32>() == 16);
    assert!(core::mem::offset_of!(MareaVec2F32, x) == 0 && core::mem::offset_of!(MareaVec2F32, y) == 4);
    assert!(core::mem::offset_of!(MareaVec2F32, padding) == 8);
    assert!(core::mem::size_of::<MareaVec2F32>() == core::mem::offset_of!(MareaVec2F32, padding) + 8);
};
```

That `align == 16` on a two-float vector is exactly the point: it is GLM's SIMD alignment, nothing a reader would infer, and the assert is what keeps it true. `marea/math/marea_glm_types.rs` is the reference with 38 of them; the crate holds 102 `size_of`, 93 `offset_of`, and 36 `align_of` asserts across 33 blocks.

Keep them even though bindgen emits its own. `layout_tests(true)` asserts that bindgen's *generated* types match the headers it parsed; the hand-written block asserts that the layout the *port was written against* is still the real one. Two independent checks, both free. `marea_abi.rs` adds the cross-compile guard on top - `MAREA_IntegerSize == usize` and `MAREA_Null == *mut c_void` catch a host/target parse mismatch that would otherwise pass silently.

Writing `#[repr(C)]` by hand is the rare exception, not the rule: the crate does it for a union bindgen handles badly (`SdlEvent`), for an arena chunk whose C layout is needed for pointer arithmetic (`marea_component.rs`), and for the private mirrors of internal C structs that sit behind an opaque handle (`MareaAppContext`, `IoStreamInternal`). Each of these files carries the assert block. If you are about to hand-write a `#[repr(C)]` mirror of a header type, the answer is almost always to allowlist the type instead.

### Hand-written `#[repr(C)]` types follow the C struct-layout rule

A hand-written mirror of a header struct obeys the same rule as the header (`c-style`, "Struct layout"), so it never disagrees with the layout bindgen or C would give it:

- **Fields in descending size**, in the header's order: 64-bit scalars and pointers first, then 4-, 2- and 1-byte fields; arrays and nested structs by their alignment.
- **No implicit padding.** Every hole is one explicit `padding: [u8; N]` field, named `padding` (`padding0`, `padding1` for several separate holes), never `paddingZ`/`pad`. Where the header's padding depends on the pointer width (`MAREA_PADDING`, `MAREA_PADDING_64`, `MAREA_PADDING_32`), the field takes `#[cfg(target_pointer_width = "64")]` / `"32"` attributes, one per width.
- **Explicit alignment**: `#[repr(C, align(N))]` with the header's `MAREA_ALIGNED(N)`, never the compiler's implicit alignment.
- **Asserts cover everything**: `size_of`, `align_of` and `offset_of` for every field *including `padding`*, one block per pointer width (`#[cfg(target_pointer_width = ...)]`) when the layout differs.

A struct literal of a bindgen type that has a `padding` field must initialize it; use `crate::marea_abi::zero_padding()`, which infers the array length from the field, so the literal never spells a width-dependent number. A struct whose padding exists on one pointer width only puts the same `#[cfg(...)]` on that field of the literal.

The inverse rule is as strict: a type that does **not** cross the ABI - the `State` and `Global` structs behind an opaque handle - is never `#[repr(C)]`, and a comment says why. `#[repr(transparent)]` belongs to the bindgen newtype enums, which is what keeps a value received from C from ever being an out-of-range Rust enum.

## Crate layout

No `src/`. `[lib]` is `name = "MareaLibRust"`, `path = "marea/lib.rs"`, `crate-type = ["staticlib"]`, and Corrosion imports that target name directly.

- `marea/lib.rs` - crate root, one `pub mod <package>;` per engine package with a `///` line each.
- `marea/<package>.rs` - aggregator, one `#[cfg(feature = "api-<id>")] pub mod marea_<module>;` per module.
- `marea/<package>/marea_<module>.rs` - the module, named after the C file it mirrors.

## Module shape

```rust
//! base.deque - behavioral Rust port of the former C++ `base/MAREADeque.cpp` (issue #85, Phase 2).

use core::ffi::{c_int, c_void};

use crate::marea_check_null;

pub use crate::marea_abi::raw::{MareaDeque, MareaDequeExcept};

const _: () = {
    assert!(core::mem::size_of::<MareaDeque>() == core::mem::size_of::<*mut c_void>());
};

type DequeState = VecDeque<*mut c_void>;

#[export_name = "MAREA_DequeCreate"]
pub extern "C" fn marea_deque_create(out_deque: *mut *mut MareaDeque) -> MareaDequeExcept {
    let out = marea_check_null!(out_deque, MareaDequeExcept::ERROR_NULL_POINTER);
    ...
    MareaDequeExcept::SUCCESS
}
```

- The `//!` header names the C or C++ file the module replaces and states the parity claim.
- FFI types come from `core::ffi`, never `std::os::raw`.
- ABI types are re-exported from `marea_abi::raw` under the `Marea*` names bindgen already emits (the `item_name` callback in `build/common.rs` spells `MAREA_Foo` as `MareaFoo`), so import paths stay stable and no import is ever renamed by hand.
- Repeated opaque-handle traversal goes in a local `macro_rules!` rather than being copied per function.

## Exporting the C ABI

`#[export_name = "MAREA_DequeCreate"]` is what makes a Rust function answer to the name in the header - 573 of them today. The Rust function name is the snake_case of the C name, so `MAREA_DequePushBack` is `marea_deque_push_back`.

A header parameter qualified `MAREA_RESTRICT` (`c-style`, "restrict") is a contract the Rust body must honor and verify: Rust receives raw pointers and `rustc` never sees the qualifier, so the body calls `marea_debug_assert_no_alias!(a, b)` for two single objects or base pointers and `marea_debug_assert_no_overlap!(a, a_bytes, b, b_bytes)` for ranges right after the `marea_check_*!` guards (`base/marea_check.rs`, the counterparts of `MAREA_ASSERT_NO_ALIAS` and `MAREA_ASSERT_NO_OVERLAP`). Both compile out of release builds, and a violation in a debug build aborts, since a panic cannot unwind out of `extern "C"`. A guard reference (`&mut T`) is passed as is; it coerces to the raw pointer the macros take. The function's header comment (`c-style`, "Documentation") says which parameters must not alias.

Signatures mirror the C: `out_`-prefixed pointer parameters for results, and the module's `MareaXExcept` as the return with `::SUCCESS` and `::ERROR_*`. An opaque handle is allocated by Rust with `Box::into_raw` and freed by Rust with `Box::from_raw` - ownership never straddles the boundary.

An internal `#[export_name]` - one no public header declares - must be declared in a `lib/internal/` header and listed in `tools/marea-api-catalog/internal_exports.txt` with the C file or test that calls it, else the build fails (`mise run test:exports-match`). A function only Rust calls takes no `#[export_name]` at all; a function nothing calls is deleted.

## Headers are written by hand

**Headers are never generated from Rust.** There is no cbindgen and none is wanted. Adding an exported function means writing its declaration in the `.h` under `lib/include/marea/` by hand, following `c-style`, and then matching it with `#[export_name]`.

The header is the contract; the Rust side conforms to it, not the reverse. Renaming a symbol or moving it to
another header therefore changes the `#[export_name]`, the snake_case Rust function name, its callers and the
mirrored module and test file in the same change set.

## Consuming external C

**Never hand-declare a symbol from an external C library.** Two steps instead:

1. Add the `#include` to the right wrapper header.
2. Add the symbol to the matching allowlist in its `build/<wrapper>.rs` (the root `build.rs` is a
   thin orchestrator; each wrapper owns its own bindgen run under `lib/rust/build/`).

| Wrapper | Covers | Allowlists in `build/<wrapper>.rs` |
|---|---|---|
| `wrapper/sdl.h` | SDL3 | `ALLOWED_FUNCTIONS`, `ALLOWED_TYPES`, `ALLOWED_VARS` (`build/sdl.rs`) |
| `wrapper/cimgui.h` | cimgui | `ALLOWED_CIMGUI_FUNCTIONS`, `_METHODS`, `_FUNCTION_PATTERNS`, `_TYPES` (`build/cimgui.rs`) |
| `wrapper/imgui_backends.h` | the ImGui SDL3 backends | `ALLOWED_IMGUI_BACKEND_FUNCTIONS` (`build/imgui_backends.rs`) |
| `wrapper/xxd.h` | the embedded-asset headers | - (`build/xxd.rs`) |

bindgen parses the real headers, so struct layouts are the target's true C ABI under cross-compile rather than a transcription.

**No `SDL_*` or `MAREA_*` value is ever retyped in Rust.** If a module needs an enumerator, allowlist the enum and use it; do not copy the number. SDL's `SDL_min`, `SDL_max`, and `SDL_clamp` are macros and cannot cross FFI - reproduce the exact ternary at the call site, never `f32::min`/`max`, whose NaN semantics differ.

Numeric `f32`/`f64` math is the one place the SDL wrapper is not the default: `sqrt`, `abs` and the few functions listed in c-style "Source independence" are native Rust, chosen by measured gain plus a bit-comparison test. See that paragraph for the list and the criterion; the `SDL_min`/`SDL_max` ternary caveat above is unchanged.

## Source independence

The order of preference is owned by [c-style](../c-style/SKILL.md) ("Source independence") and Rust follows it. In `lib/rust`, reach for the lowest available primitive, in this order, and stop at the first one that does the job:

1. **SDL3**, through the allowlisted bindgen wrapper (see "Consuming external C"), when SDL3 offers the capability: files, time, threads, memory, input, clipboard and the like. SDL3 is the portability layer, so this is the default for anything platform-facing.
2. **`core` and `alloc` / `std`**, the Rust counterpart of the C standard library, for everything SDL3 does not offer and for language-level work (`Vec`, `String`, `Option`, iterators, `checked_*` arithmetic, `copy_nonoverlapping`).
3. **A crate already in `Cargo.toml`** (`glam`, `wide`, `getrandom`, the hash and crypto crates and so on) before a hand-written routine for the same job. See "Mathematics" for `glam`.
4. **An existing engine function**: call the sibling Rust function directly. Never route through a `MAREA_*` wrapper that only forwards to a call you can make yourself.
5. **A new function**, only when none of the above exists. A new external crate is not a step of this order: it is a dependency decision owned by the [add-dependency](../add-dependency/SKILL.md) skill (permissive licenses only, the license file, the `license_embed_add` registration inside an `if`, the `Cargo.toml` comment).

The numeric-math exception stays as c-style "Source independence" and "Consuming external C" state it: `sqrt`, `abs`, `sinf`, `cosf` and `atan2f` are native Rust because of a measured gain, and every other swap needs a measured gain plus a bit-comparison test. Do not restate it here.

A hand-written routine that duplicates a primitive higher in this list is a conformance finding for [audit-code](../audit-code/SKILL.md).

## Mathematics

For mathematics implemented on the Rust side, use the facilities already provided by `glam` before writing a calculation by hand. This includes vector, matrix, quaternion, transform, geometric, interpolation, and component-wise operations. Reuse the crate's existing `marea_glm` conversion helpers at the ABI boundary; do not create a parallel engine-local math implementation when `glam` supplies the operation.

Keep the established exceptions when the public contract requires a bit-exact SDL or native scalar behavior, or when an existing engine operation intentionally has semantics that differ from `glam`. In those cases, document and test the reason rather than silently substituting a different operation.

### The one exception: engine `MAREA_*` functions

The generated `marea_wrapper.h` yields **types only**. Every `MAREA_*` function is defined by this crate, so importing one would collide with the symbol it exports, and `build/marea.rs` blocks them with `blocklist_function(".*")`.

So a module calling a `MAREA_*` function - one still implemented in C, or one a sibling Rust module exports - declares it in a small `extern "C"` block, with the types still coming from `marea_abi::raw`:

```rust
use crate::marea_abi::raw::MareaDeque;

extern "C" {
    fn MAREA_DequeCreate(out_deque: *mut *mut MareaDeque) -> c_int;
}
```

This is the only place a hand-written `extern "C"` block is correct. Do not "fix" it by allowlisting `MAREA_*` functions in `build/marea.rs` - that reintroduces the symbol collision.

## Guards and unsafe

`marea/base/marea_check.rs` is the Rust counterpart of `MAREACheck.h`: `marea_check_null!`, `marea_check_null_ref!`, `marea_check_state!`, `marea_check_bounds!`, `marea_check_result!`. Each has a fallback arm and a no-fallback arm, covering the C `*_VOID` variants. Use them instead of open-coding a null test, so the guard vocabulary stays uniform across the ABI surface.

**Every `unsafe` block carries a `// SAFETY:` line** (see `Comments` for the format) naming the invariant that makes it sound: who owns the pointee, why it cannot alias, its lifetime, its alignment and initialization state, and any thread-affinity assumption. The crate has more SAFETY comments than `unsafe` blocks; keep it that way. A `debug_assert!` may echo a SAFETY invariant for diagnostics, but it is never the proof - the proof is the type, the `marea_check_*!` guard, or release-active validation that runs whether or not debug assertions are compiled in.

### Raw pointer reads, writes and references

A pointer that crosses the ABI is untrusted. The C caller may pass null, a dangling or misaligned address, memory that is not initialized, a buffer shorter than it claims, or a pointer that aliases another argument; the compiler checks none of it. Treat every such pointer as unverified until the body has checked what the access needs:

- **Before a read or write, check** that the pointer is non-null (`marea_check_null!`), aligned for the type (or use `read_unaligned`/`write_unaligned` when the header does not promise alignment, as a packed or bindgen struct may not), and that the bytes are in bounds (a length times `size_of`, computed with `checked_mul`, compared with the buffer length before any access).
- **Do not trust `&` or `&mut` made from a raw pointer.** `&mut *p` and `marea_check_null!` (which uses `as_mut`) claim exclusive, valid, initialized, aligned access for as long as the reference lives, and the body cannot verify that claim: another ABI argument or a C callback may alias it. Keep such a reference short-lived: use it for one expression or one statement run, never store it, never return it, never keep it while the same pointer, a second reference to the pointee or a call back into C is used, and never turn two ABI pointers into two `&mut` without `marea_debug_assert_no_alias!`. When the access is one field or one value, prefer the raw operation (`ptr::read`, `ptr::write`, `addr_of!`) over a reference.
- **`ptr::write` for memory that may be uninitialized.** An out parameter points to memory the caller may not have initialized, so assigning with `*out = value` would drop the garbage old value; write it with `ptr::write(out, value)`. For a type that is not `Copy`, `ptr::read` makes a second owner of the same value: pair it with a `ptr::write` of the replacement or `mem::forget`, and never read the same owned value twice.
- **Uninitialized memory is `MaybeUninit<T>`**, not `mem::zeroed()` and not a value of `T` filled in later; zeroing is valid only for a type whose all-zero bit pattern is valid (plain integers, floats, bindgen structs with explicit `padding`), and never for references, `NonNull`, `bool` or enums without a zero variant. Use `crate::marea_abi::zero_padding()` for padding fields, as above.
- **`MaybeUninit` for a value C fills in.** Create it with `MaybeUninit::<T>::uninit()`, pass `as_mut_ptr()` to the C function, and call `assume_init()` only after the call, each with its own `// SAFETY:` line (`marea/video/marea_video_av1.rs` does this with `Dav1dSettings` and `dav1d_default_settings`). `assume_init` is valid only if the callee initializes every field on every path that returns success; on an error return, do not call it. Never read the value before `assume_init`, and write a single field with `addr_of_mut!((*p).field).write(value)`, never `(*p).field = value` on uninitialized memory.
- **`copy_nonoverlapping` only when the ranges cannot overlap**, proven by the same argument that gives the lengths (or checked with `marea_debug_assert_no_alias!`); otherwise `ptr::copy`.
- **The `// SAFETY:` line of a raw read or write names each check**: non-null, aligned, in bounds, initialized, not aliased, and who owns the value afterwards. A bare "the pointer is valid" is not a reason.

**Helper function or macro, or the plain `ptr` call?** A one-off access stays a direct `ptr::read_unaligned`/`ptr::write` with its own `// SAFETY:` line. When the same check-and-access repeats (the ui widget code repeats a null check plus `read_unaligned` and `write_unaligned` of the widget state, with the same reasoning, hundreds of times), give it one small typed `#[inline]` function, such as a generic read-copy or write-copy of `T` that checks null and returns an `Option` or a `bool`, declared `unsafe fn` with the contract in its doc comment (the pointee is valid for `size_of::<T>()` bytes), and a single audited `// SAFETY:` inside it. A function is type-checked, shows up in a backtrace, is covered once by its own tests (null, misaligned, short) instead of at every call site, and costs nothing once inlined; callers keep a one-line `// SAFETY:` that states the contract holds. A **macro** is only for a guard that must `return` from the caller (as `marea_check_null!` and `marea_check_state!` do), because a function cannot return for its caller. A macro that merely wraps an access hides the `unsafe`, repeats its `// SAFETY:` text in every expansion and gives poor error messages. None of these helpers exists yet; adding the first one is its own change with its tests, placed beside `base/marea_check.rs`.

Every new raw read or write path is covered by tests of the cases that break it: a null pointer, an offset-by-one (misaligned) pointer into a byte buffer, a buffer one element too short, an out parameter that holds garbage, and overlapping ranges where a copy is involved. A file that contains `unsafe` is also run under Miri where the nightly toolchain exists (`mise run setup:rust-miri` once, then `cargo +nightly miri test <filter>` from `lib/rust/`); the [audit-code](../audit-code/SKILL.md) bug hunt does this and says plainly when it could not.

Every `unsafe impl Send` or `unsafe impl Sync` that relies on thread affinity MUST cite an enforcing `MareaThreadGuard`, preferably emitted by `marea_single_thread_global!`; a doc-only main-thread claim is insufficient. Pick the global primitive by state shape: a flag uses `AtomicBool`, a pointer uses `AtomicPtr`, a counter uses `AtomicUsize`, rich shared state uses `Mutex`, and an escaping-pointer global uses `MareaThreadGuard` plus `UnsafeCell`. `base/marea_thread_guard.rs` is the guard mechanism; `ui/marea_imgui_font.rs` was the negative example of an unguarded thread-affinity claim and is now the positive `FONT_THREAD` pattern.

## Comments

**Every non-doc comment is exactly one line, shaped `// TAG: description.`** If an explanation does not fit on one line, that is a signal to move it into the module's `//!` header, express it as a type/guard instead, or split the code - not to wrap the comment.

The tag vocabulary:

| Tag | Use for |
|---|---|
| `SAFETY` | The invariant that makes an `unsafe` block sound (mandatory on every one). |
| `WARNING` / `WARN` | A non-obvious hazard: a trap a future edit could walk into without seeing it coming. `WARN` is the accepted shorthand of `WARNING`. |
| `ERROR` | Why a specific error path returns what it returns, when that is not obvious from the guard alone. |
| `INFO` | Rationale, cross-reference, or context that does not fit `SAFETY`/`WARNING`/`ERROR` - the general "why", never the "what". |
| `TODO` | Deferred work, always with an issue number: `// TODO(#123): description.` A TODO with no issue has no owner and does not get committed. |
| `FIXME` | Known defect in this code - incorrect or broken-by-design behavior that must be corrected, always with an issue number: `// FIXME(#123): description.` |
| `BUG` | A workaround for a bug in code this repository does not own (a vendored library, a platform or driver API): names the upstream defect and the condition under which the workaround can be removed. |

A comment explains *why*, never *what* - `index += 1` needs no comment; skipping the increment on a specific error path to keep a caller-visible prefix unchanged does. Prefer enforcing a requirement in the type system or a `marea_check_*!` guard over describing it in a comment; a comment is not a proof.

This applies to new and touched comments; it is not a mandate to rewrite untouched `//`-comment blocks elsewhere in the crate on sight.

## Formatting

rustfmt is the formatter and wins on whitespace and layout: [`lib/rust/rustfmt.toml`](../../../lib/rust/rustfmt.toml) decides edition, width and line endings, and `mise run format:rust-sources` applies it to `lib/rust` and `tools/marea-platform-check`; `mise run format:check-sources` is the read-only check. This skill restates none of those settings. What the formatter cannot decide stays a hand rule: the one-line `// TAG: description.` comment format (see `Comments`) and the copyright header.

## Error handling and invariants

Runtime (non-test, non-`build/`) code does not reach for `.unwrap()`/`.expect()` on a condition that can genuinely fail at runtime - a lock, a caller-supplied index, external input crossing the ABI. Route it through the module's `MareaXExcept` instead. Tests and `build/*.rs` scripts may use them freely; a failure there is a build or test defect, not a runtime one.

Values arriving from outside the crate's own control - an FFI argument, an externally-supplied length, a parsed offset - use checked arithmetic (`checked_add`, `checked_sub`, ...) before it feeds an index or a capacity decision, mapped to an `ERROR_*` variant on overflow rather than trusted as-is.

`debug_assert!`/`debug_assert_eq!` are for an internal invariant already established by the preceding `marea_check_*!` guard or by the types involved - never a stand-in for validating a caller-controlled value, and never carrying a side effect the release build depends on.

## Feature gating

One `api-<id>` feature per `MAREA_API_<PKG>` package, `default = []`. `ConfigureRust.cmake` is the single source of truth and passes the enabled set through Corrosion, so a package compiled out of C is compiled out of Rust too (`cmake-style` owns the CMake half).

Any two features documented as mutually exclusive in `Cargo.toml` raise a `compile_error!` when both or neither
are selected. **`cargo test --all-features` is therefore invalid for this crate**; pass an explicit list:

```bash
cargo test --no-default-features --features "api-math-aabb"
```

Disabling an API removes its Rust: with `MAREA_OPTION_API_<PKG>=OFF` the module and every `#[export_name]` in it are compiled out by the `api-<id>` feature, so the symbol is not in the export list and not in the binary. A new export therefore sits under the cargo feature that matches the `MAREA_API_<PKG>` gate of its header (a module declared under `#[cfg(feature = "api-<id>")]`, or the item itself), never under a different one and never ungated. Deleting a header declaration deletes its Rust definition and export in the same change set. `mise run test:exports-match` (and the `Marea-Lib-ExportsMatch` build target) fails on an export no public header declares, a declared symbol nothing defines, and a feature that does not match the gate. A cargo feature that enables another (`api-markup = ["api-ui"]`) keeps that module compiled while the dependency's own package is OFF, so turn off the dependents together with the package.

## Tests

Tests assert **golden bit patterns**, not approximate float compares, because the contract is bit-exact parity with the code that was ported:

```rust
assert_eq!(center.x.to_bits(), 0x3f800000);
```

### Where tests live

A module carries **no inline test code**. Its tests live in a mirrored file under `lib/rust/tests/` and are loaded as a `#[path]` child module, so the source can be read and audited without them and the tests still reach the module's private and `pub(crate)` items (a true Cargo integration test would see the public API only; `autotests = false` in `Cargo.toml` keeps cargo from discovering `tests/` on its own).

For `lib/rust/marea/<path>/<name>.rs` the tests are `lib/rust/tests/<path>/<name>.rs`, and the source ends with exactly these three lines:

```rust
#[cfg(test)]
#[path = "../../tests/<path>/<name>.rs"]
mod tests;
```

- **Depth rule**: the `#[path]` of a file-backed `mod` resolves against the directory of the declaring file, so the number of `../` is the number of directories between the declaring file and `lib/rust/`: `marea/ui/marea_accessibility.rs` uses `../../tests/ui/marea_accessibility.rs`; a top-level aggregator such as `marea/ui.rs` uses `../tests/ui.rs`. The mirror sits at the same depth as the source, so a relative `include_str!`/`include_bytes!` that climbs out of `lib/rust/` stays valid; one that names a file beside the source does not, and is rewritten to point at the source directory.
- **Extra feature conditions** (`#[cfg(all(test, feature = "api-..."))]`) are copied verbatim onto the same declaration. `use super::*;` and `super::` keep their meaning, because the parent of the child module is still the declaring module.
- **More than one test module in a file** (a `channel_tests` beside `tests`): each keeps its name, so test names do not change, and each is its own mirror file, `lib/rust/tests/<path>/<name>/<module>.rs` for every module but `tests`, which stays `<name>.rs`. These sit one directory deeper, so the `#[path]` is unchanged (it resolves from the declaring source file's directory, not from the test file) and only a relative `include_str!`/`include_bytes!` inside the deeper test file gains one `../`.
- **Test-only items on a production type** (an accessor such as `fn len(&self)` under `#[cfg(test)]`) move into the test file as an inherent `impl` block of the type: a child module sees the parent's private fields, and a `pub(crate)` method stays callable from other modules' tests.
- **A test module nested in an inline `mod`** (a platform backend written as `mod ssip { ... }` inside `marea/ui/marea_narration.rs`): an inline module has no directory on disk to climb out of, so the `#[path]` goes on the inline module itself, naming the mirror directory, and the nested declaration names the file inside it. The depth rule is the same, counted from the declaring file; the mirror file is `lib/rust/tests/<path>/<name>/<module>.rs`. Keep a `// INFO:` line above the inline module's `#[path]`, as the sources do:

  ```rust
  #[cfg(target_os = "linux")]
  // INFO: the #[path] below sets the directory of the nested test module, since an inline module has no directory on disk to climb out of.
  #[path = "../../tests/ui/marea_narration"]
  mod ssip {
      // ... production code ...

      #[cfg(test)]
      #[path = "ssip.rs"]
      mod tests;
  }
  ```

  This resolves to `lib/rust/tests/ui/marea_narration/ssip.rs`; `marea_accessibility.rs` does the same for `atspi`, `win_screen_reader` and `android_a11y` (files under `lib/rust/tests/ui/marea_accessibility/`). A backend compiled on the host only for its tests (`#[cfg(any(target_os = "android", test))]`, as `android_tts` and `android_a11y`) keeps that gate on the inline module.
- **Allowed `cfg(test)` seams in source.** The rule above bars test code, not a seam: production behaviour that must differ under `cfg(test)` so tests can observe it. Each stays in the source with a `// INFO:` line saying it is a seam. The complete list:
  - **A logging seam**: a `#[cfg(test)]` / `#[cfg(not(test))]` pair that swaps the process-wide logger for a per-thread collector the tests read. `marea/ui/marea_ui_image_log.rs` (`with_state`, `TEST_STATE`, `COLLECTED`, read by other modules' tests through `take_collected` and `reset_dedup`) and `marea/ui/marea_widget_layout_container.rs` (`LOGGED`, and the pair in `emit_warning` and `emit_error`; the tests read `super::LOGGED`).
  - **A host gate**: `#[cfg(any(target_os = "...", test))]` on a platform backend so its logic compiles and runs on the host against a fake (the nested-module case above).
  - **The crate root's two test-only declarations** in `marea/lib.rs`: `build/enum_names.rs` (see the exception below) and the `test_support` module.
  - **No test module at all**: a module whose every entry point reaches SDL, ImGui or cimgui cannot be linked by a standalone `cargo test` binary, so it carries no `#[cfg(test)]` and says so with a `// INFO:` or module-doc line naming what covers it instead (`marea_cimgui.rs` and `marea_dear_imgui.rs`; the latter points to the GoogleTest suite `tests/source/ui/MAREADearImgui.cpp`; `marea_ui_layout_disk.rs` carries the same note). Pure logic split out of such a module gets its own file and test mirror (`marea_ui_scale.rs`, split from `marea_dear_imgui.rs`).

  Anything else under `cfg(test)` in a source file is test code and moves to the mirror.
- **Helpers used by one module's tests** move into that module's test file. **Helpers shared by tests of several modules** (a fake backend, a global test lock, a log collector) go to the shared test-support module `lib/rust/tests/support/`, declared once from `lib.rs` as `#[cfg(test)] #[path = "../tests/support/mod.rs"] mod test_support;` with one `pub(crate) mod <name>;` line per helper file, and are imported as `crate::test_support::<name>`. Never declare the same helper twice.
- **A test-only module** (a reference implementation, a test kit, a bench) follows the same mirror: `lib/rust/marea/<path>/<name>.rs` becomes `lib/rust/tests/<path>/<name>.rs`, declared with `#[cfg(test)]` and `#[path]` in its aggregator. Anything another tool includes by path (`tools/marea-platform-check` includes `math/marea_math_reference.rs` and `marea_math_bits.rs`) has that path updated in the same change set.
- **Exception**: `lib/rust/build/` keeps its tests inline, because `build.rs` includes `build/enum_names.rs` as a plain `mod`; the crate root reaches that file with `#[path = "../build/enum_names.rs"]`, so its tests run from there.
- **Moving a module's tests is proven by the name list**: run `mise run test:rust-crate` before and after; the sorted test names (the `PASS [..] (n/N) marea-lib-rust <name>` lines, names only) and the `N tests run, M skipped` summary must be identical, because the module tree does not change. A difference is a bug in the move, never an accepted deviation. Format a new test file with `rustfmt --edition 2021 <file>` and never reformat the source file beside it.

### Migration recipe for one module

1. Save the baseline name list and summary with `mise run test:rust-crate`.
2. Check the module for what a plain move does not cover: more than one `#[cfg(test)]` item, a `#[cfg(test)]` helper that other modules name (`git grep -n <item> -- lib/rust tools`), an `include_str!`/`include_bytes!` inside the tests, and a feature gate on the module's own declaration (`marea/<package>.rs`).
3. For the plain case (one trailing `#[cfg(test)] mod tests { ... }` and nothing after it): cut the body, remove one indentation level (4 spaces), write it after the three copyright lines at `lib/rust/tests/<path>/<name>.rs`, and replace the block in the source with the three-line declaration. Keep the blank line before `#[cfg(test)]`. `use super::*;`, `use super::<item>;` and the `macro_rules!` macros defined above the declaration keep working unchanged, because textual macro scope reaches a file-backed child module.
4. Run `rustfmt --edition 2021` on the new test file only (it normally changes nothing, since the inline block was already formatted).
5. Run `mise run test:rust-crate` again and diff the sorted name list against the baseline. For a feature-gated module run the probe from Feature gating on both states with the features its module needs (`mise run test:rust-crate -- --features "api-fs-path"`), and compare the lists the same way.
6. Run `mise run build:engine` and `mise run test:exports-match`, and check the touched files with `lua tools/marea-ascii-check/main.lua <file>...`.

`cargo-nextest` is pinned in `mise.toml`. The engine build and the Rust checks go through the mise tasks (`build:engine`, `test:rust-crate`, `build:rust-crate`); `mise tasks` lists the rest.

Two direct cargo invocations are listed exceptions to the mise-only rule (see the Tasks section of the engine `AGENTS.md`), because they take a custom command line no task wraps (`test:rust-crate` and `build:rust-crate` accept `--features` for the common case):

- **Miri**: run `mise run setup:rust-miri` once, then `cargo +nightly miri test` for undefined-behavior checks.
- **Rust feature probes**: `cargo test --no-default-features --features "<explicit list>"` (see Feature gating), to check one `api-<id>` feature in isolation.

## Panic policy

Both profiles set `panic = "unwind"`, deliberately: an FFI callback crossing back into the runtime needs to
signal an error by unwinding, and under `abort` a wrong-type argument would SIGABRT the process instead of
raising one. This costs no FFI safety: `extern "C"` has aborted on unwind since rustc 1.71, so a panic can never
unwind into C.
