// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

static MAREA_Boolean MAREA_CALL MAREA_G_IsAssignmentOperator(MAREA_TokenKind kind)
{
    switch (kind) {
    case MAREA_TOKEN_KIND_EQUAL:
    case MAREA_TOKEN_KIND_PLUS_EQUAL:
    case MAREA_TOKEN_KIND_MINUS_EQUAL:
    case MAREA_TOKEN_KIND_STAR_EQUAL:
    case MAREA_TOKEN_KIND_SLASH_EQUAL:
    case MAREA_TOKEN_KIND_PERCENT_EQUAL:
    case MAREA_TOKEN_KIND_STAR_STAR_EQUAL:
    case MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL:
    case MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL:
    case MAREA_TOKEN_KIND_AMPERSAND_EQUAL:
    case MAREA_TOKEN_KIND_PIPE_EQUAL:
    case MAREA_TOKEN_KIND_CARET_EQUAL:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_AstNode *MAREA_CALL MAREA_ParseBlock(MAREA_ParseState *state)
{
    MAREA_AstNode *block = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_BLOCK);

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_END_OF_FILE)) {
            break;
        }

        // INFO: A declaration word inside a body means the body lost its end, so the declaration is left to the caller.
        if ((MAREA_ParseIsDeclarationStart(kind) == MAREA_BOOLEAN_TRUE) || (kind == MAREA_TOKEN_KIND_HASH_BRACKET)) {
            break;
        }

        // INFO: A branch word with no end before it is the old shared-end form: report it and skip the word.
        if ((kind == MAREA_TOKEN_KIND_KEYWORD_ELSEIF) || (kind == MAREA_TOKEN_KIND_KEYWORD_ELSE) || (kind == MAREA_TOKEN_KIND_KEYWORD_CASE) || (kind == MAREA_TOKEN_KIND_KEYWORD_DEFAULT)) {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected 'end' before this branch");

            MAREA_ParseNodeAdd(state, block, MAREA_ParseErrorNode(state));

            MAREA_IGNORE(MAREA_ParseAdvance(state));

            continue;
        }

        const MAREA_IntegerSize before = state->index;

        MAREA_ParseNodeAdd(state, block, MAREA_ParseStatement(state));

        if (state->index == before) {
            MAREA_ParseSynchronize(state);

            if (state->index == before) {
                MAREA_IGNORE(MAREA_ParseAdvance(state));
            }
        }
    }

    MAREA_ParseNodeEnd(state, block);

    return block;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseLocal(MAREA_ParseState *state)
{
    MAREA_AstNode *local = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_LOCAL_DECLARATION);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, local, MAREA_ParseIdentifier(state, "expected a local name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the local name"));

    MAREA_ParseNodeAdd(state, local, MAREA_ParseType(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_EQUAL) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, local, MAREA_ParseExpression(state));
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the local declaration"));

    MAREA_ParseNodeEnd(state, local);

    return local;
}

// INFO: Every compound statement reads: keyword, header, body block, end. This closes the last two steps.
static void MAREA_CALL MAREA_G_ParseBodyAndEnd(MAREA_ParseState *state, MAREA_AstNode *node, const MAREA_Character8 *endMessage)
{
    MAREA_ParseNodeAdd(state, node, MAREA_ParseBlock(state));

    MAREA_ParseExpectEnd(state, endMessage);

    MAREA_ParseNodeEnd(state, node);
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseIf(MAREA_ParseState *state)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_IF_STATEMENT);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_THEN, "expected 'then' after the condition"));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseBlock(state));

    MAREA_ParseExpectEnd(state, "expected 'end' to close the if branch");

    while (MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_ELSEIF) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNode *clause = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_ELSEIF_CLAUSE);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, clause, MAREA_ParseExpression(state));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_THEN, "expected 'then' after the condition"));

        MAREA_ParseNodeAdd(state, clause, MAREA_ParseBlock(state));

        MAREA_ParseExpectEnd(state, "expected 'end' to close the elseif branch");

        MAREA_ParseNodeEnd(state, clause);

        MAREA_ParseNodeAdd(state, statement, clause);
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_ELSE) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNode *clause = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_ELSE_CLAUSE);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, clause, MAREA_ParseBlock(state));

        MAREA_ParseExpectEnd(state, "expected 'end' to close the else branch");

        MAREA_ParseNodeEnd(state, clause);

        MAREA_ParseNodeAdd(state, statement, clause);
    }

    MAREA_ParseExpectEnd(state, "expected 'end' to close the if");

    MAREA_ParseNodeEnd(state, statement);

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWhile(MAREA_ParseState *state)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_WHILE_STATEMENT);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_DO, "expected 'do' after the condition"));

    MAREA_G_ParseBodyAndEnd(state, statement, "expected 'end' to close the while");

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseFor(MAREA_ParseState *state)
{
    if (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_KEYWORD_EACH) {
        MAREA_AstNode *each = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_FOR_EACH_STATEMENT);

        MAREA_IGNORE(MAREA_ParseAdvance(state));
        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, each, MAREA_ParseIdentifier(state, "expected a loop variable name"));

        if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE) {
            MAREA_ParseNodeAdd(state, each, MAREA_ParseIdentifier(state, "expected a loop variable name"));
        }

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_IN, "expected 'in' after the loop variables"));

        if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_MUTABLE) == MAREA_BOOLEAN_TRUE) {
            MAREA_AstNodeSetFlags(each, MAREA_AST_FLAG_MUTABLE);
        }

        MAREA_ParseNodeAdd(state, each, MAREA_ParseExpression(state));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_DO, "expected 'do' after the collection"));

        MAREA_G_ParseBodyAndEnd(state, each, "expected 'end' to close the for each");

        return each;
    }

    MAREA_AstNode *range = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_FOR_RANGE_STATEMENT);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, range, MAREA_ParseIdentifier(state, "expected a loop variable name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_IN, "expected 'in' after the loop variable"));

    MAREA_ParseNodeAdd(state, range, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_DOT_DOT, "expected '..' between the range ends"));

    MAREA_ParseNodeAdd(state, range, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_DO, "expected 'do' after the range"));

    MAREA_G_ParseBodyAndEnd(state, range, "expected 'end' to close the for");

    return range;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseSwitch(MAREA_ParseState *state)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_SWITCH_STATEMENT);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

    MAREA_Boolean sawDefault = MAREA_BOOLEAN_FALSE;

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_END_OF_FILE)) {
            break;
        }

        if ((MAREA_ParseIsDeclarationStart(kind) == MAREA_BOOLEAN_TRUE) || (kind == MAREA_TOKEN_KIND_HASH_BRACKET)) {
            break;
        }

        const MAREA_IntegerSize before = state->index;

        if ((sawDefault == MAREA_BOOLEAN_TRUE) && ((kind == MAREA_TOKEN_KIND_KEYWORD_CASE) || (kind == MAREA_TOKEN_KIND_KEYWORD_DEFAULT))) {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_DEFAULT_ARM_NOT_LAST, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected the default arm to be the last arm of the switch");
        }

        if (kind == MAREA_TOKEN_KIND_KEYWORD_CASE) {
            MAREA_AstNode *arm = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_SWITCH_ARM);

            MAREA_IGNORE(MAREA_ParseAdvance(state));

            do {
                MAREA_ParseNodeAdd(state, arm, MAREA_ParseExpression(state));
            } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);

            MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_THEN, "expected 'then' after the case values"));

            MAREA_ParseNodeAdd(state, arm, MAREA_ParseBlock(state));

            MAREA_ParseExpectEnd(state, "expected 'end' to close the case");

            MAREA_ParseNodeEnd(state, arm);

            MAREA_ParseNodeAdd(state, statement, arm);
        } else if (kind == MAREA_TOKEN_KIND_KEYWORD_DEFAULT) {
            MAREA_AstNode *arm = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_DEFAULT_ARM);

            MAREA_IGNORE(MAREA_ParseAdvance(state));

            MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_THEN, "expected 'then' after 'default'"));

            MAREA_ParseNodeAdd(state, arm, MAREA_ParseBlock(state));

            MAREA_ParseExpectEnd(state, "expected 'end' to close the default");

            MAREA_ParseNodeEnd(state, arm);

            MAREA_ParseNodeAdd(state, statement, arm);

            sawDefault = MAREA_BOOLEAN_TRUE;
        } else {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected 'case', 'default' or 'end'");

            MAREA_ParseSkipBadMember(state, before, statement);
        }

        if (state->index == before) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));
        }
    }

    MAREA_ParseExpectEnd(state, "expected 'end' to close the switch");

    MAREA_ParseNodeEnd(state, statement);

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWith(MAREA_ParseState *state)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_WITH_STATEMENT);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_AS, "expected 'as' after the value"));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseIdentifier(state, "expected a name after 'as'"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_DO, "expected 'do' after the name"));

    MAREA_G_ParseBodyAndEnd(state, statement, "expected 'end' to close the with");

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParsePartition(MAREA_ParseState *state)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_PARTITION_STATEMENT);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_MUTABLE) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNodeSetFlags(statement, MAREA_AST_FLAG_MUTABLE);
    }

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_INTO, "expected 'into' after the value"));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_AS, "expected 'as' after the part count"));

    MAREA_ParseNodeAdd(state, statement, MAREA_ParseIdentifier(state, "expected a name after 'as'"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_DO, "expected 'do' after the name"));

    MAREA_G_ParseBodyAndEnd(state, statement, "expected 'end' to close the partition");

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseSimple(MAREA_ParseState *state, MAREA_AstKind kind, const MAREA_Character8 *message)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, kind);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    if (kind == MAREA_AST_KIND_RETURN_STATEMENT) {
        if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_SEMICOLON) == MAREA_BOOLEAN_FALSE) {
            MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));
        }
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, message));

    MAREA_ParseNodeEnd(state, statement);

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseScope(MAREA_ParseState *state, MAREA_AstKind kind, const MAREA_Character8 *endMessage)
{
    MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, kind);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_G_ParseBodyAndEnd(state, statement, endMessage);

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseExpressionStatement(MAREA_ParseState *state)
{
    MAREA_AstNode *expression = MAREA_ParseExpression(state);

    const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

    if (MAREA_G_IsAssignmentOperator(kind) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNode *assignment = MAREA_ParseNodeBeginAt(state, MAREA_AST_KIND_ASSIGNMENT, MAREA_AstNodeGetPosition(expression));

        MAREA_AstNodeSetOperator(assignment, kind);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, assignment, expression);

        MAREA_ParseNodeAdd(state, assignment, MAREA_ParseExpression(state));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the assignment"));

        MAREA_ParseNodeEnd(state, assignment);

        return assignment;
    }

    const MAREA_AstKind statementKind = (MAREA_AstNodeGetKind(expression) == MAREA_AST_KIND_MACRO_CALL) ? MAREA_AST_KIND_MACRO_STATEMENT : MAREA_AST_KIND_EXPRESSION_STATEMENT;

    MAREA_AstNode *statement = MAREA_ParseNodeBeginAt(state, statementKind, MAREA_AstNodeGetPosition(expression));

    MAREA_ParseNodeAdd(state, statement, expression);

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the expression"));

    MAREA_ParseNodeEnd(state, statement);

    return statement;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseStatementByKind(MAREA_ParseState *state)
{
    switch (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0))) {
    case MAREA_TOKEN_KIND_KEYWORD_LOCAL:
        return MAREA_G_ParseLocal(state);
    case MAREA_TOKEN_KIND_KEYWORD_IF:
        return MAREA_G_ParseIf(state);
    case MAREA_TOKEN_KIND_KEYWORD_WHILE:
        return MAREA_G_ParseWhile(state);
    case MAREA_TOKEN_KIND_KEYWORD_FOR:
        return MAREA_G_ParseFor(state);
    case MAREA_TOKEN_KIND_KEYWORD_SWITCH:
        return MAREA_G_ParseSwitch(state);
    case MAREA_TOKEN_KIND_KEYWORD_WITH:
        return MAREA_G_ParseWith(state);
    case MAREA_TOKEN_KIND_KEYWORD_PARTITION:
        return MAREA_G_ParsePartition(state);
    case MAREA_TOKEN_KIND_KEYWORD_RETURN:
        return MAREA_G_ParseSimple(state, MAREA_AST_KIND_RETURN_STATEMENT, "expected ';' after the return");
    case MAREA_TOKEN_KIND_KEYWORD_BREAK:
        return MAREA_G_ParseSimple(state, MAREA_AST_KIND_BREAK_STATEMENT, "expected ';' after the break");
    case MAREA_TOKEN_KIND_KEYWORD_CONTINUE:
        return MAREA_G_ParseSimple(state, MAREA_AST_KIND_CONTINUE_STATEMENT, "expected ';' after the continue");
    case MAREA_TOKEN_KIND_KEYWORD_DO:
        return MAREA_G_ParseScope(state, MAREA_AST_KIND_SCOPE_BLOCK, "expected 'end' to close the scope");
    case MAREA_TOKEN_KIND_KEYWORD_UNSAFE:
        return MAREA_G_ParseScope(state, MAREA_AST_KIND_UNSAFE_BLOCK, "expected 'end' to close the unsafe block");
    case MAREA_TOKEN_KIND_KEYWORD_FORK_JOIN:
        return MAREA_G_ParseScope(state, MAREA_AST_KIND_FORK_JOIN_BLOCK, "expected 'end' to close the fork_join block");
    default:
        return MAREA_G_ParseExpressionStatement(state);
    }
}

MAREA_AstNode *MAREA_CALL MAREA_ParseStatement(MAREA_ParseState *state)
{
    if (MAREA_ParseEnter(state) == MAREA_BOOLEAN_FALSE) {
        return MAREA_ParseErrorNode(state);
    }

    MAREA_AstNode *statement = MAREA_G_ParseStatementByKind(state);

    MAREA_ParseLeave(state);

    return statement;
}
