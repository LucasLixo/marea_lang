// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_CGEN_MAREACGEN_H)
    #define MAREA_INCLUDE_CGEN_MAREACGEN_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
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
    } MAREA_CgenDiagnosticCode;

    /**
     * @brief Opaque context that drives MAREA_CgenGenerateModule: the arena that owns the written text and the
     * diagnostic sink that a construct cgen cannot lower is reported to. Never prints, never touches global state.
     */
    typedef struct MAREA_CgenContext MAREA_CgenContext;

    /**
     * @brief Creates a cgen context.
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
     * @brief Writes C11 text for every scalar function of an ir module: its expressions, its control flow
     * (if, while, the while a do-while or a for-range lowers to, and the if chain a switch lowers to) and a
     * call of another function of the same module. A structure, an enum, a lifecycle call, a host call and
     * fork_join are not covered yet (the next cgen apply); a FUNCTION_DECLARATION that uses one is reported
     * through the sink and skipped, and the call still returns MAREA_CGEN_EXCEPT_SUCCESS. Integer overflow, a
     * shift and a division are generated with the same defined outcome as the vm (wraparound arithmetic, a
     * runtime trap flag for a division by zero and for a shift past the width), never C undefined behavior.
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
