// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAHOSTCONTEXT_H)
    #define MAREA_INCLUDE_BASE_MAREAHOSTCONTEXT_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The shape of a replacement of the runtime hook "alloc". It gives a block of at least size bytes
     * whose address is a multiple of alignment (a power of two), or null when it cannot. The runtime fills the
     * block with zeros itself. size is at least 1.
     */
    typedef MAREA_Null(MAREA_CALL * MAREA_RuntimeAllocateFunction)(MAREA_IntegerSize size, MAREA_IntegerSize alignment);

    /**
     * @brief The shape of a replacement of the runtime hook "free". address is a block that the replacement of
     * "alloc" gave and that is not free yet, never null.
     */
    typedef void(MAREA_CALL * MAREA_RuntimeFreeFunction)(MAREA_Null address);

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
     * @brief What the runtime gives a C function that asks for it: the allocator hooks of the runtime and the
     * trap of the call. A context lives for one call. The function must not keep it and the runtime makes a new
     * one for the next call, so it holds no global state. The struct is open because a program that cgen writes
     * makes one on its stack for each call of a function that asks for it; MAREA_HostContextInitialize fills it.
     * A C function reads and changes it only through the functions below.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_HostContext
    {
        MAREA_RuntimeAllocateFunction allocate;
        MAREA_RuntimeFreeFunction release;
        MAREA_HostTrap trap;
        MAREA_Boolean unpaired;
        MAREA_Uinteger8 padding[3];
    } MAREA_HostContext;

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
     * @brief Makes the context of one call: no trap, no fault and the two hooks of the caller, or, when both are
     * null, the allocator of this library (malloc and free of the C library with the requested alignment). The
     * hooks come from one allocator, so a caller gives both or neither.
     * @param context Context to fill. Must not be null.
     * @param allocate Replacement of "alloc", or null together with release.
     * @param release Replacement of "free", or null together with allocate.
     * @return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS, ERROR_NULL_POINTER when context is null, or
     * ERROR_UNPAIRED_ALLOCATOR when exactly one hook is null (context is then not changed).
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostContextExcept MAREA_CALL MAREA_HostContextInitialize(MAREA_HostContext * context, MAREA_RuntimeAllocateFunction allocate, MAREA_RuntimeFreeFunction release);

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
     * @brief Gives a block back to the hook "free" of the runtime. With only one hook replaced nothing is freed
     * and the runtime reports the fault after the function returns, as for MAREA_HostContextAllocate.
     * @param context Context of the call. Null does nothing.
     * @param address Block that MAREA_HostContextAllocate or the alloc of the program gave, or null, which does
     * nothing.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_HostContextFree(MAREA_HostContext * context, MAREA_Null address);

    /**
     * @brief Raises a trap: the runtime stops the run when the function returns, and the result of the function
     * is dropped. The function must still return. The first trap of a call stays.
     * @param context Context of the call. Null does nothing.
     * @param trap Trap to raise. MAREA_HOST_TRAP_NONE and a value that is no member of MAREA_HostTrap do nothing.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_HostContextRaiseTrap(MAREA_HostContext * context, MAREA_HostTrap trap);

    /**
     * @brief Reads the trap that was raised in this call.
     * @param context Context to query. Null returns MAREA_HOST_TRAP_NONE.
     * @return The trap, or MAREA_HOST_TRAP_NONE when none was raised.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostTrap MAREA_CALL MAREA_HostContextGetTrap(const MAREA_HostContext *context);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAHOSTCONTEXT_H */
