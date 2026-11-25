// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

#if !defined(MAREA_IR_CHILD_INITIAL_CAPACITY)
    #define MAREA_IR_CHILD_INITIAL_CAPACITY MAREA_INTEGER_SIZE(4)
#endif

static const MAREA_Character8 *const MAREA_G_IrKindNames[] = {
    "MODULE",
    "CONSTANT_DECLARATION",
    "STRUCTURE_DECLARATION",
    "FIELD_DECLARATION",
    "ENUM_DECLARATION",
    "VARIANT_DECLARATION",
    "VARIANT_FIELD",
    "FUNCTION_DECLARATION",
    "PARAMETER_LIST",
    "PARAMETER",
    "BLOCK",
    "LOCAL_DECLARATION",
    "ASSIGNMENT",
    "EXPRESSION_STATEMENT",
    "IF_STATEMENT",
    "ELSEIF_CLAUSE",
    "ELSE_CLAUSE",
    "WHILE_STATEMENT",
    "RETURN_STATEMENT",
    "BREAK_STATEMENT",
    "CONTINUE_STATEMENT",
    "SCOPE_BLOCK",
    "IDENTIFIER",
    "INTEGER_LITERAL",
    "FLOAT_LITERAL",
    "STRING_LITERAL",
    "BOOLEAN_LITERAL",
    "BINARY_EXPRESSION",
    "UNARY_EXPRESSION",
    "MEMBER_ACCESS",
    "INDEX_ACCESS",
    "CALL_EXPRESSION",
    "ARGUMENT_LIST",
    "LITERAL_VALUE",
    "FIELD_ENTRY",
    "INDEX_ENTRY",
    "VALUE_ENTRY",
    "QUALIFIED_NAME",
    "UNREACHABLE_STATEMENT",
    "FORK_JOIN_BLOCK",
    "PARTITION_STATEMENT",
    "WITH_STATEMENT",
    "FOR_EACH_STATEMENT",
    "TYPE_NAME",
    "BUILTIN_NAME",
    "CHARACTER_LITERAL",
    MAREA_NULLPTR, // INFO: the retired value of ZERO_VALUE, kept so that no later kind moves; it names no kind.
    "CONSTRUCT_CALL",
    "DESTROY_CALL",
    "BACKEND_ATTRIBUTE",
};

MAREA_STATIC_ASSERT((sizeof(MAREA_G_IrKindNames) / sizeof(MAREA_G_IrKindNames[0])) == MAREA_IR_KIND_COUNT, "MAREA_G_IrKindNames must have one entry per IR kind.");

const MAREA_Character8 *MAREA_CALL MAREA_IrKindGetName(MAREA_IrKind kind)
{
    if (((MAREA_IntegerSize)kind) >= (MAREA_IntegerSize)MAREA_IR_KIND_COUNT) {
        return "UNKNOWN";
    }

    // INFO: the retired value has no name.
    return (MAREA_G_IrKindNames[kind] != MAREA_NULLPTR) ? MAREA_G_IrKindNames[kind] : "UNKNOWN";
}

MAREA_IrKind MAREA_CALL MAREA_IrNodeGetKind(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_IR_KIND_COUNT;
    }

    return node->kind;
}

MAREA_SourcePosition MAREA_CALL MAREA_IrNodeGetPosition(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        const MAREA_SourcePosition empty = { .file = MAREA_NULLPTR, .offset = MAREA_UINTEGER_32(0), .line = MAREA_UINTEGER_32(0), .column = MAREA_UINTEGER_32(0), .length = MAREA_UINTEGER_32(0) };

        return empty;
    }

    return node->position;
}

const MAREA_Character8 *MAREA_CALL MAREA_IrNodeGetText(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return node->text;
}

MAREA_Uinteger32 MAREA_CALL MAREA_IrNodeGetTextLength(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_UINTEGER_32(0);
    }

    return node->textLength;
}

MAREA_TokenKind MAREA_CALL MAREA_IrNodeGetOperator(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_TOKEN_KIND_ERROR;
    }

    return node->operatorKind;
}

MAREA_Uinteger32 MAREA_CALL MAREA_IrNodeGetFlags(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_UINTEGER_32(0);
    }

    return node->flags;
}

MAREA_IntegerSize MAREA_CALL MAREA_IrNodeGetChildCount(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return node->childCount;
}

MAREA_IrNodeExcept MAREA_CALL MAREA_IrNodeGetChildAt(const MAREA_IrNode *node, MAREA_IntegerSize index, const MAREA_IrNode **out)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_IR_NODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (out == MAREA_NULLPTR) {
        return MAREA_IR_NODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= node->childCount) {
        return MAREA_IR_NODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *out = node->children[index];

    return MAREA_IR_NODE_EXCEPT_SUCCESS;
}

const MAREA_Character8 *MAREA_CALL MAREA_IrNodeGetTypeName(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return node->typeName;
}

MAREA_IntegerSize MAREA_CALL MAREA_IrNodeGetTypeNameLength(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return node->typeNameLength;
}

const MAREA_Character8 *MAREA_CALL MAREA_IrNodeGetResolvedType(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return node->resolvedType;
}

MAREA_IntegerSize MAREA_CALL MAREA_IrNodeGetResolvedTypeLength(const MAREA_IrNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return node->resolvedTypeLength;
}

MAREA_IrExcept MAREA_CALL MAREA_IrNodeCreate(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_AstNode *astNode, MAREA_IrNode **outNode)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (astNode == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outNode == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_IrNode), MAREA_ALIGNOF(MAREA_IrNode), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IrNode *node = memory;

    node->position = MAREA_AstNodeGetPosition(astNode);
    node->text = MAREA_AstNodeGetText(astNode);
    node->typeName = MAREA_NULLPTR;
    node->resolvedType = MAREA_NULLPTR;
    node->resolvedTypeLength = MAREA_INTEGER_SIZE(0);
    node->children = MAREA_NULLPTR;
    node->layouts = MAREA_NULLPTR;
    node->typeNameLength = MAREA_INTEGER_SIZE(0);
    node->childCount = MAREA_INTEGER_SIZE(0);
    node->childCapacity = MAREA_INTEGER_SIZE(0);
    node->textLength = MAREA_AstNodeGetTextLength(astNode);
    node->flags = MAREA_AstNodeGetFlags(astNode);
    node->operatorKind = MAREA_AstNodeGetOperator(astNode);
    node->kind = kind;

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrNodeAppendChild(MAREA_IrContext *context, MAREA_IrNode *parent, MAREA_IrNode *child)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (parent == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (child == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (parent->childCount == parent->childCapacity) {
        const MAREA_IntegerSize newCapacity = (parent->childCapacity == MAREA_INTEGER_SIZE(0)) ? MAREA_IR_CHILD_INITIAL_CAPACITY : (parent->childCapacity * MAREA_INTEGER_SIZE(2));

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (newCapacity * sizeof(MAREA_IrNode *)), MAREA_ALIGNOF(MAREA_IrNode *), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_IrNode **newChildren = memory;

        if (parent->childCount > MAREA_INTEGER_SIZE(0)) {
            memcpy(newChildren, parent->children, (parent->childCount * sizeof(MAREA_IrNode *)));
        }

        parent->children = newChildren;
        parent->childCapacity = newCapacity;
    }

    parent->children[parent->childCount] = child;
    parent->childCount = (parent->childCount + MAREA_INTEGER_SIZE(1));

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetTypeNameFromTyped(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_SemaTypedNode *typedNode)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((node == MAREA_NULLPTR) || (typedNode == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if ((MAREA_SemaTypedNodeGetTypeName(typedNode, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_IrAliasType(context, text, length, &node->typeName, &node->typeNameLength);
}

MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetResolvedTypeFromTyped(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_SemaTypedNode *typedNode, MAREA_Boolean *outTyped)
{
    if (outTyped != MAREA_NULLPTR) {
        *outTyped = MAREA_BOOLEAN_FALSE;
    }

    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((node == MAREA_NULLPTR) || (typedNode == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if ((MAREA_SemaTypedNodeGetResolvedType(typedNode, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    // INFO: the text is copied, so the node never points into an arena that the context does not own.
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, length, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    memcpy(memory, text, length);

    node->resolvedType = memory;
    node->resolvedTypeLength = length;

    if (outTyped != MAREA_NULLPTR) {
        *outTyped = MAREA_BOOLEAN_TRUE;
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_IrNodeShareResolvedType(MAREA_IrNode *node, const MAREA_IrNode *source)
{
    if ((node == MAREA_NULLPTR) || (source == MAREA_NULLPTR) || (source->resolvedType == MAREA_NULLPTR)) {
        return;
    }

    node->resolvedType = source->resolvedType;
    node->resolvedTypeLength = source->resolvedTypeLength;
}

void MAREA_CALL MAREA_IrNodeSetTypeNameFromResolved(MAREA_IrNode *node)
{
    if ((node == MAREA_NULLPTR) || (node->resolvedType == MAREA_NULLPTR)) {
        return;
    }

    static const MAREA_Character8 FUNCTION_HEAD[] = "function(";

    const MAREA_IntegerSize HEAD_LENGTH = strlen(FUNCTION_HEAD);

    // INFO: a function type has no head name, as a written FUNCTION_TYPE has none.
    if ((node->resolvedTypeLength >= HEAD_LENGTH) && (memcmp(node->resolvedType, FUNCTION_HEAD, HEAD_LENGTH) == 0)) {
        return;
    }

    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    while ((length < node->resolvedTypeLength) && (node->resolvedType[length] != '<')) {
        length = (length + MAREA_INTEGER_SIZE(1));
    }

    node->typeName = node->resolvedType;
    node->typeNameLength = length;
}

void MAREA_CALL MAREA_IrNodeSetResolvedTypeStatic(MAREA_IrNode *node, const MAREA_Character8 *text)
{
    if ((node == MAREA_NULLPTR) || (text == MAREA_NULLPTR)) {
        return;
    }

    node->resolvedType = text;
    node->resolvedTypeLength = strlen(text);
}

MAREA_Boolean MAREA_CALL MAREA_IrAstQualifiedNameSpan(const MAREA_AstNode *name, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(name);

    const MAREA_AstNode *first = MAREA_NULLPTR;
    const MAREA_AstNode *last = MAREA_NULLPTR;

    if ((COUNT == MAREA_INTEGER_SIZE(0)) || (MAREA_AstNodeGetChildAt(name, MAREA_INTEGER_SIZE(0), &first) != MAREA_AST_EXCEPT_SUCCESS) || (MAREA_AstNodeGetChildAt(name, (COUNT - MAREA_INTEGER_SIZE(1)), &last) != MAREA_AST_EXCEPT_SUCCESS)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *start = MAREA_AstNodeGetText(first);
    const MAREA_Character8 *end = MAREA_AstNodeGetText(last);

    if ((start == MAREA_NULLPTR) || (end == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: the segments of a dotted name are contiguous in the source, so one span covers all of them.
    *outText = start;
    *outLength = (MAREA_IntegerSize)((end + MAREA_AstNodeGetTextLength(last)) - start);

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_IrAstTypeName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_NULLPTR;

    if (MAREA_AstNodeGetChildAt(type, MAREA_INTEGER_SIZE(0), &name) != MAREA_AST_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_IrAstQualifiedNameSpan(name, outText, outLength);
}

MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetTypeNameFromAst(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_AstNode *type)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (MAREA_IrAstTypeName(type, &text, &length) == MAREA_BOOLEAN_FALSE) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_IrAliasType(context, text, length, &node->typeName, &node->typeNameLength);
}

MAREA_IrExcept MAREA_CALL MAREA_IrNodeSetQualifiedText(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (name == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (qualifier == MAREA_NULLPTR) {
        node->text = name;
        node->textLength = (MAREA_Uinteger32)nameLength;

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (qualifierLength + MAREA_INTEGER_SIZE(1) + nameLength), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    memcpy(joined, qualifier, qualifierLength);

    joined[qualifierLength] = '.';

    memcpy((joined + qualifierLength + MAREA_INTEGER_SIZE(1)), name, nameLength);

    node->text = joined;
    node->textLength = (MAREA_Uinteger32)(qualifierLength + MAREA_INTEGER_SIZE(1) + nameLength);

    return MAREA_IR_EXCEPT_SUCCESS;
}

const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrTypedChildAt(const MAREA_SemaTypedNode *typed, MAREA_IntegerSize index)
{
    const MAREA_SemaTypedNode *child = MAREA_NULLPTR;

    if (MAREA_SemaTypedNodeGetChildAt(typed, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrTypedFindChild(const MAREA_SemaTypedNode *typed, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *child = MAREA_IrTypedChildAt(typed, index);

        if ((child != MAREA_NULLPTR) && (MAREA_IrTypedKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_AstKind MAREA_CALL MAREA_IrTypedKind(const MAREA_SemaTypedNode *typed)
{
    return MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(typed));
}
