// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_VM_MAREAVM_H)
    #define MAREA_INCLUDE_VM_MAREAVM_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/bytecode/MAREABytecode.h"
    #include "marea/bytecode/MAREABytecodeOpcode.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of a vm operation other than a run. A malformed call (an unknown function, the wrong number of
     * arguments) is not a trap: a trap is a defined outcome of a function that the vm did run.
     */
    typedef enum MAREA_VmExcept
    {
        MAREA_VM_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_VM_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_VM_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(3),
        MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION = MAREA_UINTEGER_8(4),
        MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT = MAREA_UINTEGER_8(5),
        MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR = MAREA_UINTEGER_8(6),
    } MAREA_VmExcept;

    /**
     * @brief Opaque context that a run uses: the arena for its own scratch state and the diagnostic sink. The vm
     * never prints and never reads the disk; it only interprets the module it is given.
     */
    typedef struct MAREA_VmContext MAREA_VmContext;

    /**
     * @brief Outcome of one call that the vm actually ran. A register is an untagged 64-bit value, read by the
     * caller the way the function's own result type says to (MAREA_BytecodeModuleGetFunction,
     * MAREA_BytecodeModuleGetType): the low bits of an integer, or the bit pattern of a float64 that a float32
     * result keeps inside (the same normalized form the bytecode stage stores in every register).
     */
    typedef struct MAREA_ALIGNED(8) MAREA_VmResult
    {
        MAREA_Uinteger64 value;
        MAREA_BytecodeTrapCode trap;
        MAREA_Boolean trapped;
        MAREA_Uinteger8 padding[3];
    } MAREA_VmResult;

    /**
     * @brief Creates a vm context.
     * @param arena Arena that owns the scratch state of every call. Must not be null.
     * @param sink Diagnostic sink of the context. The vm itself never raises a diagnostic (a trap is a value, not a
     * diagnostic), but the sink is kept so a host can share one context across stages. Must not be null.
     * @param outContext Receives the context. Must not be null.
     * @return MAREA_VM_EXCEPT_SUCCESS, MAREA_VM_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_VM_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_VmContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_VmContextGetArena(const MAREA_VmContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_VmContextGetDiagnostics(const MAREA_VmContext *context);

    /**
     * @brief Gives the context the registry that holds the host functions and the replacements of the runtime
     * hooks "alloc" and "free". A context has none until this call: every host call then traps
     * HOST_FUNCTION_MISSING, and alloc and free use the allocator of the vm. A host replaces both hooks or
     * neither: with one only, the first alloc or free of a run, also through the context of a host function, is
     * MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR. Every call of a host function gets a new MAREA_HostContext with
     * these hooks; a trap that the function raises there is a trap of the run.
     * @param context Context to change. Must not be null.
     * @param registry Registry to read, or null for none. It must outlive every call of the context.
     * @return MAREA_VM_EXCEPT_SUCCESS, or MAREA_VM_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmContextSetTypeRegistry(MAREA_VmContext * context, const MAREA_TypeRegistry *registry);

    /**
     * @brief Returns the registry of the context.
     * @param context Context to query. Null returns null.
     * @return The registry, or null when the context has none or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_TypeRegistry *MAREA_CALL MAREA_VmContextGetTypeRegistry(const MAREA_VmContext *context);

    /**
     * @brief Finds a function of a module by its full name (the namespace and the name, as
     * MAREA_BytecodeModuleGetFunction and MAREA_BytecodeModuleGetString give it). An import (a function with
     * MAREA_BYTECODE_FUNCTION_FLAG_HOST) is not a function of the module and is never found.
     * @param module Module to search. Must not be null.
     * @param name Bytes of the name. Must not be null.
     * @param length Number of bytes of the name.
     * @param outIndex Receives the index of the function. Must not be null.
     * @return MAREA_VM_EXCEPT_SUCCESS, MAREA_VM_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION when no function of the module has that name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmFindFunction(const MAREA_BytecodeModule *module, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex);

    /**
     * @brief Calls one function of a module by its index and runs it to completion: a return, or a trap. A fault
     * that a plain interpreter cannot recover from (a register or a jump target out of range, a call to a function
     * index that does not exist) is also reported as a trap, never a crash, because a module the vm is given may not
     * have passed MAREA_BytecodeValidate.
     * @param context Context of the call. Must not be null.
     * @param module Module that holds the function. Must not be null.
     * @param functionIndex Index of the function to run, as MAREA_BytecodeModuleGetFunction reads it.
     * @param arguments First register of every argument, in parameter order. May be null when argumentCount is 0.
     * @param argumentCount Number of arguments. Must equal the parameter count of the function.
     * @param outResult Receives the outcome. Must not be null.
     * @return MAREA_VM_EXCEPT_SUCCESS, MAREA_VM_EXCEPT_ERROR_NULL_POINTER when a required argument is null,
     * MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE when functionIndex is not a function of the module,
     * MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION when it is an import,
     * MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR when the registry replaces only one of "alloc" and "free" and the
     * run reaches one of them (also through a host context), or MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT when
     * argumentCount does not match the
     * parameter count. outResult is only filled on MAREA_VM_EXCEPT_SUCCESS.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmCall(MAREA_VmContext * context, const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_Uinteger64 *arguments, MAREA_Uinteger32 argumentCount, MAREA_VmResult *outResult);

    /**
     * @brief Finds a function by name, then calls it. The same as MAREA_VmFindFunction followed by MAREA_VmCall.
     * @param context Context of the call. Must not be null.
     * @param module Module that holds the function. Must not be null.
     * @param name Bytes of the name. Must not be null.
     * @param length Number of bytes of the name.
     * @param arguments First register of every argument, in parameter order. May be null when argumentCount is 0.
     * @param argumentCount Number of arguments. Must equal the parameter count of the function.
     * @param outResult Receives the outcome. Must not be null.
     * @return As MAREA_VmFindFunction or MAREA_VmCall, whichever fails first.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_VmExcept MAREA_CALL MAREA_VmCallByName(MAREA_VmContext * context, const MAREA_BytecodeModule *module, const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_Uinteger64 *arguments, MAREA_Uinteger32 argumentCount, MAREA_VmResult *outResult);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_VM_MAREAVM_H */
