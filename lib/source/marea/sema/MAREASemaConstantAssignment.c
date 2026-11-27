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

static MAREA_Boolean MAREA_CALL MAREA_G_SameText(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    return ((left != MAREA_NULLPTR) && (right != MAREA_NULLPTR)) ? MAREA_SemaTextEquals(MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(right)) : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChildOfKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    for (MAREA_IntegerSize index = 0; ((node != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(node))); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the one name of a type written as NAMED_TYPE, then GENERIC_NAME, then the identifier.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ImplementedName(const MAREA_AstNode *implementation)
{
    const MAREA_AstNode *type = MAREA_G_ChildOfKind(implementation, MAREA_AST_KIND_NAMED_TYPE);

    return (type == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(MAREA_SemaChildAt(type, 0), 0);
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaFindTypeConstant(const MAREA_SemaContext *context, const MAREA_AstNode *typeName, const MAREA_AstNode *name)
{
    if ((context == MAREA_NULLPTR) || (typeName == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (context->root == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *wanted = typeName;

    // INFO: Self is the type of the implementation block being visited.
    if (MAREA_SemaTextEquals(MAREA_AstNodeGetText(typeName), MAREA_AstNodeGetTextLength(typeName), "Self", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_TRUE) {
        wanted = (context->implementation == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ImplementedName(context->implementation);
    } else if (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(typeName), MAREA_AstNodeGetTextLength(typeName)) != MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(context->root); ++index) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, index);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) || (MAREA_G_SameText(MAREA_G_ImplementedName(implementation), wanted) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize member = 0; member < MAREA_AstNodeGetChildCount(implementation); ++member) {
            const MAREA_AstNode *constant = MAREA_SemaChildAt(implementation, member);

            if ((constant != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(constant) == MAREA_AST_KIND_CONSTANT_DECLARATION) && (MAREA_G_SameText(MAREA_G_ChildOfKind(constant, MAREA_AST_KIND_IDENTIFIER), name) == MAREA_BOOLEAN_TRUE)) {
                return constant;
            }
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a local of the same name hides the constant, so the local table decides first. A bare name finds a constant of the file or of the implementation block being visited.
static MAREA_Boolean MAREA_CALL MAREA_G_NamesConstant(const MAREA_SemaContext *context, const MAREA_AstNode *target, const MAREA_AstNode *root)
{
    // INFO: Type.NAME is the first member of the chain, whose base is the root.
    for (const MAREA_AstNode *link = target; (link != MAREA_NULLPTR) && (link != root); link = MAREA_SemaChildAt(link, 0)) {
        if ((MAREA_AstNodeGetKind(link) == MAREA_AST_KIND_MEMBER_ACCESS) && (MAREA_SemaChildAt(link, 0) == root) && (MAREA_SemaFindTypeConstant(context, root, MAREA_SemaChildAt(link, 1)) != MAREA_NULLPTR)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    if (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root)) != MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_SemaFindConstant(context, root) != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckConstantAssignment(MAREA_SemaContext *context, const MAREA_AstNode *assignment)
{
    if ((context == MAREA_NULLPTR) || (assignment == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *target = MAREA_SemaChildAt(assignment, 0);

    const MAREA_AstNode *root = MAREA_G_TargetRoot(target);

    if ((root == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(root) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_G_NamesConstant(context, target, root) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_TO_CONSTANT, MAREA_AstNodeGetPosition(target), "a constant cannot be assigned:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
}
