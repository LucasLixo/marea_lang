// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMEMORYRUNTIME_H)
    #define MAREA_INCLUDE_BASE_MAREAMEMORYRUNTIME_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The value of marea.memory.Owned: the address of the one block that it owns. A header of zeros owns
     * nothing, which is also what a move leaves behind. Open, as MAREA_VectorHeader is, because a backend reads
     * and writes the words in place.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_MemoryOwnedHeader
    {
        MAREA_Uinteger64 data;
    } MAREA_MemoryOwnedHeader;

    /**
     * @brief The value of marea.memory.Slice: a borrow of size elements that start at the address data. It owns
     * nothing and it trusts the two words that it holds.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_MemorySliceHeader
    {
        MAREA_Uinteger64 data;
        MAREA_Uinteger64 size;
    } MAREA_MemorySliceHeader;

    /**
     * @brief The value of marea.memory.Arena: one block of capacity bytes, of which the first used bytes are
     * taken. A header of zeros holds no block.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_MemoryArenaHeader
    {
        MAREA_Uinteger64 data;
        MAREA_Uinteger64 capacity;
        MAREA_Uinteger64 used;
    } MAREA_MemoryArenaHeader;

    /**
     * @brief Construct of an owner: takes a new block filled with zeros.
     * @param context Context of the call. Must not be null.
     * @param owned Header that owns nothing. Must not be null.
     * @param size Number of bytes. A size of 0 asks for 1 byte.
     * @param alignment Alignment of the block, a power of two.
     * When no block can be given the function raises MAREA_HOST_TRAP_OUT_OF_MEMORY and leaves the header
     * unchanged.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MemoryOwnedCreate(MAREA_HostContext * context, MAREA_MemoryOwnedHeader * owned, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment);

    /**
     * @brief Destroy of an owner: gives the block back and leaves zeros, so a second destroy does nothing. The
     * caller destroyed the value in the block first.
     * @param context Context of the call. Must not be null.
     * @param owned Header to change. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MemoryOwnedDestroy(MAREA_HostContext * context, MAREA_MemoryOwnedHeader * owned);

    /**
     * @brief The index protocol of a view: the address of one element.
     * @param slice View to read. Must not be null.
     * @param index Position of the element.
     * @param stride Number of bytes of one element.
     * @return data + index * stride, or 0 when index is not below the size. A backend makes the 0 the trap of an
     * index out of range before it uses the address.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_MemorySliceElement(const MAREA_MemorySliceHeader *slice, MAREA_Uinteger64 index, MAREA_Uinteger64 stride);

    /**
     * @brief Construct of an arena: takes the one block of capacity bytes.
     * @param context Context of the call. Must not be null.
     * @param arena Header that holds no block. Must not be null.
     * @param capacity Number of bytes. A capacity of 0 asks for 1 byte.
     * When no block can be given the function raises MAREA_HOST_TRAP_OUT_OF_MEMORY and leaves the header
     * unchanged.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MemoryArenaCreate(MAREA_HostContext * context, MAREA_MemoryArenaHeader * arena, MAREA_Uinteger64 capacity);

    /**
     * @brief Takes the next bytes of the block, filled with zeros, at an address that is a multiple of
     * alignment. The arena never grows and never gives back one part.
     * @param context Context of the call. Must not be null.
     * @param arena Arena to take from. Must not be null.
     * @param size Number of bytes.
     * @param alignment A power of two.
     * @return The address. When the bytes do not fit, the arena holds no block or alignment is not a power of
     * two, the function raises MAREA_HOST_TRAP_OUT_OF_MEMORY, returns 0 and leaves the header unchanged.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_MemoryArenaAllocate(MAREA_HostContext * context, MAREA_MemoryArenaHeader * arena, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment);

    /**
     * @brief Destroy of an arena: gives the one block back at once and leaves zeros, so a second destroy does
     * nothing.
     * @param context Context of the call. Must not be null.
     * @param arena Arena to change. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MemoryArenaDestroy(MAREA_HostContext * context, MAREA_MemoryArenaHeader * arena);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMEMORYRUNTIME_H */
