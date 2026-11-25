// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/bytecode/MAREABytecodeGenerate.h"

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"
#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include <string.h>

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_JoinName(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_BytecodeGeneratorFunction *entry)
{
    const MAREA_IntegerSize NAME_LENGTH = MAREA_IrNodeGetTextLength(node);
    const MAREA_IntegerSize PREFIX = ((generator->namespaceLength > MAREA_INTEGER_SIZE(0)) ? (generator->namespaceLength + MAREA_INTEGER_SIZE(1)) : MAREA_INTEGER_SIZE(0));

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, (PREFIX + NAME_LENGTH + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *text = memory;

    if (PREFIX > MAREA_INTEGER_SIZE(0)) {
        memcpy(text, generator->namespaceText, generator->namespaceLength);

        text[generator->namespaceLength] = '.';
    }

    memcpy(&text[PREFIX], MAREA_IrNodeGetText(node), NAME_LENGTH);

    text[(PREFIX + NAME_LENGTH)] = '\0';

    entry->name = text;
    entry->nameLength = (PREFIX + NAME_LENGTH);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Declare(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->functions, &generator->functionCapacity, generator->functionCount, sizeof(MAREA_BytecodeGeneratorFunction), MAREA_ALIGNOF(MAREA_BytecodeGeneratorFunction), &items));

    generator->functions = items;

    MAREA_BytecodeGeneratorFunction *entry = &generator->functions[generator->functionCount];

    memset(entry, 0, sizeof(*entry));

    generator->functionCount += MAREA_INTEGER_SIZE(1);

    entry->node = node;

    MAREA_BYTECODE_TRY(MAREA_G_JoinName(generator, node, entry));

    entry->resultKind = ((MAREA_IrNodeGetResolvedType(node) == MAREA_NULLPTR) ? MAREA_BYTECODE_GENERATOR_KIND_VOID : MAREA_BytecodeGeneratorKindOfNode(node));

    const MAREA_IrNode *parameters = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(parameters);

    entry->parameterCount = (MAREA_Uinteger32)COUNT;

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(generator->arena, COUNT, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        entry->parameterKinds = memory;

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
            entry->parameterKinds[index] = MAREA_BytecodeGeneratorKindOfNode(MAREA_BytecodeGeneratorChildAt(parameters, index));
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Collect(MAREA_BytecodeGenerator *generator)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(generator->root);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *child = MAREA_BytecodeGeneratorChildAt(generator->root, index);

        if (MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_FUNCTION_DECLARATION) {
            MAREA_BYTECODE_TRY(MAREA_G_Declare(generator, child));
        } else if (MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_CONSTANT_DECLARATION) {
            MAREA_Null items = MAREA_NULLPTR;

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->constants, &generator->constantCapacity, generator->constantCount, sizeof(const MAREA_IrNode *), MAREA_ALIGNOF(const MAREA_IrNode *), &items));

            generator->constants = items;
            generator->constants[generator->constantCount] = child;
            generator->constantCount += MAREA_INTEGER_SIZE(1);
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Publish(MAREA_BytecodeGenerator *generator, const MAREA_BytecodeGeneratorFunction *entry)
{
    MAREA_Uinteger32 name = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, entry->name, entry->nameLength, &name));

    MAREA_BytecodeFunctionInfo info = {
        .frameArea = (MAREA_Uinteger64)generator->maxFrameOffset,
        .name = name,
        .result = MAREA_BYTECODE_NONE,
        .registerCount = generator->maxRegister,
        .flags = MAREA_UINTEGER_32(0),
    };

    if (entry->resultKind != MAREA_BYTECODE_GENERATOR_KIND_VOID) {
        if (entry->resultKind != MAREA_BYTECODE_GENERATOR_KIND_NONE) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, entry->resultKind, &info.result));
        }
    }

    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddFunction(generator->module, &info, &index));

    for (MAREA_Uinteger32 parameter = MAREA_UINTEGER_32(0); parameter < entry->parameterCount; parameter += MAREA_UINTEGER_32(1)) {
        if (entry->parameterKinds[parameter] == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
            continue;
        }

        MAREA_BytecodeParameterInfo parameterInfo = {
            .type = MAREA_UINTEGER_32(0),
            .flags = MAREA_UINTEGER_32(0),
        };

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, entry->parameterKinds[parameter], &parameterInfo.type));

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddParameter(generator->module, index, &parameterInfo));
    }

    if (!generator->functionFailed) {
        for (MAREA_IntegerSize instruction = MAREA_INTEGER_SIZE(0); instruction < generator->codeCount; instruction += MAREA_INTEGER_SIZE(1)) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddInstruction(generator->module, index, &generator->code[instruction]));
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerFunction(MAREA_BytecodeGenerator *generator, const MAREA_BytecodeGeneratorFunction *entry)
{
    generator->current = entry;
    generator->codeCount = MAREA_INTEGER_SIZE(0);
    generator->symbolCount = MAREA_INTEGER_SIZE(0);
    generator->loopCount = MAREA_INTEGER_SIZE(0);
    generator->patchCount = MAREA_INTEGER_SIZE(0);
    generator->nextRegister = MAREA_UINTEGER_32(0);
    generator->maxRegister = MAREA_UINTEGER_32(0);
    generator->nextFrameOffset = MAREA_UINTEGER_32(0);
    generator->maxFrameOffset = MAREA_UINTEGER_32(0);
    generator->constantDepth = MAREA_UINTEGER_32(0);
    generator->functionFailed = MAREA_BOOLEAN_FALSE;

    const MAREA_IrNode *parameters = MAREA_BytecodeGeneratorChildAt(entry->node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *body = MAREA_BytecodeGeneratorChildAt(entry->node, MAREA_INTEGER_SIZE(1));

    if (entry->resultKind == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorFail(generator, entry->node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A function has a result type that is not a scalar."));
    }

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < entry->parameterCount; index += MAREA_UINTEGER_32(1)) {
        const MAREA_IrNode *parameter = MAREA_BytecodeGeneratorChildAt(parameters, index);

        if (!((entry->parameterKinds[index] < MAREA_BYTECODE_NUMBER_KIND_COUNT) || (entry->parameterKinds[index] == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN))) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorFail(generator, parameter, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A parameter has a type that is not a scalar."));

            continue;
        }

        MAREA_Uinteger32 registerIndex = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, parameter, MAREA_UINTEGER_32(1), &registerIndex));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(parameter), MAREA_IrNodeGetTextLength(parameter), registerIndex, entry->parameterKinds[index], MAREA_BYTECODE_NONE));
    }

    if ((body == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(body) != MAREA_IR_KIND_BLOCK)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorFail(generator, entry->node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A function has no body."));
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerBlock(generator, body));
    }

    if (entry->resultKind == MAREA_BYTECODE_GENERATOR_KIND_VOID) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_RETURN_VOID, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_MISSING_RETURN, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    }

    return MAREA_G_Publish(generator, entry);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGenerate(MAREA_BytecodeContext *context, const MAREA_IrNode *irModule, MAREA_BytecodeModule **outModule)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (irModule == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outModule == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    *outModule = MAREA_NULLPTR;

    if (MAREA_IrNodeGetKind(irModule) != MAREA_IR_KIND_MODULE) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_BytecodeGenerator), MAREA_ALIGNOF(MAREA_BytecodeGenerator), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_BytecodeGenerator *generator = memory;

    memset(generator, 0, sizeof(*generator));

    generator->context = context;
    generator->arena = context->arena;
    generator->root = irModule;
    generator->namespaceText = MAREA_IrNodeGetText(irModule);
    generator->namespaceLength = MAREA_IrNodeGetTextLength(irModule);

    for (MAREA_IntegerSize kind = MAREA_INTEGER_SIZE(0); kind < MAREA_ARRAY_SIZE(generator->typeIndex); kind += MAREA_INTEGER_SIZE(1)) {
        generator->typeIndex[kind] = MAREA_BYTECODE_NONE;
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleCreate(context, &generator->module));

    MAREA_BYTECODE_TRY(MAREA_G_Collect(generator));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_BYTECODE_TRY(MAREA_G_LowerFunction(generator, &generator->functions[index]));
    }

    if (!generator->anyFailed) {
        *outModule = generator->module;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
