// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

MAREA_AstNode *MAREA_CALL MAREA_ParseTypeArguments(MAREA_ParseState *state)
{
    MAREA_AstNode *arguments = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_TYPE_ARGUMENTS);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    do {
        if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_INTEGER_LITERAL) == MAREA_BOOLEAN_TRUE) {
            MAREA_ParseNodeAdd(state, arguments, MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_INTEGER_LITERAL, MAREA_ParseAdvance(state)));
        } else {
            MAREA_ParseNodeAdd(state, arguments, MAREA_ParseType(state));
        }
    } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);

    MAREA_IGNORE(MAREA_ParseExpectGreater(state, "expected '>' after the type arguments"));

    MAREA_ParseNodeEnd(state, arguments);

    return arguments;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseNamedType(MAREA_ParseState *state)
{
    MAREA_AstNode *type = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_NAMED_TYPE);

    MAREA_ParseNodeAdd(state, type, MAREA_ParseQualifiedName(state));

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LESS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, type, MAREA_ParseTypeArguments(state));
    }

    MAREA_ParseNodeEnd(state, type);

    return type;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseParameter(MAREA_ParseState *state, MAREA_Boolean allowDefault)
{
    // INFO: a parameter's attribute is written before it, like any other attribute, but is adopted last, so every existing reader of the fixed name, type and default positions keeps working.
    MAREA_AstNode *holder = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_BLOCK);

    MAREA_ParseAttributes(state, holder, MAREA_TOKEN_KIND_HASH_BRACKET);

    MAREA_AstNode *parameter = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_PARAMETER);

    MAREA_ParseNodeAdd(state, parameter, MAREA_ParseIdentifier(state, "expected a parameter name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the parameter name"));

    MAREA_ParseNodeAdd(state, parameter, MAREA_ParseType(state));

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_EQUAL) == MAREA_BOOLEAN_TRUE) {
        if (allowDefault == MAREA_BOOLEAN_FALSE) {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "a function type parameter cannot have a default value");
        }

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, parameter, MAREA_ParseExpression(state));
    }

    MAREA_ParseAdoptAttributes(state, parameter, holder);

    MAREA_ParseNodeEnd(state, parameter);

    return parameter;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseParameterList(MAREA_ParseState *state, MAREA_Boolean allowDefault)
{
    MAREA_AstNode *list = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_PARAMETER_LIST);

    if (MAREA_ParseExpect(state, MAREA_TOKEN_KIND_LEFT_PAREN, "expected '(' before the parameters") == MAREA_BOOLEAN_FALSE) {
        return list;
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_RIGHT_PAREN) == MAREA_BOOLEAN_FALSE) {
        do {
            const MAREA_IntegerSize before = state->index;

            MAREA_ParseNodeAdd(state, list, MAREA_G_ParseParameter(state, allowDefault));

            // INFO: A parameter that consumed nothing means the rest is not a parameter list.
            if (state->index == before) {
                break;
            }
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_PAREN, "expected ')' after the parameters"));

    MAREA_ParseNodeEnd(state, list);

    return list;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseFunctionType(MAREA_ParseState *state)
{
    MAREA_AstNode *type = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_FUNCTION_TYPE);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, type, MAREA_ParseParameterList(state, MAREA_BOOLEAN_FALSE));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COLON) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, type, MAREA_ParseType(state));
    }

    MAREA_ParseNodeEnd(state, type);

    return type;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseType(MAREA_ParseState *state)
{
    if (MAREA_ParseEnter(state) == MAREA_BOOLEAN_FALSE) {
        return MAREA_ParseErrorNode(state);
    }

    MAREA_AstNode *type = MAREA_NULLPTR;

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_FUNCTION) == MAREA_BOOLEAN_TRUE) {
        type = MAREA_G_ParseFunctionType(state);
    } else if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        type = MAREA_ParseNamedType(state);
    } else {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TYPE, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a type");

        type = MAREA_ParseErrorNode(state);
    }

    MAREA_ParseLeave(state);

    return type;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseGenericParameter(MAREA_ParseState *state)
{
    MAREA_AstNode *parameter = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_GENERIC_PARAMETER);

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_CONSTANT) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNodeSetFlags(parameter, MAREA_AST_FLAG_CONSTANT);

        MAREA_ParseNodeAdd(state, parameter, MAREA_ParseIdentifier(state, "expected a constant parameter name"));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the constant parameter name"));

        MAREA_ParseNodeAdd(state, parameter, MAREA_ParseType(state));
    } else {
        MAREA_ParseNodeAdd(state, parameter, MAREA_ParseIdentifier(state, "expected a generic parameter name"));
    }

    MAREA_ParseNodeEnd(state, parameter);

    return parameter;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseGenericParameters(MAREA_ParseState *state)
{
    MAREA_AstNode *parameters = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_GENERIC_PARAMETERS);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    do {
        MAREA_ParseNodeAdd(state, parameters, MAREA_G_ParseGenericParameter(state));
    } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);

    MAREA_IGNORE(MAREA_ParseExpectGreater(state, "expected '>' after the generic parameters"));

    MAREA_ParseNodeEnd(state, parameters);

    return parameters;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseConstraint(MAREA_ParseState *state)
{
    MAREA_AstNode *constraint = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_CONSTRAINT);

    MAREA_ParseNodeAdd(state, constraint, MAREA_ParseIdentifier(state, "expected a generic parameter name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the constrained name"));

    do {
        MAREA_ParseNodeAdd(state, constraint, MAREA_ParseQualifiedName(state));
    } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);

    MAREA_ParseNodeEnd(state, constraint);

    return constraint;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseWhereClause(MAREA_ParseState *state)
{
    MAREA_AstNode *clause = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_WHERE_CLAUSE);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    do {
        MAREA_ParseNodeAdd(state, clause, MAREA_G_ParseConstraint(state));
    } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_AND) == MAREA_BOOLEAN_TRUE);

    MAREA_ParseNodeEnd(state, clause);

    return clause;
}
