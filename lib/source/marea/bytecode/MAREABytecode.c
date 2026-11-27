// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include <string.h>

#if !defined(MAREA_BYTECODE_INITIAL_CAPACITY)
    #define MAREA_BYTECODE_INITIAL_CAPACITY MAREA_INTEGER_SIZE(4)
#endif

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_BytecodeContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_BytecodeContext), MAREA_ALIGNOF(MAREA_BytecodeContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_BytecodeContext *context = memory;

    context->arena = arena;
    context->sink = sink;
    context->registry = MAREA_NULLPTR;

    *outContext = context;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeContextSetTypeRegistry(MAREA_BytecodeContext *context, const MAREA_TypeRegistry *registry)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    context->registry = registry;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_BytecodeContextGetArena(const MAREA_BytecodeContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_BytecodeContextGetDiagnostics(const MAREA_BytecodeContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGrow(MAREA_Arena *arena, MAREA_Null *items, MAREA_IntegerSize *capacity, MAREA_IntegerSize count, MAREA_IntegerSize itemSize, MAREA_IntegerSize alignment)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (items == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (capacity == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (count < *capacity) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize newCapacity = (*capacity == MAREA_INTEGER_SIZE(0)) ? MAREA_BYTECODE_INITIAL_CAPACITY : (*capacity * MAREA_INTEGER_SIZE(2));

    if (newCapacity > (MAREA_INTEGER_SIZE_MAXIMUM / itemSize)) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, (newCapacity * itemSize), alignment, &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (count > MAREA_INTEGER_SIZE(0)) {
        memcpy(memory, *items, (count * itemSize));
    }

    *items = memory;
    *capacity = newCapacity;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeRaise(MAREA_BytecodeContext *context, MAREA_BytecodeDiagnosticCode code, MAREA_Uinteger32 offset, MAREA_Uinteger32 line, MAREA_Uinteger32 column, const MAREA_Character8 *message)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (message == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_SourcePosition position = {
        .file = MAREA_NULLPTR,
        .offset = offset,
        .line = line,
        .column = column,
        .length = MAREA_UINTEGER_32(0),
    };

    if (MAREA_DiagnosticSinkAdd(context->sink, code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, message) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleCreate(MAREA_BytecodeContext *context, MAREA_BytecodeModule **outModule)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outModule == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_BytecodeModule), MAREA_ALIGNOF(MAREA_BytecodeModule), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_BytecodeModule *module = memory;

    *module = (MAREA_BytecodeModule){ .context = context };

    *outModule = module;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeContext *MAREA_CALL MAREA_BytecodeModuleGetContext(const MAREA_BytecodeModule *module)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return module->context;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAppendString(MAREA_BytecodeModule *module, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((text == MAREA_NULLPTR) && (length != MAREA_INTEGER_SIZE(0))) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outIndex == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (module->stringCount >= (MAREA_Uinteger32)MAREA_UINTEGER_32_MAXIMUM) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (length == MAREA_INTEGER_SIZE_MAXIMUM) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Arena *arena = module->context->arena;

    MAREA_Null items = module->strings;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(arena, &items, &module->stringCapacity, module->stringCount, sizeof(MAREA_BytecodeString), MAREA_ALIGNOF(MAREA_BytecodeString));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    module->strings = items;

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, (length + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (length > MAREA_INTEGER_SIZE(0)) {
        memcpy(memory, text, length);
    }

    ((MAREA_Character8 *)memory)[length] = '\0';

    module->strings[module->stringCount].text = memory;
    module->strings[module->stringCount].length = length;

    *outIndex = (MAREA_Uinteger32)module->stringCount;

    module->stringCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddString(MAREA_BytecodeModule *module, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((text == MAREA_NULLPTR) && (length != MAREA_INTEGER_SIZE(0))) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outIndex == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->stringCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeString *known = &module->strings[index];

        if ((known->length == length) && ((length == MAREA_INTEGER_SIZE(0)) || (memcmp(known->text, text, length) == 0))) {
            *outIndex = (MAREA_Uinteger32)index;

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    return MAREA_BytecodeModuleAppendString(module, text, length, outIndex);
}

MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetStringCount(const MAREA_BytecodeModule *module)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return module->stringCount;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetString(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= module->stringCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outText = module->strings[index].text;
    *outLength = module->strings[index].length;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddType(MAREA_BytecodeModule *module, const MAREA_BytecodeTypeInfo *info, MAREA_Uinteger32 *outIndex)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (info == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outIndex == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null items = module->types;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(module->context->arena, &items, &module->typeCapacity, module->typeCount, sizeof(MAREA_BytecodeType), MAREA_ALIGNOF(MAREA_BytecodeType));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    module->types = items;

    MAREA_BytecodeType *type = &module->types[module->typeCount];

    *type = (MAREA_BytecodeType){ .info = *info };

    type->info.memberCount = MAREA_UINTEGER_32(0);

    *outIndex = (MAREA_Uinteger32)module->typeCount;

    module->typeCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetTypeCount(const MAREA_BytecodeModule *module)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return module->typeCount;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetType(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, MAREA_BytecodeTypeInfo *outInfo)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outInfo == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= module->typeCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outInfo = module->types[index].info;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddMember(MAREA_BytecodeModule *module, MAREA_Uinteger32 typeIndex, const MAREA_BytecodeMemberInfo *member)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (member == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typeIndex >= module->typeCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_BytecodeType *type = &module->types[typeIndex];

    MAREA_Null items = type->members;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(module->context->arena, &items, &type->memberCapacity, type->info.memberCount, sizeof(MAREA_BytecodeMemberInfo), MAREA_ALIGNOF(MAREA_BytecodeMemberInfo));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    type->members = items;

    type->members[type->info.memberCount] = *member;

    type->info.memberCount += MAREA_UINTEGER_32(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetMember(const MAREA_BytecodeModule *module, MAREA_Uinteger32 typeIndex, MAREA_Uinteger32 memberIndex, MAREA_BytecodeMemberInfo *outMember)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outMember == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typeIndex >= module->typeCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    const MAREA_BytecodeType *type = &module->types[typeIndex];

    if (memberIndex >= type->info.memberCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outMember = type->members[memberIndex];

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddFunction(MAREA_BytecodeModule *module, const MAREA_BytecodeFunctionInfo *info, MAREA_Uinteger32 *outIndex)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (info == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outIndex == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null items = module->functions;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(module->context->arena, &items, &module->functionCapacity, module->functionCount, sizeof(MAREA_BytecodeFunction), MAREA_ALIGNOF(MAREA_BytecodeFunction));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    module->functions = items;

    MAREA_BytecodeFunction *function = &module->functions[module->functionCount];

    *function = (MAREA_BytecodeFunction){ .info = *info };

    function->info.parameterCount = MAREA_UINTEGER_32(0);
    function->info.instructionCount = MAREA_UINTEGER_32(0);

    *outIndex = (MAREA_Uinteger32)module->functionCount;

    module->functionCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetFunctionCount(const MAREA_BytecodeModule *module)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return module->functionCount;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetFunction(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, MAREA_BytecodeFunctionInfo *outInfo)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outInfo == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= module->functionCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outInfo = module->functions[index].info;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddParameter(MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_BytecodeParameterInfo *parameter)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (parameter == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (functionIndex >= module->functionCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_BytecodeFunction *function = &module->functions[functionIndex];

    MAREA_Null items = function->parameters;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(module->context->arena, &items, &function->parameterCapacity, function->info.parameterCount, sizeof(MAREA_BytecodeParameterInfo), MAREA_ALIGNOF(MAREA_BytecodeParameterInfo));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    function->parameters = items;

    function->parameters[function->info.parameterCount] = *parameter;

    function->info.parameterCount += MAREA_UINTEGER_32(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetParameter(const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, MAREA_Uinteger32 parameterIndex, MAREA_BytecodeParameterInfo *outParameter)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outParameter == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (functionIndex >= module->functionCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    const MAREA_BytecodeFunction *function = &module->functions[functionIndex];

    if (parameterIndex >= function->info.parameterCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outParameter = function->parameters[parameterIndex];

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddInstruction(MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_BytecodeInstruction *instruction)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (instruction == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (functionIndex >= module->functionCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_BytecodeFunction *function = &module->functions[functionIndex];

    MAREA_Null items = function->instructions;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(module->context->arena, &items, &function->instructionCapacity, function->info.instructionCount, sizeof(MAREA_BytecodeInstruction), MAREA_ALIGNOF(MAREA_BytecodeInstruction));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    function->instructions = items;

    function->instructions[function->info.instructionCount] = *instruction;

    function->info.instructionCount += MAREA_UINTEGER_32(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetInstruction(const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, MAREA_Uinteger32 instructionIndex, MAREA_BytecodeInstruction *outInstruction)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outInstruction == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (functionIndex >= module->functionCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    const MAREA_BytecodeFunction *function = &module->functions[functionIndex];

    if (instructionIndex >= function->info.instructionCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outInstruction = function->instructions[instructionIndex];

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleAddData(MAREA_BytecodeModule *module, const MAREA_Byte *bytes, MAREA_IntegerSize size, MAREA_Uinteger32 *outIndex)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((bytes == MAREA_NULLPTR) && (size != MAREA_INTEGER_SIZE(0))) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outIndex == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null items = module->blobs;

    MAREA_BytecodeExcept except = MAREA_BytecodeGrow(module->context->arena, &items, &module->blobCapacity, module->blobCount, sizeof(MAREA_BytecodeBlob), MAREA_ALIGNOF(MAREA_BytecodeBlob));

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    module->blobs = items;

    MAREA_Null memory = MAREA_NULLPTR;

    if (size > MAREA_INTEGER_SIZE(0)) {
        if (MAREA_ArenaAllocate(module->context->arena, size, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        memcpy(memory, bytes, size);
    }

    module->blobs[module->blobCount].bytes = memory;
    module->blobs[module->blobCount].size = size;

    *outIndex = (MAREA_Uinteger32)module->blobCount;

    module->blobCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_IntegerSize MAREA_CALL MAREA_BytecodeModuleGetDataCount(const MAREA_BytecodeModule *module)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return module->blobCount;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeModuleGetData(const MAREA_BytecodeModule *module, MAREA_Uinteger32 index, const MAREA_Byte **outBytes, MAREA_IntegerSize *outSize)
{
    if (module == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outBytes == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outSize == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= module->blobCount) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *outBytes = module->blobs[index].bytes;
    *outSize = module->blobs[index].size;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypeEquals(const MAREA_BytecodeType *left, const MAREA_BytecodeType *right)
{
    if ((left->info.kind != right->info.kind) || (left->info.name != right->info.name) || (left->info.size != right->info.size) || (left->info.count != right->info.count)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((left->info.element != right->info.element) || (left->info.alignment != right->info.alignment) || (left->info.memberCount != right->info.memberCount)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < left->info.memberCount; index += MAREA_UINTEGER_32(1)) {
        if ((left->members[index].value != right->members[index].value) || (left->members[index].name != right->members[index].name) || (left->members[index].type != right->members[index].type)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_FunctionEquals(const MAREA_BytecodeFunction *left, const MAREA_BytecodeFunction *right)
{
    if ((left->info.frameArea != right->info.frameArea) || (left->info.name != right->info.name) || (left->info.result != right->info.result) || (left->info.registerCount != right->info.registerCount)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((left->info.flags != right->info.flags) || (left->info.parameterCount != right->info.parameterCount) || (left->info.instructionCount != right->info.instructionCount)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < left->info.parameterCount; index += MAREA_UINTEGER_32(1)) {
        if ((left->parameters[index].type != right->parameters[index].type) || (left->parameters[index].flags != right->parameters[index].flags)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < left->info.instructionCount; index += MAREA_UINTEGER_32(1)) {
        if (memcmp(&left->instructions[index], &right->instructions[index], sizeof(MAREA_BytecodeInstruction)) != 0) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeModuleEquals(const MAREA_BytecodeModule *left, const MAREA_BytecodeModule *right)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((left->stringCount != right->stringCount) || (left->typeCount != right->typeCount) || (left->functionCount != right->functionCount) || (left->blobCount != right->blobCount)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < left->stringCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeString *first = &left->strings[index];
        const MAREA_BytecodeString *second = &right->strings[index];

        if ((first->length != second->length) || (memcmp(first->text, second->text, first->length) != 0)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < left->typeCount; index += MAREA_INTEGER_SIZE(1)) {
        if (!MAREA_G_TypeEquals(&left->types[index], &right->types[index])) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < left->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        if (!MAREA_G_FunctionEquals(&left->functions[index], &right->functions[index])) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < left->blobCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeBlob *first = &left->blobs[index];
        const MAREA_BytecodeBlob *second = &right->blobs[index];

        if ((first->size != second->size) || ((first->size > MAREA_INTEGER_SIZE(0)) && (memcmp(first->bytes, second->bytes, first->size) != 0))) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}
