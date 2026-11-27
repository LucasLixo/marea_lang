// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACSTRING_H)
    #define MAREA_INCLUDE_BASE_MAREACSTRING_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A growable UTF-8 byte buffer: the runtime layout of the language's string primitive. The
     * struct is open for the same reason MAREA_HostContext is open - a host-registered C function that
     * borrows a string needs the concrete layout to read and write it directly. A zero-initialized value
     * ({ 0 }) is the valid empty string: data is null, lengthInBytes and capacityInBytes are both zero.
     * The byte buffer belongs to exactly one MAREA_String at a time at the C level; a memcpy of the
     * struct never aliases another string's storage once either is mutated.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_String
    {
        MAREA_Character8 *data;
        MAREA_IntegerSize lengthInBytes;
        MAREA_IntegerSize capacityInBytes;
    } MAREA_String;

    /**
     * @brief Counts the scalar values of value: every byte that is not a UTF-8 continuation byte
     * (10xxxxxx), not the raw byte count.
     * @param value String to count. Must not be null.
     * @return The scalar-value count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger64 MAREA_CALL MAREA_CStringLength(const MAREA_String *value);

    /**
     * @brief Appends the bytes of suffix to target, growing target's storage first when it has no room.
     * Growth doubles the capacity (at least to 4 bytes) until target's bytes plus suffix's bytes fit,
     * through MAREA_HostContextAllocate, a copy of the live bytes, and MAREA_HostContextFree of the old
     * block. On a failed grow, raises MAREA_HOST_TRAP_OUT_OF_MEMORY on context and leaves target exactly
     * as it was: no partial write.
     * @param context Context of the call, for the allocator and the trap. Must not be null.
     * @param target String to grow and append to. Must not be null and must not alias suffix.
     * @param suffix String whose bytes are appended. Must not be null and must not alias target.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CStringAppend(MAREA_HostContext * context, MAREA_String * target, const MAREA_String *suffix);

    /**
     * @brief Sets target's length to zero. Does not touch target's capacity and does not free its block.
     * @param target String to clear. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_CStringClear(MAREA_String * target);

    /**
     * @brief Compares left and right by byte order. A string that is a prefix of a longer one compares
     * less than it.
     * @param left First string. Must not be null.
     * @param right Second string. Must not be null.
     * @return A negative value when left sorts before right, zero when they hold the same bytes, or a
     * positive value when left sorts after right.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Sinteger32 MAREA_CALL MAREA_CStringCompare(const MAREA_String *left, const MAREA_String *right);

    /**
     * @brief Registers the library c.string: length, append, clear and compare above.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the c.string library is off in the mask of
     * the registry, which then holds none of its functions; otherwise the result of the registry
     * operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_CStringRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREACSTRING_H */
