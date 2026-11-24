// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_PATH_LIMIT MAREA_INTEGER_SIZE(16)

// INFO: the segments of a place, root first: the root name, then each field name. An index access ends the path, so an element counts as its whole collection.
typedef struct MAREA_G_Path
{
    const MAREA_AstNode *segments[MAREA_G_PATH_LIMIT];
    MAREA_IntegerSize count;
} MAREA_G_Path;

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsChain(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_INDEX_ACCESS) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_RootOf(const MAREA_AstNode *place)
{
    const MAREA_AstNode *root = place;

    while (MAREA_G_IsChain(root) == MAREA_BOOLEAN_TRUE) {
        root = MAREA_SemaChildAt(root, 0);
    }

    return (MAREA_G_IsKind(root, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) ? root : MAREA_NULLPTR;
}

static void MAREA_CALL MAREA_G_Flatten(const MAREA_AstNode *place, MAREA_G_Path *path)
{
    const MAREA_AstNode *chain[MAREA_G_PATH_LIMIT * 4];

    MAREA_IntegerSize chainCount = 0;

    const MAREA_AstNode *node = place;

    while ((MAREA_G_IsChain(node) == MAREA_BOOLEAN_TRUE) && (chainCount < (MAREA_G_PATH_LIMIT * 4))) {
        chain[chainCount] = node;
        chainCount += 1;
        node = MAREA_SemaChildAt(node, 0);
    }

    path->segments[0] = MAREA_G_RootOf(place);
    path->count = 1;

    for (MAREA_IntegerSize index = chainCount; index > 0; --index) {
        const MAREA_AstNode *link = chain[(index - 1)];

        if ((MAREA_AstNodeGetKind(link) == MAREA_AST_KIND_INDEX_ACCESS) || (path->count == MAREA_G_PATH_LIMIT)) {
            break;
        }

        path->segments[path->count] = MAREA_SemaChildAt(link, 1);
        path->count += 1;
    }
}

// INFO: two places overlap when one path is the start of the other, so a value and a field of it overlap and two fields of it do not.
static MAREA_Boolean MAREA_CALL MAREA_G_Overlap(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    MAREA_G_Path leftPath;
    MAREA_G_Path rightPath;

    MAREA_G_Flatten(left, &leftPath);
    MAREA_G_Flatten(right, &rightPath);

    const MAREA_IntegerSize SHARED = (leftPath.count < rightPath.count) ? leftPath.count : rightPath.count;

    for (MAREA_IntegerSize index = 0; index < SHARED; ++index) {
        const MAREA_AstNode *a = leftPath.segments[index];

        const MAREA_AstNode *b = rightPath.segments[index];

        if ((a == MAREA_NULLPTR) || (b == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(a), MAREA_AstNodeGetTextLength(a), MAREA_AstNodeGetText(b), MAREA_AstNodeGetTextLength(b)) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: finds the local that a name stands for and tells the value apart from another of the same name. A part, a loop name and a write-borrowed parameter are known by the node that declared them, any other local by its type node.
static MAREA_Boolean MAREA_CALL MAREA_G_Identify(const MAREA_SemaContext *context, const MAREA_AstNode *root, const MAREA_AstNode **outIdentity, MAREA_IntegerSize *outIndex)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(root);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(root);

    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        const MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, text, LENGTH) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        *outIdentity = local->declaration;
        *outIndex = (index - 1);

        for (MAREA_IntegerSize reference = context->references.count; reference > 0; --reference) {
            const MAREA_SemaName *entry = &context->references.entries[(reference - 1)];

            if ((entry->declaration == MAREA_NULLPTR) || (MAREA_SemaTextEquals(entry->text, entry->length, text, LENGTH) == MAREA_BOOLEAN_FALSE)) {
                continue;
            }

            // INFO: a local of the same name that hides a parameter has its own type node, so it is not the parameter.
            if ((MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_PARAMETER) || (local->declaration == MAREA_SemaChildAt(entry->declaration, 1))) {
                *outIdentity = entry->declaration;
            }

            break;
        }

        return MAREA_BOOLEAN_TRUE;
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_Append(MAREA_SemaContext *context, MAREA_SemaCapture entry)
{
    MAREA_SemaCaptureTable *table = &context->captures;

    if (table->count == table->capacity) {
        if (table->capacity > (MAREA_INTEGER_SIZE_MAXIMUM / MAREA_INTEGER_SIZE(2))) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        const MAREA_IntegerSize NEW_CAPACITY = (table->capacity == 0) ? MAREA_INTEGER_SIZE(8) : (table->capacity * MAREA_INTEGER_SIZE(2));

        if (NEW_CAPACITY > (MAREA_INTEGER_SIZE_MAXIMUM / sizeof(MAREA_SemaCapture))) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (NEW_CAPACITY * sizeof(MAREA_SemaCapture)), MAREA_ALIGNOF(MAREA_SemaCapture), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_SemaCapture *entries = memory;

        if (table->count > 0) {
            memcpy(entries, table->entries, (table->count * sizeof(MAREA_SemaCapture)));
        }

        table->entries = entries;
        table->capacity = NEW_CAPACITY;
    }

    table->entries[table->count] = entry;
    table->count += 1;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the earlier capture of the first LIMIT entries that the place conflicts with: the same value, overlapping, and at least one of the two a write.
static const MAREA_SemaCapture *MAREA_CALL MAREA_G_FindConflict(const MAREA_SemaContext *context, const MAREA_AstNode *place, const MAREA_AstNode *identity, MAREA_Boolean write, MAREA_IntegerSize limit)
{
    const MAREA_AstNode *root = MAREA_G_RootOf(place);

    for (MAREA_IntegerSize index = 0; index < limit; ++index) {
        const MAREA_SemaCapture *entry = &context->captures.entries[index];

        const MAREA_AstNode *entryRoot = MAREA_G_RootOf(entry->place);

        if ((entry->identity != identity) || ((write == MAREA_BOOLEAN_FALSE) && (entry->write == MAREA_BOOLEAN_FALSE)) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root), MAREA_AstNodeGetText(entryRoot), MAREA_AstNodeGetTextLength(entryRoot)) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        if (MAREA_G_Overlap(place, entry->place) == MAREA_BOOLEAN_TRUE) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_CalleeParameters(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee)) != MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (MAREA_G_IsKind(entry->declaration, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(entry->declaration); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(entry->declaration, index);

        if (MAREA_G_IsKind(child, MAREA_AST_KIND_PARAMETER_LIST) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: how a call uses the value at one argument place: 0 for a copy or a move (not shared), 1 for a read borrow, 2 for a write borrow.
static MAREA_IntegerSize MAREA_CALL MAREA_G_ArgumentUse(const MAREA_SemaContext *context, const MAREA_AstNode *parameters, MAREA_IntegerSize index)
{
    const MAREA_AstNode *parameter = (parameters == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameters, index);

    if (parameter == MAREA_NULLPTR) {
        return 0;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    // INFO: an owned, task or vector parameter takes its argument by move, so nothing is shared.
    if (MAREA_SemaClassifyType(context, MAREA_SemaChildAt(parameter, 1), &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
        return 0;
    }

    switch (MAREA_SemaGetParameterSemantics(context, parameter)) {
    case MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE:
        return 2;
    case MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_READ_ONLY:
        return 1;
    default:
        return 0;
    }
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportConflict(MAREA_SemaContext *context, const MAREA_AstNode *place, const MAREA_Character8 *prefix)
{
    const MAREA_AstNode *root = MAREA_G_RootOf(place);

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_PARALLEL_ACCESS, MAREA_AstNodeGetPosition(place), prefix, MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCaptureArgument(MAREA_SemaContext *context, const MAREA_AstNode *call, MAREA_IntegerSize index, const MAREA_AstNode *argument, MAREA_IntegerSize ownStart)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR) || (argument == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *root = MAREA_G_RootOf(argument);

    const MAREA_IntegerSize USE = MAREA_G_ArgumentUse(context, MAREA_G_CalleeParameters(context, call), index);

    const MAREA_AstNode *identity = MAREA_NULLPTR;

    MAREA_IntegerSize localIndex = 0;

    if ((root == MAREA_NULLPTR) || (USE == 0) || (MAREA_G_Identify(context, root, &identity, &localIndex) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean WRITE = (USE == 2) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (MAREA_G_FindConflict(context, argument, identity, WRITE, ownStart) != MAREA_NULLPTR) {
        const MAREA_SemaExcept RESULT = MAREA_G_ReportConflict(context, argument, "a worker writes or reads a value that another worker of the block uses, and one of them writes it:");

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    } else if ((WRITE == MAREA_BOOLEAN_TRUE) && ((context->loopDepth > 0) || (context->partitionDepth > 0)) && (localIndex < context->captureLocalBase)) {
        // INFO: a spawn that runs once for each turn of a loop, or once for each part, starts a second writer of a value that was declared before the block.
        const MAREA_SemaExcept RESULT = MAREA_G_ReportConflict(context, argument, "a spawn that repeats writes the same value in each turn:");

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_G_Append(context, (MAREA_SemaCapture){ .identity = identity, .place = argument, .write = WRITE });
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCapturePartition(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *source = MAREA_SemaChildAt(node, 0);

    const MAREA_AstNode *root = MAREA_G_RootOf(source);

    const MAREA_AstNode *identity = MAREA_NULLPTR;

    MAREA_IntegerSize localIndex = 0;

    if ((root == MAREA_NULLPTR) || (MAREA_G_Identify(context, root, &identity, &localIndex) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean WRITE = ((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) != 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (MAREA_G_FindConflict(context, source, identity, WRITE, context->captures.count) != MAREA_NULLPTR) {
        const MAREA_SemaExcept RESULT = MAREA_G_ReportConflict(context, source, "a partition takes a value that a worker of the block uses, and one of them writes it:");

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_G_Append(context, (MAREA_SemaCapture){ .identity = identity, .place = source, .write = WRITE });
}

// INFO: one use of a place by the block body, against everything the workers took.
static MAREA_SemaExcept MAREA_CALL MAREA_G_Mention(MAREA_SemaContext *context, const MAREA_AstNode *place, MAREA_Boolean write)
{
    const MAREA_AstNode *root = MAREA_G_RootOf(place);

    const MAREA_AstNode *identity = MAREA_NULLPTR;

    MAREA_IntegerSize localIndex = 0;

    if ((root == MAREA_NULLPTR) || (MAREA_G_Identify(context, root, &identity, &localIndex) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_FindConflict(context, place, identity, write, context->captures.count) == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportConflict(context, place, "the block uses a value that a worker uses, and one of them writes it:");
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_Scan(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Boolean write);

static MAREA_SemaExcept MAREA_CALL MAREA_G_ScanChildren(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_IntegerSize first, MAREA_IntegerSize end)
{
    for (MAREA_IntegerSize index = first; index < end; ++index) {
        const MAREA_SemaExcept RESULT = MAREA_G_Scan(context, MAREA_SemaChildAt(node, index), MAREA_BOOLEAN_FALSE);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ScanCall(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *parameters = MAREA_G_CalleeParameters(context, call);

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(call); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(call, index);

        MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

        if (MAREA_G_IsKind(child, MAREA_AST_KIND_ARGUMENT_LIST) == MAREA_BOOLEAN_TRUE) {
            for (MAREA_IntegerSize argument = 0; ((argument < MAREA_AstNodeGetChildCount(child)) && (result == MAREA_SEMA_EXCEPT_SUCCESS)); ++argument) {
                result = MAREA_G_Scan(context, MAREA_SemaChildAt(child, argument), (MAREA_G_ArgumentUse(context, parameters, argument) == 2) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
            }
        } else if ((index > 0) && (MAREA_G_IsKind(child, MAREA_AST_KIND_TYPE_ARGUMENTS) == MAREA_BOOLEAN_FALSE)) {
            result = MAREA_G_Scan(context, child, MAREA_BOOLEAN_FALSE);
        } else if ((index == 0) && (MAREA_G_IsKind(child, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE)) {
            result = MAREA_G_Scan(context, child, MAREA_BOOLEAN_FALSE);
        }

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: walks the code of the block itself. A spawn, a partition and a nested fork_join are not part of it: a spawn and a partition are workers, and a nested block compares its own statements when the visit reaches them.
static MAREA_SemaExcept MAREA_CALL MAREA_G_Scan(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Boolean write)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_PARTITION_STATEMENT:
    case MAREA_AST_KIND_FORK_JOIN_BLOCK:
    case MAREA_AST_KIND_NAMED_TYPE:
    case MAREA_AST_KIND_FUNCTION_TYPE:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        if (MAREA_AstNodeGetOperator(node) == MAREA_TOKEN_KIND_KEYWORD_SPAWN) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_G_ScanChildren(context, node, 0, COUNT);
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_Mention(context, node, write);
    case MAREA_AST_KIND_MEMBER_ACCESS:
    case MAREA_AST_KIND_INDEX_ACCESS:
    {
        if (MAREA_G_RootOf(node) == MAREA_NULLPTR) {
            return MAREA_G_ScanChildren(context, node, 0, COUNT);
        }

        const MAREA_SemaExcept RESULT = MAREA_G_Mention(context, node, write);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }

        // INFO: the operands of the index accesses of the chain are plain reads.
        for (const MAREA_AstNode *link = node; MAREA_G_IsChain(link) == MAREA_BOOLEAN_TRUE; link = MAREA_SemaChildAt(link, 0)) {
            if (MAREA_AstNodeGetKind(link) == MAREA_AST_KIND_INDEX_ACCESS) {
                const MAREA_SemaExcept INDEX_RESULT = MAREA_G_ScanChildren(context, link, 1, MAREA_AstNodeGetChildCount(link));

                if (INDEX_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return INDEX_RESULT;
                }
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_ASSIGNMENT:
    {
        const MAREA_SemaExcept TARGET_RESULT = MAREA_G_Scan(context, MAREA_SemaChildAt(node, 0), MAREA_BOOLEAN_TRUE);

        return (TARGET_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) ? TARGET_RESULT : MAREA_G_ScanChildren(context, node, 1, COUNT);
    }
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_ScanCall(context, node);
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return MAREA_G_ScanChildren(context, node, 2, COUNT);
    case MAREA_AST_KIND_FIELD_ENTRY:
        return MAREA_G_ScanChildren(context, node, 1, COUNT);
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
    case MAREA_AST_KIND_WITH_STATEMENT:
        return MAREA_G_ScanChildren(context, node, 1, COUNT);
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    {
        // INFO: the children are the loop names, the collection and the body; a mutable loop writes what it walks.
        if (COUNT < 3) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_SemaExcept COLLECTION_RESULT = MAREA_G_Scan(context, MAREA_SemaChildAt(node, (COUNT - 2)), ((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) != 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);

        return (COLLECTION_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) ? COLLECTION_RESULT : MAREA_G_Scan(context, MAREA_SemaChildAt(node, (COUNT - 1)), MAREA_BOOLEAN_FALSE);
    }
    default:
        return MAREA_G_ScanChildren(context, node, 0, COUNT);
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCaptureBody(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parent)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: only a statement directly in the block of a fork_join is the code of the block, and only a worker that started before it can conflict with it.
    if ((parent == MAREA_NULLPTR) || (parent != context->forkBody) || (context->captures.count == 0)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_Scan(context, node, MAREA_BOOLEAN_FALSE);
}
