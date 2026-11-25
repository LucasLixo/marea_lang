// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

#if !defined(MAREA_CGEN_BUFFER_INITIAL_CAPACITY)
    #define MAREA_CGEN_BUFFER_INITIAL_CAPACITY MAREA_INTEGER_SIZE(4096)
#endif

void MAREA_CALL MAREA_CgenBufferReserve(MAREA_CgenBuffer *buffer, MAREA_IntegerSize extra)
{
    if (buffer->failed == MAREA_BOOLEAN_TRUE) {
        return;
    }

    // INFO: One extra byte is always kept free for the final NUL.
    if ((buffer->length + extra + MAREA_INTEGER_SIZE(1)) <= buffer->capacity) {
        return;
    }

    MAREA_IntegerSize newCapacity = (buffer->capacity == MAREA_INTEGER_SIZE(0)) ? MAREA_CGEN_BUFFER_INITIAL_CAPACITY : (buffer->capacity * MAREA_INTEGER_SIZE(2));

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

void MAREA_CALL MAREA_CgenWrite(MAREA_CgenBuffer *buffer, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_CgenBufferReserve(buffer, length);

    if (buffer->failed == MAREA_BOOLEAN_TRUE) {
        return;
    }

    memcpy((buffer->data + buffer->length), text, length);

    buffer->length = (buffer->length + length);
}

void MAREA_CALL MAREA_CgenWriteText(MAREA_CgenBuffer *buffer, const MAREA_Character8 *text)
{
    MAREA_CgenWrite(buffer, text, (MAREA_IntegerSize)strlen(text));
}

// INFO: 20 digits hold the largest 64-bit value.
void MAREA_CALL MAREA_CgenWriteNumber(MAREA_CgenBuffer *buffer, MAREA_Uinteger64 value)
{
    MAREA_Character8 digits[20];

    MAREA_IntegerSize count = MAREA_INTEGER_SIZE(0);

    do {
        digits[count] = (MAREA_Character8)('0' + (value % MAREA_UINTEGER_64(10)));
        count = (count + MAREA_INTEGER_SIZE(1));
        value = (value / MAREA_UINTEGER_64(10));
    } while (value != MAREA_UINTEGER_64(0));

    while (count > MAREA_INTEGER_SIZE(0)) {
        count = (count - MAREA_INTEGER_SIZE(1));

        MAREA_CgenWrite(buffer, &digits[count], MAREA_INTEGER_SIZE(1));
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNameByte(MAREA_Character8 byte)
{
    return (((byte >= 'a') && (byte <= 'z')) || ((byte >= 'A') && (byte <= 'Z')) || ((byte >= '0') && (byte <= '9')) || (byte == '_'));
}

void MAREA_CALL MAREA_CgenWriteMangled(MAREA_CgenBuffer *buffer, const MAREA_Character8 *prefix, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_CgenWriteText(buffer, prefix);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < length; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Character8 BYTE = (MAREA_G_IsNameByte(text[index]) == MAREA_BOOLEAN_TRUE) ? text[index] : '_';

        MAREA_CgenWrite(buffer, &BYTE, MAREA_INTEGER_SIZE(1));
    }
}
