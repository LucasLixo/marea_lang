// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/lex/MAREALexInternal.h"

static MAREA_Boolean MAREA_CALL MAREA_G_LexStringIsBadByte(MAREA_Sinteger32 byte)
{
    return ((byte >= 0x80) || ((byte < 0x20) && (byte != '\t')) || (byte == 0x7F)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_LexExcept MAREA_CALL MAREA_G_LexStringUnterminated(MAREA_LexCursor *cursor, MAREA_TokenList *list, const MAREA_Character8 *text, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn)
{
    const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn);

    MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING, POSITION, "unterminated string literal");

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

            const MAREA_Sinteger32 ESCAPE_CHARACTER = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

            if ((ESCAPE_CHARACTER == 'n') || (ESCAPE_CHARACTER == 't') || (ESCAPE_CHARACTER == 'r') || (ESCAPE_CHARACTER == '0') || (ESCAPE_CHARACTER == '\\') || (ESCAPE_CHARACTER == '"')) {
                MAREA_LexCursorAdvance(cursor);

                continue;
            }

            if (ESCAPE_CHARACTER == 'x') {
                MAREA_LexCursorAdvance(cursor);

                const MAREA_Sinteger32 FIRST_HEX = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));
                const MAREA_Sinteger32 SECOND_HEX = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(1));

                if (MAREA_LexIsHexDigit(FIRST_HEX) && MAREA_LexIsHexDigit(SECOND_HEX)) {
                    MAREA_LexCursorAdvance(cursor);
                    MAREA_LexCursorAdvance(cursor);

                    continue;
                }

                const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, START_OFFSET, START_LINE, START_COLUMN);

                MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE, POSITION, "invalid hex escape sequence");

                continue;
            }

            const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, START_OFFSET, START_LINE, START_COLUMN);

            MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE, POSITION, "invalid escape sequence");

            MAREA_LexCursorAdvance(cursor);

            continue;
        }

        if (MAREA_G_LexStringIsBadByte(CURRENT)) {
            const MAREA_IntegerSize BAD_START_OFFSET = cursor->offset;
            const MAREA_Uinteger32 BAD_START_LINE = cursor->line;
            const MAREA_Uinteger32 BAD_START_COLUMN = cursor->column;

            MAREA_LexCursorAdvance(cursor);

            const MAREA_SourcePosition BAD_POSITION = MAREA_LexCursorMakePosition(cursor, BAD_START_OFFSET, BAD_START_LINE, BAD_START_COLUMN);

            MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE, BAD_POSITION, "unexpected byte in string literal");

            continue;
        }

        MAREA_LexCursorAdvance(cursor);
    }
}
