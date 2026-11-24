// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

// INFO: walks MEMBER_ACCESS and INDEX_ACCESS through child 0 and returns the root node of an assignment target.
static const MAREA_AstNode *MAREA_CALL MAREA_G_TargetRoot(const MAREA_AstNode *target)
{
    const MAREA_AstNode *root = target;

    while ((root != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_MEMBER_ACCESS) || (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_INDEX_ACCESS))) {
        root = MAREA_SemaChildAt(root, 0);
    }

    return root;
}

// INFO: a local of the same name hides the constant, so the local table decides first.
static MAREA_Boolean MAREA_CALL MAREA_G_NamesFileConstant(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) != MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) && (entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_CONSTANT_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantAssignment(MAREA_SemaContext *context, const MAREA_AstNode *assignment)
{
    if ((context == MAREA_NULLPTR) || (assignment == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *target = MAREA_SemaChildAt(assignment, 0);

    const MAREA_AstNode *root = MAREA_G_TargetRoot(target);

    if ((root == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(root) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_G_NamesFileConstant(context, root) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_TO_CONSTANT, MAREA_AstNodeGetPosition(target), "a constant cannot be assigned:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
}
