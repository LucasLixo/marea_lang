// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"
#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include <string.h>

// INFO: the ownership of the values that hold storage of the allocator, which is a vector or a value that holds one.
// A value is taken over by a copy of its bytes that leaves zeros behind, so each storage has one owner and a destroy
// of a value that was taken does nothing. A local and a borrowed temporary are destroyed when their block ends and
// at each jump out of it. The memory of a frame starts as zeros, so a destroy of a value that was never built does
// nothing.

// INFO: a structure that holds itself through a vector would be destroyed by code with no end.
#define MAREA_G_DEPTH_LIMIT MAREA_UINTEGER_32(16)

static MAREA_Boolean MAREA_CALL MAREA_G_DeclaresDestroy(const MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 aggregateIndex)
{
    const MAREA_BytecodeGeneratorAggregate *aggregate = &generator->aggregates[aggregateIndex];

    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(generator->root);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *child = MAREA_BytecodeGeneratorChildAt(generator->root, index);

        const MAREA_Character8 *text = MAREA_IrNodeGetText(child);

        if (MAREA_IrNodeIsDestroyOperator(child) && (MAREA_IrNodeGetTextLength(child) > aggregate->nameLength) && (memcmp(text, aggregate->name, aggregate->nameLength) == 0) && (text[aggregate->nameLength] == '.')) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the aggregate of a field of a structure, or MAREA_BYTECODE_NONE for a field that is none.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_FieldAt(MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 aggregateIndex, MAREA_IntegerSize child, MAREA_Uinteger32 *outAggregate, MAREA_Uinteger64 *outOffset)
{
    *outAggregate = MAREA_BYTECODE_NONE;
    *outOffset = MAREA_UINTEGER_64(0);

    const MAREA_IrNode *field = MAREA_BytecodeGeneratorChildAt(generator->aggregates[aggregateIndex].declaration, child);

    if (MAREA_IrNodeGetKind(field) != MAREA_IR_KIND_FIELD_DECLARATION) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 found = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, field, &kind, &found));

    const MAREA_IrNode *ignored = MAREA_NULLPTR;

    if ((kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) && MAREA_BytecodeGeneratorAggregateField(generator, aggregateIndex, MAREA_IrNodeGetText(field), MAREA_IrNodeGetTextLength(field), outOffset, &ignored)) {
        *outAggregate = found;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Needs(MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 depth, MAREA_Boolean *outNeeds)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[aggregateIndex];

    if (AGGREGATE.isVector) {
        *outNeeds = MAREA_BOOLEAN_TRUE;

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    *outNeeds = MAREA_BOOLEAN_FALSE;

    if (depth >= MAREA_G_DEPTH_LIMIT) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (AGGREGATE.isRun) {
        if ((AGGREGATE.count > MAREA_UINTEGER_64(0)) && (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE)) {
            return MAREA_G_Needs(generator, AGGREGATE.elementAggregate, (depth + MAREA_UINTEGER_32(1)), outNeeds);
        }

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(AGGREGATE.declaration);

    for (MAREA_IntegerSize child = MAREA_INTEGER_SIZE(0); (child < COUNT) && !*outNeeds; child += MAREA_INTEGER_SIZE(1)) {
        MAREA_Uinteger32 field = MAREA_BYTECODE_NONE;
        MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);

        MAREA_BYTECODE_TRY(MAREA_G_FieldAt(generator, aggregateIndex, child, &field, &offset));

        if (field != MAREA_BYTECODE_NONE) {
            MAREA_BYTECODE_TRY(MAREA_G_Needs(generator, field, (depth + MAREA_UINTEGER_32(1)), outNeeds));
        }
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorNeedsDrop(MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 aggregateIndex, MAREA_Boolean *outNeeds)
{
    *outNeeds = MAREA_BOOLEAN_FALSE;

    if (generator->functionFailed || (aggregateIndex == MAREA_BYTECODE_NONE)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    return MAREA_G_Needs(generator, aggregateIndex, MAREA_UINTEGER_32(0), outNeeds);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Constant(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger64 value, MAREA_Uinteger32 *outRegister)
{
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorEmitConstant(generator, node, *outRegister, value);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAt(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger64 offset, MAREA_Uinteger32 *outRegister)
{
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_FIELD, *outRegister, base, (MAREA_Uinteger32)offset, MAREA_NULLPTR);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLoadWord(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger64 offset, MAREA_Uinteger32 *outRegister)
{
    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, base, offset, &address));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, *outRegister, address, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorStoreWord(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger64 offset, MAREA_Uinteger32 value)
{
    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, base, offset, &address));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, value, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLoopOpen(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_BytecodeGeneratorCount *outLoop, MAREA_Uinteger32 first, MAREA_Uinteger32 limit)
{
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &outLoop->index));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, outLoop->index, first, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    MAREA_Uinteger32 test = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &test));

    outLoop->start = (MAREA_Uinteger32)generator->codeCount;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LESS_UNSIGNED, test, outLoop->index, limit, MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, test, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &outLoop->exit);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLoopClose(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_BytecodeGeneratorCount *loop)
{
    MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, loop->index, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));

    MAREA_Uinteger32 back = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &back));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, back, loop->start));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, loop->exit, (MAREA_Uinteger32)generator->codeCount);
}

// INFO: destination = base + index * stride, with the address arithmetic on the bit pattern.
MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorStride(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger32 index, MAREA_Uinteger64 stride, MAREA_Uinteger32 *outRegister)
{
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, stride, outRegister));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MUL, *outRegister, index, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, *outRegister, base, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR);
}

// INFO: the runtime of a vector is reached as a function of the registry, with the header as a borrow and each other value as a number.
MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorRuntimeCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *name, MAREA_Uinteger32 vector, MAREA_Uinteger32 header, const MAREA_Uinteger32 *values, MAREA_Uinteger32 count)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, ((count + MAREA_UINTEGER_32(1)) * sizeof(MAREA_BytecodeParameterInfo)), MAREA_ALIGNOF(MAREA_BytecodeParameterInfo), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_BytecodeParameterInfo *parameters = memory;

    memset(parameters, 0, ((count + MAREA_UINTEGER_32(1)) * sizeof(MAREA_BytecodeParameterInfo)));

    parameters[0].type = generator->aggregates[vector].typeIndex;
    parameters[0].flags = MAREA_BYTECODE_PARAMETER_FLAG_BORROW;

    MAREA_Uinteger32 window = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, (count + MAREA_UINTEGER_32(2)), &window));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, (window + MAREA_UINTEGER_32(1)), header, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < count; index += MAREA_UINTEGER_32(1)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, &parameters[(index + MAREA_UINTEGER_32(1))].type));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, (window + MAREA_UINTEGER_32(2) + index), values[index], MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    }

    const MAREA_BytecodeGeneratorImport IMPORT = {
        .name = name,
        .parameters = parameters,
        .nameLength = strlen(name),
        .parameterCount = (count + MAREA_UINTEGER_32(1)),
        .result = MAREA_BYTECODE_NONE,
    };

    MAREA_Uinteger32 function = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorImportIndex(generator, &IMPORT, &function));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_CALL, window, function, (count + MAREA_UINTEGER_32(1)), MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Drop(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 address, MAREA_Uinteger32 aggregateIndex, MAREA_Boolean isElement, MAREA_Uinteger32 depth);

// INFO: the elements of a run or of the storage of a vector, from the last to the first.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_DropElements(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 first, MAREA_Uinteger32 count, MAREA_Uinteger32 aggregateIndex, MAREA_Boolean isElement, MAREA_Uinteger32 depth)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[aggregateIndex];

    MAREA_Uinteger32 zero = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &zero));

    MAREA_BytecodeGeneratorCount loop = { 0 };

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoopOpen(generator, node, &loop, zero, count));

    MAREA_Uinteger32 reversed = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &reversed));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, reversed, count, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_SUB, reversed, loop.index, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_SUB, reversed, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStride(generator, node, first, reversed, AGGREGATE.stride, &element));
    MAREA_BYTECODE_TRY(MAREA_G_Drop(generator, node, element, AGGREGATE.elementAggregate, isElement, (depth + MAREA_UINTEGER_32(1))));

    return MAREA_BytecodeGeneratorLoopClose(generator, node, &loop);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Drop(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 address, MAREA_Uinteger32 aggregateIndex, MAREA_Boolean isElement, MAREA_Uinteger32 depth)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[aggregateIndex];

    // INFO: this stage lowers no destroy operator, as for the destroy of a local.
    if (isElement && !AGGREGATE.isRun && !AGGREGATE.isVector && MAREA_G_DeclaresDestroy(generator, aggregateIndex)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A destroy operator of an element is not lowered yet.");
    }

    if (depth >= MAREA_G_DEPTH_LIMIT) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A type that holds itself through a vector is not destroyed yet.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    if (AGGREGATE.isVector) {
        const MAREA_Boolean ELEMENT = (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE);

        MAREA_Boolean needs = MAREA_BOOLEAN_FALSE;

        if (ELEMENT) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorNeedsDrop(generator, AGGREGATE.elementAggregate, &needs));

            needs = (needs || MAREA_G_DeclaresDestroy(generator, AGGREGATE.elementAggregate));
        }

        if (needs) {
            MAREA_Uinteger32 data = MAREA_UINTEGER_32(0);
            MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, address, MAREA_UINTEGER_64(0), &data));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, address, MAREA_UINTEGER_64(8), &size));
            MAREA_BYTECODE_TRY(MAREA_G_DropElements(generator, node, data, size, aggregateIndex, MAREA_BOOLEAN_TRUE, depth));
        }

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorRuntimeCall(generator, node, "marea.vector.runtime_release", aggregateIndex, address, MAREA_NULLPTR, MAREA_UINTEGER_32(0)));
    } else if (AGGREGATE.isRun) {
        if ((AGGREGATE.count > MAREA_UINTEGER_64(0)) && (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE)) {
            MAREA_Uinteger32 count = MAREA_UINTEGER_32(0);

            MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, AGGREGATE.count, &count));
            MAREA_BYTECODE_TRY(MAREA_G_DropElements(generator, node, address, count, aggregateIndex, isElement, depth));
        }
    } else {
        for (MAREA_IntegerSize child = MAREA_IrNodeGetChildCount(AGGREGATE.declaration); child > MAREA_INTEGER_SIZE(0); child -= MAREA_INTEGER_SIZE(1)) {
            MAREA_Uinteger32 field = MAREA_BYTECODE_NONE;
            MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);
            MAREA_Boolean needs = MAREA_BOOLEAN_FALSE;

            MAREA_BYTECODE_TRY(MAREA_G_FieldAt(generator, aggregateIndex, (child - MAREA_INTEGER_SIZE(1)), &field, &offset));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorNeedsDrop(generator, field, &needs));

            if (needs) {
                MAREA_Uinteger32 member = MAREA_UINTEGER_32(0);

                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, address, offset, &member));
                MAREA_BYTECODE_TRY(MAREA_G_Drop(generator, node, member, field, MAREA_BOOLEAN_FALSE, (depth + MAREA_UINTEGER_32(1))));
            }
        }
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorDrop(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 address, MAREA_Uinteger32 aggregateIndex, MAREA_Boolean isElement)
{
    MAREA_Boolean needs = MAREA_BOOLEAN_FALSE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorNeedsDrop(generator, aggregateIndex, &needs));

    if (generator->functionFailed || (!needs && !(isElement && !generator->aggregates[aggregateIndex].isRun && !generator->aggregates[aggregateIndex].isVector && MAREA_G_DeclaresDestroy(generator, aggregateIndex)))) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    return MAREA_G_Drop(generator, node, address, aggregateIndex, isElement, MAREA_UINTEGER_32(0));
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_DeepCopy(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger32 source, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 depth);

// INFO: copies one element of a run or of a vector from an address to another.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_CopyElement(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger32 source, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 depth)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[aggregateIndex];

    if (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_G_DeepCopy(generator, node, destination, source, AGGREGATE.elementAggregate, (depth + MAREA_UINTEGER_32(1)));
    }

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, destination, source, (MAREA_Uinteger32)AGGREGATE.stride, MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_CopyVector(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger32 source, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 depth)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[aggregateIndex];

    MAREA_Uinteger32 values[3] = { 0 };
    MAREA_Uinteger32 from = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 zero = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, destination, (MAREA_Uinteger32)AGGREGATE.size, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, source, MAREA_UINTEGER_64(8), &values[0]));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, AGGREGATE.stride, &values[1]));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, AGGREGATE.elementAlignment, &values[2]));

    // INFO: a reserve of 0 does nothing, so an empty copy takes no storage.
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorRuntimeCall(generator, node, "marea.vector.runtime_reserve", aggregateIndex, destination, values, MAREA_UINTEGER_32(3)));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, source, MAREA_UINTEGER_64(0), &from));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &zero));

    MAREA_BytecodeGeneratorCount loop = { 0 };

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoopOpen(generator, node, &loop, zero, values[0]));

    MAREA_Uinteger32 data = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 target = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, destination, MAREA_UINTEGER_64(0), &data));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStride(generator, node, data, loop.index, AGGREGATE.stride, &target));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStride(generator, node, from, loop.index, AGGREGATE.stride, &element));
    MAREA_BYTECODE_TRY(MAREA_G_CopyElement(generator, node, target, element, aggregateIndex, depth));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &size));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, size, loop.index, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, size, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStoreWord(generator, node, destination, MAREA_UINTEGER_64(8), size));

    return MAREA_BytecodeGeneratorLoopClose(generator, node, &loop);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_DeepCopy(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger32 source, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 depth)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (depth >= MAREA_G_DEPTH_LIMIT) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A type that holds itself through a vector is not copied yet.");
    }

    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[aggregateIndex];

    MAREA_Boolean needs = MAREA_BOOLEAN_FALSE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorNeedsDrop(generator, aggregateIndex, &needs));

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    if (AGGREGATE.isVector) {
        MAREA_BYTECODE_TRY(MAREA_G_CopyVector(generator, node, destination, source, aggregateIndex, depth));
    } else if (needs && AGGREGATE.isRun) {
        MAREA_Uinteger32 zero = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 count = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 target = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &zero));
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, AGGREGATE.count, &count));

        MAREA_BytecodeGeneratorCount loop = { 0 };

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoopOpen(generator, node, &loop, zero, count));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStride(generator, node, destination, loop.index, AGGREGATE.stride, &target));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStride(generator, node, source, loop.index, AGGREGATE.stride, &element));
        MAREA_BYTECODE_TRY(MAREA_G_CopyElement(generator, node, target, element, aggregateIndex, depth));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoopClose(generator, node, &loop));
    } else {
        if (AGGREGATE.size > MAREA_UINTEGER_64(0)) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, destination, source, (MAREA_Uinteger32)AGGREGATE.size, MAREA_NULLPTR));
        }

        // INFO: the bytes hold the other fields; each field with storage gets a copy of its own over them.
        for (MAREA_IntegerSize child = MAREA_INTEGER_SIZE(0); needs && (child < MAREA_IrNodeGetChildCount(AGGREGATE.declaration)); child += MAREA_INTEGER_SIZE(1)) {
            MAREA_Uinteger32 field = MAREA_BYTECODE_NONE;
            MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);
            MAREA_Boolean fieldNeeds = MAREA_BOOLEAN_FALSE;

            MAREA_BYTECODE_TRY(MAREA_G_FieldAt(generator, aggregateIndex, child, &field, &offset));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorNeedsDrop(generator, field, &fieldNeeds));

            if (fieldNeeds) {
                MAREA_Uinteger32 target = MAREA_UINTEGER_32(0);
                MAREA_Uinteger32 member = MAREA_UINTEGER_32(0);

                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, destination, offset, &target));
                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, source, offset, &member));
                MAREA_BYTECODE_TRY(MAREA_G_DeepCopy(generator, node, target, member, field, (depth + MAREA_UINTEGER_32(1))));
            }
        }
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorDeepCopy(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger32 source, MAREA_Uinteger32 aggregateIndex)
{
    return MAREA_G_DeepCopy(generator, node, destination, source, aggregateIndex, MAREA_UINTEGER_32(0));
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPushOwned(MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 offset, MAREA_Uinteger32 aggregateIndex)
{
    MAREA_Boolean needs = MAREA_BOOLEAN_FALSE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorNeedsDrop(generator, aggregateIndex, &needs));

    if (!needs) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->owned, &generator->ownedCapacity, generator->ownedCount, sizeof(MAREA_BytecodeGeneratorOwned), MAREA_ALIGNOF(MAREA_BytecodeGeneratorOwned), &items));

    generator->owned = items;
    generator->owned[generator->ownedCount] = (MAREA_BytecodeGeneratorOwned){ .offset = offset, .aggregate = aggregateIndex };
    generator->ownedCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorDrops(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_IntegerSize mark)
{
    const MAREA_Uinteger32 MARK = generator->nextRegister;

    for (MAREA_IntegerSize index = generator->ownedCount; (index > mark) && !generator->functionFailed; index -= MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeGeneratorOwned OWNED = generator->owned[(index - MAREA_INTEGER_SIZE(1))];

        MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &address));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, address, OWNED.offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrop(generator, node, address, OWNED.aggregate, MAREA_BOOLEAN_FALSE));

        generator->nextRegister = MARK;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
