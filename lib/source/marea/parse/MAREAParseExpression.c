// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

#if !defined(MAREA_PARSE_SCAN_LIMIT)
    #define MAREA_PARSE_SCAN_LIMIT MAREA_INTEGER_SIZE(1024)
#endif

#define MAREA_G_LEVEL_NONE       MAREA_UINTEGER_32(0)
#define MAREA_G_LEVEL_OR         MAREA_UINTEGER_32(1)
#define MAREA_G_LEVEL_AND        MAREA_UINTEGER_32(2)
#define MAREA_G_LEVEL_COMPARISON MAREA_UINTEGER_32(3)
#define MAREA_G_LEVEL_BIT_OR     MAREA_UINTEGER_32(4)
#define MAREA_G_LEVEL_BIT_XOR    MAREA_UINTEGER_32(5)
#define MAREA_G_LEVEL_BIT_AND    MAREA_UINTEGER_32(6)
#define MAREA_G_LEVEL_SHIFT      MAREA_UINTEGER_32(7)
#define MAREA_G_LEVEL_ADDITIVE   MAREA_UINTEGER_32(8)
#define MAREA_G_LEVEL_MULTIPLY   MAREA_UINTEGER_32(9)

static MAREA_Uinteger32 MAREA_CALL MAREA_G_BinaryLevel(MAREA_TokenKind kind)
{
    switch (kind) {
    case MAREA_TOKEN_KIND_KEYWORD_OR:
        return MAREA_G_LEVEL_OR;
    case MAREA_TOKEN_KIND_KEYWORD_AND:
        return MAREA_G_LEVEL_AND;
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
    case MAREA_TOKEN_KIND_BANG_EQUAL:
    case MAREA_TOKEN_KIND_LESS:
    case MAREA_TOKEN_KIND_LESS_EQUAL:
    case MAREA_TOKEN_KIND_GREATER:
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return MAREA_G_LEVEL_COMPARISON;
    case MAREA_TOKEN_KIND_PIPE:
        return MAREA_G_LEVEL_BIT_OR;
    case MAREA_TOKEN_KIND_CARET:
        return MAREA_G_LEVEL_BIT_XOR;
    case MAREA_TOKEN_KIND_AMPERSAND:
        return MAREA_G_LEVEL_BIT_AND;
    case MAREA_TOKEN_KIND_SHIFT_LEFT:
    case MAREA_TOKEN_KIND_SHIFT_RIGHT:
        return MAREA_G_LEVEL_SHIFT;
    case MAREA_TOKEN_KIND_PLUS:
    case MAREA_TOKEN_KIND_MINUS:
    case MAREA_TOKEN_KIND_PLUS_PERCENT:
    case MAREA_TOKEN_KIND_MINUS_PERCENT:
    case MAREA_TOKEN_KIND_PLUS_PIPE:
    case MAREA_TOKEN_KIND_MINUS_PIPE:
        return MAREA_G_LEVEL_ADDITIVE;
    case MAREA_TOKEN_KIND_STAR:
    case MAREA_TOKEN_KIND_SLASH:
    case MAREA_TOKEN_KIND_PERCENT:
    case MAREA_TOKEN_KIND_STAR_PERCENT:
    case MAREA_TOKEN_KIND_STAR_PIPE:
        return MAREA_G_LEVEL_MULTIPLY;
    default:
        return MAREA_G_LEVEL_NONE;
    }
}

// INFO: Token level scan, no allocation. The less than at start opens type arguments only if a balanced closing angle bracket follows with only type tokens between. An attribute is a type token only inside a function type, where a parameter may carry one.
static MAREA_IntegerSize MAREA_CALL MAREA_G_ScanTypeArguments(const MAREA_ParseState *state, MAREA_IntegerSize start)
{
    MAREA_Sinteger32 angleDepth = 0;
    MAREA_Sinteger32 parenDepth = 0;
    MAREA_Boolean inAttribute = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize ahead = start; ahead < (start + MAREA_PARSE_SCAN_LIMIT); ahead = (ahead + MAREA_INTEGER_SIZE(1))) {
        switch (MAREA_ParsePeekKind(state, ahead)) {
        case MAREA_TOKEN_KIND_LESS:
            angleDepth = (angleDepth + 1);
            break;
        case MAREA_TOKEN_KIND_GREATER:
            angleDepth = (angleDepth - 1);

            if (angleDepth == 0) {
                return (ahead + MAREA_INTEGER_SIZE(1));
            }

            break;
        case MAREA_TOKEN_KIND_SHIFT_RIGHT:
            if (angleDepth < 2) {
                return MAREA_INTEGER_SIZE(0);
            }

            angleDepth = (angleDepth - 2);

            if (angleDepth == 0) {
                return (ahead + MAREA_INTEGER_SIZE(1));
            }

            break;
        case MAREA_TOKEN_KIND_LEFT_PAREN:
            parenDepth = (parenDepth + 1);
            break;
        case MAREA_TOKEN_KIND_RIGHT_PAREN:
            if (parenDepth == 0) {
                return MAREA_INTEGER_SIZE(0);
            }

            parenDepth = (parenDepth - 1);
            break;
        case MAREA_TOKEN_KIND_HASH_BRACKET:
            if ((parenDepth == 0) || (inAttribute == MAREA_BOOLEAN_TRUE)) {
                return MAREA_INTEGER_SIZE(0);
            }

            inAttribute = MAREA_BOOLEAN_TRUE;
            break;
        case MAREA_TOKEN_KIND_RIGHT_BRACKET:
            if (inAttribute == MAREA_BOOLEAN_FALSE) {
                return MAREA_INTEGER_SIZE(0);
            }

            inAttribute = MAREA_BOOLEAN_FALSE;
            break;
        case MAREA_TOKEN_KIND_EQUAL:
        case MAREA_TOKEN_KIND_STRING_LITERAL:
        case MAREA_TOKEN_KIND_KEYWORD_TRUE:
        case MAREA_TOKEN_KIND_KEYWORD_FALSE:
            if (inAttribute == MAREA_BOOLEAN_FALSE) {
                return MAREA_INTEGER_SIZE(0);
            }

            break;
        case MAREA_TOKEN_KIND_IDENTIFIER:
        case MAREA_TOKEN_KIND_DOT:
        case MAREA_TOKEN_KIND_COMMA:
        case MAREA_TOKEN_KIND_INTEGER_LITERAL:
        case MAREA_TOKEN_KIND_KEYWORD_FUNCTION:
        case MAREA_TOKEN_KIND_COLON:
            break;
        default:
            return MAREA_INTEGER_SIZE(0);
        }
    }

    return MAREA_INTEGER_SIZE(0);
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseArgumentList(MAREA_ParseState *state)
{
    MAREA_AstNode *list = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_ARGUMENT_LIST);

    if (MAREA_ParseExpect(state, MAREA_TOKEN_KIND_LEFT_PAREN, "expected '(' before the arguments") == MAREA_BOOLEAN_FALSE) {
        return list;
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_RIGHT_PAREN) == MAREA_BOOLEAN_FALSE) {
        do {
            const MAREA_IntegerSize before = state->index;

            MAREA_ParseNodeAdd(state, list, MAREA_ParseExpression(state));

            if (state->index == before) {
                break;
            }
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_PAREN, "expected ')' after the arguments"));

    MAREA_ParseNodeEnd(state, list);

    return list;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseMacroCall(MAREA_ParseState *state)
{
    MAREA_AstNode *call = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_MACRO_CALL);

    // INFO: The name may be the word namespace, as in namespace!(), so it is built from the token directly.
    MAREA_ParseNodeAdd(state, call, MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_IDENTIFIER, MAREA_ParseAdvance(state)));

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, call, MAREA_G_ParseArgumentList(state));

    MAREA_ParseNodeEnd(state, call);

    return call;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseLiteralEntry(MAREA_ParseState *state)
{
    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LEFT_BRACKET) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNode *entry = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_INDEX_ENTRY);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, entry, MAREA_ParseExpression(state));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_BRACKET, "expected ']' after the index"));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_EQUAL, "expected '=' after the index"));

        MAREA_ParseNodeAdd(state, entry, MAREA_ParseExpression(state));

        MAREA_ParseNodeEnd(state, entry);

        return entry;
    }

    if ((MAREA_ParseAt(state, MAREA_TOKEN_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_EQUAL)) {
        MAREA_AstNode *entry = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_FIELD_ENTRY);

        MAREA_ParseNodeAdd(state, entry, MAREA_ParseIdentifier(state, "expected a field name"));

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, entry, MAREA_ParseExpression(state));

        MAREA_ParseNodeEnd(state, entry);

        return entry;
    }

    MAREA_AstNode *entry = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_VALUE_ENTRY);

    MAREA_ParseNodeAdd(state, entry, MAREA_ParseExpression(state));

    MAREA_ParseNodeEnd(state, entry);

    return entry;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseLiteralValue(MAREA_ParseState *state)
{
    MAREA_AstNode *literal = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_LITERAL_VALUE);

    MAREA_ParseNodeAdd(state, literal, MAREA_ParseNamedType(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_LEFT_BRACE, "expected '{' before the literal entries"));

    for (;;) {
        if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_RIGHT_BRACE) == MAREA_BOOLEAN_TRUE) {
            break;
        }

        const MAREA_IntegerSize before = state->index;

        MAREA_ParseNodeAdd(state, literal, MAREA_G_ParseLiteralEntry(state));

        if ((state->index == before) || (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_FALSE)) {
            break;
        }
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_BRACE, "expected '}' after the literal entries"));

    MAREA_ParseNodeEnd(state, literal);

    return literal;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseGenericName(MAREA_ParseState *state)
{
    MAREA_AstNode *name = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_GENERIC_NAME);

    MAREA_ParseNodeAdd(state, name, MAREA_ParseQualifiedName(state));

    MAREA_ParseNodeAdd(state, name, MAREA_ParseTypeArguments(state));

    MAREA_ParseNodeEnd(state, name);

    return name;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseIdentifierPrimary(MAREA_ParseState *state)
{
    if ((MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_BANG) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(2)) == MAREA_TOKEN_KIND_LEFT_PAREN)) {
        return MAREA_G_ParseMacroCall(state);
    }

    MAREA_IntegerSize last = MAREA_INTEGER_SIZE(0);

    while ((MAREA_ParsePeekKind(state, (last + MAREA_INTEGER_SIZE(1))) == MAREA_TOKEN_KIND_DOT) && (MAREA_ParsePeekKind(state, (last + MAREA_INTEGER_SIZE(2))) == MAREA_TOKEN_KIND_IDENTIFIER)) {
        last = (last + MAREA_INTEGER_SIZE(2));
    }

    const MAREA_IntegerSize after = (last + MAREA_INTEGER_SIZE(1));

    const MAREA_TokenKind afterKind = MAREA_ParsePeekKind(state, after);

    if (afterKind == MAREA_TOKEN_KIND_LEFT_BRACE) {
        return MAREA_G_ParseLiteralValue(state);
    }

    if (afterKind == MAREA_TOKEN_KIND_LESS) {
        const MAREA_IntegerSize end = MAREA_G_ScanTypeArguments(state, after);

        if (end != MAREA_INTEGER_SIZE(0)) {
            const MAREA_TokenKind endKind = MAREA_ParsePeekKind(state, end);

            if (endKind == MAREA_TOKEN_KIND_LEFT_BRACE) {
                return MAREA_G_ParseLiteralValue(state);
            }

            if (endKind == MAREA_TOKEN_KIND_DOT) {
                return MAREA_G_ParseGenericName(state);
            }
        }
    }

    return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_IDENTIFIER, MAREA_ParseAdvance(state));
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParsePrimary(MAREA_ParseState *state)
{
    switch (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0))) {
    case MAREA_TOKEN_KIND_INTEGER_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_INTEGER_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_FLOAT_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_FLOAT_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_STRING_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_STRING_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_CHARACTER_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_CHARACTER_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_KEYWORD_TRUE:
    case MAREA_TOKEN_KIND_KEYWORD_FALSE:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_BOOLEAN_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_LEFT_PAREN:
    {
        MAREA_AstNode *group = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_PARENTHESIZED_EXPRESSION);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, group, MAREA_ParseExpression(state));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_PAREN, "expected ')' after the expression"));

        MAREA_ParseNodeEnd(state, group);

        return group;
    }
    case MAREA_TOKEN_KIND_KEYWORD_NAMESPACE:
        if ((MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_BANG) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(2)) == MAREA_TOKEN_KIND_LEFT_PAREN)) {
            return MAREA_G_ParseMacroCall(state);
        }

        break;
    case MAREA_TOKEN_KIND_IDENTIFIER:
        return MAREA_G_ParseIdentifierPrimary(state);
    default:
        break;
    }

    MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected an expression");

    return MAREA_ParseErrorNode(state);
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseCall(MAREA_ParseState *state, MAREA_AstNode *callee)
{
    MAREA_AstNode *call = MAREA_ParseNodeBeginAt(state, MAREA_AST_KIND_CALL_EXPRESSION, MAREA_AstNodeGetPosition(callee));

    MAREA_ParseNodeAdd(state, call, callee);

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LESS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, call, MAREA_ParseTypeArguments(state));
    }

    MAREA_ParseNodeAdd(state, call, MAREA_G_ParseArgumentList(state));

    MAREA_ParseNodeEnd(state, call);

    return call;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParsePostfix(MAREA_ParseState *state)
{
    MAREA_AstNode *expression = MAREA_G_ParsePrimary(state);

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if (kind == MAREA_TOKEN_KIND_DOT) {
            MAREA_AstNode *access = MAREA_ParseNodeBeginAt(state, MAREA_AST_KIND_MEMBER_ACCESS, MAREA_AstNodeGetPosition(expression));

            MAREA_IGNORE(MAREA_ParseAdvance(state));

            MAREA_ParseNodeAdd(state, access, expression);

            MAREA_ParseNodeAdd(state, access, MAREA_ParseIdentifier(state, "expected a member name after '.'"));

            MAREA_ParseNodeEnd(state, access);

            expression = access;
        } else if (kind == MAREA_TOKEN_KIND_LEFT_BRACKET) {
            MAREA_AstNode *access = MAREA_ParseNodeBeginAt(state, MAREA_AST_KIND_INDEX_ACCESS, MAREA_AstNodeGetPosition(expression));

            MAREA_IGNORE(MAREA_ParseAdvance(state));

            MAREA_ParseNodeAdd(state, access, expression);

            MAREA_ParseNodeAdd(state, access, MAREA_ParseExpression(state));

            MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_BRACKET, "expected ']' after the index"));

            MAREA_ParseNodeEnd(state, access);

            expression = access;
        } else if (kind == MAREA_TOKEN_KIND_LEFT_PAREN) {
            expression = MAREA_G_ParseCall(state, expression);
        } else if (kind == MAREA_TOKEN_KIND_LESS) {
            const MAREA_IntegerSize end = MAREA_G_ScanTypeArguments(state, MAREA_INTEGER_SIZE(0));

            if ((end == MAREA_INTEGER_SIZE(0)) || (MAREA_ParsePeekKind(state, end) != MAREA_TOKEN_KIND_LEFT_PAREN)) {
                break;
            }

            expression = MAREA_G_ParseCall(state, expression);
        } else {
            break;
        }
    }

    return expression;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseUnary(MAREA_ParseState *state);

static MAREA_AstNode *MAREA_CALL MAREA_G_ParsePower(MAREA_ParseState *state)
{
    MAREA_AstNode *base = MAREA_G_ParsePostfix(state);

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_STAR_STAR) == MAREA_BOOLEAN_FALSE) {
        return base;
    }

    MAREA_AstNode *power = MAREA_ParseNodeBeginAt(state, MAREA_AST_KIND_BINARY_EXPRESSION, MAREA_AstNodeGetPosition(base));

    MAREA_AstNodeSetOperator(power, MAREA_TOKEN_KIND_STAR_STAR);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, power, base);

    MAREA_ParseNodeAdd(state, power, MAREA_G_ParseUnary(state));

    MAREA_ParseNodeEnd(state, power);

    return power;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseUnary(MAREA_ParseState *state)
{
    const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

    const MAREA_Boolean isPrefix = ((kind == MAREA_TOKEN_KIND_MINUS) || (kind == MAREA_TOKEN_KIND_TILDE) || (kind == MAREA_TOKEN_KIND_KEYWORD_NOT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_Boolean isOwnership = ((kind == MAREA_TOKEN_KIND_KEYWORD_COPY) || (kind == MAREA_TOKEN_KIND_KEYWORD_MOVE) || (kind == MAREA_TOKEN_KIND_KEYWORD_SPAWN)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((isPrefix == MAREA_BOOLEAN_FALSE) && (isOwnership == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_ParsePower(state);
    }

    if (MAREA_ParseEnter(state) == MAREA_BOOLEAN_FALSE) {
        return MAREA_ParseErrorNode(state);
    }

    MAREA_AstNode *unary = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_UNARY_EXPRESSION);

    MAREA_AstNodeSetOperator(unary, kind);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, unary, (isPrefix == MAREA_BOOLEAN_TRUE) ? MAREA_G_ParseUnary(state) : MAREA_G_ParsePostfix(state));

    MAREA_ParseNodeEnd(state, unary);

    MAREA_ParseLeave(state);

    return unary;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseBinary(MAREA_ParseState *state, MAREA_Uinteger32 minimumLevel)
{
    MAREA_AstNode *left = MAREA_G_ParseUnary(state);

    MAREA_Boolean sawComparison = MAREA_BOOLEAN_FALSE;

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        const MAREA_Uinteger32 level = MAREA_G_BinaryLevel(kind);

        if ((level == MAREA_G_LEVEL_NONE) || (level < minimumLevel)) {
            break;
        }

        if (level == MAREA_G_LEVEL_COMPARISON) {
            if (sawComparison == MAREA_BOOLEAN_TRUE) {
                MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_CHAINED_COMPARISON, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "a comparison cannot be chained");
            }

            sawComparison = MAREA_BOOLEAN_TRUE;
        }

        MAREA_AstNode *binary = MAREA_ParseNodeBeginAt(state, MAREA_AST_KIND_BINARY_EXPRESSION, MAREA_AstNodeGetPosition(left));

        MAREA_AstNodeSetOperator(binary, kind);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, binary, left);

        MAREA_ParseNodeAdd(state, binary, MAREA_G_ParseBinary(state, (level + MAREA_UINTEGER_32(1))));

        MAREA_ParseNodeEnd(state, binary);

        left = binary;
    }

    return left;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseExpression(MAREA_ParseState *state)
{
    if (MAREA_ParseEnter(state) == MAREA_BOOLEAN_FALSE) {
        return MAREA_ParseErrorNode(state);
    }

    MAREA_AstNode *expression = MAREA_G_ParseBinary(state, MAREA_G_LEVEL_OR);

    MAREA_ParseLeave(state);

    return expression;
}
