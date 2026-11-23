// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREADIAGNOSTIC_H)
    #define MAREA_INCLUDE_BASE_MAREADIAGNOSTIC_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREASourcePosition.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Opaque diagnostic record held by a MAREA_DiagnosticSink.
     */
    typedef struct MAREA_Diagnostic MAREA_Diagnostic;

    /**
     * @brief Opaque, arena-backed, growing list of diagnostics. Appended to, never cleared.
     */
    typedef struct MAREA_DiagnosticSink MAREA_DiagnosticSink;

    /**
     * @brief Severity of a diagnostic.
     */
    typedef enum MAREA_DiagnosticSeverity
    {
        MAREA_DIAGNOSTIC_SEVERITY_ERROR = MAREA_UINTEGER_8(0),
        MAREA_DIAGNOSTIC_SEVERITY_WARNING = MAREA_UINTEGER_8(1),
    } MAREA_DiagnosticSeverity;

    /**
     * @brief Result of a MAREA_Diagnostic / MAREA_DiagnosticSink operation.
     */
    typedef enum MAREA_DiagnosticExcept
    {
        MAREA_DIAGNOSTIC_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(3),
    } MAREA_DiagnosticExcept;

    /**
     * @brief Creates an empty diagnostic sink backed by the given arena.
     * @param arena Arena that owns every diagnostic the sink stores. Must not be null.
     * @param outSink Receives the created sink. Must not be null.
     * @return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS, MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER when arena or
     * outSink is null, or MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_MEMORY when the backing allocation fails.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticExcept MAREA_CALL MAREA_DiagnosticSinkCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * *outSink);

    /**
     * @brief Appends a diagnostic to the sink, copying message into the sink's arena.
     * @param sink Sink to append to. Must not be null.
     * @param code Diagnostic code, stage-defined.
     * @param severity Severity of the diagnostic.
     * @param position Source position the diagnostic refers to, taken and stored by value.
     * @param message NUL-terminated text, copied into the arena. Must not be null.
     * @return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS, MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER when sink or
     * message is null, or MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_MEMORY when growing the sink fails.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticExcept MAREA_CALL MAREA_DiagnosticSinkAdd(MAREA_DiagnosticSink * sink, MAREA_Uinteger32 code, MAREA_DiagnosticSeverity severity, MAREA_SourcePosition position, const MAREA_Character8 *message);

    /**
     * @brief Returns how many diagnostics the sink holds.
     * @param sink Sink to query. Null returns 0.
     * @return The diagnostic count, or 0 when sink is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_DiagnosticSinkGetCount(const MAREA_DiagnosticSink *sink);

    /**
     * @brief Returns how many diagnostics in the sink carry MAREA_DIAGNOSTIC_SEVERITY_ERROR.
     * @param sink Sink to query. Null returns 0.
     * @return The error diagnostic count, or 0 when sink is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_DiagnosticSinkGetErrorCount(const MAREA_DiagnosticSink *sink);

    /**
     * @brief Retrieves the diagnostic stored at index.
     * @param sink Sink to query. Must not be null.
     * @param index Zero-based index into the sink.
     * @param out Receives the diagnostic pointer, valid for the arena's lifetime. Must not be null.
     * @return MAREA_DIAGNOSTIC_EXCEPT_SUCCESS, MAREA_DIAGNOSTIC_EXCEPT_ERROR_NULL_POINTER when sink or
     * out is null, or MAREA_DIAGNOSTIC_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticExcept MAREA_CALL MAREA_DiagnosticSinkGetAt(const MAREA_DiagnosticSink *sink, MAREA_IntegerSize index, const MAREA_Diagnostic **out);

    /**
     * @brief Returns the diagnostic's code.
     * @param diagnostic Diagnostic to query. Null returns 0.
     * @return The stored code, or 0 when diagnostic is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_DiagnosticGetCode(const MAREA_Diagnostic *diagnostic);

    /**
     * @brief Returns the diagnostic's severity.
     * @param diagnostic Diagnostic to query. Null returns MAREA_DIAGNOSTIC_SEVERITY_ERROR.
     * @return The stored severity, or MAREA_DIAGNOSTIC_SEVERITY_ERROR when diagnostic is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSeverity MAREA_CALL MAREA_DiagnosticGetSeverity(const MAREA_Diagnostic *diagnostic);

    /**
     * @brief Returns the diagnostic's source position by value.
     * @param diagnostic Diagnostic to query. Null returns a zeroed position.
     * @return The stored position, or a zeroed MAREA_SourcePosition when diagnostic is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SourcePosition MAREA_CALL MAREA_DiagnosticGetPosition(const MAREA_Diagnostic *diagnostic);

    /**
     * @brief Returns the diagnostic's message text.
     * @param diagnostic Diagnostic to query. Null returns null.
     * @return A NUL-terminated pointer into the sink's arena, or null when diagnostic is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_DiagnosticGetMessage(const MAREA_Diagnostic *diagnostic);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREADIAGNOSTIC_H */
