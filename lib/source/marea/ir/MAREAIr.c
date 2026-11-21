// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

#if !defined(MAREA_IR_MESSAGE_CAPACITY)
    #define MAREA_IR_MESSAGE_CAPACITY MAREA_INTEGER_SIZE(160)
#endif

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
    context->unit = MAREA_NULLPTR;

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

MAREA_IrExcept MAREA_CALL MAREA_IrRaise(MAREA_IrContext *context, MAREA_IrDiagnosticCode code, const MAREA_AstNode *at, const MAREA_Character8 *message, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (message == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Character8 buffer[MAREA_IR_MESSAGE_CAPACITY];

    MAREA_IntegerSize length = strlen(message);

    if (length > (MAREA_IR_MESSAGE_CAPACITY - MAREA_INTEGER_SIZE(1))) {
        length = (MAREA_IR_MESSAGE_CAPACITY - MAREA_INTEGER_SIZE(1));
    }

    memcpy(buffer, message, length);

    // INFO: the name is added as 'name' and cut when it does not fit; the message always ends with a NUL.
    if ((name != MAREA_NULLPTR) && ((length + MAREA_INTEGER_SIZE(4)) < MAREA_IR_MESSAGE_CAPACITY)) {
        MAREA_IntegerSize room = (MAREA_IR_MESSAGE_CAPACITY - length - MAREA_INTEGER_SIZE(4));

        if (nameLength < room) {
            room = nameLength;
        }

        buffer[length] = ' ';
        buffer[(length + MAREA_INTEGER_SIZE(1))] = '\'';
        memcpy((buffer + length + MAREA_INTEGER_SIZE(2)), name, room);
        buffer[(length + MAREA_INTEGER_SIZE(2) + room)] = '\'';
        length = (length + MAREA_INTEGER_SIZE(3) + room);
    }

    buffer[length] = '\0';

    if (MAREA_DiagnosticSinkAdd(context->sink, (MAREA_Uinteger32)code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, MAREA_AstNodeGetPosition(at), buffer) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerUnitDeclarations(MAREA_IrContext *context, const MAREA_SemaTypedNode *typedRoot, const MAREA_IrNode **outModule)
{
    const MAREA_AstNode *astRoot = MAREA_SemaTypedNodeGetAstNode(typedRoot);

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

    if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(typedRoot)) != MAREA_AST_KIND_COMPILATION_UNIT) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: the root stays in the context only while the unit is lowered, so that a call can ask which names the unit binds.
    context->unit = typedRoot;

    const MAREA_IrExcept result = MAREA_G_LowerUnitDeclarations(context, typedRoot, outModule);

    context->unit = MAREA_NULLPTR;

    return result;
}
