// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

struct MAREA_SemaTypedNode
{
    const MAREA_AstNode *astNode;
    const MAREA_AstNode *typeDeclaration;
    const MAREA_Character8 *typeName;
    MAREA_SemaTypedNode **children;
    MAREA_IntegerSize typeNameLength;
    MAREA_IntegerSize childCount;
    MAREA_SemaTypeClass typeClass;
    MAREA_SemaOwnershipMode ownershipMode;
};

static MAREA_Boolean MAREA_CALL MAREA_G_TypedAstWasChecked(const MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context->root != root) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkGetErrorCount(context->sink) != 0) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypedAstBuildNode(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaTypedNode **outNode)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_SemaTypedNode), MAREA_ALIGNOF(MAREA_SemaTypedNode), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaTypedNode *built = memory;

    built->astNode = node;
    built->typeDeclaration = MAREA_NULLPTR;
    built->typeName = MAREA_NULLPTR;
    built->children = MAREA_NULLPTR;
    built->typeNameLength = 0;
    built->childCount = MAREA_AstNodeGetChildCount(node);
    built->typeClass = MAREA_SEMA_TYPE_CLASS_PRIMITIVE;
    built->ownershipMode = MAREA_SEMA_OWNERSHIP_MODE_VALUE;

    if (built->childCount > 0) {
        MAREA_Null childrenMemory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (built->childCount * sizeof(MAREA_SemaTypedNode *)), MAREA_ALIGNOF(MAREA_SemaTypedNode *), &childrenMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        built->children = childrenMemory;

        for (MAREA_IntegerSize index = 0; index < built->childCount; ++index) {
            const MAREA_AstNode *child = MAREA_NULLPTR;

            if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
                built->children[index] = MAREA_NULLPTR;

                continue;
            }

            MAREA_SemaTypedNode *typedChild = MAREA_NULLPTR;

            const MAREA_SemaTypedAstExcept CHILD_RESULT = MAREA_G_TypedAstBuildNode(context, child, &typedChild);

            if (CHILD_RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                return CHILD_RESULT;
            }

            built->children[index] = typedChild;
        }
    }

    *outNode = built;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaBuildTypedAst(MAREA_SemaContext *context, const MAREA_AstNode *root, const MAREA_SemaTypedNode **outRoot)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outRoot == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_G_TypedAstWasChecked(context, root) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED;
    }

    MAREA_SemaTypedNode *built = MAREA_NULLPTR;

    const MAREA_SemaTypedAstExcept RESULT = MAREA_G_TypedAstBuildNode(context, root, &built);

    if (RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return RESULT;
    }

    *outRoot = built;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaTypedNodeGetAstNode(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return node->astNode;
}

MAREA_IntegerSize MAREA_CALL MAREA_SemaTypedNodeGetChildCount(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return 0;
    }

    return node->childCount;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetChildAt(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index, const MAREA_SemaTypedNode **out)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (out == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= node->childCount) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *out = node->children[index];

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypeClass MAREA_CALL MAREA_SemaTypedNodeGetTypeClass(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPE_CLASS_PRIMITIVE;
    }

    return node->typeClass;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetTypeDeclaration(const MAREA_SemaTypedNode *node, const MAREA_AstNode **out)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (out == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *out = node->typeDeclaration;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedNodeGetTypeName(const MAREA_SemaTypedNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = node->typeName;
    *outLength = node->typeNameLength;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

MAREA_SemaOwnershipMode MAREA_CALL MAREA_SemaTypedNodeGetOwnershipMode(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_OWNERSHIP_MODE_VALUE;
    }

    return node->ownershipMode;
}
