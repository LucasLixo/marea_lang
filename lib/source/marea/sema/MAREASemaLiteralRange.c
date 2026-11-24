// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: one row per integer primitive. The width is in bits.
typedef struct MAREA_G_IntegerRange
{
    const MAREA_Character8 *name;
    MAREA_Uinteger32 bits;
    MAREA_Boolean isSigned;
} MAREA_G_IntegerRange;

static const MAREA_G_IntegerRange MAREA_G_RANGES[] = {
    { "sinteger8", 8, MAREA_BOOLEAN_TRUE },
    { "sinteger16", 16, MAREA_BOOLEAN_TRUE },
    { "sinteger32", 32, MAREA_BOOLEAN_TRUE },
    { "sinteger64", 64, MAREA_BOOLEAN_TRUE },
    { "uinteger8", 8, MAREA_BOOLEAN_FALSE },
    { "uinteger16", 16, MAREA_BOOLEAN_FALSE },
    { "uinteger32", 32, MAREA_BOOLEAN_FALSE },
    { "uinteger64", 64, MAREA_BOOLEAN_FALSE },
};

// INFO: the largest value that a literal of the given sign may have in the type: 2^bits - 1 for an unsigned type or a positive literal of a signed one is 2^(bits - 1) - 1, and 2^(bits - 1) for a negative literal of a signed type.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_LimitOf(MAREA_Uinteger32 bits, MAREA_Boolean isSigned, MAREA_Boolean negative)
{
    if (isSigned == MAREA_BOOLEAN_FALSE) {
        return (bits == 64) ? UINT64_MAX : ((((MAREA_Uinteger64)1) << bits) - 1);
    }

    const MAREA_Uinteger64 HALF = ((MAREA_Uinteger64)1) << (bits - 1);

    return (negative == MAREA_BOOLEAN_TRUE) ? HALF : (HALF - 1);
}

// INFO: reads the digits of an integer literal (decimal, 0x or 0b, with "_" between digits) and gives false when the value does not fit 64 bits.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadMagnitude(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger64 *outValue)
{
    MAREA_Uinteger64 base = 10;

    MAREA_IntegerSize index = 0;

    if ((length > 2) && (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
        base = 16;
        index = 2;
    } else if ((length > 2) && (text[0] == '0') && ((text[1] == 'b') || (text[1] == 'B'))) {
        base = 2;
        index = 2;
    }

    MAREA_Uinteger64 value = 0;

    for (; index < length; ++index) {
        const MAREA_Character8 CHARACTER = text[index];

        MAREA_Uinteger64 digit = 0;

        if (CHARACTER == '_') {
            continue;
        }

        if ((CHARACTER >= '0') && (CHARACTER <= '9')) {
            digit = (MAREA_Uinteger64)(CHARACTER - '0');
        } else if ((CHARACTER >= 'a') && (CHARACTER <= 'f')) {
            digit = (MAREA_Uinteger64)(CHARACTER - 'a') + 10;
        } else if ((CHARACTER >= 'A') && (CHARACTER <= 'F')) {
            digit = (MAREA_Uinteger64)(CHARACTER - 'A') + 10;
        } else {
            return MAREA_BOOLEAN_FALSE;
        }

        if (value > ((UINT64_MAX - digit) / base)) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
    }

    *outValue = value;

    return MAREA_BOOLEAN_TRUE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_SkipParentheses(const MAREA_AstNode *node)
{
    while ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION)) {
        node = MAREA_SemaChildAt(node, 0);
    }

    return node;
}

// INFO: the range row of a primitive name, or null for a float, a string, a boolean, a character and any other name.
static const MAREA_G_IntegerRange *MAREA_CALL MAREA_G_FindRange(const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < (sizeof(MAREA_G_RANGES) / sizeof(MAREA_G_RANGES[0])); ++index) {
        if (MAREA_SemaTextEquals(name, length, MAREA_G_RANGES[index].name, strlen(MAREA_G_RANGES[index].name)) == MAREA_BOOLEAN_TRUE) {
            return &MAREA_G_RANGES[index];
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a target that is a known primitive that is not an integer (a float, a string, a boolean) is never range checked here. Any other target is checked against the widest range.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckLiteral(MAREA_SemaContext *context, MAREA_Boolean hasPrimitive, const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_AstNode *expression)
{
    const MAREA_AstNode *operand = MAREA_G_SkipParentheses(expression);

    MAREA_Boolean negative = MAREA_BOOLEAN_FALSE;

    if ((operand != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(operand) == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(operand) == MAREA_TOKEN_KIND_MINUS)) {
        negative = MAREA_BOOLEAN_TRUE;
        operand = MAREA_G_SkipParentheses(MAREA_SemaChildAt(operand, 0));
    }

    if ((operand == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(operand) != MAREA_AST_KIND_INTEGER_LITERAL)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_IntegerRange *range = (hasPrimitive == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindRange(name, length) : MAREA_NULLPTR;

    if ((hasPrimitive == MAREA_BOOLEAN_TRUE) && (range == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger64 magnitude = 0;

    MAREA_Boolean fits = MAREA_G_ReadMagnitude(MAREA_AstNodeGetText(operand), MAREA_AstNodeGetTextLength(operand), &magnitude);

    if (fits == MAREA_BOOLEAN_TRUE) {
        if (range == MAREA_NULLPTR) {
            // INFO: no known target, so the literal must fit uinteger64, or sinteger64 when it is negative.
            fits = (negative == MAREA_BOOLEAN_TRUE) ? (magnitude <= MAREA_G_LimitOf(64, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE)) : MAREA_BOOLEAN_TRUE;
        } else if ((range->isSigned == MAREA_BOOLEAN_FALSE) && (negative == MAREA_BOOLEAN_TRUE)) {
            fits = (magnitude == 0);
        } else {
            fits = (magnitude <= MAREA_G_LimitOf(range->bits, range->isSigned, negative));
        }
    }

    if (fits == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_LITERAL_OUT_OF_RANGE, MAREA_AstNodeGetPosition(expression), "integer literal is out of range for the type", (range == MAREA_NULLPTR) ? "uinteger64" : name, (range == MAREA_NULLPTR) ? strlen("uinteger64") : length);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralRange(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (expression == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_Boolean HAS_PRIMITIVE = (type == MAREA_NULLPTR) ? MAREA_BOOLEAN_FALSE : MAREA_SemaPrimitiveOfType(context, type, &name, &length);

    return MAREA_G_CheckLiteral(context, HAS_PRIMITIVE, name, length, expression);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralRangePrimitive(MAREA_SemaContext *context, const MAREA_Character8 *primitiveName, MAREA_IntegerSize primitiveLength, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (expression == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_CheckLiteral(context, (primitiveName != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE, primitiveName, primitiveLength, expression);
}

// INFO: a field of a struct literal that names a field of a structure of the file is checked against the declared type of the field. A field that is not found is checked against the widest range.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralFields(MAREA_SemaContext *context, const MAREA_AstNode *literal)
{
    if ((context == MAREA_NULLPTR) || (literal == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *typeName = MAREA_NULLPTR;

    MAREA_IntegerSize typeLength = 0;

    const MAREA_AstNode *type = MAREA_SemaChildAt(literal, 0);

    const MAREA_Boolean IS_STRUCTURE = ((type != MAREA_NULLPTR) && (MAREA_SemaClassifyType(context, type, &declaration, &typeName, &typeLength) == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_STRUCTURE_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (IS_STRUCTURE == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(literal); ++index) {
        const MAREA_AstNode *entry = MAREA_SemaChildAt(literal, index);

        if ((entry == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry) != MAREA_AST_KIND_FIELD_ENTRY)) {
            continue;
        }

        const MAREA_AstNode *entryName = MAREA_SemaChildAt(entry, 0);

        const MAREA_AstNode *fieldType = MAREA_NULLPTR;

        for (MAREA_IntegerSize fieldIndex = 0; fieldIndex < MAREA_AstNodeGetChildCount(declaration); ++fieldIndex) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(declaration, fieldIndex);

            if ((field == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(field) != MAREA_AST_KIND_FIELD_DECLARATION)) {
                continue;
            }

            const MAREA_AstNode *fieldName = MAREA_SemaChildAt(field, 0);

            if ((fieldName != MAREA_NULLPTR) && (entryName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), MAREA_AstNodeGetText(entryName), MAREA_AstNodeGetTextLength(entryName)) == MAREA_BOOLEAN_TRUE)) {
                fieldType = MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1));

                break;
            }
        }

        const MAREA_SemaExcept RESULT = MAREA_SemaCheckLiteralRange(context, fieldType, MAREA_SemaChildAt(entry, 1));

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
