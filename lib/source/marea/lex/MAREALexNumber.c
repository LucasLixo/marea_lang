// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/lex/MAREALexInternal.h"

#include <stdio.h>

static MAREA_Boolean MAREA_CALL MAREA_G_LexIsDecimalDigit(MAREA_Sinteger32 byte)
{
    return ((byte >= '0') && (byte <= '9')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_LexIsHexDigit(MAREA_Sinteger32 byte)
{
    return (MAREA_G_LexIsDecimalDigit(byte) || ((byte >= 'a') && (byte <= 'f')) || ((byte >= 'A') && (byte <= 'F'))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_LexIsBinaryDigit(MAREA_Sinteger32 byte)
{
    return ((byte == '0') || (byte == '1')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static void MAREA_CALL MAREA_G_LexSkipDigitsAndUnderscores(MAREA_LexCursor *cursor, MAREA_Boolean(MAREA_CALL *isDigit)(MAREA_Sinteger32), MAREA_IntegerSize *outDigitCount)
{
    MAREA_IntegerSize digitCount = MAREA_INTEGER_SIZE(0);

    while (!MAREA_LexCursorIsAtEnd(cursor)) {
        const MAREA_Sinteger32 BYTE = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

        if (isDigit(BYTE)) {
            digitCount = digitCount + MAREA_INTEGER_SIZE(1);

            MAREA_LexCursorAdvance(cursor);
        } else if (BYTE == '_') {
            MAREA_LexCursorAdvance(cursor);
        } else {
            break;
        }
    }

    if (outDigitCount != MAREA_NULLPTR) {
        *outDigitCount = digitCount;
    }
}

static MAREA_LexExcept MAREA_CALL MAREA_G_LexScanPrefixedInteger(MAREA_LexCursor *cursor, MAREA_TokenList *list, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn, MAREA_Boolean isHex)
{
    const MAREA_Character8 *text = &cursor->source[startOffset];

    MAREA_LexCursorAdvance(cursor);
    MAREA_LexCursorAdvance(cursor);

    MAREA_IntegerSize digitCount = MAREA_INTEGER_SIZE(0);

    MAREA_G_LexSkipDigitsAndUnderscores(cursor, isHex ? MAREA_G_LexIsHexDigit : MAREA_G_LexIsBinaryDigit, &digitCount);

    const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, startOffset, startLine, startColumn);

    if (digitCount == MAREA_INTEGER_SIZE(0)) {
        char buffer[96];

        MAREA_IGNORE(snprintf(buffer, sizeof(buffer), "malformed %s integer literal prefix", isHex ? "hexadecimal" : "binary"));

        MAREA_LexCursorReportDiagnostic(cursor, MAREA_LEX_DIAGNOSTIC_CODE_MALFORMED_NUMBER, POSITION, buffer);

        return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_ERROR, text, POSITION.length, POSITION);
    }

    return MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_INTEGER_LITERAL, text, POSITION.length, POSITION);
}

static MAREA_Boolean MAREA_CALL MAREA_G_LexTryScanExponent(MAREA_LexCursor *cursor)
{
    const MAREA_Sinteger32 MARKER = MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0));

    if ((MARKER != 'e') && (MARKER != 'E')) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize lookahead = MAREA_INTEGER_SIZE(1);

    MAREA_Sinteger32 afterMarker = MAREA_LexCursorPeek(cursor, lookahead);

    MAREA_Boolean hasSign = MAREA_BOOLEAN_FALSE;

    if ((afterMarker == '+') || (afterMarker == '-')) {
        hasSign = MAREA_BOOLEAN_TRUE;

        lookahead = lookahead + MAREA_INTEGER_SIZE(1);

        afterMarker = MAREA_LexCursorPeek(cursor, lookahead);
    }

    if (!MAREA_G_LexIsDecimalDigit(afterMarker)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_LexCursorAdvance(cursor);

    if (hasSign) {
        MAREA_LexCursorAdvance(cursor);
    }

    MAREA_G_LexSkipDigitsAndUnderscores(cursor, MAREA_G_LexIsDecimalDigit, MAREA_NULLPTR);

    return MAREA_BOOLEAN_TRUE;
}

MAREA_LexExcept MAREA_CALL MAREA_LexScanNumber(MAREA_LexCursor *cursor, MAREA_TokenList *list)
{
    const MAREA_IntegerSize START_OFFSET = cursor->offset;
    const MAREA_Uinteger32 START_LINE = cursor->line;
    const MAREA_Uinteger32 START_COLUMN = cursor->column;

    if ((MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0)) == '0') && (MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(1)) == 'x')) {
        return MAREA_G_LexScanPrefixedInteger(cursor, list, START_OFFSET, START_LINE, START_COLUMN, MAREA_BOOLEAN_TRUE);
    }

    if ((MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0)) == '0') && (MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(1)) == 'b')) {
        return MAREA_G_LexScanPrefixedInteger(cursor, list, START_OFFSET, START_LINE, START_COLUMN, MAREA_BOOLEAN_FALSE);
    }

    const MAREA_Character8 *text = &cursor->source[START_OFFSET];

    MAREA_G_LexSkipDigitsAndUnderscores(cursor, MAREA_G_LexIsDecimalDigit, MAREA_NULLPTR);

    MAREA_Boolean isFloat = MAREA_BOOLEAN_FALSE;

    if ((MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(0)) == '.') && MAREA_G_LexIsDecimalDigit(MAREA_LexCursorPeek(cursor, MAREA_INTEGER_SIZE(1)))) {
        isFloat = MAREA_BOOLEAN_TRUE;

        MAREA_LexCursorAdvance(cursor);

        MAREA_G_LexSkipDigitsAndUnderscores(cursor, MAREA_G_LexIsDecimalDigit, MAREA_NULLPTR);
    }

    if (MAREA_G_LexTryScanExponent(cursor)) {
        isFloat = MAREA_BOOLEAN_TRUE;
    }

    const MAREA_SourcePosition POSITION = MAREA_LexCursorMakePosition(cursor, START_OFFSET, START_LINE, START_COLUMN);

    return MAREA_LexTokenListAppend(list, isFloat ? MAREA_TOKEN_KIND_FLOAT_LITERAL : MAREA_TOKEN_KIND_INTEGER_LITERAL, text, POSITION.length, POSITION);
}
