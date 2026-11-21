// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

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

MAREA_SemaExcept MAREA_CALL MAREA_SemaVisit(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a node the parser could not build is skipped whole, so no check reads a broken shape.
    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_SemaVisit(context, child);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCompilationUnit(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    return MAREA_SemaVisit(context, root);
}
