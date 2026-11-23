# Marea language catalogue

## Contract

This doc owns the catalogue of the language: what it accepts, what it must reject with which diagnostic, and what
it accepts today but must not. Each case is one small program, an item, under `tests/language/`. One test runner,
`tests/source/language/MAREALanguageCatalogue.cpp`, lexes, parses and checks every item and compares the result with
the header of the item. The catalogue reads and writes no stage data of its own: it is a consumer of `lex`, `parse`
and `sema`, and it changes what none of them accepts. Every item has one row in the tables of this doc.

## Use

```sh
mise run config:cmake
cmake --build cmake-build-debug --target marea_tests --parallel
ctest --test-dir cmake-build-debug --output-on-failure -R MAREALanguageCatalogue
```

Two tests walk the folder: `EveryItemMatchesItsHeader` runs every item, and `StructureOfTheCatalogue` checks where
the items sit and how they are named. A failure names the file of the item. The other tests of the suite feed the
runner with bad items in memory, so each failure direction is proven on every build.

## Item format

One program per item, at `tests/language/<class>/<area>/<id>.marea`.

- `<class>` is `accept`, `reject` or `known_gap`.
- `<area>` is a lower case name (`[a-z][a-z0-9_]*`), for example `seed`.
- `<id>` is `L-<AREA>-<nnn>`: `AREA` is the upper case of the area folder, `nnn` is three digits. The file name is
  `<id>.marea`, and an id is unique in the whole catalogue, across the classes and the areas.
- The program is one file with the namespace `main` (the runner checks it as `main.marea`). The comment mark is
  `--`, the only Marea comment.

The first line is the header. It has one of three forms:

| First line | Class | Meaning |
|---|---|---|
| `-- expect: ok` | `accept` | Lex, parse and sema give no error. |
| `-- expect: <code> @<line>[, <code> @<line>]` | `reject` | The diagnostics are exactly these codes, in the order the compiler gives them, each on its 1-based source line. |
| `-- gap: #<issue> <code>` | `known_gap` | The program is accepted today, but the language should give `<code>`. `<code>` is `NEW` when no code exists yet. |

A `known_gap` item has `-- rule: reject` or `-- rule: accept` as its second line: the outcome the language has
decided. A `-- rule:` line in another class is an error. The copyright header follows, then the program.

The codes are the lex codes 1001-1006, the parse codes 2001-2010 and the sema codes 3001-3032 (3023 is retired).
The `@<line>` of the form is optional for the parser of the header, but a `reject` item with a code that has no
`@<line>` fails.

## The three classes

- **accept**: a program the language must accept. The runner fails if any error is given.
- **reject**: a program the language must reject. The runner fails if the codes or the lines differ from the header.
- **known_gap**: a program the compiler accepts today and a settled rule says it must not (or the reverse, see
  `-- rule:`). Each one has its own GitHub issue. The runner needs the program to be accepted now, so the day a fix
  lands the item fails and forces its move.

## Moving an item from known_gap to reject

When the fix of the issue lands, the `known_gap` item starts to give an error and the runner fails with the file
name. Then, in the same change:

1. Move the file to `reject/<area>/`, with the same id.
2. Replace the header with `-- expect: <code> @<line>`, using the real code and line, and remove the `-- rule:`
   line.
3. Move the row from the known_gap table to the reject table, and close the issue.

A `-- rule: accept` item moves to `accept/` with `-- expect: ok` in the same way.

## Settled rules

These rules bind every item (user decision 2026-11-23).

1. The header comment is `--`; `//` is a lex error (1001).
2. A non-primitive parameter is a read-only borrow by default; writing needs `mutable`.
3. A local `x: T;` with no initial value is an error.
4. A literal outside the range of its type is a compile-time error and is never truncated; plain `+`, `-` and `*`
   wrap in two's complement; checked and saturating forms are explicit operators.
5. `private` visibility is enforced across namespaces.

Where the compiler does not enforce a rule yet, the item is a `known_gap` with `-- rule:`.

## Catalogue: accept

| Id | Shows | Expected |
|---|---|---|
| L-SEED-001 | a function with a return type that ends its path with `return value;` | ok |

## Catalogue: reject

| Id | Shows | Expected |
|---|---|---|
| L-SEED-002 | a name that no scope declares | 3004 @10 |

## Catalogue: known_gap

| Id | Shows | Expected |
|---|---|---|
| L-SEED-003 | a local with no initial value (rule 3), issue #177 | gap: NEW, rule: reject |

## Robustness

One test file, `tests/source/language/MAREALanguageRobustness.cpp`, feeds broken input to `lex`, `parse` and `sema` and
checks that each call returns, with no crash and no hang, and that every diagnostic has a line inside the file (1 to
the number of lines). It uses no external fuzzer: every input is fixed or comes from a fixed seed. A watchdog aborts
the run after 60 seconds, and the whole sweep stays near 10 seconds in the debug build. It changes what no stage
accepts or rejects.

| Sweep | Input | Result |
|---|---|---|
| Truncation | each of the 23 examples cut at every token boundary (4562 cases) | returns |
| Token mutation | each example with one token deleted, duplicated or swapped with the next (13663 cases) | returns |
| Byte flips | each example with 1 to 4 bytes replaced, 300 per example, seed `0x4D415245` (6900 cases) | returns |
| Degenerate files | empty, only newlines, only comments, a NUL byte, a line of one million characters, a string never closed | returns |
| Depth | parentheses, unary and binary operators, blocks, ifs, generic arguments, closed and unclosed, at depth 100 and 1000 | returns |
| Depth 10000 | parentheses, unary operators, unclosed parentheses | returns |

Defects found. Each has a disabled test in the file, named `DISABLED_KnownDefect...`, that shows it when enabled:

| Input | Defect | Issue |
|---|---|---|
| a chain of 100000 binary operators | `sema` overflows the stack | #192 |
| 10000 nested generic arguments, closed or unclosed | `parse` takes over 60 seconds, or about 6 seconds when unclosed | #193 |
| 10000 nested ifs or blocks | about 39 seconds and about 12 seconds | #194 |

## Diagnostics

The catalogue raises no diagnostic of its own. A `reject` item carries a code of another stage:

| Codes | Stage | Reference |
|---|---|---|
| 1001-1006 | `lex` | [marea-lex.md](../lex/marea-lex.md) |
| 2001-2010 | `parse` | [marea-ast.md](../ast/marea-ast.md) |
| 3001-3032 | `sema` | [marea-sema.md](../sema/marea-sema.md) |

## Gaps

- The runner checks one file per item, so an item cannot import a second file of the catalogue. A rule that needs
  two namespaces (settled rule 5) can use only an `EXTERNAL` import, which the runner gives to the `c.`,
  `collections.`, `concurrency.`, `arenas.` prefixes and to `text`, until a later change adds a way to give an item a
  second file.
- The runner compares the codes of a `reject` item in the order the compiler gives them, so an item over two stages
  depends on the order of the stages (lex, then parse, then sema).
- Only the seed items exist. The area items are added by later changes.
