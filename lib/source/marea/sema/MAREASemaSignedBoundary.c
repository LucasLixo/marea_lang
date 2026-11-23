// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: plain + - * wrap in two's complement, so only the two shapes below have no defined result.
#define MAREA_G_FOLD_DEPTH_LIMIT 16

typedef struct MAREA_G_Folded
{
    MAREA_Boolean known;
    MAREA_Boolean negative;
    MAREA_Uinteger64 magnitude;
} MAREA_G_Folded;

static MAREA_Boolean MAREA_CALL MAREA_G_StartsWith(const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_Character8 *prefix)
{
    const size_t PREFIX_LENGTH = strlen(prefix);

    return ((length >= PREFIX_LENGTH) && (memcmp(name, prefix, PREFIX_LENGTH) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_NameIs(const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return ((length == strlen(expected)) && (memcmp(name, expected, length) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: only a plain decimal literal is folded, with underscores allowed. Any other literal, or one that does not fit 64 bits, is not known.
static MAREA_G_Folded MAREA_CALL MAREA_G_FoldLiteral(const MAREA_AstNode *node)
{
    MAREA_G_Folded folded = { .known = MAREA_BOOLEAN_FALSE, .negative = MAREA_BOOLEAN_FALSE, .magnitude = 0 };

    const MAREA_Character8 *text = MAREA_AstNodeGetText(node);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(node);

    if ((text == MAREA_NULLPTR) || (LENGTH == 0)) {
        return folded;
    }

    MAREA_Uinteger64 value = 0;

    for (MAREA_IntegerSize index = 0; index < LENGTH; ++index) {
        if (text[index] == '_') {
            continue;
        }

        if ((text[index] < '0') || (text[index] > '9')) {
            return folded;
        }

        const MAREA_Uinteger64 DIGIT = (MAREA_Uinteger64)(text[index] - '0');

        if (value > ((UINT64_MAX - DIGIT) / 10U)) {
            return folded;
        }

        value = ((value * 10U) + DIGIT);
    }

    folded.known = MAREA_BOOLEAN_TRUE;
    folded.magnitude = value;

    return folded;
}

// INFO: one leading unary minus and parentheses around an integer literal.
static MAREA_G_Folded MAREA_CALL MAREA_G_Fold(const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    const MAREA_G_Folded UNKNOWN = { .known = MAREA_BOOLEAN_FALSE, .negative = MAREA_BOOLEAN_FALSE, .magnitude = 0 };

    if ((node == MAREA_NULLPTR) || (depth >= MAREA_G_FOLD_DEPTH_LIMIT)) {
        return UNKNOWN;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
        return MAREA_G_FoldLiteral(node);
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        return MAREA_G_Fold(MAREA_SemaChildAt(node, 0), (depth + 1));
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    {
        if (MAREA_AstNodeGetOperator(node) != MAREA_TOKEN_KIND_MINUS) {
            return UNKNOWN;
        }

        const MAREA_AstNode *operand = MAREA_SemaChildAt(node, 0);

        if ((operand != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(operand) == MAREA_AST_KIND_UNARY_EXPRESSION)) {
            return UNKNOWN;
        }

        MAREA_G_Folded folded = MAREA_G_Fold(operand, (depth + 1));

        folded.negative = (folded.known == MAREA_BOOLEAN_TRUE) ? ((folded.negative == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE) : MAREA_BOOLEAN_FALSE;

        return folded;
    }
    default:
        return UNKNOWN;
    }
}

// INFO: the magnitude of the lowest value of a signed primitive, or 0 when the name is not a signed integer type.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_LowestMagnitude(const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    if (MAREA_G_NameIs(name, length, "sinteger8") == MAREA_BOOLEAN_TRUE) {
        return 128U;
    }

    if (MAREA_G_NameIs(name, length, "sinteger16") == MAREA_BOOLEAN_TRUE) {
        return 32768U;
    }

    if (MAREA_G_NameIs(name, length, "sinteger32") == MAREA_BOOLEAN_TRUE) {
        return 2147483648U;
    }

    if (MAREA_G_NameIs(name, length, "sinteger64") == MAREA_BOOLEAN_TRUE) {
        return 9223372036854775808U;
    }

    return 0;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNegation(MAREA_SemaContext *context, const MAREA_AstNode *unary)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((unary == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(unary) != MAREA_AST_KIND_UNARY_EXPRESSION) || (MAREA_AstNodeGetOperator(unary) != MAREA_TOKEN_KIND_MINUS)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((MAREA_SemaPrimitiveOfExpression(context, MAREA_SemaChildAt(unary, 0), &name, &length) == MAREA_BOOLEAN_FALSE) || (MAREA_G_StartsWith(name, length, "uinteger") == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NEGATE_UNSIGNED, MAREA_AstNodeGetPosition(unary), "negation of an unsigned value has no defined result, the type is", name, length);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckSignedDivision(MAREA_SemaContext *context, const MAREA_AstNode *targetType, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *division = expression;

    while ((division != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(division) == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION)) {
        division = MAREA_SemaChildAt(division, 0);
    }

    if ((division == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(division) != MAREA_AST_KIND_BINARY_EXPRESSION)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(division);

    if ((OPERATOR != MAREA_TOKEN_KIND_SLASH) && (OPERATOR != MAREA_TOKEN_KIND_PERCENT)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *leftNode = MAREA_SemaChildAt(division, 0);

    const MAREA_AstNode *rightNode = MAREA_SemaChildAt(division, 1);

    const MAREA_G_Folded LEFT = MAREA_G_Fold(leftNode, 0);

    const MAREA_G_Folded RIGHT = MAREA_G_Fold(rightNode, 0);

    if ((LEFT.known == MAREA_BOOLEAN_FALSE) || (RIGHT.known == MAREA_BOOLEAN_FALSE) || (LEFT.negative == MAREA_BOOLEAN_FALSE) || (RIGHT.negative == MAREA_BOOLEAN_FALSE) || (RIGHT.magnitude != 1U)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((MAREA_SemaPrimitiveOfExpression(context, leftNode, &name, &length) == MAREA_BOOLEAN_FALSE) && (MAREA_SemaPrimitiveOfExpression(context, rightNode, &name, &length) == MAREA_BOOLEAN_FALSE) && (MAREA_SemaPrimitiveOfType(context, targetType, &name, &length) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger64 LOWEST = MAREA_G_LowestMagnitude(name, length);

    if ((LOWEST == 0) || (LEFT.magnitude != LOWEST)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_SIGNED_DIVISION_OVERFLOW, MAREA_AstNodeGetPosition(division), "the lowest value divided by -1 has no defined result, the type is", name, length);
}
