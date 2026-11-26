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

void MAREA_CALL MAREA_CgenWriteTexts(MAREA_CgenBuffer *buffer, const MAREA_Character8 *const *parts, MAREA_IntegerSize count)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (parts[index] != MAREA_NULLPTR) {
            MAREA_CgenWriteText(buffer, parts[index]);
        }
    }
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenFinish(MAREA_CgenBuffer *buffer)
{
    MAREA_CgenBufferReserve(buffer, MAREA_INTEGER_SIZE(0));

    if (buffer->failed == MAREA_BOOLEAN_TRUE) {
        return "";
    }

    buffer->data[buffer->length] = '\0';

    return buffer->data;
}

// INFO: a text of the arena, with room for its NUL. A failed allocation marks the generator, whose run then fails.
static MAREA_Character8 *MAREA_CALL MAREA_G_NewText(MAREA_CgenGenerator *generator, MAREA_IntegerSize length)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, (length + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        generator->outOfMemory = MAREA_BOOLEAN_TRUE;

        return MAREA_NULLPTR;
    }

    MAREA_Character8 *text = memory;

    text[length] = '\0';

    return text;
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenJoin(MAREA_CgenGenerator *generator, const MAREA_Character8 *const *parts, MAREA_IntegerSize count)
{
    MAREA_IntegerSize total = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (parts[index] != MAREA_NULLPTR) {
            total = (total + (MAREA_IntegerSize)strlen(parts[index]));
        }
    }

    MAREA_Character8 *text = MAREA_G_NewText(generator, total);

    if (text == MAREA_NULLPTR) {
        return "";
    }

    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (parts[index] != MAREA_NULLPTR) {
            const MAREA_IntegerSize LENGTH = (MAREA_IntegerSize)strlen(parts[index]);

            memcpy((text + used), parts[index], LENGTH);

            used = (used + LENGTH);
        }
    }

    return text;
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenCopyText(MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_Character8 *copy = MAREA_G_NewText(generator, length);

    if (copy == MAREA_NULLPTR) {
        return "";
    }

    if (length > MAREA_INTEGER_SIZE(0)) {
        memcpy(copy, text, length);
    }

    return copy;
}

// INFO: 20 digits hold the largest 64-bit value.
static MAREA_IntegerSize MAREA_CALL MAREA_G_Digits(MAREA_Uinteger64 value, MAREA_Character8 outDigits[20])
{
    MAREA_Character8 reversed[20];

    MAREA_IntegerSize count = MAREA_INTEGER_SIZE(0);

    do {
        reversed[count] = (MAREA_Character8)('0' + (value % MAREA_UINTEGER_64(10)));
        count = (count + MAREA_INTEGER_SIZE(1));
        value = (value / MAREA_UINTEGER_64(10));
    } while (value != MAREA_UINTEGER_64(0));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        outDigits[index] = reversed[(count - MAREA_INTEGER_SIZE(1) - index)];
    }

    return count;
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenNumber(MAREA_CgenGenerator *generator, MAREA_Uinteger64 value)
{
    MAREA_Character8 digits[20];

    const MAREA_IntegerSize COUNT = MAREA_G_Digits(value, digits);

    return MAREA_CgenCopyText(generator, digits, COUNT);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNameByte(MAREA_Character8 byte)
{
    return (((byte >= 'a') && (byte <= 'z')) || ((byte >= 'A') && (byte <= 'Z')) || ((byte >= '0') && (byte <= '9')) || (byte == '_'));
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenMangled(MAREA_CgenGenerator *generator, const MAREA_Character8 *prefix, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize PREFIX = (MAREA_IntegerSize)strlen(prefix);

    MAREA_Character8 *name = MAREA_G_NewText(generator, (PREFIX + length));

    if (name == MAREA_NULLPTR) {
        return "";
    }

    memcpy(name, prefix, PREFIX);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < length; index = (index + MAREA_INTEGER_SIZE(1))) {
        name[(PREFIX + index)] = (MAREA_G_IsNameByte(text[index]) == MAREA_BOOLEAN_TRUE) ? text[index] : '_';
    }

    return name;
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenLocalName(MAREA_CgenGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((length > MAREA_INTEGER_SIZE(0)) && (text[0] == '$')) {
        return MAREA_CgenMangled(generator, "h_", (text + 1), (length - MAREA_INTEGER_SIZE(1)));
    }

    return MAREA_CgenMangled(generator, "v_", text, length);
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenTemporaryName(MAREA_CgenGenerator *generator)
{
    const MAREA_Character8 *name = MAREA_CGEN_JOIN(generator, "marea_t", MAREA_CgenNumber(generator, generator->temporaryCount));

    generator->temporaryCount = (generator->temporaryCount + MAREA_UINTEGER_32(1));

    return name;
}
