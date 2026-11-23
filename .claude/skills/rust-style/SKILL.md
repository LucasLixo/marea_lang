---
name: rust-style
mode: agent
effort: medium
description: Marea Rust conventions for a future thin binding crate that consumes the C registration macro and the language runtime ABI - size_of/align_of/offset_of layout asserts on every ABI type, C headers as the contract and matched by hand, error handling and no unwinding at the FFI border, raw pointer discipline, unsafe with SAFETY comments, and the one-line "// TAG: description" comment format. No Rust code exists yet. Use when writing or reviewing any .rs file, or when adding the first binding crate.
globs: "**/*.rs, **/Cargo.toml"
---

# Rust style

## Status and scope

**No Rust code exists in the repository yet.** There is no crate, no `Cargo.toml`, no `rustfmt.toml`, no bindgen run and no Rust task in `mise`. This skill applies from the day the first binding crate is added. Adding that crate is a change that must also update this skill: the layout, the formatter, the build and test commands and the dependency list are then written here, from what really exists.

Rust has one job in Marea Lang. [AGENTS.md](../../../AGENTS.md) says that one C registration macro exposes a function to the language runtime and to Rust alike, with no external generator and no hand-written glue, and that production code is C11, or Rust where a binding needs it. So Rust is a **consumer** of the C ABI. The compiler core is C11 and is never Rust (see [c-style](../c-style/SKILL.md)); no rule here moves compiler code to Rust.

The C header is the contract. The Rust side conforms to it, not the reverse. A header is never generated from Rust (no cbindgen).

Dependencies, Cargo crates included, go through [add-dependency](../add-dependency/SKILL.md). Formatting is done by rustfmt once its configuration exists; this skill does not restate those settings.

## ABI layout asserts

**Alignment agreement with C is the most sensitive property of a binding.** A wrong `align` is not a compile error on either side - it silently corrupts every value that crosses the boundary.

For every ABI type, write a `const _: () = { ... };` block that asserts `size_of` and `align_of` for the type and `offset_of` for every field, `padding` fields included. These are `const` asserts, so a mismatch fails the build, not a test.

```rust
const _: () = {
    assert!(core::mem::size_of::<Vec2F32>() == 16 && core::mem::align_of::<Vec2F32>() == 16);
    assert!(core::mem::offset_of!(Vec2F32, x) == 0 && core::mem::offset_of!(Vec2F32, y) == 4);
};
```

- A hand-written mirror of a C struct is `#[repr(C)]` and follows the same layout rule as the header ([c-style](../c-style/SKILL.md), "Struct layout"): fields in descending size, no implicit padding, every hole an explicit `padding: [u8; N]` field, and `#[repr(C, align(N))]` where the header sets an alignment.
- When the layout depends on the pointer width, write one assert block per width with `#[cfg(target_pointer_width = "64")]` or `"32"`.
- A type that does not cross the ABI (state behind an opaque handle) is never `#[repr(C)]`, and a comment says why.
- A C enum that Rust receives is `#[repr(transparent)]` over its integer, or the value is checked on entry, so an out-of-range value never becomes a Rust enum.

## Matching the C header

- Write the declaration in the `.h` by hand first, following [c-style](../c-style/SKILL.md), then match it in Rust. Renaming or moving a symbol changes the header, the Rust name and its callers in the same change.
- Never retype a C constant or enumerator in Rust with a copied number. Take it from the header definition, or assert that the Rust value equals the C one at build time.
- Use `core::ffi` types (`c_int`, `c_void`), never `std::os::raw`.
- Signatures mirror the C: result pointers as `out_`-prefixed parameters, and the module's status enum as the return value.
- Ownership never straddles the boundary: memory that Rust allocates is freed by Rust, and memory that C allocates is freed by C.
- A header parameter qualified `MAREA_RESTRICT` ([c-style](../c-style/SKILL.md), "restrict") is a contract the Rust body must honor. `rustc` never sees the qualifier, so the body checks it itself in debug builds, right after the null guards.

## FFI border: errors and panics

- **No unwinding across FFI.** A panic never unwinds into C. `extern "C"` aborts on unwind (since rustc 1.71); for a callback that must report an error, catch the panic with `std::panic::catch_unwind` and map it to the status enum. State the choice of panic strategy in the crate manifest with a comment saying why.
- Runtime code does not call `.unwrap()` or `.expect()` on a condition that can fail at runtime (a lock, a caller-supplied index, input crossing the ABI). Map it to the status enum. Tests and build scripts may use them.
- A value from outside the crate (an FFI argument, a length, an offset) goes through checked arithmetic (`checked_add`, `checked_mul`, ...) before it feeds an index or a capacity, and an overflow maps to an error value, never a wrap.
- `debug_assert!` is for an invariant that a guard or a type already established. It never validates a caller-controlled value and never carries a side effect that a release build needs.
- Use one small guard vocabulary for the null, state, bounds and result checks at the border, so the same check is not open-coded in each function.

## Unsafe and raw pointers

**Every `unsafe` block carries a `// SAFETY:` line** that names the invariant that makes it sound: who owns the pointee, why it cannot alias, its lifetime, its alignment and initialization, and any thread-affinity assumption. A `debug_assert!` may echo the invariant but is never the proof.

A pointer that crosses the ABI is untrusted. The C caller may pass null, a dangling or misaligned address, uninitialized memory, a buffer shorter than it claims, or a pointer that aliases another argument.

- **Check before access**: non-null, aligned for the type (or `read_unaligned`/`write_unaligned` when the header does not promise alignment), and in bounds (length times `size_of` computed with `checked_mul` and compared with the buffer length).
- **Do not trust a `&` or `&mut` made from a raw pointer.** It claims exclusive, valid, initialized, aligned access for as long as it lives, and the body cannot verify that. Keep it short-lived, never store or return it, and never keep it while the same pointer or a call back into C is used. For one field or one value, prefer `ptr::read`, `ptr::write` or `addr_of!`.
- **`ptr::write` for an out parameter**: the memory may be uninitialized, so `*out = value` would drop garbage. For a type that is not `Copy`, `ptr::read` creates a second owner: pair it with a write of the replacement or `mem::forget`.
- **Uninitialized memory is `MaybeUninit<T>`**, not `mem::zeroed()`. Zeroing is valid only when the all-zero pattern is valid for the type. For a value that C fills in, call `assume_init()` only after a successful call, and write a single field with `addr_of_mut!((*p).field).write(value)`.
- **`copy_nonoverlapping` only when the ranges cannot overlap**; otherwise `ptr::copy`.
- The `// SAFETY:` line of a raw read or write names each check and who owns the value afterwards. "The pointer is valid" is not a reason.
- A repeated check-and-access goes in one small typed `unsafe fn` with the contract in its doc comment and one audited `// SAFETY:` inside. A macro is for a guard that must `return` from the caller, never for a plain access.
- Every raw read or write path has tests for the cases that break it: null, a misaligned pointer, a buffer one element too short, an out parameter with garbage, and overlapping ranges for a copy.
- An `unsafe impl Send` or `Sync` that relies on thread affinity must be enforced by code, not by a doc claim.

## Comments

**Every non-doc comment is exactly one line, shaped `// TAG: description.`** If an explanation does not fit on one line, move it to the module `//!` header, express it as a type or a guard, or split the code.

| Tag | Use for |
|---|---|
| `SAFETY` | The invariant that makes an `unsafe` block sound (mandatory on every one). |
| `WARNING` / `WARN` | A non-obvious hazard that a future edit could walk into. `WARN` is the accepted short form. |
| `ERROR` | Why an error path returns what it returns, when the guard alone does not show it. |
| `INFO` | Rationale or context that fits no other tag - the "why", never the "what". |
| `TODO` | Deferred work, always with an issue number: `// TODO(#123): description.` |
| `FIXME` | A known defect in this code, always with an issue number: `// FIXME(#123): description.` |
| `BUG` | A workaround for a defect in code this repository does not own; names the upstream defect and when the workaround can go. |

A comment explains why, never what, and it is not a proof: prefer a type or a guard. The tag list is the same as in [c-style](../c-style/SKILL.md), plus `SAFETY`, which is Rust-only. Comments follow ASD-STE100, and all text is ASCII only.

## Files and tests

- Every new or substantially rewritten `.rs` file opens with the copyright header from [AGENTS.md](../../../AGENTS.md), with `//` as the comment marker.
- TDD is mandatory ([AGENTS.md](../../../AGENTS.md)): a failing test first. Tests assert golden bit patterns for floats (`to_bits()`), not approximate comparisons, wherever the contract is parity with the C side.
- Production code carries no test code. Where the tests live, and the command that runs them, are set when the first crate is added.
