// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

// INFO: a local is [name, type, value?]. The grammar keeps the value optional, and this check is the rule that sema adds: no value, no local. It needs no flow analysis.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLocalInitializer(MAREA_SemaContext *context, const MAREA_AstNode *local)
{
    if ((context == MAREA_NULLPTR) || (local == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_AstNodeGetChildCount(local) >= 3) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(local, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_ERROR_NODE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *type = MAREA_SemaChildAt(local, 1);

    if ((type != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(type) == MAREA_AST_KIND_ERROR_NODE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_LOCAL_WITHOUT_INITIAL_VALUE, MAREA_AstNodeGetPosition(local), "a local needs a value, local", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}
