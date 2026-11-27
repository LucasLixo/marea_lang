// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAVECTORRUNTIME_H)
    #define MAREA_INCLUDE_BASE_MAREAVECTORRUNTIME_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The header of a value of marea.vector.Vector: the address of the element storage, the number of
     * elements and the number of elements that fit in the storage. A header of zeros is an empty vector with no
     * storage. The struct is open because a generated C program and the vm read and write these three words in
     * place. The size is 24 bytes and the alignment is 8 on every target, so the address is a 64-bit integer.
     *
     * Every function below takes the stride and the alignment of one element. The storage holds capacity * stride
     * bytes. An element of no size has the stride 0. The functions move bytes only: the caller builds and
     * destroys the elements. A request that cannot be met raises MAREA_HOST_TRAP_OUT_OF_MEMORY on the context
     * and leaves the header unchanged. These are the one growth rule and the one allocation path of both
     * backends.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_VectorHeader
    {
        MAREA_Uinteger64 data;
        MAREA_Uinteger64 size;
        MAREA_Uinteger64 capacity;
    } MAREA_VectorHeader;

    /**
     * @brief The largest number of elements that a vector of the given stride can hold: 0x7FFFFFFFFFFFFFFF bytes
     * over the stride, and 0x7FFFFFFFFFFFFFFF for the stride 0.
     * @param stride Number of bytes of one element.
     * @return The number of elements.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_VectorMaxSize(MAREA_Uinteger64 stride);

    /**
     * @brief Makes the capacity at least count. A larger capacity is exactly count; a smaller or equal count does
     * nothing.
     * @param context Context of the run, with the allocator hooks. Must not be null.
     * @param header Vector to change. Must not be null.
     * @param count Number of elements that must fit.
     * @param stride Number of bytes of one element.
     * @param alignment Alignment of one element, a power of two.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_VectorReserve(MAREA_HostContext * context, MAREA_VectorHeader * header, MAREA_Uinteger64 count, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment);

    /**
     * @brief The growth rule: when needed elements do not fit, the new capacity is the larger of 4 and twice the
     * capacity, or needed when that is larger still.
     * @param context Context of the run. Must not be null.
     * @param header Vector to change. Must not be null.
     * @param needed Number of elements that must fit.
     * @param stride Number of bytes of one element.
     * @param alignment Alignment of one element, a power of two.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_VectorGrow(MAREA_HostContext * context, MAREA_VectorHeader * header, MAREA_Uinteger64 needed, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment);

    /**
     * @brief Opens a gap for one element at index: grows by the rule when the vector is full, moves the tail up by
     * one and adds 1 to the size. The caller then writes the element at index.
     * @param context Context of the run. Must not be null.
     * @param header Vector to change. Must not be null.
     * @param index Position of the gap, not above the size.
     * @param stride Number of bytes of one element.
     * @param alignment Alignment of one element, a power of two.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_VectorOpen(MAREA_HostContext * context, MAREA_VectorHeader * header, MAREA_Uinteger64 index, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment);

    /**
     * @brief Removes the slot at index: moves the tail down by one and subtracts 1 from the size. The caller took
     * or destroyed the element first. The capacity stays.
     * @param header Vector to change. Must not be null.
     * @param index Position of the slot, below the size.
     * @param stride Number of bytes of one element.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_VectorClose(MAREA_VectorHeader * header, MAREA_Uinteger64 index, MAREA_Uinteger64 stride);

    /**
     * @brief Makes the capacity equal to the size. An empty vector gives its storage back.
     * @param context Context of the run. Must not be null.
     * @param header Vector to change. Must not be null.
     * @param stride Number of bytes of one element.
     * @param alignment Alignment of one element, a power of two.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_VectorShrink(MAREA_HostContext * context, MAREA_VectorHeader * header, MAREA_Uinteger64 stride, MAREA_Uinteger64 alignment);

    /**
     * @brief Gives the storage back and makes the header empty. A header with no storage stays as it is, so a
     * second release does nothing. The caller destroyed the elements first.
     * @param context Context of the run. Must not be null.
     * @param header Vector to change. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_VectorRelease(MAREA_HostContext * context, MAREA_VectorHeader * header);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAVECTORRUNTIME_H */
