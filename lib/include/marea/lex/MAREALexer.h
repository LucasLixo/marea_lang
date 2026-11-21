// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_LEX_MAREALEXER_H)
    #define MAREA_INCLUDE_LEX_MAREALEXER_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/lex/MAREAToken.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Diagnostic code emitted by the lexer, in the 1000-1999 range reserved for this stage.
     */
    typedef enum MAREA_LexDiagnosticCode
    {
        MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE = MAREA_UINTEGER_32(1001),
        MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING = MAREA_UINTEGER_32(1002),
        MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE = MAREA_UINTEGER_32(1003),
        MAREA_LEX_DIAGNOSTIC_CODE_MALFORMED_NUMBER = MAREA_UINTEGER_32(1004),
    } MAREA_LexDiagnosticCode;

    /**
     * @brief Opaque context that drives MAREA_LexTokenize: the arena and diagnostic sink it writes
     * into, and the file name attached to every token position. Never prints, never touches global
     * state.
     */
    typedef struct MAREA_LexContext MAREA_LexContext;

    /**
     * @brief Creates a lex context over the given arena and diagnostic sink.
     * @param arena Arena that owns the token list and every string this context allocates. Must not
     * be null.
     * @param sink Diagnostic sink diagnostics are appended to. Must not be null.
     * @param outContext Receives the created context. Must not be null.
     * @return MAREA_LEX_EXCEPT_SUCCESS, MAREA_LEX_EXCEPT_ERROR_NULL_POINTER when arena, sink or
     * outContext is null, or MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY when the backing allocation fails.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_LexExcept MAREA_CALL MAREA_LexContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_LexContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The context's arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_LexContextGetArena(const MAREA_LexContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The context's diagnostic sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_LexContextGetDiagnostics(const MAREA_LexContext *context);

    /**
     * @brief Sets the file name attached to every token position this context produces. The pointer
     * is kept as given, never copied; the caller keeps it alive as long as the context is used.
     * @param context Context to update. Must not be null.
     * @param name File name pointer to keep.
     * @return MAREA_LEX_EXCEPT_SUCCESS, or MAREA_LEX_EXCEPT_ERROR_NULL_POINTER when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_LexExcept MAREA_CALL MAREA_LexContextSetFileName(MAREA_LexContext * context, const MAREA_Character8 *name);

    /**
     * @brief Returns the file name attached to tokens by this context.
     * @param context Context to query. Null returns "<source>".
     * @return The context's file name, defaulting to "<source>" when none was set, or when context
     * is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_LexContextGetFileName(const MAREA_LexContext *context);

    /**
     * @brief Tokenizes an ASCII source buffer into a token list. The caller keeps source alive as
     * long as the resulting tokens are read, since their text points into it. The lexer never prints
     * and never touches global state; every malformed input is reported as a diagnostic and an
     * MAREA_TOKEN_KIND_ERROR token, and lexing continues. The list always ends with exactly one
     * MAREA_TOKEN_KIND_END_OF_FILE token.
     * @param context Context that owns the arena, the diagnostic sink and the file name. Must not be
     * null.
     * @param source ASCII source buffer. Must not be null when length is not 0.
     * @param length Byte length of source. Must not exceed UINT32_MAX.
     * @param outList Receives the produced token list. Must not be null.
     * @return MAREA_LEX_EXCEPT_SUCCESS, MAREA_LEX_EXCEPT_ERROR_NULL_POINTER when context, outList, or
     * source with a non-zero length is null, MAREA_LEX_EXCEPT_ERROR_SOURCE_TOO_LARGE when length
     * exceeds UINT32_MAX, or MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY when growing the list or allocating
     * a token fails.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_LexExcept MAREA_CALL MAREA_LexTokenize(MAREA_LexContext * context, const MAREA_Character8 *source, MAREA_IntegerSize length, MAREA_TokenList **outList);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_LEX_MAREALEXER_H */
