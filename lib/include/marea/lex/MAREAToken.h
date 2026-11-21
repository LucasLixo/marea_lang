// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_LEX_MAREATOKEN_H)
    #define MAREA_INCLUDE_LEX_MAREATOKEN_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/lex/MAREATokenKind.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Result of a token or token list operation. Also returned by the MAREA_LexContext and
     * MAREA_LexTokenize functions of marea/lex/MAREALexer.h, declared here since both consume it.
     */
    typedef enum MAREA_LexExcept
    {
        MAREA_LEX_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_LEX_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_LEX_EXCEPT_ERROR_OUT_OF_RANGE = MAREA_UINTEGER_8(3),
        MAREA_LEX_EXCEPT_ERROR_SOURCE_TOO_LARGE = MAREA_UINTEGER_8(4),
    } MAREA_LexExcept;

    /**
     * @brief Opaque lexed token: an immutable result with a kind, a span of source text and a
     * position. Tokens never own their text; it points into the caller's source buffer.
     */
    typedef struct MAREA_Token MAREA_Token;

    /**
     * @brief Opaque, arena-backed, contiguous list of tokens produced by MAREA_LexTokenize.
     */
    typedef struct MAREA_TokenList MAREA_TokenList;

    /**
     * @brief Returns the kind of a token.
     * @param token Token to query. Null returns MAREA_TOKEN_KIND_ERROR.
     * @return The token's kind, or MAREA_TOKEN_KIND_ERROR when token is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TokenKind MAREA_CALL MAREA_TokenGetKind(const MAREA_Token *token);

    /**
     * @brief Returns a pointer to the token's text, pointing into the source buffer the lexer was
     * given. Not NUL-terminated; read exactly MAREA_TokenGetLength bytes.
     * @param token Token to query. Null returns null.
     * @return A pointer into the lexed source, or null when token is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_TokenGetText(const MAREA_Token *token);

    /**
     * @brief Returns the byte length of the token's text.
     * @param token Token to query. Null returns 0.
     * @return The token's length in bytes, or 0 when token is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Uinteger32 MAREA_CALL MAREA_TokenGetLength(const MAREA_Token *token);

    /**
     * @brief Returns the token's source position by value.
     * @param token Token to query. Null returns a zeroed position.
     * @return The token's position, or a zeroed MAREA_SourcePosition when token is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SourcePosition MAREA_CALL MAREA_TokenGetPosition(const MAREA_Token *token);

    /**
     * @brief Returns how many tokens the list holds, END_OF_FILE included.
     * @param list List to query. Null returns 0.
     * @return The token count, or 0 when list is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_IntegerSize MAREA_CALL MAREA_TokenListGetCount(const MAREA_TokenList *list);

    /**
     * @brief Retrieves the token stored at index.
     * @param list List to query. Must not be null.
     * @param index Zero-based index into the list.
     * @param out Receives the token pointer, valid for the arena's lifetime. Must not be null.
     * @return MAREA_LEX_EXCEPT_SUCCESS, MAREA_LEX_EXCEPT_ERROR_NULL_POINTER when list or out is null,
     * or MAREA_LEX_EXCEPT_ERROR_OUT_OF_RANGE when index is not less than the count.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_LexExcept MAREA_CALL MAREA_TokenListGetAt(const MAREA_TokenList *list, MAREA_IntegerSize index, const MAREA_Token **out);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_LEX_MAREATOKEN_H */
