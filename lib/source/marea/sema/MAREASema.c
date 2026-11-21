// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

MAREA_SemaExcept MAREA_CALL MAREA_SemaContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_SemaImportResolver *resolver, MAREA_SemaContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (resolver == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_SemaContext), MAREA_ALIGNOF(MAREA_SemaContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaContext *context = memory;

    context->arena = arena;
    context->sink = sink;
    context->resolver = resolver;
    context->names = (MAREA_SemaNameTable){ 0 };
    context->generics = (MAREA_SemaNameTable){ 0 };
    context->locals = (MAREA_SemaNameTable){ 0 };
    context->root = MAREA_NULLPTR;
    context->function = MAREA_NULLPTR;
    context->selfDepth = 0;
    context->instantiationDepth = 0;

    *outContext = context;

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_SemaContextGetArena(const MAREA_SemaContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_SemaContextGetDiagnostics(const MAREA_SemaContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_SemaImportResolver *MAREA_CALL MAREA_SemaContextGetImportResolver(const MAREA_SemaContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->resolver;
}

void MAREA_CALL MAREA_SemaReport(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *message)
{
    MAREA_IGNORE(MAREA_DiagnosticSinkAdd(context->sink, (MAREA_Uinteger32)code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, message));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaReportNamed(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *prefix, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize PREFIX_LENGTH = strlen(prefix);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (PREFIX_LENGTH + nameLength + MAREA_INTEGER_SIZE(4)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *message = memory;

    memcpy(message, prefix, PREFIX_LENGTH);
    message[PREFIX_LENGTH] = ' ';
    message[(PREFIX_LENGTH + MAREA_INTEGER_SIZE(1))] = '\'';
    memcpy((message + PREFIX_LENGTH + MAREA_INTEGER_SIZE(2)), name, nameLength);
    message[(PREFIX_LENGTH + nameLength + MAREA_INTEGER_SIZE(2))] = '\'';
    message[(PREFIX_LENGTH + nameLength + MAREA_INTEGER_SIZE(3))] = '\0';

    MAREA_SemaReport(context, code, position, message);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaVisit(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a node the parser could not build is skipped whole, so no check reads a broken shape.
    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaExcept ATTRIBUTES_RESULT = MAREA_SemaCheckAttributes(context, node);

    if (ATTRIBUTES_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return ATTRIBUTES_RESULT;
    }

    MAREA_SemaScopeMark mark = { 0 };

    const MAREA_SemaExcept SCOPE_RESULT = MAREA_SemaScopeEnter(context, node, &mark);

    if (SCOPE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return SCOPE_RESULT;
    }

    MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    MAREA_Boolean entered = MAREA_BOOLEAN_FALSE;

    if (KIND == MAREA_AST_KIND_NAMED_TYPE) {
        result = MAREA_SemaCheckNamedType(context, node);

        if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
            result = MAREA_SemaInstantiationEnter(context, node, &entered);
        }
    } else if (KIND == MAREA_AST_KIND_WHERE_CLAUSE) {
        result = MAREA_SemaCheckWhereClause(context, node);
    } else if (KIND == MAREA_AST_KIND_CALL_EXPRESSION) {
        result = MAREA_SemaCheckCall(context, node);
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; ((index < COUNT) && (result == MAREA_SEMA_EXCEPT_SUCCESS)); ++index) {
        const MAREA_AstNode *child = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
            continue;
        }

        result = MAREA_SemaVisit(context, child);
    }

    if ((result == MAREA_SEMA_EXCEPT_SUCCESS) && (KIND == MAREA_AST_KIND_LOCAL_DECLARATION)) {
        result = MAREA_SemaDeclareLocal(context, node);
    }

    MAREA_SemaInstantiationLeave(context, entered);

    MAREA_SemaScopeLeave(context, mark);

    return result;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCompilationUnit(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    context->names = (MAREA_SemaNameTable){ 0 };
    context->generics = (MAREA_SemaNameTable){ 0 };
    context->locals = (MAREA_SemaNameTable){ 0 };
    context->root = root;
    context->function = MAREA_NULLPTR;
    context->selfDepth = 0;
    context->instantiationDepth = 0;

    const MAREA_SemaExcept NAMES_RESULT = MAREA_SemaCheckNamespaceAndNames(context, root);

    if (NAMES_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return NAMES_RESULT;
    }

    const MAREA_SemaExcept DECLARATIONS_RESULT = MAREA_SemaCheckDeclarations(context, root);

    if (DECLARATIONS_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return DECLARATIONS_RESULT;
    }

    return MAREA_SemaVisit(context, root);
}
