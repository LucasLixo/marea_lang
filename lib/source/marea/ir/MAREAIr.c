// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

MAREA_IrExcept MAREA_CALL MAREA_IrContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_IrContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_IrContext), MAREA_ALIGNOF(MAREA_IrContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IrContext *context = memory;

    context->arena = arena;
    context->sink = sink;

    *outContext = context;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_IrContextGetArena(const MAREA_IrContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_IrContextGetDiagnostics(const MAREA_IrContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerCompilationUnit(MAREA_IrContext *context, const MAREA_SemaTypedNode *typedRoot, const MAREA_IrNode **outModule)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typedRoot == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outModule == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *astRoot = MAREA_SemaTypedNodeGetAstNode(typedRoot);

    if (MAREA_AstNodeGetKind(astRoot) != MAREA_AST_KIND_COMPILATION_UNIT) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_IrNode *module = MAREA_NULLPTR;

    const MAREA_IrExcept created = MAREA_IrNodeCreate(context, MAREA_IR_KIND_MODULE, astRoot, &module);

    if (created != MAREA_IR_EXCEPT_SUCCESS) {
        return created;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typedRoot);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrExcept lowered = MAREA_IrLowerDeclaration(context, MAREA_IrTypedChildAt(typedRoot, index), MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), module);

        if (lowered != MAREA_IR_EXCEPT_SUCCESS) {
            return lowered;
        }
    }

    *outModule = module;

    return MAREA_IR_EXCEPT_SUCCESS;
}
