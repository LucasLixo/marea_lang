# Marea Lang

A statically-typed, high-performance scripting language for general use together with C11. Every type has a fixed
width and a full name, checked once at compile time and fully erased before code generation. Memory is scoped to
regions with static ownership rules, so there is no garbage collector and no lifetime annotation to write.

One intermediate representation compiles to bytecode for a small reference interpreter, portable for embedding, or
to C11 for maximum performance, and both give the same observable behavior. The compiler is modular, one C11 library
per stage, so a tool can use only the lexer, the parser, the type checker or the interpreter. There is no standard
library: every type beyond the language's own primitives is a library, written in C11 or in the language itself, and
a host can replace any library or runtime function from C11.

## Build

```sh
mise run config:cmake
cmake --build cmake-build-debug --target marea_tests --parallel
ctest --test-dir cmake-build-debug --output-on-failure
```

Each stage is one library: `base`, `lex`, `parse`, `sema`, `ir` and `bytecode`. Two options choose which kinds of
library the configure builds, and they add to each other:

| Option | Default | Target of a stage | Alias |
|---|---|---|---|
| `MAREA_OPTION_STATIC_LIBS` | `ON` | `Marea-<Stage>` (for example `Marea-Lex`) | `marea::<stage>_static` |
| `MAREA_OPTION_SHARED_LIBS` | `OFF` | `Marea-<Stage>-Shared` (for example `Marea-Lex-Shared`) | `marea::<stage>_shared` |

With both `ON`, every stage has a static and a shared library next to each other, with the same base file name
(`libMarea-Lex.a` and `libMarea-Lex.so`). The alias `marea::<stage>` names the static library when it exists and the
shared one otherwise. Configure fails with a message when both options are `OFF`. Each stage also has an option,
`MAREA_OPTION_TARGET_<STAGE>`, that builds it; a stage needs the stage that it links (`lex` needs `base`, `parse` needs
`lex`, `sema` needs `parse`, `ir` needs `sema`, and `bytecode` needs `base`).
