// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_TextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength)
{
    if (leftLength != rightLength) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(left, right, leftLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsDeclarationKind(MAREA_AstKind kind)
{
    switch (kind) {
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
    case MAREA_AST_KIND_TYPE_DECLARATION:
    case MAREA_AST_KIND_STRUCTURE_DECLARATION:
    case MAREA_AST_KIND_OPAQUE_DECLARATION:
    case MAREA_AST_KIND_ENUM_DECLARATION:
    case MAREA_AST_KIND_CONTRACT_DECLARATION:
    case MAREA_AST_KIND_FUNCTION_DECLARATION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index)
{
    const MAREA_AstNode *child = MAREA_NULLPTR;

    if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

MAREA_Boolean MAREA_CALL MAREA_SemaDeclarationName(const MAREA_AstNode *node, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_IsDeclarationKind(MAREA_AstNodeGetKind(node)) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_G_ChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_IDENTIFIER)) {
            *outText = MAREA_AstNodeGetText(child);
            *outLength = MAREA_AstNodeGetTextLength(child);

            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaQualifiedNameJoin(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((context == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (outText == MAREA_NULLPTR) || (outLength == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(name);

    MAREA_IntegerSize total = 0;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        total += (MAREA_AstNodeGetTextLength(MAREA_G_ChildAt(name, index)) + MAREA_INTEGER_SIZE(1));
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (total + MAREA_INTEGER_SIZE(1)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *joined = memory;

    MAREA_IntegerSize length = 0;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *part = MAREA_G_ChildAt(name, index);

        if (index > 0) {
            joined[length] = '.';
            length += 1;
        }

        const MAREA_IntegerSize PART_LENGTH = MAREA_AstNodeGetTextLength(part);

        if (PART_LENGTH > 0) {
            memcpy((joined + length), MAREA_AstNodeGetText(part), PART_LENGTH);
        }

        length += PART_LENGTH;
    }

    joined[length] = '\0';

    *outText = joined;
    *outLength = length;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

const MAREA_SemaName *MAREA_CALL MAREA_SemaNameTableFind(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < context->names.count; ++index) {
        const MAREA_SemaName *entry = &context->names.entries[index];

        if (MAREA_G_TextEquals(entry->text, entry->length, text, length) == MAREA_BOOLEAN_TRUE) {
            return entry;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaTableAppend(MAREA_SemaContext *context, MAREA_SemaNameTable *table, MAREA_SemaName entry)
{
    if ((context == MAREA_NULLPTR) || (table == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (table->count == table->capacity) {
        const MAREA_IntegerSize NEW_CAPACITY = (table->capacity == 0) ? MAREA_INTEGER_SIZE(16) : (table->capacity * MAREA_INTEGER_SIZE(2));

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(context->arena, (NEW_CAPACITY * sizeof(MAREA_SemaName)), MAREA_ALIGNOF(MAREA_SemaName), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_SemaName *entries = memory;

        if (table->count > 0) {
            memcpy(entries, table->entries, (table->count * sizeof(MAREA_SemaName)));
        }

        table->entries = entries;
        table->capacity = NEW_CAPACITY;
    }

    table->entries[table->count] = entry;
    table->count += 1;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaNameTableAdd(MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration)
{
    return MAREA_SemaTableAppend(context, &context->names, (MAREA_SemaName){ .text = text, .declaration = declaration, .length = length, .kind = kind });
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_AddName(MAREA_SemaContext *context, MAREA_SourcePosition position, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_SemaNameKind kind, const MAREA_AstNode *declaration)
{
    if (MAREA_SemaNameTableFind(context, text, length) != MAREA_NULLPTR) {
        const MAREA_SemaExcept REPORT_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED, position, "name is already declared:", text, length);

        if (REPORT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return REPORT_RESULT;
        }
    }

    return MAREA_SemaNameTableAdd(context, text, length, kind, declaration);
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasPublicSymbol(const MAREA_AstNode *root, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_G_ChildAt(root, index);

        const MAREA_Character8 *declarationName = MAREA_NULLPTR;

        MAREA_IntegerSize declarationLength = 0;

        if (MAREA_SemaDeclarationName(declaration, &declarationName, &declarationLength) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if ((MAREA_G_TextEquals(declarationName, declarationLength, name, nameLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaAttributeIsOn(root, declaration, "public") == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaImportResult MAREA_CALL MAREA_G_Resolve(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_AstNode **outRoot)
{
    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    *outRoot = MAREA_NULLPTR;

    context->resolver->resolve(context->resolver->userData, text, length, &result, outRoot);

    return result;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsPlainName(const MAREA_AstNode *qualifiedName)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    if (COUNT == 0) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_G_ChildAt(qualifiedName, index);

        if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNamespacePath(MAREA_SemaContext *context, const MAREA_AstNode *root, const MAREA_AstNode *qualifiedName)
{
    if (MAREA_G_IsPlainName(qualifiedName) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *file = MAREA_AstNodeGetPosition(root).file;

    if ((file == MAREA_NULLPTR) || (file[0] == '\0')) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize fileLength = strlen(file);

    if ((fileLength >= MAREA_INTEGER_SIZE(6)) && (memcmp((file + fileLength - MAREA_INTEGER_SIZE(6)), ".marea", MAREA_INTEGER_SIZE(6)) == 0)) {
        fileLength -= MAREA_INTEGER_SIZE(6);
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    MAREA_IntegerSize position = 0;

    MAREA_Boolean matches = MAREA_BOOLEAN_TRUE;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_G_ChildAt(qualifiedName, index);

        const MAREA_IntegerSize SEGMENT_LENGTH = MAREA_AstNodeGetTextLength(segment);

        if (index > 0) {
            if ((position >= fileLength) || (file[position] != '/')) {
                matches = MAREA_BOOLEAN_FALSE;
                break;
            }

            position += 1;
        }

        if (((position + SEGMENT_LENGTH) > fileLength) || (memcmp((file + position), MAREA_AstNodeGetText(segment), SEGMENT_LENGTH) != 0)) {
            matches = MAREA_BOOLEAN_FALSE;
            break;
        }

        position += SEGMENT_LENGTH;
    }

    if ((matches == MAREA_BOOLEAN_TRUE) && (position != fileLength)) {
        matches = MAREA_BOOLEAN_FALSE;
    }

    if (matches == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualifiedName, &joined, &joinedLength);

    if (JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return JOIN_RESULT;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAMESPACE_PATH_MISMATCH, MAREA_AstNodeGetPosition(qualifiedName), "namespace does not match the file path:", joined, joinedLength);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckImport(MAREA_SemaContext *context, const MAREA_AstNode *import)
{
    const MAREA_AstNode *qualifiedName = MAREA_G_ChildAt(import, 0);

    if ((qualifiedName == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualifiedName) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize SEGMENT_COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    if (MAREA_G_IsPlainName(qualifiedName) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *last = MAREA_G_ChildAt(qualifiedName, (SEGMENT_COUNT - MAREA_INTEGER_SIZE(1)));

    if ((last == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(last) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *alias = MAREA_NULLPTR;

    if ((MAREA_AstNodeGetFlags(import) & MAREA_AST_FLAG_HAS_ALIAS) != 0) {
        alias = MAREA_G_ChildAt(import, 1);
    }

    const MAREA_AstNode *localName = ((alias != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(alias) == MAREA_AST_KIND_IDENTIFIER)) ? alias : last;

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualifiedName, &joined, &joinedLength);

    if (JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return JOIN_RESULT;
    }

    const MAREA_AstNode *foundRoot = MAREA_NULLPTR;

    MAREA_SemaNameKind kind = MAREA_SEMA_NAME_KIND_NAMESPACE;

    MAREA_Boolean resolved = MAREA_BOOLEAN_TRUE;

    if (MAREA_G_Resolve(context, joined, joinedLength, &foundRoot) == MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) {
        resolved = MAREA_BOOLEAN_FALSE;
        kind = MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL;

        // INFO: the last segment may name a public symbol of the namespace one level up.
        if (SEGMENT_COUNT > 1) {
            const MAREA_IntegerSize PARENT_LENGTH = (joinedLength - MAREA_AstNodeGetTextLength(last) - MAREA_INTEGER_SIZE(1));

            const MAREA_SemaImportResult PARENT_RESULT = MAREA_G_Resolve(context, joined, PARENT_LENGTH, &foundRoot);

            if (PARENT_RESULT == MAREA_SEMA_IMPORT_RESULT_EXTERNAL) {
                resolved = MAREA_BOOLEAN_TRUE;
            }

            if ((PARENT_RESULT == MAREA_SEMA_IMPORT_RESULT_FOUND) && (foundRoot != MAREA_NULLPTR) && (MAREA_G_HasPublicSymbol(foundRoot, MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last)) == MAREA_BOOLEAN_TRUE)) {
                resolved = MAREA_BOOLEAN_TRUE;
            }
        }
    }

    if (resolved == MAREA_BOOLEAN_FALSE) {
        const MAREA_SemaExcept REPORT_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT, MAREA_AstNodeGetPosition(import), "unresolved import:", joined, joinedLength);

        if (REPORT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return REPORT_RESULT;
        }
    }

    return MAREA_G_AddName(context, MAREA_AstNodeGetPosition(localName), MAREA_AstNodeGetText(localName), MAREA_AstNodeGetTextLength(localName), kind, MAREA_NULLPTR);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamespaceAndNames(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_G_ChildAt(root, index);

        if (child == MAREA_NULLPTR) {
            continue;
        }

        const MAREA_AstKind KIND = MAREA_AstNodeGetKind(child);

        MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

        if (KIND == MAREA_AST_KIND_NAMESPACE_DECLARATION) {
            const MAREA_AstNode *qualifiedName = MAREA_G_ChildAt(child, 0);

            if ((qualifiedName != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(qualifiedName) == MAREA_AST_KIND_QUALIFIED_NAME)) {
                result = MAREA_G_CheckNamespacePath(context, root, qualifiedName);
            }
        } else if (KIND == MAREA_AST_KIND_IMPORT_DECLARATION) {
            result = MAREA_G_CheckImport(context, child);
        } else {
            const MAREA_Character8 *name = MAREA_NULLPTR;

            MAREA_IntegerSize nameLength = 0;

            if (MAREA_SemaDeclarationName(child, &name, &nameLength) == MAREA_BOOLEAN_TRUE) {
                result = MAREA_G_AddName(context, MAREA_AstNodeGetPosition(child), name, nameLength, MAREA_SEMA_NAME_KIND_DECLARATION, child);
            }
        }

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
