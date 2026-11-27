// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_NamesAType(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(generator->root);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *child = MAREA_BytecodeGeneratorChildAt(generator->root, index);
        const MAREA_IrKind KIND = MAREA_IrNodeGetKind(child);

        if (((KIND == MAREA_IR_KIND_STRUCTURE_DECLARATION) || (KIND == MAREA_IR_KIND_ENUM_DECLARATION)) && (MAREA_IrNodeGetTextLength(child) == length) && (memcmp(MAREA_IrNodeGetText(child), text, length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorHostName(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *callee, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    *outName = MAREA_NULLPTR;
    *outLength = MAREA_INTEGER_SIZE(0);

    // INFO: a path through an alias has one identifier as its base, a written full path has a chain of members.
    const MAREA_IrNode *root = callee;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    while ((root != MAREA_NULLPTR) && (MAREA_IrNodeGetKind(root) == MAREA_IR_KIND_MEMBER_ACCESS)) {
        const MAREA_IrNode *member = MAREA_BytecodeGeneratorChildAt(root, MAREA_INTEGER_SIZE(1));

        if (member == MAREA_NULLPTR) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        length += (MAREA_IrNodeGetTextLength(member) + MAREA_INTEGER_SIZE(1));
        root = MAREA_BytecodeGeneratorChildAt(root, MAREA_INTEGER_SIZE(0));
    }

    if ((root == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(root) != MAREA_IR_KIND_IDENTIFIER) || (MAREA_IrNodeGetResolvedType(root) != MAREA_NULLPTR)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *rootText = MAREA_IrNodeGetText(root);
    const MAREA_IntegerSize ROOT_LENGTH = MAREA_IrNodeGetTextLength(root);

    if ((MAREA_BytecodeGeneratorFindSymbol(generator, rootText, ROOT_LENGTH) != MAREA_NULLPTR) || MAREA_G_NamesAType(generator, rootText, ROOT_LENGTH)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    length += ROOT_LENGTH;

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, length, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *name = memory;

    MAREA_IntegerSize end = length;

    for (const MAREA_IrNode *node = callee; MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS; node = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0))) {
        const MAREA_IrNode *member = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));
        const MAREA_IntegerSize MEMBER_LENGTH = MAREA_IrNodeGetTextLength(member);

        end -= MEMBER_LENGTH;

        memcpy(&name[end], MAREA_IrNodeGetText(member), MEMBER_LENGTH);

        end -= MAREA_INTEGER_SIZE(1);

        name[end] = '.';
    }

    memcpy(name, rootText, ROOT_LENGTH);

    *outName = name;
    *outLength = length;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: an argument is a borrow of a structure (the register takes its address) or a scalar value.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerHostArgument(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *argument, MAREA_Uinteger32 destination, MAREA_BytecodeParameterInfo *outParameter)
{
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 aggregateIndex = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, argument, &kind, &aggregateIndex));

    if (kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        const MAREA_Uinteger64 SIZE = generator->aggregates[aggregateIndex].size;
        const MAREA_Uinteger64 ALIGNMENT = generator->aggregates[aggregateIndex].alignment;

        outParameter->type = generator->aggregates[aggregateIndex].typeIndex;
        outParameter->flags = MAREA_BYTECODE_PARAMETER_FLAG_BORROW;

        const MAREA_IrKind NODE_KIND = MAREA_IrNodeGetKind(argument);

        if ((NODE_KIND == MAREA_IR_KIND_IDENTIFIER) || (NODE_KIND == MAREA_IR_KIND_MEMBER_ACCESS)) {
            MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);
            MAREA_Uinteger32 found = MAREA_BYTECODE_NONE;

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, argument, &address, &found));

            return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, destination, address, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
        }

        MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, argument, SIZE, ALIGNMENT, &offset));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, destination, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

        return MAREA_BytecodeGeneratorConstructAggregate(generator, argument, destination, aggregateIndex);
    }

    if (MAREA_IrNodeGetResolvedType(argument) == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, argument, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNTYPED_OPERAND, "An argument of a host call has no type.");
    }

    if (!((kind < MAREA_BYTECODE_NUMBER_KIND_COUNT) || (kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN))) {
        return MAREA_BytecodeGeneratorFail(generator, argument, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An argument of a host call has a type that is not a value here.");
    }

    outParameter->flags = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, kind, &outParameter->type));

    MAREA_Uinteger8 gotKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    return MAREA_BytecodeGeneratorLowerInto(generator, argument, destination, kind, &gotKind);
}

static MAREA_Boolean MAREA_CALL MAREA_G_SameImport(const MAREA_BytecodeGeneratorImport *left, const MAREA_BytecodeGeneratorImport *right)
{
    if ((left->nameLength != right->nameLength) || (left->parameterCount != right->parameterCount) || (left->result != right->result) || (memcmp(left->name, right->name, left->nameLength) != 0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < left->parameterCount; index += MAREA_UINTEGER_32(1)) {
        if ((left->parameters[index].type != right->parameters[index].type) || (left->parameters[index].flags != right->parameters[index].flags)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the imports come after the functions of the unit, so the index of one is known before it is published.
MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorImportIndex(MAREA_BytecodeGenerator *generator, const MAREA_BytecodeGeneratorImport *import, MAREA_Uinteger32 *outIndex)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->importCount; index += MAREA_INTEGER_SIZE(1)) {
        if (MAREA_G_SameImport(&generator->imports[index], import)) {
            *outIndex = (MAREA_Uinteger32)(generator->functionCount + index);

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->imports, &generator->importCapacity, generator->importCount, sizeof(MAREA_BytecodeGeneratorImport), MAREA_ALIGNOF(MAREA_BytecodeGeneratorImport), &items));

    generator->imports = items;
    generator->imports[generator->importCount] = *import;

    *outIndex = (MAREA_Uinteger32)(generator->functionCount + generator->importCount);

    generator->importCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerHostCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 destination, MAREA_Boolean valueUsed, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(arguments);

    const MAREA_Character8 *resolved = MAREA_IrNodeGetResolvedType(node);
    const MAREA_Boolean NO_RESULT = ((resolved == MAREA_NULLPTR) || ((MAREA_IrNodeGetResolvedTypeLength(node) == MAREA_INTEGER_SIZE(4)) && (memcmp(resolved, "void", 4) == 0)));

    *outKind = MAREA_BYTECODE_GENERATOR_KIND_VOID;

    // INFO: sema types a call only from the signature of the host, so a value with no type is never guessed.
    if (NO_RESULT && valueUsed) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNTYPED_OPERAND, "A host call that is used as a value has no type.");
    }

    MAREA_Uinteger8 resultKind = MAREA_BYTECODE_GENERATOR_KIND_VOID;
    MAREA_Uinteger32 aggregateIndex = MAREA_BYTECODE_NONE;

    MAREA_BytecodeGeneratorImport import = {
        .name = name,
        .parameters = MAREA_NULLPTR,
        .nameLength = length,
        .parameterCount = (MAREA_Uinteger32)COUNT,
        .result = MAREA_BYTECODE_NONE,
    };

    if (!NO_RESULT) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, node, &resultKind, &aggregateIndex));

        if (resultKind == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A host call has a result type that is not a value here.");
        }
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger32 window = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, ((MAREA_Uinteger32)COUNT + MAREA_UINTEGER_32(1)), &window));

    if (resultKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        const MAREA_Uinteger64 SIZE = generator->aggregates[aggregateIndex].size;
        const MAREA_Uinteger64 ALIGNMENT = generator->aggregates[aggregateIndex].alignment;

        import.result = generator->aggregates[aggregateIndex].typeIndex;

        MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, SIZE, ALIGNMENT, &offset));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, window, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, window, (MAREA_Uinteger32)SIZE, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    } else if (resultKind != MAREA_BYTECODE_GENERATOR_KIND_VOID) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, resultKind, &import.result));
    }

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(generator->arena, (COUNT * sizeof(MAREA_BytecodeParameterInfo)), MAREA_ALIGNOF(MAREA_BytecodeParameterInfo), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_BytecodeParameterInfo *parameters = memory;

        memset(parameters, 0, (COUNT * sizeof(MAREA_BytecodeParameterInfo)));

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
            MAREA_BYTECODE_TRY(MAREA_G_LowerHostArgument(generator, MAREA_BytecodeGeneratorChildAt(arguments, index), (window + MAREA_UINTEGER_32(1) + (MAREA_Uinteger32)index), &parameters[index]));
        }

        import.parameters = parameters;
    }

    // INFO: a failed argument leaves the shape incomplete, and the module is dropped anyway.
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 functionIndex = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorImportIndex(generator, &import, &functionIndex));

    if (functionIndex > MAREA_BYTECODE_GENERATOR_REGISTER_LIMIT) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW, "A module needs more than 65535 functions and imports.");
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_CALL, window, functionIndex, (MAREA_Uinteger32)COUNT, MAREA_NULLPTR));

    if ((resultKind != MAREA_BYTECODE_GENERATOR_KIND_VOID) && (window != destination)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, destination, window, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    }

    *outKind = resultKind;

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPublishImports(MAREA_BytecodeGenerator *generator)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->importCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeGeneratorImport *import = &generator->imports[index];

        MAREA_Uinteger32 name = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, import->name, import->nameLength, &name));

        const MAREA_BytecodeFunctionInfo INFO = {
            .frameArea = MAREA_UINTEGER_64(0),
            .name = name,
            .result = import->result,
            .registerCount = MAREA_UINTEGER_32(0),
            .flags = MAREA_BYTECODE_FUNCTION_FLAG_HOST,
        };

        MAREA_Uinteger32 function = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddFunction(generator->module, &INFO, &function));

        for (MAREA_Uinteger32 parameter = MAREA_UINTEGER_32(0); parameter < import->parameterCount; parameter += MAREA_UINTEGER_32(1)) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddParameter(generator->module, function, &import->parameters[parameter]));
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
