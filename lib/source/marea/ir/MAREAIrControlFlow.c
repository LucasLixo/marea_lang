// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

// INFO: the text of the synthetic increment of a for-range loop.
static const MAREA_Character8 MAREA_G_IncrementText[] = "1";

// INFO: a child that lowers to nothing makes the whole parent lower to nothing, so no partial tree is produced.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerChildrenInto(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean *outComplete)
{
    *outComplete = MAREA_BOOLEAN_TRUE;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_IrNode *lowered = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_G_LowerStatement(context, MAREA_IrTypedChildAt(typed, index), &lowered);

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

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerKept(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, kind, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_G_LowerChildrenInto(context, typed, node, &complete);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_TRUE)) {
        *outNode = node;
    }

    return result;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerLocal(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_LOCAL_DECLARATION, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_IrAppendInitializer(context, typed, node, &complete);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_TRUE)) {
        *outNode = node;
    }

    return result;
}

// INFO: unreachable!() is the one built-in macro that has a lowering; every other macro is a Gap.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerMacroStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *call = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_MACRO_CALL);

    const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(call, MAREA_AST_KIND_IDENTIFIER);

    if (name == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

    const MAREA_IntegerSize LENGTH = strlen("unreachable");

    if ((MAREA_AstNodeGetTextLength(nameAst) != LENGTH) || (memcmp(MAREA_AstNodeGetText(nameAst), "unreachable", LENGTH) != 0)) {
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO, MAREA_SemaTypedNodeGetAstNode(typed), "Macro is not supported by the IR yet:", MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));
    }

    return MAREA_IrNodeCreate(context, MAREA_IR_KIND_UNREACHABLE_STATEMENT, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
}

// INFO: for name in first .. last do body end becomes BLOCK(LOCAL name = first, WHILE name <= last: body, name += 1).
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerForRange(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_AstNode *forAst = MAREA_SemaTypedNodeGetAstNode(typed);

    const MAREA_SemaTypedNode *nameTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    const MAREA_SemaTypedNode *firstTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(1));

    const MAREA_SemaTypedNode *lastTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(2));

    const MAREA_SemaTypedNode *bodyTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(3));

    if ((nameTyped == MAREA_NULLPTR) || (firstTyped == MAREA_NULLPTR) || (lastTyped == MAREA_NULLPTR) || (bodyTyped == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(nameTyped);

    MAREA_IrNode *first = MAREA_NULLPTR;
    MAREA_IrNode *last = MAREA_NULLPTR;
    MAREA_IrNode *body = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, firstTyped, &first);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrLowerExpression(context, lastTyped, &last);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrLowerBlock(context, bodyTyped, &body);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if ((first == MAREA_NULLPTR) || (last == MAREA_NULLPTR) || (body == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *outer = MAREA_NULLPTR;
    MAREA_IrNode *local = MAREA_NULLPTR;
    MAREA_IrNode *loop = MAREA_NULLPTR;
    MAREA_IrNode *condition = MAREA_NULLPTR;
    MAREA_IrNode *increment = MAREA_NULLPTR;
    MAREA_IrNode *reference = MAREA_NULLPTR;
    MAREA_IrNode *one = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, forAst, &outer);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_LOCAL_DECLARATION, nameAst, &local);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_WHILE_STATEMENT, forAst, &loop);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BINARY_EXPRESSION, forAst, &condition);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_ASSIGNMENT, forAst, &increment);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_INTEGER_LITERAL, forAst, &one);
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: the hidden local is written by the increment, so it is a write target.
    local->flags = (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE;

    // INFO: a range includes both ends (docs/grammar/marea.ebnf), so the bound is <=, not <.
    condition->operatorKind = MAREA_TOKEN_KIND_LESS_EQUAL;
    increment->operatorKind = MAREA_TOKEN_KIND_PLUS_EQUAL;
    one->text = MAREA_G_IncrementText;
    one->textLength = (MAREA_Uinteger32)strlen(MAREA_G_IncrementText);

    result = MAREA_IrNodeAppendChild(context, local, first);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, nameAst, &reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, condition, reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, condition, last);
    }

    reference = MAREA_NULLPTR;

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, nameAst, &reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, increment, reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, increment, one);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, body, increment);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, loop, condition);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, loop, body);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, local);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, loop);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *outNode = outer;
    }

    return result;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    *outNode = MAREA_NULLPTR;

    switch (MAREA_IrTypedKind(typed)) {
    case MAREA_AST_KIND_BLOCK:
        return MAREA_IrLowerBlock(context, typed, outNode);
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return MAREA_G_LowerLocal(context, typed, outNode);
    case MAREA_AST_KIND_ASSIGNMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_ASSIGNMENT, typed, outNode);
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_EXPRESSION_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_IF_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_IF_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_ELSEIF_CLAUSE, typed, outNode);
    case MAREA_AST_KIND_ELSE_CLAUSE:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_ELSE_CLAUSE, typed, outNode);
    case MAREA_AST_KIND_WHILE_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_WHILE_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_RETURN_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_RETURN_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_BREAK_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_BREAK_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_CONTINUE_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_CONTINUE_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_SCOPE_BLOCK:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_SCOPE_BLOCK, typed, outNode);
    case MAREA_AST_KIND_UNSAFE_BLOCK:
        // INFO: the permission check already happened in sema, so an unsafe block is its plain BLOCK.
        return MAREA_IrLowerBlock(context, MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0)), outNode);
    case MAREA_AST_KIND_MACRO_STATEMENT:
        return MAREA_G_LowerMacroStatement(context, typed, outNode);
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return MAREA_G_LowerForRange(context, typed, outNode);
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_SWITCH, MAREA_SemaTypedNodeGetAstNode(typed), "A switch statement is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_FOR_EACH, MAREA_SemaTypedNodeGetAstNode(typed), "A for each statement is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
    case MAREA_AST_KIND_FORK_JOIN_BLOCK:
    case MAREA_AST_KIND_PARTITION_STATEMENT:
    case MAREA_AST_KIND_WITH_STATEMENT:
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_FORK_JOIN, MAREA_SemaTypedNodeGetAstNode(typed), "A fork_join, partition or with statement is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
    default:
        // INFO: any other kind is an expression; a Gap inside it raises its own diagnostic.
        return MAREA_IrLowerExpression(context, typed, outNode);
    }
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerBlock(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outNode == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    *outNode = MAREA_NULLPTR;

    if (MAREA_IrTypedKind(typed) != MAREA_AST_KIND_BLOCK) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_G_LowerKept(context, MAREA_IR_KIND_BLOCK, typed, outNode);
}
