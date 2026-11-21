// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

void MAREA_CALL MAREA_ParseExpectEnd(MAREA_ParseState *state, const MAREA_Character8 *message)
{
    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_END) == MAREA_BOOLEAN_TRUE) {
        return;
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_END_OF_FILE) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), message);

        return;
    }

    MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), message);
}

MAREA_Boolean MAREA_CALL MAREA_ParseIsDeclarationStart(MAREA_TokenKind kind)
{
    switch (kind) {
    case MAREA_TOKEN_KIND_KEYWORD_CONSTANT:
    case MAREA_TOKEN_KIND_KEYWORD_TYPE:
    case MAREA_TOKEN_KIND_KEYWORD_STRUCTURE:
    case MAREA_TOKEN_KIND_KEYWORD_OPAQUE:
    case MAREA_TOKEN_KIND_KEYWORD_ENUM:
    case MAREA_TOKEN_KIND_KEYWORD_CONTRACT:
    case MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION:
    case MAREA_TOKEN_KIND_KEYWORD_FUNCTION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static void MAREA_CALL MAREA_G_AdoptAttributes(MAREA_ParseState *state, MAREA_AstNode *node, const MAREA_AstNode *holder)
{
    const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(holder);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstNode *attribute = MAREA_NULLPTR;

        if (MAREA_AstNodeGetChildAt(holder, index, &attribute) == MAREA_AST_EXCEPT_SUCCESS) {
            MAREA_ParseNodeAdd(state, node, (MAREA_AstNode *)attribute);
        }
    }
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseConstant(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_CONSTANT_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected a constant name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the constant name"));

    MAREA_AstNode *type = MAREA_ParseType(state);

    MAREA_ParseNodeAdd(state, declaration, type);

    if (MAREA_AstNodeGetKind(type) == MAREA_AST_KIND_ERROR_NODE) {
        MAREA_ParseSynchronize(state);

        MAREA_ParseNodeEnd(state, declaration);

        return declaration;
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_EQUAL, "expected '=' before the constant value"));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseExpression(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the constant"));

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseTypeDeclaration(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_TYPE_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected a type name"));

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LESS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseGenericParameters(state));
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_EQUAL, "expected '=' after the type name"));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseType(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the type"));

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

// INFO: A member loop must move on every pass, so a bad member is marked, skipped to a safe token and then forced forward.
static void MAREA_CALL MAREA_G_SkipBadMember(MAREA_ParseState *state, MAREA_IntegerSize before, MAREA_AstNode *parent)
{
    MAREA_ParseNodeAdd(state, parent, MAREA_ParseErrorNode(state));

    MAREA_ParseSynchronize(state);

    if (state->index == before) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));
    }
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseField(MAREA_ParseState *state)
{
    MAREA_AstNode *holder = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_BLOCK);

    MAREA_ParseAttributes(state, holder, MAREA_TOKEN_KIND_HASH_BRACKET);

    MAREA_AstNode *field = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_FIELD_DECLARATION);

    MAREA_G_AdoptAttributes(state, field, holder);

    MAREA_ParseNodeAdd(state, field, MAREA_ParseIdentifier(state, "expected a field name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the field name"));

    MAREA_ParseNodeAdd(state, field, MAREA_ParseType(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the field"));

    MAREA_ParseNodeEnd(state, field);

    return field;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseStructure(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_STRUCTURE_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected a structure name"));

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LESS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseGenericParameters(state));
    }

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_END_OF_FILE)) {
            break;
        }

        const MAREA_IntegerSize before = state->index;

        if ((kind == MAREA_TOKEN_KIND_IDENTIFIER) || (kind == MAREA_TOKEN_KIND_HASH_BRACKET)) {
            MAREA_ParseNodeAdd(state, declaration, MAREA_G_ParseField(state));
        } else {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a field or 'end'");

            MAREA_G_SkipBadMember(state, before, declaration);
        }

        if (state->index == before) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));
        }
    }

    MAREA_ParseExpectEnd(state, "expected 'end' to close the structure");

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseOpaque(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_OPAQUE_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_KEYWORD_STRUCTURE, "expected 'structure' after 'opaque'"));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected a structure name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the opaque structure"));

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseVariantField(MAREA_ParseState *state)
{
    MAREA_AstNode *field = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_VARIANT_FIELD);

    MAREA_ParseNodeAdd(state, field, MAREA_ParseIdentifier(state, "expected a variant field name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_COLON, "expected ':' after the variant field name"));

    MAREA_ParseNodeAdd(state, field, MAREA_ParseType(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_EQUAL) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, field, MAREA_ParseExpression(state));
    }

    MAREA_ParseNodeEnd(state, field);

    return field;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseVariant(MAREA_ParseState *state)
{
    MAREA_AstNode *variant = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_VARIANT_DECLARATION);

    MAREA_ParseNodeAdd(state, variant, MAREA_ParseIdentifier(state, "expected a variant name"));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_LEFT_PAREN) == MAREA_BOOLEAN_TRUE) {
        do {
            const MAREA_IntegerSize before = state->index;

            MAREA_ParseNodeAdd(state, variant, MAREA_G_ParseVariantField(state));

            if (state->index == before) {
                break;
            }
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_PAREN, "expected ')' after the variant fields"));
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the variant"));

    MAREA_ParseNodeEnd(state, variant);

    return variant;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseEnum(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_ENUM_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected an enum name"));

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_END_OF_FILE)) {
            break;
        }

        const MAREA_IntegerSize before = state->index;

        if (kind == MAREA_TOKEN_KIND_IDENTIFIER) {
            MAREA_ParseNodeAdd(state, declaration, MAREA_G_ParseVariant(state));
        } else {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a variant or 'end'");

            MAREA_G_SkipBadMember(state, before, declaration);
        }

        if (state->index == before) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));
        }
    }

    MAREA_ParseExpectEnd(state, "expected 'end' to close the enum");

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseContractFunction(MAREA_ParseState *state)
{
    MAREA_AstNode *function = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_CONTRACT_FUNCTION);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, function, MAREA_ParseIdentifier(state, "expected a function name"));

    MAREA_ParseNodeAdd(state, function, MAREA_ParseParameterList(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COLON) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, function, MAREA_ParseType(state));
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the contract function"));

    MAREA_ParseNodeEnd(state, function);

    return function;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseContract(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_CONTRACT_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected a contract name"));

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_END_OF_FILE)) {
            break;
        }

        const MAREA_IntegerSize before = state->index;

        if (kind == MAREA_TOKEN_KIND_KEYWORD_FUNCTION) {
            MAREA_ParseNodeAdd(state, declaration, MAREA_G_ParseContractFunction(state));
        } else {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a function or 'end'");

            MAREA_G_SkipBadMember(state, before, declaration);
        }

        if (state->index == before) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));
        }
    }

    MAREA_ParseExpectEnd(state, "expected 'end' to close the contract");

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseFunction(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_FUNCTION_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected a function name"));

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LESS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseGenericParameters(state));
    }

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseParameterList(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COLON) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseType(state));
    }

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_WHERE) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseWhereClause(state));
    }

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseBlock(state));

    MAREA_ParseExpectEnd(state, "expected 'end' to close the function");

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseImplementation(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION);

    MAREA_G_AdoptAttributes(state, declaration, holder);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LESS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseGenericParameters(state));
    }

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseNamedType(state));

    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_END_OF_FILE)) {
            break;
        }

        const MAREA_IntegerSize before = state->index;

        MAREA_AstNode *memberHolder = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_BLOCK);

        MAREA_ParseAttributes(state, memberHolder, MAREA_TOKEN_KIND_HASH_BRACKET);

        const MAREA_TokenKind memberKind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if (memberKind == MAREA_TOKEN_KIND_KEYWORD_FUNCTION) {
            MAREA_ParseNodeAdd(state, declaration, MAREA_G_ParseFunction(state, memberHolder));
        } else if (memberKind == MAREA_TOKEN_KIND_KEYWORD_CONSTANT) {
            MAREA_ParseNodeAdd(state, declaration, MAREA_G_ParseConstant(state, memberHolder));
        } else {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_DECLARATION, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a function, a constant or 'end'");

            MAREA_G_SkipBadMember(state, before, declaration);
        }

        if (state->index == before) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));
        }
    }

    MAREA_ParseExpectEnd(state, "expected 'end' to close the implementation");

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseDeclaration(MAREA_ParseState *state, MAREA_AstNode *holder)
{
    switch (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0))) {
    case MAREA_TOKEN_KIND_KEYWORD_CONSTANT:
        return MAREA_G_ParseConstant(state, holder);
    case MAREA_TOKEN_KIND_KEYWORD_TYPE:
        return MAREA_G_ParseTypeDeclaration(state, holder);
    case MAREA_TOKEN_KIND_KEYWORD_STRUCTURE:
        return MAREA_G_ParseStructure(state, holder);
    case MAREA_TOKEN_KIND_KEYWORD_OPAQUE:
        return MAREA_G_ParseOpaque(state, holder);
    case MAREA_TOKEN_KIND_KEYWORD_ENUM:
        return MAREA_G_ParseEnum(state, holder);
    case MAREA_TOKEN_KIND_KEYWORD_CONTRACT:
        return MAREA_G_ParseContract(state, holder);
    case MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION:
        return MAREA_G_ParseImplementation(state, holder);
    default:
        return MAREA_G_ParseFunction(state, holder);
    }
}
