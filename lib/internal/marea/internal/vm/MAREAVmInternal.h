// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_VM_MAREAVMINTERNAL_H)
    #define MAREA_INTERNAL_VM_MAREAVMINTERNAL_H

    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREAHostFunction.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/bytecode/MAREABytecode.h"
    #include "marea/vm/MAREAVm.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Largest number of registers one frame keeps on the native stack, and the largest call depth, so a
     * recursive native run of the interpreter stays inside an ordinary thread stack. A function that needs more
     * registers, or a call chain that goes deeper, is a vm limit, not a language trap: MAREA_VmCall reports it as
     * MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE. A chain that only overflows by depth, within the register limit, is the
     * language's own MAREA_BYTECODE_TRAP_CODE_STACK_OVERFLOW instead, a defined result.
     */
    #if !defined(MAREA_VM_FRAME_REGISTER_LIMIT)
        #define MAREA_VM_FRAME_REGISTER_LIMIT MAREA_UINTEGER_32(256)
    #endif

    #if !defined(MAREA_VM_CALL_DEPTH_LIMIT)
        #define MAREA_VM_CALL_DEPTH_LIMIT MAREA_UINTEGER_32(512)
    #endif

    /**
     * @brief Largest number of bytes one frame keeps in its own memory area, the storage of a structure, an enum,
     * an array or a tuple that a local of the function owns. MAREA_BytecodeGeneratorAllocateFrame refuses a
     * function whose locals need more than this, so a run never sees an address past the end of it. The area sits
     * on the native stack beside the register file, so a pointer into it stays valid for as long as this frame is
     * on the call chain, including every call it makes.
     */
    #if !defined(MAREA_VM_FRAME_MEMORY_LIMIT)
        #define MAREA_VM_FRAME_MEMORY_LIMIT MAREA_UINTEGER_32(4096)
    #endif

    /**
     * @brief Alignment of the first byte of the frame memory area. The generator aligns every offset inside the
     * area to the alignment of its type, so a type with an alignment up to this one is aligned in memory too.
     */
    #if !defined(MAREA_VM_FRAME_MEMORY_ALIGNMENT)
        #define MAREA_VM_FRAME_MEMORY_ALIGNMENT 16
    #endif

    /**
     * @brief Largest number of parameters of a host function, the limit of the binding macro. A call of an import
     * with more is a vm limit.
     */
    #if !defined(MAREA_VM_HOST_PARAMETER_LIMIT)
        #define MAREA_VM_HOST_PARAMETER_LIMIT MAREA_UINTEGER_32(MAREA_HOST_FUNCTION_PARAMETER_LIMIT)
    #endif

    /**
     * @brief The context every vm call reads: the arena and the sink it was created with, and the registry of the
     * host, or null.
     */
    typedef struct MAREA_VmContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_TypeRegistry *registry;
    } MAREA_VmContext;

    /**
     * @brief Runs a call of an import: finds the C function in the registry of the context and calls it.
     * @param context State of the call. Must not be null.
     * @param module Module that holds the import. Must not be null.
     * @param functionIndex Index of the import.
     * @param info The import itself. Must not be null.
     * @param window The call window: window[0] takes a scalar result, or holds the address of a structure result,
     * and window[1] up are the arguments. Must not be null and must hold 1 + argumentCount registers.
     * @param argumentCount Number of arguments.
     * @param outResult Receives a trap, or an untrapped result. Must not be null.
     * @return MAREA_VM_EXCEPT_SUCCESS, or MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE for a vm limit or a malformed module.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmCallHost(MAREA_VmContext * context, const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_BytecodeFunctionInfo *info, MAREA_Uinteger64 *window, MAREA_Uinteger32 argumentCount, MAREA_VmResult *outResult);

    /**
     * @brief Runs the alloc opcode with the allocator of the host, or with the one of the vm.
     * @param context State of the call. Must not be null.
     * @param size Number of bytes. 0 asks for one byte.
     * @param alignment Alignment, a power of two.
     * @param outAddress Receives the register bits of the address of the zeroed block. Must not be null.
     * @param outResult Receives the trap OUT_OF_MEMORY when no block is given. Must not be null.
     * @return MAREA_VM_EXCEPT_SUCCESS, MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE for an alignment that is not a power of
     * two, or MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmAllocate(const MAREA_VmContext *context, MAREA_Uinteger64 size, MAREA_Uinteger32 alignment, MAREA_Uinteger64 *outAddress, MAREA_VmResult *outResult);

    /**
     * @brief Runs the free opcode.
     * @param context State of the call. Must not be null.
     * @param address Register bits of the address. 0 does nothing.
     * @return MAREA_VM_EXCEPT_SUCCESS or MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmFree(const MAREA_VmContext *context, MAREA_Uinteger64 address);

    /**
     * @brief Runs one function to completion, recursively for a call instruction. Every fault that is not a vm
     * limit is a trap in outResult, never a crash.
     * @param context State of the call. Must not be null.
     * @param module Module that holds the function. Must not be null.
     * @param functionIndex Index of the function to run.
     * @param arguments First register of every argument, in parameter order. May be null when argumentCount is 0.
     * @param argumentCount Number of arguments.
     * @param depth Number of frames above this one, including this one, used against MAREA_VM_CALL_DEPTH_LIMIT.
     * @param outResult Receives the outcome. Must not be null.
     * @return MAREA_VM_EXCEPT_SUCCESS, or MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE for a vm limit or a malformed module.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmRun(MAREA_VmContext * context, const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_Uinteger64 *arguments, MAREA_Uinteger32 argumentCount, MAREA_Uinteger32 depth, MAREA_VmResult *outResult);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_VM_MAREAVMINTERNAL_H */
