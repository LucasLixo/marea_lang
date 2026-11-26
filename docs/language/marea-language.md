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

Two tests of the runner walk the folder: `EveryItemMatchesItsHeader` runs every item, and `StructureOfTheCatalogue` checks where
the items sit and how they are named. A failure names the file of the item. The other tests of the suite feed the
runner with bad items in memory, so each failure direction is proven on every build.

A second file, `tests/source/language/MAREALanguageCoverage.cpp`, checks that the catalogue is complete: the examples,
the doc rows, the counts and the diagnostic codes (see Examples and coverage). Run it with `-R MAREALanguageCoverage`.

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

The codes are the lex codes 1001-1006, the parse codes 2001-2010 and the sema codes 3001-3081 (3023 is retired, and not every other number of the range is a live code).
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
2. A non-primitive parameter is a read-only borrow by default; writing needs the explicit `#[semantics = reference_mutable]`.
3. A local `x: T;` with no initial value is an error.
4. A literal outside the range of its type is a compile-time error and is never truncated; plain `+`, `-` and `*`
   wrap in two's complement; checked and saturating forms are explicit operators.
5. `private` visibility is enforced across namespaces (3066, 3067, 3068, proven by `tests/source/sema/MAREASemaVisibility.cpp`). The default visibility is public.

Where the compiler does not enforce a rule yet, the item is a `known_gap` with `-- rule:`.

## Catalogue: accept

| Id | Shows | Expected |
|---|---|---|
| L-SEED-001 | a function with a return type that ends its path with `return value;` | ok |

## Catalogue: reject

| Id | Shows | Expected |
|---|---|---|
| L-SEED-002 | a name that no scope declares | 3004 @10 |
| L-SEED-003 | a local with no initial value (rule 3) | 3065 @10 |

## Catalogue: known_gap

| Id | Shows | Expected |
|---|---|---|

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
| L-BINDINGS-062 | A local of an inner block that has the name of a parameter | ok |
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
| L-BINDINGS-022 | A literal too big for any integer type | 3033 @10 |
| L-BINDINGS-026 | A uinteger8 literal of 256 is out of range (rule 4) | 3033 @10 |
| L-BINDINGS-027 | A sinteger8 literal of 128 is out of range (rule 4) | 3033 @10 |
| L-BINDINGS-028 | A sinteger8 literal of -129 is out of range (rule 4) | 3033 @10 |
| L-BINDINGS-029 | A sinteger32 literal of 2147483648 is out of range (rule 4) | 3033 @10 |
| L-BINDINGS-031 | A negated unsigned literal | 3033 @10 |
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
| L-BINDINGS-032 | A negated unsigned value | 3036 @11 |
| L-BINDINGS-037 | The minimum value divided by minus one | 3037 @10 |
| L-BINDINGS-033 | A constant division by zero | 3034 @10 |
| L-BINDINGS-034 | A constant remainder by zero | 3034 @10 |
| L-BINDINGS-035 | A shift count equal to the width of the type | 3035 @11 |
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
| L-BINDINGS-055 | A bit operator on a float | 3038 @12 |
| L-BINDINGS-056 | A logical operator on integers | 3039 @12 |
| L-BINDINGS-058 | An implicit widening is a mismatch | 3011 @11 |
| L-BINDINGS-059 | A string converted to a number is a mismatch | 3011 @11 |
| L-BINDINGS-061 | A local redeclared in the same scope | 3003 @11 |
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
| L-BINDINGS-084 | A constant assigned inside a function | 3040 @12 |
| L-BINDINGS-085 | A constant declared inside a function | 2001 @10, 2004 @11, 2004 @11, 2005 @12, 3022 @9 |
| L-BINDINGS-086 | A function and a constant with the same name | 3003 @11 |
| L-BINDINGS-089 | A scope block with no end | 2009 @14 |
| L-BINDINGS-090 | A scope block written with curly braces | 2002 @10, 2002 @12 |
| L-BINDINGS-093 | An unsafe block with no end | 2009 @14 |
| L-BINDINGS-094 | A raw pointer local outside an unsafe block | 3041 @10, 3065 @10 |
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

## Ownership and borrows

Assignment with `copy` and `move`, the passing mode of a parameter, aliasing of arguments, writes through a read-only
borrow, references that escape, constants as borrows, owned parameters, use after move and uninitialized locals. The
items sit in `tests/language/<class>/ownership/`. Every reject has a sibling accept that differs only by the fix, and the
path of the value changes: local, parameter, field, vector element, `for each` element, `with` name, branch of `if`,
arm of `switch`, loop.

What sema enforces today is small: 3016 (an existing structure, field or element assigned with no word), 3017
(`value_read_only` on a non-primitive), 3018 (a borrow or a field of a borrow returned, stored in a field, appended to a vector, or a partition part stored outside its body), 3019 (a bare name passed twice
with a write borrow among them, or two places of one call that overlap, a field and its owner, one field or element twice, a loop element and its own vector, a value and a move of it, when one is a write borrow), 3032 (a file constant, or a field or element of one, as a write borrow), 3069 (a write through a read-only
borrow, and a write borrow of a call result or a literal), 3073 (the source of a `for each` or a `with` block written, moved or appended to inside its body). Everything else in the known_gap tables below is accepted
today. The holes are grouped by kind: memory, permission and aliasing. A gap that shares an
issue shows one hole through more than one program.

Settled rule 2 (the default borrow is read-only) holds in sema, the grammar and the examples: a parameter with no
attribute is a read-only borrow, unless its type is by value (a primitive, an enum, a registered type of the copy class) or
moves in (a registered type of the move class). A write needs the explicit `#[semantics = reference_mutable]`. Sema rejects a
write through a read-only borrow with 3069: an assignment, a `for each ... in mutable` or `partition mutable` over a
read-only source, a read-only borrow given to a write-borrow parameter, and a call result or a literal given to a
write-borrow parameter.

### Ownership and borrows: accept

| Id | Shows | Expected |
|---|---|---|
| L-OWNERSHIP-001 | A local initialized from an existing structure with copy | ok |
| L-OWNERSHIP-003 | A local initialized from an existing structure with move | ok |
| L-OWNERSHIP-005 | An assignment of an existing structure with copy | ok |
| L-OWNERSHIP-006 | An assignment of an existing structure with move | ok |
| L-OWNERSHIP-008 | A field of a structure assigned with copy | ok |
| L-OWNERSHIP-010 | An element of a vector assigned with copy | ok |
| L-OWNERSHIP-012 | A parameter assigned to a local with copy | ok |
| L-OWNERSHIP-014 | A for each element assigned to a local with copy | ok |
| L-OWNERSHIP-016 | An existing structure assigned with copy in a branch of if | ok |
| L-OWNERSHIP-018 | An existing structure assigned with copy in an arm of switch | ok |
| L-OWNERSHIP-020 | An existing structure assigned with copy in a loop | ok |
| L-OWNERSHIP-022 | A with name assigned to a local with copy | ok |
| L-OWNERSHIP-024 | A structure constant assigned to a local with copy | ok |
| L-OWNERSHIP-025 | A primitive is assigned with no word | ok |
| L-OWNERSHIP-026 | A primitive may be moved and copied | ok |
| L-OWNERSHIP-027 | An enum value is assigned with no word | ok |
| L-OWNERSHIP-028 | A new value from a call needs no word | ok |
| L-OWNERSHIP-029 | A new value from a literal needs no word | ok |
| L-OWNERSHIP-030 | A slice is a view and is assigned with no word | ok |
| L-OWNERSHIP-031 | An array of primitives is copied by assignment | ok |
| L-OWNERSHIP-033 | A vector assigned with copy | ok |
| L-OWNERSHIP-034 | A vector assigned with move | ok |
| L-OWNERSHIP-037 | The new owner is used after the move | ok |
| L-OWNERSHIP-040 | A structure is copied in a loop body | ok |
| L-OWNERSHIP-043 | A structure is moved in a branch that leaves the function | ok |
| L-OWNERSHIP-045 | A structure is moved in an arm of switch that leaves the function | ok |
| L-OWNERSHIP-047 | A structure is last used by the call that moves it | ok |
| L-OWNERSHIP-049 | A moved name is assigned again and then used | ok |
| L-OWNERSHIP-052 | A field is moved out and put back before the owner is used | ok |
| L-OWNERSHIP-059 | A local with an initial value is read | ok |
| L-OWNERSHIP-061 | A local is assigned in both branches and then read | ok |
| L-OWNERSHIP-063 | A local is assigned in every arm of switch and then read | ok |
| L-OWNERSHIP-067 | Two values are passed to two write-borrow parameters | ok |
| L-OWNERSHIP-070 | One value is passed to two read-borrow parameters | ok |
| L-OWNERSHIP-075 | A field and a copy of its owner are passed to a write and a read borrow | ok |
| L-OWNERSHIP-078 | Two elements are passed to two write-borrow parameters | ok |
| L-OWNERSHIP-080 | A value is passed to a write borrow and another is moved in the same call | ok |
| L-OWNERSHIP-081 | A write borrow and a copy of the same value in one call | ok |
| L-OWNERSHIP-083 | A vector is moved after a for each ended | ok |
| L-OWNERSHIP-085 | The elements are written through a for each mutable | ok |
| L-OWNERSHIP-087 | The source of a with block is only read inside the block | ok |
| L-OWNERSHIP-090 | A parameter with no attribute is only read | ok |
| L-OWNERSHIP-091 | A parameter with the write attribute is written | ok |
| L-OWNERSHIP-095 | A nested field of a write borrow is written | ok |
| L-OWNERSHIP-102 | A write borrow is passed on to a write-borrow parameter | ok |
| L-OWNERSHIP-104 | A parameter with no attribute is passed on to a read-borrow parameter | ok |
| L-OWNERSHIP-106 | A primitive parameter may be marked as a write borrow | ok |
| L-OWNERSHIP-107 | A primitive parameter may be marked as a read-only borrow | ok |
| L-OWNERSHIP-109 | The value copy attribute on a primitive parameter | ok |
| L-OWNERSHIP-114 | A copy of a parameter with no attribute is returned | ok |
| L-OWNERSHIP-118 | A copy of a for each variable is returned | ok |
| L-OWNERSHIP-120 | A local is returned and the owner goes up | ok |
| L-OWNERSHIP-122 | A copy of a borrow is stored in a field of a literal | ok |
| L-OWNERSHIP-124 | A copy of a borrow is assigned to a field | ok |
| L-OWNERSHIP-127 | A copy of a borrow is appended to a vector | ok |
| L-OWNERSHIP-130 | A read borrow of a call result | ok |
| L-OWNERSHIP-132 | A read borrow of a literal | ok |
| L-OWNERSHIP-134 | A file constant is passed to a read-borrow parameter | ok |
| L-OWNERSHIP-135 | A copy of a file constant is passed to a write-borrow parameter | ok |
| L-OWNERSHIP-137 | A field of a file constant is passed to a read-borrow parameter | ok |
| L-OWNERSHIP-139 | A value is moved into an owned parameter | ok |
| L-OWNERSHIP-140 | A value is copied into an owned parameter | ok |
| L-OWNERSHIP-141 | A new value is given to an owned parameter | ok |
| L-OWNERSHIP-143 | A local of an owned type takes a value with move | ok |
| L-OWNERSHIP-145 | An owned parameter is returned and the owner goes up | ok |

### Ownership and borrows: reject

| Id | Shows | Expected |
|---|---|---|
| L-OWNERSHIP-002 | A local initialized from an existing structure with no word | 3016 @25 |
| L-OWNERSHIP-004 | A plain assignment of an existing structure | 3016 @26 |
| L-OWNERSHIP-007 | A field of a structure assigned with no word | 3016 @25 |
| L-OWNERSHIP-009 | An element of a vector assigned with no word | 3016 @20 |
| L-OWNERSHIP-011 | A parameter assigned to a local with no word | 3016 @20 |
| L-OWNERSHIP-013 | A for each element assigned to a local with no word | 3016 @21 |
| L-OWNERSHIP-015 | An existing structure assigned with no word in a branch of if | 3016 @26 |
| L-OWNERSHIP-017 | An existing structure assigned with no word in an arm of switch | 3016 @27 |
| L-OWNERSHIP-019 | An existing structure assigned with no word in a loop | 3016 @26 |
| L-OWNERSHIP-021 | A with name assigned to a local with no word | 3016 @26 |
| L-OWNERSHIP-023 | A structure constant assigned to a local with no word | 3016 @22 |
| L-OWNERSHIP-032 | A vector assigned with no word | 3016 @20 |
| L-OWNERSHIP-035 | A library array assigned with no word | 3016 @22 |
| L-OWNERSHIP-036 | A structure is used after it was moved | 3070 @26 |
| L-OWNERSHIP-038 | A structure is moved twice | 3071 @26 |
| L-OWNERSHIP-039 | A structure is moved in a loop body, so the second turn uses a moved value | 3071 @27 |
| L-OWNERSHIP-041 | A structure is moved in a for loop and the owner is outside | 3071 @26 |
| L-OWNERSHIP-042 | A structure is moved in one branch and used after the branch | 3070 @28 |
| L-OWNERSHIP-044 | A structure is moved in one arm of switch and used after it | 3070 @30 |
| L-OWNERSHIP-046 | A structure is used after it was moved into an owned parameter | 3070 @34 |
| L-OWNERSHIP-048 | An owned parameter is used after it was moved on | 3070 @25 |
| L-OWNERSHIP-050 | A structure is used after the box that holds it was moved | 3070 @26 |
| L-OWNERSHIP-051 | A field is moved out and the owner is used | 3072 @26 |
| L-OWNERSHIP-053 | A field is moved out and then the whole structure is moved | 3072 @26 |
| L-OWNERSHIP-054 | An element of a vector is moved out and the vector is used | 3072 @21 |
| L-OWNERSHIP-055 | A for each element is moved out of the loop owner | 3072 @21 |
| L-OWNERSHIP-058 | A local with no initial value is read | 3065 @20 |
| L-OWNERSHIP-060 | A local is assigned in one branch only and then read | 3065 @20 |
| L-OWNERSHIP-062 | A local is assigned in one arm of switch only and then read | 3065 @20 |
| L-OWNERSHIP-064 | A local is assigned in a loop only and then read | 3065 @20 |
| L-OWNERSHIP-065 | A structure local with no initial value | 3065 @20 |
| L-OWNERSHIP-066 | One value is passed to two write-borrow parameters | 3019 @30 |
| L-OWNERSHIP-068 | One value is passed to a write borrow and then a read borrow | 3019 @29 |
| L-OWNERSHIP-069 | One value is passed to a read borrow and then a write borrow | 3019 @29 |
| L-OWNERSHIP-071 | A parameter is passed to two write-borrow parameters | 3019 @25 |
| L-OWNERSHIP-072 | A for each element is passed to two write-borrow parameters | 3019 @26 |
| L-OWNERSHIP-073 | One value is passed to two write-borrow parameters in a branch | 3019 @31 |
| L-OWNERSHIP-074 | A field and its owner are passed to a write and a read borrow | 3019 @29 |
| L-OWNERSHIP-076 | One field is passed to two write-borrow parameters | 3019 @30 |
| L-OWNERSHIP-077 | One element is passed to two write-borrow parameters | 3019 @25 |
| L-OWNERSHIP-079 | A value is passed to a write borrow and moved in the same call | 3019 @29 |
| L-OWNERSHIP-082 | A vector is moved while a for each reads it | 3073 @21 |
| L-OWNERSHIP-084 | An element is written while a for each reads the vector | 3073 @21 |
| L-OWNERSHIP-086 | The source of a with block is written inside the block | 3073 @26 |
| L-OWNERSHIP-088 | The source of a with block is moved inside the block | 3073 @26 |
| L-OWNERSHIP-089 | A parameter with no attribute is written (rule 2: the default borrow is read-only) | 3069 @20 |
| L-OWNERSHIP-092 | A field of an explicit read-only borrow is written | 3069 @20 |
| L-OWNERSHIP-093 | A field of an explicit read-only borrow is changed with a compound operator | 3069 @20 |
| L-OWNERSHIP-094 | A nested field of a read-only borrow is written | 3069 @20 |
| L-OWNERSHIP-096 | An element of a read-only vector is written | 3069 @20 |
| L-OWNERSHIP-097 | A whole read-only borrow is assigned | 3069 @24 |
| L-OWNERSHIP-098 | A for each variable is written | 3069 @21 |
| L-OWNERSHIP-099 | A with name is written | 3069 @26 |
| L-OWNERSHIP-100 | A for each mutable over a read-only vector | 3069 @20, 3069 @21 |
| L-OWNERSHIP-101 | A read-only borrow is passed on to a write-borrow parameter | 3069 @24 |
| L-OWNERSHIP-103 | A parameter with no attribute is passed on to a write-borrow parameter | 3069 @24 |
| L-OWNERSHIP-105 | The keyword mutable on a parameter is a parse error (the grammar spells the write borrow as an attribute) | 2001 @19 |
| L-OWNERSHIP-108 | The value copy attribute on a structure parameter | 3017 @19 |
| L-OWNERSHIP-110 | The value copy attribute on an enum parameter | 3017 @24 |
| L-OWNERSHIP-111 | The semantics attribute with an unknown member | 3006 @19 |
| L-OWNERSHIP-112 | The retired value attribute on a structure | 3005 @19 |
| L-OWNERSHIP-113 | A parameter with no attribute is returned | 3018 @20 |
| L-OWNERSHIP-115 | A read-only borrow is returned | 3018 @20 |
| L-OWNERSHIP-116 | A write borrow is returned | 3018 @20 |
| L-OWNERSHIP-117 | A for each variable is returned | 3018 @21 |
| L-OWNERSHIP-119 | A with name is returned | 3018 @26 |
| L-OWNERSHIP-121 | A borrow is stored in a field of a literal | 3018 @20 |
| L-OWNERSHIP-123 | A borrow is assigned to a field | 3018 @25, 3016 @25 |
| L-OWNERSHIP-125 | A borrow is stored in an element of a vector | 3016 @20 |
| L-OWNERSHIP-126 | A borrow is appended to a vector | 3018 @20 |
| L-OWNERSHIP-128 | A borrow is returned through a generic function | 3018 @20 |
| L-OWNERSHIP-129 | A write borrow of a call result | 3069 @28 |
| L-OWNERSHIP-131 | A write borrow of a literal | 3069 @24 |
| L-OWNERSHIP-133 | A file constant is passed to a write-borrow parameter | 3032 @26 |
| L-OWNERSHIP-136 | A field of a file constant is passed to a write-borrow parameter | 3032 @26 |
| L-OWNERSHIP-138 | A file constant is passed to a write-borrow parameter in a branch | 2001 @29, 3032 @27 |
| L-OWNERSHIP-144 | A local of an owned type takes a value with no word | 3016 @25 |

### Ownership and borrows: known_gap

#### Memory holes

| Id | Shows | Expected |
|---|---|---|
| L-OWNERSHIP-056 | A read-only borrow is moved out | gap: #200 NEW, rule: reject |
| L-OWNERSHIP-057 | A file constant is moved out | gap: #200 NEW, rule: reject |
| L-OWNERSHIP-142 | An existing value is given to an owned parameter with no word | 3016 @29 |

## Memory and regions

Scopes that end, data that goes up with `return value;` and `move`, `construct` and `destroy`, the explicit call of a
lifecycle operator (3027) and structured `fork_join` with `partition` and `spawn` (3020, 3024, 3074). The items sit in
`tests/language/<class>/memory/`. An accept item of `construct` and `destroy` carries the expected order in a
`-- output:` comment for the future vm; the runner reads no output today.

### Memory and regions: accept

| Id | Shows | Expected |
|---|---|---|
| L-MEMORY-001 | A function returns a local and the owner goes up to the caller | ok |
| L-MEMORY-002 | Data goes up through two calls | ok |
| L-MEMORY-003 | A local of a scope block ends at its end | ok |
| L-MEMORY-005 | An owner moves up from an inner scope to an outer one | ok |
| L-MEMORY-010 | A structure with construct and destroy is made and goes out of scope once | ok |
| L-MEMORY-011 | Two locals are destroyed in reverse order of their creation | ok |
| L-MEMORY-012 | A moved structure is destroyed once, by the new owner | ok |
| L-MEMORY-013 | A structure made in each turn of a loop is destroyed in the same turn | ok |
| L-MEMORY-014 | A structure of an inner scope block is destroyed at its end | ok |
| L-MEMORY-015 | A returned structure is destroyed once, by the caller | ok |
| L-MEMORY-016 | A structure is made by its construct operator with a call form | ok |
| L-MEMORY-022 | A partition body spawns a worker on its part | ok |
| L-MEMORY-024 | A worker spawned on a read-borrow parameter | ok |
| L-MEMORY-027 | Two workers read the same local | ok |
| L-MEMORY-033 | A field of a local is read inside its own scope block | ok |

### Memory and regions: reject

| Id | Shows | Expected |
|---|---|---|
| L-MEMORY-004 | A local of a scope block is used after the block | 3004 @27 |
| L-MEMORY-006 | An inner owner is used after it moved up | 3070 @28 |
| L-MEMORY-007 | An outer owner is moved down into an inner scope and used after | 3070 @28 |
| L-MEMORY-008 | A name of a nested scope block does not leave it | 3004 @28 |
| L-MEMORY-009 | A reference stored in an outer local from an inner scope | 3073 @27, 3016 @27 |
| L-MEMORY-017 | The destroy operator is called by its bare name | 3004 @34, 3027 @34 |
| L-MEMORY-018 | The destroy operator is called by the static form | 3027 @34 |
| L-MEMORY-019 | The construct operator is called by the static form | 3027 @34 |
| L-MEMORY-020 | The destroy operator is called on a value | 3027 @35 |
| L-MEMORY-021 | A structure is destroyed twice by moving it and keeping the old owner | 3071 @36 |
| L-MEMORY-023 | A worker spawned on a write-borrow parameter | 3020 @29 |
| L-MEMORY-025 | A worker spawned on a call result | 3020 @29 |
| L-MEMORY-026 | Two workers write the same local | 3074 @31 |
| L-MEMORY-028 | A worker writes a local that the block reads at the same time | 3074 @35 |
| L-MEMORY-029 | A return leaves the body of a partition | 3024 @22 |
| L-MEMORY-030 | A worker is spawned outside of a fork_join block | 3062 @29 |
| L-MEMORY-031 | A worker takes the part of a partition mutable while the source is also spawned | 3020 @30 |
| L-MEMORY-032 | A field of a scope block local is read after the block | 3004 @27 |

### Memory and regions: known_gap

| Id | Shows | Expected |
|---|---|---|

## Functions and calls

Function declaration and call, parameters and defaults, the result and `return value;` on every path, recursion, function types as values, the passing mode of a parameter, and the diagnostics 3010, 3011, 3017 to 3019, 3022, 3030, 3032, 3053 and 3054 as these show them. The items sit in `tests/language/<class>/functions/`.

### Functions and calls: accept

| Id | Shows | Expected |
|---|---|---|
| L-FUNCTIONS-001 | a function with two parameters and a result | ok |
| L-FUNCTIONS-002 | a function with no result that leaves early with a bare return | ok |
| L-FUNCTIONS-003 | direct recursion | ok |
| L-FUNCTIONS-004 | a function type alias used for a parameter, and a function passed as a value | ok |
| L-FUNCTIONS-005 | a function type with no result held by a local | ok |
| L-FUNCTIONS-006 | a default parameter value, a call with and a call without the argument | ok |
| L-FUNCTIONS-007 | a function that returns a structure literal | ok |
| L-FUNCTIONS-008 | return value on every path of an if, elseif and else chain | ok |
| L-FUNCTIONS-009 | return value on every arm of a switch that has a default | ok |
| L-FUNCTIONS-010 | return value inside a scope block | ok |
| L-FUNCTIONS-011 | unreachable ends the path of a function with a result | ok |
| L-FUNCTIONS-012 | nested calls, a call as a statement and an explicit conversion | ok |
| L-FUNCTIONS-013 | a function that reads a field of a structure parameter | ok |
| L-FUNCTIONS-014 | an explicit write borrow parameter that changes its argument | ok |
| L-FUNCTIONS-015 | an explicit read-only borrow parameter that only reads | ok |
| L-FUNCTIONS-016 | a function that returns a function type value | ok |
| L-FUNCTIONS-017 | an assignment of a local that a call result initializes | ok |
| L-FUNCTIONS-018 | a unary function with the result of a boolean operator | ok |
| L-FUNCTIONS-019 | a function with a constant default and a file constant | ok |

### Functions and calls: reject

| Id | Shows | Expected |
|---|---|---|
| L-FUNCTIONS-020 | a function with a result and an empty body | 3022 @9 |
| L-FUNCTIONS-021 | an if with no else as the whole body of a function with a result | 3022 @9 |
| L-FUNCTIONS-022 | an if and elseif chain with no else | 3022 @9 |
| L-FUNCTIONS-023 | a while loop is never an ending path | 3022 @9 |
| L-FUNCTIONS-024 | a switch with no default over an integer, the last path has no return | 3022 @9 |
| L-FUNCTIONS-025 | one branch of an if returns and the other falls through | 3022 @9 |
| L-FUNCTIONS-026 | a call with too few arguments | 3010 @14 |
| L-FUNCTIONS-027 | a call with too many arguments | 3010 @14 |
| L-FUNCTIONS-028 | a call with no argument for a function that needs two | 3010 @14 |
| L-FUNCTIONS-029 | a call with too many arguments when one parameter has a default | 3010 @14 |
| L-FUNCTIONS-030 | a call with no argument when only the second parameter has a default | 3010 @14 |
| L-FUNCTIONS-031 | a text argument for an integer parameter | 3011 @14 |
| L-FUNCTIONS-032 | an integer argument of another width | 3011 @15 |
| L-FUNCTIONS-033 | a float argument for an integer parameter | 3011 @15 |
| L-FUNCTIONS-034 | a return value of another type | 3011 @10 |
| L-FUNCTIONS-035 | a text return value for an integer result | 3011 @10 |
| L-FUNCTIONS-036 | a local initialized with a value of another type | 3011 @10 |
| L-FUNCTIONS-037 | a call of a function that no scope declares | 3004 @10 |
| L-FUNCTIONS-038 | a name that no scope declares, used as an argument | 3004 @14 |
| L-FUNCTIONS-039 | a default value that names an undeclared constant | 3004 @9 |
| L-FUNCTIONS-040 | a default value of another type | 3011 @9 |
| L-FUNCTIONS-041 | two functions with one name | 3003 @13 |
| L-FUNCTIONS-042 | a function and a structure with one name | 3003 @13 |
| L-FUNCTIONS-043 | a parameter with a default before a parameter without one | 3030 @9 |
| L-FUNCTIONS-044 | two parameters with a default before a required one | 3030 @9, 3030 @9 |
| L-FUNCTIONS-045 | a parameter of a type that does not exist | 3009 @9 |
| L-FUNCTIONS-046 | a result type that does not exist | 3009 @9 |
| L-FUNCTIONS-047 | a local of a type that does not exist | 3009 @10 |
| L-FUNCTIONS-048 | a value_read_only parameter of a structure | 3017 @14 |
| L-FUNCTIONS-049 | a borrow parameter returned | 3018 @15 |
| L-FUNCTIONS-050 | a read-only borrow parameter returned | 3018 @15 |
| L-FUNCTIONS-051 | one structure passed to two write borrow parameters | 3019 @20 |
| L-FUNCTIONS-052 | a file constant passed to a write borrow parameter | 3032 @21 |
| L-FUNCTIONS-053 | a structure assigned from a name with no copy or move | 3016 @17 |
| L-FUNCTIONS-054 | a local initialized from another structure name with no copy or move | 3016 @16 |
| L-FUNCTIONS-055 | an arithmetic operator on an integer and a float | 3011 @12 |
| L-FUNCTIONS-056 | a text local given to a parameter of an integer type | 3011 @15 |
| L-FUNCTIONS-057 | a function type local given a text value | 3011 @12 |
| L-FUNCTIONS-058 | a call of a name that no scope declares, as if it were a function value | 3004 @10 |
| L-FUNCTIONS-059 | a default value that calls a function that no scope declares | 3004 @9 |
| L-FUNCTIONS-060 | a write through a default read-only structure parameter (settled rule 2) | 3069 @15 |
| L-FUNCTIONS-061 | a write through a default structure parameter inside a method (settled rule 2) | 3069 @16 |
| L-FUNCTIONS-062 | a default read-only parameter passed on to a write borrow parameter (settled rule 2) | 3069 @19 |
| L-FUNCTIONS-063 | a function type argument whose signature does not match the parameter type | 3053 @20 |
| L-FUNCTIONS-064 | a call of a function type value with too many arguments | 3010 @12 |
| L-FUNCTIONS-065 | a return with no value in a function that has a result | 3022 @9 |
| L-FUNCTIONS-066 | a return with a value in a function that has no result | 3054 @10 |
| L-FUNCTIONS-067 | the result of a function with no result used as a value | 3054 @14 |
| L-FUNCTIONS-068 | a function used before its declaration (the grammar says a name is declared before use) | 3004 @10 |
| L-FUNCTIONS-069 | two parameters with one name | 3003 @9 |
| L-FUNCTIONS-070 | a parameter that shares its name with a local of the body | 3003 @10 |
| L-FUNCTIONS-071 | a call of a value that is not a function | 3011 @11 |
| L-FUNCTIONS-072 | a non-boolean value as the result of a boolean function | 3011 @10 |
| L-FUNCTIONS-073 | a condition of an integer type in an if | 3043 @10 |

## Types and control flow

`structure`, `enum` with tag and data, `type` aliases, contracts and `implementation` blocks, generics, `if`, `while`, `for`, `switch`, `break` and `continue`, the lifecycle operators, and the parse codes 2001 to 2010. The items sit in `tests/language/<class>/types/`. A parse item can show the sema codes that follow it, because sema runs on the tree that the parser recovers; the `reject` row lists every code in the order given.

### Types and control flow: accept

| Id | Shows | Expected |
|---|---|---|
| L-TYPES-001 | a structure and a field read | ok |
| L-TYPES-002 | a struct literal with every field | ok |
| L-TYPES-003 | a nested struct literal and a nested field read | ok |
| L-TYPES-004 | a field assignment of a local structure | ok |
| L-TYPES-005 | a structure that holds an enum field | ok |
| L-TYPES-006 | a type alias of a primitive and of a structure | ok |
| L-TYPES-007 | a structure with a layout attribute | ok |
| L-TYPES-008 | an opaque structure of a foreign library | ok |
| L-TYPES-009 | a structure with no field | ok |
| L-TYPES-010 | a structure that holds another structure by value, declared before it | ok |
| L-TYPES-011 | a generic structure with two type parameters | ok |
| L-TYPES-012 | a constant of a structure type | ok |
| L-TYPES-032 | an enum, a variant value and a comparison | ok |
| L-TYPES-033 | an enum with a tag type | ok |
| L-TYPES-034 | an enum with a tag type of each integer width | ok |
| L-TYPES-035 | an enum with data in a variant | ok |
| L-TYPES-036 | a switch over an enum that names every variant | ok |
| L-TYPES-037 | a switch over an enum with a default and a missing variant | ok |
| L-TYPES-038 | one case that lists many variants | ok |
| L-TYPES-039 | a function in an implementation block of an enum | ok |
| L-TYPES-040 | a constant of an implementation block of an enum | ok |
| L-TYPES-041 | an enum local that two variants assign | ok |
| L-TYPES-042 | an enum passed by value and compared with not equal | ok |
| L-TYPES-062 | a switch over an integer with a default and many values | ok |
| L-TYPES-064 | a switch over a character | ok |
| L-TYPES-065 | a switch with only a default arm | ok |
| L-TYPES-066 | a switch whose arms assign and do not return | ok |
| L-TYPES-067 | a switch inside a loop that uses break in an arm | ok |
| L-TYPES-075 | a contract, an implementation that meets it and a generic call that needs it | ok |
| L-TYPES-076 | two implementation blocks of one type with different functions | ok |
| L-TYPES-077 | a static function of an implementation, called through the type | ok |
| L-TYPES-078 | a method call through a local | ok |
| L-TYPES-079 | a contract with two functions | ok |
| L-TYPES-080 | a contract function with no result | ok |
| L-TYPES-081 | an operator function in an implementation | ok |
| L-TYPES-082 | a construct function with defaults on every parameter | ok |
| L-TYPES-083 | a destroy function with no parameter and no result | ok |
| L-TYPES-104 | a generic function called with the type inferred | ok |
| L-TYPES-105 | a generic function called with the type written | ok |
| L-TYPES-106 | a generic function with two type parameters | ok |
| L-TYPES-107 | a generic structure with a value parameter | ok |
| L-TYPES-108 | a generic implementation of a generic structure | ok |
| L-TYPES-109 | a generic function with two contracts on one parameter | ok |
| L-TYPES-110 | a generic function with a where clause for two parameters | ok |
| L-TYPES-111 | a generic function that stores and returns its argument with no contract | ok |
| L-TYPES-112 | a generic type used as a parameter type | ok |
| L-TYPES-113 | a type alias of a generic type | ok |
| L-TYPES-126 | an if, elseif and else chain that assigns | ok |
| L-TYPES-127 | a while loop with a counter | ok |
| L-TYPES-128 | a for loop over a range that includes both ends | ok |
| L-TYPES-129 | a for each loop over an array with break and continue | ok |
| L-TYPES-130 | a break and a continue in a while loop | ok |
| L-TYPES-131 | a break in the inner of two nested loops | ok |
| L-TYPES-132 | a for loop with an empty body | ok |
| L-TYPES-133 | a while loop with a boolean name as condition | ok |
| L-TYPES-134 | an if with a comparison of two logical operands | ok |
| L-TYPES-135 | a scope block that ends its locals | ok |
| L-TYPES-136 | the loop variable of a for range used in the body | ok |
| L-TYPES-202 | a word that looks like a keyword but is not one | ok |
| L-TYPES-203 | a name that starts with a primitive type name | ok |
| L-TYPES-204 | A do-while loop whose condition reads a name of the outer scope | ok |
| L-TYPES-205 | A do-while loop with a break and a continue in its body | ok |

### Types and control flow: reject

| Id | Shows | Expected |
|---|---|---|
| L-TYPES-013 | a field of a type that does not exist | 3009 @10 |
| L-TYPES-014 | a structure declared twice | 3003 @13 |
| L-TYPES-015 | a structure and a constant with one name | 3003 @13 |
| L-TYPES-016 | a type alias that clashes with a structure | 3003 @14 |
| L-TYPES-017 | a generic structure used with too few type arguments | 3014 @14 |
| L-TYPES-018 | a generic structure used with too many type arguments | 3014 @13 |
| L-TYPES-019 | a local of an undeclared structure | 3009 @10, 3009 @10 |
| L-TYPES-020 | a field read assigned to a local of another type | 3011 @15 |
| L-TYPES-021 | a struct literal field of another type than the field | 3011 @15 |
| L-TYPES-022 | a nested field of a type that does not exist, after a valid field | 3009 @16 |
| L-TYPES-023 | a struct literal with a missing field | 3077 @15 |
| L-TYPES-024 | a struct literal with a field that the structure does not have | 3044 @15 |
| L-TYPES-025 | a struct literal that gives one field twice | 3012 @15 |
| L-TYPES-026 | a field read of a field that the structure does not have | 3044 @15 |
| L-TYPES-027 | a field assignment of a field that the structure does not have | 3044 @15 |
| L-TYPES-028 | a structure with two fields of one name | 3012 @11 |
| L-TYPES-029 | a structure that holds itself by value | 3045 @10 |
| L-TYPES-030 | two structures that hold each other by value | 3045 @10, 3045 @14 |
| L-TYPES-031 | a structure with a generic parameter that is declared twice | 3003 @9 |
| L-TYPES-043 | an enum tag of a float type | 3031 @9 |
| L-TYPES-044 | an enum tag of the string type | 3031 @9 |
| L-TYPES-045 | an enum tag of the boolean type | 3031 @9 |
| L-TYPES-046 | an enum tag that is a structure | 3031 @14 |
| L-TYPES-047 | an enum tag that is an alias of an integer | 3031 @11 |
| L-TYPES-048 | an enum tag that is not a type | 3031 @9, 3009 @9 |
| L-TYPES-049 | an enum declared twice | 3003 @15 |
| L-TYPES-050 | a switch over an enum that misses a variant | 3021 @16 |
| L-TYPES-051 | a switch over an enum with no arm | 3021 @16 |
| L-TYPES-052 | a switch that lists a variant twice in two arms | 3025 @20 |
| L-TYPES-053 | a switch with two default arms | 2010 @14, 3025 @14 |
| L-TYPES-054 | a switch with a value twice in one case list | 3025 @11 |
| L-TYPES-055 | a switch with one integer value in two arms | 3025 @14 |
| L-TYPES-056 | a switch where a hex and a decimal literal are one value | 3025 @14 |
| L-TYPES-057 | a switch where a constant and its literal are one value | 3025 @16 |
| L-TYPES-058 | a switch with a repeated value and a missing variant gives both codes | 3025 @20, 3021 @16 |
| L-TYPES-060 | a switch value of a name that no scope declares | 3004 @10 |
| L-TYPES-061 | a switch arm that returns the wrong type | 3011 @12 |
| L-TYPES-072 | a zero-member enum | 3047 @9 |
| L-TYPES-073 | an enum with one variant declared twice | 3012 @11 |
| L-TYPES-074 | a variant of an enum that does not exist | 3048 @16 |
| L-TYPES-084 | two implementation blocks that declare one function | 3012 @21 |
| L-TYPES-085 | one implementation block that declares one function twice | 3012 @19 |
| L-TYPES-086 | a generic call with a type that does not meet the contract | 3013 @24, 3013 @24 |
| L-TYPES-087 | a type that has the contract function with another signature | 3013 @30, 3013 @30 |
| L-TYPES-088 | a where clause that names a contract that does not exist | 3004 @9 |
| L-TYPES-089 | a where clause that names a name that is not a generic parameter | 3004 @13 |
| L-TYPES-090 | a contract declared twice | 3003 @13 |
| L-TYPES-091 | a construct function that is called directly | 3027 @22 |
| L-TYPES-092 | a destroy function that is called directly | 3027 @22 |
| L-TYPES-093 | a construct function at the top level | 3085 @9, 3028 @10 |
| L-TYPES-094 | a destroy function with a parameter | 3029 @16 |
| L-TYPES-095 | a construct function with a result | 3029 @16 |
| L-TYPES-096 | a construct function with a parameter that has no default | 3029 @16 |
| L-TYPES-097 | a type call with too many arguments for a type that has no construct | 3010 @15 |
| L-TYPES-098 | an implementation of a type that does not exist | 3009 @9, 3009 @10 |
| L-TYPES-099 | a contract that lists one function twice | 3012 @11 |
| L-TYPES-100 | a method call of a function that the type does not have | 3049 @15 |
| L-TYPES-101 | a static call of a function that the type does not have | 3049 @15 |
| L-TYPES-102 | an operator function whose signature does not match the operator | 3050 @16 |
| L-TYPES-103 | an implementation constant used outside its block | 3004 @19 |
| L-TYPES-114 | a generic argument count that does not match | 3014 @13 |
| L-TYPES-115 | a generic type with no argument where one is needed | 3014 @13 |
| L-TYPES-116 | a number given for a type parameter | 3014 @13 |
| L-TYPES-117 | a type nested 65 levels deep | 3015 @13 |
| L-TYPES-118 | a generic function with a bound that the argument type does not meet | 3013 @24, 3013 @24 |
| L-TYPES-119 | a generic parameter named as another declaration | 3003 @13 |
| L-TYPES-120 | a generic call with a wrong argument count | 3010 @14 |
| L-TYPES-121 | an unsatisfied bound inside the body of a generic function, an operation the bound does not give | 3051 @10 |
| L-TYPES-122 | two generic parameters with one name in a function | 3003 @9 |
| L-TYPES-123 | two overlapping implementation blocks of a generic and a concrete type | 3052 @20 |
| L-TYPES-124 | a cyclic type alias | 3046 @9, 3046 @10 |
| L-TYPES-125 | a type alias of itself | 3046 @9 |
| L-TYPES-138 | a break outside a loop | 3026 @10 |
| L-TYPES-139 | a continue outside a loop | 3026 @10 |
| L-TYPES-140 | a break in an if that is not in a loop | 3026 @11 |
| L-TYPES-141 | a break in a switch arm that is not in a loop | 3026 @12 |
| L-TYPES-142 | a break after the loop ended | 3026 @13 |
| L-TYPES-143 | a continue in the else branch of a function with no loop | 3026 @14 |
| L-TYPES-144 | a loop variable used after its loop | 3004 @13 |
| L-TYPES-145 | a name used after its scope block ended | 3004 @13 |
| L-TYPES-146 | a for range with a bound that no scope declares | 3004 @10 |
| L-TYPES-148 | a for each over a name that no scope declares | 3004 @10 |
| L-TYPES-149 | a while condition that names an undeclared name | 3004 @10 |
| L-TYPES-150 | an if condition that names an undeclared name | 3004 @10 |
| L-TYPES-151 | an elseif condition that names an undeclared name | 3004 @13 |
| L-TYPES-152 | a while condition of an integer type | 3043 @10 |
| L-TYPES-153 | an if condition of a text type | 3043 @10 |
| L-TYPES-156 | a local with no semicolon | 2001 @11 |
| L-TYPES-157 | an if with no then | 2001 @11 |
| L-TYPES-158 | a while with no do | 2001 @11 |
| L-TYPES-159 | a call with no closing parenthesis | 2001 @14 |
| L-TYPES-160 | a function with no parameter list | 2001 @9 |
| L-TYPES-161 | a local with no colon before its type | 2001 @10 |
| L-TYPES-162 | a structure field with no semicolon | 2001 @11 |
| L-TYPES-163 | a return with no semicolon | 2001 @11 |
| L-TYPES-164 | a local initialized with nothing | 2002 @10 |
| L-TYPES-165 | a binary operator with no right side | 2002 @10 |
| L-TYPES-166 | a call with a trailing comma | 2002 @14 |
| L-TYPES-167 | a local with no type | 2003 @10 |
| L-TYPES-168 | a parameter with no type | 2003 @9 |
| L-TYPES-169 | a function with a result mark and no type | 2003 @10 |
| L-TYPES-170 | a stray number at the top level | 2004 @9 |
| L-TYPES-171 | a stray word at the top level | 2004 @9, 2004 @9 |
| L-TYPES-172 | a statement keyword used as a top level item | 2004 @9, 2004 @9 |
| L-TYPES-173 | an extra end after a function | 2005 @12 |
| L-TYPES-174 | an end before the else, the old form that closed each branch | 2001 @13, 2004 @14, 2004 @14, 2005 @15, 2005 @16 |
| L-TYPES-175 | a case arm outside a switch | 2001 @10, 2004 @10, 2004 @11, 2004 @11, 2005 @12, 2005 @13 |
| L-TYPES-176 | a function type parameter with a default value | 2005 @9 |
| L-TYPES-177 | a chained comparison | 2006 @10, 3011 @10 |
| L-TYPES-178 | a chained equality | 2006 @10, 3011 @10 |
| L-TYPES-179 | a mixed chained comparison | 2006 @10, 3011 @10 |
| L-TYPES-180 | a file with no namespace | 2007 @7 |
| L-TYPES-181 | a file that starts with a constant before the namespace | 2007 @7, 2004 @8 |
| L-TYPES-182 | an unclosed function | 2009 @11 |
| L-TYPES-183 | an unclosed if | 2009 @13 |
| L-TYPES-184 | an unclosed structure | 2009 @11 |
| L-TYPES-185 | an unclosed switch | 2009 @14 |
| L-TYPES-186 | an unclosed while | 2009 @12 |
| L-TYPES-187 | a default arm that is not last | 2010 @14 |
| L-TYPES-188 | an expression nested 255 parentheses deep | 2008 @10, 2001 @10 |
| L-TYPES-189 | a block nested 257 levels deep | 2008 @266, 2005 @525 |
| L-TYPES-190 | two independent errors give two diagnostics and no cascade | 2002 @10, 2001 @16 |
| L-TYPES-191 | a missing semicolon gives one diagnostic and the next statement still parses | 2001 @11 |
| L-TYPES-192 | a statement keyword used as a local name | 2001 @10, 2002 @10, 2009 @12 |
| L-TYPES-193 | a declaration keyword used as a function name | 2001 @9, 2001 @9, 2001 @10, 2001 @10 |
| L-TYPES-194 | a block keyword used as a parameter name | 2001 @9, 2004 @9, 2004 @10, 2004 @10, 2005 @11 |
| L-TYPES-195 | a logical keyword used as a local name | 2001 @10, 2002 @10 |
| L-TYPES-196 | a boolean literal used as a local name | 2001 @10, 2001 @10 |
| L-TYPES-197 | an ownership keyword used as a variable name | 2001 @10, 2002 @10 |
| L-TYPES-198 | a loop keyword used as a structure name | 2001 @9 |
| L-TYPES-199 | a type declaration keyword used as an enum name | 2001 @9 |
| L-TYPES-200 | a keyword used as a field name | 2001 @10, 2001 @10 |
| L-TYPES-201 | a keyword used as an enum variant name | 2001 @10, 2001 @10 |
| L-TYPES-206 | A do-while loop whose condition is an integer | 3043 @11 |
| L-TYPES-207 | A do-while loop whose condition reads a name of its body | 3004 @12 |
| L-TYPES-208 | a slice value for a primitive local | 3011 @10 |
| L-TYPES-209 | a field of a structure type given to a primitive parameter | 3011 @18 |
| L-TYPES-210 | a field of a function type with another signature than the parameter | 3053 @15 |
| L-TYPES-211 | an owned vector given to a vector parameter | 3014 @13 |
| L-TYPES-212 | the result of a generic call compared after its type argument is bound | 3011 @13 |
| L-TYPES-213 | the line of a call stored in a string | 3011 @8 |
| L-TYPES-214 | the bytes of a file stored in a string | 3011 @8 |
| L-TYPES-215 | arithmetic on two strings | 3011 @10 |
| L-TYPES-216 | a comparison of two booleans | 3011 @10 |
| L-TYPES-217 | a bitwise operator on two characters | 3038 @10 |
| L-TYPES-218 | a declared operator with an operand of another type | 3011 @21 |
| L-TYPES-219 | a condition that is a field of another type | 3043 @14 |
| L-TYPES-220 | a false static assertion on the size of a structure | 3059 @14 |
| L-TYPES-070 | a switch arm whose value has another type than the switch value | 3011 @11 |
| L-TYPES-071 | a case value of a variant of another enum | 3011 @21 |
| L-TYPES-147 | a for range with a bound of another type | 3011 @10 |

### Types and control flow: known_gap

| Id | Shows | Expected |
|---|---|---|

## Namespaces and imports

Namespaces, `main`, imports and the resolver outcomes, the reserved first segments `c` and `marea`, and
visibility. The items sit in `tests/language/<class>/namespaces/`. Every item is one file named `main.marea` for the
runner, so its namespace is `main`; a namespace of another name gives 3002 (L-NAMESPACES-024 to 027). The runner
registers the library types the items write (`demo.array`, `demo.vector`, `demo.slice`, `demo.pointer`, `demo.owned`, `demo.task`) with
their roles and ownership class, because sema knows no library by name, and answers `EXTERNAL` to a `c.` name, to `demo` and every `demo.` name (the neutral root of the fixtures), and to the
three names `marea.array`, `marea.memory` and `marea.tasks`. It answers `NOT_FOUND` to every other name, so
`marea.text`, a project namespace and the namespace itself give 3001 (L-NAMESPACES-020 to 023, 038, 039, 042, 057).
An import binds no name: the use writes the full path, and a short name exists only through `as`
(L-NAMESPACES-007 to 014, 065 to 067, 070 to 076). An import or a require that no resolver finds gives 3001 or 3079. `main` is an ordinary function: it has no
fixed signature and none is required (L-NAMESPACES-002 to 006).

Settled rule 5 (a `private` name used from another namespace is a reject) is enforced by sema (3066, 3067 and 3068) and
proven by `tests/source/sema/MAREASemaVisibility.cpp`, but it has no item. The runner checks one file per
item and an item cannot import a second file, so a private name cannot be reached from another namespace until issue
#250 lands. A private
name used inside its own namespace is accepted (L-NAMESPACES-011, 012, 063, 064). An import cycle and a private enum
import need the same second file, so they have no item either; a namespace that imports itself gives 3001
(L-NAMESPACES-039).

### Namespaces and imports: accept

| Id | Shows | Expected |
|---|---|---|
| L-NAMESPACES-001 | A namespace that matches the file path, with one function | ok |
| L-NAMESPACES-002 | main is an ordinary function with no parameter and an integer result | ok |
| L-NAMESPACES-003 | main has no fixed signature: one number parameter and a float result | ok |
| L-NAMESPACES-004 | main may have no result | ok |
| L-NAMESPACES-005 | A file with no main at all | ok |
| L-NAMESPACES-006 | main takes the argument array of the future driver | ok |
| L-NAMESPACES-007 | A required C library is used through the alias of its last segment | ok |
| L-NAMESPACES-008 | A required C library renamed by as | ok |
| L-NAMESPACES-009 | One symbol of a c library renamed by as | ok |
| L-NAMESPACES-010 | A require list with two members, one of them renamed | ok |
| L-NAMESPACES-011 | A public and a private function, the private one used inside the namespace | ok |
| L-NAMESPACES-012 | A private constant and a private structure used inside the namespace | ok |
| L-NAMESPACES-013 | Two requires with different local names | ok |
| L-NAMESPACES-014 | The same C library required twice under two aliases | ok |
| L-NAMESPACES-015 | A file attribute after the namespace and a public function | ok |
| L-NAMESPACES-063 | A private field of a public structure read inside its own namespace | ok |
| L-NAMESPACES-064 | A private enum used inside its own namespace | ok |
| L-NAMESPACES-065 | A call written with the full path of a required namespace | ok |
| L-NAMESPACES-066 | A wildcard require covers the full path of every namespace below it | ok |
| L-NAMESPACES-067 | A type of a required library written with its full path | ok |

### Namespaces and imports: reject

| Id | Shows | Expected |
|---|---|---|
| L-NAMESPACES-020 | An import of a namespace that no resolver knows | 3001 @9 |
| L-NAMESPACES-021 | A marea import with no library behind it | 3001 @9 |
| L-NAMESPACES-022 | An import of the bare first segment marea | 3001 @9 |
| L-NAMESPACES-023 | An import of the bare first segment c has no library | 3001 @9 |
| L-NAMESPACES-024 | A namespace that does not match the file path | 3002 @7 |
| L-NAMESPACES-025 | A namespace with an extra segment does not match the file path | 3002 @7 |
| L-NAMESPACES-026 | A file cannot declare a namespace under the reserved first segment c | 3002 @7 |
| L-NAMESPACES-027 | A file cannot declare a namespace under the reserved first segment marea | 3002 @7, 3081 @7 |
| L-NAMESPACES-028 | The same require twice gives a name that is already declared | 3003 @10 |
| L-NAMESPACES-029 | Two requires end in the same local name | 3003 @10 |
| L-NAMESPACES-030 | An alias clashes with an earlier require | 3003 @10 |
| L-NAMESPACES-031 | A require name clashes with a function of the namespace | 3003 @11 |
| L-NAMESPACES-032 | An alias hides the reserved namespace c | 3003 @9 |
| L-NAMESPACES-033 | A type alias named c hides the reserved namespace c | 3003 @9 |
| L-NAMESPACES-034 | Two functions with the same name | 3003 @13 |
| L-NAMESPACES-035 | A function and a constant with the same name | 3003 @11 |
| L-NAMESPACES-036 | A namespace used with no import | 3004 @10 |
| L-NAMESPACES-037 | The original name of an alias used as a namespace | 3004 @12 |
| L-NAMESPACES-038 | A name of the namespace of a missing import | 3001 @9 |
| L-NAMESPACES-039 | A namespace cannot import itself | 3001 @9 |
| L-NAMESPACES-040 | An unknown attribute key | 3005 @9 |
| L-NAMESPACES-041 | A visibility value that is not public or private | 3006 @9 |
| L-NAMESPACES-042 | An import list of a namespace that no resolver knows | 3001 @9 |
| L-NAMESPACES-050 | A call of a function that no scope declares | 3004 @10 |
| L-NAMESPACES-051 | A user function named like a built-in macro | 3056 @9 |
| L-NAMESPACES-052 | A local named like a built-in macro | 3056 @10 |
| L-NAMESPACES-053 | A require alias that shadows a primitive type | 3003 @9 |
| L-NAMESPACES-054 | A require alias named like a built-in macro | 3056 @9, 3004 @12 |
| L-NAMESPACES-055 | A require alias clashes with a function of the namespace | 3003 @11 |
| L-NAMESPACES-056 | Two visibility values on one function conflict | 3007 @9 |
| L-NAMESPACES-057 | A marea import with a member list and no library behind it | 3001 @9 |
| L-NAMESPACES-059 | The same member twice in one require list | 3003 @9 |
| L-NAMESPACES-060 | A second namespace declaration in one file | 2004 @9 |
| L-NAMESPACES-061 | The namespace declaration is not the first statement | 2007 @7, 2004 @11 |
| L-NAMESPACES-070 | A call of a namespace that no import or require covers | 3078 @10 |
| L-NAMESPACES-071 | A type of another namespace that no import or require covers | 3078 @9 |
| L-NAMESPACES-072 | The last segment of a required path binds no name | 3004 @12 |
| L-NAMESPACES-073 | A require of a name that nothing registered | 3079 @9 |
| L-NAMESPACES-074 | A wildcard require of a namespace that nothing registered | 3079 @9 |
| L-NAMESPACES-075 | One full path covered by an import and by a require | 3080 @10 |
| L-NAMESPACES-076 | A wildcard require binds no short name | 3004 @12 |
| L-NAMESPACES-077 | A file cannot declare a namespace under marea when the host did not register it | 3002 @7, 3081 @7 |
| L-NAMESPACES-078 | a loop name that is a built-in macro | 3056 @10 |

### Namespaces and imports: known_gap

| Id | Shows | Expected |
|---|---|---|

## Macros and the C border

Interop with C, opaque types, owners and raw pointers of a C border, and the built-in macros. The items sit in
`tests/language/<class>/interop/`. A `c.` import is `EXTERNAL` in the runner, so the C call itself has no signature
and its argument and result types are not checked. A macro name without `!` is an undeclared name (L-INTEROP-054) and
a user-written `macro` is a parse error (L-INTEROP-020). A call of a macro that the compiler does not define is 3060
(L-INTEROP-021), a call with the wrong arguments is 3061 (L-INTEROP-045, 046, 047, 052, 053, 055, 062), and a macro
result used as a value of another type is 3011 (L-INTEROP-056, 061); the macro table is in
[marea-sema.md](../sema/marea-sema.md). The two compile-time macros are the exception to the argument check:
`compile_error!` always gives 3059 (L-INTEROP-044) and a `static_assert!` over a constant false condition gives 3059
(L-INTEROP-043); a condition the compiler cannot fold is accepted. The type of each value that a `{}` takes is not
checked. The type `handle<T>` is rejected with 3009 by the type checker, so it has no accept item.

### Macros and the C border: accept

| Id | Shows | Expected |
|---|---|---|
| L-INTEROP-001 | A function of a c library called with float arguments | ok |
| L-INTEROP-002 | An opaque structure declared with no body | ok |
| L-INTEROP-003 | An opaque type used through a borrow parameter | ok |
| L-INTEROP-004 | An opaque type used through a read-only borrow parameter | ok |
| L-INTEROP-005 | An owner of an opaque type comes from a c call and goes up with return | ok |
| L-INTEROP-006 | An owner of an opaque type held in a local and passed as a borrow | ok |
| L-INTEROP-007 | A structure with the exact layout of a C structure | ok |
| L-INTEROP-008 | A raw pointer from a c call inside an unsafe block | ok |
| L-INTEROP-009 | A mapped buffer is a name that lives in a with block | ok |
| L-INTEROP-011 | The print macro with one value | ok |
| L-INTEROP-012 | The log macro with a level and one value | ok |
| L-INTEROP-013 | The format macro builds a string | ok |
| L-INTEROP-014 | The assert and debug_assert macros with a condition and a message | ok |
| L-INTEROP-015 | A static_assert macro of a true constant condition | ok |
| L-INTEROP-016 | The unreachable macro after a branch that cannot run | ok |
| L-INTEROP-017 | The place macros give the line, the file and the namespace | ok |
| L-INTEROP-060 | An unsafe block with a plain local | ok |
| L-INTEROP-018 | A build_value macro takes its default, and a static_assert reads it | ok |

### Macros and the C border: reject

| Id | Shows | Expected |
|---|---|---|
| L-INTEROP-020 | A user cannot declare a macro | 2004 @9, 2004 @10, 2004 @10, 2005 @11 |
| L-INTEROP-021 | A call of a macro that the compiler does not define | 3060 @10 |
| L-INTEROP-022 | An opaque structure has no body | 2001 @10, 2005 @11 |
| L-INTEROP-023 | A parameter of a type that no declaration names | 3009 @9 |
| L-INTEROP-024 | A require placed after a function | 2004 @13 |
| L-INTEROP-040 | An opaque structure is built with a struct literal | 3057 @12 |
| L-INTEROP-041 | A field of an opaque structure is read | 3057 @12 |
| L-INTEROP-042 | An operator is applied to two opaque values | 3057 @12 |
| L-INTEROP-043 | A static_assert macro of a false condition | 3059 @9 |
| L-INTEROP-044 | A compile_error macro call | 3059 @9 |
| L-INTEROP-045 | A print macro with fewer values than the format text needs | 3061 @10 |
| L-INTEROP-046 | A print macro with a number as the format text | 3061 @10 |
| L-INTEROP-047 | An assert macro with a number as the condition | 3061 @10 |
| L-INTEROP-048 | A raw pointer local outside an unsafe block | 3041 @12 |
| L-INTEROP-049 | A name of a with block used after its end | 3004 @15 |
| L-INTEROP-050 | An owner of an opaque type assigned with no move | 3016 @15 |
| L-INTEROP-051 | An owner of an opaque type is used after it moved | 3070 @16 |
| L-INTEROP-052 | A print macro with no argument | 3061 @10 |
| L-INTEROP-053 | An unreachable macro with an argument | 3061 @10 |
| L-INTEROP-054 | A macro name called without the bang | 3004 @10 |
| L-INTEROP-055 | A log macro with a level that does not exist | 3061 @10 |
| L-INTEROP-056 | A macro that gives no value used as a value | 3011 @10 |
| L-INTEROP-057 | A structure with an opaque field held by value | 3058 @12 |
| L-INTEROP-058 | An opaque structure declared twice | 3003 @11 |
| L-INTEROP-059 | An opaque structure and a structure with the same name | 3003 @11 |
| L-INTEROP-061 | A format macro result returned as an integer | 3011 @10 |
| L-INTEROP-062 | An include_text macro with a number as the path | 3061 @9 |
| L-INTEROP-063 | A borrowed opaque value returned by value | 3018 @12 |
| L-INTEROP-064 | A build_value macro with no default | 3061 @9 |
| L-INTEROP-065 | A build_value macro with a default of another type | 3011 @9 |
| L-INTEROP-066 | A build_value macro whose type is not the declared type | 3011 @9 |
| L-INTEROP-067 | A build_value macro over an enum with a field that has no default | 3061 @14 |
| L-INTEROP-068 | A qualified macro call of a name that its library does not register | 3060 @12 |
| L-INTEROP-069 | A qualified macro call of a library that no require covers | 3078 @10 |
| L-INTEROP-070 | A qualified macro call of a file that the include resolver does not know | 3061 @12 |
| L-INTEROP-071 | a print macro with a structure for a placeholder | 3061 @14 |

### Macros and the C border: known_gap

| Id | Shows | Expected |
|---|---|---|

## Attributes

The attribute keys, the placement matrix of the attribute map, the `when` item and the `backend` key. The items sit in
`tests/language/<class>/attributes/`. Every row of the placement matrix has a `reject` item (L-ATTRIBUTES-010 to
023, code 3085). The catalogue runner gives sema no target description, no build settings and no include resolver, so
every `when` predicate is false and `BACKEND_UNAVAILABLE` (3083) and `INVALID_BUILD_VALUE` (3086) have no item (see
Gaps); `tests/source/sema/MAREASemaBackendWhen.cpp` and `tests/source/sema/MAREASemaBuildValue.cpp` prove them.

### Attributes: accept

| Id | Shows | Expected |
|---|---|---|
| L-ATTRIBUTES-001 | A when item on a function, with no target given so the predicate is false | ok |
| L-ATTRIBUTES-002 | A plain key and a when item on the same key | ok |
| L-ATTRIBUTES-003 | Every key at a place that the placement matrix gives it | ok |

### Attributes: reject

| Id | Shows | Expected |
|---|---|---|
| L-ATTRIBUTES-010 | An align key on the file | 3085 @9 |
| L-ATTRIBUTES-011 | An align key on a top-level function | 3085 @9 |
| L-ATTRIBUTES-012 | An inline key on a top-level constant | 3085 @9 |
| L-ATTRIBUTES-013 | An inline key on a structure | 3085 @9 |
| L-ATTRIBUTES-014 | A layout key on an opaque structure | 3085 @9 |
| L-ATTRIBUTES-015 | An export key on an enum | 3085 @9 |
| L-ATTRIBUTES-016 | A build key on a type alias | 3085 @9 |
| L-ATTRIBUTES-017 | An align key on a contract | 3085 @9 |
| L-ATTRIBUTES-018 | An export key on an implementation block | 3085 @13 |
| L-ATTRIBUTES-019 | A pack key on a field of a structure | 3085 @10 |
| L-ATTRIBUTES-020 | A layout key on a function of an implementation | 3085 @14 |
| L-ATTRIBUTES-021 | An inline key on a constant of an implementation | 3085 @14 |
| L-ATTRIBUTES-022 | A visibility key on a parameter | 3085 @13 |
| L-ATTRIBUTES-023 | A visibility key on a macro statement | 3085 @9 |
| L-ATTRIBUTES-024 | A backend key in a file that is not marea.math | 3084 @9 |
| L-ATTRIBUTES-025 | Two when items that hold give the same key | 3007 @9 |
| L-ATTRIBUTES-026 | A when item with a predicate name that does not exist | 3006 @9 |
| L-ATTRIBUTES-027 | An export key on a generic function | 3085 @9 |
| L-ATTRIBUTES-028 | An export label that is not a C identifier | 3006 @9 |
| L-ATTRIBUTES-029 | A key at a wrong place inside a when item | 3085 @9 |

## Parallel code

`fork_join` blocks, `partition` and `partition mutable`, `spawn` and its argument rule (3020), jumps out of a parallel
body (3024), two workers or a worker and the block that use one value when one writes it (3074), a partition body that writes an outer name (3075) or uses the source of its partition mutable (3076), the `task` result read after the join, and the races that compile today. The items sit in
`tests/language/<class>/parallel/`. Ids are `L-PARALLEL-nnn`; the numbers are not continuous, because a gap in the
numbers groups the kinds (accept 001-020 and 079, 3020 rejects 030-039, 3024 rejects 040-051, 3074 rejects 060-063 and 076, 3075 and 3076 rejects 065-067 and 069).

What sema enforces today is small, and it sits in `lib/source/marea/sema/MAREASemaParallel.c`. 3020: in a `fork_join`,
each argument of a `spawn` is a part, a literal, a file constant, a name that is not a write-borrow parameter and not
the source of a `partition mutable`, or a field of such a name; any other expression (a call, an index, a computed
value, `copy x`, `move x`) is rejected. 3024: a `return`, `break` or `continue` that leaves a `partition` body or stands
directly in a `fork_join` block. 3062: a `spawn` or a `partition` with no enclosing `fork_join` block in the same
function (L-MEMORY-030, L-PARALLEL-064, L-PARALLEL-070). 3063: the operand of a `spawn` is not a call, a macro call included
(L-PARALLEL-071). 3064: the source of a `partition` is known not to be an array, a number, a primitive, a structure or an
enum (L-PARALLEL-072). 3074: a table of what the workers of one `fork_join` block took (`MAREASemaCapture.c`) is kept
from the block start to its end, and a use that overlaps an earlier one, where one of the two writes, is rejected: two
workers on one value, a worker and a later worker or `partition`, a spawn in a loop or a `partition` body that writes a
value declared before the block, and a statement of the block that follows a worker and uses what it writes (or writes
what it reads). A value and a field of it overlap, two fields do not. A nested block is compared with its outer block
(L-PARALLEL-060 to 063 and 076, L-MEMORY-026 and 028, L-ADVERSARIAL-008, 024 and 025, issue #210). 3075: an assignment in a
`partition` body whose target (a name, a field or an element, by its root name) is a local or a parameter declared outside the
body; a name declared in the body (a local, a loop variable, a `with` name, a nested part) and the part itself are the
body's own (L-PARALLEL-065 to 068). 3076: in a `partition mutable`, any use of the name of its source in the body,
except as an argument of a `spawn`, which 3020 owns (L-PARALLEL-069, L-ADVERSARIAL-023). A call that passes an outer local
to a function that writes it is not seen. Nothing else about
parallel code is checked: there is no full data-race analysis. No `known_gap` item of the parallel area is left.

Facts the items fix, checked by running the compiler:

- A `fork_join` in a loop, two blocks in a row, and a block inside a block are accepted. A jump in a nested block is
  rejected as in the outer one. A loop outside the whole block never catches a `break` or `continue` (item 049, 051).
- A `move x` or `copy x` argument of a `spawn` is rejected with 3020, because the argument rule lists no other form
  (items 036, 037). The rule says nothing of ownership, so this is the "simple first rule" of the sema doc, not a
  decided ownership rule.
- A part is a name of the body only: its use after the `end` of the body is the undeclared name 3004 (item 080).
- A parameter with no attribute is a read-only borrow (settled rule 2), so `spawn` accepts it as a name that is not a
  write-borrow parameter. `examples/parallel/scaling.marea` writes `#[semantics = reference_mutable]` on the two
  parameters it writes (`scale_slice`, `scale_all`) and requires `marea.array`, `c.memory` and `c.threads`.

### Parallel code: accept

| Id | Shows | Expected |
|---|---|---|
| L-PARALLEL-001 | A spawn of a function on a local, read only | ok |
| L-PARALLEL-002 | A spawn with literal arguments | ok |
| L-PARALLEL-003 | A spawn with a file constant as an argument | ok |
| L-PARALLEL-004 | Two spawns on two different locals, each with a write borrow | ok |
| L-PARALLEL-005 | A partition that reads and spawns a worker on each part | ok |
| L-PARALLEL-006 | A partition mutable of a write-borrow parameter | ok |
| L-PARALLEL-008 | A partition nested in a partition body | ok |
| L-PARALLEL-009 | A break that leaves a loop inside a partition body | ok |
| L-PARALLEL-010 | A continue in a loop inside a partition body | ok |
| L-PARALLEL-011 | A break in a loop that stands directly in a fork_join block | ok |
| L-PARALLEL-013 | A fork_join block in a loop | ok |
| L-PARALLEL-014 | Two fork_join blocks one after the other | ok |
| L-PARALLEL-015 | A field of a local as the argument of a spawn | ok |
| L-PARALLEL-016 | A spawn of a read-only parameter | ok |
| L-PARALLEL-017 | A part and a literal as the arguments of a spawn | ok |
| L-PARALLEL-018 | A nested fork_join inside a fork_join | ok |
| L-PARALLEL-019 | A part is read in the partition body with a for each | ok |
| L-PARALLEL-020 | A break in a loop that stands in an if inside a partition body | ok |
| L-PARALLEL-079 | A local is written after the join | ok |

### Parallel code: reject

| Id | Shows | Expected |
|---|---|---|
| L-PARALLEL-030 | A write-borrow parameter is spawned | 3020 @53 |
| L-PARALLEL-031 | A write-borrow parameter is the second argument of a spawn | 3020 @58 |
| L-PARALLEL-032 | A call is the argument of a spawn | 3020 @53 |
| L-PARALLEL-033 | An element of a vector is the argument of a spawn | 3020 @57 |
| L-PARALLEL-034 | A computed value is the argument of a spawn | 3020 @58 |
| L-PARALLEL-035 | A field of a write-borrow parameter is the argument of a spawn | 3020 @53 |
| L-PARALLEL-036 | A moved value is the argument of a spawn | 3020 @58 |
| L-PARALLEL-037 | A copy is the argument of a spawn | 3020 @54 |
| L-PARALLEL-038 | A write-borrow parameter is spawned in a nested fork_join | 3020 @54 |
| L-PARALLEL-039 | The source of a partition mutable is spawned inside its own partition | 3020 @54 |
| L-PARALLEL-040 | A break leaves the body of a partition | 3024 @54 |
| L-PARALLEL-041 | A continue leaves the body of a partition | 3024 @54 |
| L-PARALLEL-042 | A return directly in a fork_join block | 3024 @53 |
| L-PARALLEL-043 | A break directly in a fork_join block | 3024 @53 |
| L-PARALLEL-044 | A continue directly in a fork_join block | 3024 @53 |
| L-PARALLEL-045 | A return in a loop inside a partition body still leaves the body | 3024 @55 |
| L-PARALLEL-046 | A return in the inner partition of a nested partition | 3024 @55 |
| L-PARALLEL-047 | A break in an if directly in a fork_join block | 3024 @54 |
| L-PARALLEL-048 | A return in an arm of switch inside a partition body | 3024 @56 |
| L-PARALLEL-049 | A loop outside the fork_join block is no target for a break | 3024 @54 |
| L-PARALLEL-050 | A return in a nested fork_join | 3024 @54 |
| L-PARALLEL-051 | A continue in a partition body inside a loop that is outside the block | 3024 @55 |
| L-PARALLEL-060 | One part is passed to two workers that write it | 3074 @55 |
| L-PARALLEL-061 | A worker writes a local and another worker reads it | 3074 @55 |
| L-PARALLEL-062 | A spawn in a loop writes the same local in each turn | 3074 @55 |
| L-PARALLEL-063 | A worker writes the local that the partition reads | 3074 @56 |
| L-PARALLEL-064 | A worker is spawned after the end of a fork_join block | 3062 @56 |
| L-PARALLEL-065 | A partition body writes a local of the function, so the workers race on it | 3075 @55 |
| L-PARALLEL-066 | A partition body adds to a local of the function | 3075 @55 |
| L-PARALLEL-067 | A partition body writes a field of a local of the function | 3075 @55 |
| L-PARALLEL-068 | A part is stored in a local of the function and outlives the body | 3075 @55, 3018 @55 |
| L-PARALLEL-069 | The source of a partition mutable is written in the body while the parts are written | 3076 @54 |
| L-PARALLEL-070 | A partition is used outside of a fork_join block | 3062 @52 |
| L-PARALLEL-071 | A spawn statement that is not a call | 3063 @54 |
| L-PARALLEL-072 | A partition of a value that is no array | 3064 @54 |
| L-PARALLEL-073 | A part of a partition that only reads is written | 3069 @54, 3069 @55 |
| L-PARALLEL-075 | A read-only parameter is spawned into a function that writes it | 3069 @53 |
| L-PARALLEL-076 | A local is written by a worker and by the body of the block | 3074 @55 |
| L-PARALLEL-077 | A part is passed to a function that writes it, with partition and no mutable | 3069 @54 |
| L-PARALLEL-080 | A part is used after the partition body | 3004 @56 |

### Parallel code: known_gap

| Id | Shows | Expected |
|---|---|---|

## Adversarial

Programs that no single area owns: features that meet in one program, and accept items of the other areas with one small
edit. The items sit in `tests/language/<class>/adversarial/` with ids `L-ADVERSARIAL-nnn`. There is no accept item: each
program is a reject (the compiler already refuses it) or a known_gap (the compiler accepts it and a settled rule or a
decided outcome says it must not). Items 001 to 033 are cross-feature programs; items 034 to 072 are mutants, and each
mutant names the accept item it comes from in a `-- from: <id>` line below the copyright header.

A mutant is made by one edit of the program of its source item: delete or swap `copy` and `move`, drop the write
attribute or make it read-only, drop `mutable` from a `for each` or a `partition`, drop an `owned` wrapper, drop a
`return value;` path, drop an initial value, narrow one type. The edits were applied to every accept item of the five
areas by a throw-away script (551 mutants, 185 refused and 366 still accepted); the script is not part of the
repository. Of the still accepted mutants, a mutant that only changes a comment or gives a valid program (a `copy` to
`move` on a value that is not used later, a narrowed type that fits) is not an item. The items are the mutants that the
language must refuse and the compiler accepts, and a sample of the mutants that it refuses, which shows that the edit
changes the result. 39 items are mutants.

Facts the items fix, checked by running the compiler:

- A `return value;` path dropped from a function with a result gives 3022 on every shape tried: plain function, struct
  literal, else branch, method, operator, generic with a bound (items 034 to 039).
- The word that makes an assignment explicit is checked on a name that is a plain value (3016, items 040, 041, 045), on a
  returned borrow (3018, items 043, 044, 046, 047, 048) and on one call that shares a write borrow (3019, item 042). The
  same word swapped from `copy` to `move` is never checked against the source: a `move` of a borrow or a file constant
  is accepted (issue #200, items 013, 050, 052, 056 to 060, 066). A `move` of a `for each` element, of a field or of an
  element of an owner gives 3072 (issue #199, items 015, 051, 053 to 055, 057).
- The use of a value after `move` gives 3070, and a second move gives 3071, through every path tried here: a loop in a
  `fork_join` block, the body of a partition, a generic function, a method call and a second move into a generic
  function, and after a branch where any arm that reaches the end moved the value (issue #198, items 001, 002, 003, 004,
  016, 026, 027, 030, 042, 044). The moved bit joins the arms by OR, so a move in only some arms counts after the
  branch; an arm that ends in `return` or `break` is left out. A partial move (3072) still rounds toward valid: a
  moved part counts after a branch only when every arm that reaches the end moved the same part.
- A read-only borrow that is written, or passed on to a write borrow, gives 3069 through a generic function, a
  `for each` over a read-only vector, a worker and a `partition mutable` of a read-only source (issues #203 and #204,
  items 009, 014, 031, 033, 054, 055, 061 to 065).
- A literal out of range gives 3033 as a `spawn` argument, a default value, a struct literal field and a constant
  (issue #181, items 018 to 021). A local with no initial value gives 3065 also when a worker reads it (item 017).
- The type of a field read through a `for each` variable is compared now (item 070, 3011), as for a parameter, a local
  or an index, and the type arguments of a library type or of a user generic structure are compared with the declared
  type (items 067 to 069, 3014). A pair that mentions a generic parameter, an alias or an import, and a different head
  such as an owner of a vector of `T` into a vector of `T`, is still accepted.
- A file constant passed to a generic function that takes a write borrow gives 3032, as for any function (item 010); a
  spawn of a write borrow of a call result through a generic function gives 3020 (item 029).

Settled rule 5 (a `private` name is a reject across namespaces) is enforced (3066 to 3068) but has no item here: a
private symbol needs a second namespace, and the runner gives an item one file only (see Gaps). A private symbol that a
generic or an alias leads to is not checked by sema either (see the sema doc, Gaps).

### Adversarial: reject

| Id | Shows | Expected |
|---|---|---|
| L-ADVERSARIAL-001 | A value is moved in each turn of a loop inside a fork_join block | 3071 @39 |
| L-ADVERSARIAL-002 | An outer local is moved in the body of a partition, once for each part | 3071 @59 |
| L-ADVERSARIAL-003 | A value moved in an arm of switch is spawned after the switch | 3070 @63 |
| L-ADVERSARIAL-004 | A generic function moves its argument twice | 3071 @40 |
| L-ADVERSARIAL-005 | A value is passed to a generic function twice as a write borrow | 3019 @40 |
| L-ADVERSARIAL-006 | A for each mutable element and its own vector are passed to one call | 3019 @40 |
| L-ADVERSARIAL-007 | A for each mutable appends to the vector it walks | 3073 @37 |
| L-ADVERSARIAL-008 | A partition mutable of a field and a worker that reads the owner of that field | 3074 @73 |
| L-ADVERSARIAL-009 | A read-only borrow is passed to a generic function that writes it | 3069 @39 |
| L-ADVERSARIAL-010 | A file constant is passed to a generic function as a write borrow | 3032 @41 |
| L-ADVERSARIAL-011 | A field of a borrow is returned from an arm of switch | 3018 @38 |
| L-ADVERSARIAL-012 | A contract method stores its receiver in a write borrow | 3018 @41, 3016 @41 |
| L-ADVERSARIAL-014 | A contract method writes its receiver with the default borrow | 3069 @41 |
| L-ADVERSARIAL-015 | A for each element is moved into an owned parameter | 3072 @37 |
| L-ADVERSARIAL-016 | A value moved into an owned parameter is spawned afterwards | 3070 @59 |
| L-ADVERSARIAL-017 | A local with no initial value is spawned | 3065 @56 |
| L-ADVERSARIAL-018 | A literal out of range is the argument of a spawn | 3033 @61 |
| L-ADVERSARIAL-019 | A literal out of range is the default value of a parameter | 3033 @35 |
| L-ADVERSARIAL-020 | A literal out of range is a field of a struct literal | 3033 @40 |
| L-ADVERSARIAL-021 | A literal out of range is the value of a constant used by a generic call | 3033 @35 |
| L-ADVERSARIAL-022 | An unsigned value is negated in an arm of switch | 3036 @38 |
| L-ADVERSARIAL-023 | The source of a partition mutable is read in the body while the parts are written | 3076 @60, 3075 @61 |
| L-ADVERSARIAL-024 | A nested fork_join reads what the outer block writes | 3074 @60 |
| L-ADVERSARIAL-025 | Two workers over one for each mutable element, one writes and one reads | 3074 @59 |
| L-ADVERSARIAL-026 | A value moved into a generic function is read afterwards | 3070 @42 |
| L-ADVERSARIAL-027 | A method is called on a value that was moved | 3070 @44 |
| L-ADVERSARIAL-028 | A return in an arm of switch stands directly in a fork_join block | 3024 @60 |
| L-ADVERSARIAL-029 | A write borrow of a call result is spawned through a generic function | 3020 @60 |
| L-ADVERSARIAL-030 | A generic function returns an owned parameter and the caller keeps the old name | 3071 @42 |
| L-ADVERSARIAL-031 | A worker is spawned on an element of a read-only vector and writes it | 3069 @58 |
| L-ADVERSARIAL-032 | A struct literal takes a borrow inside a loop in a fork_join block | 3018 @58 |
| L-ADVERSARIAL-033 | A nested field of a read-only parameter is written in a switch arm of a generic function | 3069 @38 |
| L-ADVERSARIAL-034 | Mutant of L-MEMORY-001: the return of the last path is dropped | 3022 @24 |
| L-ADVERSARIAL-035 | Mutant of L-FUNCTIONS-007: the return of a struct literal is dropped | 3022 @15 |
| L-ADVERSARIAL-036 | Mutant of L-FUNCTIONS-008: the return of the else path is dropped | 3022 @10 |
| L-ADVERSARIAL-037 | Mutant of L-TYPES-075: the return of a method of an implementation is dropped | 3022 @18 |
| L-ADVERSARIAL-038 | Mutant of L-TYPES-081: the return of an operator function is dropped | 3022 @17 |
| L-ADVERSARIAL-039 | Mutant of L-TYPES-109: the return of a generic function with a contract bound is dropped | 3022 @18 |
| L-ADVERSARIAL-040 | Mutant of L-MEMORY-005: the move word is dropped when an owner goes up a scope | 3016 @28 |
| L-ADVERSARIAL-041 | Mutant of L-MEMORY-012: the move word is dropped from a structure with a destroy operator | 3016 @37 |
| L-ADVERSARIAL-042 | Mutant of L-OWNERSHIP-081: the copy word is dropped, so a write borrow and the same value meet in one call | 3019 @30, 3016 @30 |
| L-ADVERSARIAL-043 | Mutant of L-OWNERSHIP-114: the copy word is dropped from the returned borrow | 3018 @21 |
| L-ADVERSARIAL-044 | Mutant of L-OWNERSHIP-122: the copy word is dropped from a borrow in a struct literal | 3018 @21 |
| L-ADVERSARIAL-045 | Mutant of L-OWNERSHIP-124: the copy word is dropped from a borrow stored in a field | 3018 @26, 3016 @26 |
| L-ADVERSARIAL-046 | Mutant of L-OWNERSHIP-145: the owned wrapper is dropped from a returned parameter | 3018 @21 |
| L-ADVERSARIAL-047 | Mutant of L-TYPES-111: the owned wrapper is dropped from a generic parameter that is returned | 3018 @11 |
| L-ADVERSARIAL-048 | Mutant of L-TYPES-075: the copy word is dropped from a generic result | 3018 @25 |
| L-ADVERSARIAL-049 | Mutant of L-BINDINGS-092: the unsafe word is dropped | 2004 @14, 2004 @14, 2005 @15, 3022 @10 |
| L-ADVERSARIAL-051 | Mutant of L-OWNERSHIP-014: copy is swapped for move on a for each element | 3072 @22 |
| L-ADVERSARIAL-054 | Mutant of L-OWNERSHIP-078: the write attribute becomes read-only on the owner of the elements that two calls write | 3069 @26, 3069 @26 |
| L-ADVERSARIAL-055 | Mutant of L-OWNERSHIP-106: the write attribute becomes read-only on a primitive parameter that the body writes | 3069 @21 |
| L-ADVERSARIAL-057 | Mutant of L-OWNERSHIP-118: copy is swapped for move in a return of a for each element | 3072 @22 |
| L-ADVERSARIAL-061 | Mutant of L-PARALLEL-001: the mutable word is dropped from a for each that writes its element | 3069 @40 |
| L-ADVERSARIAL-062 | Mutant of L-PARALLEL-006: the write attribute becomes read-only on the source of a partition mutable | 3069 @54, 3069 @55 |
| L-ADVERSARIAL-063 | Mutant of L-PARALLEL-006: the write attribute is dropped from the source of a partition mutable | 3069 @54, 3069 @55 |
| L-ADVERSARIAL-064 | Mutant of L-FUNCTIONS-014: the write attribute becomes read-only on a parameter the body writes | 3069 @16 |
| L-ADVERSARIAL-065 | Mutant of L-FUNCTIONS-014: the write attribute is dropped from a parameter the body writes | 3069 @16 |
| L-ADVERSARIAL-067 | Mutant of L-OWNERSHIP-031: one type argument of a local array is narrowed | 3014 @21 |
| L-ADVERSARIAL-068 | Mutant of L-OWNERSHIP-033: one type argument of a local vector is narrowed | 3014 @21 |
| L-ADVERSARIAL-069 | Mutant of L-TYPES-011: one type argument of a returned generic struct literal is narrowed | 3014 @16 |
| L-ADVERSARIAL-070 | Mutant of L-OWNERSHIP-083: a field of a for each element is stored in a narrower local | 3011 @22 |
| L-ADVERSARIAL-071 | Mutant of L-BINDINGS-036: the initial value of a local is dropped | 3065 @11 |
| L-ADVERSARIAL-072 | Mutant of L-BINDINGS-036: the initial value of a local computed from another is dropped | 3065 @12 |

### Adversarial: known_gap

| Id | Shows | Expected |
|---|---|---|
| L-ADVERSARIAL-013 | A contract method moves its receiver into a write borrow | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-050 | Mutant of L-OWNERSHIP-012: copy is swapped for move on a borrowed parameter | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-052 | Mutant of L-OWNERSHIP-024: copy is swapped for move on a file constant | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-053 | Mutant of L-OWNERSHIP-047: the move word is dropped from an owned argument | 3016 @35 |
| L-ADVERSARIAL-056 | Mutant of L-OWNERSHIP-114: copy is swapped for move in a return of a borrow | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-058 | Mutant of L-OWNERSHIP-122: copy is swapped for move on a borrow stored in a struct literal | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-059 | Mutant of L-OWNERSHIP-124: copy is swapped for move on a borrow stored in a field | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-060 | Mutant of L-OWNERSHIP-127: copy is swapped for move on a borrow appended to a vector | gap: #200 NEW, rule: reject |
| L-ADVERSARIAL-066 | Mutant of L-TYPES-075: copy is swapped for move in a generic return of a borrow | gap: #200 NEW, rule: reject |

## Examples and coverage

The 23 files under `examples/` are the programs the project shows. 20 of them have an accept item in
`tests/language/accept/examples/` with ids `L-EXAMPLES-nnn`: the item is the example with its `namespace` line set to
`main` and the line `-- example: <path>` below the header. The other 3 have no accept item and are named in Gaps with
the reason. `tests/source/language/MAREALanguageCoverage.cpp` fails when an example has no accept item whose header
or doc row names it and is not in its exemption list, when a doc row has no file, when a file has no row, when the
Counts table differs from the files, when a live diagnostic code has no `reject` item and no named exemption, when a
`known_gap` code does not exist, and when a rule that the language still lacks (see Gaps) has no `known_gap` item, no
reject code or no issue named in Gaps. Each direction is proven on every build with seed data in memory.

### Examples: accept

| Id | Shows | Expected |
|---|---|---|
| L-EXAMPLES-001 | the example `examples/attributes/inline_and_build.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-002 | the example `examples/attributes/layout.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-003 | the example `examples/attributes/simd.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-004 | the example `examples/attributes/visibility.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-005 | the example `examples/basics/blocks.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-006 | the example `examples/basics/control_flow.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-007 | the example `examples/basics/functions.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-008 | the example `examples/basics/operators.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-009 | the example `examples/basics/primitive_types.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-010 | the example `examples/interop/c_library.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-011 | the example `examples/memory/regions.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-012 | the example `examples/namespaces/library.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-013 | the example `examples/types/collections.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-014 | the example `examples/types/enum_methods.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-015 | the example `examples/types/generics.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-016 | the example `examples/types/structures.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-017 | the example `examples/basics/ownership.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-018 | the example `examples/parallel/scaling.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-019 | the example `examples/types/methods.marea`, with its namespace set to `main` | ok |
| L-EXAMPLES-020 | the example `examples/interop/gpu_handles.marea`, with its namespace set to `main` | ok |

### Coverage: reject

| Id | Shows | Expected |
|---|---|---|
| L-COVERAGE-001 | a constant that is its own initial value | 3055 @9 |

## Counts

The files on disk, per area and class. `CountsTableEqualsTheFilesOnDisk` fails when a number differs.

| area | accept | reject | known_gap |
|---|---|---|---|
| adversarial | 0 | 64 | 8 |
| attributes | 3 | 20 | 0 |
| bindings | 33 | 80 | 0 |
| coverage | 0 | 1 | 0 |
| examples | 20 | 0 | 0 |
| functions | 19 | 54 | 0 |
| interop | 18 | 36 | 0 |
| memory | 15 | 18 | 0 |
| namespaces | 20 | 43 | 0 |
| ownership | 65 | 78 | 2 |
| parallel | 19 | 40 | 0 |
| seed | 1 | 2 | 0 |
| types | 62 | 151 | 0 |
| total | 275 | 587 | 10 |

## Parity

The language has two backends from one IR: the bytecode interpreter and the C11 code. The parity test
(`tests/source/language/MAREALanguageParity.cpp`) runs every item of `tests/language/accept/` that has a function with a
result through both backends and compares the observable behavior. Only an accept item is a valid program, so it is
the input of the parity test: a `reject` item has no program to run, and a `known_gap` item is outside the contract
until it moves. When a `known_gap` item with `-- rule: accept` moves to `accept/`, it joins the parity set. An item that
a backend cannot run yet is listed in the test with its reason. The way the test calls the functions of an item is in
[marea-cgen.md](../cgen/marea-cgen.md), "Parity with the vm".

## Diagnostics

The catalogue raises no diagnostic of its own. A `reject` item carries a code of another stage:

| Codes | Stage | Reference |
|---|---|---|
| 1001-1006 | `lex` | [marea-lex.md](../lex/marea-lex.md) |
| 2001-2010 | `parse` | [marea-ast.md](../ast/marea-ast.md) |
| 3001-3032 | `sema` | [marea-sema.md](../sema/marea-sema.md) (3033 to 3086 follow in the rows below) |
| 3056 | `sema` | [marea-sema.md](../sema/marea-sema.md), `RESERVED_MACRO_NAME` |
| 3057-3058 | `sema` | [marea-sema.md](../sema/marea-sema.md), `OPAQUE_TYPE_MISUSE` and `OPAQUE_FIELD_BY_VALUE` |
| 3033 | `sema` | [marea-sema.md](../sema/marea-sema.md), `LITERAL_OUT_OF_RANGE` |
| 3034-3035 | `sema` | [marea-sema.md](../sema/marea-sema.md), the division and shift rules of constant operands |
| 3036-3037 | `sema` | [marea-sema.md](../sema/marea-sema.md) |
| 3038-3039 | `sema` | [marea-sema.md](../sema/marea-sema.md) |
| 3040 | `sema` | [marea-sema.md](../sema/marea-sema.md), `ASSIGNMENT_TO_CONSTANT` |
| 3041 | `sema` | [marea-sema.md](../sema/marea-sema.md), `RAW_POINTER_OUTSIDE_UNSAFE` |
| 3043 | `sema` | [marea-sema.md](../sema/marea-sema.md), `NON_BOOLEAN_CONDITION` |
| 3045-3046 | `sema` | [marea-sema.md](../sema/marea-sema.md), `RECURSIVE_TYPE` and `CYCLIC_TYPE_ALIAS` |
| 3047-3048 | `sema` | [marea-sema.md](../sema/marea-sema.md), `EMPTY_ENUM` and `UNKNOWN_ENUM_VARIANT` |
| 3049-3050 | `sema` | [marea-sema.md](../sema/marea-sema.md), `UNKNOWN_TYPE_FUNCTION` and `OPERATOR_SIGNATURE_MISMATCH` |
| 3051-3052 | `sema` | [marea-sema.md](../sema/marea-sema.md), `BOUND_OPERATION_NOT_GIVEN` and `OVERLAPPING_IMPLEMENTATION` |
| 3053 | `sema` | [marea-sema.md](../sema/marea-sema.md), `FUNCTION_TYPE_MISMATCH` |
| 3054 | `sema` | [marea-sema.md](../sema/marea-sema.md), `INVALID_RESULT_USE` |
| 3055 | `sema` | [marea-sema.md](../sema/marea-sema.md), `CYCLIC_CONSTANT` |
| 3059 | `sema` | [marea-sema.md](../sema/marea-sema.md), `COMPILE_TIME_ASSERTION_FAILED` |
| 3060-3061 | `sema` | [marea-sema.md](../sema/marea-sema.md), `UNKNOWN_MACRO` and `INVALID_MACRO_ARGUMENT` |
| 3062 | `sema` | [marea-sema.md](../sema/marea-sema.md), `PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN` |
| 3063-3064 | `sema` | [marea-sema.md](../sema/marea-sema.md), `SPAWN_NEEDS_CALL` and `PARTITION_SOURCE_NOT_ARRAY` |
| 3065 | `sema` | [marea-sema.md](../sema/marea-sema.md), `LOCAL_WITHOUT_INITIAL_VALUE` |
| 3066-3068 | `sema` | [marea-sema.md](../sema/marea-sema.md), `PRIVATE_SYMBOL_ACCESS`, `PRIVATE_TYPE_ACCESS`, `PRIVATE_FIELD_ACCESS`, no item reaches them (see Gaps) |
| 3069 | `sema` | [marea-sema.md](../sema/marea-sema.md), `WRITE_THROUGH_READ_ONLY_BORROW` |
| 3075-3076 | `sema` | [marea-sema.md](../sema/marea-sema.md), `PARTITION_BODY_WRITES_OUTER_NAME` and `PARTITION_SOURCE_USED_IN_BODY` |
| 3077 | `sema` | [marea-sema.md](../sema/marea-sema.md), `MISSING_FIELD` |
| 3078-3081 | `sema` | [marea-sema.md](../sema/marea-sema.md), `UNIMPORTED_NAMESPACE`, `UNRESOLVED_REQUIRE`, `AMBIGUOUS_NAMESPACE` and `RESERVED_NAMESPACE_ROOT` |
| 3083-3086 | `sema` | [marea-sema.md](../sema/marea-sema.md), `BACKEND_UNAVAILABLE`, `ATTRIBUTE_NOT_ALLOWED_HERE`, `MISPLACED_ATTRIBUTE` and `INVALID_BUILD_VALUE`; 3083 and 3086 have no item (see Gaps) |
| 4004-4006 | `ir` | [marea-ir.md](../ir/marea-ir.md), no item reaches them (see Gaps) |

## Gaps

- The runner checks one file per item, so an item cannot import a second file of the catalogue. A rule that needs
  two namespaces (settled rule 5) can use only an `EXTERNAL` import, which the runner gives to the `c.`,
  `collections.`, `concurrency.`, `arenas.` prefixes and to `text`, until a later change adds a way to give an item a
  second file.
- The codes 3066, 3067 and 3068 (a private symbol, type or field of another namespace) have no item for the same
  reason: they need a second namespace (issue #250). `tests/source/sema/MAREASemaVisibility.cpp` proves them.
- The code 3082 (`ELEMENT_WITHOUT_MEASURED_LAYOUT`, an element with no measured layout held by value) has no item
  yet: no shipped library type exists for an item to write, since sema now knows no library and only reads the
  descriptors a registry holds (a library type in an item is the fixture root `demo`, registered by the runner). `tests/source/sema/MAREASemaTypeRegistry.cpp` proves the code with a fixture descriptor instead.
- The codes 3083 (`BACKEND_UNAVAILABLE`) and 3086 (`INVALID_BUILD_VALUE`) have no item: the runner gives sema no target
  description and no build settings, and `include_bytes!` of a library has no include resolver there either (the `reject`
  item L-INTEROP-070 shows the missing-file case only). `tests/source/sema/MAREASemaBackendWhen.cpp`,
  `tests/source/sema/MAREASemaBuildValue.cpp` and `tests/source/sema/MAREASemaRegisteredMacros.cpp` prove them.
- The runner compares the codes of a `reject` item in the order the compiler gives them, so an item over two stages
  depends on the order of the stages (lex, then parse, then sema).
- A known_gap item must be accepted today, so a valid program that the compiler rejects by mistake has no item. Known:
  a `switch` over a `boolean` with `case true` and `case false` and no `default` gives 3022 (issue #226).
- The parse recovery is shown as it is: after a parse error the later stages still run, so some parse items list sema
  codes after the parse code, and a keyword used as a name gives a short chain of parse codes.
- The adversarial area has no item for a private symbol reached through a generic or an alias (settled rule 5): the
  runner gives an item one file. The mutation of the accept items was run once, by a script that is not in the
  repository, so a new accept item gets no mutants until someone runs that sweep again. A copy of a structure with a
  `destroy` operator (the `copy` of L-MEMORY-012) is accepted; no rule says whether it must be refused, so it has no item.

### Open decisions and holes of the catalogue

- Accepted `copy` of a structure with a `destroy` operator: the language has no rule that says whether it is allowed
  and no item shows it (see the adversarial note above). The decision waits for the user.
- Issue #226 (a `switch` over a `boolean` with `case true` and `case false` and no `default` is rejected with 3022)
  cannot be an item. A known_gap item must be accepted today and a reject item states the wanted outcome, so the
  program fits neither class. The runner has no class for a valid program that the compiler rejects.
- Settled rule 5 (a `private` name is a reject across namespaces, enforced by 3066 to 3068) and an import cycle cannot be shown with one file per
  item. Issue #250 asks for a runner that gives an item a second file. Until then these two rules have no item.
- Settled rule 2 (a non-primitive parameter with no attribute is a read-only borrow) holds in the grammar, the sema
  doc, `MAREA_SemaGetParameterSemantics` and the four examples that write a parameter
  (`examples/basics/ownership.marea`, `examples/types/methods.marea`, `examples/interop/gpu_handles.marea` and
  `examples/parallel/scaling.marea`: each names `#[semantics = reference_mutable]` on the parameter it writes). Sema
  rejects a write through a read-only borrow with 3069. It does not see a write through a method call on a read-only
  receiver or a library call that mutates (`values.append(x)`): sema does not know which calls mutate.

### Examples with no accept item

| Example | Why |
|---|---|
| `examples/macros/builtin.marea` | imports the project namespace `attributes.layout`, and an item is one file (issue #250) |
| `examples/main.marea` | imports eight project namespaces, and an item is one file (issue #250) |
| `examples/namespaces/consumer.marea` | imports the project namespace `namespaces.library`, and an item is one file (issue #250) |

The coverage test holds this list; an example that gains an item must leave it.

### Diagnostic codes with no reject item

Every live lex, parse and sema code (1001-1006, 2001-2010, 3001-3086 without 3023 and the numbers no code uses) has a `reject` item, except 3066,
3067 and 3068 (3082, 3083 and 3086 are exempt as the Gaps list above says): no item can reach a second namespace (issue #250), and `tests/source/sema/MAREASemaVisibility.cpp`
proves them. The ir codes
4004, 4005 and 4006 are exempt: the runner runs lex, parse and sema only, so no item can reach the ir. The exemption ends
when a runner or an apply makes the ir run in it. The retired codes 3023 and 4001-4003 are excluded and must not
return in a header.

### Rules the language still lacks

Each rule is kept as a `known_gap` item with its issue, as reject codes, or as an issue with no item:

| Missing rule | Where it is kept |
|---|---|
| use of a moved value | reject items of 3070 and 3071 (issue #198), a move in any arm of a branch included; a partial move (3072) still rounds toward valid |
| use of an uninitialized value | reject items of 3065 (a local with no initial value, issue #177) |
| write through a read-only borrow | reject items of 3069 (a write through a read-only borrow, issues #203, #204, #206 and #214) |
| literal out of range | reject items with 3033 (L-BINDINGS-022, 026 to 029, 031, L-ADVERSARIAL-018 to 021) |
| constant division by zero | reject items of 3034 |
| shift count too large | reject items of 3035 |
| cyclic constant | reject items of 3055 (L-COVERAGE-001) |
| unreachable switch arm | reject items of 3025 (a repeated case) and 2010 (a default that is not last) |
| import cycle | no item, issue #250 |
| private access across namespaces | enforced by 3066, 3067 and 3068; no item, issue #250 |
