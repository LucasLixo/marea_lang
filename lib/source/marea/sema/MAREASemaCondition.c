// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

// INFO: a literal has no primitive of its own, so its node kind says what it is. An unknown type is left alone, as sema cannot judge it.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCondition(MAREA_SemaContext *context, const MAREA_AstNode *condition)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    while ((condition != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(condition) == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION)) {
        condition = MAREA_SemaChildAt(condition, 0);
    }

    if (condition == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    switch (MAREA_AstNodeGetKind(condition)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_NON_BOOLEAN_CONDITION, MAREA_AstNodeGetPosition(condition), "a condition must be a boolean, this one is a literal of another type");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    default:
        break;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaPrimitiveOfExpression(context, condition, &name, &length) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_SemaTextEquals(name, length, "boolean", 7) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NON_BOOLEAN_CONDITION, MAREA_AstNodeGetPosition(condition), "a condition must be a boolean, this one is", name, length);
}
