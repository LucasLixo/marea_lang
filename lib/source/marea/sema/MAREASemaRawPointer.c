// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

// INFO: a local is [name, type, value?]. The type is a NAMED_TYPE whose first child is a QUALIFIED_NAME of one segment when it is written without a namespace.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckRawPointerLocal(MAREA_SemaContext *context, const MAREA_AstNode *local)
{
    if ((context == MAREA_NULLPTR) || (local == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (context->unsafeDepth > 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *type = MAREA_SemaChildAt(local, 1);

    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *qualifiedName = MAREA_SemaChildAt(type, 0);

    if ((qualifiedName == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualifiedName) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(qualifiedName) != 1)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(qualifiedName, 0);

    if ((segment == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment), "pointer", MAREA_INTEGER_SIZE(7)) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(local, 0);

    if (name == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE, MAREA_AstNodeGetPosition(local), "a raw pointer exists only inside an unsafe block, local", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}
