// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

// INFO: the type system is erased, so a type or a type-argument child never becomes an IR node.
static MAREA_Boolean MAREA_CALL MAREA_G_IsErasedChild(MAREA_AstKind kind)
{
    return ((kind == MAREA_AST_KIND_NAMED_TYPE) || (kind == MAREA_AST_KIND_FUNCTION_TYPE) || (kind == MAREA_AST_KIND_TYPE_ARGUMENTS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: lowers every kept child of typed and appends it to node. A child that lowers to nothing makes
// the whole parent lower to nothing, so no partial tree is produced.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerChildren(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean *outComplete)
{
    *outComplete = MAREA_BOOLEAN_TRUE;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *child = MAREA_IrTypedChildAt(typed, index);

        if (MAREA_G_IsErasedChild(MAREA_IrTypedKind(child)) == MAREA_BOOLEAN_TRUE) {
            continue;
        }

        MAREA_IrNode *lowered = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_IrLowerExpression(context, child, &lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        if (lowered == MAREA_NULLPTR) {
            *outComplete = MAREA_BOOLEAN_FALSE;

            return MAREA_IR_EXCEPT_SUCCESS;
        }

        result = MAREA_IrNodeAppendChild(context, node, lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerWithChildren(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, kind, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_G_LowerChildren(context, typed, node, &complete);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_TRUE)) {
        *outNode = node;
    }

    return result;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *callee = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    // INFO: a generic callee is a Gap, raised by the part that owns the Gap diagnostics.
    if ((callee == MAREA_NULLPTR) || (MAREA_IrTypedKind(callee) == MAREA_AST_KIND_GENERIC_NAME)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_CALL_EXPRESSION, typed, &node);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (node == MAREA_NULLPTR)) {
        return result;
    }

    // INFO: an unbound generic result type names no concrete type, so no type name is carried over.
    if (MAREA_SemaTypedNodeGetTypeClass(typed) != MAREA_SEMA_TYPE_CLASS_GENERIC) {
        MAREA_IrNodeSetTypeNameFromTyped(node, typed);
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerLiteralValue(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    const MAREA_IrExcept result = MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_LITERAL_VALUE, typed, &node);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (node == MAREA_NULLPTR)) {
        return result;
    }

    const MAREA_SemaTypedNode *type = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_NAMED_TYPE);

    MAREA_IrNodeSetTypeNameFromAst(node, MAREA_SemaTypedNodeGetAstNode(type));

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: a field entry is named after its IDENTIFIER and holds the lowered value as its only child.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerFieldEntry(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *name = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    const MAREA_SemaTypedNode *value = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(1));

    if ((name == MAREA_NULLPTR) || (value == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_FIELD_ENTRY, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

    result = MAREA_IrNodeSetQualifiedText(context, node, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNode *lowered = MAREA_NULLPTR;

    result = MAREA_IrLowerExpression(context, value, &lowered);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR)) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, node, lowered);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerExpression(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    if ((context == MAREA_NULLPTR) || (typed == MAREA_NULLPTR) || (outNode == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    *outNode = MAREA_NULLPTR;

    switch (MAREA_IrTypedKind(typed)) {
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_INTEGER_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_INTEGER_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_FLOAT_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_FLOAT_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_STRING_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_STRING_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_BOOLEAN_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_QUALIFIED_NAME:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_QUALIFIED_NAME, typed, outNode);
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        // INFO: the parentheses only shaped the tree; the inner expression lowers directly.
        return MAREA_IrLowerExpression(context, MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0)), outNode);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_BINARY_EXPRESSION, typed, outNode);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        // INFO: spawn is a Gap, raised by the part that owns the Gap diagnostics.
        if (MAREA_AstNodeGetOperator(MAREA_SemaTypedNodeGetAstNode(typed)) == MAREA_TOKEN_KIND_KEYWORD_SPAWN) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }

        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_UNARY_EXPRESSION, typed, outNode);
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_MEMBER_ACCESS, typed, outNode);
    case MAREA_AST_KIND_INDEX_ACCESS:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_INDEX_ACCESS, typed, outNode);
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_LowerCall(context, typed, outNode);
    case MAREA_AST_KIND_ARGUMENT_LIST:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_ARGUMENT_LIST, typed, outNode);
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_G_LowerLiteralValue(context, typed, outNode);
    case MAREA_AST_KIND_FIELD_ENTRY:
        return MAREA_G_LowerFieldEntry(context, typed, outNode);
    case MAREA_AST_KIND_INDEX_ENTRY:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_INDEX_ENTRY, typed, outNode);
    case MAREA_AST_KIND_VALUE_ENTRY:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_VALUE_ENTRY, typed, outNode);
    default:
        return MAREA_IR_EXCEPT_SUCCESS;
    }
}
