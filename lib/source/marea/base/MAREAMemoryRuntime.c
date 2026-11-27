// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMemoryRuntime.h"

#include "marea/base/MAREACMemoryRuntime.h"

// INFO: the alignment of the one block of an arena. A part that asks for more is aligned by its address.
#define MAREA_G_ARENA_ALIGNMENT MAREA_UINTEGER_64(16)

void MAREA_CALL MAREA_MemoryOwnedCreate(MAREA_HostContext *context, MAREA_MemoryOwnedHeader *owned, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment)
{
    const MAREA_Uinteger64 BLOCK = MAREA_CMemoryAllocate(context, size, alignment);

    if (BLOCK != MAREA_UINTEGER_64(0)) {
        owned->data = BLOCK;
    }
}

void MAREA_CALL MAREA_MemoryOwnedDestroy(MAREA_HostContext *context, MAREA_MemoryOwnedHeader *owned)
{
    if (owned->data != MAREA_UINTEGER_64(0)) {
        MAREA_CMemoryFree(context, owned->data);

        owned->data = MAREA_UINTEGER_64(0);
    }
}

MAREA_Uinteger64 MAREA_CALL MAREA_MemorySliceElement(const MAREA_MemorySliceHeader *slice, MAREA_Uinteger64 index, MAREA_Uinteger64 stride)
{
    return (index < slice->size) ? MAREA_CMemoryOffset(slice->data, (MAREA_Sinteger64)index, stride) : MAREA_UINTEGER_64(0);
}

void MAREA_CALL MAREA_MemoryArenaCreate(MAREA_HostContext *context, MAREA_MemoryArenaHeader *arena, MAREA_Uinteger64 capacity)
{
    const MAREA_Uinteger64 BLOCK = MAREA_CMemoryAllocate(context, capacity, MAREA_G_ARENA_ALIGNMENT);

    if (BLOCK != MAREA_UINTEGER_64(0)) {
        arena->data = BLOCK;
        arena->capacity = capacity;
        arena->used = MAREA_UINTEGER_64(0);
    }
}

MAREA_Uinteger64 MAREA_CALL MAREA_MemoryArenaAllocate(MAREA_HostContext *context, MAREA_MemoryArenaHeader *arena, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment)
{
    if ((arena->data != MAREA_UINTEGER_64(0)) && (alignment != MAREA_UINTEGER_64(0)) && ((alignment & (alignment - MAREA_UINTEGER_64(1))) == MAREA_UINTEGER_64(0))) {
        const MAREA_Uinteger64 NEXT = arena->data + arena->used;

        // INFO: the bytes up to the next multiple of a power of two, with unsigned wraparound.
        const MAREA_Uinteger64 PADDING = (MAREA_UINTEGER_64(0) - NEXT) & (alignment - MAREA_UINTEGER_64(1));

        const MAREA_Uinteger64 LEFT = arena->capacity - arena->used;

        // INFO: both tests subtract from what is left, so no sum can wrap.
        if ((PADDING <= LEFT) && (size <= (LEFT - PADDING))) {
            const MAREA_Uinteger64 ADDRESS = NEXT + PADDING;

            arena->used += PADDING + size;

            MAREA_CMemoryFillBytes(ADDRESS, MAREA_UINTEGER_8(0), size);

            return ADDRESS;
        }
    }

    MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

    return MAREA_UINTEGER_64(0);
}

void MAREA_CALL MAREA_MemoryArenaDestroy(MAREA_HostContext *context, MAREA_MemoryArenaHeader *arena)
{
    if (arena->data != MAREA_UINTEGER_64(0)) {
        MAREA_CMemoryFree(context, arena->data);

        arena->data = MAREA_UINTEGER_64(0);
        arena->capacity = MAREA_UINTEGER_64(0);
        arena->used = MAREA_UINTEGER_64(0);
    }
}
