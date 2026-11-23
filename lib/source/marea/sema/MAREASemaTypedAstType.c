// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: a chain of type aliases is followed this many steps, the same limit the ir uses; a longer chain, or a cycle, stops at the last name.
#define MAREA_G_TYPED_TYPE_ALIAS_DEPTH_LIMIT MAREA_INTEGER_SIZE(8)

// INFO: the text is written twice with the same code: once with no buffer, to count the bytes, once into the arena memory.
typedef struct MAREA_G_TypeText
{
    MAREA_Character8 *buffer;
    MAREA_IntegerSize length;
    MAREA_Boolean failed;
} MAREA_G_TypeText;

static void MAREA_CALL MAREA_G_TypeTextAppend(MAREA_G_TypeText *text, const MAREA_Character8 *part, MAREA_IntegerSize length)
{
    if ((text->buffer != MAREA_NULLPTR) && (length > 0)) {
        memcpy((text->buffer + text->length), part, length);
    }

    text->length += length;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_TypeTextFindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypeTextIsType(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_TypeTextFirstType(const MAREA_AstNode *node)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if (MAREA_G_TypeTextIsType(child) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static void MAREA_CALL MAREA_G_TypeTextRender(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_IntegerSize depth, MAREA_G_TypeText *text);

// INFO: the type arguments of a written type, in angle brackets, joined by a comma with no space.
static void MAREA_CALL MAREA_G_TypeTextRenderArguments(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_IntegerSize depth, MAREA_G_TypeText *text)
{
    const MAREA_AstNode *arguments = MAREA_G_TypeTextFindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if (arguments == MAREA_NULLPTR) {
        return;
    }

    MAREA_G_TypeTextAppend(text, "<", MAREA_INTEGER_SIZE(1));

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arguments);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        if (index > 0) {
            MAREA_G_TypeTextAppend(text, ",", MAREA_INTEGER_SIZE(1));
        }

        MAREA_G_TypeTextRender(context, MAREA_SemaChildAt(arguments, index), depth, text);
    }

    MAREA_G_TypeTextAppend(text, ">", MAREA_INTEGER_SIZE(1));
}

static void MAREA_CALL MAREA_G_TypeTextRenderFunction(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_IntegerSize depth, MAREA_G_TypeText *text)
{
    MAREA_G_TypeTextAppend(text, "function(", MAREA_INTEGER_SIZE(9));

    const MAREA_AstNode *parameters = MAREA_G_TypeTextFindChild(type, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_IntegerSize COUNT = (parameters == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(parameters);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        if (index > 0) {
            MAREA_G_TypeTextAppend(text, ",", MAREA_INTEGER_SIZE(1));
        }

        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        MAREA_G_TypeTextRender(context, (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_TypeTextFirstType(parameter), depth, text);
    }

    MAREA_G_TypeTextAppend(text, ")", MAREA_INTEGER_SIZE(1));

    const MAREA_AstNode *result = MAREA_SemaReturnType(type);

    if (result != MAREA_NULLPTR) {
        MAREA_G_TypeTextAppend(text, ":", MAREA_INTEGER_SIZE(1));

        MAREA_G_TypeTextRender(context, result, depth, text);
    }
}

// INFO: the name of a type, then its arguments. An import alias is replaced by what it stands for, the way the ir does it: a whole-path or a member alias by its original, the head of a dotted name that is an import alias by its original with the tail kept. A local type alias without generic parameters, written with no arguments, is replaced by the type it names, so the full type is kept.
static void MAREA_CALL MAREA_G_TypeTextRenderNamed(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_IntegerSize depth, MAREA_G_TypeText *text)
{
    const MAREA_AstNode *qualified = MAREA_SemaChildAt(type, 0);

    const MAREA_IntegerSize SEGMENT_COUNT = (qualified == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(qualified);

    const MAREA_AstNode *head = (SEGMENT_COUNT == 0) ? MAREA_NULLPTR : MAREA_SemaChildAt(qualified, 0);

    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualified) != MAREA_AST_KIND_QUALIFIED_NAME) || (head == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(head) != MAREA_AST_KIND_IDENTIFIER)) {
        text->failed = MAREA_BOOLEAN_TRUE;

        return;
    }

    const MAREA_Character8 *name = MAREA_AstNodeGetText(head);

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(head);

    const MAREA_SemaName *entry = (MAREA_SemaIsGenericInScope(context, name, NAME_LENGTH) == MAREA_BOOLEAN_TRUE) ? MAREA_NULLPTR : MAREA_SemaNameTableFind(context, name, NAME_LENGTH);

    const MAREA_Boolean HAS_ARGUMENTS = (MAREA_G_TypeTextFindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    // INFO: a local type alias is followed when it has no generic parameters and the use gives no arguments; the target is read as written in the file.
    if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) && (SEGMENT_COUNT == 1) && (HAS_ARGUMENTS == MAREA_BOOLEAN_FALSE) && (depth < MAREA_G_TYPED_TYPE_ALIAS_DEPTH_LIMIT) && (entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_TYPE_DECLARATION) && (MAREA_G_TypeTextFindChild(entry->declaration, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_NULLPTR)) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(entry->declaration, (MAREA_AstNodeGetChildCount(entry->declaration) - MAREA_INTEGER_SIZE(1)));

        if (MAREA_G_TypeTextIsType(target) == MAREA_BOOLEAN_TRUE) {
            MAREA_G_TypeTextRender(context, target, (depth + MAREA_INTEGER_SIZE(1)), text);

            return;
        }
    }

    // INFO: only an import name that has an original is an alias; an import without "as" stands for itself.
    if ((entry != MAREA_NULLPTR) && (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) && (entry->original != MAREA_NULLPTR)) {
        MAREA_G_TypeTextAppend(text, entry->original, entry->originalLength);
    } else {
        MAREA_G_TypeTextAppend(text, name, NAME_LENGTH);
    }

    for (MAREA_IntegerSize index = 1; index < SEGMENT_COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(qualified, index);

        if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
            text->failed = MAREA_BOOLEAN_TRUE;

            return;
        }

        MAREA_G_TypeTextAppend(text, ".", MAREA_INTEGER_SIZE(1));
        MAREA_G_TypeTextAppend(text, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
    }

    MAREA_G_TypeTextRenderArguments(context, type, depth, text);
}

static void MAREA_CALL MAREA_G_TypeTextRender(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_IntegerSize depth, MAREA_G_TypeText *text)
{
    if (type == MAREA_NULLPTR) {
        text->failed = MAREA_BOOLEAN_TRUE;

        return;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(type);

    if (KIND == MAREA_AST_KIND_INTEGER_LITERAL) {
        MAREA_G_TypeTextAppend(text, MAREA_AstNodeGetText(type), MAREA_AstNodeGetTextLength(type));

        return;
    }

    if (KIND == MAREA_AST_KIND_FUNCTION_TYPE) {
        MAREA_G_TypeTextRenderFunction(context, type, depth, text);

        return;
    }

    if (KIND == MAREA_AST_KIND_NAMED_TYPE) {
        MAREA_G_TypeTextRenderNamed(context, type, depth, text);

        return;
    }

    text->failed = MAREA_BOOLEAN_TRUE;
}

// INFO: the written type of a node that declares one: the first type child, or, for a function, the type after its parameter list.
static const MAREA_AstNode *MAREA_CALL MAREA_G_TypeTextDeclaredType(const MAREA_AstNode *node)
{
    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    if (KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) {
        return MAREA_SemaReturnType(node);
    }

    if ((KIND == MAREA_AST_KIND_PARAMETER) || (KIND == MAREA_AST_KIND_LOCAL_DECLARATION) || (KIND == MAREA_AST_KIND_FIELD_DECLARATION) || (KIND == MAREA_AST_KIND_VARIANT_FIELD) || (KIND == MAREA_AST_KIND_CONSTANT_DECLARATION)) {
        return MAREA_G_TypeTextFirstType(node);
    }

    return MAREA_NULLPTR;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstResolveDeclaredType(MAREA_SemaContext *context, MAREA_SemaTypedNode *built)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    if (built == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    built->resolvedType = MAREA_NULLPTR;
    built->resolvedTypeLength = 0;

    const MAREA_AstNode *type = MAREA_G_TypeTextDeclaredType(built->astNode);

    if (type == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_G_TypeText counter = { .buffer = MAREA_NULLPTR, .length = 0, .failed = MAREA_BOOLEAN_FALSE };

    MAREA_G_TypeTextRender(context, type, 0, &counter);

    if ((counter.failed == MAREA_BOOLEAN_TRUE) || (counter.length == 0)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, counter.length, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_G_TypeText writer = { .buffer = memory, .length = 0, .failed = MAREA_BOOLEAN_FALSE };

    MAREA_G_TypeTextRender(context, type, 0, &writer);

    built->resolvedType = memory;
    built->resolvedTypeLength = writer.length;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}
