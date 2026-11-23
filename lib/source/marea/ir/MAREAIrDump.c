// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

#if !defined(MAREA_IR_DUMP_INITIAL_CAPACITY)
    #define MAREA_IR_DUMP_INITIAL_CAPACITY MAREA_INTEGER_SIZE(1024)
#endif

typedef struct MAREA_G_DumpBuffer
{
    MAREA_Arena *arena;
    MAREA_Character8 *data;
    MAREA_IntegerSize length;
    MAREA_IntegerSize capacity;
    MAREA_Boolean failed;
} MAREA_G_DumpBuffer;

static void MAREA_CALL MAREA_G_DumpReserve(MAREA_G_DumpBuffer *buffer, MAREA_IntegerSize extra)
{
    if (buffer->failed == MAREA_BOOLEAN_TRUE) {
        return;
    }

    // INFO: One extra byte is always kept free for the final NUL.
    if ((buffer->length + extra + MAREA_INTEGER_SIZE(1)) <= buffer->capacity) {
        return;
    }

    MAREA_IntegerSize newCapacity = (buffer->capacity == MAREA_INTEGER_SIZE(0)) ? MAREA_IR_DUMP_INITIAL_CAPACITY : (buffer->capacity * MAREA_INTEGER_SIZE(2));

    while ((buffer->length + extra + MAREA_INTEGER_SIZE(1)) > newCapacity) {
        newCapacity = (newCapacity * MAREA_INTEGER_SIZE(2));
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(buffer->arena, newCapacity, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        buffer->failed = MAREA_BOOLEAN_TRUE;

        return;
    }

    if (buffer->length > MAREA_INTEGER_SIZE(0)) {
        memcpy(memory, buffer->data, buffer->length);
    }

    buffer->data = memory;
    buffer->capacity = newCapacity;
}

static void MAREA_CALL MAREA_G_DumpWrite(MAREA_G_DumpBuffer *buffer, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_G_DumpReserve(buffer, length);

    if (buffer->failed == MAREA_BOOLEAN_TRUE) {
        return;
    }

    memcpy((buffer->data + buffer->length), text, length);

    buffer->length = (buffer->length + length);
}

static void MAREA_CALL MAREA_G_DumpWriteText(MAREA_G_DumpBuffer *buffer, const MAREA_Character8 *text)
{
    MAREA_G_DumpWrite(buffer, text, strlen(text));
}

static void MAREA_CALL MAREA_G_DumpWriteNumber(MAREA_G_DumpBuffer *buffer, MAREA_Uinteger32 value)
{
    MAREA_Character8 digits[16];

    MAREA_IntegerSize count = MAREA_INTEGER_SIZE(0);

    do {
        digits[count] = (MAREA_Character8)('0' + (value % MAREA_UINTEGER_32(10)));
        count = (count + MAREA_INTEGER_SIZE(1));
        value = (value / MAREA_UINTEGER_32(10));
    } while (value != MAREA_UINTEGER_32(0));

    while (count > MAREA_INTEGER_SIZE(0)) {
        count = (count - MAREA_INTEGER_SIZE(1));

        MAREA_G_DumpWrite(buffer, &digits[count], MAREA_INTEGER_SIZE(1));
    }
}

static void MAREA_CALL MAREA_G_DumpWriteEscaped(MAREA_G_DumpBuffer *buffer, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    static const MAREA_Character8 hexDigits[] = "0123456789ABCDEF";

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < length; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Uinteger8 byte = (MAREA_Uinteger8)text[index];

        if (byte == (MAREA_Uinteger8)'"') {
            MAREA_G_DumpWriteText(buffer, "\\\"");
        } else if (byte == (MAREA_Uinteger8)'\\') {
            MAREA_G_DumpWriteText(buffer, "\\\\");
        } else if ((byte < (MAREA_Uinteger8)0x20) || (byte > (MAREA_Uinteger8)0x7E)) {
            const MAREA_Character8 escape[4] = { '\\', 'x', hexDigits[(byte >> 4)], hexDigits[(byte & 0x0F)] };

            MAREA_G_DumpWrite(buffer, escape, MAREA_INTEGER_SIZE(4));
        } else {
            MAREA_G_DumpWrite(buffer, &text[index], MAREA_INTEGER_SIZE(1));
        }
    }
}

typedef struct MAREA_G_DumpFrame
{
    const MAREA_IrNode *node;
    MAREA_IntegerSize depth;
} MAREA_G_DumpFrame;

typedef struct MAREA_G_DumpStack
{
    MAREA_G_DumpFrame *frames;
    MAREA_IntegerSize count;
    MAREA_IntegerSize capacity;
} MAREA_G_DumpStack;

// INFO: The stack lives in the caller arena, so a deep tree costs heap and never call-stack depth.
static void MAREA_CALL MAREA_G_DumpPush(MAREA_G_DumpBuffer *buffer, MAREA_G_DumpStack *stack, const MAREA_IrNode *node, MAREA_IntegerSize depth)
{
    if (buffer->failed == MAREA_BOOLEAN_TRUE) {
        return;
    }

    if (stack->count == stack->capacity) {
        const MAREA_IntegerSize newCapacity = (stack->capacity == MAREA_INTEGER_SIZE(0)) ? MAREA_INTEGER_SIZE(64) : (stack->capacity * MAREA_INTEGER_SIZE(2));

        MAREA_Null memory = MAREA_NULLPTR;

        if (MAREA_ArenaAllocate(buffer->arena, (newCapacity * sizeof(MAREA_G_DumpFrame)), MAREA_ALIGNOF(MAREA_G_DumpFrame), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            buffer->failed = MAREA_BOOLEAN_TRUE;

            return;
        }

        if (stack->count > MAREA_INTEGER_SIZE(0)) {
            memcpy(memory, stack->frames, (stack->count * sizeof(MAREA_G_DumpFrame)));
        }

        stack->frames = memory;
        stack->capacity = newCapacity;
    }

    stack->frames[stack->count].node = node;
    stack->frames[stack->count].depth = depth;
    stack->count = (stack->count + MAREA_INTEGER_SIZE(1));
}

static void MAREA_CALL MAREA_G_DumpNodeLine(MAREA_G_DumpBuffer *buffer, const MAREA_IrNode *node, MAREA_IntegerSize depth)
{
    for (MAREA_IntegerSize level = MAREA_INTEGER_SIZE(0); level < depth; level = (level + MAREA_INTEGER_SIZE(1))) {
        MAREA_G_DumpWriteText(buffer, "  ");
    }

    MAREA_G_DumpWriteText(buffer, MAREA_IrKindGetName(node->kind));

    if (node->text != MAREA_NULLPTR) {
        MAREA_G_DumpWriteText(buffer, " \"");
        MAREA_G_DumpWriteEscaped(buffer, node->text, node->textLength);
        MAREA_G_DumpWriteText(buffer, "\"");
    }

    if (node->typeName != MAREA_NULLPTR) {
        MAREA_G_DumpWriteText(buffer, " type=");
        MAREA_G_DumpWrite(buffer, node->typeName, node->typeNameLength);
    }

    if (node->operatorKind != MAREA_TOKEN_KIND_ERROR) {
        MAREA_G_DumpWriteText(buffer, " op=");
        MAREA_G_DumpWriteText(buffer, MAREA_TokenKindGetName(node->operatorKind));
    }

    if (node->flags != MAREA_UINTEGER_32(0)) {
        MAREA_G_DumpWriteText(buffer, " flags=");
        MAREA_G_DumpWriteNumber(buffer, node->flags);
    }

    MAREA_G_DumpWriteText(buffer, "\n");
}

static void MAREA_CALL MAREA_G_DumpNode(MAREA_G_DumpBuffer *buffer, const MAREA_IrNode *root)
{
    MAREA_G_DumpStack stack = { .frames = MAREA_NULLPTR, .count = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0) };

    MAREA_G_DumpPush(buffer, &stack, root, MAREA_INTEGER_SIZE(0));

    while ((stack.count > MAREA_INTEGER_SIZE(0)) && (buffer->failed == MAREA_BOOLEAN_FALSE)) {
        stack.count = (stack.count - MAREA_INTEGER_SIZE(1));

        const MAREA_IrNode *node = stack.frames[stack.count].node;
        const MAREA_IntegerSize depth = stack.frames[stack.count].depth;

        MAREA_G_DumpNodeLine(buffer, node, depth);

        // INFO: Children go on in reverse so that the first child comes off first and the order stays the same.
        for (MAREA_IntegerSize index = node->childCount; index > MAREA_INTEGER_SIZE(0); index = (index - MAREA_INTEGER_SIZE(1))) {
            MAREA_G_DumpPush(buffer, &stack, node->children[(index - MAREA_INTEGER_SIZE(1))], (depth + MAREA_INTEGER_SIZE(1)));
        }
    }
}

MAREA_IrExcept MAREA_CALL MAREA_IrModuleDump(MAREA_Arena *arena, const MAREA_IrNode *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (module == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_G_DumpBuffer buffer = { .arena = arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };

    MAREA_G_DumpReserve(&buffer, MAREA_INTEGER_SIZE(0));

    MAREA_G_DumpNode(&buffer, module);

    if (buffer.failed == MAREA_BOOLEAN_TRUE) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    buffer.data[buffer.length] = '\0';

    *outText = buffer.data;
    *outLength = buffer.length;

    return MAREA_IR_EXCEPT_SUCCESS;
}
