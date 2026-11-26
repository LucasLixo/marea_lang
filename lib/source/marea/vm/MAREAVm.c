// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/vm/MAREAVm.h"

#include "marea/internal/vm/MAREAVmInternal.h"

#include <string.h>

MAREA_VmExcept MAREA_CALL MAREA_VmContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_VmContext **outContext)
{
    if (outContext == MAREA_NULLPTR) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    *outContext = MAREA_NULLPTR;

    if ((arena == MAREA_NULLPTR) || (sink == MAREA_NULLPTR)) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_VmContext), MAREA_ALIGNOF(MAREA_VmContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_VmContext *context = memory;

    context->arena = arena;
    context->sink = sink;
    context->registry = MAREA_NULLPTR;

    *outContext = context;

    return MAREA_VM_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_VmContextGetArena(const MAREA_VmContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_VmContextGetDiagnostics(const MAREA_VmContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_VmExcept MAREA_CALL MAREA_VmContextSetTypeRegistry(MAREA_VmContext *context, const MAREA_TypeRegistry *registry)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    context->registry = registry;

    return MAREA_VM_EXCEPT_SUCCESS;
}

const MAREA_TypeRegistry *MAREA_CALL MAREA_VmContextGetTypeRegistry(const MAREA_VmContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->registry;
}

MAREA_VmExcept MAREA_CALL MAREA_VmFindFunction(const MAREA_BytecodeModule *module, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex)
{
    if (outIndex == MAREA_NULLPTR) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    *outIndex = MAREA_UINTEGER_32(0);

    if ((module == MAREA_NULLPTR) || (name == MAREA_NULLPTR)) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_BytecodeModuleGetFunctionCount(module);

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < (MAREA_Uinteger32)COUNT; index += MAREA_UINTEGER_32(1)) {
        MAREA_BytecodeFunctionInfo info = { 0 };

        if (MAREA_BytecodeModuleGetFunction(module, index, &info) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            continue;
        }

        if ((info.flags & MAREA_BYTECODE_FUNCTION_FLAG_HOST) != MAREA_UINTEGER_32(0)) {
            continue;
        }

        const MAREA_Character8 *text = MAREA_NULLPTR;
        MAREA_IntegerSize textLength = MAREA_INTEGER_SIZE(0);

        if (MAREA_BytecodeModuleGetString(module, info.name, &text, &textLength) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            continue;
        }

        if ((textLength == length) && (memcmp(text, name, length) == 0)) {
            *outIndex = index;

            return MAREA_VM_EXCEPT_SUCCESS;
        }
    }

    return MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION;
}

MAREA_VmExcept MAREA_CALL MAREA_VmCall(MAREA_VmContext *context, const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_Uinteger64 *arguments, MAREA_Uinteger32 argumentCount, MAREA_VmResult *outResult)
{
    if (outResult == MAREA_NULLPTR) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    outResult->value = MAREA_UINTEGER_64(0);
    outResult->trap = MAREA_BYTECODE_TRAP_CODE_NONE;
    outResult->trapped = MAREA_BOOLEAN_FALSE;

    if ((context == MAREA_NULLPTR) || (module == MAREA_NULLPTR)) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((arguments == MAREA_NULLPTR) && (argumentCount > MAREA_UINTEGER_32(0))) {
        return MAREA_VM_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((MAREA_IntegerSize)functionIndex >= MAREA_BytecodeModuleGetFunctionCount(module)) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_BytecodeFunctionInfo info = { 0 };

    if (MAREA_BytecodeModuleGetFunction(module, functionIndex, &info) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    if ((info.flags & MAREA_BYTECODE_FUNCTION_FLAG_HOST) != MAREA_UINTEGER_32(0)) {
        return MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION;
    }

    if (info.parameterCount != argumentCount) {
        return MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT;
    }

    return MAREA_VmRun(context, module, functionIndex, arguments, argumentCount, MAREA_UINTEGER_32(1), outResult);
}

MAREA_VmExcept MAREA_CALL MAREA_VmCallByName(MAREA_VmContext *context, const MAREA_BytecodeModule *module, const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_Uinteger64 *arguments, MAREA_Uinteger32 argumentCount, MAREA_VmResult *outResult)
{
    MAREA_Uinteger32 functionIndex = MAREA_UINTEGER_32(0);

    const MAREA_VmExcept FOUND = MAREA_VmFindFunction(module, name, length, &functionIndex);

    if (FOUND != MAREA_VM_EXCEPT_SUCCESS) {
        if (outResult != MAREA_NULLPTR) {
            outResult->value = MAREA_UINTEGER_64(0);
            outResult->trap = MAREA_BYTECODE_TRAP_CODE_NONE;
            outResult->trapped = MAREA_BOOLEAN_FALSE;
        }

        return FOUND;
    }

    return MAREA_VmCall(context, module, functionIndex, arguments, argumentCount, outResult);
}
