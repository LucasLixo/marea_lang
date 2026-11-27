// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_SemaImportResult MAREA_CALL MAREA_G_Ask(const MAREA_SemaImportResolver *resolver, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_AstNode **outRoot)
{
    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    *outRoot = MAREA_NULLPTR;

    if ((resolver == MAREA_NULLPTR) || (resolver->resolve == MAREA_NULLPTR) || (text == MAREA_NULLPTR) || (length == 0)) {
        return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    }

    resolver->resolve(resolver->userData, text, length, &result, outRoot);

    return result;
}

MAREA_SemaImportResult MAREA_CALL MAREA_SemaResolveNamespace(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_AstNode **outRoot)
{
    const MAREA_SemaImportResult FIRST = MAREA_G_Ask(context->resolver, text, length, outRoot);

    if ((FIRST != MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) || (context->requireResolver == MAREA_NULLPTR)) {
        return FIRST;
    }

    return MAREA_G_Ask(context->requireResolver, text, length, outRoot);
}

MAREA_SemaImportResult MAREA_CALL MAREA_SemaResolveForKeyword(const MAREA_SemaContext *context, MAREA_SemaCoverKind kind, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_AstNode **outRoot)
{
    if ((kind == MAREA_SEMA_COVER_KIND_REQUIRE) && (context->requireResolver != MAREA_NULLPTR)) {
        return MAREA_G_Ask(context->requireResolver, text, length, outRoot);
    }

    return MAREA_G_Ask(context->resolver, text, length, outRoot);
}

const MAREA_SemaImportResolver *MAREA_CALL MAREA_SemaResolverOf(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_AstNode *root = MAREA_NULLPTR;

    if (MAREA_G_Ask(context->resolver, text, length, &root) != MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) {
        return context->resolver;
    }

    if ((context->requireResolver != MAREA_NULLPTR) && (MAREA_G_Ask(context->requireResolver, text, length, &root) != MAREA_SEMA_IMPORT_RESULT_NOT_FOUND)) {
        return context->requireResolver;
    }

    return MAREA_NULLPTR;
}

// INFO: true when a path equals a prefix or lies below it, cut at a dot.
static MAREA_Boolean MAREA_CALL MAREA_G_PathStartsWith(const MAREA_Character8 *path, MAREA_IntegerSize pathLength, const MAREA_Character8 *prefix, MAREA_IntegerSize prefixLength)
{
    if (pathLength < prefixLength) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (memcmp(path, prefix, prefixLength) != 0) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((pathLength == prefixLength) || (path[prefixLength] == '.')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaPathIsCovered(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < context->covers.count; ++index) {
        const MAREA_SemaCover *cover = &context->covers.entries[index];

        if (MAREA_G_PathStartsWith(text, length, cover->text, cover->length) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_AppendCover(MAREA_SemaContext *context, MAREA_SemaCover cover)
{
    MAREA_SemaCoverTable *table = &context->covers;

    if (table->count == table->capacity) {
        // ERROR: a capacity or byte size that does not fit in MAREA_IntegerSize would allocate a small block and then copy past it.
        if (table->capacity > (MAREA_INTEGER_SIZE_MAXIMUM / MAREA_INTEGER_SIZE(2))) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        const MAREA_IntegerSize NEW_CAPACITY = (table->capacity == 0) ? MAREA_INTEGER_SIZE(8) : (table->capacity * MAREA_INTEGER_SIZE(2));

        if (NEW_CAPACITY > (MAREA_INTEGER_SIZE_MAXIMUM / sizeof(MAREA_SemaCover))) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (NEW_CAPACITY * sizeof(MAREA_SemaCover)), MAREA_ALIGNOF(MAREA_SemaCover), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_SemaCover *entries = memory;

        if (table->count > 0) {
            memcpy(entries, table->entries, (table->count * sizeof(MAREA_SemaCover)));
        }

        table->entries = entries;
        table->capacity = NEW_CAPACITY;
    }

    table->entries[table->count] = cover;
    table->count += 1;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCoverAdd(MAREA_SemaContext *context, MAREA_SemaCoverKind kind, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((context == MAREA_NULLPTR) || (text == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Boolean known = MAREA_BOOLEAN_FALSE;

    MAREA_Boolean ambiguous = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 0; index < context->covers.count; ++index) {
        const MAREA_SemaCover *cover = &context->covers.entries[index];

        if (cover->kind == kind) {
            known = ((known == MAREA_BOOLEAN_TRUE) || (MAREA_SemaTextEquals(cover->text, cover->length, text, length) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

            continue;
        }

        // INFO: two paths meet when one lies below the other, so some full path is under both keywords.
        if ((MAREA_G_PathStartsWith(text, length, cover->text, cover->length) == MAREA_BOOLEAN_TRUE) || (MAREA_G_PathStartsWith(cover->text, cover->length, text, length) == MAREA_BOOLEAN_TRUE)) {
            ambiguous = MAREA_BOOLEAN_TRUE;
        }
    }

    if (ambiguous == MAREA_BOOLEAN_TRUE) {
        const MAREA_SemaExcept REPORT_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_AMBIGUOUS_NAMESPACE, position, "an import and a require cover the same path:", text, length);

        if (REPORT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return REPORT_RESULT;
        }
    }

    if (known == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_AppendCover(context, (MAREA_SemaCover){ .text = text, .length = length, .kind = kind });
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaChainJoin(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength, const MAREA_AstNode **outHead)
{
    if ((context == MAREA_NULLPTR) || (outText == MAREA_NULLPTR) || (outLength == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outText = MAREA_NULLPTR;
    *outLength = 0;

    if (outHead != MAREA_NULLPTR) {
        *outHead = MAREA_NULLPTR;
    }

    MAREA_IntegerSize total = 0;

    const MAREA_AstNode *current = node;

    // INFO: the chain is walked down its base side with a loop, because a chain of any length must not use the stack.
    while ((current != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(current) == MAREA_AST_KIND_MEMBER_ACCESS)) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(current, 1);

        if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_IDENTIFIER)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        total += (MAREA_AstNodeGetTextLength(member) + MAREA_INTEGER_SIZE(1));

        current = MAREA_SemaChildAt(current, 0);
    }

    if ((current == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(current) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    total += MAREA_AstNodeGetTextLength(current);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (total + MAREA_INTEGER_SIZE(1)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    MAREA_IntegerSize end = total;

    const MAREA_AstNode *cursor = node;

    while (MAREA_AstNodeGetKind(cursor) == MAREA_AST_KIND_MEMBER_ACCESS) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(cursor, 1);

        const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(member);

        end -= LENGTH;

        memcpy((joined + end), MAREA_AstNodeGetText(member), LENGTH);

        end -= MAREA_INTEGER_SIZE(1);

        joined[end] = '.';

        cursor = MAREA_SemaChildAt(cursor, 0);
    }

    memcpy(joined, MAREA_AstNodeGetText(cursor), MAREA_AstNodeGetTextLength(cursor));

    joined[total] = '\0';

    *outText = joined;
    *outLength = total;

    if (outHead != MAREA_NULLPTR) {
        *outHead = cursor;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the length of the first n segments of a dotted text, or the whole text when it has fewer.
static MAREA_IntegerSize MAREA_CALL MAREA_G_PrefixLength(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_IntegerSize segments)
{
    MAREA_IntegerSize seen = 0;

    for (MAREA_IntegerSize index = 0; index < length; ++index) {
        if (text[index] == '.') {
            seen += 1;

            if (seen == segments) {
                return index;
            }
        }
    }

    return length;
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_SegmentCount(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_IntegerSize count = (length == 0) ? MAREA_INTEGER_SIZE(0) : MAREA_INTEGER_SIZE(1);

    for (MAREA_IntegerSize index = 0; index < length; ++index) {
        if (text[index] == '.') {
            count += 1;
        }
    }

    return count;
}

typedef struct MAREA_G_PathReading
{
    const MAREA_Character8 *expanded;
    MAREA_IntegerSize expandedLength;
    MAREA_IntegerSize namespaceLength;
    MAREA_SemaPathStatus status;
} MAREA_G_PathReading;

static MAREA_SemaExcept MAREA_CALL MAREA_G_Classify(MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean lastIsMember, MAREA_G_PathReading *outReading)
{
    *outReading = (MAREA_G_PathReading){ .expanded = text, .expandedLength = length, .namespaceLength = 0, .status = MAREA_SEMA_PATH_STATUS_NOT_A_PATH };

    const MAREA_IntegerSize HEAD_LENGTH = MAREA_G_PrefixLength(text, length, 1);

    if ((MAREA_SemaLocalFind(context, text, HEAD_LENGTH) != MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, text, HEAD_LENGTH) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, HEAD_LENGTH);

    MAREA_Boolean aliased = MAREA_BOOLEAN_FALSE;

    if (entry != MAREA_NULLPTR) {
        if (entry->kind != MAREA_SEMA_NAME_KIND_NAMESPACE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_Character8 *aliasText = MAREA_NULLPTR;

        MAREA_IntegerSize aliasLength = 0;

        if (MAREA_SemaNamespaceOf(context, entry, &aliasText, &aliasLength) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        const MAREA_IntegerSize REST_LENGTH = (length - HEAD_LENGTH);

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (aliasLength + REST_LENGTH + MAREA_INTEGER_SIZE(1)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_Character8 *joined = memory;

        memcpy(joined, aliasText, aliasLength);

        if (REST_LENGTH > 0) {
            memcpy((joined + aliasLength), (text + HEAD_LENGTH), REST_LENGTH);
        }

        joined[(aliasLength + REST_LENGTH)] = '\0';

        outReading->expanded = joined;
        outReading->expandedLength = (aliasLength + REST_LENGTH);

        aliased = MAREA_BOOLEAN_TRUE;
    }

    const MAREA_IntegerSize SEGMENTS = MAREA_G_SegmentCount(outReading->expanded, outReading->expandedLength);

    const MAREA_IntegerSize LIMIT = (lastIsMember == MAREA_BOOLEAN_TRUE) ? (SEGMENTS - MAREA_INTEGER_SIZE(1)) : SEGMENTS;

    outReading->status = MAREA_SEMA_PATH_STATUS_UNKNOWN;

    for (MAREA_IntegerSize count = LIMIT; count >= MAREA_INTEGER_SIZE(1); --count) {
        const MAREA_IntegerSize PREFIX = MAREA_G_PrefixLength(outReading->expanded, outReading->expandedLength, count);

        const MAREA_AstNode *root = MAREA_NULLPTR;

        if (MAREA_SemaResolveNamespace(context, outReading->expanded, PREFIX, &root) != MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) {
            outReading->namespaceLength = PREFIX;
            outReading->status = ((aliased == MAREA_BOOLEAN_TRUE) || (MAREA_SemaPathIsCovered(context, outReading->expanded, outReading->expandedLength) == MAREA_BOOLEAN_TRUE)) ? MAREA_SEMA_PATH_STATUS_COVERED : MAREA_SEMA_PATH_STATUS_UNIMPORTED;

            break;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaClassifyPath(MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Boolean lastIsMember, MAREA_SemaPathStatus *outStatus, const MAREA_Character8 **outNamespace, MAREA_IntegerSize *outNamespaceLength)
{
    if ((context == MAREA_NULLPTR) || (text == MAREA_NULLPTR) || (outStatus == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_G_PathReading reading = { 0 };

    const MAREA_SemaExcept RESULT = MAREA_G_Classify(context, text, length, lastIsMember, &reading);

    *outStatus = reading.status;

    if (outNamespace != MAREA_NULLPTR) {
        *outNamespace = reading.expanded;
    }

    if (outNamespaceLength != MAREA_NULLPTR) {
        *outNamespaceLength = reading.namespaceLength;
    }

    return RESULT;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaBaseEntry(MAREA_SemaContext *context, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_SemaName **outEntry)
{
    if ((context == MAREA_NULLPTR) || (outEntry == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outEntry = MAREA_NULLPTR;

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_AstNode *head = MAREA_NULLPTR;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaChainJoin(context, base, &text, &length, &head);

    if ((JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return JOIN_RESULT;
    }

    const MAREA_Character8 *headText = MAREA_AstNodeGetText(head);

    const MAREA_IntegerSize HEAD_LENGTH = MAREA_AstNodeGetTextLength(head);

    if ((MAREA_SemaLocalFind(context, headText, HEAD_LENGTH) != MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, headText, HEAD_LENGTH) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (head == base) {
        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, length);

        if (entry != MAREA_NULLPTR) {
            *outEntry = entry;

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    }

    if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize MEMBER_LENGTH = MAREA_AstNodeGetTextLength(member);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (length + MEMBER_LENGTH + MAREA_INTEGER_SIZE(2)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    memcpy(joined, text, length);

    joined[length] = '.';

    memcpy((joined + length + MAREA_INTEGER_SIZE(1)), MAREA_AstNodeGetText(member), MEMBER_LENGTH);

    const MAREA_IntegerSize JOINED_LENGTH = (length + MEMBER_LENGTH + MAREA_INTEGER_SIZE(1));

    joined[JOINED_LENGTH] = '\0';

    MAREA_G_PathReading reading = { 0 };

    const MAREA_SemaExcept CLASSIFY_RESULT = MAREA_G_Classify(context, joined, JOINED_LENGTH, MAREA_BOOLEAN_TRUE, &reading);

    if ((CLASSIFY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (reading.status != MAREA_SEMA_PATH_STATUS_COVERED)) {
        return CLASSIFY_RESULT;
    }

    // INFO: only a whole namespace is a base for a call or a constant; a type or an enum before the member is another rule.
    if (reading.namespaceLength != (reading.expandedLength - MEMBER_LENGTH - MAREA_INTEGER_SIZE(1))) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_SemaName), MAREA_ALIGNOF(MAREA_SemaName), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaName *synthesized = memory;

    *synthesized = (MAREA_SemaName){ 0 };

    // INFO: the entry reads as the import of the namespace read: its last segment is the name, the rest is the qualifier.
    MAREA_IntegerSize nameStart = reading.namespaceLength;

    while ((nameStart > 0) && (reading.expanded[(nameStart - MAREA_INTEGER_SIZE(1))] != '.')) {
        nameStart -= 1;
    }

    synthesized->kind = MAREA_SEMA_NAME_KIND_NAMESPACE;
    synthesized->text = (reading.expanded + nameStart);
    synthesized->length = (reading.namespaceLength - nameStart);

    if (nameStart > 0) {
        synthesized->qualifier = reading.expanded;
        synthesized->qualifierLength = (nameStart - MAREA_INTEGER_SIZE(1));
    }

    *outEntry = synthesized;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
