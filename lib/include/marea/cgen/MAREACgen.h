// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_CGEN_MAREACGEN_H)
    #define MAREA_INCLUDE_CGEN_MAREACGEN_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/ir/MAREAIrNode.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of a cgen operation. A construct that cannot be lowered yet, or a type the C11 backend does
     * not cover, is not an exception: it is a diagnostic, and the call still succeeds (see MAREACgenDiagnosticCode).
     */
    typedef enum MAREA_CgenExcept
    {
        MAREA_CGEN_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_CGEN_EXCEPT_ERROR_BAD_ROOT = MAREA_UINTEGER_8(3),
        MAREA_CGEN_EXCEPT_ERROR_UNPAIRED_ALLOCATOR = MAREA_UINTEGER_8(4),
    } MAREA_CgenExcept;

    /**
     * @brief Diagnostic code emitted by cgen, in the 7000-7999 range reserved for this stage. Every code is an
     * error, the same way the ir and the bytecode stage report what they cannot lower.
     */
    typedef enum MAREA_CgenDiagnosticCode
    {
        MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT = MAREA_UINTEGER_32(7001),
        MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE = MAREA_UINTEGER_32(7002),
        MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME = MAREA_UINTEGER_32(7003),
        MAREA_CGEN_DIAGNOSTIC_CODE_UNTYPED_OPERAND = MAREA_UINTEGER_32(7004),
    } MAREA_CgenDiagnosticCode;

    /**
     * @brief The contextPosition of a C function that takes no MAREA_HostContext.
     */
    #if !defined(MAREA_CGEN_HOST_NO_CONTEXT)
        #define MAREA_CGEN_HOST_NO_CONTEXT MAREA_INTEGER_SIZE_MAXIMUM
    #endif

    /**
     * @brief What cgen must know about one C function that the host or a library registered: the name of the
     * typed C function (not of its wrapper) and where it takes a MAREA_HostContext *. The struct is open because
     * the caller of cgen fills it, in a MAREA_CgenHostResolveFunction.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_CgenHostFunction
    {
        const MAREA_Character8 *symbol;
        MAREA_IntegerSize symbolLength;
        MAREA_IntegerSize contextPosition;
        MAREA_PADDING_32(4)
    } MAREA_CgenHostFunction;

    /**
     * @brief Function that the caller of cgen gives so that it can call the C functions of the host. The generated
     * C calls the typed function itself, with its own parameter list: a primitive by value, a pointer for each
     * borrowed structure, a last pointer for a structure result, and a MAREA_HostContext * at contextPosition.
     * @param userData Pointer given to MAREA_CgenContextSetHostResolver.
     * @param name Full path of the registration, for example "demo.host.add", not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @param outFunction Receives the function. Read only when the call returns true; cgen gives it with no symbol
     * and MAREA_CGEN_HOST_NO_CONTEXT. The symbol must stay valid until MAREA_CgenGenerateModule returns.
     * @return True when the host has a C function for the name, false when it has none.
     */
    typedef MAREA_Boolean(MAREA_CALL * MAREA_CgenHostResolveFunction)(MAREA_Null userData, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_CgenHostFunction *outFunction);

    /**
     * @brief Opaque context that drives MAREA_CgenGenerateModule: the arena that owns the written text and the
     * diagnostic sink that a construct cgen cannot lower is reported to, and what the caller knows of the host: the
     * registry of the library types, the C functions and the headers that declare them. Never prints, never touches
     * global state.
     */
    typedef struct MAREA_CgenContext MAREA_CgenContext;

    /**
     * @brief Creates a cgen context, with no registry, no host function, no header and the default allocator.
     * @param arena Arena that owns the generated text and the working data. Must not be null.
     * @param sink Diagnostic sink that diagnostics are appended to. Must not be null.
     * @param outContext Receives the context. Must not be null.
     * @return MAREA_CGEN_EXCEPT_SUCCESS, MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_CgenContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_CgenContextGetArena(const MAREA_CgenContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_CgenContextGetDiagnostics(const MAREA_CgenContext *context);

    /**
     * @brief Gives the context the registry that describes the library types, the same registry that sema was
     * given. cgen reads one shape from it: an inline run (an index protocol, no role, a copy class and two type
     * arguments, the element and the count), which it writes as a C array. Any other library type has no form.
     * @param context Context to change. Must not be null.
     * @param registry Registry to read, or null for none. It must outlive every generation of the context.
     * @return MAREA_CGEN_EXCEPT_SUCCESS, or MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetTypeRegistry(MAREA_CgenContext * context, const MAREA_TypeRegistry *registry);

    /**
     * @brief Returns the registry of the context.
     * @param context Context to query. Null returns null.
     * @return The registry, or null when the context has none or context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_TypeRegistry *MAREA_CALL MAREA_CgenContextGetTypeRegistry(const MAREA_CgenContext *context);

    /**
     * @brief Gives the context the function that names the C functions of the host. A call of a name that the host
     * or a library registered goes to the C function that the resolver gives. A name with no answer, and every such
     * call when there is no resolver, is the trap HOST_FUNCTION_MISSING at run time, as in the vm.
     * @param context Context to change. Must not be null.
     * @param resolve Resolver, or null for none.
     * @param userData Pointer given back to the resolver. May be null.
     * @return MAREA_CGEN_EXCEPT_SUCCESS, or MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetHostResolver(MAREA_CgenContext * context, MAREA_CgenHostResolveFunction resolve, MAREA_Null userData);

    /**
     * @brief Adds one header that the generated text includes with quotes, in the order of the calls, before any
     * code. The headers declare the C functions that the resolver names and their types.
     * @param context Context to change. Must not be null.
     * @param header Name of the header, not NUL terminated. Must not be null. The context copies it.
     * @param length Number of bytes of header.
     * @return MAREA_CGEN_EXCEPT_SUCCESS, MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER when context or header is null, or
     * MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenContextAddInclude(MAREA_CgenContext * context, const MAREA_Character8 *header, MAREA_IntegerSize length);

    /**
     * @brief Names the C functions that replace the runtime hooks "alloc" and "free" in the context that the
     * generated C gives to a host function, with the shapes MAREA_RuntimeAllocateFunction and
     * MAREA_RuntimeFreeFunction; a header of the context declares them. With no name the context uses the
     * allocator of MAREA_HostContextInitialize. Both names are given, or neither.
     * @param context Context to change. Must not be null.
     * @param allocate Name of the allocate function, not NUL terminated, or null.
     * @param allocateLength Number of bytes of allocate.
     * @param release Name of the free function, not NUL terminated, or null.
     * @param releaseLength Number of bytes of release.
     * @return MAREA_CGEN_EXCEPT_SUCCESS, MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER when context is null,
     * MAREA_CGEN_EXCEPT_ERROR_UNPAIRED_ALLOCATOR when exactly one name is given, or
     * MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetAllocator(MAREA_CgenContext * context, const MAREA_Character8 *allocate, MAREA_IntegerSize allocateLength, const MAREA_Character8 *release, MAREA_IntegerSize releaseLength);

    /**
     * @brief Writes C11 text for every function of an ir module: scalar code, structures, enums, arrays of a
     * registered inline run, constants, construct and destroy operators, calls of the unit and of the host, and
     * fork_join as a sequential loop. The behavior is the behavior of the vm for the same module: operands run in
     * the order of the source, a fault is a trap that stops the run, and arithmetic never relies on C undefined
     * behavior. docs/cgen/marea-cgen.md gives the forms, the names of the C symbols and what is not written. A
     * function that uses something cgen does not write is reported through the sink and left out, and the call
     * still returns MAREA_CGEN_EXCEPT_SUCCESS.
     * @param context Context that owns the arena and the sink. Must not be null.
     * @param module MODULE root of the ir tree to generate from. Must not be null.
     * @param outText Receives a NUL-terminated copy of the generated C11 source. Must not be null.
     * @param outLength Receives the length of the text without the NUL. Must not be null.
     * @return MAREA_CGEN_EXCEPT_SUCCESS, MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER when an argument is null,
     * MAREA_CGEN_EXCEPT_ERROR_BAD_ROOT when module is not a MODULE node, or MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY
     * when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_CgenExcept MAREA_CALL MAREA_CgenGenerateModule(MAREA_CgenContext * context, const MAREA_IrNode *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_CGEN_MAREACGEN_H */
