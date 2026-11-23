// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

typedef enum MAREA_G_OperandClass
{
    MAREA_G_OPERAND_CLASS_UNKNOWN = MAREA_UINTEGER_8(0),
    MAREA_G_OPERAND_CLASS_INTEGER = MAREA_UINTEGER_8(1),
    MAREA_G_OPERAND_CLASS_FLOAT = MAREA_UINTEGER_8(2),
    MAREA_G_OPERAND_CLASS_BOOLEAN = MAREA_UINTEGER_8(3),
    MAREA_G_OPERAND_CLASS_STRING = MAREA_UINTEGER_8(4),
    MAREA_G_OPERAND_CLASS_CHARACTER = MAREA_UINTEGER_8(5),
} MAREA_G_OperandClass;

static MAREA_Boolean MAREA_CALL MAREA_G_NameIs(const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_Character8 *text)
{
    MAREA_IntegerSize size = 0;

    while (text[size] != '\0') {
        size++;
    }

    return MAREA_SemaTextEquals(name, length, text, size);
}

// INFO: a number literal has no primitive of its own, so its node kind says what it is. A primitive that sema knows is sorted by name. An unknown type has no class.
static MAREA_G_OperandClass MAREA_CALL MAREA_G_ClassOf(const MAREA_SemaContext *context, const MAREA_AstNode *operand)
{
    while ((operand != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(operand) == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION)) {
        operand = MAREA_SemaChildAt(operand, 0);
    }

    if (operand == MAREA_NULLPTR) {
        return MAREA_G_OPERAND_CLASS_UNKNOWN;
    }

    if (MAREA_AstNodeGetKind(operand) == MAREA_AST_KIND_INTEGER_LITERAL) {
        return MAREA_G_OPERAND_CLASS_INTEGER;
    }

    if (MAREA_AstNodeGetKind(operand) == MAREA_AST_KIND_FLOAT_LITERAL) {
        return MAREA_G_OPERAND_CLASS_FLOAT;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaPrimitiveOfExpression(context, operand, &name, &length) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_OPERAND_CLASS_UNKNOWN;
    }

    if ((MAREA_G_NameIs(name, length, "float32") == MAREA_BOOLEAN_TRUE) || (MAREA_G_NameIs(name, length, "float64") == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_OPERAND_CLASS_FLOAT;
    }

    if (MAREA_G_NameIs(name, length, "boolean") == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_OPERAND_CLASS_BOOLEAN;
    }

    if (MAREA_G_NameIs(name, length, "string") == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_OPERAND_CLASS_STRING;
    }

    return (MAREA_G_NameIs(name, length, "character") == MAREA_BOOLEAN_TRUE) ? MAREA_G_OPERAND_CLASS_CHARACTER : MAREA_G_OPERAND_CLASS_INTEGER;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBitwise(MAREA_TokenKind operatorKind)
{
    switch (operatorKind) {
    case MAREA_TOKEN_KIND_AMPERSAND:
    case MAREA_TOKEN_KIND_PIPE:
    case MAREA_TOKEN_KIND_CARET:
    case MAREA_TOKEN_KIND_TILDE:
    case MAREA_TOKEN_KIND_AMPERSAND_EQUAL:
    case MAREA_TOKEN_KIND_PIPE_EQUAL:
    case MAREA_TOKEN_KIND_CARET_EQUAL:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: a bitwise operand is wrong when it is a float, a boolean or a string. A character is left alone, as the rule names no character.
static MAREA_Boolean MAREA_CALL MAREA_G_BadForBitwise(MAREA_G_OperandClass operandClass)
{
    return ((operandClass == MAREA_G_OPERAND_CLASS_FLOAT) || (operandClass == MAREA_G_OPERAND_CLASS_BOOLEAN) || (operandClass == MAREA_G_OPERAND_CLASS_STRING)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOperatorOperands(MAREA_SemaContext *context, MAREA_TokenKind operatorKind, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_SourcePosition position)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_G_OperandClass leftClass = MAREA_G_ClassOf(context, left);

    const MAREA_G_OperandClass rightClass = MAREA_G_ClassOf(context, right);

    if (MAREA_G_IsBitwise(operatorKind) == MAREA_BOOLEAN_TRUE) {
        if ((MAREA_G_BadForBitwise(leftClass) == MAREA_BOOLEAN_TRUE) || (MAREA_G_BadForBitwise(rightClass) == MAREA_BOOLEAN_TRUE)) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_BITWISE_OPERAND_NOT_INTEGER, position, "a bitwise operator needs integer operands");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if ((operatorKind != MAREA_TOKEN_KIND_KEYWORD_AND) && (operatorKind != MAREA_TOKEN_KIND_KEYWORD_OR) && (operatorKind != MAREA_TOKEN_KIND_KEYWORD_NOT)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (((leftClass != MAREA_G_OPERAND_CLASS_UNKNOWN) && (leftClass != MAREA_G_OPERAND_CLASS_BOOLEAN)) || ((rightClass != MAREA_G_OPERAND_CLASS_UNKNOWN) && (rightClass != MAREA_G_OPERAND_CLASS_BOOLEAN))) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_LOGICAL_OPERAND_NOT_BOOLEAN, position, "a logical operator needs boolean operands");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
