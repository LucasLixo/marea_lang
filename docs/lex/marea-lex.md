# Marea lex stage

## Contract

The `lex` stage reads a raw ASCII source buffer and writes a token list and diagnostics. It has no
global state and never prints: the caller gives it an arena (that owns the token list and the
context) and a diagnostic sink in a `MAREA_LexContext`. It never reads the disk. A token owns no
text: its text points into the caller's source buffer, so the caller keeps that buffer alive as long
as it reads the tokens. Bad input is never fatal: every malformed input is a diagnostic plus a
`MAREA_TOKEN_KIND_ERROR` token (a few cases are a diagnostic only), and lexing goes on. The list
always ends with exactly one `MAREA_TOKEN_KIND_END_OF_FILE` token. Every lexer diagnostic has the
severity `MAREA_DIAGNOSTIC_SEVERITY_ERROR`.

The lexical grammar is in [the grammar](../grammar/marea.ebnf).

## Use

```c
MAREA_Arena *arena;
MAREA_DiagnosticSink *sink;
MAREA_LexContext *lex;
MAREA_TokenList *tokens;

MAREA_ArenaCreate(0, &arena);
MAREA_DiagnosticSinkCreate(arena, &sink);
MAREA_LexContextCreate(arena, sink, &lex);
MAREA_LexContextSetFileName(lex, "src/main.marea");   /* optional, default "<source>" */
MAREA_LexTokenize(lex, source, source_length, &tokens);

for (MAREA_IntegerSize i = 0; i < MAREA_TokenListGetCount(tokens); i++) {
    const MAREA_Token *token;
    MAREA_TokenListGetAt(tokens, i, &token);
    /* MAREA_TokenGetKind, MAREA_TokenGetText + MAREA_TokenGetLength, MAREA_TokenGetPosition */
}
```

`MAREA_LexTokenize` returns an error only for a null argument (`context`, `outList`, or `source`
with a non-zero length), a `length` above `UINT32_MAX` (`MAREA_LEX_EXCEPT_ERROR_SOURCE_TOO_LARGE`),
or an out of memory (`MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY`). The out of memory result also covers a
diagnostic that the sink cannot store: the run stops at once, so a lost diagnostic is never a clean
run, and `outList` is not set. A lexical error is a diagnostic in the sink, not a return value.
`MAREA_LexContextCreate` returns a null-pointer error when `arena`, `sink` or `outContext` is null.
`MAREA_TokenListGetAt` returns `MAREA_LEX_EXCEPT_ERROR_OUT_OF_RANGE` for an index not below the
count. Every accessor on a null token or list returns a default (kind `ERROR`, null text, length 0,
a zeroed position, count 0). A zero-length source is valid and gives only the `END_OF_FILE` token.

The file name is kept as given, never copied, and goes in the position of every token and
diagnostic. Later stages read it (the `sema` path rule uses it), so give a project relative path.
`MAREA_LexContextSetFileName` accepts a null name (not rejected by the code).

## Source text

- Only ASCII is valid. A byte outside the language is `UNKNOWN_BYTE` (see [Recovery](#recovery)).
- Whitespace (space, tab, `\r`, `\n`) separates tokens and produces none. `\r` is whitespace, so
  `\r\n` works.
- A comment is `--` to the end of the line. It produces no token. `--` always starts a comment, even
  right after another `-`. There is no block comment.
- Position: `line` and `column` are 1-based; `offset` is the byte offset; `length` is the byte
  length. A line feed starts a new line; a tab counts as one column. The `END_OF_FILE` token has
  length 0 and null text, at the end offset.
- The lexer takes the longest match: `**=` before `**` before `*`.

## Token kinds

`MAREA_TokenKind` lists, in order: 7 base kinds, 44 keywords, 36 operators, 14 separators, then
`MAREA_TOKEN_KIND_COUNT`. `MAREA_TokenKindGetName` gives the stable name (`KEYWORD_AND`, `PLUS`),
or `"UNKNOWN"` for a value at or above `COUNT`.

- **Base kinds:** `END_OF_FILE`, `ERROR`, `IDENTIFIER`, `INTEGER_LITERAL`, `FLOAT_LITERAL`,
  `STRING_LITERAL`, `CHARACTER_LITERAL`.
- **Identifier:** `(letter | "_") { letter | digit | "_" }`, ASCII letters.
- **Keywords (44, never an identifier):** `and`, `as`, `break`, `case`, `constant`, `continue`,
  `contract`, `copy`, `default`, `do`, `each`, `else`, `elseif`, `end`, `enum`, `false`, `for`,
  `fork_join`, `function`, `if`, `implementation`, `import`, `in`, `into`, `local`, `move`,
  `mutable`, `namespace`, `not`, `opaque`, `or`, `partition`, `require`, `return`, `spawn`,
  `structure`, `switch`, `then`, `true`, `type`, `unsafe`, `where`, `while`, `with`. The kind is
  `KEYWORD_<NAME>`. Keywords are case sensitive; `true` and `false` are the kinds `KEYWORD_TRUE` and
  `KEYWORD_FALSE`, not a boolean literal kind. `reference` is an identifier. `when`, `all` and `any`
  are identifiers too: the parser gives them a meaning inside an attribute item `when(...)`.
  `MAREA_TokenKindLookupKeyword(text, length)` returns the keyword kind or `IDENTIFIER`.
- **Operators (36):** `+ - * / % ** +% -% *% +| -| *| ~ & | ^ << >> == != < <= > >= = += -= *= /= %=
  **= <<= >>= &= |= ^=`.
- **Separators (14):** `( ) [ ] { } , ; : . .. ! #[ #![`.

Primitive type names (`boolean`, `integer32`, `string` and the rest), `vector`, `pointer`, `Self`,
`self`, `c` and the built-in call names are not lexer data. The lexer has no table of predeclared
names: they lex as plain `IDENTIFIER` tokens and a later stage gives them meaning. `public` and
`private` are identifiers too.

The lexer never splits `>>`: it is always one `SHIFT_RIGHT` token (or `SHIFT_RIGHT_EQUAL` for
`>>=`), including in a type argument list. A consumer that needs two `>` has to handle it: the
parser splits the token when it reads a type argument list (a decision, see Gaps).

## Number literals

- Decimal: digits and `_` (an `_` is skipped and never counted as a digit). Kind `INTEGER_LITERAL`.
- Hexadecimal `0x...` and binary `0b...` (lowercase prefix only), with digits and `_`. The first byte
  after the prefix must be a digit; `0x_1` is malformed.
- Float: a decimal integer, then optionally `.` followed by at least one digit, then optionally an
  exponent `e` or `E`, an optional `+` or `-`, and at least one digit. The kind is `FLOAT_LITERAL`
  when a fraction or an exponent is present. There is no float form of hex or binary.
- A point with no digit after it is not part of the number: `1.` is `1` then `.`, and `1..2` is
  `1`, `..`, `2`. An `e` with no digit after it is not an exponent: `1e` is `1` then the identifier
  `e`.
- A number glued to letters splits with no diagnostic: `12abc` is `12` then `abc`.
- There is no sign in a literal: `-1` is `MINUS` then `1`.
- The lexer does not check the range or the value of a number.

## String and character literals

A string is `"..."`. A character literal is `'...'`. Both hold ASCII text and the escapes `\n`, `\t`,
`\r`, `\0`, `\\`, `\"` and `\xHH` (two hex digits); a character literal also accepts `\'`. The
token text keeps the quotes and the escapes as written: the lexer does not decode them. A `--` or a
`'` inside a string is plain text. A character literal counts an escape as one character; it holds
exactly one, otherwise it is an error (see below). There is no string interpolation.

Inside both literals, a byte of 0x80 or above, a control byte below 0x20 other than tab, and 0x7F
are `UNKNOWN_BYTE`; the literal goes on.

## Recovery

The lexer never stops on a lexical error.

- **Unknown byte** (a byte that starts no token, such as `@`, `$`, `?`, a lone `#`, a non-ASCII byte,
  a NUL or a control byte): one `UNKNOWN_BYTE` diagnostic, one `ERROR` token of length 1, then the
  scan goes on at the next byte.
- **Unterminated string or character literal** (cut off by `\n`, `\r`, a `\` at the end of input, or
  the end of input): one `UNTERMINATED_STRING` diagnostic and one `ERROR` token for the text read so
  far. The line end is not part of the token, so lexing resumes at it. A character literal uses the
  same code (the message says "character literal").
- **Invalid escape** (a bad `\x` or any other byte after `\`): `INVALID_ESCAPE`, no `ERROR` token;
  the literal goes on and ends as a normal literal. A bad escape followed by a line end does not
  consume it, so the literal is then unterminated too.
- **Bad byte inside a literal:** `UNKNOWN_BYTE`, no `ERROR` token; the literal goes on.
- **Out of memory of the sink:** a diagnostic that the sink cannot store ends the run with
  `MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY`. No `ERROR` token is added for it.
- **Malformed number** (`0x` or `0b` with no digit, or with `_` first): `MALFORMED_NUMBER` and an
  `ERROR` token for the prefix only; the bytes after it are lexed as normal tokens (`0x_1` is `ERROR`
  then identifier `_1`).
- **Empty or long character literal** (`''`, `'ab'`): one diagnostic and one `ERROR` token for the
  whole literal.

The position of an `UNKNOWN_BYTE` outside a literal and inside a literal points at that byte. The
position of the other diagnostics spans from the start of the literal or number to the point where
the problem was found.

## Diagnostics

Codes are in the 1000-1999 range. Every one is an error. The messages are non-empty ASCII text.

| Code | Name | Example |
|---|---|---|
| 1001 | `UNKNOWN_BYTE` | `@`, a non-ASCII byte, or a control byte inside a string |
| 1002 | `UNTERMINATED_STRING` | `"abc` then a line end; also an unterminated `'a` |
| 1003 | `INVALID_ESCAPE` | `"\q"`, `"\xZ1"` |
| 1004 | `MALFORMED_NUMBER` | `0x`, `0b`, `0x_1` |
| 1005 | `EMPTY_CHARACTER` | `''` |
| 1006 | `TOO_MANY_CHARACTERS` | `'ab'` |

The C names are `MAREA_LEX_DIAGNOSTIC_CODE_<NAME>`.

## Gaps

- The lexer never splits `>>`, by decision: the parser splits it in a type argument list. A consumer of
  the token list that reads types itself has to do the same.
- There is no block comment; only `--` to the end of the line. This is a decision, not a missing
  feature.
- Escapes are checked but not decoded, and a number is not checked for range, by decision: sema decodes
  an escape when it types a literal and reports a literal out of range (code 3033).
- An uppercase prefix (`0X`, `0B`) is not a prefix, by decision: `0X1` is `0` then the identifier
  `X1`, and the lexer gives no diagnostic for it. A test pins the result. A clear error for it is not
  done.
- Only ASCII is accepted. There is no UTF-8 in identifiers, strings or comments (a byte of 0x80 or
  above is `UNKNOWN_BYTE` outside a comment; inside a comment it is skipped without a diagnostic). The
  plan is UTF-8 in strings and comments with ASCII identifiers; it needs a rule for a byte sequence
  that is not valid UTF-8 and a new diagnostic code, so it is not done.
