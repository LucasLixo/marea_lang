#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

namespace
{

    struct MAREA_KeywordRow
    {
        const char *text;
        MAREA_TokenKind kind;
    };

    const MAREA_KeywordRow MAREA_G_KEYWORD_TABLE[]{
        { "and", MAREA_TOKEN_KIND_KEYWORD_AND },
        { "as", MAREA_TOKEN_KIND_KEYWORD_AS },
        { "break", MAREA_TOKEN_KIND_KEYWORD_BREAK },
        { "case", MAREA_TOKEN_KIND_KEYWORD_CASE },
        { "constant", MAREA_TOKEN_KIND_KEYWORD_CONSTANT },
        { "continue", MAREA_TOKEN_KIND_KEYWORD_CONTINUE },
        { "contract", MAREA_TOKEN_KIND_KEYWORD_CONTRACT },
        { "copy", MAREA_TOKEN_KIND_KEYWORD_COPY },
        { "default", MAREA_TOKEN_KIND_KEYWORD_DEFAULT },
        { "do", MAREA_TOKEN_KIND_KEYWORD_DO },
        { "each", MAREA_TOKEN_KIND_KEYWORD_EACH },
        { "else", MAREA_TOKEN_KIND_KEYWORD_ELSE },
        { "elseif", MAREA_TOKEN_KIND_KEYWORD_ELSEIF },
        { "end", MAREA_TOKEN_KIND_KEYWORD_END },
        { "enum", MAREA_TOKEN_KIND_KEYWORD_ENUM },
        { "false", MAREA_TOKEN_KIND_KEYWORD_FALSE },
        { "for", MAREA_TOKEN_KIND_KEYWORD_FOR },
        { "fork_join", MAREA_TOKEN_KIND_KEYWORD_FORK_JOIN },
        { "function", MAREA_TOKEN_KIND_KEYWORD_FUNCTION },
        { "if", MAREA_TOKEN_KIND_KEYWORD_IF },
        { "implementation", MAREA_TOKEN_KIND_KEYWORD_IMPLEMENTATION },
        { "import", MAREA_TOKEN_KIND_KEYWORD_IMPORT },
        { "in", MAREA_TOKEN_KIND_KEYWORD_IN },
        { "into", MAREA_TOKEN_KIND_KEYWORD_INTO },
        { "local", MAREA_TOKEN_KIND_KEYWORD_LOCAL },
        { "move", MAREA_TOKEN_KIND_KEYWORD_MOVE },
        { "mutable", MAREA_TOKEN_KIND_KEYWORD_MUTABLE },
        { "namespace", MAREA_TOKEN_KIND_KEYWORD_NAMESPACE },
        { "not", MAREA_TOKEN_KIND_KEYWORD_NOT },
        { "opaque", MAREA_TOKEN_KIND_KEYWORD_OPAQUE },
        { "or", MAREA_TOKEN_KIND_KEYWORD_OR },
        { "partition", MAREA_TOKEN_KIND_KEYWORD_PARTITION },
        { "reference", MAREA_TOKEN_KIND_KEYWORD_REFERENCE },
        { "return", MAREA_TOKEN_KIND_KEYWORD_RETURN },
        { "spawn", MAREA_TOKEN_KIND_KEYWORD_SPAWN },
        { "structure", MAREA_TOKEN_KIND_KEYWORD_STRUCTURE },
        { "switch", MAREA_TOKEN_KIND_KEYWORD_SWITCH },
        { "then", MAREA_TOKEN_KIND_KEYWORD_THEN },
        { "true", MAREA_TOKEN_KIND_KEYWORD_TRUE },
        { "type", MAREA_TOKEN_KIND_KEYWORD_TYPE },
        { "unsafe", MAREA_TOKEN_KIND_KEYWORD_UNSAFE },
        { "where", MAREA_TOKEN_KIND_KEYWORD_WHERE },
        { "while", MAREA_TOKEN_KIND_KEYWORD_WHILE },
        { "with", MAREA_TOKEN_KIND_KEYWORD_WITH },
    };

} // namespace

TEST(MAREALexerKeywords, KeywordTableHas44Rows)
{
    GTEST_COUT << "Validating the test's own keyword table, written from the grammar, has 44 rows." << std::endl;

    ASSERT_EQ(sizeof(MAREA_G_KEYWORD_TABLE) / sizeof(MAREA_G_KEYWORD_TABLE[0]), 44U) << "The keyword table must have 44 rows.";
}

TEST(MAREALexerKeywords, AllKeywordsLexToTheirKind)
{
    GTEST_COUT << "Validating every one of the 44 keywords lexes to its own kind." << std::endl;

    for (const MAREA_KeywordRow &row : MAREA_G_KEYWORD_TABLE) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(row.text), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing '" << row.text << "' must succeed.";

        const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

        ASSERT_EQ(KINDS.size(), 2U) << "'" << row.text << "' must lex to exactly one token plus END_OF_FILE.";

        EXPECT_EQ(KINDS[0], row.kind) << "'" << row.text << "' must lex to its own keyword kind.";
    }
}

TEST(MAREALexerKeywords, KeywordFollowedByPunctuation)
{
    GTEST_COUT << "Validating a keyword immediately followed by punctuation still lexes as the keyword." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("end;"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_KEYWORD_END, MAREA_TOKEN_KIND_SEMICOLON, MAREA_TOKEN_KIND_END_OF_FILE })) << "'end;' must lex as KEYWORD_END then SEMICOLON.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("end)"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_KEYWORD_END, MAREA_TOKEN_KIND_RIGHT_PAREN, MAREA_TOKEN_KIND_END_OF_FILE })) << "'end)' must lex as KEYWORD_END then RIGHT_PAREN.";
}

TEST(MAREALexerKeywords, KeywordPrefixIsIdentifier)
{
    GTEST_COUT << "Validating a name that only starts with a keyword lexes as a plain identifier." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("ending endx function1 _end"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_IDENTIFIER) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "A name that merely starts with a keyword must lex as a plain identifier.";
    }
}

TEST(MAREALexerKeywords, BooleanLiteralsAreKeywordKinds)
{
    GTEST_COUT << "Validating true and false lex as their own keyword kinds, since the grammar has no boolean literal kind." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("true false"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_KEYWORD_TRUE, MAREA_TOKEN_KIND_KEYWORD_FALSE, MAREA_TOKEN_KIND_END_OF_FILE })) << "true and false must lex as KEYWORD_TRUE and KEYWORD_FALSE.";
}

TEST(MAREALexerKeywords, KeywordsAreCaseSensitive)
{
    GTEST_COUT << "Validating a keyword spelled with a different case lexes as a plain identifier." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("End FUNCTION True"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_IDENTIFIER) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "A keyword with the wrong case must lex as a plain identifier.";
    }
}
