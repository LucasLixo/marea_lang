// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/lex/MAREALexInternal.h"

MAREA_Boolean MAREA_CALL MAREA_LexIsBadLiteralByte(MAREA_Sinteger32 byte)
{
    return ((byte >= 0x80) || ((byte < 0x20) && (byte != '\t')) || (byte == 0x7F)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_LexExcept MAREA_CALL MAREA_LexScanEscape(MAREA_LexCursor *cursor, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn, MAREA_Boolean allowSingleQuote)
{
    const MAREA_Sinteger32 ESCAPE_CHARACTER = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

    if ((ESCAPE_CHARACTER == 'n') || (ESCAPE_CHARACTER == 't') || (ESCAPE_CHARACTER == 'r') || (ESCAPE_CHARACTER == '0') || (ESCAPE_CHARACTER == '\\') || (ESCAPE_CHARACTER == '"') || ((ESCAPE_CHARACTER == '\'') && (allowSingleQuote == MAREA_BOOLEAN_TRUE))) {
        MAREA_LexCursorAdvance(cursor);

        return MAREA_LEX_EXCEPT_SUCCESS;
    }

    if (ESCAPE_CHARACTER == 'x') {
        MAREA_LexCursorAdvance(cursor);

        if (MAREA_LexIsHexDigit(MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0))) && MAREA_LexIsHexDigit(MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(1)))) {
            MAREA_LexCursorAdvance(cursor);
            MAREA_LexCursorAdvance(cursor);

            return MAREA_LEX_EXCEPT_SUCCESS;
        }

        return MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE, MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn), "invalid hex escape sequence");
    }

    const MAREA_LexExcept reportResult = MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE, MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn), "invalid escape sequence");

    if (reportResult != MAREA_LEX_EXCEPT_SUCCESS) {
        return reportResult;
    }

    // INFO: a line end is not part of the escape, so the next loop turn of the caller reports the cut-off literal at it.
    if ((ESCAPE_CHARACTER != '\n') && (ESCAPE_CHARACTER != '\r')) {
        MAREA_LexCursorAdvance(cursor);
    }

    return MAREA_LEX_EXCEPT_SUCCESS;
}

static MAREA_LexExcept MAREA_CALL MAREA_G_LexStringUnterminated(MAREA_LexCursor *cursor, MAREA_TokenList *list, const MAREA_Character8 *text, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn)
{
    const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn);

    const MAREA_LexExcept reportResult = MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING, POSITION, "unterminated string literal");

    if (reportResult != MAREA_LEX_EXCEPT_SUCCESS) {
        return reportResult;
    }

    return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_ERROR, text, POSITION.length, POSITION);
}

MAREA_LexExcept MAREA_CALL MAREA_LexScanString(MAREA_LexCursor *cursor, MAREA_TokenList *list)
{
    const MAREA_IntegerSize START_OFFSET = cursor->offset;
    const MAREA_Uinteger32 START_LINE = cursor->line;
    const MAREA_Uinteger32 START_COLUMN = cursor->column;
    const MAREA_Character8 *text = &cursor->source[START_OFFSET];

    MAREA_LexCursorAdvance(cursor);

    for (;;) {
        if (MAREA_LexCursorIsAtEnd(cursor)) {
            return MAREA_G_LexStringUnterminated(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN);
        }

        const MAREA_Sinteger32 CURRENT = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

        if ((CURRENT == '\n') || (CURRENT == '\r')) {
            return MAREA_G_LexStringUnterminated(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN);
        }

        if (CURRENT == '"') {
            MAREA_LexCursorAdvance(cursor);

            const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, START_OFFSET, START_LINE, START_COLUMN);

            return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_STRING_LITERAL, text, POSITION.length, POSITION);
        }

        if (CURRENT == '\\') {
            MAREA_LexCursorAdvance(cursor);

            if (MAREA_LexCursorIsAtEnd(cursor)) {
                return MAREA_G_LexStringUnterminated(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN);
            }

            const MAREA_LexExcept escapeResult = MAREA_LexScanEscape(cursor, START_OFFSET, START_LINE, START_COLUMN, MAREA_BOOLEAN_FALSE);

            if (escapeResult != MAREA_LEX_EXCEPT_SUCCESS) {
                return escapeResult;
            }

            continue;
        }

        if (MAREA_LexIsBadLiteralByte(CURRENT)) {
            const MAREA_IntegerSize BAD_START_OFFSET = cursor->offset;
            const MAREA_Uinteger32 BAD_START_LINE = cursor->line;
            const MAREA_Uinteger32 BAD_START_COLUMN = cursor->column;

            MAREA_LexCursorAdvance(cursor);

            const MAREA_SourcePosition BAD_POSITION = MAREA_LexCursorMakePosition(cursor, BAD_START_OFFSET, BAD_START_LINE, BAD_START_COLUMN);

            const MAREA_LexExcept reportResult = MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE, BAD_POSITION, "unexpected byte in string literal");

            if (reportResult != MAREA_LEX_EXCEPT_SUCCESS) {
                return reportResult;
            }

            continue;
        }

        MAREA_LexCursorAdvance(cursor);
    }
}
