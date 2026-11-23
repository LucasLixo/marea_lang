// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/lex/MAREALexInternal.h"

static MAREA_Boolean MAREA_CALL MAREA_G_LexCharacterIsBadByte(MAREA_Sinteger32 byte)
{
    return ((byte >= 0x80) || ((byte < 0x20) && (byte != '\t')) || (byte == 0x7F)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_LexExcept MAREA_CALL MAREA_G_LexCharacterError(MAREA_LexCursor *cursor, MAREA_TokenList *list, const MAREA_Character8 *text, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn, MAREA_Uinteger32 code, const MAREA_Character8 *message)
{
    const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn);

    MAREA_LexCursorReportDiagnostic(cursor, code, POSITION, message);

    return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_ERROR, text, POSITION.length, POSITION);
}

// INFO: reads the escape after a backslash and reports a bad one; the literal goes on either way.
static void MAREA_CALL MAREA_G_LexCharacterEscape(MAREA_LexCursor *cursor, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn)
{
    const MAREA_Sinteger32 ESCAPE_CHARACTER = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

    if ((ESCAPE_CHARACTER == 'n') || (ESCAPE_CHARACTER == 't') || (ESCAPE_CHARACTER == 'r') || (ESCAPE_CHARACTER == '0') || (ESCAPE_CHARACTER == '\\') || (ESCAPE_CHARACTER == '"') || (ESCAPE_CHARACTER == '\'')) {
        MAREA_LexCursorAdvance(cursor);

        return;
    }

    if (ESCAPE_CHARACTER == 'x') {
        MAREA_LexCursorAdvance(cursor);

        if (MAREA_LexIsHexDigit(MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0))) && MAREA_LexIsHexDigit(MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(1)))) {
            MAREA_LexCursorAdvance(cursor);
            MAREA_LexCursorAdvance(cursor);

            return;
        }

        MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE, MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn), "invalid hex escape sequence");

        return;
    }

    MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE, MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn), "invalid escape sequence");

    // INFO: a line end is not part of the escape, so the next loop turn reports the cut-off literal at it.
    if ((ESCAPE_CHARACTER != '\n') && (ESCAPE_CHARACTER != '\r')) {
        MAREA_LexCursorAdvance(cursor);
    }
}

MAREA_LexExcept MAREA_CALL MAREA_LexScanCharacter(MAREA_LexCursor *cursor, MAREA_TokenList *list)
{
    const MAREA_IntegerSize START_OFFSET = cursor->offset;
    const MAREA_Uinteger32 START_LINE = cursor->line;
    const MAREA_Uinteger32 START_COLUMN = cursor->column;
    const MAREA_Character8 *text = &cursor->source[START_OFFSET];
    MAREA_IntegerSize characterCount = MAREA_INTEGER_SIZE(0);

    MAREA_LexCursorAdvance(cursor);

    for (;;) {
        if (MAREA_LexCursorIsAtEnd(cursor)) {
            return MAREA_G_LexCharacterError(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN, MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING, "unterminated character literal");
        }

        const MAREA_Sinteger32 CURRENT = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

        if ((CURRENT == '\n') || (CURRENT == '\r')) {
            return MAREA_G_LexCharacterError(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN, MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING, "unterminated character literal");
        }

        if (CURRENT == '\'') {
            MAREA_LexCursorAdvance(cursor);

            if (characterCount == MAREA_INTEGER_SIZE(0)) {
                return MAREA_G_LexCharacterError(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN, MAREA_LEX_DIAGNOSTIC_CODE_EMPTY_CHARACTER, "empty character literal");
            }

            if (characterCount > MAREA_INTEGER_SIZE(1)) {
                return MAREA_G_LexCharacterError(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN, MAREA_LEX_DIAGNOSTIC_CODE_TOO_MANY_CHARACTERS, "character literal holds more than one character");
            }

            const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, START_OFFSET, START_LINE, START_COLUMN);

            return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_CHARACTER_LITERAL, text, POSITION.length, POSITION);
        }

        characterCount += MAREA_INTEGER_SIZE(1);

        if (CURRENT == '\\') {
            MAREA_LexCursorAdvance(cursor);

            if (MAREA_LexCursorIsAtEnd(cursor)) {
                return MAREA_G_LexCharacterError(cursor, list, text, START_OFFSET, START_LINE, START_COLUMN, MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING, "unterminated character literal");
            }

            MAREA_G_LexCharacterEscape(cursor, START_OFFSET, START_LINE, START_COLUMN);

            continue;
        }

        if (MAREA_G_LexCharacterIsBadByte(CURRENT)) {
            const MAREA_IntegerSize BAD_START_OFFSET = cursor->offset;
            const MAREA_Uinteger32 BAD_START_LINE = cursor->line;
            const MAREA_Uinteger32 BAD_START_COLUMN = cursor->column;

            MAREA_LexCursorAdvance(cursor);

            MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE, MAREA_LexCursorMakePosition(cursor, BAD_START_OFFSET, BAD_START_LINE, BAD_START_COLUMN), "unexpected byte in character literal");

            continue;
        }

        MAREA_LexCursorAdvance(cursor);
    }
}
