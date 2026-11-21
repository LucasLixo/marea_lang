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
    "KEYWORD_REFERENCE",
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
    { "and", 3, MAREA_TOKEN_KIND_KEYWORD_AND },
    { "as", 2, MAREA_TOKEN_KIND_KEYWORD_AS },
    { "break", 5, MAREA_TOKEN_KIND_KEYWORD_BREAK },
    { "case", 4, MAREA_TOKEN_KIND_KEYWORD_CASE },
    { "constant", 8, MAREA_TOKEN_KIND_KEYWORD_CONSTANT },
    { "continue", 8, MAREA_TOKEN_KIND_KEYWORD_CONTINUE },
    { "contract", 8, MAREA_TOKEN_KIND_KEYWORD_CONTRACT },
    { "copy", 4, MAREA_TOKEN_KIND_KEYWORD_COPY },
    { "default", 7, MAREA_TOKEN_KIND_KEYWORD_DEFAULT },
    { "do", 2, MAREA_TOKEN_KIND_KEYWORD_DO },
    { "each", 4, MAREA_TOKEN_KIND_KEYWORD_EACH },
    { "else", 4, MAREA_TOKEN_KIND_KEYWORD_ELSE },
    { "elseif", 6, MAREA_TOKEN_KIND_KEYWORD_ELSEIF },
    { "end", 3, MAREA_TOKEN_KIND_KEYWORD_END },
    { "enum", 4, MAREA_TOKEN_KIND_KEYWORD_ENUM },
    { "false", 5, MAREA_TOKEN_KIND_KEYWORD_FALSE },
    { "for", 3, MAREA_TOKEN_KIND_KEYWORD_FOR },
    { "fork_join", 9, MAREA_TOKEN_KIND_KEYWORD_FORK_JOIN },
    { "function", 8, MAREA_TOKEN_KIND_KEYWORD_FUNCTION },
    { "if", 2, MAREA_TOKEN_KIND_KEYWORD_IF },
    { "implementation", 14, MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION },
    { "import", 6, MAREA_TOKEN_KIND_KEYWORD_IMPORT },
    { "in", 2, MAREA_TOKEN_KIND_KEYWORD_IN },
    { "into", 4, MAREA_TOKEN_KIND_KEYWORD_INTO },
    { "local", 5, MAREA_TOKEN_KIND_KEYWORD_LOCAL },
    { "move", 4, MAREA_TOKEN_KIND_KEYWORD_MOVE },
    { "mutable", 7, MAREA_TOKEN_KIND_KEYWORD_MUTABLE },
    { "namespace", 9, MAREA_TOKEN_KIND_KEYWORD_NAMESPACE },
    { "not", 3, MAREA_TOKEN_KIND_KEYWORD_NOT },
    { "opaque", 6, MAREA_TOKEN_KIND_KEYWORD_OPAQUE },
    { "or", 2, MAREA_TOKEN_KIND_KEYWORD_OR },
    { "partition", 9, MAREA_TOKEN_KIND_KEYWORD_PARTITION },
    { "reference", 9, MAREA_TOKEN_KIND_KEYWORD_REFERENCE },
    { "return", 6, MAREA_TOKEN_KIND_KEYWORD_RETURN },
    { "spawn", 5, MAREA_TOKEN_KIND_KEYWORD_SPAWN },
    { "structure", 9, MAREA_TOKEN_KIND_KEYWORD_STRUCTURE },
    { "switch", 6, MAREA_TOKEN_KIND_KEYWORD_SWITCH },
    { "then", 4, MAREA_TOKEN_KIND_KEYWORD_THEN },
    { "true", 4, MAREA_TOKEN_KIND_KEYWORD_TRUE },
    { "type", 4, MAREA_TOKEN_KIND_KEYWORD_TYPE },
    { "unsafe", 6, MAREA_TOKEN_KIND_KEYWORD_UNSAFE },
    { "where", 5, MAREA_TOKEN_KIND_KEYWORD_WHERE },
    { "while", 5, MAREA_TOKEN_KIND_KEYWORD_WHILE },
    { "with", 4, MAREA_TOKEN_KIND_KEYWORD_WITH },
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
