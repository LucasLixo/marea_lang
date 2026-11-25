// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/lex/MAREATokenKind.h"

#include <string.h>

static const MAREA_Character8 *const MAREA_G_TokenKindNames[] = {
    "END_OF_FILE",
    "ERROR",
    "IDENTIFIER",
    "INTEGER_LITERAL",
    "FLOAT_LITERAL",
    "STRING_LITERAL",
    "CHARACTER_LITERAL",

    "KEYWORD_AND",
    "KEYWORD_AS",
    "KEYWORD_BREAK",
    "KEYWORD_CASE",
    "KEYWORD_CONSTANT",
    "KEYWORD_CONTINUE",
    "KEYWORD_CONTRACT",
    "KEYWORD_COPY",
    "KEYWORD_DEFAULT",
    "KEYWORD_DO",
    "KEYWORD_EACH",
    "KEYWORD_ELSE",
    "KEYWORD_ELSEIF",
    "KEYWORD_END",
    "KEYWORD_ENUM",
    "KEYWORD_FALSE",
    "KEYWORD_FOR",
    "KEYWORD_FORK_JOIN",
    "KEYWORD_FUNCTION",
    "KEYWORD_IF",
    "KEYWORD_IMPLEMENTATION",
    "KEYWORD_IMPORT",
    "KEYWORD_IN",
    "KEYWORD_INTO",
    "KEYWORD_LOCAL",
    "KEYWORD_MOVE",
    "KEYWORD_MUTABLE",
    "KEYWORD_NAMESPACE",
    "KEYWORD_NOT",
    "KEYWORD_OPAQUE",
    "KEYWORD_OR",
    "KEYWORD_PARTITION",
    "KEYWORD_REQUIRE",
    "KEYWORD_RETURN",
    "KEYWORD_SPAWN",
    "KEYWORD_STRUCTURE",
    "KEYWORD_SWITCH",
    "KEYWORD_THEN",
    "KEYWORD_TRUE",
    "KEYWORD_TYPE",
    "KEYWORD_UNSAFE",
    "KEYWORD_WHERE",
    "KEYWORD_WHILE",
    "KEYWORD_WITH",

    "PLUS",
    "MINUS",
    "STAR",
    "SLASH",
    "PERCENT",
    "STAR_STAR",
    "PLUS_PERCENT",
    "MINUS_PERCENT",
    "STAR_PERCENT",
    "PLUS_PIPE",
    "MINUS_PIPE",
    "STAR_PIPE",
    "TILDE",
    "AMPERSAND",
    "PIPE",
    "CARET",
    "SHIFT_LEFT",
    "SHIFT_RIGHT",
    "EQUAL_EQUAL",
    "BANG_EQUAL",
    "LESS",
    "LESS_EQUAL",
    "GREATER",
    "GREATER_EQUAL",
    "EQUAL",
    "PLUS_EQUAL",
    "MINUS_EQUAL",
    "STAR_EQUAL",
    "SLASH_EQUAL",
    "PERCENT_EQUAL",
    "STAR_STAR_EQUAL",
    "SHIFT_LEFT_EQUAL",
    "SHIFT_RIGHT_EQUAL",
    "AMPERSAND_EQUAL",
    "PIPE_EQUAL",
    "CARET_EQUAL",

    "LEFT_PAREN",
    "RIGHT_PAREN",
    "LEFT_BRACKET",
    "RIGHT_BRACKET",
    "LEFT_BRACE",
    "RIGHT_BRACE",
    "COMMA",
    "SEMICOLON",
    "COLON",
    "DOT",
    "DOT_DOT",
    "BANG",
    "HASH_BRACKET",
    "HASH_BANG_BRACKET",
};

MAREA_STATIC_ASSERT((sizeof(MAREA_G_TokenKindNames) / sizeof(MAREA_G_TokenKindNames[0])) == MAREA_TOKEN_KIND_COUNT, "MAREA_G_TokenKindNames must have one entry per token kind.");

typedef struct MAREA_G_KeywordRow
{
    const MAREA_Character8 *text;
    MAREA_IntegerSize length;
    MAREA_TokenKind kind;
} MAREA_G_KeywordRow;

static const MAREA_G_KeywordRow MAREA_G_KeywordTable[] = {
    { .text = "and", .length = 3, .kind = MAREA_TOKEN_KIND_KEYWORD_AND },
    { .text = "as", .length = 2, .kind = MAREA_TOKEN_KIND_KEYWORD_AS },
    { .text = "break", .length = 5, .kind = MAREA_TOKEN_KIND_KEYWORD_BREAK },
    { .text = "case", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_CASE },
    { .text = "constant", .length = 8, .kind = MAREA_TOKEN_KIND_KEYWORD_CONSTANT },
    { .text = "continue", .length = 8, .kind = MAREA_TOKEN_KIND_KEYWORD_CONTINUE },
    { .text = "contract", .length = 8, .kind = MAREA_TOKEN_KIND_KEYWORD_CONTRACT },
    { .text = "copy", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_COPY },
    { .text = "default", .length = 7, .kind = MAREA_TOKEN_KIND_KEYWORD_DEFAULT },
    { .text = "do", .length = 2, .kind = MAREA_TOKEN_KIND_KEYWORD_DO },
    { .text = "each", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_EACH },
    { .text = "else", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_ELSE },
    { .text = "elseif", .length = 6, .kind = MAREA_TOKEN_KIND_KEYWORD_ELSEIF },
    { .text = "end", .length = 3, .kind = MAREA_TOKEN_KIND_KEYWORD_END },
    { .text = "enum", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_ENUM },
    { .text = "false", .length = 5, .kind = MAREA_TOKEN_KIND_KEYWORD_FALSE },
    { .text = "for", .length = 3, .kind = MAREA_TOKEN_KIND_KEYWORD_FOR },
    { .text = "fork_join", .length = 9, .kind = MAREA_TOKEN_KIND_KEYWORD_FORK_JOIN },
    { .text = "function", .length = 8, .kind = MAREA_TOKEN_KIND_KEYWORD_FUNCTION },
    { .text = "if", .length = 2, .kind = MAREA_TOKEN_KIND_KEYWORD_IF },
    { .text = "implementation", .length = 14, .kind = MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION },
    { .text = "import", .length = 6, .kind = MAREA_TOKEN_KIND_KEYWORD_IMPORT },
    { .text = "in", .length = 2, .kind = MAREA_TOKEN_KIND_KEYWORD_IN },
    { .text = "into", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_INTO },
    { .text = "local", .length = 5, .kind = MAREA_TOKEN_KIND_KEYWORD_LOCAL },
    { .text = "move", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_MOVE },
    { .text = "mutable", .length = 7, .kind = MAREA_TOKEN_KIND_KEYWORD_MUTABLE },
    { .text = "namespace", .length = 9, .kind = MAREA_TOKEN_KIND_KEYWORD_NAMESPACE },
    { .text = "not", .length = 3, .kind = MAREA_TOKEN_KIND_KEYWORD_NOT },
    { .text = "opaque", .length = 6, .kind = MAREA_TOKEN_KIND_KEYWORD_OPAQUE },
    { .text = "or", .length = 2, .kind = MAREA_TOKEN_KIND_KEYWORD_OR },
    { .text = "partition", .length = 9, .kind = MAREA_TOKEN_KIND_KEYWORD_PARTITION },
    { .text = "require", .length = 7, .kind = MAREA_TOKEN_KIND_KEYWORD_REQUIRE },
    { .text = "return", .length = 6, .kind = MAREA_TOKEN_KIND_KEYWORD_RETURN },
    { .text = "spawn", .length = 5, .kind = MAREA_TOKEN_KIND_KEYWORD_SPAWN },
    { .text = "structure", .length = 9, .kind = MAREA_TOKEN_KIND_KEYWORD_STRUCTURE },
    { .text = "switch", .length = 6, .kind = MAREA_TOKEN_KIND_KEYWORD_SWITCH },
    { .text = "then", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_THEN },
    { .text = "true", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_TRUE },
    { .text = "type", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_TYPE },
    { .text = "unsafe", .length = 6, .kind = MAREA_TOKEN_KIND_KEYWORD_UNSAFE },
    { .text = "where", .length = 5, .kind = MAREA_TOKEN_KIND_KEYWORD_WHERE },
    { .text = "while", .length = 5, .kind = MAREA_TOKEN_KIND_KEYWORD_WHILE },
    { .text = "with", .length = 4, .kind = MAREA_TOKEN_KIND_KEYWORD_WITH },
};

MAREA_STATIC_ASSERT((sizeof(MAREA_G_KeywordTable) / sizeof(MAREA_G_KeywordTable[0])) == 44, "MAREA_G_KeywordTable must have 44 rows.");

const MAREA_Character8 *MAREA_CALL MAREA_TokenKindGetName(MAREA_TokenKind kind)
{
    if (((MAREA_IntegerSize)kind) >= (MAREA_IntegerSize)(sizeof(MAREA_G_TokenKindNames) / sizeof(MAREA_G_TokenKindNames[0]))) {
        return "UNKNOWN";
    }

    return MAREA_G_TokenKindNames[kind];
}

MAREA_TokenKind MAREA_CALL MAREA_TokenKindLookupKeyword(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < (sizeof(MAREA_G_KeywordTable) / sizeof(MAREA_G_KeywordTable[0])); index = index + MAREA_INTEGER_SIZE(1)) {
        if ((MAREA_G_KeywordTable[index].length == length) && (memcmp(MAREA_G_KeywordTable[index].text, text, length) == 0)) {
            return MAREA_G_KeywordTable[index].kind;
        }
    }

    return MAREA_TOKEN_KIND_IDENTIFIER;
}
