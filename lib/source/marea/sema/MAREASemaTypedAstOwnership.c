// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstIsValueTypeClass(const MAREA_SemaContext *context, MAREA_SemaTypeClass typeClass, const MAREA_AstNode *declaration)
{
    if ((typeClass == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) || (typeClass == MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN)) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (typeClass != MAREA_SEMA_TYPE_CLASS_USER) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: an enum is a plain integer, so it is always passed by value, same rule MAREA_G_IsByValue uses.
    if (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION) {
        return MAREA_BOOLEAN_TRUE;
    }

    return MAREA_SemaAttributeIsOn(context->root, declaration, "value");
}

static MAREA_SemaOwnershipMode MAREA_CALL MAREA_G_TypedAstClassifyParameter(MAREA_SemaContext *context, const MAREA_AstNode *parameter)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, MAREA_SemaChildAt(parameter, 1), &declaration, &name, &length);

    if (MAREA_G_TypedAstIsValueTypeClass(context, CLASS, declaration) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_OWNERSHIP_MODE_VALUE;
    }

    if (CLASS == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
        return MAREA_SEMA_OWNERSHIP_MODE_MOVE;
    }

    const MAREA_Uinteger32 FLAGS = MAREA_AstNodeGetFlags(parameter);

    if (((FLAGS & MAREA_AST_FLAG_MUTABLE) != 0) && ((FLAGS & MAREA_AST_FLAG_REFERENCE) == 0)) {
        return MAREA_SEMA_OWNERSHIP_MODE_MUTABLE_BORROW;
    }

    return MAREA_SEMA_OWNERSHIP_MODE_BORROW;
}

MAREA_SemaOwnershipMode MAREA_CALL MAREA_SemaTypedAstClassifyOwnership(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    if ((context == MAREA_NULLPTR) || (built == MAREA_NULLPTR)) {
        return MAREA_SEMA_OWNERSHIP_MODE_VALUE;
    }

    const MAREA_AstNode *node = built->astNode;

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    if (KIND == MAREA_AST_KIND_UNARY_EXPRESSION) {
        const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(node);

        if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_COPY) {
            return MAREA_SEMA_OWNERSHIP_MODE_COPY;
        }

        if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_MOVE) {
            return MAREA_SEMA_OWNERSHIP_MODE_MOVE;
        }
    }

    if (KIND == MAREA_AST_KIND_PARAMETER) {
        return MAREA_G_TypedAstClassifyParameter(context, node);
    }

    if ((KIND == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaIsReference(context, MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node)) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_OWNERSHIP_MODE_BORROW;
    }

    if (built->typeClass == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
        return MAREA_SEMA_OWNERSHIP_MODE_MOVE;
    }

    // INFO: every other expression (including one this builder could not infer a type for)
    // defaults to VALUE, same as MAREA_SemaInferType's own best-effort fallback.
    return MAREA_SEMA_OWNERSHIP_MODE_VALUE;
}
