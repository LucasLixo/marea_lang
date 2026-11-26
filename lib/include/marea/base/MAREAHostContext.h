// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAHOSTCONTEXT_H)
    #define MAREA_INCLUDE_BASE_MAREAHOSTCONTEXT_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief What the runtime gives a C function that asks for it: the allocator hooks of the runtime and the
     * trap of the call. A context lives for one call. The function must not keep it and the runtime makes a new
     * one for the next call, so it holds no global state.
     */
    typedef struct MAREA_HostContext MAREA_HostContext;

    /**
     * @brief The traps that a C function can raise. Each value is the value of the trap of the same name in every
     * backend (MAREA_BytecodeTrapCode in the vm).
     */
    typedef enum MAREA_HostTrap
    {
        MAREA_HOST_TRAP_NONE = MAREA_UINTEGER_8(0),
        MAREA_HOST_TRAP_OUT_OF_MEMORY = MAREA_UINTEGER_8(9),
    } MAREA_HostTrap;

    /**
     * @brief Result of an operation of a host context.
     */
    typedef enum MAREA_HostContextExcept
    {
        MAREA_HOST_CONTEXT_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(3),
        MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR = MAREA_UINTEGER_8(4),
    } MAREA_HostContextExcept;

    /**
     * @brief Asks the allocator of the runtime, the replacement of the hook "alloc" when a host gave one, for a
     * block filled with zeros. A failure raises no trap: the function decides, usually with
     * MAREA_HostContextRaiseTrap and MAREA_HOST_TRAP_OUT_OF_MEMORY.
     * @param context Context of the call. Must not be null.
     * @param size Number of bytes. 0 asks for 1 byte.
     * @param alignment Alignment of the block, a power of two.
     * @param outBlock Receives the block, or null when the call fails. Must not be null.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context or outBlock is null,
     * ERROR_OUT_OF_RANGE when alignment is not a power of two, ERROR_OUT_OF_MEMORY when the allocator gives no
     * block, or ERROR_UNPAIRED_ALLOCATOR when the host replaced only one of the two hooks (the runtime reports it
     * after the function returns).
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_HostContextAllocate(MAREA_HostContext * context, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null * outBlock);

    /**
     * @brief Gives a block back to the hook "free" of the runtime.
     * @param context Context of the call. Must not be null.
     * @param address Block that MAREA_HostContextAllocate or the alloc of the program gave, or null, which does
     * nothing.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or
     * ERROR_UNPAIRED_ALLOCATOR, the same as MAREA_HostContextAllocate.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_HostContextFree(MAREA_HostContext * context, MAREA_Null address);

    /**
     * @brief Raises a trap: the runtime stops the run when the function returns, and the result of the function
     * is dropped. The function must still return. The first trap of a call stays.
     * @param context Context of the call. Must not be null.
     * @param trap Trap to raise, a value of MAREA_HostTrap that is not NONE.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or
     * ERROR_OUT_OF_RANGE when trap is NONE or no value of MAREA_HostTrap.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_HostContextRaiseTrap(MAREA_HostContext * context, MAREA_HostTrap trap);

    /**
     * @brief Reads the trap that was raised in this call.
     * @param context Context to query. Null returns MAREA_HOST_TRAP_NONE.
     * @return The trap, or MAREA_HOST_TRAP_NONE when none was raised.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostTrap MAREA_CALL MAREA_HostContextGetTrap(const MAREA_HostContext *context);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAHOSTCONTEXT_H */
