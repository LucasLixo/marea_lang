// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeInternal.h"
#include "marea/bytecode/MAREABytecodeFormat.h"

#include <string.h>

#define MAREA_G_SECTION_COUNT       MAREA_INTEGER_SIZE(5)
#define MAREA_G_HEADER_SIZE         MAREA_INTEGER_SIZE(16)
#define MAREA_G_SECTION_ENTRY_SIZE  MAREA_INTEGER_SIZE(20)
#define MAREA_G_TABLE_END           (MAREA_G_HEADER_SIZE + (MAREA_G_SECTION_COUNT * MAREA_G_SECTION_ENTRY_SIZE))
#define MAREA_G_TYPE_FIXED_SIZE     MAREA_INTEGER_SIZE(33)
#define MAREA_G_MEMBER_SIZE         MAREA_INTEGER_SIZE(16)
#define MAREA_G_FUNCTION_FIXED_SIZE MAREA_INTEGER_SIZE(36)
#define MAREA_G_PARAMETER_SIZE      MAREA_INTEGER_SIZE(8)
#define MAREA_G_INSTRUCTION_SIZE    MAREA_INTEGER_SIZE(8)
#define MAREA_G_BLOB_FIXED_SIZE     MAREA_INTEGER_SIZE(8)

// INFO: the section ids follow the design table of the bytecode stage: strings, types, functions, code, data.
static const MAREA_Uinteger32 MAREA_G_SECTION_IDS[5] = { 1, 2, 4, 5, 6 };

typedef enum MAREA_G_SectionSlot
{
    MAREA_G_SECTION_SLOT_STRINGS = 0,
    MAREA_G_SECTION_SLOT_TYPES = 1,
    MAREA_G_SECTION_SLOT_FUNCTIONS = 2,
    MAREA_G_SECTION_SLOT_CODE = 3,
    MAREA_G_SECTION_SLOT_DATA = 4,
} MAREA_G_SectionSlot;

typedef struct MAREA_G_Writer
{
    MAREA_Byte *bytes;
    MAREA_IntegerSize position;
} MAREA_G_Writer;

static void MAREA_CALL MAREA_G_Put8(MAREA_G_Writer *writer, MAREA_Uinteger8 value)
{
    writer->bytes[writer->position] = value;
    writer->position += MAREA_INTEGER_SIZE(1);
}

static void MAREA_CALL MAREA_G_Put16(MAREA_G_Writer *writer, MAREA_Uinteger16 value)
{
    for (MAREA_Uinteger32 shift = MAREA_UINTEGER_32(0); shift < MAREA_UINTEGER_32(16); shift += MAREA_UINTEGER_32(8)) {
        MAREA_G_Put8(writer, (MAREA_Uinteger8)((value >> shift) & MAREA_UINTEGER_16(0xFF)));
    }
}

static void MAREA_CALL MAREA_G_Put32(MAREA_G_Writer *writer, MAREA_Uinteger32 value)
{
    for (MAREA_Uinteger32 shift = MAREA_UINTEGER_32(0); shift < MAREA_UINTEGER_32(32); shift += MAREA_UINTEGER_32(8)) {
        MAREA_G_Put8(writer, (MAREA_Uinteger8)((value >> shift) & MAREA_UINTEGER_32(0xFF)));
    }
}

static void MAREA_CALL MAREA_G_Put64(MAREA_G_Writer *writer, MAREA_Uinteger64 value)
{
    for (MAREA_Uinteger32 shift = MAREA_UINTEGER_32(0); shift < MAREA_UINTEGER_32(64); shift += MAREA_UINTEGER_32(8)) {
        MAREA_G_Put8(writer, (MAREA_Uinteger8)((value >> shift) & MAREA_UINTEGER_64(0xFF)));
    }
}

static void MAREA_CALL MAREA_G_PutBytes(MAREA_G_Writer *writer, const MAREA_Byte *bytes, MAREA_IntegerSize size)
{
    if (size > MAREA_INTEGER_SIZE(0)) {
        memcpy(&writer->bytes[writer->position], bytes, size);
        writer->position += size;
    }
}

// Adds one section size, and tells when the sum no longer fits in 64 bits
static MAREA_Boolean MAREA_CALL MAREA_G_AddSize(MAREA_IntegerSize *total, MAREA_IntegerSize amount)
{
    if (amount > (MAREA_INTEGER_SIZE_MAXIMUM - *total)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *total += amount;

    return MAREA_BOOLEAN_TRUE;
}

// The section sizes in bytes, and false when the module is too large for the format
static MAREA_Boolean MAREA_CALL MAREA_G_MeasureSections(const MAREA_BytecodeModule *module, MAREA_IntegerSize sizes[5])
{
    MAREA_IntegerSize instructions = MAREA_INTEGER_SIZE(0);

    sizes[MAREA_G_SECTION_SLOT_STRINGS] = MAREA_INTEGER_SIZE(4);
    sizes[MAREA_G_SECTION_SLOT_TYPES] = MAREA_INTEGER_SIZE(4);
    sizes[MAREA_G_SECTION_SLOT_FUNCTIONS] = MAREA_INTEGER_SIZE(4);
    sizes[MAREA_G_SECTION_SLOT_CODE] = MAREA_INTEGER_SIZE(0);
    sizes[MAREA_G_SECTION_SLOT_DATA] = MAREA_INTEGER_SIZE(4);

    if ((module->stringCount > MAREA_UINTEGER_32_MAXIMUM) || (module->typeCount > MAREA_UINTEGER_32_MAXIMUM) || (module->functionCount > MAREA_UINTEGER_32_MAXIMUM) || (module->blobCount > MAREA_UINTEGER_32_MAXIMUM)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->stringCount; index += MAREA_INTEGER_SIZE(1)) {
        if ((module->strings[index].length > MAREA_UINTEGER_32_MAXIMUM) || !MAREA_G_AddSize(&sizes[MAREA_G_SECTION_SLOT_STRINGS], (MAREA_INTEGER_SIZE(4) + module->strings[index].length))) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->typeCount; index += MAREA_INTEGER_SIZE(1)) {
        if (!MAREA_G_AddSize(&sizes[MAREA_G_SECTION_SLOT_TYPES], (MAREA_G_TYPE_FIXED_SIZE + (module->types[index].info.memberCount * MAREA_G_MEMBER_SIZE)))) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeFunction *function = &module->functions[index];

        if (!MAREA_G_AddSize(&sizes[MAREA_G_SECTION_SLOT_FUNCTIONS], (MAREA_G_FUNCTION_FIXED_SIZE + (function->info.parameterCount * MAREA_G_PARAMETER_SIZE)))) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (!MAREA_G_AddSize(&instructions, function->info.instructionCount)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    if ((instructions > MAREA_UINTEGER_32_MAXIMUM) || (instructions > (MAREA_INTEGER_SIZE_MAXIMUM / MAREA_G_INSTRUCTION_SIZE))) {
        return MAREA_BOOLEAN_FALSE;
    }

    sizes[MAREA_G_SECTION_SLOT_CODE] = (instructions * MAREA_G_INSTRUCTION_SIZE);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->blobCount; index += MAREA_INTEGER_SIZE(1)) {
        if (!MAREA_G_AddSize(&sizes[MAREA_G_SECTION_SLOT_DATA], (MAREA_G_BLOB_FIXED_SIZE + module->blobs[index].size))) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static void MAREA_CALL MAREA_G_WriteStrings(MAREA_G_Writer *writer, const MAREA_BytecodeModule *module)
{
    MAREA_G_Put32(writer, (MAREA_Uinteger32)module->stringCount);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->stringCount; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_G_Put32(writer, (MAREA_Uinteger32)module->strings[index].length);
        MAREA_G_PutBytes(writer, (const MAREA_Byte *)module->strings[index].text, module->strings[index].length);
    }
}

static void MAREA_CALL MAREA_G_WriteTypes(MAREA_G_Writer *writer, const MAREA_BytecodeModule *module)
{
    MAREA_G_Put32(writer, (MAREA_Uinteger32)module->typeCount);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->typeCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeType *type = &module->types[index];

        MAREA_G_Put8(writer, (MAREA_Uinteger8)type->info.kind);
        MAREA_G_Put32(writer, type->info.name);
        MAREA_G_Put32(writer, type->info.alignment);
        MAREA_G_Put32(writer, type->info.element);
        MAREA_G_Put64(writer, type->info.size);
        MAREA_G_Put64(writer, type->info.count);
        MAREA_G_Put32(writer, type->info.memberCount);

        for (MAREA_Uinteger32 member = MAREA_UINTEGER_32(0); member < type->info.memberCount; member += MAREA_UINTEGER_32(1)) {
            MAREA_G_Put32(writer, type->members[member].name);
            MAREA_G_Put32(writer, type->members[member].type);
            MAREA_G_Put64(writer, type->members[member].value);
        }
    }
}

static void MAREA_CALL MAREA_G_WriteFunctions(MAREA_G_Writer *writer, const MAREA_BytecodeModule *module)
{
    MAREA_Uinteger32 running = MAREA_UINTEGER_32(0);

    MAREA_G_Put32(writer, (MAREA_Uinteger32)module->functionCount);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeFunction *function = &module->functions[index];

        MAREA_G_Put32(writer, function->info.name);
        MAREA_G_Put32(writer, function->info.result);
        MAREA_G_Put32(writer, function->info.registerCount);
        MAREA_G_Put32(writer, function->info.flags);
        MAREA_G_Put64(writer, function->info.frameArea);
        MAREA_G_Put32(writer, function->info.parameterCount);

        for (MAREA_Uinteger32 parameter = MAREA_UINTEGER_32(0); parameter < function->info.parameterCount; parameter += MAREA_UINTEGER_32(1)) {
            MAREA_G_Put32(writer, function->parameters[parameter].type);
            MAREA_G_Put32(writer, function->parameters[parameter].flags);
        }

        MAREA_G_Put32(writer, running);
        MAREA_G_Put32(writer, function->info.instructionCount);

        running += function->info.instructionCount;
    }
}

static void MAREA_CALL MAREA_G_WriteCode(MAREA_G_Writer *writer, const MAREA_BytecodeModule *module)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeFunction *function = &module->functions[index];

        for (MAREA_Uinteger32 instruction = MAREA_UINTEGER_32(0); instruction < function->info.instructionCount; instruction += MAREA_UINTEGER_32(1)) {
            MAREA_G_Put16(writer, function->instructions[instruction].opcode);
            MAREA_G_Put16(writer, function->instructions[instruction].a);
            MAREA_G_Put16(writer, function->instructions[instruction].b);
            MAREA_G_Put16(writer, function->instructions[instruction].c);
        }
    }
}

static void MAREA_CALL MAREA_G_WriteData(MAREA_G_Writer *writer, const MAREA_BytecodeModule *module)
{
    MAREA_G_Put32(writer, (MAREA_Uinteger32)module->blobCount);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->blobCount; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_G_Put64(writer, (MAREA_Uinteger64)module->blobs[index].size);
        MAREA_G_PutBytes(writer, module->blobs[index].bytes, module->blobs[index].size);
    }
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeSerialize(const MAREA_BytecodeModule *module, const MAREA_Byte **outBytes, MAREA_IntegerSize *outSize)
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

    MAREA_IntegerSize sizes[5];

    if (!MAREA_G_MeasureSections(module, sizes)) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IntegerSize total =
MAREA_G_TABLE_END
;

for (MAREA_IntegerSize slot = MAREA_INTEGER_SIZE(0); slot < MAREA_G_SECTION_COUNT; slot += MAREA_INTEGER_SIZE(1)) {
    if (!MAREA_G_AddSize(&total, sizes[slot])) {
        return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }
}

MAREA_Null memory = MAREA_NULLPTR;

if (MAREA_ArenaAllocate(module->context->arena, total, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
    return MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;
}

MAREA_G_Writer writer = { .bytes = memory, .position = MAREA_INTEGER_SIZE(0) };

MAREA_G_PutBytes(&writer, (const MAREA_Byte *)"MBC", MAREA_INTEGER_SIZE(3));
MAREA_G_Put8(&writer, MAREA_UINTEGER_8(0));
MAREA_G_Put16(&writer, MAREA_BYTECODE_FORMAT_VERSION_MAJOR);
MAREA_G_Put16(&writer, MAREA_BYTECODE_FORMAT_VERSION_MINOR);
MAREA_G_Put32(&writer, MAREA_UINTEGER_32(0));
MAREA_G_Put32(&writer, (MAREA_Uinteger32)MAREA_G_SECTION_COUNT);

MAREA_IntegerSize offset =
MAREA_G_TABLE_END
;

for (MAREA_IntegerSize slot = MAREA_INTEGER_SIZE(0); slot < MAREA_G_SECTION_COUNT; slot += MAREA_INTEGER_SIZE(1)) {
    MAREA_G_Put32(&writer, MAREA_G_SECTION_IDS[slot]);
    MAREA_G_Put64(&writer, (MAREA_Uinteger64)offset);
    MAREA_G_Put64(&writer, (MAREA_Uinteger64)sizes[slot]);

    offset += sizes[slot];
}

MAREA_G_WriteStrings(&writer, module);
MAREA_G_WriteTypes(&writer, module);
MAREA_G_WriteFunctions(&writer, module);
MAREA_G_WriteCode(&writer, module);
MAREA_G_WriteData(&writer, module);

*outBytes = writer.bytes;
*outSize = writer.position;

return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

typedef struct MAREA_G_Reader
{
    MAREA_BytecodeModule *module;
    const MAREA_Byte *bytes;
    MAREA_IntegerSize position;
    MAREA_IntegerSize end;
    MAREA_BytecodeExcept except;
    MAREA_Boolean ok;
} MAREA_G_Reader;

static void MAREA_CALL MAREA_G_Fail(MAREA_G_Reader *reader, MAREA_BytecodeDiagnosticCode code, MAREA_IntegerSize offset, const MAREA_Character8 *message)
{
    if (!reader->ok) {
        return;
    }

    reader->ok = MAREA_BOOLEAN_FALSE;
    reader->except = MAREA_BytecodeRaise(reader->module->context, code, (MAREA_Uinteger32)offset, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), message);
}

static void MAREA_CALL MAREA_G_Check(MAREA_G_Reader *reader, MAREA_BytecodeExcept except)
{
    if ((except != MAREA_BYTECODE_EXCEPT_SUCCESS) && reader->ok) {
        reader->ok = MAREA_BOOLEAN_FALSE;
        reader->except = except;
    }
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_Remaining(const MAREA_G_Reader *reader)
{
    return (reader->end - reader->position);
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_Get(MAREA_G_Reader *reader, MAREA_IntegerSize width)
{
    if (!reader->ok) {
        return MAREA_UINTEGER_64(0);
    }

    if (MAREA_G_Remaining(reader) < width) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, reader->position, "A section ends before its content does.");

        return MAREA_UINTEGER_64(0);
    }

    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < width; index += MAREA_INTEGER_SIZE(1)) {
        value |= (((MAREA_Uinteger64)reader->bytes[(reader->position + index)]) << (MAREA_UINTEGER_32(8) * (MAREA_Uinteger32)index));
    }

    reader->position += width;

    return value;
}

// A count of records that each need at least minimum bytes cannot be larger than what is left in the section
static MAREA_Uinteger32 MAREA_CALL MAREA_G_GetCount(MAREA_G_Reader *reader, MAREA_IntegerSize minimum)
{
    const MAREA_IntegerSize offset = reader->position;

    const MAREA_Uinteger32 count = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));

    if (reader->ok && (count > (MAREA_G_Remaining(reader) / minimum))) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, offset, "A count is larger than the bytes that are left in the section.");

        return MAREA_UINTEGER_32(0);
    }

    return count;
}

static void MAREA_CALL MAREA_G_ReadStrings(MAREA_G_Reader *reader)
{
    const MAREA_Uinteger32 count = MAREA_G_GetCount(reader, MAREA_INTEGER_SIZE(4));

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); (index < count) && reader->ok; index += MAREA_UINTEGER_32(1)) {
        const MAREA_Uinteger32 length = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));

        if (reader->ok && (length > MAREA_G_Remaining(reader))) {
            MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, reader->position, "A string is longer than the bytes that are left in the section.");
        }

        if (reader->ok) {
            MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

            MAREA_G_Check(reader, MAREA_BytecodeModuleAppendString(reader->module, (const MAREA_Character8 *)&reader->bytes[reader->position], length, &added));

            reader->position += length;
        }
    }
}

static void MAREA_CALL MAREA_G_ReadTypes(MAREA_G_Reader *reader)
{
    const MAREA_Uinteger32 count = MAREA_G_GetCount(reader, MAREA_G_TYPE_FIXED_SIZE);

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); (index < count) && reader->ok; index += MAREA_UINTEGER_32(1)) {
        MAREA_BytecodeTypeInfo info = { 0 };

        info.kind = (MAREA_BytecodeTypeKind)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(1));
        info.name = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.alignment = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.element = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.size = MAREA_G_Get(reader, MAREA_INTEGER_SIZE(8));
        info.count = MAREA_G_Get(reader, MAREA_INTEGER_SIZE(8));

        const MAREA_Uinteger32 memberCount = MAREA_G_GetCount(reader, MAREA_G_MEMBER_SIZE);

        if (!reader->ok) {
            return;
        }

        MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

        MAREA_G_Check(reader, MAREA_BytecodeModuleAddType(reader->module, &info, &added));

        for (MAREA_Uinteger32 member = MAREA_UINTEGER_32(0); (member < memberCount) && reader->ok; member += MAREA_UINTEGER_32(1)) {
            MAREA_BytecodeMemberInfo value = { 0 };

            value.name = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
            value.type = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
            value.value = MAREA_G_Get(reader, MAREA_INTEGER_SIZE(8));

            if (reader->ok) {
                MAREA_G_Check(reader, MAREA_BytecodeModuleAddMember(reader->module, added, &value));
            }
        }
    }
}

static void MAREA_CALL MAREA_G_ReadFunctions(MAREA_G_Reader *reader, const MAREA_Byte *code, MAREA_IntegerSize instructionTotal, MAREA_IntegerSize codeBase)
{
    MAREA_IntegerSize running = MAREA_INTEGER_SIZE(0);

    const MAREA_Uinteger32 count = MAREA_G_GetCount(reader, MAREA_G_FUNCTION_FIXED_SIZE);

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); (index < count) && reader->ok; index += MAREA_UINTEGER_32(1)) {
        MAREA_BytecodeFunctionInfo info = { 0 };

        info.name = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.result = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.registerCount = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.flags = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        info.frameArea = MAREA_G_Get(reader, MAREA_INTEGER_SIZE(8));

        const MAREA_Uinteger32 parameterCount = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));

        if (reader->ok && (parameterCount > (MAREA_G_Remaining(reader) / MAREA_G_PARAMETER_SIZE))) {
            MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, reader->position, "A parameter count is larger than the bytes that are left in the section.");
        }

        if (!reader->ok) {
            return;
        }

        MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

        MAREA_G_Check(reader, MAREA_BytecodeModuleAddFunction(reader->module, &info, &added));

        for (MAREA_Uinteger32 parameter = MAREA_UINTEGER_32(0); (parameter < parameterCount) && reader->ok; parameter += MAREA_UINTEGER_32(1)) {
            MAREA_BytecodeParameterInfo value = { 0 };

            value.type = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
            value.flags = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));

            if (reader->ok) {
                MAREA_G_Check(reader, MAREA_BytecodeModuleAddParameter(reader->module, added, &value));
            }
        }

        const MAREA_IntegerSize offsetPosition = reader->position;

        const MAREA_Uinteger32 codeOffset = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));
        const MAREA_Uinteger32 codeLength = (MAREA_Uinteger32)MAREA_G_Get(reader, MAREA_INTEGER_SIZE(4));

        if (reader->ok && ((codeOffset != running) || (codeLength > (instructionTotal - running)))) {
            MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, offsetPosition, "The code range of a function does not follow the one before it.");
        }

        for (MAREA_Uinteger32 instruction = MAREA_UINTEGER_32(0); (instruction < codeLength) && reader->ok; instruction += MAREA_UINTEGER_32(1)) {
            const MAREA_Byte *at = &code[((running + instruction) * MAREA_G_INSTRUCTION_SIZE)];

            MAREA_BytecodeInstruction value = {
                .opcode = (MAREA_Uinteger16)(at[0] | (at[1] << 8)),
                .a = (MAREA_Uinteger16)(at[2] | (at[3] << 8)),
                .b = (MAREA_Uinteger16)(at[4] | (at[5] << 8)),
                .c = (MAREA_Uinteger16)(at[6] | (at[7] << 8)),
            };

            MAREA_G_Check(reader, MAREA_BytecodeModuleAddInstruction(reader->module, added, &value));
        }

        running += codeLength;
    }

    if (reader->ok && (running != instructionTotal)) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, codeBase, "The code section holds instructions that no function owns.");
    }
}

static void MAREA_CALL MAREA_G_ReadData(MAREA_G_Reader *reader)
{
    const MAREA_Uinteger32 count = MAREA_G_GetCount(reader, MAREA_G_BLOB_FIXED_SIZE);

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); (index < count) && reader->ok; index += MAREA_UINTEGER_32(1)) {
        const MAREA_Uinteger64 size = MAREA_G_Get(reader, MAREA_INTEGER_SIZE(8));

        if (reader->ok && (size > MAREA_G_Remaining(reader))) {
            MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, reader->position, "A blob is longer than the bytes that are left in the section.");
        }

        if (reader->ok) {
            MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

            MAREA_G_Check(reader, MAREA_BytecodeModuleAddData(reader->module, &reader->bytes[reader->position], (MAREA_IntegerSize)size, &added));

            reader->position += (MAREA_IntegerSize)size;
        }
    }
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_PeekLittle(const MAREA_Byte *bytes, MAREA_IntegerSize width)
{
    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < width; index += MAREA_INTEGER_SIZE(1)) {
        value |= (((MAREA_Uinteger64)bytes[index]) << (MAREA_UINTEGER_32(8) * (MAREA_Uinteger32)index));
    }

    return value;
}

// Checks the header and the section table, and gives the offset and the size of each section
static MAREA_Boolean MAREA_CALL MAREA_G_ReadTable(MAREA_G_Reader *reader, MAREA_IntegerSize size, MAREA_IntegerSize offsets[5], MAREA_IntegerSize sizes[5])
{
    const MAREA_Byte *bytes = reader->bytes;

    if (size < MAREA_G_HEADER_SIZE) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED, size, "The input is shorter than the header.");

        return MAREA_BOOLEAN_FALSE;
    }

    if (memcmp(bytes, "MBC", MAREA_INTEGER_SIZE(3)) != 0 || bytes[3] != 0) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_MAGIC, MAREA_INTEGER_SIZE(0), "The input does not start with the magic bytes of the format.");

        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_G_PeekLittle(&bytes[4], MAREA_INTEGER_SIZE(2)) != MAREA_BYTECODE_FORMAT_VERSION_MAJOR) || (MAREA_G_PeekLittle(&bytes[6], MAREA_INTEGER_SIZE(2)) > MAREA_BYTECODE_FORMAT_VERSION_MINOR)) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION, MAREA_INTEGER_SIZE(4), "The version of the format is not supported.");

        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_PeekLittle(&bytes[8], MAREA_INTEGER_SIZE(4)) != MAREA_UINTEGER_32(0)) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_FLAGS, MAREA_INTEGER_SIZE(8), "A flag bit that this version does not define is set.");

        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_PeekLittle(&bytes[12], MAREA_INTEGER_SIZE(4)) != MAREA_G_SECTION_COUNT) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE, MAREA_INTEGER_SIZE(12), "The section count is not the one of this version.");

        return MAREA_BOOLEAN_FALSE;
    }

    if (size < MAREA_G_TABLE_END) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED, size, "The input is shorter than the section table.");

        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize expected =
MAREA_G_TABLE_END
;

for (MAREA_IntegerSize slot = MAREA_INTEGER_SIZE(0); slot < MAREA_G_SECTION_COUNT; slot += MAREA_INTEGER_SIZE(1)) {
    const MAREA_IntegerSize entry = (MAREA_G_HEADER_SIZE + (slot * MAREA_G_SECTION_ENTRY_SIZE));

    const MAREA_Uinteger64 id = MAREA_G_PeekLittle(&bytes[entry], MAREA_INTEGER_SIZE(4));
    const MAREA_Uinteger64 offset = MAREA_G_PeekLittle(&bytes[(entry + MAREA_INTEGER_SIZE(4))], MAREA_INTEGER_SIZE(8));
    const MAREA_Uinteger64 length = MAREA_G_PeekLittle(&bytes[(entry + MAREA_INTEGER_SIZE(12))], MAREA_INTEGER_SIZE(8));

    if ((id != MAREA_G_SECTION_IDS[slot]) || (offset != expected) || (length > (MAREA_UINTEGER_64_MAXIMUM - offset))) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE, entry, "A section entry has the wrong id, offset or size.");

        return MAREA_BOOLEAN_FALSE;
    }

    if ((offset + length) > size) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED, size, "A section ends after the end of the input.");

        return MAREA_BOOLEAN_FALSE;
    }

    offsets[slot] = (MAREA_IntegerSize)offset;
    sizes[slot] = (MAREA_IntegerSize)length;

    expected = (MAREA_IntegerSize)(offset + length);
}

if (expected != size) {
    MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE, expected, "Bytes follow the last section.");

    return MAREA_BOOLEAN_FALSE;
}

return MAREA_BOOLEAN_TRUE;
}

static void MAREA_CALL MAREA_G_EnterSection(MAREA_G_Reader *reader, MAREA_IntegerSize offset, MAREA_IntegerSize length)
{
    reader->position = offset;
    reader->end = (offset + length);
}

static void MAREA_CALL MAREA_G_LeaveSection(MAREA_G_Reader *reader)
{
    if (reader->ok && (reader->position != reader->end)) {
        MAREA_G_Fail(reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, reader->position, "A section is longer than its content.");
    }
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeDeserialize(MAREA_BytecodeContext *context, const MAREA_Byte *bytes, MAREA_IntegerSize size, MAREA_BytecodeModule **outModule)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((bytes == MAREA_NULLPTR) && (size != MAREA_INTEGER_SIZE(0))) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outModule == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    *outModule = MAREA_NULLPTR;

    MAREA_BytecodeModule *module = MAREA_NULLPTR;

    MAREA_BytecodeExcept except = MAREA_BytecodeModuleCreate(context, &module);

    if (except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return except;
    }

    MAREA_G_Reader reader = { .module = module, .bytes = bytes, .except = MAREA_BYTECODE_EXCEPT_SUCCESS, .ok = MAREA_BOOLEAN_TRUE };

    MAREA_IntegerSize offsets[5];
    MAREA_IntegerSize sizes[5];

    if (!MAREA_G_ReadTable(&reader, size, offsets, sizes)) {
        return reader.except;
    }

    if ((sizes[MAREA_G_SECTION_SLOT_CODE] % MAREA_G_INSTRUCTION_SIZE) != MAREA_INTEGER_SIZE(0)) {
        MAREA_G_Fail(&reader, MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION, offsets[MAREA_G_SECTION_SLOT_CODE], "The code section is not a whole number of instructions.");

        return reader.except;
    }

    MAREA_G_EnterSection(&reader, offsets[MAREA_G_SECTION_SLOT_STRINGS], sizes[MAREA_G_SECTION_SLOT_STRINGS]);
    MAREA_G_ReadStrings(&reader);
    MAREA_G_LeaveSection(&reader);

    MAREA_G_EnterSection(&reader, offsets[MAREA_G_SECTION_SLOT_TYPES], sizes[MAREA_G_SECTION_SLOT_TYPES]);
    MAREA_G_ReadTypes(&reader);
    MAREA_G_LeaveSection(&reader);

    MAREA_G_EnterSection(&reader, offsets[MAREA_G_SECTION_SLOT_FUNCTIONS], sizes[MAREA_G_SECTION_SLOT_FUNCTIONS]);
    MAREA_G_ReadFunctions(&reader, &bytes[offsets[MAREA_G_SECTION_SLOT_CODE]], (sizes[MAREA_G_SECTION_SLOT_CODE] / MAREA_G_INSTRUCTION_SIZE), offsets[MAREA_G_SECTION_SLOT_CODE]);
    MAREA_G_LeaveSection(&reader);

    MAREA_G_EnterSection(&reader, offsets[MAREA_G_SECTION_SLOT_DATA], sizes[MAREA_G_SECTION_SLOT_DATA]);
    MAREA_G_ReadData(&reader);
    MAREA_G_LeaveSection(&reader);

    if (!reader.ok) {
        return reader.except;
    }

    MAREA_Boolean valid = MAREA_BOOLEAN_FALSE;

    except = MAREA_BytecodeModuleValidate(module, &valid);

    if ((except != MAREA_BYTECODE_EXCEPT_SUCCESS) || !valid) {
        return except;
    }

    *outModule = module;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
