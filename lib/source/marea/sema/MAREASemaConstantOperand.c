// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_FOLD_DEPTH_LIMIT 64U

#define MAREA_G_PREFIX_LENGTH 8

typedef struct
{
    MAREA_Boolean negative;
    MAREA_Uinteger64 magnitude;
} MAREA_G_Folded;

static MAREA_Boolean MAREA_CALL MAREA_G_DigitValue(MAREA_Character8 character, MAREA_Uinteger32 base, MAREA_Uinteger32 *outValue)
{
    MAREA_Uinteger32 value = 99U;

    if ((character >= '0') && (character <= '9')) {
        value = (MAREA_Uinteger32)(character - '0');
    } else if ((character >= 'a') && (character <= 'f')) {
        value = (MAREA_Uinteger32)(character - 'a') + 10U;
    } else if ((character >= 'A') && (character <= 'F')) {
        value = (MAREA_Uinteger32)(character - 'A') + 10U;
    }

    *outValue = value;

    return (value < base) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: folds the text of an INTEGER_LITERAL (decimal, 0x, 0b, "_" digits). A value that does not fit 64 bits is not constant.
static MAREA_Boolean MAREA_CALL MAREA_G_FoldLiteral(const MAREA_AstNode *node, MAREA_Uinteger64 *outMagnitude)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(node);

    MAREA_Uinteger32 length = MAREA_AstNodeGetTextLength(node);

    MAREA_Uinteger32 base = 10U;

    MAREA_Uinteger32 index = 0U;

    if ((text == MAREA_NULLPTR) || (length == 0U)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((length > 2U) && (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'b'))) {
        base = (text[1] == 'x') ? 16U : 2U;

        index = 2U;
    }

    MAREA_Uinteger64 magnitude = 0U;

    for (; index < length; index += 1U) {
        MAREA_Uinteger32 digit = 0U;

        if (text[index] == '_') {
            continue;
        }

        if (MAREA_G_DigitValue(text[index], base, &digit) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (magnitude > ((UINT64_MAX - digit) / base)) {
            return MAREA_BOOLEAN_FALSE;
        }

        magnitude = ((magnitude * base) + digit);
    }

    *outMagnitude = magnitude;

    return MAREA_BOOLEAN_TRUE;
}

// INFO: a constant is an INTEGER_LITERAL, one leading unary minus on a constant and a parenthesized constant. Nothing else is folded.
static MAREA_Boolean MAREA_CALL MAREA_G_Fold(const MAREA_AstNode *node, MAREA_Uinteger32 depth, MAREA_G_Folded *outFolded)
{
    if ((node == MAREA_NULLPTR) || (depth > MAREA_G_FOLD_DEPTH_LIMIT)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    if (KIND == MAREA_AST_KIND_INTEGER_LITERAL) {
        outFolded->negative = MAREA_BOOLEAN_FALSE;

        return MAREA_G_FoldLiteral(node, &outFolded->magnitude);
    }

    if (KIND == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) {
        return MAREA_G_Fold(MAREA_SemaChildAt(node, 0), (depth + 1U), outFolded);
    }

    if ((KIND == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_MINUS)) {
        if (MAREA_G_Fold(MAREA_SemaChildAt(node, 0), (depth + 1U), outFolded) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }

        outFolded->negative = (outFolded->negative == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;

        return MAREA_BOOLEAN_TRUE;
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: gives the width in bits of an integer primitive name such as sinteger32, or 0 for any other name.
static MAREA_Uinteger32 MAREA_CALL MAREA_G_IntegerWidth(const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    if ((length <= MAREA_G_PREFIX_LENGTH) || ((memcmp(name, "sinteger", MAREA_G_PREFIX_LENGTH) != 0) && (memcmp(name, "uinteger", MAREA_G_PREFIX_LENGTH) != 0))) {
        return 0U;
    }

    MAREA_Uinteger32 width = 0U;

    for (MAREA_IntegerSize index = MAREA_G_PREFIX_LENGTH; index < length; index += 1) {
        if ((name[index] < '0') || (name[index] > '9')) {
            return 0U;
        }

        width = ((width * 10U) + (MAREA_Uinteger32)(name[index] - '0'));
    }

    return width;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsDivision(MAREA_TokenKind operatorKind)
{
    return ((operatorKind == MAREA_TOKEN_KIND_SLASH) || (operatorKind == MAREA_TOKEN_KIND_PERCENT) || (operatorKind == MAREA_TOKEN_KIND_SLASH_EQUAL) || (operatorKind == MAREA_TOKEN_KIND_PERCENT_EQUAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsShift(MAREA_TokenKind operatorKind)
{
    return ((operatorKind == MAREA_TOKEN_KIND_SHIFT_LEFT) || (operatorKind == MAREA_TOKEN_KIND_SHIFT_RIGHT) || (operatorKind == MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL) || (operatorKind == MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantOperands(MAREA_SemaContext *context, MAREA_TokenKind operatorKind, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_SourcePosition position)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_G_Folded count = { MAREA_BOOLEAN_FALSE, 0U };

    if (((MAREA_G_IsDivision(operatorKind) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsShift(operatorKind) == MAREA_BOOLEAN_FALSE)) || (MAREA_G_Fold(right, 0U, &count) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_Boolean HAS_PRIMITIVE = MAREA_SemaPrimitiveOfExpression(context, left, &name, &length);

    const MAREA_Uinteger32 WIDTH = (HAS_PRIMITIVE == MAREA_BOOLEAN_TRUE) ? MAREA_G_IntegerWidth(name, length) : 0U;

    if (MAREA_G_IsDivision(operatorKind) == MAREA_BOOLEAN_TRUE) {
        MAREA_G_Folded unused = { MAREA_BOOLEAN_FALSE, 0U };

        // INFO: the left side is an integer when it has an integer primitive type or is itself a constant; a float or an unknown type is not checked.
        if ((count.magnitude == 0U) && ((WIDTH != 0U) || (MAREA_G_Fold(left, 0U, &unused) == MAREA_BOOLEAN_TRUE))) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_DIVISION_BY_ZERO, position, "division or remainder by a constant zero");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a count of -0 is zero, so it is not negative.
    if ((WIDTH != 0U) && (count.magnitude != 0U) && ((count.negative == MAREA_BOOLEAN_TRUE) || (count.magnitude >= WIDTH))) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_SHIFT_COUNT_TOO_LARGE, position, "a shift count is negative or not below the width of the left type");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
