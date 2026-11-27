// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMemoryRuntime.h"

// TODO: not implemented yet.
void MAREA_CALL MAREA_MemoryOwnedCreate(MAREA_HostContext *context, MAREA_MemoryOwnedHeader *owned, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(owned);
    MAREA_IGNORE(size);
    MAREA_IGNORE(alignment);
}

// TODO: not implemented yet.
void MAREA_CALL MAREA_MemoryOwnedDestroy(MAREA_HostContext *context, MAREA_MemoryOwnedHeader *owned)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(owned);
}

// TODO: not implemented yet.
MAREA_Uinteger64 MAREA_CALL MAREA_MemorySliceElement(const MAREA_MemorySliceHeader *slice, MAREA_Uinteger64 index, MAREA_Uinteger64 stride)
{
    MAREA_IGNORE(slice);
    MAREA_IGNORE(index);
    MAREA_IGNORE(stride);

    return MAREA_UINTEGER_64(0);
}

// TODO: not implemented yet.
void MAREA_CALL MAREA_MemoryArenaCreate(MAREA_HostContext *context, MAREA_MemoryArenaHeader *arena, MAREA_Uinteger64 capacity)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(arena);
    MAREA_IGNORE(capacity);
}

// TODO: not implemented yet.
MAREA_Uinteger64 MAREA_CALL MAREA_MemoryArenaAllocate(MAREA_HostContext *context, MAREA_MemoryArenaHeader *arena, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(arena);
    MAREA_IGNORE(size);
    MAREA_IGNORE(alignment);

    return MAREA_UINTEGER_64(0);
}

// TODO: not implemented yet.
void MAREA_CALL MAREA_MemoryArenaDestroy(MAREA_HostContext *context, MAREA_MemoryArenaHeader *arena)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(arena);
}
