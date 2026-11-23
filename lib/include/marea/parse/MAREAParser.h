// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_PARSE_MAREAPARSER_H)
    #define MAREA_INCLUDE_PARSE_MAREAPARSER_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/lex/MAREAToken.h"
    #include "marea/parse/MAREAAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of a parser operation. A syntax error is not an exception: it is a diagnostic.
     */
    typedef enum MAREA_ParseExcept
    {
        MAREA_PARSE_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_PARSE_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
    } MAREA_ParseExcept;

    /**
     * @brief Diagnostic code emitted by the parser, in the 2000-2999 range reserved for this stage.
     */
    typedef enum MAREA_ParseDiagnosticCode
    {
        MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN = MAREA_UINTEGER_32(2001),
        MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION = MAREA_UINTEGER_32(2002),
        MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TYPE = MAREA_UINTEGER_32(2003),
        MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_DECLARATION = MAREA_UINTEGER_32(2004),
        MAREA_PARSE_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN = MAREA_UINTEGER_32(2005),
        MAREA_PARSE_DIAGNOSTIC_CODE_CHAINED_COMPARISON = MAREA_UINTEGER_32(2006),
        MAREA_PARSE_DIAGNOSTIC_CODE_MISSING_NAMESPACE = MAREA_UINTEGER_32(2007),
        MAREA_PARSE_DIAGNOSTIC_CODE_RECURSION_TOO_DEEP = MAREA_UINTEGER_32(2008),
        MAREA_PARSE_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK = MAREA_UINTEGER_32(2009),
        MAREA_PARSE_DIAGNOSTIC_CODE_DEFAULT_ARM_NOT_LAST = MAREA_UINTEGER_32(2010),
    } MAREA_ParseDiagnosticCode;

    /**
     * @brief Opaque context that drives MAREA_ParseCompilationUnit: the arena and diagnostic sink it
     * writes into. Never prints, never touches global state.
     */
    typedef struct MAREA_ParseContext MAREA_ParseContext;

    /**
     * @brief Creates a parse context over the given arena and diagnostic sink.
     * @param arena Arena that owns the tree. Must not be null.
     * @param sink Diagnostic sink that diagnostics are appended to. Must not be null.
     * @param outContext Receives the context. Must not be null.
     * @return MAREA_PARSE_EXCEPT_SUCCESS, MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER when an argument is
     * null, or MAREA_PARSE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_ParseExcept MAREA_CALL MAREA_ParseContextCreate(MAREA_Arena * arena, MAREA_DiagnosticSink * sink, MAREA_ParseContext * *outContext);

    /**
     * @brief Returns the arena the context was created with.
     * @param context Context to query. Null returns null.
     * @return The arena, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Arena *MAREA_CALL MAREA_ParseContextGetArena(const MAREA_ParseContext *context);

    /**
     * @brief Returns the diagnostic sink the context was created with.
     * @param context Context to query. Null returns null.
     * @return The sink, or null when context is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_DiagnosticSink *MAREA_CALL MAREA_ParseContextGetDiagnostics(const MAREA_ParseContext *context);

    /**
     * @brief Parses a token list into a tree. A syntax error is reported to the sink, the parser
     * recovers, and an ERROR_NODE marks the place. The token list and the source that its tokens
     * point into must stay alive as long as the tree is read.
     * @param context Context that owns the arena and the sink. Must not be null.
     * @param list Token list from MAREA_LexTokenize. Must not be null.
     * @param outRoot Receives the COMPILATION_UNIT node. Must not be null.
     * @return MAREA_PARSE_EXCEPT_SUCCESS, MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER when an argument is
     * null, or MAREA_PARSE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_ParseExcept MAREA_CALL MAREA_ParseCompilationUnit(MAREA_ParseContext * context, const MAREA_TokenList *list, const MAREA_AstNode **outRoot);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_PARSE_MAREAPARSER_H */
