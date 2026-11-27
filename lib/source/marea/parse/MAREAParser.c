// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/parse/MAREAParseInternal.h"

#include <string.h>

MAREA_ParseExcept MAREA_CALL MAREA_ParseContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_ParseContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_ParseContext), MAREA_ALIGNOF(MAREA_ParseContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_PARSE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_ParseContext *context = memory;

    context->arena = arena;
    context->sink = sink;

    *outContext = context;

    return MAREA_PARSE_EXCEPT_SUCCESS;
}

MAREA_Arena *MAREA_CALL MAREA_ParseContextGetArena(const MAREA_ParseContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_ParseContextGetDiagnostics(const MAREA_ParseContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return context->sink;
}

MAREA_AstNode *MAREA_CALL MAREA_ParseIdentifier(MAREA_ParseState *state, const MAREA_Character8 *message)
{
    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), message);

        return MAREA_ParseErrorNode(state);
    }

    return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_IDENTIFIER, MAREA_ParseAdvance(state));
}

MAREA_AstNode *MAREA_CALL MAREA_ParseQualifiedName(MAREA_ParseState *state)
{
    MAREA_AstNode *name = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_QUALIFIED_NAME);

    MAREA_AstNode *first = MAREA_ParseIdentifier(state, "expected a name");

    MAREA_ParseNodeAdd(state, name, first);

    if (MAREA_AstNodeGetKind(first) == MAREA_AST_KIND_ERROR_NODE) {
        MAREA_ParseNodeEnd(state, name);

        return name;
    }

    while ((MAREA_ParseAt(state, MAREA_TOKEN_KIND_DOT) == MAREA_BOOLEAN_TRUE) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_IDENTIFIER)) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_ParseNodeAdd(state, name, MAREA_ParseIdentifier(state, "expected a name"));
    }

    MAREA_ParseNodeEnd(state, name);

    return name;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseAttributeValue(MAREA_ParseState *state)
{
    switch (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0))) {
    case MAREA_TOKEN_KIND_KEYWORD_TRUE:
    case MAREA_TOKEN_KIND_KEYWORD_FALSE:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_BOOLEAN_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_INTEGER_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_INTEGER_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_STRING_LITERAL:
        return MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_STRING_LITERAL, MAREA_ParseAdvance(state));
    case MAREA_TOKEN_KIND_IDENTIFIER:
        return MAREA_ParseQualifiedName(state);
    default:
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected an attribute value");

        return MAREA_ParseErrorNode(state);
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_PeekIsWord(const MAREA_ParseState *state, MAREA_IntegerSize ahead, const MAREA_Character8 *word)
{
    const MAREA_Token *token = MAREA_ParsePeekToken(state, ahead);

    if (MAREA_TokenGetKind(token) != MAREA_TOKEN_KIND_IDENTIFIER) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize wordLength = strlen(word);

    return ((MAREA_TokenGetLength(token) == wordLength) && (memcmp(MAREA_TokenGetText(token), word, wordLength) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWhenPredicate(MAREA_ParseState *state);

// INFO: The combinators nest, so the call goes through MAREA_ParseEnter in MAREA_G_ParseWhenPredicate.
static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWhenCombinator(MAREA_ParseState *state)
{
    MAREA_AstNode *combinator = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_WHEN_COMBINATOR);

    const MAREA_Boolean isNot = MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_NOT);

    MAREA_ParseNodeAdd(state, combinator, MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_IDENTIFIER, MAREA_ParseAdvance(state)));

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    if (isNot == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, combinator, MAREA_G_ParseWhenPredicate(state));
    } else {
        do {
            MAREA_ParseNodeAdd(state, combinator, MAREA_G_ParseWhenPredicate(state));
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_PAREN, "expected ')' after the predicates"));

    MAREA_ParseNodeEnd(state, combinator);

    return combinator;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWhenComparison(MAREA_ParseState *state)
{
    MAREA_AstNode *comparison = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_WHEN_COMPARISON);

    MAREA_ParseNodeAdd(state, comparison, MAREA_ParseIdentifier(state, "expected a predicate name"));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_EQUAL, "expected '=' after the predicate name"));

    switch (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0))) {
    case MAREA_TOKEN_KIND_STRING_LITERAL:
        MAREA_ParseNodeAdd(state, comparison, MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_STRING_LITERAL, MAREA_ParseAdvance(state)));
        break;
    case MAREA_TOKEN_KIND_INTEGER_LITERAL:
        MAREA_ParseNodeAdd(state, comparison, MAREA_ParseNodeFromToken(state, MAREA_AST_KIND_INTEGER_LITERAL, MAREA_ParseAdvance(state)));
        break;
    default:
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a string or an integer after '=' in the predicate");

        MAREA_ParseNodeAdd(state, comparison, MAREA_ParseErrorNode(state));
        break;
    }

    MAREA_ParseNodeEnd(state, comparison);

    return comparison;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWhenPredicate(MAREA_ParseState *state)
{
    if (MAREA_ParseEnter(state) == MAREA_BOOLEAN_FALSE) {
        return MAREA_ParseErrorNode(state);
    }

    MAREA_AstNode *predicate = MAREA_NULLPTR;

    const MAREA_Boolean OPENS_PARENTHESIS = (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_LEFT_PAREN) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_Boolean IS_COMBINATOR = ((OPENS_PARENTHESIS == MAREA_BOOLEAN_TRUE) && ((MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_NOT) == MAREA_BOOLEAN_TRUE) || (MAREA_G_PeekIsWord(state, MAREA_INTEGER_SIZE(0), "all") == MAREA_BOOLEAN_TRUE) || (MAREA_G_PeekIsWord(state, MAREA_INTEGER_SIZE(0), "any") == MAREA_BOOLEAN_TRUE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (IS_COMBINATOR == MAREA_BOOLEAN_TRUE) {
        predicate = MAREA_G_ParseWhenCombinator(state);
    } else if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        predicate = MAREA_G_ParseWhenComparison(state);
    } else {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a predicate for the when item");

        predicate = MAREA_ParseErrorNode(state);
    }

    MAREA_ParseLeave(state);

    return predicate;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseAttributeItem(MAREA_ParseState *state, MAREA_Boolean allowWhen);

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseWhenItem(MAREA_ParseState *state)
{
    MAREA_AstNode *item = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_WHEN_ITEM);

    MAREA_IGNORE(MAREA_ParseAdvance(state));
    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, item, MAREA_G_ParseWhenPredicate(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE) {
        do {
            MAREA_ParseNodeAdd(state, item, MAREA_G_ParseAttributeItem(state, MAREA_BOOLEAN_FALSE));
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);
    } else {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected ',' and an attribute after the predicate of the when item");
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_PAREN, "expected ')' after the when item"));

    MAREA_ParseNodeEnd(state, item);

    return item;
}

// INFO: The word when with a parenthesis after it starts a when item. Inside a when item the word is a plain key, so the item never nests.
static MAREA_AstNode *MAREA_CALL MAREA_G_ParseAttributeItem(MAREA_ParseState *state, MAREA_Boolean allowWhen)
{
    if ((allowWhen == MAREA_BOOLEAN_TRUE) && (MAREA_G_PeekIsWord(state, MAREA_INTEGER_SIZE(0), "when") == MAREA_BOOLEAN_TRUE) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_LEFT_PAREN)) {
        return MAREA_G_ParseWhenItem(state);
    }

    MAREA_AstNode *item = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_ATTRIBUTE_ITEM);

    MAREA_AstNode *key = MAREA_ParseIdentifier(state, "expected an attribute name");

    MAREA_ParseNodeAdd(state, item, key);

    if ((MAREA_AstNodeGetKind(key) != MAREA_AST_KIND_ERROR_NODE) && (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_EQUAL) == MAREA_BOOLEAN_TRUE)) {
        MAREA_ParseNodeAdd(state, item, MAREA_G_ParseAttributeValue(state));
    }

    MAREA_ParseNodeEnd(state, item);

    return item;
}

static void MAREA_CALL MAREA_G_SkipToAttributeEnd(MAREA_ParseState *state)
{
    for (;;) {
        const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

        if (kind == MAREA_TOKEN_KIND_RIGHT_BRACKET) {
            MAREA_IGNORE(MAREA_ParseAdvance(state));

            return;
        }

        if ((kind == MAREA_TOKEN_KIND_END_OF_FILE) || (kind == MAREA_TOKEN_KIND_SEMICOLON) || (MAREA_ParseIsSynchronizationKind(kind) == MAREA_BOOLEAN_TRUE)) {
            return;
        }

        MAREA_IGNORE(MAREA_ParseAdvance(state));
    }
}

void MAREA_CALL MAREA_ParseAttributes(MAREA_ParseState *state, MAREA_AstNode *holder, MAREA_TokenKind opener)
{
    const MAREA_AstKind attributeKind = (opener == MAREA_TOKEN_KIND_HASH_BANG_BRACKET) ? MAREA_AST_KIND_FILE_ATTRIBUTE : MAREA_AST_KIND_ATTRIBUTE;

    while (MAREA_ParseAt(state, opener) == MAREA_BOOLEAN_TRUE) {
        MAREA_AstNode *attribute = MAREA_ParseNodeBegin(state, attributeKind);

        MAREA_IGNORE(MAREA_ParseAdvance(state));

        do {
            MAREA_ParseNodeAdd(state, attribute, MAREA_G_ParseAttributeItem(state, MAREA_BOOLEAN_TRUE));
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);

        if (MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_BRACKET, "expected ']' after the attribute") == MAREA_BOOLEAN_FALSE) {
            MAREA_G_SkipToAttributeEnd(state);
        }

        MAREA_ParseNodeEnd(state, attribute);

        MAREA_ParseNodeAdd(state, holder, attribute);
    }
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseNamespace(MAREA_ParseState *state)
{
    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_NAMESPACE_DECLARATION);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseQualifiedName(state));

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the namespace name"));

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static MAREA_AstNode *MAREA_CALL MAREA_G_ParseImportMember(MAREA_ParseState *state)
{
    MAREA_AstNode *member = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_IMPORT_MEMBER);

    MAREA_ParseNodeAdd(state, member, MAREA_ParseQualifiedName(state));

    if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_AS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, member, MAREA_ParseIdentifier(state, "expected an alias name after 'as'"));

        MAREA_AstNodeSetFlags(member, MAREA_AST_FLAG_HAS_ALIAS);
    }

    MAREA_ParseNodeEnd(state, member);

    return member;
}

// INFO: The braces of a member list are a list, never a block. The caller has checked that the next token is the opening brace.
static void MAREA_CALL MAREA_G_ParseImportMemberList(MAREA_ParseState *state, MAREA_AstNode *declaration)
{
    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_AstNodeSetFlags(declaration, MAREA_AST_FLAG_HAS_MEMBERS);

    if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_RIGHT_BRACE) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "an import member list needs at least one member");
    } else {
        do {
            if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_RIGHT_BRACE) == MAREA_BOOLEAN_TRUE) {
                MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "an import member list has no trailing comma");

                break;
            }

            MAREA_ParseNodeAdd(state, declaration, MAREA_G_ParseImportMember(state));
        } while (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_COMMA) == MAREA_BOOLEAN_TRUE);
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_RIGHT_BRACE, "expected '}' after the import member list"));
}

// INFO: An import and a require take the same forms. Only the node kind and the words of the messages differ.
static MAREA_AstNode *MAREA_CALL MAREA_G_ParseImportOrRequire(MAREA_ParseState *state, MAREA_AstKind kind)
{
    const MAREA_Boolean IS_REQUIRE = (kind == MAREA_AST_KIND_REQUIRE_DECLARATION) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    MAREA_AstNode *declaration = MAREA_ParseNodeBegin(state, kind);

    MAREA_IGNORE(MAREA_ParseAdvance(state));

    MAREA_ParseNodeAdd(state, declaration, MAREA_ParseQualifiedName(state));

    if ((MAREA_ParseAt(state, MAREA_TOKEN_KIND_DOT) == MAREA_BOOLEAN_TRUE) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_STAR)) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));
        MAREA_IGNORE(MAREA_ParseAdvance(state));

        MAREA_AstNodeSetFlags(declaration, MAREA_AST_FLAG_WILDCARD);

        if ((MAREA_ParseAt(state, MAREA_TOKEN_KIND_KEYWORD_AS) == MAREA_BOOLEAN_TRUE) || (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LEFT_BRACE) == MAREA_BOOLEAN_TRUE)) {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), (IS_REQUIRE == MAREA_BOOLEAN_TRUE) ? "a wildcard require cannot have an alias or a member list" : "a wildcard import cannot have an alias or a member list");
        }
    } else if (MAREA_ParseAccept(state, MAREA_TOKEN_KIND_KEYWORD_AS) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, declaration, MAREA_ParseIdentifier(state, "expected an alias name after 'as'"));

        MAREA_AstNodeSetFlags(declaration, MAREA_AST_FLAG_HAS_ALIAS);

        if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LEFT_BRACE) == MAREA_BOOLEAN_TRUE) {
            MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), (IS_REQUIRE == MAREA_BOOLEAN_TRUE) ? "a require cannot have an alias and a member list" : "an import cannot have an alias and a member list");
        }
    } else if (MAREA_ParseAt(state, MAREA_TOKEN_KIND_LEFT_BRACE) == MAREA_BOOLEAN_TRUE) {
        MAREA_G_ParseImportMemberList(state, declaration);
    }

    MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, (IS_REQUIRE == MAREA_BOOLEAN_TRUE) ? "expected ';' after the require" : "expected ';' after the import"));

    MAREA_ParseNodeEnd(state, declaration);

    return declaration;
}

static void MAREA_CALL MAREA_G_ParseTopLevelItem(MAREA_ParseState *state, MAREA_AstNode *root)
{
    // INFO: Attributes wait in a scratch node until the item that owns them is parsed.
    MAREA_AstNode *holder = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_BLOCK);

    MAREA_ParseAttributes(state, holder, MAREA_TOKEN_KIND_HASH_BRACKET);

    const MAREA_IntegerSize before = state->index;

    const MAREA_TokenKind kind = MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(0));

    if ((kind == MAREA_TOKEN_KIND_KEYWORD_IMPORT) || (kind == MAREA_TOKEN_KIND_KEYWORD_REQUIRE)) {
        const MAREA_Boolean IS_REQUIRE = (kind == MAREA_TOKEN_KIND_KEYWORD_REQUIRE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_DECLARATION, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), (IS_REQUIRE == MAREA_BOOLEAN_TRUE) ? "a require must come before the declarations" : "an import must come before the declarations");

        MAREA_ParseNodeAdd(state, root, MAREA_G_ParseImportOrRequire(state, (IS_REQUIRE == MAREA_BOOLEAN_TRUE) ? MAREA_AST_KIND_REQUIRE_DECLARATION : MAREA_AST_KIND_IMPORT_DECLARATION));

        return;
    }

    if (MAREA_ParseIsDeclarationStart(kind) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(state, root, MAREA_ParseDeclaration(state, holder));

        return;
    }

    if ((kind == MAREA_TOKEN_KIND_IDENTIFIER) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(1)) == MAREA_TOKEN_KIND_BANG) && (MAREA_ParsePeekKind(state, MAREA_INTEGER_SIZE(2)) == MAREA_TOKEN_KIND_LEFT_PAREN)) {
        MAREA_AstNode *statement = MAREA_ParseNodeBegin(state, MAREA_AST_KIND_MACRO_STATEMENT);

        MAREA_ParseAdoptAttributes(state, statement, holder);

        MAREA_ParseNodeAdd(state, statement, MAREA_ParseExpression(state));

        MAREA_IGNORE(MAREA_ParseExpect(state, MAREA_TOKEN_KIND_SEMICOLON, "expected ';' after the macro call"));

        MAREA_ParseNodeEnd(state, statement);

        MAREA_ParseNodeAdd(state, root, statement);

        return;
    }

    MAREA_AstNode *errorNode = MAREA_ParseErrorNode(state);

    if ((kind == MAREA_TOKEN_KIND_KEYWORD_END) || (kind == MAREA_TOKEN_KIND_KEYWORD_CASE) || (kind == MAREA_TOKEN_KIND_KEYWORD_DEFAULT) || (kind == MAREA_TOKEN_KIND_KEYWORD_ELSEIF) || (kind == MAREA_TOKEN_KIND_KEYWORD_ELSE)) {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "unexpected block terminator");
    } else {
        MAREA_ParseReport(state, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_DECLARATION, MAREA_ParsePeekToken(state, MAREA_INTEGER_SIZE(0)), "expected a declaration");
    }

    MAREA_ParseSynchronize(state);

    // INFO: Recovery must always move, or the top level loop would never end.
    if (state->index == before) {
        MAREA_IGNORE(MAREA_ParseAdvance(state));
    }

    MAREA_ParseNodeEnd(state, errorNode);

    MAREA_ParseNodeAdd(state, root, errorNode);
}

MAREA_ParseExcept MAREA_CALL MAREA_ParseCompilationUnit(MAREA_ParseContext *context, const MAREA_TokenList *list, const MAREA_AstNode **outRoot)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (list == MAREA_NULLPTR) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outRoot == MAREA_NULLPTR) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_ParseState state = {
        .context = context,
        .list = list,
        .count = MAREA_TokenListGetCount(list),
        .index = MAREA_INTEGER_SIZE(0),
        .lastEnd = MAREA_UINTEGER_32(0),
        .lastErrorOffset = MAREA_UINTEGER_32(0),
        .depth = MAREA_UINTEGER_32(0),
        .halfGreater = MAREA_BOOLEAN_FALSE,
        .hasError = MAREA_BOOLEAN_FALSE,
        .outOfMemory = MAREA_BOOLEAN_FALSE,
    };

    if (state.count == MAREA_INTEGER_SIZE(0)) {
        return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_AstNode *root = MAREA_ParseNodeBegin(&state, MAREA_AST_KIND_COMPILATION_UNIT);

    if (MAREA_ParseAt(&state, MAREA_TOKEN_KIND_KEYWORD_NAMESPACE) == MAREA_BOOLEAN_TRUE) {
        MAREA_ParseNodeAdd(&state, root, MAREA_G_ParseNamespace(&state));
    } else {
        MAREA_ParseReport(&state, MAREA_PARSE_DIAGNOSTIC_CODE_MISSING_NAMESPACE, MAREA_ParsePeekToken(&state, MAREA_INTEGER_SIZE(0)), "a file must start with a namespace declaration");
    }

    MAREA_ParseAttributes(&state, root, MAREA_TOKEN_KIND_HASH_BANG_BRACKET);

    for (;;) {
        if (MAREA_ParseAt(&state, MAREA_TOKEN_KIND_KEYWORD_IMPORT) == MAREA_BOOLEAN_TRUE) {
            MAREA_ParseNodeAdd(&state, root, MAREA_G_ParseImportOrRequire(&state, MAREA_AST_KIND_IMPORT_DECLARATION));
        } else if (MAREA_ParseAt(&state, MAREA_TOKEN_KIND_KEYWORD_REQUIRE) == MAREA_BOOLEAN_TRUE) {
            MAREA_ParseNodeAdd(&state, root, MAREA_G_ParseImportOrRequire(&state, MAREA_AST_KIND_REQUIRE_DECLARATION));
        } else {
            break;
        }
    }

    while (MAREA_ParseAt(&state, MAREA_TOKEN_KIND_END_OF_FILE) == MAREA_BOOLEAN_FALSE) {
        MAREA_G_ParseTopLevelItem(&state, root);
    }

    MAREA_ParseNodeEnd(&state, root);

    if ((state.outOfMemory == MAREA_BOOLEAN_TRUE) || (root == MAREA_NULLPTR)) {
        return MAREA_PARSE_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    *outRoot = root;

    return MAREA_PARSE_EXCEPT_SUCCESS;
}
