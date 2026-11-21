// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAArena.h"

#include <stdint.h>
#include <stdlib.h>

#if !defined(MAREA_ARENA_DEFAULT_CHUNK_SIZE)
    #define MAREA_ARENA_DEFAULT_CHUNK_SIZE MAREA_INTEGER_SIZE(65536)
#endif

typedef struct MAREA_G_ArenaChunk
{
    struct MAREA_G_ArenaChunk *next;
    MAREA_Uinteger8 *buffer;
    MAREA_IntegerSize capacity;
    MAREA_IntegerSize used;
} MAREA_G_ArenaChunk;

typedef struct MAREA_Arena
{
    MAREA_G_ArenaChunk *head;
    MAREA_IntegerSize chunkSize;
} MAREA_Arena;

static MAREA_Boolean MAREA_CALL MAREA_G_ArenaIsPowerOfTwo(MAREA_IntegerSize value)
{
    return (value != MAREA_INTEGER_SIZE(0)) && ((value & (value - MAREA_INTEGER_SIZE(1))) == MAREA_INTEGER_SIZE(0));
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_ArenaAlignedOffset(const MAREA_G_ArenaChunk *chunk, MAREA_IntegerSize alignment)
{
    const uintptr_t base = (uintptr_t)chunk->buffer;
    const uintptr_t aligned = ((base + chunk->used) + (alignment - MAREA_INTEGER_SIZE(1))) & ~((uintptr_t)alignment - MAREA_INTEGER_SIZE(1));

    return (MAREA_IntegerSize)(aligned - base);
}

static MAREA_G_ArenaChunk *MAREA_CALL MAREA_G_ArenaChunkCreate(MAREA_IntegerSize capacity)
{
    MAREA_G_ArenaChunk *chunk = malloc(sizeof(MAREA_G_ArenaChunk));

    if (chunk == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    chunk->buffer = malloc(capacity);

    if (chunk->buffer == MAREA_NULLPTR) {
        free(chunk);

        return MAREA_NULLPTR;
    }

    chunk->next = MAREA_NULLPTR;
    chunk->capacity = capacity;
    chunk->used = MAREA_INTEGER_SIZE(0);

    return chunk;
}

MAREA_ArenaExcept MAREA_CALL MAREA_ArenaCreate(MAREA_IntegerSize chunkSize, MAREA_Arena **outArena)
{
    if (outArena == MAREA_NULLPTR) {
        return MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize resolvedChunkSize = (chunkSize == MAREA_INTEGER_SIZE(0)) ? MAREA_ARENA_DEFAULT_CHUNK_SIZE : chunkSize;

    MAREA_Arena *arena = malloc(sizeof(MAREA_Arena));

    if (arena == MAREA_NULLPTR) {
        return MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    arena->head = MAREA_G_ArenaChunkCreate(resolvedChunkSize);

    if (arena->head == MAREA_NULLPTR) {
        free(arena);

        return MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    arena->chunkSize = resolvedChunkSize;

    *outArena = arena;

    return MAREA_ARENA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_ArenaDestroy(MAREA_Arena *arena)
{
    if (arena == MAREA_NULLPTR) {
        return;
    }

    MAREA_G_ArenaChunk *chunk = arena->head;

    while (chunk != MAREA_NULLPTR) {
        MAREA_G_ArenaChunk *next = chunk->next;

        free(chunk->buffer);
        free(chunk);

        chunk = next;
    }

    free(arena);
}

MAREA_ArenaExcept MAREA_CALL MAREA_ArenaAllocate(MAREA_Arena *arena, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null *outMemory)
{
    if ((arena == MAREA_NULLPTR) || (outMemory == MAREA_NULLPTR)) {
        return MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((size == MAREA_INTEGER_SIZE(0)) || (!MAREA_G_ArenaIsPowerOfTwo(alignment))) {
        return MAREA_ARENA_EXCEPT_ERROR_INVALID_ARGUMENT;
    }

    MAREA_G_ArenaChunk *head = arena->head;

    const MAREA_IntegerSize offset = MAREA_G_ArenaAlignedOffset(head, alignment);

    if ((offset <= head->capacity) && ((head->capacity - offset) >= size)) {
        head->used = offset + size;

        *outMemory = (head->buffer + offset);

        return MAREA_ARENA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize baseCapacity = (size > arena->chunkSize) ? size : arena->chunkSize;

    if (baseCapacity > (MAREA_INTEGER_SIZE_MAXIMUM - alignment)) {
        return MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_G_ArenaChunk *newChunk = MAREA_G_ArenaChunkCreate(baseCapacity + alignment);

    if (newChunk == MAREA_NULLPTR) {
        return MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    newChunk->next = arena->head;
    arena->head = newChunk;

    const MAREA_IntegerSize newOffset = MAREA_G_ArenaAlignedOffset(newChunk, alignment);

    newChunk->used = newOffset + size;

    *outMemory = (newChunk->buffer + newOffset);

    return MAREA_ARENA_EXCEPT_SUCCESS;
}
