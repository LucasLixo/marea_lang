// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACMEMORYRUNTIME_H)
    #define MAREA_INCLUDE_BASE_MAREACMEMORYRUNTIME_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The runtime of c.memory: raw memory, with an address as a 64-bit integer. The address 0 is the null
     * address. Every function that reads or writes through an address trusts it: a wrong address is undefined.
     * Allocation uses the hooks of the context, the same allocation path as the other libraries of the language.
     */

    /**
     * @brief Gives a new block filled with zeros. A size of 0 asks for 1 byte.
     * @param context Context of the call, with the allocator hooks. Must not be null.
     * @param size Number of bytes.
     * @param alignment Alignment of the block, a power of two.
     * @return The address of the block. When no block can be given, or alignment is not a power of two, the
     * function raises MAREA_HOST_TRAP_OUT_OF_MEMORY on the context and returns 0.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_CMemoryAllocate(MAREA_HostContext * context, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment);

    /**
     * @brief Gives a block back. The address 0 does nothing.
     * @param context Context of the call. Must not be null.
     * @param address Address that MAREA_CMemoryAllocate gave and that is not free yet, or 0.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CMemoryFree(MAREA_HostContext * context, MAREA_Uinteger64 address);

    /**
     * @brief Copies bytes with no type: the raw read and the raw write. The two ranges can overlap.
     * @param destination Address to write. Not read when count is 0.
     * @param source Address to read. Not read when count is 0.
     * @param count Number of bytes.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CMemoryCopyBytes(MAREA_Uinteger64 destination, MAREA_Uinteger64 source, MAREA_Uinteger64 count);

    /**
     * @brief Writes one value to each byte of a range.
     * @param destination Address to write. Not read when count is 0.
     * @param value Value of each byte.
     * @param count Number of bytes.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CMemoryFillBytes(MAREA_Uinteger64 destination, MAREA_Uinteger8 value, MAREA_Uinteger64 count);

    /**
     * @brief Pointer arithmetic: address + count * stride, which wraps in two's complement as plain integer
     * arithmetic does. The function reads no memory.
     * @param address Start address.
     * @param count Number of elements, negative to go back.
     * @param stride Number of bytes of one element.
     * @return The new address.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_CMemoryOffset(MAREA_Uinteger64 address, MAREA_Sinteger64 count, MAREA_Uinteger64 stride);

    /**
     * @brief The null check.
     * @param address Address to check.
     * @return True when the address is 0.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CMemoryIsNull(MAREA_Uinteger64 address);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREACMEMORYRUNTIME_H */
