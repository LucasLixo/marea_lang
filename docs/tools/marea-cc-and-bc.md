# The marea-cc and marea-bc command-line tools

Not a compiler stage: `marea-cc` and `marea-bc` drive the seven stage libraries end to end from a
command line. Neither one owns compiler logic of its own, they only call the public API of `lex`,
`parse`, `sema`, `ir`, and then `cgen` (`marea-cc`) or `bytecode` (`marea-bc`), in order. This page
lists their real CLI surface; it is not a stage doc, so it has no Contract/Use/Gaps pattern.

Both tools are built by `MAREA_OPTION_TARGET_CC` / `MAREA_OPTION_TARGET_BC` (ON by default, each
depending on its own backend stage's own option), and are what `cmake/settings/
ConfigureMareaFunctions.cmake`'s `marea_add_library`/`marea_add_executable` locate through
`FindExecutable(NAME marea-cc ...)` / `marea-bc`.

## marea-cc

```
marea-cc <input.marea> -o <output> [options]
```

Lexes, parses, checks, lowers and runs cgen on `<input.marea>`, then, unless `--emit c`, invokes a
system C compiler as a subprocess to produce an object file or a linked executable.

| Flag | Meaning |
|---|---|
| `-o <output>` | Output path. Required. |
| `--emit c\|object\|executable` | What to write (default: `executable`). |
| `--cc <path>` | C compiler to invoke. Default: detect `clang` then `gcc` on `PATH`. |
| `--cflags <flags>` | Extra flags appended to the compiler invocation. |
| `--libraries <name,...>` | Enabled shipped libraries (`MAREA_LibraryMask`). Default: every one. |
| `--settings <NAME=value>` | A build setting (`MAREA_SemaSetting`); repeatable. |
| `--include <dir>` | An import search directory; repeatable. |
| `--target-architecture`, `--target-os`, `--target-compiler`, `--target-endianness`, `--target-pointer-width`, `--target-feature` | Override one field of the target description. Default: the host marea-cc itself runs on. `--target-feature` is repeatable. |
| `--list-settings` | Lists every `--settings` pair given on this command line, then exits. See the caveat below. |
| `--help`, `--version` | Usage and version text. |

For `--emit executable`, since cgen never writes a C `main` and the language has no fixed
entry-point convention of its own (see `AGENTS.md`, "Entry point"), a trivial
`int main(void) { return 0; }` is appended to the generated text before linking, only so the system
linker has an entry point for the demo binary - this does not define the language's own entry point.
The link step also adds `-lm` when linking to an executable, since a function that uses `**` or `%`
on a float calls `pow`/`fmod`.

A failure of the C compiler subprocess (non-zero exit) is reported as a plain error line to stderr,
with the compiler's own stderr forwarded, never as a `MAREA_Diagnostic` - the C compiler is outside
every stage's diagnostic model.

## marea-bc

```
marea-bc <mode> [options]
```

| Mode | Meaning |
|---|---|
| `compile <input.marea> -o <output.mbc>` | Lex, parse, check, lower and `MAREA_BytecodeGenerate`, then `MAREA_BytecodeSerialize` to `<output.mbc>`. |
| `assemble <input.mbca> -o <output.mbc>` | `MAREA_BytecodeAssemble` a text module, then serialize it. |
| `disassemble <input.mbc> -o <output.mbca>` | Deserialize a binary module, then `MAREA_BytecodeDisassemble` to text. |
| `dump-tokens <input.marea>` | Lexes and prints one line per token (index, kind, line:column, text) to stdout. |
| `dump-ast <input.marea>` | Lexes and parses, then prints `MAREA_AstDump`'s own text form to stdout. |
| `dump-ir <input.marea>` | Lexes, parses, checks and lowers, then prints `MAREA_IrModuleDump`'s own text form to stdout. |

The options are the same `--libraries`, `--settings`, `--include`, `--target-*`, `--list-settings`,
`--help` and `--version` flags `marea-cc` takes, with the same meaning.

## The --list-settings caveat

The plan (`DECISIONS.md` row 52) calls for "list settings"/"list libraries" to list every
`build_value!` call of the compiled unit, with its name, type and default. The typed-AST accessor
API does not yet expose reading a `build_value!` call site back out of a checked unit, so both tools
give the close, honest approximation this apply's own text allows: `--list-settings` lists every
`--settings NAME=value` pair actually given on the command line, not every `build_value!` call the
unit makes. Closing that gap needs a typed-AST accessor this apply does not add.

## Exit codes

Both tools exit `0` on success. Any failure - a missing or malformed argument, a diagnostic of a
driven stage, a failing C compiler subprocess (`marea-cc` only) - exits `1`, with the explanation on
stderr.
