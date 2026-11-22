// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_SameText(const MAREA_AstNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return ((MAREA_AstNodeGetTextLength(node) == length) && (memcmp(MAREA_AstNodeGetText(node), text, length) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_SemaName *MAREA_CALL MAREA_G_FindLocal(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        const MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_G_SameText(name, local->text, local->length) == MAREA_BOOLEAN_TRUE) {
            return local;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsPartitionPart(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = context->references.count; index > 0; --index) {
        const MAREA_SemaName *entry = &context->references.entries[(index - 1)];

        if (MAREA_G_SameText(name, entry->text, entry->length) == MAREA_BOOLEAN_TRUE) {
            return ((entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_PARTITION_STATEMENT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsFileConstant(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if (MAREA_G_FindLocal(context, name) != MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) && (entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_CONSTANT_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsWriteBorrowParameter(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_AstNode *parameters = MAREA_NULLPTR;

    for (MAREA_IntegerSize index = 0; ((context->function != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(context->function))); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(context->function, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_PARAMETER_LIST)) {
            parameters = child;
        }
    }

    for (MAREA_IntegerSize index = 0; ((parameters != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(parameters))); ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_AstNode *parameterName = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 0);

        if ((parameterName == MAREA_NULLPTR) || (MAREA_G_SameText(name, MAREA_AstNodeGetText(parameterName), MAREA_AstNodeGetTextLength(parameterName)) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_SemaName *local = MAREA_G_FindLocal(context, name);

        const MAREA_Boolean SHADOWED = ((local != MAREA_NULLPTR) && (local->declaration != MAREA_SemaChildAt(parameter, 1))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        return ((SHADOWED == MAREA_BOOLEAN_FALSE) && ((MAREA_AstNodeGetFlags(parameter) & MAREA_AST_FLAG_MUTABLE) != 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsMutablePartitionSource(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = 0; index < context->references.count; ++index) {
        const MAREA_AstNode *partition = context->references.entries[index].declaration;

        if ((partition == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(partition) != MAREA_AST_KIND_PARTITION_STATEMENT) || ((MAREA_AstNodeGetFlags(partition) & MAREA_AST_FLAG_MUTABLE) == 0)) {
            continue;
        }

        const MAREA_AstNode *source = MAREA_SemaChildAt(partition, 0);

        if ((source != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(source) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_G_SameText(name, MAREA_AstNodeGetText(source), MAREA_AstNodeGetTextLength(source)) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsValidArgument(const MAREA_SemaContext *context, const MAREA_AstNode *argument)
{
    switch (MAREA_AstNodeGetKind(argument)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_AST_KIND_IDENTIFIER:
        if ((MAREA_G_IsPartitionPart(context, argument) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsFileConstant(context, argument) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }

        return ((MAREA_G_IsWriteBorrowParameter(context, argument) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsMutablePartitionSource(context, argument) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
    case MAREA_AST_KIND_MEMBER_ACCESS:
    {
        // INFO: a field is as safe to share as the value it is read from.
        const MAREA_AstNode *base = MAREA_SemaChildAt(argument, 0);

        return ((base != MAREA_NULLPTR) && (MAREA_G_IsValidArgument(context, base) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckSpawn(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (context->forkDepth == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *call = MAREA_SemaChildAt(node, 0);

    if ((call == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(call) != MAREA_AST_KIND_CALL_EXPRESSION)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(call, (MAREA_AstNodeGetChildCount(call) - 1));

    if ((arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(arguments) != MAREA_AST_KIND_ARGUMENT_LIST)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(arguments); ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((argument == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_ERROR_NODE) || (MAREA_G_IsValidArgument(context, argument) == MAREA_BOOLEAN_TRUE)) {
            continue;
        }

        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT, MAREA_AstNodeGetPosition(argument), "a spawn argument must be a part, a literal, a constant or a name that is not a write borrow");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckJump(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: a fork_join block is a join barrier too, so a jump directly in it, outside every partition body, must not leave it either, the same as a jump leaving a partition body.
    if ((context->partitionDepth == 0) && (context->forkDepth == 0)) {
        // INFO: outside any parallel construct, a break or continue still needs an enclosing loop; a return does not.
        const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

        if ((context->loopDepth == 0) && ((KIND == MAREA_AST_KIND_BREAK_STATEMENT) || (KIND == MAREA_AST_KIND_CONTINUE_STATEMENT))) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_JUMP_OUTSIDE_LOOP, MAREA_AstNodeGetPosition(node), (KIND == MAREA_AST_KIND_BREAK_STATEMENT) ? "a break must be inside a loop" : "a continue must be inside a loop");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean IN_PARTITION = (context->partitionDepth > 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_RETURN_STATEMENT:
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP, MAREA_AstNodeGetPosition(node), (IN_PARTITION == MAREA_BOOLEAN_TRUE) ? "a return cannot leave the body of a partition" : "a return cannot leave a fork_join block before its workers join");
        break;
    case MAREA_AST_KIND_BREAK_STATEMENT:
        if (context->loopDepth == 0) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP, MAREA_AstNodeGetPosition(node), (IN_PARTITION == MAREA_BOOLEAN_TRUE) ? "a break cannot leave the body of a partition" : "a break cannot leave a fork_join block before its workers join");
        }

        break;
    case MAREA_AST_KIND_CONTINUE_STATEMENT:
        if (context->loopDepth == 0) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP, MAREA_AstNodeGetPosition(node), (IN_PARTITION == MAREA_BOOLEAN_TRUE) ? "a continue cannot leave the body of a partition" : "a continue cannot leave a fork_join block before its workers join");
        }

        break;
    default:
        break;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
