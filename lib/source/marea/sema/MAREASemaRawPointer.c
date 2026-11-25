// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

// INFO: whether type carries the raw pointer role: a registered type a library gave that role, read by
// MAREA_SemaDescriptorOfType, never by a name. issue #190, decision 30 of the 2026-11-24 plan, extends
// this rule to a parameter, a result, a field and an alias; only the local a declaration binds is
// wired in today, so the extension is still open (see the apply's open points).
static MAREA_Boolean MAREA_CALL MAREA_G_HasRawPointerRole(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE) || (MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) != MAREA_TYPE_ROLE_NONE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a local is [name, type, value?].
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckRawPointerLocal(MAREA_SemaContext *context, const MAREA_AstNode *local)
{
    if ((context == MAREA_NULLPTR) || (local == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (context->unsafeDepth > 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *type = MAREA_SemaChildAt(local, 1);

    if (MAREA_G_HasRawPointerRole(context, type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(local, 0);

    if (name == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE, MAREA_AstNodeGetPosition(local), "a raw pointer exists only inside an unsafe block, local", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}
