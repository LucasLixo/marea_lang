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

## Bindings

Local variables, constants, primitive types, operators, scope and unsafe blocks, attributes, and the lex codes 1001-1006. The items sit in `tests/language/<class>/bindings/`.
A `known_gap` item that shares an issue shows one hole of the compiler through more than one program.

### Bindings: accept

| Id | Shows | Expected |
|---|---|---|
| L-BINDINGS-023 | An escape set of a string and a -- inside a string | ok |
| L-BINDINGS-024 | Character literals with escapes | ok |
| L-BINDINGS-025 | Number forms: separators, hex, binary, float, exponent | ok |
| L-BINDINGS-030 | The limits of sinteger8 and uinteger8 fit | ok |
| L-BINDINGS-036 | A shift count inside the width | ok |
| L-BINDINGS-046 | A shift count of another integer width fits | ok |
| L-BINDINGS-047 | A literal takes the family of the other operand | ok |
| L-BINDINGS-049 | The wrapping operators of the grammar | ok |
| L-BINDINGS-050 | The saturating operators of the grammar | ok |
| L-BINDINGS-051 | The plain operators of an integer | ok |
| L-BINDINGS-052 | The bit and shift operators of an unsigned integer | ok |
| L-BINDINGS-053 | The comparison and logical operators | ok |
| L-BINDINGS-057 | The conversion between widths is written | ok |
| L-BINDINGS-060 | A file with CRLF line ends | ok |
| L-BINDINGS-062 | A local that has the name of a parameter | ok |
| L-BINDINGS-063 | A local in a nested scope that has the name of an outer local | ok |
| L-BINDINGS-064 | Two locals of the same name in two sibling scopes | ok |
| L-BINDINGS-065 | A local that has the name of a file constant | ok |
| L-BINDINGS-069 | A name of the outer scope is visible inside | ok |
| L-BINDINGS-076 | A local of each primitive type with a literal | ok |
| L-BINDINGS-079 | Constants at file level | ok |
| L-BINDINGS-083 | A constant built from another constant | ok |
| L-BINDINGS-087 | A scope block frees its names at the end | ok |
| L-BINDINGS-088 | Scope blocks nested three deep | ok |
| L-BINDINGS-091 | A function that ends in a scope block with a return | ok |
| L-BINDINGS-092 | An unsafe block | ok |
| L-BINDINGS-096 | A visibility attribute, public and private | ok |
| L-BINDINGS-097 | A flag key with no value is true | ok |
| L-BINDINGS-098 | A flag key set to false | ok |
| L-BINDINGS-099 | An inline and a build attribute on a function | ok |
| L-BINDINGS-100 | Every member of the inline key | ok |
| L-BINDINGS-107 | A file attribute and a symbol attribute that differ | ok |
| L-BINDINGS-108 | A layout and an align attribute on a structure | ok |

### Bindings: reject

| Id | Shows | Expected |
|---|---|---|
| L-BINDINGS-001 | A stray at sign is an unknown byte | 1001 @10, 2001 @10 |
| L-BINDINGS-002 | A C style // comment is no comment, it is two divide operators | 2002 @10, 2002 @10, 2001 @10, 2001 @10, 2001 @11, 3004 @10, 3004 @10, 3004 @10 |
| L-BINDINGS-003 | A dollar sign is an unknown byte | 1001 @10, 2002 @10 |
| L-BINDINGS-004 | A lone hash is an unknown byte | 1001 @10, 2002 @10 |
| L-BINDINGS-005 | A question mark is an unknown byte | 1001 @10, 2001 @10 |
| L-BINDINGS-006 | A stray backslash is an unknown byte | 1001 @10, 2001 @10 |
| L-BINDINGS-007 | A control byte in code is an unknown byte | 1001 @10, 2001 @10 |
| L-BINDINGS-008 | A string cut by the line end is unterminated | 1002 @10, 2002 @10 |
| L-BINDINGS-009 | A character literal cut by the line end is unterminated | 1002 @10, 2002 @10 |
| L-BINDINGS-010 | A string cut by the end of the file is unterminated | 1002 @9, 2002 @9 |
| L-BINDINGS-011 | A bad escape in a string is INVALID_ESCAPE | 1003 @10 |
| L-BINDINGS-012 | A short hex escape is INVALID_ESCAPE | 1003 @10 |
| L-BINDINGS-013 | A bad escape in a character literal is INVALID_ESCAPE | 1003 @10 |
| L-BINDINGS-014 | A hex literal with no digit is malformed | 1004 @10, 2002 @10 |
| L-BINDINGS-015 | A binary literal with no digit is malformed | 1004 @10, 2002 @10 |
| L-BINDINGS-016 | A hex literal with an underscore first is malformed | 1004 @10, 2002 @10 |
| L-BINDINGS-017 | A binary literal with a decimal digit 2 after the prefix | 1004 @10, 2002 @10 |
| L-BINDINGS-018 | An exponent with no digit after the sign | 2001 @10, 2002 @10, 3004 @10 |
| L-BINDINGS-019 | An empty character literal | 1005 @10, 2002 @10 |
| L-BINDINGS-020 | A character literal of two characters | 1006 @10, 2002 @10 |
| L-BINDINGS-021 | A number glued to letters | 2001 @10, 3004 @10 |
| L-BINDINGS-038 | Sinteger32 plus sinteger64 is a mismatch | 3011 @12 |
| L-BINDINGS-039 | Sinteger32 plus float64 is a mismatch | 3011 @12 |
| L-BINDINGS-040 | A comparison of two widths is a mismatch | 3011 @12 |
| L-BINDINGS-041 | A bit operator of two widths is a mismatch | 3011 @12 |
| L-BINDINGS-042 | A wrapping operator of two widths is a mismatch | 3011 @12 |
| L-BINDINGS-043 | A power with a float exponent on an integer is a mismatch | 3011 @12 |
| L-BINDINGS-044 | A compound assignment of two widths is a mismatch | 3011 @12 |
| L-BINDINGS-045 | A float shift count is a mismatch | 3011 @12 |
| L-BINDINGS-048 | A fraction literal added to an integer is a mismatch | 3011 @11 |
| L-BINDINGS-054 | A chained comparison is not accepted | 2006 @13, 3011 @13 |
| L-BINDINGS-058 | An implicit widening is a mismatch | 3011 @11 |
| L-BINDINGS-059 | A string converted to a number is a mismatch | 3011 @11 |
| L-BINDINGS-066 | A name used before its declaration | 3004 @10 |
| L-BINDINGS-067 | A name used inside its own initializer | 3004 @10 |
| L-BINDINGS-068 | A name used after the scope that declared it | 3004 @13 |
| L-BINDINGS-070 | A retired integer name is an unknown type | 3009 @10 |
| L-BINDINGS-071 | A C type name is an unknown type | 3009 @10 |
| L-BINDINGS-072 | A boolean initialized by a number | 3011 @10 |
| L-BINDINGS-073 | A number initialized by a boolean | 3011 @10 |
| L-BINDINGS-074 | A string initialized by a number | 3011 @10 |
| L-BINDINGS-075 | A character initialized by a number | 3011 @10 |
| L-BINDINGS-077 | A local without a type | 2001 @10, 3004 @11 |
| L-BINDINGS-078 | A local without the final semicolon | 2001 @11 |
| L-BINDINGS-080 | A constant declared twice | 3003 @10 |
| L-BINDINGS-081 | A constant that holds the wrong type | 3011 @9 |
| L-BINDINGS-082 | A constant with no initial value | 2001 @9 |
| L-BINDINGS-085 | A constant declared inside a function | 2001 @10, 2004 @11, 2004 @11, 2005 @12, 3022 @9 |
| L-BINDINGS-086 | A function and a constant with the same name | 3003 @11 |
| L-BINDINGS-089 | A scope block with no end | 2009 @14 |
| L-BINDINGS-090 | A scope block written with curly braces | 2002 @10, 2002 @12 |
| L-BINDINGS-093 | An unsafe block with no end | 2009 @14 |
| L-BINDINGS-095 | A name declared in an unsafe block ends at its end | 3004 @13 |
| L-BINDINGS-101 | An unknown member of an enum key | 3006 @9 |
| L-BINDINGS-102 | A visibility key with a member that does not exist | 3006 @9 |
| L-BINDINGS-103 | A retired public key | 3005 @9 |
| L-BINDINGS-104 | An unknown attribute key | 3005 @9 |
| L-BINDINGS-105 | A boolean key with an enum member | 3006 @9 |
| L-BINDINGS-106 | The visibility key twice on one symbol | 3007 @9 |
| L-BINDINGS-109 | An align that is not a power of two | 3008 @9 |
| L-BINDINGS-110 | Align and pack on one structure | 3007 @9 |
| L-BINDINGS-111 | A layout key with an unknown member | 3006 @9 |
| L-BINDINGS-112 | An attribute with no symbol after it | 2004 @10 |
| L-BINDINGS-113 | An attribute line that ends with a semicolon | 2004 @9 |

### Bindings: known_gap

| Id | Shows | Expected |
|---|---|---|
| L-BINDINGS-022 | A literal too big for any integer type | gap: #181 NEW, rule: reject |
| L-BINDINGS-026 | A uinteger8 literal of 256 is out of range (rule 4) | gap: #181 NEW, rule: reject |
| L-BINDINGS-027 | A sinteger8 literal of 128 is out of range (rule 4) | gap: #181 NEW, rule: reject |
| L-BINDINGS-028 | A sinteger8 literal of -129 is out of range (rule 4) | gap: #181 NEW, rule: reject |
| L-BINDINGS-029 | A sinteger32 literal of 2147483648 is out of range (rule 4) | gap: #181 NEW, rule: reject |
| L-BINDINGS-031 | A negated unsigned literal | gap: #181 NEW, rule: reject |
| L-BINDINGS-032 | A negated unsigned value | gap: #182 NEW, rule: reject |
| L-BINDINGS-033 | A constant division by zero | gap: #183 NEW, rule: reject |
| L-BINDINGS-034 | A constant remainder by zero | gap: #183 NEW, rule: reject |
| L-BINDINGS-035 | A shift count equal to the width of the type | gap: #184 NEW, rule: reject |
| L-BINDINGS-037 | The minimum value divided by minus one | gap: #185 NEW, rule: reject |
| L-BINDINGS-055 | A bit operator on a float | gap: #186 NEW, rule: reject |
| L-BINDINGS-056 | A logical operator on integers | gap: #187 NEW, rule: reject |
| L-BINDINGS-061 | A local redeclared in the same scope | gap: #188 3003, rule: reject |
| L-BINDINGS-084 | A constant assigned inside a function | gap: #189 NEW, rule: reject |
| L-BINDINGS-094 | A raw pointer local outside an unsafe block | gap: #190 NEW, rule: reject |

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
