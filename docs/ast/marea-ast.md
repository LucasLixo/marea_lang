# Marea parse stage

## Contract

The `parse` stage turns the token list of the `lex` stage into a syntax tree. It reads public data
(tokens) and writes public data (a tree and diagnostics). It has no global state, never prints and
never reads the disk. The grammar is [marea.ebnf](../grammar/marea.ebnf).

## Use

```c
MAREA_Arena *arena;            /* the caller owns it; destroying it frees everything */
MAREA_DiagnosticSink *sink;
MAREA_LexContext *lex;
MAREA_TokenList *tokens;
MAREA_ParseContext *parse;
const MAREA_AstNode *root;

MAREA_ArenaCreate(0, &arena);
MAREA_DiagnosticSinkCreate(arena, &sink);
MAREA_LexContextCreate(arena, sink, &lex);
MAREA_LexTokenize(lex, source, length, &tokens);
MAREA_ParseContextCreate(arena, sink, &parse);
MAREA_ParseCompilationUnit(parse, tokens, &root);
```

The source buffer and the token list stay alive as long as the tree is read: the text of a node
points into the source. `MAREA_ParseCompilationUnit` returns an error only for a null argument or an
out of memory. A syntax error is a diagnostic in the sink, and the tree is still returned.

A node is opaque. Read it with `MAREA_AstNodeGetKind`, `GetPosition` (by value), `GetText` and
`GetTextLength` (a pointer into the source, not NUL terminated), `GetOperator` (a token kind, ERROR
when none), `GetFlags`, `GetChildCount` and `GetChildAt`. `MAREA_AstKindGetName` gives the stable
name of a kind. A getter on a null node gives a neutral value.

## Text dump

`MAREA_AstDump(arena, root, &text, &length)` writes one node per line into the arena:

- two spaces per level of depth, then the kind name;
- ` "text"` when the node has text (`"` and `\` are written `\"` and `\\`, a byte below 0x20 or above
  0x7E is written `\xHH` in upper case);
- ` op=NAME` when the node has an operator (the token kind name, for example `op=STAR_STAR`);
- ` flags=N` when the flags are not 0;
- a new line. A position is never written.

The dump walks the tree with a stack of its own in the arena, not with recursion, so the call stack stays
flat for a tree of any depth, for example a left-deep chain of binary operators.

```
COMPILATION_UNIT
  NAMESPACE_DECLARATION
    QUALIFIED_NAME
      IDENTIFIER "main"
```

## Node kinds and children

`x*` is zero or more, `x+` one or more, `[x]` optional. "type" is `NAMED_TYPE` or `FUNCTION_TYPE`,
"expr" is any expression kind. A leading run of `ATTRIBUTE` children belongs to the declaration,
field or member that owns it.

| Kind | Children |
|---|---|
| `COMPILATION_UNIT` | `NAMESPACE_DECLARATION`, `FILE_ATTRIBUTE*`, `IMPORT_DECLARATION*`, item* |
| `NAMESPACE_DECLARATION` | `QUALIFIED_NAME` |
| `IMPORT_DECLARATION` (flag HAS_ALIAS or HAS_MEMBERS, never both) | `QUALIFIED_NAME` (the namespace), then [`IDENTIFIER` alias] when HAS_ALIAS, or `IMPORT_MEMBER`+ when HAS_MEMBERS |
| `IMPORT_MEMBER` (flag HAS_ALIAS) | `QUALIFIED_NAME` (the member path, one or two parts for the checker), [`IDENTIFIER` alias] |
| `QUALIFIED_NAME` | `IDENTIFIER`+ |
| `IDENTIFIER`, `INTEGER_LITERAL`, `FLOAT_LITERAL`, `STRING_LITERAL`, `CHARACTER_LITERAL`, `BOOLEAN_LITERAL` | none, they have text (a string or a character keeps its quotes and is not decoded) |
| `ATTRIBUTE`, `FILE_ATTRIBUTE` | `ATTRIBUTE_ITEM`+ |
| `ATTRIBUTE_ITEM` | `IDENTIFIER` key [`BOOLEAN_LITERAL`, `INTEGER_LITERAL`, `STRING_LITERAL` or `QUALIFIED_NAME`] |
| `CONSTANT_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, type, expr |
| `TYPE_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, [`GENERIC_PARAMETERS`], type |
| `STRUCTURE_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, [`GENERIC_PARAMETERS`], `FIELD_DECLARATION*` |
| `FIELD_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, type |
| `OPAQUE_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER` |
| `ENUM_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, [tag type], `VARIANT_DECLARATION*` |
| `VARIANT_DECLARATION` | `IDENTIFIER`, `VARIANT_FIELD*` |
| `VARIANT_FIELD` | `IDENTIFIER`, type, [expr] |
| `CONTRACT_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, `CONTRACT_FUNCTION*` |
| `CONTRACT_FUNCTION` | `IDENTIFIER`, `PARAMETER_LIST`, [type] |
| `IMPLEMENTATION_DECLARATION` | `ATTRIBUTE*`, [`GENERIC_PARAMETERS`], `NAMED_TYPE`, (`FUNCTION_DECLARATION` or `CONSTANT_DECLARATION`)* |
| `FUNCTION_DECLARATION` | `ATTRIBUTE*`, `IDENTIFIER`, [`GENERIC_PARAMETERS`], `PARAMETER_LIST`, [type], [`WHERE_CLAUSE`], `BLOCK` |
| `PARAMETER_LIST` | `PARAMETER*` (always present) |
| `PARAMETER` | `IDENTIFIER`, type, [default expression], `ATTRIBUTE*` |
| `GENERIC_PARAMETERS` | `GENERIC_PARAMETER`+ |
| `GENERIC_PARAMETER` (flag CONSTANT) | `IDENTIFIER`, and a type when the flag is set |
| `WHERE_CLAUSE` | `CONSTRAINT`+ |
| `CONSTRAINT` | `IDENTIFIER`, `QUALIFIED_NAME`+ |
| `NAMED_TYPE` | `QUALIFIED_NAME`, [`TYPE_ARGUMENTS`] |
| `FUNCTION_TYPE` | `PARAMETER_LIST`, [type] |
| `TYPE_ARGUMENTS` | (type or `INTEGER_LITERAL`)+ |
| `BLOCK` | statement* |
| `LOCAL_DECLARATION` | `IDENTIFIER`, type, [expr] |
| `ASSIGNMENT` (op = the assignment token) | target expr, value expr |
| `EXPRESSION_STATEMENT` | expr |
| `IF_STATEMENT` | expr, `BLOCK`, `ELSEIF_CLAUSE*`, [`ELSE_CLAUSE`] |
| `ELSEIF_CLAUSE` | expr, `BLOCK` |
| `ELSE_CLAUSE` | `BLOCK` |
| `WHILE_STATEMENT` | expr, `BLOCK` |
| `FOR_RANGE_STATEMENT` | `IDENTIFIER`, first expr, last expr, `BLOCK` |
| `FOR_EACH_STATEMENT` (flag MUTABLE) | `IDENTIFIER`, [`IDENTIFIER`], expr, `BLOCK` |
| `SWITCH_STATEMENT` | expr, `SWITCH_ARM*`, [`DEFAULT_ARM`] |
| `SWITCH_ARM` | expr+, `BLOCK` |
| `DEFAULT_ARM` | `BLOCK` |
| `WITH_STATEMENT` | expr, `IDENTIFIER`, `BLOCK` |
| `PARTITION_STATEMENT` (flag MUTABLE) | value expr, count expr, `IDENTIFIER`, `BLOCK` |
| `RETURN_STATEMENT` | [expr] |
| `BREAK_STATEMENT`, `CONTINUE_STATEMENT` | none |
| `SCOPE_BLOCK`, `UNSAFE_BLOCK`, `FORK_JOIN_BLOCK` | `BLOCK` |
| `MACRO_STATEMENT` | `ATTRIBUTE*` (top level only), `MACRO_CALL` |
| `BINARY_EXPRESSION` (op = the operator token) | expr, expr |
| `UNARY_EXPRESSION` (op = `-`, `~`, `not`, `copy`, `move` or `spawn`) | expr |
| `PARENTHESIZED_EXPRESSION` | expr |
| `MEMBER_ACCESS` | expr, `IDENTIFIER` |
| `INDEX_ACCESS` | expr, expr |
| `CALL_EXPRESSION` | callee expr, [`TYPE_ARGUMENTS`], `ARGUMENT_LIST` |
| `ARGUMENT_LIST` | expr* |
| `GENERIC_NAME` | `QUALIFIED_NAME`, `TYPE_ARGUMENTS` |
| `LITERAL_VALUE` | `NAMED_TYPE`, (`FIELD_ENTRY`, `INDEX_ENTRY` or `VALUE_ENTRY`)* |
| `FIELD_ENTRY` | `IDENTIFIER`, expr |
| `INDEX_ENTRY` | expr, expr |
| `VALUE_ENTRY` | expr |
| `MACRO_CALL` | `IDENTIFIER` (text is the macro name), `ARGUMENT_LIST` |
| `ERROR_NODE` | none, placed at the bad token |

The flag values are MUTABLE 1, CONSTANT 4, HAS_ALIAS 8, FILE 16 and HAS_MEMBERS 32. Bit 2
(REFERENCE) is retired: the parser never sets it any more. A `PARAMETER`'s own `#[semantics = ...]`
attribute, adopted last so the name, type and default keep their fixed positions (unlike every
other declaration, whose attributes lead), is the one source sema reads for its passing mode -
see [the sema stage doc](../sema/marea-sema.md) ("Ownership").
The word after the namespace path of an import decides its form: `as` or `;` is the whole path
form, `{` is the member list. For example, `import a.b { f as g, T };` dumps as:

```
  IMPORT_DECLARATION flags=32
    QUALIFIED_NAME
      IDENTIFIER "a"
      IDENTIFIER "b"
    IMPORT_MEMBER flags=8
      QUALIFIED_NAME
        IDENTIFIER "f"
      IDENTIFIER "g"
    IMPORT_MEMBER
      QUALIFIED_NAME
        IDENTIFIER "T"
```

The parser keeps no word list of its own for names: `array<T, 4>` and `vector<T>` are both a `NAMED_TYPE` whose
`TYPE_ARGUMENTS` hold types or an `INTEGER_LITERAL` (the split of a fixed `array` from a growable `vector`, and
which names are primitives such as `character`, is sema's). An `enum` tag type (`enum Light : uinteger8`) is
any type to the parser, a type child right after the name; that it must be an integer primitive is sema's rule.
An attribute is read as a plain list: the parser never checks a key or a value, so `#[visibility = public]`,
`#[operator = "construct"]` (a `STRING_LITERAL` value) and `#[semantics = reference_mutable]` (a
`QUALIFIED_NAME` value) are the same shape as an unknown key.

`reference` is not a reserved word any more (it lexes as an `IDENTIFIER`), and `mutable` is read only by
`for each` and `partition`, never on a parameter.

A bare name in an expression is an `IDENTIFIER` node. `or`, `and`, comparison, bit, shift, additive,
multiplicative operators and `**` are all `BINARY_EXPRESSION` with the token kind as operator.

Creating a value has two spellings and the parser tells them apart by the next token only. `Point{ x = 1.0 }`
is a `LITERAL_VALUE`. `Point(1.0)` is an ordinary `CALL_EXPRESSION` whose callee is an `IDENTIFIER` (or a
`MEMBER_ACCESS` chain for a qualified type name): the parser does not know the name is a type, so deciding
that it is a construction call is sema's job. No test builds a call on a type name on purpose; the shape is
the same one `f(1)` has in `PostfixChains`.

## Precedence

From the weakest to the strongest: `or`; `and`; comparison (does not chain); `|`; `^`; `&`; `<<` and
`>>`; `+ - +% -% +| -|`; `* / % *% *|`; unary `- ~ not` and `copy move spawn`; `**`; postfix.
All binary levels are left to right except `**`, which is right to left and stronger than the unary
operators on its left: `-a ** 2` is `-(a ** 2)`, and the right side may be unary: `2 ** -1`.
`copy`, `move` and `spawn` take a postfix operand.

## Lookahead rules

- **Type arguments in an expression.** A `<` after a name starts type arguments only when a balanced
  `>` closes it and the token after that is `(` (a call) or `{` (a literal value), or `.` when the
  name is a primary (`array<float32>.filled(0.0)` is a `GENERIC_NAME`). Between the brackets only
  type tokens are allowed: names, `.`, `,`, integers, `function`, `:` and balanced parentheses. A `#[` is not in
  that vocabulary, so a function type with a parameter attribute inside the brackets of an expression
  (`f<function(#[semantics = reference_mutable] a: T)>(x)`) is read as a comparison (see Gaps). A `>>`
  closes two levels.
  The scan is limited to 1024 tokens and allocates
  nothing. Otherwise the `<` is a comparison. `a < b > (c)` is therefore a generic call.
- **Type arguments in a type.** A `<` always starts type arguments. A `>>` token is split in two
  closing brackets (`array<array<float32>>`).
- **Literal value.** A `{` after a name, with or without type arguments, starts a literal value. A
  `{` never starts a block.
- **Statement.** A statement that is not a keyword statement is parsed as an expression first. When
  an assignment operator follows, it is an `ASSIGNMENT`; a bare macro call is a `MACRO_STATEMENT`;
  anything else is an `EXPRESSION_STATEMENT`.
- **`for`.** `for each` starts a `FOR_EACH_STATEMENT`, any other `for` a `FOR_RANGE_STATEMENT`.
- **Macro name.** `name!(` is a macro call. The word `namespace` may also name a macro
  (`namespace!()`), the one keyword exception that the grammar lists in `macro_call`.

## Recovery

- A bad top level token gives an `ERROR_NODE` child of the root, and the parser skips to a `;`
  (consumed) or to a safe word: `end`, `case`, `default`, `elseif`, `else`, a keyword that starts an
  item or a statement, or `#[`.
- A bad member of a structure, enum, contract or implementation, and a bad arm of a switch, is
  treated the same way. Every loop of the parser moves at least one token on each pass.
- A missing `;`, `then`, `do` or closing bracket is reported and nothing is skipped, so the next
  construct is parsed normally.
- An import member list that is empty, ends with a comma, has an `as` without a name, has no `}`, or
  follows an `as` alias is reported once and nothing more is skipped by the list itself. A missing
  `;` is then reported at the same token, so it adds no second diagnostic, and the parser goes on
  with the next import or item. A token that cannot start a member ends the list and the top level
  recovery skips to the next `;`.
- A block stops at `elseif`, `else`, `case`, `default` or `end`, at the end of the file, or at the start
  of a top level declaration, so an unfinished body never hides the next function. An `if` and a
  `switch` close with exactly one `end` for the whole statement. A branch of an `if` (the `if` branch,
  each `elseif` and the `else`) and an arm of a `switch` (each `case` and the `default`) has no `end` of
  its own, and its block stops without a diagnostic at the next branch or arm keyword or at that
  closing `end`. The old form, with an `end` after every branch or arm and one more for the statement,
  is reported: the `end` of a branch closes the statement, so the next `elseif`, `else`, `case` or
  `default` is a stray token and the last `end` is a stray token or an unterminated block.
- Nesting is limited to 256 levels of expression, statement and type. Deeper input gives
  `RECURSION_TOO_DEEP` and that construct is left unparsed.

## Diagnostics

All are errors, from 2001 up. A diagnostic is not repeated at the same byte offset twice in a row.

| Code | Name | Example |
|---|---|---|
| 2001 | `EXPECTED_TOKEN` | `namespace main` (no `;`), `import a { };` (empty member list), `import a { f, };`, `import a { f as };`, `import a as b { f };` |
| 2002 | `EXPECTED_EXPRESSION` | `constant x: sinteger32 = 1 +;` |
| 2003 | `EXPECTED_TYPE` | `constant x: = 1;` |
| 2004 | `EXPECTED_DECLARATION` | `namespace main; 42;`, an import after a declaration |
| 2005 | `UNEXPECTED_TOKEN` | a stray `end`, `case` outside a `switch`, an `else` after a statement that an earlier `end` already closed (the old per-branch form), or a default value in a function type parameter (`function(a: sinteger32 = 0)`, the tree is still built) |
| 2006 | `CHAINED_COMPARISON` | `a < b < c` (the tree is still built) |
| 2007 | `MISSING_NAMESPACE` | a file that does not start with `namespace` |
| 2008 | `RECURSION_TOO_DEEP` | 300 nested parentheses, `if` blocks or type arguments |
| 2009 | `UNTERMINATED_BLOCK` | a structure, function or other block that reaches the end of the file |
| 2010 | `DEFAULT_ARM_NOT_LAST` | `switch a default then x = 1; case 2 then y = 2; end` (the tree is still built) |

## Gaps

- A function type that carries a parameter attribute (`#[semantics = ...]`) inside the type arguments of
  an expression is not found by the type-argument scan, so `f<function(#[semantics = reference_mutable] a: T)>(x)`
  parses as comparisons. In a type position the same text parses (`MAREAParserTypes`, `FunctionTypes`). No
  test covers the expression case; this is read from the scan's token list in `MAREAParseExpression.c`.
- No test builds a call on a type name (`Point(1.0)`) or an `enum` tag type that is not an integer; both
  parse as plain calls and plain types, and the construction and tag rules are sema's.
- The semantic rules that the grammar gives to "the checker" are not parsed: the namespace against
  the path, attribute keys and values, ownership, switch coverage.
- An `IMPORT_MEMBER` path may have any number of parts in the tree. This is not a parser gap: only one
  part (a function, a type or a constant) or two parts (an enum variant) are valid, and that rule is
  sema's job by design, enforced in `MAREA_G_MemberResolves` (`MAREASemaNamespace.c`), like the other
  rules the grammar gives to "the checker".
