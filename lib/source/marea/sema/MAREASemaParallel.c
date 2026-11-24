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

        const MAREA_Boolean shadowed = ((local != MAREA_NULLPTR) && (local->declaration != MAREA_SemaChildAt(parameter, 1))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        return ((shadowed == MAREA_BOOLEAN_FALSE) && (MAREA_SemaGetParameterSemantics(context, parameter) == MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
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
    case MAREA_AST_KIND_CHARACTER_LITERAL:
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
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN, MAREA_AstNodeGetPosition(node), "a spawn needs an enclosing fork_join block");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *call = MAREA_SemaChildAt(node, 0);

    if (call == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_AstNodeGetKind(call) != MAREA_AST_KIND_CALL_EXPRESSION) {
        // INFO: a macro call is not a function call, so it is rejected too; a broken operand raises nothing here.
        if (MAREA_AstNodeGetKind(call) != MAREA_AST_KIND_ERROR_NODE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_SPAWN_NEEDS_CALL, MAREA_AstNodeGetPosition(call), "a spawn needs a call");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(call, (MAREA_AstNodeGetChildCount(call) - 1));

    if ((arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(arguments) != MAREA_AST_KIND_ARGUMENT_LIST)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize OWN_START = context->captures.count;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(arguments); ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((argument == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_ERROR_NODE)) {
            continue;
        }

        if (MAREA_G_IsValidArgument(context, argument) == MAREA_BOOLEAN_TRUE) {
            const MAREA_SemaExcept CAPTURE_RESULT = MAREA_SemaCaptureArgument(context, call, index, argument, OWN_START);

            if (CAPTURE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return CAPTURE_RESULT;
            }

            continue;
        }

        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT, MAREA_AstNodeGetPosition(argument), "a spawn argument must be a part, a literal, a constant or a name that is not a write borrow");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

typedef struct MAREA_PartitionScan
{
    MAREA_SemaContext *context;
    const MAREA_AstNode *body;
    const MAREA_AstNode *partName;
    const MAREA_AstNode *source;
} MAREA_PartitionScan;

static MAREA_Boolean MAREA_CALL MAREA_G_SameNode(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    return ((left != MAREA_NULLPTR) && (right != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(left) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_AstNodeGetKind(right) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_G_SameText(left, MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(right)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the number of children of a node that are declaration names, so a scan never reads them as uses; outFirst is the index of the first.
static MAREA_IntegerSize MAREA_CALL MAREA_G_DeclarationCount(const MAREA_AstNode *node, MAREA_IntegerSize *outFirst)
{
    *outFirst = 0;

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_LOCAL_DECLARATION:
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return 1;
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        return (MAREA_AstNodeGetChildCount(node) > 2) ? (MAREA_AstNodeGetChildCount(node) - 2) : 0;
    case MAREA_AST_KIND_WITH_STATEMENT:
        *outFirst = 1;
        return 1;
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        *outFirst = 2;
        return 1;
    default:
        return 0;
    }
}

// INFO: walks in source order and stops at "stop"; a name declared before it, in any block, is a name of the body for the rest of the scan.
static MAREA_Boolean MAREA_CALL MAREA_G_DeclaredBefore(const MAREA_AstNode *node, const MAREA_AstNode *name, const MAREA_AstNode *stop, MAREA_Boolean *reached)
{
    if (node == stop) {
        *reached = MAREA_BOOLEAN_TRUE;

        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize first = 0;
    const MAREA_IntegerSize COUNT = MAREA_G_DeclarationCount(node, &first);

    for (MAREA_IntegerSize index = first; index < (first + COUNT); ++index) {
        if (MAREA_G_SameNode(MAREA_SemaChildAt(node, index), name) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_G_DeclaredBefore(child, name, stop, reached) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }

        if (*reached == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBodyName(const MAREA_PartitionScan *scan, const MAREA_AstNode *name, const MAREA_AstNode *at)
{
    MAREA_Boolean reached = MAREA_BOOLEAN_FALSE;

    return ((MAREA_G_SameNode(scan->partName, name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_DeclaredBefore(scan->body, name, at, &reached) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_RootName(const MAREA_AstNode *target)
{
    while ((target != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(target) == MAREA_AST_KIND_MEMBER_ACCESS) || (MAREA_AstNodeGetKind(target) == MAREA_AST_KIND_INDEX_ACCESS) || (MAREA_AstNodeGetKind(target) == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION))) {
        target = MAREA_SemaChildAt(target, 0);
    }

    return ((target != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(target) == MAREA_AST_KIND_IDENTIFIER)) ? target : MAREA_NULLPTR;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ScanPartition(const MAREA_PartitionScan *scan, const MAREA_AstNode *node)
{
    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    // INFO: a type is not a value, and the arguments of a spawn belong to rule 3020.
    if ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE) || (KIND == MAREA_AST_KIND_GENERIC_NAME) || (KIND == MAREA_AST_KIND_QUALIFIED_NAME) || ((KIND == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_KEYWORD_SPAWN))) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_IDENTIFIER) {
        if ((scan->source != MAREA_NULLPTR) && (MAREA_G_SameNode(scan->source, node) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsBodyName(scan, node, node) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_SemaReportNamed(scan->context, MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_SOURCE_USED_IN_BODY, MAREA_AstNodeGetPosition(node), "the source of a partition mutable cannot be used in the body:", MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_ASSIGNMENT) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(node, 0);
        const MAREA_AstNode *root = MAREA_G_RootName(target);

        // INFO: the source of a partition mutable is rule 3076's; a file constant or an unknown name is not a local.
        if ((root != MAREA_NULLPTR) && (MAREA_G_SameNode(scan->source, root) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsBodyName(scan, root, target) == MAREA_BOOLEAN_FALSE) && (MAREA_G_FindLocal(scan->context, root) != MAREA_NULLPTR)) {
            const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(scan->context, MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_BODY_WRITES_OUTER_NAME, MAREA_AstNodeGetPosition(target), "a partition body cannot write an outer name:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    }

    MAREA_IntegerSize first = 0;
    const MAREA_IntegerSize DECLARED = MAREA_G_DeclarationCount(node, &first);
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        // INFO: a declared name, a member name, a field key and a macro name are not uses of a value; the body of a nested partition has its own scan.
        const MAREA_Boolean SKIP = (((index >= first) && (index < (first + DECLARED))) || ((KIND == MAREA_AST_KIND_MEMBER_ACCESS) && (index == 1)) || ((KIND == MAREA_AST_KIND_FIELD_ENTRY) && (index == 0)) || ((KIND == MAREA_AST_KIND_MACRO_CALL) && (index == 0)) || ((KIND == MAREA_AST_KIND_PARTITION_STATEMENT) && (index == 3))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        if ((child == MAREA_NULLPTR) || (SKIP == MAREA_BOOLEAN_TRUE)) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_G_ScanPartition(scan, child);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckPartition(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: the scope check comes first; a construct that fails it raises no other scope or shape code.
    if (context->forkDepth == 0) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN, MAREA_AstNodeGetPosition(node), "a partition needs an enclosing fork_join block");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *source = MAREA_SemaChildAt(node, 0);

    if ((source != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(source) != MAREA_AST_KIND_ERROR_NODE) && (MAREA_SemaExpressionIsKnownNotArray(context, source) == MAREA_BOOLEAN_TRUE)) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_SOURCE_NOT_ARRAY, MAREA_AstNodeGetPosition(source), "a partition needs an array");
    }

    const MAREA_AstNode *body = MAREA_SemaChildAt(node, (MAREA_AstNodeGetChildCount(node) - 1));

    if ((body != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(body) == MAREA_AST_KIND_BLOCK)) {
        const MAREA_Boolean IS_MUTABLE = ((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) != 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        const MAREA_PartitionScan SCAN = { .context = context, .body = body, .partName = MAREA_SemaChildAt(node, 2), .source = ((IS_MUTABLE == MAREA_BOOLEAN_TRUE) && (source != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(source) == MAREA_AST_KIND_IDENTIFIER)) ? source : MAREA_NULLPTR };

        const MAREA_SemaExcept SCAN_RESULT = MAREA_G_ScanPartition(&SCAN, body);

        if (SCAN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return SCAN_RESULT;
        }
    }

    return MAREA_SemaCapturePartition(context, node);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckJump(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: a fork_join block is a join barrier too, so a jump directly in it, outside every partition body, must not leave it either, the same as a jump leaving a partition body.
    if ((context->partitionDepth == 0) && (context->forkDepth == 0)) {
        // INFO: outside any parallel construct, a break or continue still needs an enclosing loop; a return does not.
        const MAREA_AstKind kind = MAREA_AstNodeGetKind(node);

        if ((context->loopDepth == 0) && ((kind == MAREA_AST_KIND_BREAK_STATEMENT) || (kind == MAREA_AST_KIND_CONTINUE_STATEMENT))) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_JUMP_OUTSIDE_LOOP, MAREA_AstNodeGetPosition(node), (kind == MAREA_AST_KIND_BREAK_STATEMENT) ? "a break must be inside a loop" : "a continue must be inside a loop");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean inPartition = (context->partitionDepth > 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_RETURN_STATEMENT:
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP, MAREA_AstNodeGetPosition(node), (inPartition == MAREA_BOOLEAN_TRUE) ? "a return cannot leave the body of a partition" : "a return cannot leave a fork_join block before its workers join");
        break;
    case MAREA_AST_KIND_BREAK_STATEMENT:
        if (context->loopDepth == 0) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP, MAREA_AstNodeGetPosition(node), (inPartition == MAREA_BOOLEAN_TRUE) ? "a break cannot leave the body of a partition" : "a break cannot leave a fork_join block before its workers join");
        }

        break;
    case MAREA_AST_KIND_CONTINUE_STATEMENT:
        if (context->loopDepth == 0) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP, MAREA_AstNodeGetPosition(node), (inPartition == MAREA_BOOLEAN_TRUE) ? "a continue cannot leave the body of a partition" : "a continue cannot leave a fork_join block before its workers join");
        }

        break;
    default:
        break;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
