// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"
#include "marea/internal/bytecode/MAREABytecodeInternal.h"

#include <stdlib.h>
#include <string.h>

#if !defined(MAREA_BYTECODE_GENERATOR_LITERAL_LIMIT)
    #define MAREA_BYTECODE_GENERATOR_LITERAL_LIMIT MAREA_INTEGER_SIZE(128)
#endif

typedef struct MAREA_G_KindRow
{
    const MAREA_Character8 *name;
    MAREA_Uinteger8 kind;
    MAREA_Uinteger8 size;
    MAREA_Uinteger8 padding[6];
} MAREA_G_KindRow;

static const MAREA_G_KindRow MAREA_G_KIND_TABLE[] = {
    { .name = "sinteger8", .kind = MAREA_BYTECODE_NUMBER_KIND_SINTEGER_8, .size = MAREA_UINTEGER_8(1) },
    { .name = "sinteger16", .kind = MAREA_BYTECODE_NUMBER_KIND_SINTEGER_16, .size = MAREA_UINTEGER_8(2) },
    { .name = "sinteger32", .kind = MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32, .size = MAREA_UINTEGER_8(4) },
    { .name = "sinteger64", .kind = MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64, .size = MAREA_UINTEGER_8(8) },
    { .name = "uinteger8", .kind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8, .size = MAREA_UINTEGER_8(1) },
    { .name = "uinteger16", .kind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_16, .size = MAREA_UINTEGER_8(2) },
    { .name = "uinteger32", .kind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_32, .size = MAREA_UINTEGER_8(4) },
    { .name = "uinteger64", .kind = MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, .size = MAREA_UINTEGER_8(8) },
    { .name = "float32", .kind = MAREA_BYTECODE_NUMBER_KIND_FLOAT_32, .size = MAREA_UINTEGER_8(4) },
    { .name = "float64", .kind = MAREA_BYTECODE_NUMBER_KIND_FLOAT_64, .size = MAREA_UINTEGER_8(8) },
    { .name = "boolean", .kind = MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN, .size = MAREA_UINTEGER_8(1) },
};

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorGrow(MAREA_BytecodeGenerator *generator, MAREA_Null current, MAREA_IntegerSize *capacity, MAREA_IntegerSize count, MAREA_IntegerSize itemSize, MAREA_IntegerSize alignment, MAREA_Null *outItems)
{
    MAREA_Null items = current;

    const MAREA_BytecodeExcept except = MAREA_BytecodeGrow(generator->arena, &items, capacity, count, itemSize, alignment);

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    *outItems = items;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorFail(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_BytecodeDiagnosticCode code, const MAREA_Character8 *message)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    generator->functionFailed = MAREA_BOOLEAN_TRUE;
    generator->anyFailed = MAREA_BOOLEAN_TRUE;

    const MAREA_SourcePosition position = MAREA_IrNodeGetPosition(node);

    return MAREA_BytecodeRaise(generator->context, code, position.offset, position.line, position.column, message);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorEmit(MAREA_BytecodeGenerator *generator, MAREA_Uinteger16 opcode, MAREA_Uinteger32 a, MAREA_Uinteger32 b, MAREA_Uinteger32 c, MAREA_Uinteger32 *outIndex)
{
    if (outIndex != MAREA_NULLPTR) {
        *outIndex = (MAREA_Uinteger32)generator->codeCount;
    }

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->code, &generator->codeCapacity, generator->codeCount, sizeof(MAREA_BytecodeInstruction), MAREA_ALIGNOF(MAREA_BytecodeInstruction), &items));

    generator->code = items;

    MAREA_BytecodeInstruction *instruction = &generator->code[generator->codeCount];

    instruction->opcode = opcode;
    instruction->a = (MAREA_Uinteger16)a;
    instruction->b = (MAREA_Uinteger16)b;
    instruction->c = (MAREA_Uinteger16)c;

    generator->codeCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPatchJump(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 index, MAREA_Uinteger32 target)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_Sinteger64 distance = ((MAREA_Sinteger64)target - ((MAREA_Sinteger64)index + MAREA_SINTEGER_64(1)));

    if ((distance < MAREA_SINTEGER_64(-32768)) || (distance > MAREA_SINTEGER_64(32767))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW, "A jump is farther than 32767 instructions.");
    }

    generator->code[index].b = (MAREA_Uinteger16)((MAREA_Sinteger16)distance);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAllocate(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 count, MAREA_Uinteger32 *outFirst)
{
    *outFirst = MAREA_UINTEGER_32(0);

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (count > (MAREA_BYTECODE_GENERATOR_REGISTER_LIMIT - generator->nextRegister)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW, "A function needs more than 65535 registers.");
    }

    *outFirst = generator->nextRegister;

    generator->nextRegister += count;

    if (generator->nextRegister > generator->maxRegister) {
        generator->maxRegister = generator->nextRegister;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorEmitConstant(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger64 bits)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_Sinteger64 SIGNED = (MAREA_Sinteger64)bits;

    if ((SIGNED >= (MAREA_Sinteger64)MAREA_SINTEGER_32_MINIMUM) && (SIGNED <= (MAREA_Sinteger64)MAREA_SINTEGER_32_MAXIMUM)) {
        const MAREA_Uinteger32 IMMEDIATE = (MAREA_Uinteger32)((MAREA_Sinteger32)SIGNED);

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, destination, (IMMEDIATE & MAREA_UINTEGER_32(0xFFFF)), (IMMEDIATE >> MAREA_UINTEGER_32(16)), MAREA_NULLPTR);
    }

    MAREA_IntegerSize found = generator->dataCount;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->dataCount; index += MAREA_INTEGER_SIZE(1)) {
        if (generator->dataValues[index] == bits) {
            found = index;

            break;
        }
    }

    if (found == generator->dataCount) {
        if (generator->dataCount >= MAREA_INTEGER_SIZE(65535)) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW, "A module needs more than 65535 constants.");
        }

        MAREA_Null items = MAREA_NULLPTR;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->dataValues, &generator->dataCapacity, generator->dataCount, sizeof(MAREA_Uinteger64), MAREA_ALIGNOF(MAREA_Uinteger64), &items));

        generator->dataValues = items;

        MAREA_Byte bytes[8];

        for (MAREA_Uinteger32 position = MAREA_UINTEGER_32(0); position < MAREA_UINTEGER_32(8); position += MAREA_UINTEGER_32(1)) {
            bytes[position] = (MAREA_Byte)((bits >> (position * MAREA_UINTEGER_32(8))) & MAREA_UINTEGER_64(0xFF));
        }

        MAREA_Uinteger32 blob = MAREA_UINTEGER_32(0);

        if (MAREA_BytecodeModuleAddData(generator->module, bytes, MAREA_INTEGER_SIZE(8), &blob) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->dataBlobs, &generator->dataBlobCapacity, generator->dataCount, sizeof(MAREA_Uinteger32), MAREA_ALIGNOF(MAREA_Uinteger32), &items));

        generator->dataBlobs = items;
        generator->dataBlobs[generator->dataCount] = blob;
        generator->dataValues[generator->dataCount] = bits;

        generator->dataCount += MAREA_INTEGER_SIZE(1);
    }

    return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD_DATA, destination, generator->dataBlobs[found], MAREA_UINTEGER_32(0), MAREA_NULLPTR);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorPushSymbol(MAREA_BytecodeGenerator *generator, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 registerIndex, MAREA_Uinteger8 kind, MAREA_Uinteger32 aggregateIndex)
{
    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->symbols, &generator->symbolCapacity, generator->symbolCount, sizeof(MAREA_BytecodeGeneratorSymbol), MAREA_ALIGNOF(MAREA_BytecodeGeneratorSymbol), &items));

    generator->symbols = items;

    MAREA_BytecodeGeneratorSymbol *symbol = &generator->symbols[generator->symbolCount];

    symbol->name = name;
    symbol->length = length;
    symbol->registerIndex = registerIndex;
    symbol->kind = kind;
    symbol->aggregateType = aggregateIndex;
    symbol->isPlace = MAREA_BOOLEAN_FALSE;

    generator->symbolCount += MAREA_INTEGER_SIZE(1);

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

const MAREA_BytecodeGeneratorSymbol *MAREA_CALL MAREA_BytecodeGeneratorFindSymbol(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = generator->symbolCount; index > MAREA_INTEGER_SIZE(0); index -= MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeGeneratorSymbol *symbol = &generator->symbols[(index - MAREA_INTEGER_SIZE(1))];

        if ((symbol->length == length) && (memcmp(symbol->name, name, length) == 0)) {
            return symbol;
        }
    }

    return MAREA_NULLPTR;
}

const MAREA_IrNode *MAREA_CALL MAREA_BytecodeGeneratorChildAt(const MAREA_IrNode *node, MAREA_IntegerSize index)
{
    const MAREA_IrNode *child = MAREA_NULLPTR;

    if (MAREA_IrNodeGetChildAt(node, index, &child) != MAREA_IR_NODE_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

MAREA_Uinteger8 MAREA_CALL MAREA_BytecodeGeneratorKindFromText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (text == MAREA_NULLPTR) {
        return MAREA_BYTECODE_GENERATOR_KIND_NONE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(MAREA_G_KIND_TABLE); index += MAREA_INTEGER_SIZE(1)) {
        if ((strlen(MAREA_G_KIND_TABLE[index].name) == length) && (memcmp(MAREA_G_KIND_TABLE[index].name, text, length) == 0)) {
            return MAREA_G_KIND_TABLE[index].kind;
        }
    }

    return MAREA_BYTECODE_GENERATOR_KIND_NONE;
}

MAREA_Uinteger8 MAREA_CALL MAREA_BytecodeGeneratorKindOfNode(const MAREA_IrNode *node)
{
    return MAREA_BytecodeGeneratorKindFromText(MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node));
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTypeIndex(MAREA_BytecodeGenerator *generator, MAREA_Uinteger8 kind, MAREA_Uinteger32 *outIndex)
{
    if (generator->typeIndex[kind] != MAREA_BYTECODE_NONE) {
        *outIndex = generator->typeIndex[kind];

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_G_KindRow *row = MAREA_NULLPTR;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(MAREA_G_KIND_TABLE); index += MAREA_INTEGER_SIZE(1)) {
        if (MAREA_G_KIND_TABLE[index].kind == kind) {
            row = &MAREA_G_KIND_TABLE[index];
        }
    }

    if (row == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_Uinteger32 name = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, row->name, strlen(row->name), &name));

    const MAREA_BytecodeTypeInfo info = {
        .size = row->size,
        .count = MAREA_UINTEGER_64(0),
        .name = name,
        .element = MAREA_BYTECODE_NONE,
        .alignment = row->size,
        .memberCount = MAREA_UINTEGER_32(0),
        .kind = MAREA_BYTECODE_TYPE_KIND_PRIMITIVE,
    };

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddType(generator->module, &info, &generator->typeIndex[kind]));

    *outIndex = generator->typeIndex[kind];

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorParseInteger(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger64 *outValue)
{
    MAREA_Uinteger64 base = MAREA_UINTEGER_64(10);
    MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0);

    if ((length > MAREA_INTEGER_SIZE(2)) && (text[0] == '0') && (text[1] == 'x')) {
        base = MAREA_UINTEGER_64(16);
        position = MAREA_INTEGER_SIZE(2);
    } else if ((length > MAREA_INTEGER_SIZE(2)) && (text[0] == '0') && (text[1] == 'b')) {
        base = MAREA_UINTEGER_64(2);
        position = MAREA_INTEGER_SIZE(2);
    }

    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);
    MAREA_Boolean seen = MAREA_BOOLEAN_FALSE;

    for (; position < length; position += MAREA_INTEGER_SIZE(1)) {
        const MAREA_Character8 symbol = text[position];

        if (symbol == '_') {
            continue;
        }

        MAREA_Uinteger64 digit = MAREA_UINTEGER_64(0);

        if ((symbol >= '0') && (symbol <= '9')) {
            digit = (MAREA_Uinteger64)(symbol - '0');
        } else if ((symbol >= 'a') && (symbol <= 'f')) {
            digit = (MAREA_Uinteger64)((symbol - 'a') + 10);
        } else if ((symbol >= 'A') && (symbol <= 'F')) {
            digit = (MAREA_Uinteger64)((symbol - 'A') + 10);
        } else {
            return MAREA_BOOLEAN_FALSE;
        }

        if (digit >= base) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (value > ((MAREA_UINTEGER_64_MAXIMUM - digit) / base)) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
        seen = MAREA_BOOLEAN_TRUE;
    }

    if (!seen) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outValue = value;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorParseFloat(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Float64 *outValue)
{
    MAREA_Character8 buffer[MAREA_BYTECODE_GENERATOR_LITERAL_LIMIT];
    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0); position < length; position += MAREA_INTEGER_SIZE(1)) {
        if (text[position] == '_') {
            continue;
        }

        if (used >= (MAREA_BYTECODE_GENERATOR_LITERAL_LIMIT - MAREA_INTEGER_SIZE(1))) {
            return MAREA_BOOLEAN_FALSE;
        }

        buffer[used] = text[position];
        used += MAREA_INTEGER_SIZE(1);
    }

    buffer[used] = '\0';

    if (used == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Character8 *end = MAREA_NULLPTR;

    *outValue = strtod(buffer, &end);

    return (*end == '\0');
}

// INFO: a structure field of this unit's own structures, by name among the FIELD_DECLARATION children of
// aggregate->declaration, in declaration order (the same order MAREA_IrLayoutGetMemberOffset counts by).
static MAREA_Boolean MAREA_CALL MAREA_G_FindField(const MAREA_BytecodeGeneratorAggregate *aggregate, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_IntegerSize *outIndex, const MAREA_IrNode **outField)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(aggregate->declaration);
    MAREA_IntegerSize fieldIndex = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *field = MAREA_BytecodeGeneratorChildAt(aggregate->declaration, index);

        if (MAREA_IrNodeGetKind(field) != MAREA_IR_KIND_FIELD_DECLARATION) {
            continue;
        }

        if ((MAREA_IrNodeGetTextLength(field) == length) && (memcmp(MAREA_IrNodeGetText(field), name, length) == 0)) {
            *outIndex = fieldIndex;
            *outField = field;

            return MAREA_BOOLEAN_TRUE;
        }

        fieldIndex += MAREA_INTEGER_SIZE(1);
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_BuildAggregate(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *declaration, const MAREA_IrLayout *layout, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_Uinteger32 *outIndex)
{
    MAREA_Uinteger64 size = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 alignment = MAREA_UINTEGER_64(0);

    if ((MAREA_IrLayoutGetSize(layout, &size) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) || (MAREA_IrLayoutGetAlignment(layout, &alignment) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS)) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_Uinteger32 nameString = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, name, nameLength, &nameString));

    const MAREA_BytecodeTypeInfo INFO = {
        .size = size,
        .count = MAREA_UINTEGER_64(0),
        .name = nameString,
        .element = MAREA_BYTECODE_NONE,
        .alignment = (MAREA_Uinteger32)alignment,
        .memberCount = MAREA_UINTEGER_32(0),
        .kind = MAREA_BYTECODE_TYPE_KIND_STRUCTURE,
    };

    MAREA_Uinteger32 typeIndex = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddType(generator->module, &INFO, &typeIndex));

    // INFO: the cache entry is pushed before any field is resolved, so a structure that holds itself through a
    // chain of other structures finds this entry already made instead of recursing without end. Sema is expected
    // to reject such a type as an infinite size, so this is a safety net, never an exercised path.
    MAREA_Null items = MAREA_NULLPTR;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorGrow(generator, generator->aggregates, &generator->aggregateCapacity, generator->aggregateCount, sizeof(MAREA_BytecodeGeneratorAggregate), MAREA_ALIGNOF(MAREA_BytecodeGeneratorAggregate), &items));

    generator->aggregates = items;

    const MAREA_Uinteger32 SELF = (MAREA_Uinteger32)generator->aggregateCount;
    MAREA_BytecodeGeneratorAggregate *aggregate = &generator->aggregates[SELF];

    memset(aggregate, 0, sizeof(*aggregate));

    aggregate->declaration = declaration;
    aggregate->layout = layout;
    aggregate->name = name;
    aggregate->nameLength = nameLength;
    aggregate->typeIndex = typeIndex;
    aggregate->size = size;
    aggregate->alignment = alignment;

    generator->aggregateCount += MAREA_INTEGER_SIZE(1);

    const MAREA_IntegerSize MEMBER_COUNT = MAREA_IrLayoutGetMemberCount(layout);
    const MAREA_IntegerSize FIELD_COUNT = MAREA_IrNodeGetChildCount(declaration);
    MAREA_IntegerSize member = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < FIELD_COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *field = MAREA_BytecodeGeneratorChildAt(declaration, index);

        if (MAREA_IrNodeGetKind(field) != MAREA_IR_KIND_FIELD_DECLARATION) {
            continue;
        }

        if (member >= MEMBER_COUNT) {
            return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        MAREA_Uinteger8 fieldKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
        MAREA_Uinteger32 fieldAggregate = MAREA_BYTECODE_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, field, &fieldKind, &fieldAggregate));

        MAREA_Uinteger32 fieldType = MAREA_UINTEGER_32(0);

        if (fieldKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
            fieldType = generator->aggregates[fieldAggregate].typeIndex;
        } else if (fieldKind < MAREA_BYTECODE_GENERATOR_KIND_VOID) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorTypeIndex(generator, fieldKind, &fieldType));
        } else {
            return MAREA_BytecodeGeneratorFail(generator, field, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A field has a type that is not lowered yet.");
        }

        MAREA_Uinteger32 fieldName = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddString(generator->module, MAREA_IrNodeGetText(field), MAREA_IrNodeGetTextLength(field), &fieldName));

        MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);

        if (MAREA_IrLayoutGetMemberOffset(layout, member, &offset) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
            return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        const MAREA_BytecodeMemberInfo MEMBER_INFO = {
            .value = offset,
            .name = fieldName,
            .type = fieldType,
        };

        MAREA_BYTECODE_TRY(MAREA_BytecodeModuleAddMember(generator->module, typeIndex, &MEMBER_INFO));

        member += MAREA_INTEGER_SIZE(1);
    }

    *outIndex = SELF;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAggregateKind(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex)
{
    *outAggregateIndex = MAREA_BYTECODE_NONE;

    const MAREA_Uinteger8 SCALAR = MAREA_BytecodeGeneratorKindOfNode(node);

    if (SCALAR != MAREA_BYTECODE_GENERATOR_KIND_NONE) {
        *outKind = SCALAR;

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    return MAREA_BytecodeGeneratorTextKind(generator, node, MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node), outKind, outAggregateIndex);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorTextKind(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger8 *outKind, MAREA_Uinteger32 *outAggregateIndex)
{
    *outAggregateIndex = MAREA_BYTECODE_NONE;
    *outKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    if ((text == MAREA_NULLPTR) || (length == MAREA_INTEGER_SIZE(0))) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->aggregateCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeGeneratorAggregate *aggregate = &generator->aggregates[index];

        if ((aggregate->nameLength == length) && (memcmp(aggregate->name, text, length) == 0)) {
            *outKind = MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE;
            *outAggregateIndex = (MAREA_Uinteger32)index;

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    MAREA_Uinteger32 run = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorRunKind(generator, node, text, length, &run));

    if (run != MAREA_BYTECODE_NONE) {
        *outKind = MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE;
        *outAggregateIndex = run;

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_IrLayout *layout = MAREA_NULLPTR;

    if (MAREA_IrModuleFindLayout(generator->root, text, length, &layout) != MAREA_IR_LAYOUT_EXCEPT_SUCCESS) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if ((MAREA_IrLayoutGetKind(layout) != MAREA_IR_LAYOUT_KIND_STRUCTURE) || (MAREA_IrLayoutGetVerdict(layout) != MAREA_SEMA_MEASURE_VERDICT_MEASURED)) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize MODULE_COUNT = MAREA_IrNodeGetChildCount(generator->root);
    const MAREA_IrNode *declaration = MAREA_NULLPTR;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MODULE_COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *child = MAREA_BytecodeGeneratorChildAt(generator->root, index);

        if ((MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_STRUCTURE_DECLARATION) && (MAREA_IrNodeGetTextLength(child) == length) && (memcmp(MAREA_IrNodeGetText(child), text, length) == 0)) {
            declaration = child;

            break;
        }
    }

    if (declaration == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    MAREA_Uinteger32 index = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_G_BuildAggregate(generator, declaration, layout, text, length, &index));

    *outKind = MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE;
    *outAggregateIndex = index;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorAggregateField(const MAREA_BytecodeGenerator *generator, MAREA_Uinteger32 aggregateIndex, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger64 *outOffset, const MAREA_IrNode **outField)
{
    if ((aggregateIndex >= generator->aggregateCount) || generator->aggregates[aggregateIndex].isRun || generator->aggregates[aggregateIndex].isVector || generator->aggregates[aggregateIndex].isTuple) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_BytecodeGeneratorAggregate *aggregate = &generator->aggregates[aggregateIndex];
    MAREA_IntegerSize fieldIndex = MAREA_INTEGER_SIZE(0);

    if (!MAREA_G_FindField(aggregate, name, length, &fieldIndex, outField)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_IrLayoutGetMemberOffset(aggregate->layout, fieldIndex, outOffset) == MAREA_IR_LAYOUT_EXCEPT_SUCCESS);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAllocateFrame(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment, MAREA_Uinteger32 *outOffset)
{
    *outOffset = MAREA_UINTEGER_32(0);

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger64 MASK = ((alignment > MAREA_UINTEGER_64(0)) ? (alignment - MAREA_UINTEGER_64(1)) : MAREA_UINTEGER_64(0));
    const MAREA_Uinteger64 ALIGNED = (((MAREA_Uinteger64)generator->nextFrameOffset + MASK) & ~MASK);

    if ((ALIGNED + size) > (MAREA_Uinteger64)MAREA_BYTECODE_GENERATOR_FRAME_LIMIT) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW, "A function needs more frame memory than this slice allows.");
    }

    *outOffset = (MAREA_Uinteger32)ALIGNED;

    generator->nextFrameOffset = (MAREA_Uinteger32)(ALIGNED + size);

    if (generator->nextFrameOffset > generator->maxFrameOffset) {
        generator->maxFrameOffset = generator->nextFrameOffset;
    }

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeGeneratorOperatorOpcode(MAREA_TokenKind token, MAREA_Uinteger16 *outOpcode, MAREA_Boolean *outThreeRegisters)
{
    *outThreeRegisters = MAREA_BOOLEAN_FALSE;

    switch (token) {
    case MAREA_TOKEN_KIND_PLUS:
    case MAREA_TOKEN_KIND_PLUS_PERCENT:
    case MAREA_TOKEN_KIND_PLUS_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_ADD;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_MINUS:
    case MAREA_TOKEN_KIND_MINUS_PERCENT:
    case MAREA_TOKEN_KIND_MINUS_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_SUB;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_STAR:
    case MAREA_TOKEN_KIND_STAR_PERCENT:
    case MAREA_TOKEN_KIND_STAR_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_MUL;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_SLASH:
    case MAREA_TOKEN_KIND_SLASH_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_DIV;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_PERCENT:
    case MAREA_TOKEN_KIND_PERCENT_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_REM;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_STAR_STAR:
    case MAREA_TOKEN_KIND_STAR_STAR_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_POW;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_PLUS_PIPE:
        *outOpcode = MAREA_BYTECODE_OPCODE_SATURATING_ADD;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_MINUS_PIPE:
        *outOpcode = MAREA_BYTECODE_OPCODE_SATURATING_SUB;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_STAR_PIPE:
        *outOpcode = MAREA_BYTECODE_OPCODE_SATURATING_MUL;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_SHIFT_LEFT:
    case MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_SHIFT_LEFT;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_SHIFT_RIGHT:
    case MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_SHIFT_RIGHT;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_AMPERSAND:
    case MAREA_TOKEN_KIND_AMPERSAND_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_BIT_AND;
        *outThreeRegisters = MAREA_BOOLEAN_TRUE;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_PIPE:
    case MAREA_TOKEN_KIND_PIPE_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_BIT_OR;
        *outThreeRegisters = MAREA_BOOLEAN_TRUE;
        return MAREA_BOOLEAN_TRUE;
    case MAREA_TOKEN_KIND_CARET:
    case MAREA_TOKEN_KIND_CARET_EQUAL:
        *outOpcode = MAREA_BYTECODE_OPCODE_BIT_XOR;
        *outThreeRegisters = MAREA_BOOLEAN_TRUE;
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}
