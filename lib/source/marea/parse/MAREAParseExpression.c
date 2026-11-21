// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

MAREA_AstNode *MAREA_CALL MAREA_ParseExpression(MAREA_ParseState *state)
{
    switch (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0))) {
    case MAREA_TOKEN_KIND_INTEGER_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_INTEGER_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_FLOAT_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_FLOAT_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_STRING_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_STRING_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_KEYWORD_TRUE:
    case MAREA_TOKEN_KIND_KEYWORD_FALSE:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_BOOLEAN_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_IDENTIFIER:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_IDENTIFIER, MAREA_ParseAdvance(state));
    default:
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected an expression");

        return MAREA_ParseErrorNode(state);
    }
}
