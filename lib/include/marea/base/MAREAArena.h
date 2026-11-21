// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAARENA_H)
    #define MAREA_INCLUDE_BASE_MAREAARENA_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Opaque bump-pointer arena allocator with a growing list of chunks. The caller owns the
     * arena; every allocation it hands out stays valid until MAREA_ArenaDestroy is called.
     */
    typedef struct MAREA_Arena MAREA_Arena;

    /**
     * @brief Result of a MAREA_Arena operation.
     */
    typedef enum MAREA_ArenaExcept
    {
        MAREA_ARENA_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_ARENA_EXCEPT_ERROR_INVALID_ARGUMENT = MAREA_UINTEGER_8(3),
    } MAREA_ArenaExcept;

    /**
     * @brief Creates an arena that allocates its chunks in units of chunkSize bytes.
     * @param chunkSize Byte size of each backing chunk. 0 means 65536 bytes.
     * @param outArena Receives the created arena. Must not be null.
     * @return MAREA_ARENA_EXCEPT_SUCCESS, MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER when outArena is null,
     * or MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY when the backing allocation fails.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_ArenaExcept MAREA_CALL MAREA_ArenaCreate(MAREA_IntegerSize chunkSize, MAREA_Arena * *outArena);

    /**
     * @brief Destroys an arena and every chunk it owns, invalidating every pointer it ever handed out.
     * @param arena Arena to destroy. Null is accepted and is a no-op.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_ArenaDestroy(MAREA_Arena * arena);

    /**
     * @brief Allocates size bytes aligned to alignment from the arena, taking a new chunk when the
     * current one has no room left.
     * @param arena Arena to allocate from. Must not be null.
     * @param size Number of bytes requested. Must not be 0.
     * @param alignment Required alignment, must be a power of two.
     * @param outMemory Receives the allocated block. Must not be null.
     * @return MAREA_ARENA_EXCEPT_SUCCESS, MAREA_ARENA_EXCEPT_ERROR_NULL_POINTER when arena or outMemory
     * is null, MAREA_ARENA_EXCEPT_ERROR_INVALID_ARGUMENT when size is 0 or alignment is not a power of
     * two, or MAREA_ARENA_EXCEPT_ERROR_OUT_OF_MEMORY when a new chunk cannot be allocated.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_ArenaExcept MAREA_CALL MAREA_ArenaAllocate(MAREA_Arena * arena, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null * outMemory);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAARENA_H */
