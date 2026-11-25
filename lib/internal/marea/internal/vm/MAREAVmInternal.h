// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_VM_MAREAVMINTERNAL_H)
    #define MAREA_INTERNAL_VM_MAREAVMINTERNAL_H

    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREADiagnostic.h"
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
     * @brief The context every vm call reads: the arena and the sink it was created with.
     */
    typedef struct MAREA_VmContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
    } MAREA_VmContext;

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
