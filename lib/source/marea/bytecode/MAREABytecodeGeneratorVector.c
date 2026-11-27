// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"
#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include "marea/base/MAREAVectorRuntime.h"

#include <string.h>

// INFO: the members, the literal and the index of a vector. The header is three words: data at 0, size at 8 and
// capacity at 16. The storage is changed only by the runtime of marea/base/MAREAVectorRuntime.h, which the generated
// C calls too; the elements are read and written here.

#define MAREA_G_DATA     MAREA_UINTEGER_64(0)
#define MAREA_G_SIZE     MAREA_UINTEGER_64(8)
#define MAREA_G_CAPACITY MAREA_UINTEGER_64(16)

// INFO: one call of a member: the vector, its address and the register of the result.
typedef struct MAREA_G_Member
{
    const MAREA_IrNode *node;
    const MAREA_IrNode *arguments;
    MAREA_Uinteger32 vector;
    MAREA_Uinteger32 header;
    MAREA_Uinteger32 destination;
    MAREA_Uinteger8 *outKind;
} MAREA_G_Member;

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_IrNode *node, const MAREA_Character8 *expected)
{
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(node);

    return ((strlen(expected) == LENGTH) && (memcmp(MAREA_IrNodeGetText(node), expected, LENGTH) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Uinteger8 MAREA_CALL MAREA_G_Width(MAREA_Uinteger8 kind)
{
    return ((kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 : kind);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Constant(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger64 value, MAREA_Uinteger32 *outRegister)
{
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorEmitConstant(generator, node, *outRegister, value);
}

// INFO: traps with an index out of bounds when the test register is not zero.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_TrapWhen(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 test)
{
    MAREA_Uinteger32 passed = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, test, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &passed));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_INDEX_OUT_OF_BOUNDS, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, passed, (MAREA_Uinteger32)generator->codeCount);
}

// INFO: traps when left < right, or when left <= right with orEqual, as unsigned values.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_TrapBelow(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 left, MAREA_Uinteger32 right, MAREA_Boolean orEqual)
{
    MAREA_Uinteger32 test = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &test));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, (orEqual ? MAREA_BYTECODE_OPCODE_LESS_EQUAL_UNSIGNED : MAREA_BYTECODE_OPCODE_LESS_UNSIGNED), test, left, right, MAREA_NULLPTR));

    return MAREA_G_TrapWhen(generator, node, test);
}

// INFO: a count or an index is read as an unsigned 64-bit value, so a negative one, which a register holds sign extended, is past every size.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Number(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outRegister)
{
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, node, *outRegister, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, &kind));

    if (!generator->functionFailed && (kind >= MAREA_BYTECODE_NUMBER_KIND_FLOAT_32)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A count or an index of a vector is an integer.");
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: bytes of the frame for a value, and the register of their address.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Buffer(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment, MAREA_Uinteger32 destination)
{
    MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, size, alignment, &offset));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, destination, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
}

// INFO: builds a value that an element takes over: a number in a register, or a value of its own bytes.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Value(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 vector, MAREA_Uinteger32 *outRegister)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[vector];

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    if (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        const MAREA_Uinteger64 ALIGNMENT = AGGREGATE.elementAlignment;

        MAREA_BYTECODE_TRY(MAREA_G_Buffer(generator, node, AGGREGATE.stride, ALIGNMENT, *outRegister));

        return MAREA_BytecodeGeneratorConstructAggregate(generator, node, *outRegister, AGGREGATE.elementAggregate);
    }

    MAREA_Uinteger8 got = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    return MAREA_BytecodeGeneratorLowerInto(generator, node, *outRegister, AGGREGATE.elementKind, &got);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Store(MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 vector, MAREA_Uinteger32 address, MAREA_Uinteger32 value)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[vector];

    if (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, address, value, (MAREA_Uinteger32)AGGREGATE.stride, MAREA_NULLPTR);
    }

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, value, MAREA_G_Width(AGGREGATE.elementKind), MAREA_NULLPTR);
}

// INFO: the address of the element at the index that a register holds, which is known to exist.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Element(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 vector, MAREA_Uinteger32 header, MAREA_Uinteger32 index, MAREA_Uinteger32 *outRegister)
{
    MAREA_Uinteger32 data = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, header, MAREA_G_DATA, &data));

    return MAREA_BytecodeGeneratorStride(generator, node, data, index, generator->aggregates[vector].stride, outRegister);
}

// INFO: copies the element at an address into the result of the member: a number, or bytes of its own.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Result(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member, MAREA_Uinteger32 address)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[member->vector];

    *member->outKind = AGGREGATE.elementKind;

    if (AGGREGATE.elementKind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, member->destination, address, MAREA_G_Width(AGGREGATE.elementKind), MAREA_NULLPTR);
    }

    MAREA_BYTECODE_TRY(MAREA_G_Buffer(generator, member->node, AGGREGATE.stride, AGGREGATE.elementAlignment, member->destination));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, member->destination, address, (MAREA_Uinteger32)AGGREGATE.stride, MAREA_NULLPTR);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorVectorPlace(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 vector, MAREA_Uinteger32 header, MAREA_Uinteger32 *outRegister)
{
    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Number(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), &index));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, header, MAREA_G_SIZE, &size));
    MAREA_BYTECODE_TRY(MAREA_G_TrapBelow(generator, node, size, index, MAREA_BOOLEAN_TRUE));

    return MAREA_G_Element(generator, node, vector, header, index, outRegister);
}

// INFO: the stride and the alignment of an element, after the values that a runtime function takes first.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Runtime(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *name, MAREA_Uinteger32 vector, MAREA_Uinteger32 header, const MAREA_Uinteger32 *first)
{
    MAREA_Uinteger32 values[3] = { 0 };
    MAREA_Uinteger32 count = MAREA_UINTEGER_32(0);

    if (first != MAREA_NULLPTR) {
        values[count] = *first;
        count += MAREA_UINTEGER_32(1);
    }

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, generator->aggregates[vector].stride, &values[count]));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, generator->aggregates[vector].elementAlignment, &values[(count + MAREA_UINTEGER_32(1))]));

    return MAREA_BytecodeGeneratorRuntimeCall(generator, node, name, vector, header, values, (count + MAREA_UINTEGER_32(2)));
}

// INFO: destroys the elements from the last one down to a size.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Truncate(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 vector, MAREA_Uinteger32 header, MAREA_Uinteger32 wanted)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[vector];

    MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 test = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 done = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 back = MAREA_UINTEGER_32(0);

    const MAREA_Uinteger32 START = (MAREA_Uinteger32)generator->codeCount;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, header, MAREA_G_SIZE, &size));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &test));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LESS_UNSIGNED, test, wanted, size, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, test, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &done));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_SUB, size, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStoreWord(generator, node, header, MAREA_G_SIZE, size));

    if (AGGREGATE.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        MAREA_BYTECODE_TRY(MAREA_G_Element(generator, node, vector, header, size, &element));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorDrop(generator, node, element, AGGREGATE.elementAggregate, MAREA_BOOLEAN_TRUE));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &back));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, back, START));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, done, (MAREA_Uinteger32)generator->codeCount);
}

// INFO: every element from a first index to a limit becomes the value; the caller destroyed the old ones.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Repeat(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 vector, MAREA_Uinteger32 header, MAREA_Uinteger32 first, MAREA_Uinteger32 limit, MAREA_Uinteger32 value)
{
    MAREA_BytecodeGeneratorCount loop = { 0 };

    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoopOpen(generator, node, &loop, first, limit));
    MAREA_BYTECODE_TRY(MAREA_G_Element(generator, node, vector, header, loop.index, &element));
    MAREA_BYTECODE_TRY(MAREA_G_Store(generator, vector, element, value));

    return MAREA_BytecodeGeneratorLoopClose(generator, node, &loop);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Add(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member, MAREA_Boolean isInsert)
{
    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);

    if (isInsert) {
        MAREA_BYTECODE_TRY(MAREA_G_Number(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(0)), &index));
    }

    // INFO: the value is built before the vector changes, since it may be one of its own elements.
    MAREA_BYTECODE_TRY(MAREA_G_Value(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, (isInsert ? MAREA_INTEGER_SIZE(1) : MAREA_INTEGER_SIZE(0))), member->vector, &value));

    MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, member->node, member->header, MAREA_G_SIZE, &size));

    if (isInsert) {
        MAREA_BYTECODE_TRY(MAREA_G_TrapBelow(generator, member->node, size, index, MAREA_BOOLEAN_FALSE));
    } else {
        index = size;
    }

    MAREA_BYTECODE_TRY(MAREA_G_Runtime(generator, member->node, "marea.vector.runtime_open", member->vector, member->header, &index));
    MAREA_BYTECODE_TRY(MAREA_G_Element(generator, member->node, member->vector, member->header, index, &element));

    return MAREA_G_Store(generator, member->vector, element, value);
}

// INFO: pop and erase give the element up; the slot then leaves the vector.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Take(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member, MAREA_Boolean isErase)
{
    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);

    if (isErase) {
        MAREA_BYTECODE_TRY(MAREA_G_Number(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(0)), &index));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, member->node, member->header, MAREA_G_SIZE, &size));
        MAREA_BYTECODE_TRY(MAREA_G_TrapBelow(generator, member->node, size, index, MAREA_BOOLEAN_TRUE));
    } else {
        MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, member->node, member->header, MAREA_G_SIZE, &index));
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, member->node, MAREA_UINTEGER_64(1), &one));
        MAREA_BYTECODE_TRY(MAREA_G_TrapBelow(generator, member->node, index, one, MAREA_BOOLEAN_FALSE));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_SUB, index, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
    }

    MAREA_BYTECODE_TRY(MAREA_G_Element(generator, member->node, member->vector, member->header, index, &element));
    MAREA_BYTECODE_TRY(MAREA_G_Result(generator, member, element));

    if (!isErase) {
        return MAREA_BytecodeGeneratorStoreWord(generator, member->node, member->header, MAREA_G_SIZE, index);
    }

    MAREA_Uinteger32 values[2] = { index, 0 };

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, member->node, generator->aggregates[member->vector].stride, &values[1]));

    return MAREA_BytecodeGeneratorRuntimeCall(generator, member->node, "marea.vector.runtime_close", member->vector, member->header, values, MAREA_UINTEGER_32(2));
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Resize(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member)
{
    MAREA_Uinteger32 count = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Number(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(0)), &count));
    MAREA_BYTECODE_TRY(MAREA_G_Value(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(1)), member->vector, &value));
    MAREA_BYTECODE_TRY(MAREA_G_Truncate(generator, member->node, member->vector, member->header, count));
    MAREA_BYTECODE_TRY(MAREA_G_Runtime(generator, member->node, "marea.vector.runtime_grow", member->vector, member->header, &count));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, member->node, member->header, MAREA_G_SIZE, &size));
    MAREA_BYTECODE_TRY(MAREA_G_Repeat(generator, member->node, member->vector, member->header, size, count, value));

    return MAREA_BytecodeGeneratorStoreWord(generator, member->node, member->header, MAREA_G_SIZE, count);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Swap(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member)
{
    MAREA_Uinteger32 other = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 aggregate = MAREA_BYTECODE_NONE;
    MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(0)), &other, &aggregate));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (aggregate != member->vector) {
        return MAREA_BytecodeGeneratorFail(generator, member->node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A swap takes a vector of the same type.");
    }

    const MAREA_Uinteger32 SIZE = (MAREA_Uinteger32)generator->aggregates[member->vector].size;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, member->node, MAREA_UINTEGER_32(1), &held));
    MAREA_BYTECODE_TRY(MAREA_G_Buffer(generator, member->node, SIZE, generator->aggregates[member->vector].alignment, held));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, held, member->header, SIZE, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, member->header, other, SIZE, MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, other, held, SIZE, MAREA_NULLPTR);
}

// INFO: the members that give a value: the two counts, the empty test, the largest size and the two ends.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Query(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member, const MAREA_IrNode *name, MAREA_Boolean *outHandled)
{
    const MAREA_IrNode *node = member->node;

    *outHandled = MAREA_BOOLEAN_TRUE;

    if (MAREA_G_TextIs(name, "size") || MAREA_G_TextIs(name, "capacity") || MAREA_G_TextIs(name, "empty")) {
        MAREA_Uinteger32 word = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, member->header, (MAREA_G_TextIs(name, "capacity") ? MAREA_G_CAPACITY : MAREA_G_SIZE), &word));

        if (!MAREA_G_TextIs(name, "empty")) {
            *member->outKind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64;

            return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, member->destination, word, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
        }

        MAREA_Uinteger32 zero = MAREA_UINTEGER_32(0);

        *member->outKind = MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN;

        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &zero));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_EQUAL, member->destination, word, zero, MAREA_NULLPTR);
    }

    if (MAREA_G_TextIs(name, "max_size")) {
        *member->outKind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64;

        return MAREA_BytecodeGeneratorEmitConstant(generator, node, member->destination, MAREA_VectorMaxSize(generator->aggregates[member->vector].stride));
    }

    if (MAREA_G_TextIs(name, "front") || MAREA_G_TextIs(name, "back")) {
        MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, member->header, MAREA_G_SIZE, &index));
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
        MAREA_BYTECODE_TRY(MAREA_G_TrapBelow(generator, node, index, one, MAREA_BOOLEAN_FALSE));

        if (MAREA_G_TextIs(name, "back")) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_SUB, index, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
        } else {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, index, MAREA_UINTEGER_64(0)));
        }

        MAREA_BYTECODE_TRY(MAREA_G_Element(generator, node, member->vector, member->header, index, &element));

        return MAREA_G_Result(generator, member, element);
    }

    *outHandled = MAREA_BOOLEAN_FALSE;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Change(MAREA_BytecodeGenerator *generator, const MAREA_G_Member *member, const MAREA_IrNode *name)
{
    const MAREA_IrNode *node = member->node;

    if (MAREA_G_TextIs(name, "append") || MAREA_G_TextIs(name, "insert")) {
        return MAREA_G_Add(generator, member, MAREA_G_TextIs(name, "insert"));
    }

    if (MAREA_G_TextIs(name, "pop") || MAREA_G_TextIs(name, "erase")) {
        return MAREA_G_Take(generator, member, MAREA_G_TextIs(name, "erase"));
    }

    if (MAREA_G_TextIs(name, "reserve")) {
        MAREA_Uinteger32 count = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_Number(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(0)), &count));

        return MAREA_G_Runtime(generator, node, "marea.vector.runtime_reserve", member->vector, member->header, &count);
    }

    if (MAREA_G_TextIs(name, "shrink_to_fit")) {
        return MAREA_G_Runtime(generator, node, "marea.vector.runtime_shrink", member->vector, member->header, MAREA_NULLPTR);
    }

    if (MAREA_G_TextIs(name, "clear")) {
        MAREA_Uinteger32 zero = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &zero));

        return MAREA_G_Truncate(generator, node, member->vector, member->header, zero);
    }

    if (MAREA_G_TextIs(name, "resize")) {
        return MAREA_G_Resize(generator, member);
    }

    if (MAREA_G_TextIs(name, "fill")) {
        MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 zero = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_Value(generator, MAREA_BytecodeGeneratorChildAt(member->arguments, MAREA_INTEGER_SIZE(0)), member->vector, &value));
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &zero));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, member->header, MAREA_G_SIZE, &size));

        return MAREA_G_Repeat(generator, node, member->vector, member->header, zero, size, value);
    }

    if (MAREA_G_TextIs(name, "swap")) {
        return MAREA_G_Swap(generator, member);
    }

    return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A member of a vector is not lowered.");
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorVectorCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 vector, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *callee = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *name = MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(1));

    MAREA_G_Member member = {
        .node = node,
        .arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)),
        .vector = vector,
        .header = MAREA_UINTEGER_32(0),
        .destination = destination,
        .outKind = outKind,
    };

    MAREA_Uinteger32 aggregate = MAREA_BYTECODE_NONE;

    *outKind = MAREA_BYTECODE_GENERATOR_KIND_VOID;

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(0)), &member.header, &aggregate));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Boolean handled = MAREA_BOOLEAN_FALSE;

    MAREA_BYTECODE_TRY(MAREA_G_Query(generator, &member, name, &handled));

    if (!handled) {
        MAREA_BYTECODE_TRY(MAREA_G_Change(generator, &member, name));
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructVector(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger32 vector)
{
    const MAREA_BytecodeGeneratorAggregate AGGREGATE = generator->aggregates[vector];

    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);
    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, destination, (MAREA_Uinteger32)AGGREGATE.size, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_Uinteger32 count = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, COUNT, &count));
        MAREA_BYTECODE_TRY(MAREA_G_Runtime(generator, node, "marea.vector.runtime_reserve", vector, destination, &count));
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index < COUNT) && !generator->functionFailed; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *entry = MAREA_BytecodeGeneratorChildAt(node, index);

        if (MAREA_IrNodeGetKind(entry) != MAREA_IR_KIND_VALUE_ENTRY) {
            return MAREA_BytecodeGeneratorFail(generator, entry, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A literal of a vector holds only values.");
        }

        MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 position = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 size = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_Value(generator, MAREA_BytecodeGeneratorChildAt(entry, MAREA_INTEGER_SIZE(0)), vector, &value));
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, entry, index, &position));
        MAREA_BYTECODE_TRY(MAREA_G_Element(generator, entry, vector, destination, position, &element));
        MAREA_BYTECODE_TRY(MAREA_G_Store(generator, vector, element, value));
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, entry, (index + MAREA_INTEGER_SIZE(1)), &size));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStoreWord(generator, entry, destination, MAREA_G_SIZE, size));

        generator->nextRegister = MARK;
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
