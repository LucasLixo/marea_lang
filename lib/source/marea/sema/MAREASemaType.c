// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the core types the language itself defines. Every other type is a library or a user type.
static const MAREA_Character8 *const MAREA_G_BUILTIN_TYPES[] = {
    "boolean",
    "integer8",
    "integer16",
    "integer32",
    "integer64",
    "unsigned8",
    "unsigned16",
    "unsigned32",
    "unsigned64",
    "float32",
    "float64",
    "string",
    "vector",
    "pointer",
    "owned",
    "slice",
    "task",
};

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index)
{
    const MAREA_AstNode *child = MAREA_NULLPTR;

    if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right)
{
    const MAREA_IntegerSize RIGHT_LENGTH = strlen(right);

    if (leftLength != RIGHT_LENGTH) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(left, right, leftLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBuiltin(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_BUILTIN_TYPES); ++index) {
        if (MAREA_G_TextEquals(text, length, MAREA_G_BUILTIN_TYPES[index]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsTypeDeclaration(const MAREA_AstNode *declaration)
{
    if (declaration == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    switch (MAREA_AstNodeGetKind(declaration)) {
    case MAREA_AST_KIND_STRUCTURE_DECLARATION:
    case MAREA_AST_KIND_ENUM_DECLARATION:
    case MAREA_AST_KIND_OPAQUE_DECLARATION:
    case MAREA_AST_KIND_TYPE_DECLARATION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsGenericInScope(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < context->generics.count; ++index) {
        const MAREA_SemaName *entry = &context->generics.entries[index];

        if ((entry->length == length) && (memcmp(entry->text, text, length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_PushGeneric(MAREA_SemaContext *context, const MAREA_AstNode *identifier)
{
    MAREA_SemaNameTable *table = &context->generics;

    if (table->count == table->capacity) {
        const MAREA_IntegerSize NEW_CAPACITY = (table->capacity == 0) ? MAREA_INTEGER_SIZE(8) : (table->capacity * MAREA_INTEGER_SIZE(2));

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

    table->entries[table->count] = (MAREA_SemaName){ .text = MAREA_AstNodeGetText(identifier), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(identifier), .kind = MAREA_SEMA_NAME_KIND_DECLARATION };
    table->count += 1;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaScopeEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaScopeMark *outMark)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outMark == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    outMark->genericCount = context->generics.count;
    outMark->selfDepth = context->selfDepth;

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    if ((KIND == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) || (KIND == MAREA_AST_KIND_CONTRACT_DECLARATION)) {
        context->selfDepth += 1;
    }

    if ((KIND != MAREA_AST_KIND_STRUCTURE_DECLARATION) && (KIND != MAREA_AST_KIND_TYPE_DECLARATION) && (KIND != MAREA_AST_KIND_FUNCTION_DECLARATION) && (KIND != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *parameters = MAREA_G_ChildAt(node, index);

        if ((parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(parameters) != MAREA_AST_KIND_GENERIC_PARAMETERS)) {
            continue;
        }

        for (MAREA_IntegerSize parameterIndex = 0; parameterIndex < MAREA_AstNodeGetChildCount(parameters); ++parameterIndex) {
            const MAREA_AstNode *parameter = MAREA_G_ChildAt(parameters, parameterIndex);

            const MAREA_AstNode *name = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ChildAt(parameter, 0);

            if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
                continue;
            }

            const MAREA_SemaExcept RESULT = MAREA_G_PushGeneric(context, name);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaScopeLeave(MAREA_SemaContext *context, MAREA_SemaScopeMark mark)
{
    context->generics.count = mark.genericCount;
    context->selfDepth = mark.selfDepth;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamedType(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *qualifiedName = MAREA_G_ChildAt(node, 0);

    if ((qualifiedName == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualifiedName) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize SEGMENT_COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    if (SEGMENT_COUNT == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < SEGMENT_COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_G_ChildAt(qualifiedName, index);

        if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    }

    const MAREA_AstNode *first = MAREA_G_ChildAt(qualifiedName, 0);

    const MAREA_Character8 *firstText = MAREA_AstNodeGetText(first);

    const MAREA_IntegerSize FIRST_LENGTH = MAREA_AstNodeGetTextLength(first);

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, firstText, FIRST_LENGTH);

    if (SEGMENT_COUNT == 1) {
        if ((MAREA_G_IsBuiltin(firstText, FIRST_LENGTH) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsGenericInScope(context, firstText, FIRST_LENGTH) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if ((context->selfDepth > 0) && (MAREA_G_TextEquals(firstText, FIRST_LENGTH, "Self") == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if ((entry != MAREA_NULLPTR) && ((entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (MAREA_G_IsTypeDeclaration(entry->declaration) == MAREA_BOOLEAN_TRUE))) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    } else if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_NAMESPACE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a name that no local rule knows is a library type, so the resolver has the last word.
    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualifiedName, &joined, &joinedLength);

    if (JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return JOIN_RESULT;
    }

    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    const MAREA_AstNode *foundRoot = MAREA_NULLPTR;

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION)) {
        context->resolver->resolve(context->resolver->userData, joined, joinedLength, &result, &foundRoot);
    }

    if (result != MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE, MAREA_AstNodeGetPosition(node), "unknown type:", joined, joinedLength);
}
