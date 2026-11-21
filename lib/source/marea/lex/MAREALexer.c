// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/lex/MAREALexInternal.h"

#include <stdint.h>
#include <string.h>

#if !defined(MAREA_LEX_TOKEN_LIST_INITIAL_CAPACITY)
    #define MAREA_LEX_TOKEN_LIST_INITIAL_CAPACITY MAREA_INTEGER_SIZE(64)
#endif

MAREA_Sinteger32 MAREA_CALL MAREA_LexCursorPeek(const MAREA_LexCursor *cursor, MAREA_IntegerSize offsetFromCurrent)
{
    const MAREA_IntegerSize position = cursor->offset + offsetFromCurrent;

    if (position >= cursor->length) {
        return -1;
    }

    return (MAREA_Sinteger32)(MAREA_Uinteger8)cursor->source[position];
}

MAREA_Boolean MAREA_CALL MAREA_LexCursorIsAtEnd(const MAREA_LexCursor *cursor)
{
    return (cursor->offset >= cursor->length) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

void MAREA_CALL MAREA_LexCursorAdvance(MAREA_LexCursor *cursor)
{
    if (cursor->source[cursor->offset] == '\n') {
        cursor->line = cursor->line + MAREA_UINTEGER_32(1);
        cursor->column = MAREA_UINTEGER_32(1);
    } else {
        cursor->column = cursor->column + MAREA_UINTEGER_32(1);
    }

    cursor->offset = cursor->offset + MAREA_INTEGER_SIZE(1);
}

MAREA_SourcePosition MAREA_CALL MAREA_LexCursorMakePosition(const MAREA_LexCursor *cursor, MAREA_IntegerSize startOffset, MAREA_Uinteger32 startLine, MAREA_Uinteger32 startColumn)
{
    MAREA_SourcePosition position = {
        MAREA_LexContextGetFileName(cursor->context),
        (MAREA_Uinteger32)startOffset,
        startLine,
        startColumn,
        (MAREA_Uinteger32)(cursor->offset - startOffset),
    };

    return position;
}

void MAREA_CALL MAREA_LexCursorReportDiagnostic(const MAREA_LexCursor *cursor, MAREA_LexDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *message)
{
    MAREA_IGNORE(MAREA_DiagnosticSinkAdd(MAREA_LexContextGetDiagnostics(cursor->context), (MAREA_Uinteger32)code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, message));
}

static MAREA_LexExcept MAREA_CALL MAREA_G_LexTokenListGrow(MAREA_TokenList *list)
{
    const MAREA_IntegerSize newCapacity = (list->capacity == MAREA_INTEGER_SIZE(0)) ? MAREA_LEX_TOKEN_LIST_INITIAL_CAPACITY : (list->capacity * MAREA_INTEGER_SIZE(2));

    MAREA_Null newItemsMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(list->arena, (newCapacity * sizeof(MAREA_Token)), MAREA_ALIGNOF(MAREA_Token), &newItemsMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Token *newItems = newItemsMemory;

    if (list->count > MAREA_INTEGER_SIZE(0)) {
        memcpy(newItems, list->items, (list->count * sizeof(MAREA_Token)));
    }

    list->items = newItems;
    list->capacity = newCapacity;

    return MAREA_LEX_EXCEPT_SUCCESS;
}

MAREA_LexExcept MAREA_CALL MAREA_LexTokenListAppend(MAREA_TokenList *list, MAREA_TokenKind kind, const MAREA_Character8 *text, MAREA_Uinteger32 length, MAREA_SourcePosition position)
{
    if (list->count == list->capacity) {
        const MAREA_LexExcept growResult = MAREA_G_LexTokenListGrow(list);

        if (growResult != MAREA_LEX_EXCEPT_SUCCESS) {
            return growResult;
        }
    }

    MAREA_Token *token = &list->items[list->count];

    token->position = position;
    token->text = text;
    token->length = length;
    token->kind = kind;

    list->count = list->count + MAREA_INTEGER_SIZE(1);

    return MAREA_LEX_EXCEPT_SUCCESS;
}

MAREA_TokenKind MAREA_CALL MAREA_TokenGetKind(const MAREA_Token *token)
{
    if (token == MAREA_NULLPTR) {
        return MAREA_TOKEN_KIND_ERROR;
    }

    return token->kind;
}

const MAREA_Character8 *MAREA_CALL MAREA_TokenGetText(const MAREA_Token *token)
{
    if (token == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return token->text;
}

MAREA_Uinteger32 MAREA_CALL MAREA_TokenGetLength(const MAREA_Token *token)
{
    if (token == MAREA_NULLPTR) {
        return MAREA_UINTEGER_32(0);
    }

    return token->length;
}

MAREA_SourcePosition MAREA_CALL MAREA_TokenGetPosition(const MAREA_Token *token)
{
    if (token == MAREA_NULLPTR) {
        MAREA_SourcePosition zeroed = { MAREA_NULLPTR, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0) };

        return zeroed;
    }

    return token->position;
}

MAREA_IntegerSize MAREA_CALL MAREA_TokenListGetCount(const MAREA_TokenList *list)
{
    if (list == MAREA_NULLPTR) {
        return MAREA_INTEGER_SIZE(0);
    }

    return list->count;
}

MAREA_LexExcept MAREA_CALL MAREA_TokenListGetAt(const MAREA_TokenList *list, MAREA_IntegerSize index, const MAREA_Token **out)
{
    if ((list == MAREA_NULLPTR) || (out == MAREA_NULLPTR)) {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    if (index >= list->count) {
        return MAREA_LEX_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    *out = &list->items[index];

    return MAREA_LEX_EXCEPT_SUCCESS;
}

MAREA_LexExcept MAREA_CALL MAREA_LexContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_LexContext **outContext)
{
    if ((arena == MAREA_NULLPTR) || (sink == MAREA_NULLPTR) || (outContext == MAREA_NULLPTR)) {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null contextMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_LexContext), MAREA_ALIGNOF(MAREA_LexContext), &contextMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_LexContext *context = contextMemory;

    context->arena = arena;
    context->sink = sink;
    context->fileName = "<source>";

    *outContext = context;

    return MAREA_LEX_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_LexContextGetArena(const MAREA_LexContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_LexContextGetDiagnostics(const MAREA_LexContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_LexExcept MAREA_CALL MAREA_LexContextSetFileName(MAREA_LexContext *context, const MAREA_Character8 *name)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    context->fileName = name;

    return MAREA_LEX_EXCEPT_SUCCESS;
}

const MAREA_Character8 *MAREA_CALL MAREA_LexContextGetFileName(const MAREA_LexContext *context)
{
    if (context == MAREA_NULLPTR) {
        return "<source>";
    }

    return context->fileName;
}

static MAREA_Boolean MAREA_CALL MAREA_G_LexIsIdentifierStart(MAREA_Sinteger32 byte)
{
    return (((byte >= 'A') && (byte <= 'Z')) || ((byte >= 'a') && (byte <= 'z')) || (byte == '_')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_LexIsIdentifierContinue(MAREA_Sinteger32 byte)
{
    return (MAREA_G_LexIsIdentifierStart(byte) || ((byte >= '0') && (byte <= '9'))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_LexIsWhitespace(MAREA_Sinteger32 byte)
{
    return ((byte == ' ') || (byte == '\t') || (byte == '\r') || (byte == '\n')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

typedef struct MAREA_G_LexSymbolRow
{
    const MAREA_Character8 *text;
    MAREA_IntegerSize length;
    MAREA_TokenKind kind;
} MAREA_G_LexSymbolRow;

static const MAREA_G_LexSymbolRow MAREA_G_LexSymbolsLength3[] = {
    { "**=", 3, MAREA_TOKEN_KIND_STAR_STAR_EQUAL },
    { "<<=", 3, MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL },
    { ">>=", 3, MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL },
    { "#![", 3, MAREA_TOKEN_KIND_HASH_BANG_BRACKET },
};

static const MAREA_G_LexSymbolRow MAREA_G_LexSymbolsLength2[] = {
    { "**", 2, MAREA_TOKEN_KIND_STAR_STAR },
    { "+%", 2, MAREA_TOKEN_KIND_PLUS_PERCENT },
    { "-%", 2, MAREA_TOKEN_KIND_MINUS_PERCENT },
    { "*%", 2, MAREA_TOKEN_KIND_STAR_PERCENT },
    { "+|", 2, MAREA_TOKEN_KIND_PLUS_PIPE },
    { "-|", 2, MAREA_TOKEN_KIND_MINUS_PIPE },
    { "*|", 2, MAREA_TOKEN_KIND_STAR_PIPE },
    { "<<", 2, MAREA_TOKEN_KIND_SHIFT_LEFT },
    { ">>", 2, MAREA_TOKEN_KIND_SHIFT_RIGHT },
    { "==", 2, MAREA_TOKEN_KIND_EQUAL_EQUAL },
    { "!=", 2, MAREA_TOKEN_KIND_BANG_EQUAL },
    { "<=", 2, MAREA_TOKEN_KIND_LESS_EQUAL },
    { ">=", 2, MAREA_TOKEN_KIND_GREATER_EQUAL },
    { "+=", 2, MAREA_TOKEN_KIND_PLUS_EQUAL },
    { "-=", 2, MAREA_TOKEN_KIND_MINUS_EQUAL },
    { "*=", 2, MAREA_TOKEN_KIND_STAR_EQUAL },
    { "/=", 2, MAREA_TOKEN_KIND_SLASH_EQUAL },
    { "%=", 2, MAREA_TOKEN_KIND_PERCENT_EQUAL },
    { "&=", 2, MAREA_TOKEN_KIND_AMPERSAND_EQUAL },
    { "|=", 2, MAREA_TOKEN_KIND_PIPE_EQUAL },
    { "^=", 2, MAREA_TOKEN_KIND_CARET_EQUAL },
    { "..", 2, MAREA_TOKEN_KIND_DOT_DOT },
    { "#[", 2, MAREA_TOKEN_KIND_HASH_BRACKET },
};

static const MAREA_G_LexSymbolRow MAREA_G_LexSymbolsLength1[] = {
    { "+", 1, MAREA_TOKEN_KIND_PLUS },
    { "-", 1, MAREA_TOKEN_KIND_MINUS },
    { "*", 1, MAREA_TOKEN_KIND_STAR },
    { "/", 1, MAREA_TOKEN_KIND_SLASH },
    { "%", 1, MAREA_TOKEN_KIND_PERCENT },
    { "~", 1, MAREA_TOKEN_KIND_TILDE },
    { "&", 1, MAREA_TOKEN_KIND_AMPERSAND },
    { "|", 1, MAREA_TOKEN_KIND_PIPE },
    { "^", 1, MAREA_TOKEN_KIND_CARET },
    { "<", 1, MAREA_TOKEN_KIND_LESS },
    { ">", 1, MAREA_TOKEN_KIND_GREATER },
    { "=", 1, MAREA_TOKEN_KIND_EQUAL },
    { "(", 1, MAREA_TOKEN_KIND_LEFT_PAREN },
    { ")", 1, MAREA_TOKEN_KIND_RIGHT_PAREN },
    { "[", 1, MAREA_TOKEN_KIND_LEFT_BRACKET },
    { "]", 1, MAREA_TOKEN_KIND_RIGHT_BRACKET },
    { "{", 1, MAREA_TOKEN_KIND_LEFT_BRACE },
    { "}", 1, MAREA_TOKEN_KIND_RIGHT_BRACE },
    { ",", 1, MAREA_TOKEN_KIND_COMMA },
    { ";", 1, MAREA_TOKEN_KIND_SEMICOLON },
    { ":", 1, MAREA_TOKEN_KIND_COLON },
    { ".", 1, MAREA_TOKEN_KIND_DOT },
    { "!", 1, MAREA_TOKEN_KIND_BANG },
};

static MAREA_Boolean MAREA_CALL MAREA_G_LexTryMatchSymbol(const MAREA_LexCursor *cursor, const MAREA_G_LexSymbolRow *table, MAREA_IntegerSize tableLength, MAREA_IntegerSize symbolLength, MAREA_TokenKind *outKind)
{
    if ((cursor->offset + symbolLength) > cursor->length) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < tableLength; index = index + MAREA_INTEGER_SIZE(1)) {
        if (memcmp(&cursor->source[cursor->offset], table[index].text, symbolLength) == 0) {
            *outKind = table[index].kind;

            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_LexExcept MAREA_CALL MAREA_LexTokenize(MAREA_LexContext *context, const MAREA_Character8 *source, MAREA_IntegerSize length, MAREA_TokenList **outList)
{
    if ((context == MAREA_NULLPTR) || (outList == MAREA_NULLPTR) || ((source == MAREA_NULLPTR) && (length > MAREA_INTEGER_SIZE(0)))) {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    if (length > (MAREA_IntegerSize)UINT32_MAX) {
        return MAREA_LEX_EXCEPT_ERROR_SOURCE_TOO_LARGE;
    }

    MAREA_Arena *arena = MAREA_LexContextGetArena(context);

    MAREA_Null listMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_TokenList), MAREA_ALIGNOF(MAREA_TokenList), &listMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_LEX_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_TokenList *list = listMemory;

    list->arena = arena;
    list->items = MAREA_NULLPTR;
    list->count = MAREA_INTEGER_SIZE(0);
    list->capacity = MAREA_INTEGER_SIZE(0);

    MAREA_LexCursor cursor = { context, source, length, MAREA_INTEGER_SIZE(0), MAREA_UINTEGER_32(1), MAREA_UINTEGER_32(1) };

    while (!MAREA_LexCursorIsAtEnd(&cursor)) {
        const MAREA_Sinteger32 CURRENT = MAREA_LexCursorPeek(&cursor, MAREA_INTEGER_SIZE(0));

        if (MAREA_G_LexIsWhitespace(CURRENT)) {
            MAREA_LexCursorAdvance(&cursor);

            continue;
        }

        if ((CURRENT == '-') && (MAREA_LexCursorPeek(&cursor, MAREA_INTEGER_SIZE(1)) == '-')) {
            while (!MAREA_LexCursorIsAtEnd(&cursor) && (MAREA_LexCursorPeek(&cursor, MAREA_INTEGER_SIZE(0)) != '\n')) {
                MAREA_LexCursorAdvance(&cursor);
            }

            continue;
        }

        const MAREA_IntegerSize START_OFFSET = cursor.offset;
        const MAREA_Uinteger32 START_LINE = cursor.line;
        const MAREA_Uinteger32 START_COLUMN = cursor.column;

        if (MAREA_G_LexIsIdentifierStart(CURRENT)) {
            while (!MAREA_LexCursorIsAtEnd(&cursor) && MAREA_G_LexIsIdentifierContinue(MAREA_LexCursorPeek(&cursor, MAREA_INTEGER_SIZE(0)))) {
                MAREA_LexCursorAdvance(&cursor);
            }

            const MAREA_Character8 *text = &source[START_OFFSET];
            const MAREA_Uinteger32 textLength = (MAREA_Uinteger32)(cursor.offset - START_OFFSET);

            const MAREA_TokenKind kind = MAREA_TokenKindLookupKeyword(text, textLength);

            const MAREA_LexExcept appendResult = MAREA_LexTokenListAppend(list, kind, text, textLength, MAREA_LexCursorMakePosition(&cursor, START_OFFSET, START_LINE, START_COLUMN));

            if (appendResult != MAREA_LEX_EXCEPT_SUCCESS) {
                return appendResult;
            }

            continue;
        }

        MAREA_TokenKind symbolKind;
        MAREA_IntegerSize symbolLength = MAREA_INTEGER_SIZE(0);

        if (MAREA_G_LexTryMatchSymbol(&cursor, MAREA_G_LexSymbolsLength3, MAREA_ARRAY_SIZE(MAREA_G_LexSymbolsLength3), MAREA_INTEGER_SIZE(3), &symbolKind)) {
            symbolLength = MAREA_INTEGER_SIZE(3);
        } else if (MAREA_G_LexTryMatchSymbol(&cursor, MAREA_G_LexSymbolsLength2, MAREA_ARRAY_SIZE(MAREA_G_LexSymbolsLength2), MAREA_INTEGER_SIZE(2), &symbolKind)) {
            symbolLength = MAREA_INTEGER_SIZE(2);
        } else if (MAREA_G_LexTryMatchSymbol(&cursor, MAREA_G_LexSymbolsLength1, MAREA_ARRAY_SIZE(MAREA_G_LexSymbolsLength1), MAREA_INTEGER_SIZE(1), &symbolKind)) {
            symbolLength = MAREA_INTEGER_SIZE(1);
        }

        if (symbolLength > MAREA_INTEGER_SIZE(0)) {
            const MAREA_Character8 *text = &source[START_OFFSET];

            for (MAREA_IntegerSize consumed = MAREA_INTEGER_SIZE(0); consumed < symbolLength; consumed = consumed + MAREA_INTEGER_SIZE(1)) {
                MAREA_LexCursorAdvance(&cursor);
            }

            const MAREA_LexExcept appendResult = MAREA_LexTokenListAppend(list, symbolKind, text, (MAREA_Uinteger32)symbolLength, MAREA_LexCursorMakePosition(&cursor, START_OFFSET, START_LINE, START_COLUMN));

            if (appendResult != MAREA_LEX_EXCEPT_SUCCESS) {
                return appendResult;
            }

            continue;
        }

        // TODO(#1): digits and quotes are treated as unknown bytes until the number and string
        // scanners land.
        MAREA_LexCursorAdvance(&cursor);

        const MAREA_SourcePosition BAD_BYTE_POSITION = MAREA_LexCursorMakePosition(&cursor, START_OFFSET, START_LINE, START_COLUMN);

        MAREA_LexCursorReportDiagnostic(&cursor, MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE, BAD_BYTE_POSITION, "unexpected byte in source");

        const MAREA_LexExcept appendResult = MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_ERROR, &source[START_OFFSET], MAREA_UINTEGER_32(1), BAD_BYTE_POSITION);

        if (appendResult != MAREA_LEX_EXCEPT_SUCCESS) {
            return appendResult;
        }
    }

    const MAREA_LexExcept endOfFileResult = MAREA_LexTokenListAppend(list, MAREA_TOKEN_KIND_END_OF_FILE, MAREA_NULLPTR, MAREA_UINTEGER_32(0), MAREA_LexCursorMakePosition(&cursor, cursor.offset, cursor.line, cursor.column));

    if (endOfFileResult != MAREA_LEX_EXCEPT_SUCCESS) {
        return endOfFileResult;
    }

    *outList = list;

    return MAREA_LEX_EXCEPT_SUCCESS;
}
