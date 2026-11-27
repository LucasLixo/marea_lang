// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

const MAREA_Token *MAREA_CALL MAREA_ParsePeekToken(const MAREA_ParseState *state, MAREA_IntegerSize ahead)
{
    MAREA_IntegerSize position = (state->index + ahead);

    if (position >= state->count) {
        position = (state->count - MAREA_INTEGER_SIZE(1));
    }

    const MAREA_Token *token = MAREA_NULLPTR;

    // INFO: The position is in range by the clamp above, so the lookup cannot fail.
    MAREA_IGNORE(MAREA_TokenListGetAt(state->list, position, &token));

    return token;
}

MAREA_TokenKind MAREA_CALL MAREA_ParsePeekKind(const MAREA_ParseState *state, MAREA_IntegerSize ahead)
{
    if ((ahead == MAREA_INTEGER_SIZE(0)) && (state->halfGreater == MAREA_BOOLEAN_TRUE)) {
        return MAREA_TOKEN_KIND_GREATER;
    }

    return MAREA_TokenGetKind(MAREA_ParsePeekToken(state, ahead));
}

MAREA_Boolean MAREA_CALL MAREA_ParseAt(const MAREA_ParseState *state, MAREA_TokenKind kind)
{
    return (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0)) == kind) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

const MAREA_Token *MAREA_CALL MAREA_ParseAdvance(MAREA_ParseState *state)
{
    const MAREA_Token *token = MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0));

    if (MAREA_TokenGetKind(token) == MAREA_TOKEN_KIND_END_OF_FILE) {
        return token;
    }

    state->lastEnd = (MAREA_TokenGetPosition(token).offset + MAREA_TokenGetLength(token));
    state->halfGreater = MAREA_BOOLEAN_FALSE;
    state->index = (state->index + MAREA_INTEGER_SIZE(1));

    return token;
}

MAREA_Boolean MAREA_CALL MAREA_ParseAccept(MAREA_ParseState *state, MAREA_TokenKind kind)
{
    if (MAREA_ParseAt(state, kind) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_ParseExpect(MAREA_ParseState *state, MAREA_TokenKind kind, const MAREA_Character8 *message)
{
    if (MAREA_ParseAccept(state, kind) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), message);

    return MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_ParseExpectGreater(MAREA_ParseState *state, const MAREA_Character8 *message)
{
    if (state->halfGreater == MAREA_BOOLEAN_TRUE) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));

        return MAREA_BOOLEAN_TRUE;
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_GREATER) == MAREA_BOOLEAN_TRUE) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));

        return MAREA_BOOLEAN_TRUE;
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_SHIFT_RIGHT) == MAREA_BOOLEAN_TRUE) {
        const MAREA_Token *token = MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0));

        state->lastEnd = (MAREA_TokenGetPosition(token).offset + MAREA_UINTEGER_32(1));
        state->halfGreater = MAREA_BOOLEAN_TRUE;

        return MAREA_BOOLEAN_TRUE;
    }

    MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), message);

    return MAREA_BOOLEAN_FALSE;
}

void MAREA_CALL MAREA_ParseReport(MAREA_ParseState *state, MAREA_ParseDiagnosticCode code, const MAREA_Token *token, const MAREA_Character8 *message)
{
    const MAREA_SourcePosition position = MAREA_TokenGetPosition(token);

    if ((state->hasError == MAREA_BOOLEAN_TRUE) && (state->lastErrorOffset == position.offset)) {
        return;
    }

    state->hasError = MAREA_BOOLEAN_TRUE;
    state->lastErrorOffset = position.offset;

    MAREA_IGNORE(MAREA_DiagnosticSinkAdd(state->context->sink, (MAREA_Uinteger32)code, MAREA_DIAGNOSTIC_SEVERITY_ERROR, position, message));
}

MAREA_Boolean MAREA_CALL MAREA_ParseEnter(MAREA_ParseState *state)
{
    if (state->depth >= MAREA_PARSE_DEPTH_LIMIT) {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_RECURSION_TOO_DEEP, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "nesting is too deep");

        return MAREA_BOOLEAN_FALSE;
    }

    state->depth = (state->depth + MAREA_UINTEGER_32(1));

    return MAREA_BOOLEAN_TRUE;
}

void MAREA_CALL MAREA_ParseLeave(MAREA_ParseState *state)
{
    state->depth = (state->depth - MAREA_UINTEGER_32(1));
}

MAREA_Boolean MAREA_CALL MAREA_ParseIsSynchronizationKind(MAREA_TokenKind kind)
{
    switch (kind) {
    case MAREA_TOKEN_KIND_KEYWORD_END:
    case MAREA_TOKEN_KIND_KEYWORD_CASE:
    case MAREA_TOKEN_KIND_KEYWORD_DEFAULT:
    case MAREA_TOKEN_KIND_KEYWORD_ELSEIF:
    case MAREA_TOKEN_KIND_KEYWORD_ELSE:
    case MAREA_TOKEN_KIND_KEYWORD_CONSTANT:
    case MAREA_TOKEN_KIND_KEYWORD_STRUCTURE:
    case MAREA_TOKEN_KIND_KEYWORD_OPAQUE:
    case MAREA_TOKEN_KIND_KEYWORD_ENUM:
    case MAREA_TOKEN_KIND_KEYWORD_TYPE:
    case MAREA_TOKEN_KIND_KEYWORD_CONTRACT:
    case MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION:
    case MAREA_TOKEN_KIND_KEYWORD_FUNCTION:
    case MAREA_TOKEN_KIND_KEYWORD_IMPORT:
    case MAREA_TOKEN_KIND_KEYWORD_REQUIRE:
    case MAREA_TOKEN_KIND_KEYWORD_LOCAL:
    case MAREA_TOKEN_KIND_KEYWORD_IF:
    case MAREA_TOKEN_KIND_KEYWORD_WHILE:
    case MAREA_TOKEN_KIND_KEYWORD_FOR:
    case MAREA_TOKEN_KIND_KEYWORD_SWITCH:
    case MAREA_TOKEN_KIND_KEYWORD_WITH:
    case MAREA_TOKEN_KIND_KEYWORD_PARTITION:
    case MAREA_TOKEN_KIND_KEYWORD_RETURN:
    case MAREA_TOKEN_KIND_KEYWORD_BREAK:
    case MAREA_TOKEN_KIND_KEYWORD_CONTINUE:
    case MAREA_TOKEN_KIND_KEYWORD_DO:
    case MAREA_TOKEN_KIND_KEYWORD_UNSAFE:
    case MAREA_TOKEN_KIND_KEYWORD_FORK_JOIN:
    case MAREA_TOKEN_KIND_HASH_BRACKET:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

void MAREA_CALL MAREA_ParseSynchronize(MAREA_ParseState *state)
{
    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if (kind == MAREA_TOKEN_KIND_END_OF_FILE) {
            return;
        }

        if (kind == MAREA_TOKEN_KIND_SEMICOLON) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));

            return;
        }

        if (MAREA_ParseIsSynchronizationKind(kind) == MAREA_BOOLEAN_TRUE) {
            return;
        }

        MAREA_IGNORE(MAREA_ParseAdvance(state));
    }
}

MAREA_AstNode *MAREA_CALL MAREA_ParseNodeBegin(MAREA_ParseState *state, MAREA_AstKind kind)
{
    MAREA_AstNode *node = MAREA_NULLPTR;

    const MAREA_Token *token = MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0));

    if (MAREA_AstNodeCreate(state->context->arena, kind, MAREA_TokenGetPosition(token), MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node) != MAREA_AST_EXCEPT_SUCCESS) {
        state->outOfMemory = MAREA_BOOLEAN_TRUE;

        return MAREA_NULLPTR;
    }

    return node;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseNodeBeginAt(MAREA_ParseState *state, MAREA_AstKind kind, MAREA_SourcePosition position)
{
    MAREA_AstNode *node = MAREA_NULLPTR;

    if (MAREA_AstNodeCreate(state->context->arena, kind, position, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node) != MAREA_AST_EXCEPT_SUCCESS) {
        state->outOfMemory = MAREA_BOOLEAN_TRUE;

        return MAREA_NULLPTR;
    }

    return node;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseNodeFromToken(MAREA_ParseState *state, MAREA_AstKind kind, const MAREA_Token *token)
{
    MAREA_AstNode *node = MAREA_NULLPTR;

    if (MAREA_AstNodeCreate(state->context->arena, kind, MAREA_TokenGetPosition(token), MAREA_TokenGetText(token), MAREA_TokenGetLength(token), &node) != MAREA_AST_EXCEPT_SUCCESS) {
        state->outOfMemory = MAREA_BOOLEAN_TRUE;

        return MAREA_NULLPTR;
    }

    return node;
}

void MAREA_CALL MAREA_ParseNodeEnd(const MAREA_ParseState *state, MAREA_AstNode *node)
{
    MAREA_AstNodeSetEnd(node, state->lastEnd);
}

void MAREA_CALL MAREA_ParseNodeAdd(MAREA_ParseState *state, MAREA_AstNode *parent, MAREA_AstNode *child)
{
    if ((parent == MAREA_NULLPTR) || (child == MAREA_NULLPTR)) {
        return;
    }

    if (MAREA_AstNodeAppendChild(state->context->arena, parent, child) != MAREA_AST_EXCEPT_SUCCESS) {
        state->outOfMemory = MAREA_BOOLEAN_TRUE;
    }
}

MAREA_AstNode *MAREA_CALL MAREA_ParseErrorNode(MAREA_ParseState *state)
{
    return MAREA_ParseNodeBegin(state, MAREA_AST_KIND_ERROR_NODE);
}

void MAREA_CALL MAREA_ParseAdoptAttributes(MAREA_ParseState *state, MAREA_AstNode *node, const MAREA_AstNode *holder)
{
    const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(holder);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstNode *attribute = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(holder, index, &attribute) == MAREA_AST_EXCEPT_SUCCESS) {
            MAREA_ParseNodeAdd(state, node, (MAREA_AstNode *)attribute);
        }
    }
}

// INFO: A member loop must move on every pass, so a bad member is marked, skipped to a safe token and then forced forward.
void MAREA_CALL MAREA_ParseSkipBadMember(MAREA_ParseState *state, MAREA_IntegerSize before, MAREA_AstNode *parent)
{
    MAREA_ParseNodeAdd(state, parent, MAREA_ParseErrorNode(state));

    MAREA_ParseSynchronize(state);

    if (state->index == before) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));
    }
}
