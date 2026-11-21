// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

#include <string.h>

#if !defined(MAREA_PARSE_CHILD_INITIAL_CAPACITY)
    #define MAREA_PARSE_CHILD_INITIAL_CAPACITY MAREA_INTEGER_SIZE(4)
#endif

MAREA_AstKind MAREA_CALL MAREA_AstNodeGetKind(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_AST_KIND_ERROR_NODE;
    }

    return node->kind;
}

MAREA_SourcePosition MAREA_CALL MAREA_AstNodeGetPosition(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        const MAREA_SourcePosition empty = { MAREA_NULLPTR, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0) };

        return empty;
    }

    return node->position;
}

const MAREA_Character8 *MAREA_CALL MAREA_AstNodeGetText(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return node->text;
}

MAREA_Uinteger32 MAREA_CALL MAREA_AstNodeGetTextLength(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_UINTEGER_32(0);
    }

    return node->textLength;
}

MAREA_TokenKind MAREA_CALL MAREA_AstNodeGetOperator(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_TOKEN_KIND_ERROR;
    }

    return node->operatorKind;
}

MAREA_Uinteger32 MAREA_CALL MAREA_AstNodeGetFlags(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_UINTEGER_32(0);
    }

    return node->flags;
}

MAREA_IntegerSize MAREA_CALL MAREA_AstNodeGetChildCount(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return node->childCount;
}

MAREA_AstExcept MAREA_CALL MAREA_AstNodeGetChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index, const MAREA_AstNode **out)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (out == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= node->childCount) {
        return MAREA_AST_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *out = node->children[index];

    return MAREA_AST_EXCEPT_SUCCESS;
}

MAREA_AstExcept MAREA_CALL MAREA_AstNodeCreate(MAREA_Arena *arena, MAREA_AstKind kind, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize textLength, MAREA_AstNode **outNode)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outNode == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_AstNode), MAREA_ALIGNOF(MAREA_AstNode), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_AstNode *node = memory;

    node->position = position;
    node->text = text;
    node->children = MAREA_NULLPTR;
    node->childCount = MAREA_INTEGER_SIZE(0);
    node->childCapacity = MAREA_INTEGER_SIZE(0);
    node->textLength = (MAREA_Uinteger32)textLength;
    node->flags = MAREA_UINTEGER_32(0);
    node->operatorKind = MAREA_TOKEN_KIND_ERROR;
    node->kind = kind;

    *outNode = node;

    return MAREA_AST_EXCEPT_SUCCESS;
}

MAREA_AstExcept MAREA_CALL MAREA_AstNodeAppendChild(MAREA_Arena *arena, MAREA_AstNode *parent, MAREA_AstNode *child)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (parent == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (child == MAREA_NULLPTR) {
        return MAREA_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (parent->childCount == parent->childCapacity) {
        const MAREA_IntegerSize newCapacity = (parent->childCapacity == MAREA_INTEGER_SIZE(0)) ? MAREA_PARSE_CHILD_INITIAL_CAPACITY : (parent->childCapacity * MAREA_INTEGER_SIZE(2));

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(arena, (newCapacity * sizeof(MAREA_AstNode *)), MAREA_ALIGNOF(MAREA_AstNode *), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_AstNode **newChildren = memory;

        if (parent->childCount > MAREA_INTEGER_SIZE(0)) {
            memcpy(newChildren, parent->children, (parent->childCount * sizeof(MAREA_AstNode *)));
        }

        parent->children = newChildren;
        parent->childCapacity = newCapacity;
    }

    parent->children[parent->childCount] = child;
    parent->childCount = (parent->childCount + MAREA_INTEGER_SIZE(1));

    return MAREA_AST_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_AstNodeSetOperator(MAREA_AstNode *node, MAREA_TokenKind operatorKind)
{
    if (node != MAREA_NULLPTR) {
        node->operatorKind = operatorKind;
    }
}

void MAREA_CALL MAREA_AstNodeSetFlags(MAREA_AstNode *node, MAREA_Uinteger32 flags)
{
    if (node != MAREA_NULLPTR) {
        node->flags = flags;
    }
}

void MAREA_CALL MAREA_AstNodeSetEnd(MAREA_AstNode *node, MAREA_Uinteger32 endOffset)
{
    if ((node != MAREA_NULLPTR) && (endOffset > node->position.offset)) {
        node->position.length = (endOffset - node->position.offset);
    }
}
