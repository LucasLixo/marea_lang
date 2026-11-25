// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/lex/MAREALexInternal.h"

static MAREA_LexExcept MAREA_CALL MAREA_G_LexCharacterError(MAREA_LexCursor *cursor, MAREA_TokenList *list, const MAREA_Character8 *text, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn, MAREA_Uinteger32 code, const MAREA_Character8 *message)
{
    const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn);

    const MAREA_LexExcept reportResult = MAREA_LexCursorReportDiagnostic(cursor, code, POSITION, message);

    if (reportResult != MAREA_LEX_EXCEPT_SUCCESS) {
        return reportResult;
    }

    return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_ERROR, text, POSITION.length, POSITION);
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

            const MAREA_LexExcept escapeResult = MAREA_LexScanEscape(cursor, START_OFFSET, START_LINE, START_COLUMN, MAREA_BOOLEAN_TRUE);

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

            const MAREA_LexExcept reportResult = MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE, MAREA_LexCursorMakePosition(cursor, BAD_START_OFFSET, BAD_START_LINE, BAD_START_COLUMN), "unexpected byte in character literal");

            if (reportResult != MAREA_LEX_EXCEPT_SUCCESS) {
                return reportResult;
            }

            continue;
        }

        MAREA_LexCursorAdvance(cursor);
    }
}
