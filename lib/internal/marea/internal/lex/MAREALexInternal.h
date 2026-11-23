// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_LEX_MAREALEXINTERNAL_H)
    #define MAREA_INTERNAL_LEX_MAREALEXINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/base/MAREASourcePosition.h"
    #include "marea/lex/MAREALexer.h"
    #include "marea/lex/MAREAToken.h"
    #include "marea/lex/MAREATokenKind.h"

struct MAREA_Token
{
    MAREA_SourcePosition position;
    const MAREA_Character8 *text;
    MAREA_Uinteger32 length;
    MAREA_TokenKind kind;
};

struct MAREA_TokenList
{
    MAREA_Arena *arena;
    MAREA_Token *items;
    MAREA_IntegerSize count;
    MAREA_IntegerSize capacity;
};

struct MAREA_LexContext
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_Character8 *fileName;
};

/**
 * @brief Cursor over the source buffer being scanned: current byte offset and 1-based line and
 * column, plus the context lexing is done for.
 */
typedef struct MAREA_LexCursor
{
    MAREA_LexContext *context;
    const MAREA_Character8 *source;
    MAREA_IntegerSize length;
    MAREA_IntegerSize offset;
    MAREA_Uinteger32 line;
    MAREA_Uinteger32 column;
} MAREA_LexCursor;

/**
 * @brief Returns the byte at offsetFromCurrent bytes past the cursor, without consuming it.
 * @param cursor Cursor to read. Must not be null.
 * @param offsetFromCurrent How many bytes past the current position to read.
 * @return The byte value (0-255), or -1 when the position is at or past the end of source.
 */
MAREA_Sinteger32 MAREA_CALL MAREA_LexCursorPeek(const MAREA_LexCursor *cursor, MAREA_IntegerSize offsetFromCurrent);

/**
 * @brief Returns whether the cursor has consumed every byte of source.
 * @param cursor Cursor to query. Must not be null.
 * @return MAREA_BOOLEAN_TRUE when the cursor is at or past the end of source, else
 * MAREA_BOOLEAN_FALSE.
 */
MAREA_Boolean MAREA_CALL MAREA_LexCursorIsAtEnd(const MAREA_LexCursor *cursor);

/**
 * @brief Consumes exactly one byte, updating offset, line and column: a line feed starts a new
 * line and resets the column, any other byte advances the column by one.
 * @param cursor Cursor to advance. Must not be null. Must not already be at the end.
 */
void MAREA_CALL MAREA_LexCursorAdvance(MAREA_LexCursor *cursor);

/**
 * @brief Builds the source position of a span that starts where startOffset, startLine and
 * startColumn record and ends at the cursor's current offset.
 * @param cursor Cursor whose file name and current offset close the span. Must not be null.
 * @param startOffset Byte offset the span started at.
 * @param startLine Line the span started at.
 * @param startColumn Column the span started at.
 * @return The resulting MAREA_SourcePosition, by value.
 */
MAREA_SourcePosition MAREA_CALL MAREA_LexCursorMakePosition(const MAREA_LexCursor *cursor, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn);

/**
 * @brief Appends a diagnostic to the cursor's context sink at the given position.
 * @param cursor Cursor whose context sink receives the diagnostic. Must not be null.
 * @param code Lexer diagnostic code.
 * @param position Position the diagnostic refers to.
 * @param message NUL-terminated message, copied by the sink. Must not be null.
 */
void MAREA_CALL MAREA_LexCursorReportDiagnostic(const MAREA_LexCursor *cursor, MAREA_LexDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *message);

/**
 * @brief Appends a token to the list, growing its backing array by doubling when full.
 * @param list List to append to. Must not be null.
 * @param kind Kind of the new token.
 * @param text Pointer into the lexed source for the token's text. May be null when length is 0.
 * @param length Byte length of the token's text.
 * @param position Source position of the token.
 * @return MAREA_LEX_EXCEPT_SUCCESS, or MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY when growing the
 * list fails.
 */
MAREA_LexExcept MAREA_CALL MAREA_LexTokenListAppend(MAREA_TokenList *list, MAREA_TokenKind kind, const MAREA_Character8 *text, MAREA_Uinteger32 length, MAREA_SourcePosition position);

/**
 * @brief Returns whether byte is an ASCII hexadecimal digit (0-9, a-f or A-F). Shared by the
 * number and string scanners.
 * @param byte Byte value to classify, as returned by MAREA_LexCursorPeek.
 * @return MAREA_BOOLEAN_TRUE when byte is a hexadecimal digit, else MAREA_BOOLEAN_FALSE.
 */
MAREA_Boolean MAREA_CALL MAREA_LexIsHexDigit(MAREA_Sinteger32 byte);

/**
 * @brief Scans a number literal (decimal, 0x hexadecimal or 0b binary, with an optional float
 * fraction and exponent on the decimal form) starting at the cursor's current digit, and appends
 * the resulting token to list. A 0x or 0b prefix with no valid digit reports
 * MAREA_LEX_DIAGNOSTIC_CODE_MALFORMED_NUMBER and appends an MAREA_TOKEN_KIND_ERROR token instead.
 * @param cursor Cursor positioned at the first digit. Must not be null.
 * @param list List to append the resulting token to. Must not be null.
 * @return MAREA_LEX_EXCEPT_SUCCESS, or MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY when appending fails.
 */
MAREA_LexExcept MAREA_CALL MAREA_LexScanNumber(MAREA_LexCursor *cursor, MAREA_TokenList *list);

/**
 * @brief Scans a string literal starting at the cursor's opening quote, and appends the
 * resulting token to list. A bad escape or a non-ASCII or control byte other than tab reports a
 * diagnostic and the string continues; a string cut off by a line end, a trailing backslash or
 * end of input reports MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING and appends an
 * MAREA_TOKEN_KIND_ERROR token instead of MAREA_TOKEN_KIND_STRING_LITERAL.
 * @param cursor Cursor positioned at the opening quote. Must not be null.
 * @param list List to append the resulting token to. Must not be null.
 * @return MAREA_LEX_EXCEPT_SUCCESS, or MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY when appending fails.
 */
MAREA_LexExcept MAREA_CALL MAREA_LexScanString(MAREA_LexCursor *cursor, MAREA_TokenList *list);

/**
 * @brief Scans a character literal starting at the cursor's opening single quote, and appends the
 * resulting token to list. The escapes are the string escapes plus an escaped single quote. A bad
 * escape or byte reports a diagnostic and the literal continues. An empty literal reports
 * MAREA_LEX_DIAGNOSTIC_CODE_EMPTY_CHARACTER, a literal with more than one character reports
 * MAREA_LEX_DIAGNOSTIC_CODE_TOO_MANY_CHARACTERS, and a literal cut off by a line end or end of
 * input reports MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING; each of the three appends an
 * MAREA_TOKEN_KIND_ERROR token instead of MAREA_TOKEN_KIND_CHARACTER_LITERAL.
 * @param cursor Cursor positioned at the opening quote. Must not be null.
 * @param list List to append the resulting token to. Must not be null.
 * @return MAREA_LEX_EXCEPT_SUCCESS, or MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY when appending fails.
 */
MAREA_LexExcept MAREA_CALL MAREA_LexScanCharacter(MAREA_LexCursor *cursor, MAREA_TokenList *list);

#endif /* MAREA_INTERNAL_LEX_MAREALEXINTERNAL_H */
