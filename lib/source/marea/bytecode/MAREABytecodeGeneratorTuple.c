// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"
#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include <string.h>

// INFO: a tuple is a structure with no declaration: one row of the type table for each distinct spelling, whose
// members are named by their places. This file lowers what a structure does not have: the constant index, the
// literal in order, == and != and swap.

static const MAREA_Character8 *const MAREA_G_PLACES[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15" };

// INFO: how deep a tuple of tuples is compared.
#define MAREA_G_DEPTH_LIMIT MAREA_UINTEGER_32(16)

// INFO: the next top-level type argument of a spelling, from the place *ioAt up to the comma or the end.
static MAREA_Boolean MAREA_CALL MAREA_G_NextArgument(const MAREA_Character8 *text, MAREA_IntegerSize end, MAREA_IntegerSize *ioAt, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (*ioAt >= end) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize from = *ioAt;
    MAREA_IntegerSize to = from;

    MAREA_Sinteger32 depth = MAREA_SINTEGER_32(0);

    while ((to < end) && ((text[to] != ',') || (depth != MAREA_SINTEGER_32(0)))) {
        if ((text[to] == '<') || (text[to] == '(')) {
            depth += MAREA_SINTEGER_32(1);
        } else if ((text[to] == '>') || (text[to] == ')')) {
            depth -= MAREA_SINTEGER_32(1);
        }

        to += MAREA_INTEGER_SIZE(1);
    }

    *ioAt = (to + MAREA_INTEGER_SIZE(1));

    while ((from < to) && (text[from] == ' ')) {
        from += MAREA_INTEGER_SIZE(1);
    }

    while ((to > from) && (text[(to - MAREA_INTEGER_SIZE(1))] == ' ')) {
        to -= MAREA_INTEGER_SIZE(1);
    }

    *outText = (text + from);
    *outLength = (to - from);

    return MAREA_BOOLEAN_TRUE;
}

// INFO: one element at the next multiple of its alignment. outFound is false for a type that is not lowered.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_ElementOfText(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_BytecodeGeneratorElement *outElement, MAREA_Uinteger32 *outType, MAREA_Uinteger64 *outSize, MAREA_Uinteger64 *outAlignment, MAREA_Boolean *outFound)
{
    *outFound = MAREA_BOOLEAN_FALSE;

    outElement->aggregate = MAREA_BYTECODE_NONE;
    outElement->kind = MAREA_BytecodeGeneratorKindFromText(text, length);

    if (outElement->kind == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTextKind(generator, node, text, length, &outElement->kind, &outElement->aggregate));

        if (outElement->kind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        *outType = generator->aggregates[outElement->aggregate].typeIndex;
        *outSize = generator->aggregates[outElement->aggregate].size;
        *outAlignment = generator->aggregates[outElement->aggregate].alignment;
    } else {
        if (outElement->kind == MAREA_BYTECODE_GENERATOR_KIND_VOID) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, outElement->kind, outType));

        MAREA_BytecodeTypeInfo info = { 0 };

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleGetType(generator->module, *outType, &info));

        *outSize = info.size;
        *outAlignment = info.alignment;
    }

    *outAlignment = ((*outAlignment == MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64(1) : *outAlignment);
    *outFound = MAREA_BOOLEAN_TRUE;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_RoundUp(MAREA_Uinteger64 value, MAREA_Uinteger64 alignment)
{
    return ((((value + alignment) - MAREA_UINTEGER_64(1)) / alignment) * alignment);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTupleKind(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outAggregateIndex)
{
    *outAggregateIndex = MAREA_BYTECODE_NONE;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->aggregateCount; index += MAREA_INTEGER_SIZE(1)) {
        if ((generator->aggregates[index].nameLength == length) && (memcmp(generator->aggregates[index].name, text, length) == 0)) {
            *outAggregateIndex = (MAREA_Uinteger32)index;

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    MAREA_IntegerSize at = MAREA_INTEGER_SIZE(0);

    while ((at < length) && (text[at] != '<')) {
        at += MAREA_INTEGER_SIZE(1);
    }

    if (((at + MAREA_INTEGER_SIZE(1)) >= length) || (text[(length - MAREA_INTEGER_SIZE(1))] != '>')) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    at += MAREA_INTEGER_SIZE(1);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, (MAREA_TYPE_TUPLE_MAXIMUM_ARITY * sizeof(MAREA_BytecodeGeneratorElement)), MAREA_ALIGNOF(MAREA_BytecodeGeneratorElement), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_BytecodeGeneratorElement *elements = memory;

    MAREA_Uinteger32 types[16] = { 0 };

    MAREA_BytecodeGeneratorAggregate made = { 0 };

    made.name = text;
    made.nameLength = length;
    made.isTuple = MAREA_BOOLEAN_TRUE;
    made.elements = elements;
    made.elementAggregate = MAREA_BYTECODE_NONE;
    made.alignment = MAREA_UINTEGER_64(1);

    const MAREA_Character8 *element = MAREA_NULLPTR;
    MAREA_IntegerSize elementLength = MAREA_INTEGER_SIZE(0);

    while (MAREA_G_NextArgument(text, (length - MAREA_INTEGER_SIZE(1)), &at, &element, &elementLength)) {
        if (made.count >= MAREA_TYPE_TUPLE_MAXIMUM_ARITY) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        MAREA_Uinteger64 size = MAREA_UINTEGER_64(0);
        MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(1);
        MAREA_Boolean found = MAREA_BOOLEAN_FALSE;

        MAREA_BYTECODE_TRY(MAREA_G_ElementOfText(generator, node, element, elementLength, &elements[made.count], &types[made.count], &size, &alignment, &found));

        if (!found) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        elements[made.count].offset = MAREA_G_RoundUp(made.size, alignment);

        made.size = (elements[made.count].offset + size);
        made.alignment = ((alignment > made.alignment) ? alignment : made.alignment);
        made.count += MAREA_UINTEGER_64(1);
    }

    made.size = MAREA_G_RoundUp(made.size, made.alignment);

    MAREA_Uinteger32 nameString = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, text, length, &nameString));

    const MAREA_BytecodeTypeInfo INFO = {
        .size = made.size,
        .count = MAREA_UINTEGER_64(0),
        .name = nameString,
        .element = MAREA_BYTECODE_NONE,
        .alignment = (MAREA_Uinteger32)made.alignment,
        .memberCount = MAREA_UINTEGER_32(0),
        .kind = MAREA_BYTECODE_TYPE_KIND_STRUCTURE,
    };

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddType(generator->module, &INFO, &made.typeIndex));

    for (MAREA_Uinteger64 member = MAREA_UINTEGER_64(0); member < made.count; member += MAREA_UINTEGER_64(1)) {
        MAREA_Uinteger32 memberName = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, MAREA_G_PLACES[member], strlen(MAREA_G_PLACES[member]), &memberName));

        const MAREA_BytecodeMemberInfo MEMBER_INFO = {
            .value = elements[member].offset,
            .name = memberName,
            .type = types[member],
        };

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddMember(generator->module, made.typeIndex, &MEMBER_INFO));
    }

    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->aggregates, &generator->aggregateCapacity, generator->aggregateCount, sizeof(MAREA_BytecodeGeneratorAggregate), MAREA_ALIGNOF(MAREA_BytecodeGeneratorAggregate), &items));

    generator->aggregates = items;
    generator->aggregates[generator->aggregateCount] = made;

    *outAggregateIndex = (MAREA_Uinteger32)generator->aggregateCount;

    generator->aggregateCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTuplePlace(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 tuple, MAREA_Uinteger32 base, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex)
{
    const MAREA_IrNode *key = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

    MAREA_Uinteger64 position = MAREA_UINTEGER_64(0);

    if ((MAREA_IrNodeGetKind(key) != MAREA_IR_KIND_INTEGER_LITERAL) || !MAREA_BytecodeGeneratorParseInteger(MAREA_IrNodeGetText(key), MAREA_IrNodeGetTextLength(key), &position) || (position >= generator->aggregates[tuple].count)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "The index of a tuple is an integer literal below its arity.");
    }

    const MAREA_BytecodeGeneratorElement ELEMENT = generator->aggregates[tuple].elements[position];

    *outKind = ELEMENT.kind;
    *outAggregateIndex = ELEMENT.aggregate;

    return MAREA_BytecodeGeneratorAt(generator, node, base, ELEMENT.offset, outRegister);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructTuple(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 tuple)
{
    // INFO: a copy of the entry, since the table may move while a value inside it is lowered.
    const MAREA_BytecodeGeneratorAggregate TUPLE = generator->aggregates[tuple];

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, destinationAddress, (MAREA_Uinteger32)TUPLE.size, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);
    const MAREA_Uinteger32 MARK = generator->nextRegister;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index < COUNT) && !generator->functionFailed; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *entry = MAREA_BytecodeGeneratorChildAt(node, index);

        if ((MAREA_IrNodeGetKind(entry) != MAREA_IR_KIND_VALUE_ENTRY) || (index >= TUPLE.count)) {
            return MAREA_BytecodeGeneratorFail(generator, entry, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A tuple literal holds one value for each element, in order.");
        }

        MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, entry, destinationAddress, TUPLE.elements[index].offset, &address));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStoreElement(generator, MAREA_BytecodeGeneratorChildAt(entry, MAREA_INTEGER_SIZE(0)), address, TUPLE.elements[index].kind, TUPLE.elements[index].aggregate));

        generator->nextRegister = MARK;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: the address of bytes of this frame that hold a copy of a tuple.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Hold(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 tuple, MAREA_Uinteger32 source, MAREA_Uinteger32 *outRegister)
{
    const MAREA_Uinteger64 SIZE = generator->aggregates[tuple].size;

    MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, SIZE, generator->aggregates[tuple].alignment, &offset));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, *outRegister, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, *outRegister, source, (MAREA_Uinteger32)SIZE, MAREA_NULLPTR);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTupleCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 tuple, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *callee = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *member = MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(1));
    const MAREA_IrNode *arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

    *outKind = MAREA_BYTECODE_GENERATOR_KIND_VOID;

    const MAREA_Boolean IS_SWAP = ((MAREA_IrNodeGetTextLength(member) == strlen("swap")) && (memcmp(MAREA_IrNodeGetText(member), "swap", strlen("swap")) == 0));

    if (!IS_SWAP || (arguments == MAREA_NULLPTR) || (MAREA_IrNodeGetChildCount(arguments) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "swap is the only method of a tuple.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;
    const MAREA_Uinteger32 SIZE = (MAREA_Uinteger32)generator->aggregates[tuple].size;

    MAREA_Uinteger32 left = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 right = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 leftAggregate = MAREA_BYTECODE_NONE;
    MAREA_Uinteger32 rightAggregate = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(0)), &left, &leftAggregate));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, MAREA_BytecodeGeneratorChildAt(arguments, MAREA_INTEGER_SIZE(0)), &right, &rightAggregate));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Hold(generator, node, tuple, left, &held));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, left, right, SIZE, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, right, held, SIZE, MAREA_NULLPTR));

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: clears the destination at the first element that differs.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Equality(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 tuple, MAREA_Uinteger32 left, MAREA_Uinteger32 right, MAREA_Uinteger32 destination, MAREA_Uinteger32 depth)
{
    const MAREA_BytecodeGeneratorAggregate TUPLE = generator->aggregates[tuple];

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    for (MAREA_Uinteger64 element = MAREA_UINTEGER_64(0); (element < TUPLE.count) && !generator->functionFailed; element += MAREA_UINTEGER_64(1)) {
        const MAREA_BytecodeGeneratorElement ELEMENT = TUPLE.elements[element];

        MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 other = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, left, ELEMENT.offset, &one));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAt(generator, node, right, ELEMENT.offset, &other));

        if (ELEMENT.kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
            if ((depth >= MAREA_G_DEPTH_LIMIT) || !generator->aggregates[ELEMENT.aggregate].isTuple) {
                return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An element of a tuple has no == that is lowered.");
            }

            MAREA_BYTECODE_TRY(MAREA_G_Equality(generator, node, ELEMENT.aggregate, one, other, destination, (depth + MAREA_UINTEGER_32(1))));
        } else {
            const MAREA_Uinteger8 WIDTH = ((ELEMENT.kind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 : ELEMENT.kind);

            const MAREA_Boolean IS_FLOAT = ((WIDTH == MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) || (WIDTH == MAREA_BYTECODE_NUMBER_KIND_FLOAT_64));

            MAREA_Uinteger32 first = MAREA_UINTEGER_32(0);
            MAREA_Uinteger32 second = MAREA_UINTEGER_32(0);
            MAREA_Uinteger32 test = MAREA_UINTEGER_32(0);
            MAREA_Uinteger32 same = MAREA_UINTEGER_32(0);

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &first));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &second));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &test));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, first, one, WIDTH, MAREA_NULLPTR));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, second, other, WIDTH, MAREA_NULLPTR));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, (IS_FLOAT ? MAREA_BYTECODE_OPCODE_EQUAL_FLOAT : MAREA_BYTECODE_OPCODE_EQUAL), test, first, second, MAREA_NULLPTR));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF, test, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &same));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, MAREA_UINTEGER_64(0)));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPatchJump(generator, node, same, (MAREA_Uinteger32)generator->codeCount));
        }

        generator->nextRegister = MARK;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTupleComparison(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 tuple, MAREA_Uinteger32 destination)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    if ((OPERATOR != MAREA_TOKEN_KIND_EQUAL_EQUAL) && (OPERATOR != MAREA_TOKEN_KIND_BANG_EQUAL)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A tuple has no order.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger32 left = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 right = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 leftAggregate = MAREA_BYTECODE_NONE;
    MAREA_Uinteger32 rightAggregate = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)), &left, &leftAggregate));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    // INFO: operands run in the order of the source, so the left one is held before the right one runs.
    MAREA_BYTECODE_TRY(MAREA_G_Hold(generator, node, tuple, left, &held));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), &right, &rightAggregate));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (rightAggregate != tuple) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A comparison needs two tuples of one type.");
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, MAREA_UINTEGER_64(1)));
    MAREA_BYTECODE_TRY(MAREA_G_Equality(generator, node, tuple, held, right, destination, MAREA_UINTEGER_32(0)));

    generator->nextRegister = MARK;

    if (OPERATOR == MAREA_TOKEN_KIND_BANG_EQUAL) {
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_NOT, destination, destination, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
