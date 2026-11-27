// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"
#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include "marea/base/MAREAVectorRuntime.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_IrNode *node, const MAREA_Character8 *expected)
{
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(node);

    return ((strlen(expected) == LENGTH) && (memcmp(MAREA_IrNodeGetText(node), expected, LENGTH) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the descriptor of a dotted path, whose last segment is the type name.
static const MAREA_TypeDescriptor *MAREA_CALL MAREA_G_Describe(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *path, MAREA_IntegerSize length)
{
    MAREA_IntegerSize dot = length;

    while ((dot > MAREA_INTEGER_SIZE(0)) && (path[(dot - MAREA_INTEGER_SIZE(1))] != '.')) {
        dot = (dot - MAREA_INTEGER_SIZE(1));
    }

    if (dot == MAREA_INTEGER_SIZE(0)) {
        return MAREA_NULLPTR;
    }

    return MAREA_TypeRegistryFind(generator->context->registry, path, (dot - MAREA_INTEGER_SIZE(1)), (path + dot), (length - dot));
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_HeadLength(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_IntegerSize head = MAREA_INTEGER_SIZE(0);

    while ((head < length) && (text[head] != '<') && (text[head] != '(')) {
        head = (head + MAREA_INTEGER_SIZE(1));
    }

    return head;
}

// INFO: a spelling that goes through an alias is no registered path, so the full path is read from a node of the
// module that spells the same type: its type name is the full path.
static const MAREA_TypeDescriptor *MAREA_CALL MAREA_G_DescribeSpelling(const MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *head, MAREA_IntegerSize length)
{
    const MAREA_Character8 *resolved = MAREA_IrNodeGetResolvedType(node);
    const MAREA_Character8 *name = MAREA_IrNodeGetTypeName(node);

    if ((resolved != MAREA_NULLPTR) && (name != MAREA_NULLPTR) && (MAREA_G_HeadLength(resolved, MAREA_IrNodeGetResolvedTypeLength(node)) == length) && (memcmp(resolved, head, length) == 0)) {
        const MAREA_TypeDescriptor *found = MAREA_G_Describe(generator, name, MAREA_IrNodeGetTypeNameLength(node));

        if (found != MAREA_NULLPTR) {
            return found;
        }
    }

    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_TypeDescriptor *found = MAREA_G_DescribeSpelling(generator, MAREA_BytecodeGeneratorChildAt(node, index), head, length);

        if (found != MAREA_NULLPTR) {
            return found;
        }
    }

    return MAREA_NULLPTR;
}

const MAREA_TypeDescriptor *MAREA_CALL MAREA_BytecodeGeneratorDescribe(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((generator->context->registry == MAREA_NULLPTR) || (text == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize HEAD = MAREA_G_HeadLength(text, length);

    if ((HEAD >= length) || (text[HEAD] != '<')) {
        return MAREA_NULLPTR;
    }

    const MAREA_TypeDescriptor *descriptor = MAREA_G_Describe(generator, text, HEAD);

    return (descriptor != MAREA_NULLPTR) ? descriptor : MAREA_G_DescribeSpelling(generator, generator->root, text, HEAD);
}

// INFO: splits the resolved spelling of a run into its element and, for an inline run, its literal count. The shapes
// are the ones that sema reads: an index protocol and no role; an inline run, head<element,count>, has a copy class
// and two type parameters; a growable run, head<element>, a move class and one.
static MAREA_Boolean MAREA_CALL MAREA_G_SplitRun(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 **outElement, MAREA_IntegerSize *outElementLength, MAREA_Uinteger64 *outCount, MAREA_Boolean *outIsVector)
{
    if ((generator->context->registry == MAREA_NULLPTR) || (text == MAREA_NULLPTR) || (length == MAREA_INTEGER_SIZE(0)) || (text[(length - MAREA_INTEGER_SIZE(1))] != '>')) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize HEAD = MAREA_G_HeadLength(text, length);

    if ((HEAD >= length) || (text[HEAD] != '<')) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_TypeDescriptor *descriptor = MAREA_G_Describe(generator, text, HEAD);

    if (descriptor == MAREA_NULLPTR) {
        descriptor = MAREA_G_DescribeSpelling(generator, generator->root, text, HEAD);
    }

    if ((descriptor == MAREA_NULLPTR) || (descriptor->hasIndexProtocol == MAREA_BOOLEAN_FALSE) || (descriptor->roles != MAREA_TYPE_ROLE_NONE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outIsVector = ((descriptor->ownershipClass == MAREA_TYPE_OWNERSHIP_CLASS_MOVE) && (descriptor->typeParameterCount == MAREA_INTEGER_SIZE(1)));
    *outCount = MAREA_UINTEGER_64(0);

    if (*outIsVector) {
        MAREA_IntegerSize from = (HEAD + MAREA_INTEGER_SIZE(1));
        MAREA_IntegerSize to = (length - MAREA_INTEGER_SIZE(1));

        while ((from < to) && (text[from] == ' ')) {
            from += MAREA_INTEGER_SIZE(1);
        }

        while ((to > from) && (text[(to - MAREA_INTEGER_SIZE(1))] == ' ')) {
            to -= MAREA_INTEGER_SIZE(1);
        }

        *outElement = (text + from);
        *outElementLength = (to - from);

        return (to > from);
    }

    if ((descriptor->ownershipClass != MAREA_TYPE_OWNERSHIP_CLASS_COPY) || (descriptor->typeParameterCount != MAREA_INTEGER_SIZE(2))) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize comma = length;
    MAREA_Sinteger32 depth = MAREA_SINTEGER_32(0);

    for (MAREA_IntegerSize index = (HEAD + MAREA_INTEGER_SIZE(1)); index < (length - MAREA_INTEGER_SIZE(1)); index += MAREA_INTEGER_SIZE(1)) {
        if ((text[index] == '<') || (text[index] == '(')) {
            depth += MAREA_SINTEGER_32(1);
        } else if ((text[index] == '>') || (text[index] == ')')) {
            depth -= MAREA_SINTEGER_32(1);
        } else if ((text[index] == ',') && (depth == MAREA_SINTEGER_32(0))) {
            comma = index;
        }
    }

    if (comma == length) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize first = (HEAD + MAREA_INTEGER_SIZE(1));
    MAREA_IntegerSize last = comma;
    MAREA_IntegerSize second = (comma + MAREA_INTEGER_SIZE(1));

    while ((first < last) && (text[first] == ' ')) {
        first += MAREA_INTEGER_SIZE(1);
    }

    while ((last > first) && (text[(last - MAREA_INTEGER_SIZE(1))] == ' ')) {
        last -= MAREA_INTEGER_SIZE(1);
    }

    while ((second < length) && (text[second] == ' ')) {
        second += MAREA_INTEGER_SIZE(1);
    }

    *outElement = (text + first);
    *outElementLength = (last - first);

    return MAREA_BytecodeGeneratorParseInteger((text + second), (length - second - MAREA_INTEGER_SIZE(1)), outCount);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorRunKind(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outAggregateIndex)
{
    *outAggregateIndex = MAREA_BYTECODE_NONE;

    const MAREA_Character8 *element = MAREA_NULLPTR;
    MAREA_IntegerSize elementLength = MAREA_INTEGER_SIZE(0);
    MAREA_Uinteger64 count = MAREA_UINTEGER_64(0);
    MAREA_Boolean isVector = MAREA_BOOLEAN_FALSE;

    const MAREA_TypeDescriptor *descriptor = MAREA_BytecodeGeneratorDescribe(generator, text, length);

    // INFO: a tuple is passed and held by its bytes, as a run is.
    if ((descriptor != MAREA_NULLPTR) && MAREA_TYPE_DESCRIPTOR_IS_TUPLE(descriptor)) {
        return MAREA_BytecodeGeneratorTupleKind(generator, node, text, length, outAggregateIndex);
    }

    if (!MAREA_G_SplitRun(generator, text, length, &element, &elementLength, &count, &isVector)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->aggregateCount; index += MAREA_INTEGER_SIZE(1)) {
        if ((generator->aggregates[index].nameLength == length) && (memcmp(generator->aggregates[index].name, text, length) == 0)) {
            *outAggregateIndex = (MAREA_Uinteger32)index;

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    MAREA_BytecodeGeneratorAggregate made = { 0 };

    made.name = text;
    made.nameLength = length;
    made.isRun = !isVector;
    made.isVector = isVector;
    made.count = count;
    made.elementAggregate = MAREA_BYTECODE_NONE;
    made.elementKind = MAREA_BytecodeGeneratorKindFromText(element, elementLength);

    MAREA_Uinteger32 elementType = MAREA_UINTEGER_32(0);

    if (made.elementKind == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTextKind(generator, node, element, elementLength, &made.elementKind, &made.elementAggregate));

        if (made.elementKind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        elementType = generator->aggregates[made.elementAggregate].typeIndex;
        made.stride = generator->aggregates[made.elementAggregate].size;
        made.alignment = generator->aggregates[made.elementAggregate].alignment;
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, made.elementKind, &elementType));

        MAREA_BytecodeTypeInfo elementInfo = { 0 };

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleGetType(generator->module, elementType, &elementInfo));

        made.stride = elementInfo.size;
        made.alignment = elementInfo.alignment;
    }

    made.elementAlignment = ((made.alignment == MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64(1) : made.alignment);

    // INFO: a run that cannot fit the frame area is reported where a place of it is reserved.
    made.size = ((made.stride != MAREA_UINTEGER_64(0)) && (count > (MAREA_UINTEGER_64(0xFFFFFFFF) / made.stride))) ? MAREA_UINTEGER_64(0xFFFFFFFF) : (made.stride * count);

    // INFO: a vector is the header of the runtime, whatever its element is.
    if (isVector) {
        made.size = sizeof(MAREA_VectorHeader);
        made.alignment = MAREA_ALIGNOF(MAREA_VectorHeader);
    }

    MAREA_Uinteger32 nameString = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, text, length, &nameString));

    const MAREA_BytecodeTypeInfo INFO = {
        .size = made.size,
        .count = count,
        .name = nameString,
        .element = elementType,
        .alignment = (MAREA_Uinteger32)made.alignment,
        .memberCount = MAREA_UINTEGER_32(0),
        .kind = (isVector ? MAREA_BYTECODE_TYPE_KIND_VECTOR : MAREA_BYTECODE_TYPE_KIND_ARRAY),
    };

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddType(generator->module, &INFO, &made.typeIndex));

    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->aggregates, &generator->aggregateCapacity, generator->aggregateCount, sizeof(MAREA_BytecodeGeneratorAggregate), MAREA_ALIGNOF(MAREA_BytecodeGeneratorAggregate), &items));

    generator->aggregates = items;
    generator->aggregates[generator->aggregateCount] = made;

    *outAggregateIndex = (MAREA_Uinteger32)generator->aggregateCount;

    generator->aggregateCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
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

// INFO: destination = base + index * stride, with the address arithmetic on the bit pattern.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_ElementAt(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger32 index, MAREA_Uinteger64 stride, MAREA_Uinteger32 destination)
{
    MAREA_Uinteger32 scale = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, stride, &scale));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MUL, scale, index, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, scale, base, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, destination, scale, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
}

// INFO: an index is compared as an unsigned value, so a negative one, which a register holds sign extended, is past
// the count too.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_CheckIndex(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 index, MAREA_Uinteger64 count)
{
    MAREA_Uinteger32 limit = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, count, &limit));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LESS_UNSIGNED, limit, index, limit, MAREA_NULLPTR));

    MAREA_Uinteger32 inside = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF, limit, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &inside));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_INDEX_OUT_OF_BOUNDS, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, inside, (MAREA_Uinteger32)generator->codeCount);
}

// INFO: the register receives the address of the element at a constant position that is known to exist.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_ElementField(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger64 offset, MAREA_Uinteger32 *outRegister)
{
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_FIELD, *outRegister, base, (MAREA_Uinteger32)offset, MAREA_NULLPTR);
}

// INFO: the element of a raw pointer: the address is the value of the pointer, and no count limits the index.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_PointerPlace(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *pointer, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind)
{
    const MAREA_Character8 *element = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    MAREA_IGNORE(MAREA_BytecodeGeneratorPointee(generator, MAREA_IrNodeGetResolvedType(pointer), MAREA_IrNodeGetResolvedTypeLength(pointer), &element, &length));

    const MAREA_Uinteger8 KIND = MAREA_BytecodeGeneratorKindFromText(element, length);

    if (!((KIND < MAREA_BYTECODE_NUMBER_KIND_COUNT) || (KIND == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A pointer reaches a number or a boolean only.");
    }

    // INFO: the bytes of each number kind, in the order of MAREA_BytecodeNumberKind.
    static const MAREA_Uinteger8 SIZES[MAREA_BYTECODE_NUMBER_KIND_COUNT] = { 1, 2, 4, 8, 1, 2, 4, 8, 4, 8 };

    const MAREA_Uinteger64 STRIDE = ((KIND == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_UINTEGER_64(1) : (MAREA_Uinteger64)SIZES[KIND]);

    MAREA_Uinteger32 base = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 baseKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger8 indexKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, pointer, MAREA_BYTECODE_GENERATOR_KIND_NONE, &base, &baseKind));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), MAREA_BYTECODE_GENERATOR_KIND_NONE, &index, &indexKind));

    if (indexKind >= MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An index is an integer.");
    }

    *outKind = KIND;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_G_ElementAt(generator, node, base, index, STRIDE, *outRegister);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorElementPlace(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex)
{
    MAREA_Uinteger32 base = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 run = MAREA_BYTECODE_NONE;

    const MAREA_IrNode *indexed = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    if (MAREA_BytecodeGeneratorIsPointer(generator, indexed)) {
        return MAREA_G_PointerPlace(generator, node, indexed, outRegister, outKind);
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, indexed, &base, &run));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (generator->aggregates[run].isVector) {
        *outKind = generator->aggregates[run].elementKind;
        *outAggregateIndex = generator->aggregates[run].elementAggregate;

        return MAREA_BytecodeGeneratorVectorPlace(generator, node, run, base, outRegister);
    }

    if (generator->aggregates[run].isTuple) {
        return MAREA_BytecodeGeneratorTuplePlace(generator, node, run, base, outRegister, outKind, outAggregateIndex);
    }

    if (!generator->aggregates[run].isRun) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only an array or a vector is indexed.");
    }

    const MAREA_Uinteger64 COUNT = generator->aggregates[run].count;
    const MAREA_Uinteger64 STRIDE = generator->aggregates[run].stride;

    *outKind = generator->aggregates[run].elementKind;
    *outAggregateIndex = generator->aggregates[run].elementAggregate;

    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 indexKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), MAREA_BYTECODE_GENERATOR_KIND_NONE, &index, &indexKind));

    if (indexKind >= MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An index is an integer.");
    }

    MAREA_BYTECODE_TRY(MAREA_G_CheckIndex(generator, node, index, COUNT));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_G_ElementAt(generator, node, base, index, STRIDE, *outRegister);
}

// INFO: stores one value at the address of an element.
MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorStoreElement(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *value, MAREA_Uinteger32 address, MAREA_Uinteger8 kind, MAREA_Uinteger32 aggregateIndex)
{
    if (kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_BytecodeGeneratorConstructAggregate(generator, value, address, aggregateIndex);
    }

    MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 got = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, value, MAREA_UINTEGER_32(1), &held));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, value, held, kind, &got));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, held, MAREA_G_Width(kind), MAREA_NULLPTR);
}

// INFO: a loop that gives every element of a run the same value: a register, or the bytes of a run or a structure.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Fill(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger32 run, MAREA_Uinteger32 value)
{
    const MAREA_Uinteger64 COUNT = generator->aggregates[run].count;
    const MAREA_Uinteger64 STRIDE = generator->aggregates[run].stride;
    const MAREA_Uinteger8 KIND = generator->aggregates[run].elementKind;

    if (COUNT == MAREA_UINTEGER_64(0)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 limit = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 test = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(0), &index));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, COUNT, &limit));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &test));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &address));

    const MAREA_Uinteger32 START = (MAREA_Uinteger32)generator->codeCount;

    MAREA_BYTECODE_TRY(MAREA_G_ElementAt(generator, node, base, index, STRIDE, address));

    if (KIND == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, address, value, (MAREA_Uinteger32)STRIDE, MAREA_NULLPTR));
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, address, value, MAREA_G_Width(KIND), MAREA_NULLPTR));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, index, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LESS_UNSIGNED, test, index, limit, MAREA_NULLPTR));

    MAREA_Uinteger32 back = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF, test, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &back));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, back, START);
}

// INFO: the value that fills a run, held in bytes of its own, since it may be an element of the run that is written.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_HoldValue(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *value, MAREA_Uinteger8 kind, MAREA_Uinteger32 aggregateIndex, MAREA_Uinteger32 *outRegister)
{
    if (kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_BytecodeGeneratorMaterialize(generator, value, aggregateIndex, outRegister);
    }

    MAREA_Uinteger8 got = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, value, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorLowerInto(generator, value, *outRegister, kind, &got);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Swap(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger32 run, const MAREA_IrNode *argument)
{
    MAREA_Uinteger32 other = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 otherRun = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, argument, &other, &otherRun));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (otherRun != run) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A swap takes an array of the same type.");
    }

    const MAREA_Uinteger64 SIZE = generator->aggregates[run].size;

    MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, SIZE, generator->aggregates[run].alignment, &offset));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &held));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, held, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, held, base, (MAREA_Uinteger32)SIZE, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, base, other, (MAREA_Uinteger32)SIZE, MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, other, held, (MAREA_Uinteger32)SIZE, MAREA_NULLPTR);
}

// INFO: front and back read the element at one end; with no element they are the trap of an index out of bounds.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_End(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 base, MAREA_Uinteger32 run, MAREA_Boolean back, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_BytecodeGeneratorAggregate RUN = generator->aggregates[run];

    *outKind = RUN.elementKind;

    if (RUN.count == MAREA_UINTEGER_64(0)) {
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_INDEX_OUT_OF_BOUNDS, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_ElementField(generator, node, base, (back ? ((RUN.count - MAREA_UINTEGER_64(1)) * RUN.stride) : MAREA_UINTEGER_64(0)), &address));

    if (RUN.elementKind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, destination, address, MAREA_G_Width(RUN.elementKind), MAREA_NULLPTR);
    }

    // INFO: the result is a copy of the element, in bytes of its own.
    MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, RUN.stride, generator->aggregates[RUN.elementAggregate].alignment, &offset));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, destination, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, destination, address, (MAREA_Uinteger32)RUN.stride, MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_Filled(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *arguments, MAREA_Uinteger32 destination, MAREA_Uinteger32 run)
{
    const MAREA_BytecodeGeneratorAggregate RUN = generator->aggregates[run];

    MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_HoldValue(generator, MAREA_BytecodeGeneratorChildAt(arguments, MAREA_INTEGER_SIZE(0)), RUN.elementKind, RUN.elementAggregate, &value));

    return MAREA_G_Fill(generator, node, destination, run, value);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorRunCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Boolean *outHandled, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *callee = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));
    const MAREA_IrNode *base = MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *member = MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(1));

    *outHandled = MAREA_BOOLEAN_FALSE;

    if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean IS_STATIC = (MAREA_IrNodeGetKind(base) == MAREA_IR_KIND_TYPE_NAME);

    const MAREA_IrNode *typed = (IS_STATIC ? node : base);

    MAREA_Uinteger32 run = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorRunKind(generator, typed, MAREA_IrNodeGetResolvedType(typed), MAREA_IrNodeGetResolvedTypeLength(typed), &run));

    if (run == MAREA_BYTECODE_NONE) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    *outHandled = MAREA_BOOLEAN_TRUE;

    if (generator->aggregates[run].isTuple) {
        return MAREA_BytecodeGeneratorTupleCall(generator, node, run, outKind);
    }

    if (generator->aggregates[run].isVector && !IS_STATIC) {
        return MAREA_BytecodeGeneratorVectorCall(generator, node, run, destination, outKind);
    }
    *outKind = MAREA_BYTECODE_GENERATOR_KIND_VOID;

    const MAREA_IntegerSize GIVEN = MAREA_IrNodeGetChildCount(arguments);

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    if (IS_STATIC) {
        if (!MAREA_G_TextIs(member, "filled") || (GIVEN != MAREA_INTEGER_SIZE(1))) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only filled is a static function of an array.");
        }

        MAREA_Uinteger32 offset = MAREA_UINTEGER_32(0);

        *outKind = MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocateFrame(generator, node, generator->aggregates[run].size, generator->aggregates[run].alignment, &offset));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADDRESS_OF, destination, offset, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

        return MAREA_G_Filled(generator, node, arguments, destination, run);
    }

    MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 got = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, base, &address, &got));

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if ((MAREA_G_TextIs(member, "size") || MAREA_G_TextIs(member, "max_size")) && (GIVEN == MAREA_INTEGER_SIZE(0))) {
        *outKind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, generator->aggregates[run].count));
    } else if (MAREA_G_TextIs(member, "empty") && (GIVEN == MAREA_INTEGER_SIZE(0))) {
        *outKind = MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, ((generator->aggregates[run].count == MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0))));
    } else if ((MAREA_G_TextIs(member, "front") || MAREA_G_TextIs(member, "back")) && (GIVEN == MAREA_INTEGER_SIZE(0))) {
        MAREA_BYTECODE_TRY(MAREA_G_End(generator, node, address, run, MAREA_G_TextIs(member, "back"), destination, outKind));
    } else if (MAREA_G_TextIs(member, "fill") && (GIVEN == MAREA_INTEGER_SIZE(1))) {
        MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_G_HoldValue(generator, MAREA_BytecodeGeneratorChildAt(arguments, MAREA_INTEGER_SIZE(0)), generator->aggregates[run].elementKind, generator->aggregates[run].elementAggregate, &value));
        MAREA_BYTECODE_TRY(MAREA_G_Fill(generator, node, address, run, value));
    } else if (MAREA_G_TextIs(member, "swap") && (GIVEN == MAREA_INTEGER_SIZE(1))) {
        MAREA_BYTECODE_TRY(MAREA_G_Swap(generator, node, address, run, MAREA_BytecodeGeneratorChildAt(arguments, MAREA_INTEGER_SIZE(0))));
    } else {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A member of an array is not lowered.");
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructRun(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 run)
{
    const MAREA_BytecodeGeneratorAggregate RUN = generator->aggregates[run];

    // INFO: zero of b clears the register itself, so a run of no byte is not cleared.
    if (RUN.size > MAREA_UINTEGER_64(0)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, destinationAddress, (MAREA_Uinteger32)RUN.size, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    }

    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);
    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger64 position = MAREA_UINTEGER_64(0);
    MAREA_Boolean positionKnown = MAREA_BOOLEAN_TRUE;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index < COUNT) && !generator->functionFailed; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *entry = MAREA_BytecodeGeneratorChildAt(node, index);
        const MAREA_Boolean INDEXED = (MAREA_IrNodeGetKind(entry) == MAREA_IR_KIND_INDEX_ENTRY);
        const MAREA_IrNode *key = (INDEXED ? MAREA_BytecodeGeneratorChildAt(entry, MAREA_INTEGER_SIZE(0)) : MAREA_NULLPTR);
        const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(entry, (INDEXED ? MAREA_INTEGER_SIZE(1) : MAREA_INTEGER_SIZE(0)));

        if (!INDEXED && ((MAREA_IrNodeGetKind(entry) != MAREA_IR_KIND_VALUE_ENTRY) || !positionKnown)) {
            return MAREA_BytecodeGeneratorFail(generator, entry, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A literal holds an entry that is not lowered.");
        }

        MAREA_Boolean literalKey = MAREA_BOOLEAN_FALSE;

        if (INDEXED && (MAREA_IrNodeGetKind(key) == MAREA_IR_KIND_INTEGER_LITERAL) && MAREA_BytecodeGeneratorParseInteger(MAREA_IrNodeGetText(key), MAREA_IrNodeGetTextLength(key), &position)) {
            literalKey = MAREA_BOOLEAN_TRUE;
            positionKnown = MAREA_BOOLEAN_TRUE;
        }

        MAREA_Uinteger32 address = MAREA_UINTEGER_32(0);

        if (!INDEXED || literalKey) {
            if (position >= RUN.count) {
                return MAREA_BytecodeGeneratorFail(generator, entry, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A literal has more elements than its array holds.");
            }

            MAREA_BYTECODE_TRY(MAREA_G_ElementField(generator, entry, destinationAddress, (position * RUN.stride), &address));

            position += MAREA_UINTEGER_64(1);
        } else {
            MAREA_Uinteger32 held = MAREA_UINTEGER_32(0);
            MAREA_Uinteger8 heldKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, key, MAREA_BYTECODE_GENERATOR_KIND_NONE, &held, &heldKind));
            MAREA_BYTECODE_TRY(MAREA_G_CheckIndex(generator, entry, held, RUN.count));
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, entry, MAREA_UINTEGER_32(1), &address));
            MAREA_BYTECODE_TRY(MAREA_G_ElementAt(generator, entry, destinationAddress, held, RUN.stride, address));

            positionKnown = MAREA_BOOLEAN_FALSE;
        }

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorStoreElement(generator, value, address, RUN.elementKind, RUN.elementAggregate));

        generator->nextRegister = MARK;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_Byte MAREA_CALL MAREA_G_HexDigit(MAREA_Character8 digit)
{
    return (MAREA_Byte)((digit <= '9') ? (digit - '0') : (digit - 'a' + 10));
}

// INFO: the bytes of an included file go to the read only data section, and are copied from there to the place of the value.
MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructIncluded(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 run)
{
    const MAREA_IntegerSize SIZE = (MAREA_IrNodeGetTextLength(node) / MAREA_INTEGER_SIZE(2));

    if (SIZE != generator->aggregates[run].size) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An included file has another size than its array.");
    }

    if (SIZE == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, SIZE, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Byte *bytes = memory;

    const MAREA_Character8 *text = MAREA_IrNodeGetText(node);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < SIZE; index += MAREA_INTEGER_SIZE(1)) {
        bytes[index] = (MAREA_Byte)((MAREA_G_HexDigit(text[(index * MAREA_INTEGER_SIZE(2))]) << 4) | MAREA_G_HexDigit(text[((index * MAREA_INTEGER_SIZE(2)) + MAREA_INTEGER_SIZE(1))]));
    }

    MAREA_Uinteger32 blob = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddData(generator->module, bytes, SIZE, &blob));

    if (blob > MAREA_UINTEGER_32(65535)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW, "A module needs more than 65535 constants.");
    }

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD_DATA_BLOCK, destinationAddress, blob, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
}

// INFO: the loop starts before the first element, so a continue jumps to the step; the key is a copy of the index.
MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerForEach(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node)
{
    const MAREA_IntegerSize CHILDREN = MAREA_IrNodeGetChildCount(node);

    if ((CHILDREN < MAREA_INTEGER_SIZE(2)) || (CHILDREN > MAREA_INTEGER_SIZE(3))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A for each has no collection or no body.");
    }

    const MAREA_IrNode *collection = MAREA_BytecodeGeneratorChildAt(node, (CHILDREN - MAREA_INTEGER_SIZE(2)));

    MAREA_Uinteger32 run = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorRunKind(generator, collection, MAREA_IrNodeGetResolvedType(collection), MAREA_IrNodeGetResolvedTypeLength(collection), &run));

    if (run == MAREA_BYTECODE_NONE) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNWALKABLE_COLLECTION, "Only an array or a vector is walked by a for each.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;
    const MAREA_IntegerSize SYMBOL_MARK = generator->symbolCount;
    const MAREA_BytecodeGeneratorAggregate RUN = generator->aggregates[run];

    MAREA_Uinteger32 base = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 got = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, collection, &base, &got));

    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 one = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 limit = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 test = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 key = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 element = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 value = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64_MAXIMUM, &index));
    MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, MAREA_UINTEGER_64(1), &one));
    // INFO: the count of a vector is read once, when the loop starts.
    if (RUN.isVector) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, base, MAREA_UINTEGER_64(8), &limit));
    } else {
        MAREA_BYTECODE_TRY(MAREA_G_Constant(generator, node, RUN.count, &limit));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &test));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &key));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &element));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &value));

    const MAREA_Uinteger32 START = (MAREA_Uinteger32)generator->codeCount;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ADD, index, one, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_NULLPTR));
    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LESS_UNSIGNED, test, index, limit, MAREA_NULLPTR));

    MAREA_Uinteger32 exitJump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, test, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &exitJump));

    const MAREA_Uinteger32 LOOP_MARK = generator->nextRegister;

    if (RUN.isVector) {
        MAREA_Uinteger32 data = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLoadWord(generator, node, base, MAREA_UINTEGER_64(0), &data));
        MAREA_BYTECODE_TRY(MAREA_G_ElementAt(generator, node, data, index, RUN.stride, element));
    } else {
        MAREA_BYTECODE_TRY(MAREA_G_ElementAt(generator, node, base, index, RUN.stride, element));
    }

    generator->nextRegister = LOOP_MARK;

    if (CHILDREN == MAREA_INTEGER_SIZE(3)) {
        const MAREA_IrNode *name = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, key, index, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(name), MAREA_IrNodeGetTextLength(name), key, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, MAREA_BYTECODE_NONE));
    }

    // INFO: the name of a structure or of a run is the address of the element. A scalar of a mutable loop is the
    // element itself, so a write reaches the collection; a plain loop copies it.
    if (RUN.elementKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), element, RUN.elementKind, RUN.elementAggregate));
    } else if ((MAREA_IrNodeGetFlags(node) & (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE) != MAREA_UINTEGER_32(0)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), element, RUN.elementKind, MAREA_BYTECODE_NONE));

        generator->symbols[(generator->symbolCount - MAREA_INTEGER_SIZE(1))].isPlace = MAREA_BOOLEAN_TRUE;
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, value, element, MAREA_G_Width(RUN.elementKind), MAREA_NULLPTR));
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorPushSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), value, RUN.elementKind, MAREA_BYTECODE_NONE));
    }

    return MAREA_BytecodeGeneratorLowerLoopBody(generator, node, MAREA_BytecodeGeneratorChildAt(node, (CHILDREN - MAREA_INTEGER_SIZE(1))), START, exitJump, SYMBOL_MARK, MARK);
}
