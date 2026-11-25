# Marea parse stage

## Contract

The `parse` stage turns the token list of the `lex` stage into a syntax tree. It reads public data
(tokens) and writes public data (a tree and diagnostics). It has no global state, never prints and
never reads the disk. The grammar is [marea.ebnf](../grammar/marea.ebnf). The parser reads the shape
only: the rules that the grammar gives to "the checker" (the namespace against the path, attribute keys
and values, ownership, switch coverage, the names and values of a `when` predicate) are not parsed, by
design, and belong to the `sema` stage.

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
| `COMPILATION_UNIT` | `NAMESPACE_DECLARATION`, `FILE_ATTRIBUTE*`, (`IMPORT_DECLARATION` or `REQUIRE_DECLARATION`)*, item* |
| `NAMESPACE_DECLARATION` | `QUALIFIED_NAME` |
| `IMPORT_DECLARATION` (flag HAS_ALIAS, HAS_MEMBERS or WILDCARD, never two of them) | `QUALIFIED_NAME` (the namespace, without the `.*` of a wildcard), then [`IDENTIFIER` alias] when HAS_ALIAS, or `IMPORT_MEMBER`+ when HAS_MEMBERS |
| `REQUIRE_DECLARATION` (same flags as an import) | the same children as `IMPORT_DECLARATION` |
| `IMPORT_MEMBER` (flag HAS_ALIAS) | `QUALIFIED_NAME` (the member path, one or two parts for the checker), [`IDENTIFIER` alias] |
| `QUALIFIED_NAME` | `IDENTIFIER`+ |
| `IDENTIFIER`, `INTEGER_LITERAL`, `FLOAT_LITERAL`, `STRING_LITERAL`, `CHARACTER_LITERAL`, `BOOLEAN_LITERAL` | none, they have text (a string or a character keeps its quotes and is not decoded) |
| `ATTRIBUTE`, `FILE_ATTRIBUTE` | (`ATTRIBUTE_ITEM` or `WHEN_ITEM`)+ |
| `ATTRIBUTE_ITEM` | `IDENTIFIER` key [`BOOLEAN_LITERAL`, `INTEGER_LITERAL`, `STRING_LITERAL` or `QUALIFIED_NAME`] |
| `WHEN_ITEM` | predicate (`WHEN_COMPARISON` or `WHEN_COMBINATOR`), `ATTRIBUTE_ITEM`+ (plain items only: a `when` never nests) |
| `WHEN_COMPARISON` | `IDENTIFIER` (the predicate name), `STRING_LITERAL` or `INTEGER_LITERAL` (the value) |
| `WHEN_COMBINATOR` | `IDENTIFIER` (`all`, `any` or `not`, as written), predicate+ (exactly one for `not`) |
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
| `DO_WHILE_STATEMENT` | `BLOCK` (the body, runs first), expr (the condition) |
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
| `MACRO_CALL` | `IDENTIFIER` (text is the macro name) or `QUALIFIED_NAME` (the macro of a library, `array.include_bytes!`), `ARGUMENT_LIST` |
| `ERROR_NODE` | none, placed at the bad token |

The flag values are MUTABLE 1, CONSTANT 4, HAS_ALIAS 8, FILE 16, HAS_MEMBERS 32 and WILDCARD 64. Bit 2
(REFERENCE) is retired: the parser never sets it any more. A `PARAMETER`'s own `#[semantics = ...]`
attribute, adopted last so the name, type and default keep their fixed positions (unlike every
other declaration, whose attributes lead), is the one source sema reads for its passing mode -
see [the sema stage doc](../sema/marea-sema.md) ("Ownership").
The word after the namespace path of an import or a require decides its form: `as` or `;` is the whole
path form, `{` is the member list, and `.` followed by `*` is the wildcard. The wildcard sets the flag
WILDCARD and adds no child: `import a.b.*;` is an `IMPORT_DECLARATION flags=64` with the `QUALIFIED_NAME`
`a.b` only. `require` builds `REQUIRE_DECLARATION` in the same places (the header of the file takes the
two words in any order); which names each word may reach is sema's rule. For example, `import a.b { f as g, T };` dumps as:

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
any type to the parser, a type child right after the name (`float32`, `string` and `boolean` parse, and a test pins
it); that it must be an integer primitive is sema's rule.
An attribute is read as a plain list: the parser never checks a key or a value, so `#[visibility = public]`,
`#[operator = "construct"]` (a `STRING_LITERAL` value) and `#[semantics = reference_mutable]` (a
`QUALIFIED_NAME` value) are the same shape as an unknown key.

A `when` item is `when(predicate, key = value, ...)`: a predicate, then the plain items that apply when it
holds. The parser builds three kinds and no kind per predicate name: a `WHEN_COMPARISON` holds the name as an
`IDENTIFIER` and the value, and a `WHEN_COMBINATOR` holds `all`, `any` or `not` and its predicates, so a new
predicate name needs no change of the parser. Sema owns the names (`architecture`, `operating_system`,
`feature`, `compiler`, `pointer_width`, `endianness`), the values, the target description and the meaning of
a predicate that the target does not answer. For example,
`#[when(not(feature = "sse2"), backend = scalar)] function f() end` dumps as:

```
  FUNCTION_DECLARATION
    ATTRIBUTE
      WHEN_ITEM
        WHEN_COMBINATOR
          IDENTIFIER "not"
          WHEN_COMPARISON
            IDENTIFIER "feature"
            STRING_LITERAL "\"sse2\""
        ATTRIBUTE_ITEM
          IDENTIFIER "backend"
          QUALIFIED_NAME
            IDENTIFIER "scalar"
    IDENTIFIER "f"
    PARAMETER_LIST
    BLOCK
```

A `do` body that ends at `end` is a `SCOPE_BLOCK`; one that ends at `while`, a condition and `end` is a
`DO_WHILE_STATEMENT` with the body `BLOCK` first and the condition second (see "Lookahead rules").

`reference` is not a reserved word any more (it lexes as an `IDENTIFIER`), and `mutable` is read only by
`for each` and `partition`, never on a parameter.

A bare name in an expression is an `IDENTIFIER` node. `or`, `and`, comparison, bit, shift, additive,
multiplicative operators and `**` are all `BINARY_EXPRESSION` with the token kind as operator.

Creating a value has two spellings and the parser tells them apart by the next token only. `Point{ x = 1.0 }`
is a `LITERAL_VALUE`. `Point(1.0)` is an ordinary `CALL_EXPRESSION` whose callee is an `IDENTIFIER` (or a
`MEMBER_ACCESS` chain for a qualified type name): the parser does not know the name is a type, so deciding
that it is a construction call is sema's job. `MAREAParserExpressions` (`CallOnATypeNameBuildsAPlainCall`)
pins the shape, which is the same one `f(1)` has.

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
  type tokens are allowed: names, `.`, `,`, integers, `function`, `:` and balanced parentheses. An
  attribute is also allowed, but only inside the parentheses of a function type: `#[`, `]`, `=`, a string
  and `true` or `false` count as type tokens between a `#[` and its `]`, and nowhere else. So
  `f<function(#[semantics = reference_mutable] a: T)>(x)` is a generic call. A `>>`
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
- **`do`.** After `do` and its body, `end` closes a `SCOPE_BLOCK` and `while` starts the test of a
  `DO_WHILE_STATEMENT`. A `while` that is a direct statement of the `do` body can also start an ordinary
  `while`, so the parser reads `while` and the condition and then looks at the next token: `end` or the end
  of the file means the test of the enclosing do-while, anything else an ordinary `while`, which needs its
  `do` (a missing one is reported as for any `while`). An ordinary `while` cannot be followed by `end` right
  after its condition, so there is no backtracking and the condition is parsed once. A `while` inside
  another statement of the body (an `if`, a nested `while`) is always an ordinary one. Nesting is safe:
  `do while a do end while b end` is an ordinary `while` inside a do-while.
- **`when`.** The identifier `when` followed by `(` inside an attribute list starts a `WHEN_ITEM`. A `when`
  with no `(` is a plain key. A predicate is a comparison (`name = "text"` or `name = 12`) or `all(...)`,
  `any(...)` or `not(...)` of predicates, told apart by the name and the `(` after it; the combinators nest
  within the depth limit.
- **Import and require.** `import` and `require` are keywords; a `.` followed by `*` after the path is the
  wildcard, and nothing but `;` may follow it.
- **`for`.** `for each` starts a `FOR_EACH_STATEMENT`, any other `for` a `FOR_RANGE_STATEMENT`.
- **Macro name.** `name!(` is a macro call. The word `namespace` may also name a macro
  (`namespace!()`), the one keyword exception that the grammar lists in `macro_call`. Names joined by `.` before the
  `!` (`array.include_bytes!(...)`, `marea.array.include_bytes!(...)`) are one `QUALIFIED_NAME` in the call; a single
  name stays an `IDENTIFIER`. A macro statement at the top of a file is still a single name only.

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
- A wildcard import or require followed by `as` or `{` is reported once (`EXPECTED_TOKEN`) and the rest of
  the declaration is skipped like any other bad top level token. A star with no path before it
  (`import *;`), a star that is not last (`require c.*.math;`) and a missing `;` after the star are
  `EXPECTED_TOKEN` too. A `require` after a declaration is reported like an `import` there
  (`EXPECTED_DECLARATION`) and is still read as a `REQUIRE_DECLARATION`.
- A do-while that reaches the end of the file after its condition gives one `UNTERMINATED_BLOCK` and is
  still a `DO_WHILE_STATEMENT`. A missing condition (`do ... while end`) gives `EXPECTED_EXPRESSION` and an
  `ERROR_NODE` for the condition. A `do` body that stops at a declaration word is reported and keeps the next
  declaration.
- A `when` item with no predicate, with no attribute after the predicate, with a predicate name and no
  `=`, with a value that is not a string or an integer, or with no `)` is reported at the bad token and the
  attribute list goes on: the `)` and the `]` are still read, so the declaration after the attribute parses.
  A `when` inside a `when` has no meaning to the parser (the word is a plain key there), so it ends in the
  error of the missing `)` or `]`.
- A block stops at `elseif`, `else`, `case`, `default` or `end`, at the end of the file, or at the start
  of a top level declaration, so an unfinished body never hides the next function. An `if` and a
  `switch` close with exactly one `end` for the whole statement. A branch of an `if` (the `if` branch,
  each `elseif` and the `else`) and an arm of a `switch` (each `case` and the `default`) has no `end` of
  its own, and its block stops without a diagnostic at the next branch or arm keyword or at that
  closing `end`. The old form, with an `end` after every branch or arm and one more for the statement,
  is reported: the `end` of a branch closes the statement, so the next `elseif`, `else`, `case` or
  `default` is a stray token and the last `end` is a stray token or an unterminated block.
- Nesting is limited to 256 levels of expression, statement, type and `when` predicate. Deeper input gives
  `RECURSION_TOO_DEEP` and that construct is left unparsed.

## Diagnostics

All are errors, from 2001 up. A diagnostic is not repeated at the same byte offset twice in a row.

| Code | Name | Example |
|---|---|---|
| 2001 | `EXPECTED_TOKEN` | `namespace main` (no `;`), `import a { };` (empty member list), `import a { f, };`, `import a { f as };`, `import a as b { f };`, `import a.* as x;`, `import *;`, `require c.*.math;`, a `when` with no attribute after the predicate |
| 2002 | `EXPECTED_EXPRESSION` | `constant x: sinteger32 = 1 +;`, a `do ... while end` with no condition |
| 2003 | `EXPECTED_TYPE` | `constant x: = 1;` |
| 2004 | `EXPECTED_DECLARATION` | `namespace main; 42;`, an import or a require after a declaration |
| 2005 | `UNEXPECTED_TOKEN` | a stray `end`, `case` outside a `switch`, an `else` after a statement that an earlier `end` already closed (the old per-branch form), or a default value in a function type parameter (`function(a: sinteger32 = 0)`, the tree is still built) |
| 2006 | `CHAINED_COMPARISON` | `a < b < c` (the tree is still built) |
| 2007 | `MISSING_NAMESPACE` | a file that does not start with `namespace` |
| 2008 | `RECURSION_TOO_DEEP` | 300 nested parentheses, `if` blocks, type arguments, `do` blocks or `not(` predicates |
| 2009 | `UNTERMINATED_BLOCK` | a structure, function, do-while or other block that reaches the end of the file |
| 2010 | `DEFAULT_ARM_NOT_LAST` | `switch a default then x = 1; case 2 then y = 2; end` (the tree is still built) |

## Gaps

- The `sema` stage does not know the new nodes yet, so they are accepted without a check:
  a `REQUIRE_DECLARATION` is skipped (a `require` of a name that nothing registered raises nothing);
  an `import` with the WILDCARD flag is checked as a plain `import` of the same path (the flag is
  ignored); a `DO_WHILE_STATEMENT` is not a loop for sema (its condition is not checked and not required
  to be a boolean, a `break` or `continue` in its body is `JUMP_OUTSIDE_LOOP`, and a `return` in its body
  does not complete a function); a `WHEN_ITEM` is skipped by the attribute check (no predicate name, value
  or conflict is checked). Later changes of `sema` close each of these.
- The `ir` stage does not lower a `DO_WHILE_STATEMENT`: in a test, a function whose body holds one was left
  out of the module with no diagnostic in the sink. A later change of `ir` lowers it to the loop that the
  IR already has.
- A `when` item cannot hold another `when` item: the listed items are plain, and the word `when` inside one
  is a plain key that fails at the next token, with a generic message.
- An `IMPORT_MEMBER` path may have any number of parts in the tree. Only one part (a function, a type or a
  constant) or two parts (an enum variant) are valid, and sema enforces it (`MAREA_G_MemberResolves` in
  `MAREASemaNamespace.c`). The rule that every member of a list needs an alias is decided but not in sema
  yet; the parser accepts a member with no alias.
