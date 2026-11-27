// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeInternal.h"
#include "marea/bytecode/MAREABytecodeFormat.h"
#include "marea/bytecode/MAREABytecodeOpcode.h"

#include <string.h>

typedef struct MAREA_G_Buffer
{
    MAREA_Arena *arena;
    MAREA_Character8 *data;
    MAREA_IntegerSize size;
    MAREA_IntegerSize capacity;
    MAREA_BytecodeExcept except;
} MAREA_G_Buffer;

static void MAREA_CALL MAREA_G_Append(MAREA_G_Buffer *buffer, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (buffer->except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return;
    }

    if (length > (MAREA_INTEGER_SIZE_MAXIMUM - buffer->size - MAREA_INTEGER_SIZE(1))) {
        buffer->except = MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY;

        return;
    }

    while ((buffer->size + length + MAREA_INTEGER_SIZE(1)) > buffer->capacity) {
        MAREA_Null memory = buffer->data;

        buffer->except = MAREA_BytecodeGrow(buffer->arena, &memory, &buffer->capacity, buffer->capacity, MAREA_INTEGER_SIZE(1), MAREA_INTEGER_SIZE(1));

        if (buffer->except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return;
        }

        buffer->data = memory;
    }

    memcpy(&buffer->data[buffer->size], text, length);

    buffer->size += length;

    buffer->data[buffer->size] = '\0';
}

static void MAREA_CALL MAREA_G_AppendLiteral(MAREA_G_Buffer *buffer, const MAREA_Character8 *text)
{
    MAREA_G_Append(buffer, text, strlen(text));
}

static void MAREA_CALL MAREA_G_AppendNumber(MAREA_G_Buffer *buffer, MAREA_Uinteger64 value)
{
    MAREA_Character8 digits[24];

    MAREA_IntegerSize position = MAREA_ARRAY_SIZE(digits);

    do {
        position -= MAREA_INTEGER_SIZE(1);

        digits[position] = (MAREA_Character8)('0' + (value % MAREA_UINTEGER_64(10)));

        value /= MAREA_UINTEGER_64(10);
    } while (value != MAREA_UINTEGER_64(0));

    MAREA_G_Append(buffer, &digits[position], (MAREA_ARRAY_SIZE(digits) - position));
}

// A reference to a string, a type or an element: a letter and a number, or the word none
static void MAREA_CALL MAREA_G_AppendReference(MAREA_G_Buffer *buffer, MAREA_Character8 letter, MAREA_Uinteger32 index)
{
    if (index == MAREA_BYTECODE_NONE) {
        MAREA_G_AppendLiteral(buffer, "none");

        return;
    }

    MAREA_G_Append(buffer, &letter, MAREA_INTEGER_SIZE(1));

    MAREA_G_AppendNumber(buffer, index);
}

static void MAREA_CALL MAREA_G_AppendHexByte(MAREA_G_Buffer *buffer, MAREA_Byte value)
{
    static const MAREA_Character8 HEXADECIMAL[] = "0123456789abcdef";

    const MAREA_Character8 pair[2] = { HEXADECIMAL[(value >> 4)], HEXADECIMAL[(value & MAREA_UINTEGER_8(0x0F))] };

    MAREA_G_Append(buffer, pair, MAREA_INTEGER_SIZE(2));
}

static void MAREA_CALL MAREA_G_AppendQuoted(MAREA_G_Buffer *buffer, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_G_AppendLiteral(buffer, "\"");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < length; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_Byte value = (MAREA_Byte)text[index];

        if ((value == '"') || (value == '\\')) {
            MAREA_G_AppendLiteral(buffer, "\\");

            MAREA_G_Append(buffer, &text[index], MAREA_INTEGER_SIZE(1));
        } else if ((value >= MAREA_UINTEGER_8(0x20)) && (value < MAREA_UINTEGER_8(0x7F))) {
            MAREA_G_Append(buffer, &text[index], MAREA_INTEGER_SIZE(1));
        } else {
            MAREA_G_AppendLiteral(buffer, "\\x");

            MAREA_G_AppendHexByte(buffer, value);
        }
    }

    MAREA_G_AppendLiteral(buffer, "\"");
}

static void MAREA_CALL MAREA_G_WriteTypeLines(MAREA_G_Buffer *buffer, const MAREA_BytecodeModule *module)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->typeCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeType *type = &module->types[index];

        MAREA_G_AppendReference(buffer, 't', (MAREA_Uinteger32)index);
        MAREA_G_AppendLiteral(buffer, " ");

        const MAREA_Character8 *kind = MAREA_BytecodeTypeKindGetName((MAREA_Uinteger32)type->info.kind);

        MAREA_G_AppendLiteral(buffer, (kind != MAREA_NULLPTR) ? kind : "primitive");
        MAREA_G_AppendLiteral(buffer, " name ");
        MAREA_G_AppendReference(buffer, 's', type->info.name);
        MAREA_G_AppendLiteral(buffer, " size ");
        MAREA_G_AppendNumber(buffer, type->info.size);
        MAREA_G_AppendLiteral(buffer, " align ");
        MAREA_G_AppendNumber(buffer, type->info.alignment);
        MAREA_G_AppendLiteral(buffer, " element ");
        MAREA_G_AppendReference(buffer, 't', type->info.element);
        MAREA_G_AppendLiteral(buffer, " count ");
        MAREA_G_AppendNumber(buffer, type->info.count);
        MAREA_G_AppendLiteral(buffer, "\n");

        for (MAREA_Uinteger32 member = MAREA_UINTEGER_32(0); member < type->info.memberCount; member += MAREA_UINTEGER_32(1)) {
            MAREA_G_AppendLiteral(buffer, "  member name ");
            MAREA_G_AppendReference(buffer, 's', type->members[member].name);
            MAREA_G_AppendLiteral(buffer, " type ");
            MAREA_G_AppendReference(buffer, 't', type->members[member].type);
            MAREA_G_AppendLiteral(buffer, " value ");
            MAREA_G_AppendNumber(buffer, type->members[member].value);
            MAREA_G_AppendLiteral(buffer, "\n");
        }
    }
}

static void MAREA_CALL MAREA_G_WriteFunctionLines(MAREA_G_Buffer *buffer, const MAREA_BytecodeModule *module)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeFunction *function = &module->functions[index];

        MAREA_G_AppendReference(buffer, 'f', (MAREA_Uinteger32)index);
        MAREA_G_AppendLiteral(buffer, " name ");
        MAREA_G_AppendReference(buffer, 's', function->info.name);
        MAREA_G_AppendLiteral(buffer, " result ");
        MAREA_G_AppendReference(buffer, 't', function->info.result);
        MAREA_G_AppendLiteral(buffer, " registers ");
        MAREA_G_AppendNumber(buffer, function->info.registerCount);
        MAREA_G_AppendLiteral(buffer, " area ");
        MAREA_G_AppendNumber(buffer, function->info.frameArea);
        MAREA_G_AppendLiteral(buffer, " flags ");
        MAREA_G_AppendNumber(buffer, function->info.flags);
        MAREA_G_AppendLiteral(buffer, "\n");

        for (MAREA_Uinteger32 parameter = MAREA_UINTEGER_32(0); parameter < function->info.parameterCount; parameter += MAREA_UINTEGER_32(1)) {
            MAREA_G_AppendLiteral(buffer, "  param type ");
            MAREA_G_AppendReference(buffer, 't', function->parameters[parameter].type);
            MAREA_G_AppendLiteral(buffer, " flags ");
            MAREA_G_AppendNumber(buffer, function->parameters[parameter].flags);
            MAREA_G_AppendLiteral(buffer, "\n");
        }

        for (MAREA_Uinteger32 instruction = MAREA_UINTEGER_32(0); instruction < function->info.instructionCount; instruction += MAREA_UINTEGER_32(1)) {
            const MAREA_BytecodeInstruction *value = &function->instructions[instruction];

            const MAREA_Character8 *name = MAREA_BytecodeOpcodeGetName(value->opcode);

            MAREA_G_AppendLiteral(buffer, "  ins ");
            MAREA_G_AppendLiteral(buffer, (name != MAREA_NULLPTR) ? name : "nop");
            MAREA_G_AppendLiteral(buffer, " ");
            MAREA_G_AppendNumber(buffer, value->a);
            MAREA_G_AppendLiteral(buffer, " ");
            MAREA_G_AppendNumber(buffer, value->b);
            MAREA_G_AppendLiteral(buffer, " ");
            MAREA_G_AppendNumber(buffer, value->c);
            MAREA_G_AppendLiteral(buffer, "\n");
        }
    }
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeDisassemble(const MAREA_BytecodeModule *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
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

    MAREA_G_Buffer buffer = { .arena = module->context->arena, .except = MAREA_BYTECODE_EXCEPT_SUCCESS };

    MAREA_G_AppendLiteral(&buffer, ".marea-bytecode ");
    MAREA_G_AppendNumber(&buffer, MAREA_BYTECODE_FORMAT_VERSION_MAJOR);
    MAREA_G_AppendLiteral(&buffer, ".");
    MAREA_G_AppendNumber(&buffer, MAREA_BYTECODE_FORMAT_VERSION_MINOR);
    MAREA_G_AppendLiteral(&buffer, "\n.strings\n");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->stringCount; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_G_AppendReference(&buffer, 's', (MAREA_Uinteger32)index);
        MAREA_G_AppendLiteral(&buffer, " ");
        MAREA_G_AppendQuoted(&buffer, module->strings[index].text, module->strings[index].length);
        MAREA_G_AppendLiteral(&buffer, "\n");
    }

    MAREA_G_AppendLiteral(&buffer, ".types\n");
    MAREA_G_WriteTypeLines(&buffer, module);

    MAREA_G_AppendLiteral(&buffer, ".functions\n");
    MAREA_G_WriteFunctionLines(&buffer, module);

    MAREA_G_AppendLiteral(&buffer, ".data\n");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < module->blobCount; index += MAREA_INTEGER_SIZE(1)) {
        MAREA_G_AppendReference(&buffer, 'd', (MAREA_Uinteger32)index);
        MAREA_G_AppendLiteral(&buffer, " ");

        if (module->blobs[index].size == MAREA_INTEGER_SIZE(0)) {
            MAREA_G_AppendLiteral(&buffer, "-");
        }

        for (MAREA_IntegerSize byte = MAREA_INTEGER_SIZE(0); byte < module->blobs[index].size; byte += MAREA_INTEGER_SIZE(1)) {
            MAREA_G_AppendHexByte(&buffer, module->blobs[index].bytes[byte]);
        }

        MAREA_G_AppendLiteral(&buffer, "\n");
    }

    if (buffer.except != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return buffer.except;
    }

    *outText = buffer.data;
    *outLength = buffer.size;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

typedef enum MAREA_G_TextSection
{
    MAREA_G_TEXT_SECTION_NONE = 0,
    MAREA_G_TEXT_SECTION_STRINGS = 1,
    MAREA_G_TEXT_SECTION_TYPES = 2,
    MAREA_G_TEXT_SECTION_FUNCTIONS = 3,
    MAREA_G_TEXT_SECTION_DATA = 4,
} MAREA_G_TextSection;

typedef struct MAREA_G_Parser
{
    MAREA_BytecodeModule *module;
    const MAREA_Character8 *text;
    MAREA_IntegerSize length;
    MAREA_IntegerSize cursor;
    MAREA_IntegerSize lineStart;
    MAREA_IntegerSize lineEnd;
    MAREA_Uinteger32 line;
    MAREA_BytecodeExcept except;
    MAREA_Boolean ok;
} MAREA_G_Parser;

static void MAREA_CALL MAREA_G_Fail(MAREA_G_Parser *parser, MAREA_BytecodeDiagnosticCode code, MAREA_IntegerSize at, const MAREA_Character8 *message)
{
    if (!parser->ok) {
        return;
    }

    parser->ok = MAREA_BOOLEAN_FALSE;
    parser->except = MAREA_BytecodeRaise(parser->module->context, code, (MAREA_Uinteger32)at, parser->line, (MAREA_Uinteger32)((at - parser->lineStart) + MAREA_INTEGER_SIZE(1)), message);
}

static void MAREA_CALL MAREA_G_FailAtEnd(MAREA_G_Parser *parser, MAREA_BytecodeDiagnosticCode code, const MAREA_Character8 *message)
{
    if (!parser->ok) {
        return;
    }

    parser->ok = MAREA_BOOLEAN_FALSE;
    parser->except = MAREA_BytecodeRaise(parser->module->context, code, (MAREA_Uinteger32)parser->length, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), message);
}

static void MAREA_CALL MAREA_G_Check(MAREA_G_Parser *parser, MAREA_BytecodeExcept except)
{
    if ((except != MAREA_BYTECODE_EXCEPT_SUCCESS) && parser->ok) {
        parser->ok = MAREA_BOOLEAN_FALSE;
        parser->except = except;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBlank(MAREA_Character8 value)
{
    return ((value == ' ') || (value == '\t') || (value == '\r'));
}

static void MAREA_CALL MAREA_G_SkipBlanks(MAREA_G_Parser *parser)
{
    while ((parser->cursor < parser->lineEnd) && MAREA_G_IsBlank(parser->text[parser->cursor])) {
        parser->cursor += MAREA_INTEGER_SIZE(1);
    }
}

// True when only blanks and a comment are left on the line
static MAREA_Boolean MAREA_CALL MAREA_G_AtEnd(MAREA_G_Parser *parser)
{
    MAREA_G_SkipBlanks(parser);

    return ((parser->cursor >= parser->lineEnd) || (parser->text[parser->cursor] == ';'));
}

// The next word of the line: from here to a blank or to a semicolon. A word never holds a quote.
static MAREA_Boolean MAREA_CALL MAREA_G_NextWord(MAREA_G_Parser *parser, MAREA_IntegerSize *outStart, MAREA_IntegerSize *outLength)
{
    if (MAREA_G_AtEnd(parser)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outStart = parser->cursor;

    while ((parser->cursor < parser->lineEnd) && !MAREA_G_IsBlank(parser->text[parser->cursor]) && (parser->text[parser->cursor] != ';')) {
        parser->cursor += MAREA_INTEGER_SIZE(1);
    }

    *outLength = (parser->cursor - *outStart);

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_WordIs(const MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length, const MAREA_Character8 *literal)
{
    return ((strlen(literal) == length) && (memcmp(&parser->text[start], literal, length) == 0));
}

// Reads the next word and checks it is the given key
static MAREA_Boolean MAREA_CALL MAREA_G_ExpectKey(MAREA_G_Parser *parser, const MAREA_Character8 *key)
{
    MAREA_IntegerSize start = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (!parser->ok) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (!MAREA_G_NextWord(parser, &start, &length)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The line ends before a field that it needs.");

        return MAREA_BOOLEAN_FALSE;
    }

    if (!MAREA_G_WordIs(parser, start, length, key)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "A field name is missing or in the wrong place.");

        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

// Decimal digits only, at most maximum
static MAREA_Uinteger64 MAREA_CALL MAREA_G_ParseDigits(MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length, MAREA_Uinteger64 maximum)
{
    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

    if (length == MAREA_INTEGER_SIZE(0)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, start, "A number has no digit.");

        return MAREA_UINTEGER_64(0);
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < length; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_Character8 digit = parser->text[(start + index)];

        if ((digit < '0') || (digit > '9')) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, start, "A number holds a character that is not a digit.");

            return MAREA_UINTEGER_64(0);
        }

        const MAREA_Uinteger64 step = (MAREA_Uinteger64)(digit - '0');

        if (value > ((maximum - step) / MAREA_UINTEGER_64(10))) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, start, "A number is larger than its field allows.");

            return MAREA_UINTEGER_64(0);
        }

        value = ((value * MAREA_UINTEGER_64(10)) + step);
    }

    return value;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_ReadNumber(MAREA_G_Parser *parser, MAREA_Uinteger64 maximum)
{
    MAREA_IntegerSize start = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (!parser->ok) {
        return MAREA_UINTEGER_64(0);
    }

    if (!MAREA_G_NextWord(parser, &start, &length)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The line ends before a number that it needs.");

        return MAREA_UINTEGER_64(0);
    }

    return MAREA_G_ParseDigits(parser, start, length, maximum);
}

// Reads a letter and a number (s3, t0), or the word none when optional is set, and checks nothing else
static MAREA_Uinteger32 MAREA_CALL MAREA_G_ReadReference(MAREA_G_Parser *parser, MAREA_Character8 letter, MAREA_Boolean optional)
{
    MAREA_IntegerSize start = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (!parser->ok) {
        return MAREA_UINTEGER_32(0);
    }

    if (!MAREA_G_NextWord(parser, &start, &length)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The line ends before a reference that it needs.");

        return MAREA_UINTEGER_32(0);
    }

    if (optional && MAREA_G_WordIs(parser, start, length, "none")) {
        return MAREA_BYTECODE_NONE;
    }

    if ((length < MAREA_INTEGER_SIZE(2)) || (parser->text[start] != letter)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, start, "A reference does not have the form that its field needs.");

        return MAREA_UINTEGER_32(0);
    }

    return (MAREA_Uinteger32)MAREA_G_ParseDigits(parser, (start + MAREA_INTEGER_SIZE(1)), (length - MAREA_INTEGER_SIZE(1)), (MAREA_Uinteger64)(MAREA_BYTECODE_NONE - MAREA_UINTEGER_32(1)));
}

static void MAREA_CALL MAREA_G_ExpectLineEnd(MAREA_G_Parser *parser)
{
    if (parser->ok && !MAREA_G_AtEnd(parser)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "Text follows the end of the line content.");
    }
}

// The first word of an item line must be the letter and the number that the next item has
static void MAREA_CALL MAREA_G_ExpectIndex(MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length, MAREA_Character8 letter, MAREA_IntegerSize expected)
{
    MAREA_Character8 digits[24];

    MAREA_IntegerSize position = MAREA_ARRAY_SIZE(digits);

    MAREA_IntegerSize rest = expected;

    do {
        position -= MAREA_INTEGER_SIZE(1);

        digits[position] = (MAREA_Character8)('0' + (rest % MAREA_INTEGER_SIZE(10)));

        rest /= MAREA_INTEGER_SIZE(10);
    } while (rest != MAREA_INTEGER_SIZE(0));

    const MAREA_IntegerSize width = (MAREA_ARRAY_SIZE(digits) - position);

    if ((length != (width + MAREA_INTEGER_SIZE(1))) || (parser->text[start] != letter) || (memcmp(&parser->text[(start + MAREA_INTEGER_SIZE(1))], &digits[position], width) != 0)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_INDEX, start, "An item does not have the index that comes next.");
    }
}

static void MAREA_CALL MAREA_G_ParseStringLine(MAREA_G_Parser *parser)
{
    MAREA_G_SkipBlanks(parser);

    if ((parser->cursor >= parser->lineEnd) || (parser->text[parser->cursor] != '"')) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, parser->cursor, "A string must start with a quote.");

        return;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(parser->module->context->arena, ((parser->lineEnd - parser->cursor) + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        MAREA_G_Check(parser, MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY);

        return;
    }

    MAREA_Character8 *decoded = memory;

    MAREA_IntegerSize size = MAREA_INTEGER_SIZE(0);

    parser->cursor += MAREA_INTEGER_SIZE(1);

    while (parser->ok) {
        if (parser->cursor >= parser->lineEnd) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, parser->cursor, "A string has no closing quote.");

            return;
        }

        const MAREA_Character8 value = parser->text[parser->cursor];

        if (value == '"') {
            parser->cursor += MAREA_INTEGER_SIZE(1);

            break;
        }

        if (((MAREA_Byte)value < MAREA_UINTEGER_8(0x20)) || ((MAREA_Byte)value >= MAREA_UINTEGER_8(0x7F))) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, parser->cursor, "A string holds a character that is not printable ASCII.");

            return;
        }

        if (value != '\\') {
            decoded[size] = value;
            size += MAREA_INTEGER_SIZE(1);
            parser->cursor += MAREA_INTEGER_SIZE(1);

            continue;
        }

        const MAREA_IntegerSize escape = (parser->cursor + MAREA_INTEGER_SIZE(1));

        if ((escape < parser->lineEnd) && ((parser->text[escape] == '"') || (parser->text[escape] == '\\'))) {
            decoded[size] = parser->text[escape];
            size += MAREA_INTEGER_SIZE(1);
            parser->cursor += MAREA_INTEGER_SIZE(2);

            continue;
        }

        MAREA_Uinteger32 code = MAREA_UINTEGER_32(0);

        MAREA_Boolean valid = ((escape + MAREA_INTEGER_SIZE(2)) < parser->lineEnd) && (parser->text[escape] == 'x');

        for (MAREA_IntegerSize digit = MAREA_INTEGER_SIZE(1); valid && (digit <= MAREA_INTEGER_SIZE(2)); digit += MAREA_INTEGER_SIZE(1)) {
            const MAREA_Character8 hex = parser->text[(escape + digit)];

            code <<= MAREA_UINTEGER_32(4);

            if ((hex >= '0') && (hex <= '9')) {
                code |= (MAREA_Uinteger32)(hex - '0');
            } else if ((hex >= 'a') && (hex <= 'f')) {
                code |= (MAREA_Uinteger32)((hex - 'a') + 10);
            } else if ((hex >= 'A') && (hex <= 'F')) {
                code |= (MAREA_Uinteger32)((hex - 'A') + 10);
            } else {
                valid = MAREA_BOOLEAN_FALSE;
            }
        }

        if (!valid) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_STRING, parser->cursor, "A string holds an escape that is not known.");

            return;
        }

        decoded[size] = (MAREA_Character8)code;
        size += MAREA_INTEGER_SIZE(1);
        parser->cursor += MAREA_INTEGER_SIZE(4);
    }

    if (parser->ok) {
        MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

        MAREA_G_Check(parser, MAREA_BytecodeModuleAppendString(parser->module, decoded, size, &added));
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_ReadKind(MAREA_G_Parser *parser, MAREA_BytecodeTypeKind *outKind)
{
    MAREA_IntegerSize start = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (!MAREA_G_NextWord(parser, &start, &length)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The line ends before the type kind.");

        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_Uinteger32 kind = MAREA_UINTEGER_32(0); kind <= (MAREA_Uinteger32)MAREA_BYTECODE_TYPE_KIND_MAXIMUM; kind += MAREA_UINTEGER_32(1)) {
        if (MAREA_G_WordIs(parser, start, length, MAREA_BytecodeTypeKindGetName(kind))) {
            *outKind = (MAREA_BytecodeTypeKind)kind;

            return MAREA_BOOLEAN_TRUE;
        }
    }

    MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "The type kind is not known.");

    return MAREA_BOOLEAN_FALSE;
}

static void MAREA_CALL MAREA_G_ParseTypeLine(MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length)
{
    if (MAREA_G_WordIs(parser, start, length, "member")) {
        if (parser->module->typeCount == MAREA_INTEGER_SIZE(0)) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "A member line must follow a type line.");

            return;
        }

        MAREA_BytecodeMemberInfo member = { 0 };

        MAREA_IGNORE(MAREA_G_ExpectKey(parser, "name"));
        member.name = MAREA_G_ReadReference(parser, 's', MAREA_BOOLEAN_TRUE);
        MAREA_IGNORE(MAREA_G_ExpectKey(parser, "type"));
        member.type = MAREA_G_ReadReference(parser, 't', MAREA_BOOLEAN_FALSE);
        MAREA_IGNORE(MAREA_G_ExpectKey(parser, "value"));
        member.value = MAREA_G_ReadNumber(parser, MAREA_UINTEGER_64_MAXIMUM);
        MAREA_G_ExpectLineEnd(parser);

        if (parser->ok) {
            MAREA_G_Check(parser, MAREA_BytecodeModuleAddMember(parser->module, (MAREA_Uinteger32)(parser->module->typeCount - MAREA_INTEGER_SIZE(1)), &member));
        }

        return;
    }

    if (MAREA_G_WordIs(parser, start, length, "param") || MAREA_G_WordIs(parser, start, length, "ins")) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "This line does not belong in the types section.");

        return;
    }

    MAREA_G_ExpectIndex(parser, start, length, 't', parser->module->typeCount);

    MAREA_BytecodeTypeInfo info = { 0 };

    MAREA_BytecodeTypeKind kind = MAREA_BYTECODE_TYPE_KIND_PRIMITIVE;

    if (parser->ok && MAREA_G_ReadKind(parser, &kind)) {
        info.kind = kind;
    }

    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "name"));
    info.name = MAREA_G_ReadReference(parser, 's', MAREA_BOOLEAN_TRUE);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "size"));
    info.size = MAREA_G_ReadNumber(parser, MAREA_UINTEGER_64_MAXIMUM);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "align"));
    info.alignment = (MAREA_Uinteger32)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_32_MAXIMUM);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "element"));
    info.element = MAREA_G_ReadReference(parser, 't', MAREA_BOOLEAN_TRUE);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "count"));
    info.count = MAREA_G_ReadNumber(parser, MAREA_UINTEGER_64_MAXIMUM);
    MAREA_G_ExpectLineEnd(parser);

    if (parser->ok) {
        MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

        MAREA_G_Check(parser, MAREA_BytecodeModuleAddType(parser->module, &info, &added));
    }
}

static void MAREA_CALL MAREA_G_ParseFunctionLine(MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length)
{
    const MAREA_Boolean isParameter = MAREA_G_WordIs(parser, start, length, "param");

    const MAREA_Boolean isInstruction = MAREA_G_WordIs(parser, start, length, "ins");

    if (MAREA_G_WordIs(parser, start, length, "member")) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "This line does not belong in the functions section.");

        return;
    }

    if (isParameter || isInstruction) {
        if (parser->module->functionCount == MAREA_INTEGER_SIZE(0)) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "A parameter or instruction line must follow a function line.");

            return;
        }

        const MAREA_Uinteger32 function = (MAREA_Uinteger32)(parser->module->functionCount - MAREA_INTEGER_SIZE(1));

        if (isParameter) {
            MAREA_BytecodeParameterInfo parameter = { 0 };

            MAREA_IGNORE(MAREA_G_ExpectKey(parser, "type"));
            parameter.type = MAREA_G_ReadReference(parser, 't', MAREA_BOOLEAN_FALSE);
            MAREA_IGNORE(MAREA_G_ExpectKey(parser, "flags"));
            parameter.flags = (MAREA_Uinteger32)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_32_MAXIMUM);
            MAREA_G_ExpectLineEnd(parser);

            if (parser->ok) {
                MAREA_G_Check(parser, MAREA_BytecodeModuleAddParameter(parser->module, function, &parameter));
            }

            return;
        }

        MAREA_IntegerSize nameStart = MAREA_INTEGER_SIZE(0);
        MAREA_IntegerSize nameLength = MAREA_INTEGER_SIZE(0);

        MAREA_BytecodeInstruction instruction = { 0 };

        if (!MAREA_G_NextWord(parser, &nameStart, &nameLength)) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The line ends before the opcode name.");

            return;
        }

        if (!MAREA_BytecodeOpcodeFromName(&parser->text[nameStart], nameLength, &instruction.opcode)) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNKNOWN_OPCODE, nameStart, "The opcode name is not in the table.");

            return;
        }

        instruction.a = (MAREA_Uinteger16)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_16_MAXIMUM);
        instruction.b = (MAREA_Uinteger16)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_16_MAXIMUM);
        instruction.c = (MAREA_Uinteger16)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_16_MAXIMUM);
        MAREA_G_ExpectLineEnd(parser);

        if (parser->ok) {
            MAREA_G_Check(parser, MAREA_BytecodeModuleAddInstruction(parser->module, function, &instruction));
        }

        return;
    }

    MAREA_G_ExpectIndex(parser, start, length, 'f', parser->module->functionCount);

    MAREA_BytecodeFunctionInfo info = { 0 };

    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "name"));
    info.name = MAREA_G_ReadReference(parser, 's', MAREA_BOOLEAN_TRUE);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "result"));
    info.result = MAREA_G_ReadReference(parser, 't', MAREA_BOOLEAN_TRUE);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "registers"));
    info.registerCount = (MAREA_Uinteger32)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_32_MAXIMUM);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "area"));
    info.frameArea = MAREA_G_ReadNumber(parser, MAREA_UINTEGER_64_MAXIMUM);
    MAREA_IGNORE(MAREA_G_ExpectKey(parser, "flags"));
    info.flags = (MAREA_Uinteger32)MAREA_G_ReadNumber(parser, MAREA_UINTEGER_32_MAXIMUM);
    MAREA_G_ExpectLineEnd(parser);

    if (parser->ok) {
        MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

        MAREA_G_Check(parser, MAREA_BytecodeModuleAddFunction(parser->module, &info, &added));
    }
}

static MAREA_Sinteger32 MAREA_CALL MAREA_G_HexValue(MAREA_Character8 digit)
{
    if ((digit >= '0') && (digit <= '9')) {
        return (MAREA_Sinteger32)(digit - '0');
    }

    if ((digit >= 'a') && (digit <= 'f')) {
        return (MAREA_Sinteger32)((digit - 'a') + 10);
    }

    if ((digit >= 'A') && (digit <= 'F')) {
        return (MAREA_Sinteger32)((digit - 'A') + 10);
    }

    return -1;
}

static void MAREA_CALL MAREA_G_ParseDataLine(MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length)
{
    MAREA_G_ExpectIndex(parser, start, length, 'd', parser->module->blobCount);

    MAREA_IntegerSize hexStart = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize hexLength = MAREA_INTEGER_SIZE(0);

    if (parser->ok && !MAREA_G_NextWord(parser, &hexStart, &hexLength)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The line ends before the bytes of the blob.");
    }

    MAREA_G_ExpectLineEnd(parser);

    if (!parser->ok) {
        return;
    }

    const MAREA_Boolean isEmpty = MAREA_G_WordIs(parser, hexStart, hexLength, "-");

    if (!isEmpty && ((hexLength % MAREA_INTEGER_SIZE(2)) != MAREA_INTEGER_SIZE(0))) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, hexStart, "The bytes of a blob need two hexadecimal digits each.");

        return;
    }

    const MAREA_IntegerSize size = (isEmpty ? MAREA_INTEGER_SIZE(0) : (hexLength / MAREA_INTEGER_SIZE(2)));

    MAREA_Null memory = MAREA_NULLPTR;

    if (size > MAREA_INTEGER_SIZE(0)) {
        if (MAREA_ArenaAllocate(parser->module->context->arena, size, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            MAREA_G_Check(parser, MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY);

            return;
        }
    }

    MAREA_Byte *bytes = memory;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < size; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_Sinteger32 high = MAREA_G_HexValue(parser->text[(hexStart + (index * MAREA_INTEGER_SIZE(2)))]);
        const MAREA_Sinteger32 low = MAREA_G_HexValue(parser->text[(hexStart + (index * MAREA_INTEGER_SIZE(2)) + MAREA_INTEGER_SIZE(1))]);

        if ((high < 0) || (low < 0)) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, hexStart, "The bytes of a blob hold a character that is not hexadecimal.");

            return;
        }

        bytes[index] = (MAREA_Byte)((high << 4) | low);
    }

    MAREA_Uinteger32 added = MAREA_UINTEGER_32(0);

    MAREA_G_Check(parser, MAREA_BytecodeModuleAddData(parser->module, bytes, size, &added));
}

static void MAREA_CALL MAREA_G_ParseHeaderLine(MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length)
{
    if (!MAREA_G_WordIs(parser, start, length, ".marea-bytecode")) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, start, "The first line must be the marea-bytecode header.");

        return;
    }

    MAREA_IntegerSize versionStart = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize versionLength = MAREA_INTEGER_SIZE(0);

    if (!MAREA_G_NextWord(parser, &versionStart, &versionLength)) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, parser->cursor, "The header has no version.");

        return;
    }

    MAREA_IntegerSize dot = MAREA_INTEGER_SIZE(0);

    while ((dot < versionLength) && (parser->text[(versionStart + dot)] != '.')) {
        dot += MAREA_INTEGER_SIZE(1);
    }

    if (dot == versionLength) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_NUMBER, versionStart, "The version must be a major number, a dot and a minor number.");

        return;
    }

    const MAREA_Uinteger64 major = MAREA_G_ParseDigits(parser, versionStart, dot, MAREA_UINTEGER_32_MAXIMUM);

    const MAREA_Uinteger64 minor = MAREA_G_ParseDigits(parser, (versionStart + dot + MAREA_INTEGER_SIZE(1)), (versionLength - dot - MAREA_INTEGER_SIZE(1)), MAREA_UINTEGER_32_MAXIMUM);

    MAREA_G_ExpectLineEnd(parser);

    if (parser->ok && ((major != MAREA_BYTECODE_FORMAT_VERSION_MAJOR) || (minor > MAREA_BYTECODE_FORMAT_VERSION_MINOR))) {
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION, versionStart, "The version of the text form is not supported.");
    }
}

static MAREA_G_TextSection MAREA_CALL MAREA_G_SectionOfDirective(const MAREA_G_Parser *parser, MAREA_IntegerSize start, MAREA_IntegerSize length)
{
    if (MAREA_G_WordIs(parser, start, length, ".strings")) {
        return MAREA_G_TEXT_SECTION_STRINGS;
    }

    if (MAREA_G_WordIs(parser, start, length, ".types")) {
        return MAREA_G_TEXT_SECTION_TYPES;
    }

    if (MAREA_G_WordIs(parser, start, length, ".functions")) {
        return MAREA_G_TEXT_SECTION_FUNCTIONS;
    }

    if (MAREA_G_WordIs(parser, start, length, ".data")) {
        return MAREA_G_TEXT_SECTION_DATA;
    }

    return MAREA_G_TEXT_SECTION_NONE;
}

static void MAREA_CALL MAREA_G_ParseLine(MAREA_G_Parser *parser, MAREA_Boolean *headerSeen, MAREA_G_TextSection *section)
{
    MAREA_IntegerSize start = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (!MAREA_G_NextWord(parser, &start, &length)) {
        return;
    }

    if (!*headerSeen) {
        MAREA_G_ParseHeaderLine(parser, start, length);

        *headerSeen = MAREA_BOOLEAN_TRUE;

        return;
    }

    if (parser->text[start] == '.') {
        const MAREA_G_TextSection next = MAREA_G_SectionOfDirective(parser, start, length);

        if ((next == MAREA_G_TEXT_SECTION_NONE) || (next != (MAREA_G_TextSection)((int)*section + 1))) {
            MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, start, "A section is unknown, repeated or out of order.");

            return;
        }

        MAREA_G_ExpectLineEnd(parser);

        *section = next;

        return;
    }

    switch (*section) {
    case MAREA_G_TEXT_SECTION_STRINGS:
        MAREA_G_ExpectIndex(parser, start, length, 's', parser->module->stringCount);

        if (parser->ok) {
            MAREA_G_ParseStringLine(parser);
        }

        MAREA_G_ExpectLineEnd(parser);

        break;
    case MAREA_G_TEXT_SECTION_TYPES:
        MAREA_G_ParseTypeLine(parser, start, length);

        break;
    case MAREA_G_TEXT_SECTION_FUNCTIONS:
        MAREA_G_ParseFunctionLine(parser, start, length);

        break;
    case MAREA_G_TEXT_SECTION_DATA:
        MAREA_G_ParseDataLine(parser, start, length);

        break;
    case MAREA_G_TEXT_SECTION_NONE:
    default:
        MAREA_G_Fail(parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, start, "A line comes before the first section.");

        break;
    }
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeAssemble(MAREA_BytecodeContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_BytecodeModule **outModule)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((text == MAREA_NULLPTR) && (length != MAREA_INTEGER_SIZE(0))) {
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

    MAREA_G_Parser parser = { .module = module, .text = text, .length = length, .except = MAREA_BYTECODE_EXCEPT_SUCCESS, .ok = MAREA_BOOLEAN_TRUE };

    MAREA_Boolean headerSeen = MAREA_BOOLEAN_FALSE;

    MAREA_G_TextSection section = MAREA_G_TEXT_SECTION_NONE;

    MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0);

    while ((position < length) && parser.ok) {
        MAREA_IntegerSize end = position;

        while ((end < length) && (text[end] != '\n')) {
            end += MAREA_INTEGER_SIZE(1);
        }

        parser.line += MAREA_UINTEGER_32(1);
        parser.lineStart = position;
        parser.lineEnd = end;
        parser.cursor = position;

        MAREA_G_ParseLine(&parser, &headerSeen, &section);

        position = (end + MAREA_INTEGER_SIZE(1));
    }

    if (parser.ok && !headerSeen) {
        MAREA_G_FailAtEnd(&parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_SYNTAX, "The input has no marea-bytecode header.");
    }

    if (parser.ok && (section != MAREA_G_TEXT_SECTION_DATA)) {
        MAREA_G_FailAtEnd(&parser, MAREA_BYTECODE_DIAGNOSTIC_CODE_TEXT_BAD_SECTION, "A section is missing at the end of the input.");
    }

    if (!parser.ok) {
        return parser.except;
    }

    MAREA_Boolean valid = MAREA_BOOLEAN_FALSE;

    except = MAREA_BytecodeModuleValidate(module, &valid);

    if ((except != MAREA_BYTECODE_EXCEPT_SUCCESS) || !valid) {
        return except;
    }

    *outModule = module;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}
