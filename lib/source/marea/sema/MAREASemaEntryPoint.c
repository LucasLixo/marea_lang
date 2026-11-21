// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index)
{
    const MAREA_AstNode *child = MAREA_NULLPTR;

    if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_AstNode *node, const MAREA_Character8 *text)
{
    if ((node == MAREA_NULLPTR) || (MAREA_AstNodeGetText(node) == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize LENGTH = strlen(text);

    return ((MAREA_AstNodeGetTextLength(node) == LENGTH) && (memcmp(MAREA_AstNodeGetText(node), text, LENGTH) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a NAMED_TYPE of one plain name with exactly argumentCount type arguments, or null when it has another shape.
static const MAREA_AstNode *MAREA_CALL MAREA_G_PlainNamedType(const MAREA_AstNode *type, const MAREA_Character8 *name, MAREA_IntegerSize argumentCount)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *qualified = MAREA_G_ChildAt(type, 0);

    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(qualified) != 1) || (MAREA_G_TextIs(MAREA_G_ChildAt(qualified, 0), name) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *arguments = MAREA_G_ChildAt(type, 1);

    const MAREA_IntegerSize ACTUAL = ((arguments != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(arguments) == MAREA_AST_KIND_TYPE_ARGUMENTS)) ? MAREA_AstNodeGetChildCount(arguments) : MAREA_INTEGER_SIZE(0);

    return (ACTUAL == argumentCount) ? type : MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsValidMain(const MAREA_AstNode *function)
{
    const MAREA_AstNode *parameters = MAREA_NULLPTR;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(function); ++index) {
        const MAREA_AstNode *child = MAREA_G_ChildAt(function, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_PARAMETER_LIST)) {
            parameters = child;
        }
    }

    if ((parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(parameters) != 1)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *parameter = MAREA_G_ChildAt(parameters, 0);

    if ((parameter == MAREA_NULLPTR) || (MAREA_AstNodeGetFlags(parameter) != 0) || (MAREA_G_TextIs(MAREA_G_ChildAt(parameter, 0), "arguments") == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *array = MAREA_G_PlainNamedType(MAREA_G_ChildAt(parameter, 1), "array", 1);

    if (array == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_PlainNamedType(MAREA_G_ChildAt(MAREA_G_ChildAt(array, 1), 0), "string", 0) == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_G_PlainNamedType(MAREA_SemaReturnType(function), "integer32", 0) != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasErrorChild(const MAREA_AstNode *node)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *child = MAREA_G_ChildAt(node, index);

        if ((child == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_ERROR_NODE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckEntryPoint(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if ((context == MAREA_NULLPTR) || (root == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *declaration = MAREA_G_ChildAt(root, 0);

    const MAREA_AstNode *qualified = ((declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_NAMESPACE_DECLARATION)) ? MAREA_G_ChildAt(declaration, 0) : MAREA_NULLPTR;

    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualified) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(qualified) != 1) || (MAREA_G_TextIs(MAREA_G_ChildAt(qualified, 0), "main") == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(root); ++index) {
        const MAREA_AstNode *function = MAREA_G_ChildAt(root, index);

        if ((function == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(function) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
            continue;
        }

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        if (MAREA_SemaDeclarationName(function, &name, &length) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if ((length != MAREA_INTEGER_SIZE(4)) || (memcmp(name, "main", 4) != 0) || (MAREA_G_HasErrorChild(function) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsValidMain(function) == MAREA_BOOLEAN_TRUE)) {
            continue;
        }

        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MAIN_SIGNATURE, MAREA_AstNodeGetPosition(function), "main must have the signature (arguments: array<string>): integer32");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
