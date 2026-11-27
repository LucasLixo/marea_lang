// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAVectorRuntime.h"

#include <stdint.h>
#include <string.h>

#define MAREA_G_BYTE_LIMIT      MAREA_UINTEGER_64(0x7FFFFFFFFFFFFFFF)
#define MAREA_G_SMALLEST_GROWTH MAREA_UINTEGER_64(4)

MAREA_STATIC_ASSERT(sizeof(MAREA_VectorHeader) == 24, "the header of a vector is 24 bytes");

static MAREA_Byte *MAREA_CALL MAREA_G_BytesOf(const MAREA_VectorHeader *header)
{
    return (MAREA_Byte *)(uintptr_t)header->data;
}

MAREA_Uinteger64 MAREA_CALL MAREA_VectorMaxSize(MAREA_Uinteger64 stride)
{
    return (stride == MAREA_UINTEGER_64(0)) ? MAREA_G_BYTE_LIMIT : (MAREA_G_BYTE_LIMIT / stride);
}

// INFO: the one place that changes the storage: a new block of capacity elements, the elements moved there, the old block given back.
static void MAREA_CALL MAREA_G_Move(MAREA_HostContext *context, MAREA_VectorHeader *header, MAREA_Uinteger64 capacity, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment)
{
    const MAREA_Uinteger64 BYTES = (capacity * stride);

    MAREA_Null block = MAREA_NULLPTR;

    if ((BYTES > (MAREA_Uinteger64)MAREA_INTEGER_SIZE_MAXIMUM) || (MAREA_HostContextAllocate(context, (MAREA_IntegerSize)BYTES, (MAREA_IntegerSize)alignment, &block) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS)) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

        return;
    }

    const MAREA_Uinteger64 USED = (header->size * stride);

    if (USED > MAREA_UINTEGER_64(0)) {
        memcpy(block, MAREA_G_BytesOf(header), (MAREA_IntegerSize)USED);
    }

    MAREA_HostContextFree(context, MAREA_G_BytesOf(header));

    header->data = (MAREA_Uinteger64)(uintptr_t)block;
    header->capacity = capacity;
}

void MAREA_CALL MAREA_VectorReserve(MAREA_HostContext *context, MAREA_VectorHeader *header, MAREA_Uinteger64 count, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment)
{
    if (count <= header->capacity) {
        return;
    }

    if (count > MAREA_VectorMaxSize(stride)) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

        return;
    }

    MAREA_G_Move(context, header, count, stride, alignment);
}

void MAREA_CALL MAREA_VectorGrow(MAREA_HostContext *context, MAREA_VectorHeader *header, MAREA_Uinteger64 needed, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment)
{
    if (needed <= header->capacity) {
        return;
    }

    const MAREA_Uinteger64 LIMIT = MAREA_VectorMaxSize(stride);

    // INFO: a capacity is at most 2^63 - 1, so twice a capacity does not wrap.
    MAREA_Uinteger64 grown = (header->capacity * MAREA_UINTEGER_64(2));

    grown = (grown < MAREA_G_SMALLEST_GROWTH) ? MAREA_G_SMALLEST_GROWTH : grown;
    grown = (grown > LIMIT) ? LIMIT : grown;
    grown = (grown < needed) ? needed : grown;

    MAREA_VectorReserve(context, header, grown, stride, alignment);
}

void MAREA_CALL MAREA_VectorOpen(MAREA_HostContext *context, MAREA_VectorHeader *header, MAREA_Uinteger64 index, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment)
{
    MAREA_VectorGrow(context, header, (header->size + MAREA_UINTEGER_64(1)), stride, alignment);

    if (header->capacity <= header->size) {
        return;
    }

    const MAREA_Uinteger64 TAIL = ((header->size - index) * stride);

    if (TAIL > MAREA_UINTEGER_64(0)) {
        MAREA_Byte *slot = (MAREA_G_BytesOf(header) + (index * stride));

        memmove((slot + stride), slot, (MAREA_IntegerSize)TAIL);
    }

    header->size += MAREA_UINTEGER_64(1);
}

void MAREA_CALL MAREA_VectorClose(MAREA_VectorHeader *header, MAREA_Uinteger64 index, MAREA_Uinteger64 stride)
{
    const MAREA_Uinteger64 TAIL = ((header->size - index - MAREA_UINTEGER_64(1)) * stride);

    if (TAIL > MAREA_UINTEGER_64(0)) {
        MAREA_Byte *slot = (MAREA_G_BytesOf(header) + (index * stride));

        memmove(slot, (slot + stride), (MAREA_IntegerSize)TAIL);
    }

    header->size -= MAREA_UINTEGER_64(1);
}

void MAREA_CALL MAREA_VectorShrink(MAREA_HostContext *context, MAREA_VectorHeader *header, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment)
{
    if (header->capacity == header->size) {
        return;
    }

    if (header->size == MAREA_UINTEGER_64(0)) {
        MAREA_VectorRelease(context, header);

        return;
    }

    MAREA_G_Move(context, header, header->size, stride, alignment);
}

void MAREA_CALL MAREA_VectorRelease(MAREA_HostContext *context, MAREA_VectorHeader *header)
{
    if (header->data != MAREA_UINTEGER_64(0)) {
        MAREA_HostContextFree(context, MAREA_G_BytesOf(header));
    }

    header->data = MAREA_UINTEGER_64(0);
    header->size = MAREA_UINTEGER_64(0);
    header->capacity = MAREA_UINTEGER_64(0);
}
