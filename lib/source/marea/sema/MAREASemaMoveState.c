// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

typedef enum MAREA_G_BranchKind
{
    MAREA_G_BRANCH_KIND_NONE = MAREA_UINTEGER_8(0),
    MAREA_G_BRANCH_KIND_IF = MAREA_UINTEGER_8(1),
    MAREA_G_BRANCH_KIND_SWITCH = MAREA_UINTEGER_8(2),
    MAREA_G_BRANCH_KIND_LOOP = MAREA_UINTEGER_8(3),
} MAREA_G_BranchKind;

static MAREA_G_BranchKind MAREA_CALL MAREA_G_BranchKindOf(const MAREA_AstNode *node)
{
    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_IF_STATEMENT:
        return MAREA_G_BRANCH_KIND_IF;
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_BRANCH_KIND_SWITCH;
    case MAREA_AST_KIND_WHILE_STATEMENT:
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return MAREA_G_BRANCH_KIND_LOOP;
    default:
        return MAREA_G_BRANCH_KIND_NONE;
    }
}

// INFO: an arm is a child that runs on one path only: a block of an if, a clause of an if, an arm of a switch, or the body of a loop. The condition, the scrutinee and the bounds run before every arm.
static MAREA_Boolean MAREA_CALL MAREA_G_IsArm(MAREA_G_BranchKind branch, const MAREA_AstNode *child)
{
    if (child == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

    switch (branch) {
    case MAREA_G_BRANCH_KIND_IF:
        return ((KIND == MAREA_AST_KIND_BLOCK) || (KIND == MAREA_AST_KIND_ELSEIF_CLAUSE) || (KIND == MAREA_AST_KIND_ELSE_CLAUSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_G_BRANCH_KIND_SWITCH:
        return ((KIND == MAREA_AST_KIND_SWITCH_ARM) || (KIND == MAREA_AST_KIND_DEFAULT_ARM)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_G_BRANCH_KIND_LOOP:
        return (KIND == MAREA_AST_KIND_BLOCK) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ArmBlock(const MAREA_AstNode *arm)
{
    if (MAREA_AstNodeGetKind(arm) == MAREA_AST_KIND_BLOCK) {
        return arm;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arm);

    return (COUNT == 0) ? MAREA_NULLPTR : MAREA_SemaChildAt(arm, (COUNT - 1));
}

static MAREA_SemaName *MAREA_CALL MAREA_G_FindLocal(MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
            return local;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsIdentifier(const MAREA_AstNode *node)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_IDENTIFIER)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsMove(const MAREA_AstNode *node)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_KEYWORD_MOVE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsPlainAssignmentTarget(const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    return ((parent != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(parent) == MAREA_AST_KIND_ASSIGNMENT) && (index == 0) && (MAREA_AstNodeGetOperator(parent) == MAREA_TOKEN_KIND_EQUAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a name is read where the parent takes a value at that place. A declared name, a member name, a field key and a case label are not reads.
static MAREA_Boolean MAREA_CALL MAREA_G_IsValuePosition(const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    if (parent == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    switch (MAREA_AstNodeGetKind(parent)) {
    case MAREA_AST_KIND_ASSIGNMENT:
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
    case MAREA_AST_KIND_RETURN_STATEMENT:
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
    case MAREA_AST_KIND_ARGUMENT_LIST:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_INDEX_ENTRY:
    case MAREA_AST_KIND_VALUE_ENTRY:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_AST_KIND_IF_STATEMENT:
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
    case MAREA_AST_KIND_WHILE_STATEMENT:
    case MAREA_AST_KIND_SWITCH_STATEMENT:
    case MAREA_AST_KIND_WITH_STATEMENT:
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return (index == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return (index <= 1) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return ((index == 1) || (index == 2)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        // INFO: the children are the loop names, the collection and the body, so the collection is the second child from the end.
        return ((MAREA_AstNodeGetChildCount(parent) >= 2) && (index == (MAREA_AstNodeGetChildCount(parent) - 2)) && (index > 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FIELD_ENTRY:
        return (index == 1) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return (index == 2) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveCheckUse(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parent, MAREA_IntegerSize parentIndex)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((MAREA_G_IsIdentifier(node) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsValuePosition(parent, parentIndex) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsPlainAssignmentTarget(parent, parentIndex) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *local = MAREA_G_FindLocal(context, node);

    if ((local == MAREA_NULLPTR) || (local->moved == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_IsMove(parent) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_MOVED_AGAIN, MAREA_AstNodeGetPosition(node), "value is moved a second time:", MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_USE_OF_MOVED_VALUE, MAREA_AstNodeGetPosition(node), "value is used after it was moved:", MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
}

// INFO: only a local of a type that is not by value can be moved out. A primitive, an enum and a type that sema cannot see are copied, so a move of them leaves them valid.
void MAREA_CALL MAREA_SemaMoveRecord(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (MAREA_G_IsMove(node) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *operand = MAREA_SemaChildAt(node, 0);

        if (MAREA_G_IsIdentifier(operand) == MAREA_BOOLEAN_FALSE) {
            return;
        }

        MAREA_SemaName *local = MAREA_G_FindLocal(context, operand);

        if ((local == MAREA_NULLPTR) || (local->declaration == MAREA_NULLPTR) || (MAREA_SemaTypeIsByValue(context, local->declaration, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_TRUE)) {
            return;
        }

        local->moved = MAREA_BOOLEAN_TRUE;
        local->movedAt = node;

        return;
    }

    // INFO: a plain assignment to the whole local gives it a new value, so it is live again once the right side was read.
    if ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ASSIGNMENT) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_EQUAL)) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(node, 0);

        if (MAREA_G_IsIdentifier(target) == MAREA_BOOLEAN_FALSE) {
            return;
        }

        MAREA_SemaName *local = MAREA_G_FindLocal(context, target);

        if (local != MAREA_NULLPTR) {
            local->moved = MAREA_BOOLEAN_FALSE;
        }
    }
}

static void MAREA_CALL MAREA_G_Save(const MAREA_SemaContext *context, MAREA_Boolean *into, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = 0; index < count; ++index) {
        into[index] = context->locals.entries[index].moved;
    }
}

static void MAREA_CALL MAREA_G_Load(MAREA_SemaContext *context, const MAREA_Boolean *from, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = 0; index < count; ++index) {
        context->locals.entries[index].moved = from[index];
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaMoveFrame *outFrame)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outFrame == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outFrame = (MAREA_SemaMoveFrame){ 0 };

    const MAREA_G_BranchKind BRANCH = MAREA_G_BranchKindOf(node);

    if ((BRANCH == MAREA_G_BRANCH_KIND_NONE) || (context->locals.count == 0)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (context->locals.count * MAREA_INTEGER_SIZE(2) * sizeof(MAREA_Boolean)), MAREA_ALIGNOF(MAREA_Boolean), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    outFrame->count = context->locals.count;
    outFrame->base = memory;
    outFrame->merged = (outFrame->base + outFrame->count);

    MAREA_G_Save(context, outFrame->base, outFrame->count);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaMoveChildEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *child, MAREA_SemaMoveFrame *frame)
{
    if ((frame->base == MAREA_NULLPTR) || (MAREA_G_IsArm(MAREA_G_BranchKindOf(node), child) == MAREA_BOOLEAN_FALSE)) {
        return;
    }

    MAREA_G_Load(context, frame->base, frame->count);
}

// INFO: a body that can leave the loop on the same pass (a break or a return among its own statements) does not run the move twice.
static MAREA_Boolean MAREA_CALL MAREA_G_BodyLeaves(const MAREA_SemaContext *context, const MAREA_AstNode *body)
{
    if (body == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (MAREA_SemaBlockTerminates(context, body) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(body); ++index) {
        if (MAREA_AstNodeGetKind(MAREA_SemaChildAt(body, index)) == MAREA_AST_KIND_BREAK_STATEMENT) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportRepeatedMoves(MAREA_SemaContext *context, const MAREA_SemaMoveFrame *frame, const MAREA_AstNode *body)
{
    if (MAREA_G_BodyLeaves(context, body) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < frame->count; ++index) {
        const MAREA_SemaName *local = &context->locals.entries[index];

        if ((local->moved == MAREA_BOOLEAN_FALSE) || (frame->base[index] == MAREA_BOOLEAN_TRUE) || (local->movedAt == MAREA_NULLPTR)) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_VALUE_MOVED_AGAIN, MAREA_AstNodeGetPosition(local->movedAt), "value moved in a loop body is moved again on the next turn:", local->text, local->length);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaMoveChildLeave(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *child, MAREA_SemaMoveFrame *frame)
{
    if (frame->base == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_BranchKind BRANCH = MAREA_G_BranchKindOf(node);

    // INFO: a condition, a scrutinee or a bound runs before every arm, so what it moves is part of the base of the arms.
    if (MAREA_G_IsArm(BRANCH, child) == MAREA_BOOLEAN_FALSE) {
        MAREA_G_Save(context, frame->base, frame->count);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *block = MAREA_G_ArmBlock(child);

    if (BRANCH == MAREA_G_BRANCH_KIND_LOOP) {
        return MAREA_G_ReportRepeatedMoves(context, frame, block);
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

    if ((KIND == MAREA_AST_KIND_ELSE_CLAUSE) || (KIND == MAREA_AST_KIND_DEFAULT_ARM)) {
        frame->hasFallback = MAREA_BOOLEAN_TRUE;
    }

    // INFO: an arm that ends the function never reaches the code after the branch, so it takes no part in the merge.
    if ((block != MAREA_NULLPTR) && (MAREA_SemaBlockTerminates(context, block) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < frame->count; ++index) {
        const MAREA_Boolean MOVED = context->locals.entries[index].moved;

        if (frame->armSeen == MAREA_BOOLEAN_FALSE) {
            frame->merged[index] = MOVED;
        } else if (MOVED == MAREA_BOOLEAN_FALSE) {
            frame->merged[index] = MAREA_BOOLEAN_FALSE;
        }
    }

    frame->armSeen = MAREA_BOOLEAN_TRUE;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaMoveLeave(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaMoveFrame *frame)
{
    if (frame->base == MAREA_NULLPTR) {
        return;
    }

    const MAREA_G_BranchKind BRANCH = MAREA_G_BranchKindOf(node);

    // INFO: a loop body may run zero times, and an if with no else or a switch with no default has a path that moves nothing, so that path joins the merge with the base state.
    if ((BRANCH == MAREA_G_BRANCH_KIND_LOOP) || (frame->armSeen == MAREA_BOOLEAN_FALSE)) {
        MAREA_G_Load(context, frame->base, frame->count);

        return;
    }

    for (MAREA_IntegerSize index = 0; index < frame->count; ++index) {
        const MAREA_Boolean MERGED = (frame->hasFallback == MAREA_BOOLEAN_TRUE) ? frame->merged[index] : (((frame->merged[index] == MAREA_BOOLEAN_TRUE) && (frame->base[index] == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);

        context->locals.entries[index].moved = MERGED;
    }
}
