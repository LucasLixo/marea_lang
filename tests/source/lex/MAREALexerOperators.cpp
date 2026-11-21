#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

namespace
{

    struct MAREA_OperatorRow
    {
        const char *text;
        MAREA_TokenKind kind;
    };

    const MAREA_OperatorRow MAREA_G_OPERATOR_TABLE[]{
        { "+", MAREA_TOKEN_KIND_PLUS },
        { "-", MAREA_TOKEN_KIND_MINUS },
        { "*", MAREA_TOKEN_KIND_STAR },
        { "/", MAREA_TOKEN_KIND_SLASH },
        { "%", MAREA_TOKEN_KIND_PERCENT },
        { "**", MAREA_TOKEN_KIND_STAR_STAR },
        { "+%", MAREA_TOKEN_KIND_PLUS_PERCENT },
        { "-%", MAREA_TOKEN_KIND_MINUS_PERCENT },
        { "*%", MAREA_TOKEN_KIND_STAR_PERCENT },
        { "+|", MAREA_TOKEN_KIND_PLUS_PIPE },
        { "-|", MAREA_TOKEN_KIND_MINUS_PIPE },
        { "*|", MAREA_TOKEN_KIND_STAR_PIPE },
        { "~", MAREA_TOKEN_KIND_TILDE },
        { "&", MAREA_TOKEN_KIND_AMPERSAND },
        { "|", MAREA_TOKEN_KIND_PIPE },
        { "^", MAREA_TOKEN_KIND_CARET },
        { "<<", MAREA_TOKEN_KIND_SHIFT_LEFT },
        { ">>", MAREA_TOKEN_KIND_SHIFT_RIGHT },
        { "==", MAREA_TOKEN_KIND_EQUAL_EQUAL },
        { "!=", MAREA_TOKEN_KIND_BANG_EQUAL },
        { "<", MAREA_TOKEN_KIND_LESS },
        { "<=", MAREA_TOKEN_KIND_LESS_EQUAL },
        { ">", MAREA_TOKEN_KIND_GREATER },
        { ">=", MAREA_TOKEN_KIND_GREATER_EQUAL },
        { "=", MAREA_TOKEN_KIND_EQUAL },
        { "+=", MAREA_TOKEN_KIND_PLUS_EQUAL },
        { "-=", MAREA_TOKEN_KIND_MINUS_EQUAL },
        { "*=", MAREA_TOKEN_KIND_STAR_EQUAL },
        { "/=", MAREA_TOKEN_KIND_SLASH_EQUAL },
        { "%=", MAREA_TOKEN_KIND_PERCENT_EQUAL },
        { "**=", MAREA_TOKEN_KIND_STAR_STAR_EQUAL },
        { "<<=", MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL },
        { ">>=", MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL },
        { "&=", MAREA_TOKEN_KIND_AMPERSAND_EQUAL },
        { "|=", MAREA_TOKEN_KIND_PIPE_EQUAL },
        { "^=", MAREA_TOKEN_KIND_CARET_EQUAL },
    };

    const MAREA_OperatorRow MAREA_G_SEPARATOR_TABLE[]{
        { "(", MAREA_TOKEN_KIND_LEFT_PAREN },
        { ")", MAREA_TOKEN_KIND_RIGHT_PAREN },
        { "[", MAREA_TOKEN_KIND_LEFT_BRACKET },
        { "]", MAREA_TOKEN_KIND_RIGHT_BRACKET },
        { "{", MAREA_TOKEN_KIND_LEFT_BRACE },
        { "}", MAREA_TOKEN_KIND_RIGHT_BRACE },
        { ",", MAREA_TOKEN_KIND_COMMA },
        { ";", MAREA_TOKEN_KIND_SEMICOLON },
        { ":", MAREA_TOKEN_KIND_COLON },
        { ".", MAREA_TOKEN_KIND_DOT },
        { "..", MAREA_TOKEN_KIND_DOT_DOT },
        { "!", MAREA_TOKEN_KIND_BANG },
        { "#[", MAREA_TOKEN_KIND_HASH_BRACKET },
        { "#![", MAREA_TOKEN_KIND_HASH_BANG_BRACKET },
    };

} // namespace

TEST(MAREALexerOperators, TableCountsAre36And14)
{
    GTEST_COUT << "Validating the test's own operator and separator tables, written from the grammar, have 36 and 14 rows." << std::endl;

    ASSERT_EQ(sizeof(MAREA_G_OPERATOR_TABLE) / sizeof(MAREA_G_OPERATOR_TABLE[0]), 36U) << "The operator table must have 36 rows.";

    ASSERT_EQ(sizeof(MAREA_G_SEPARATOR_TABLE) / sizeof(MAREA_G_SEPARATOR_TABLE[0]), 14U) << "The separator table must have 14 rows.";
}

TEST(MAREALexerOperators, AllOperatorsLexToTheirKind)
{
    GTEST_COUT << "Validating every one of the 36 operators lexes to its own kind." << std::endl;

    for (const MAREA_OperatorRow &row : MAREA_G_OPERATOR_TABLE) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(row.text), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing '" << row.text << "' must succeed.";

        const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

        ASSERT_EQ(KINDS.size(), 2U) << "'" << row.text << "' must lex to exactly one token plus END_OF_FILE.";

        EXPECT_EQ(KINDS[0], row.kind) << "'" << row.text << "' must lex to its own operator kind.";
    }
}

TEST(MAREALexerOperators, AllSeparatorsLexToTheirKind)
{
    GTEST_COUT << "Validating every one of the 14 separators lexes to its own kind." << std::endl;

    for (const MAREA_OperatorRow &row : MAREA_G_SEPARATOR_TABLE) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(row.text), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing '" << row.text << "' must succeed.";

        const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

        ASSERT_EQ(KINDS.size(), 2U) << "'" << row.text << "' must lex to exactly one token plus END_OF_FILE.";

        EXPECT_EQ(KINDS[0], row.kind) << "'" << row.text << "' must lex to its own separator kind.";
    }
}

TEST(MAREALexerOperators, LongestMatchPrefersLongerOperator)
{
    GTEST_COUT << "Validating the lexer always takes the longest matching operator." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("**= ** * <<= << <= < >>= >> >= >"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> EXPECTED{
        MAREA_TOKEN_KIND_STAR_STAR_EQUAL,
        MAREA_TOKEN_KIND_STAR_STAR,
        MAREA_TOKEN_KIND_STAR,
        MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL,
        MAREA_TOKEN_KIND_SHIFT_LEFT,
        MAREA_TOKEN_KIND_LESS_EQUAL,
        MAREA_TOKEN_KIND_LESS,
        MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL,
        MAREA_TOKEN_KIND_SHIFT_RIGHT,
        MAREA_TOKEN_KIND_GREATER_EQUAL,
        MAREA_TOKEN_KIND_GREATER,
        MAREA_TOKEN_KIND_END_OF_FILE,
    };

    EXPECT_EQ(fixture.MAREA_kinds(), EXPECTED) << "Each operator must lex to its own, longest-matching kind.";
}

TEST(MAREALexerOperators, LongestMatchWithoutSpaces)
{
    GTEST_COUT << "Validating longest match still applies with no spaces around the operators." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a<=b a**b x+%y a*|b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> EXPECTED{
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_LESS_EQUAL,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_STAR_STAR,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_PLUS_PERCENT,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_STAR_PIPE,
        MAREA_TOKEN_KIND_IDENTIFIER,
        MAREA_TOKEN_KIND_END_OF_FILE,
    };

    EXPECT_EQ(fixture.MAREA_kinds(), EXPECTED) << "Operators glued to identifiers must still take the longest match.";
}

TEST(MAREALexerOperators, AdjacentOperatorsSplitGreedy)
{
    GTEST_COUT << "Validating adjacent operator bytes split by taking the longest match first, every time." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("+=-"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_PLUS_EQUAL, MAREA_TOKEN_KIND_MINUS, MAREA_TOKEN_KIND_END_OF_FILE })) << "'+=-' must split into PLUS_EQUAL and MINUS.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("==="), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_EQUAL_EQUAL, MAREA_TOKEN_KIND_EQUAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "'===' must split into EQUAL_EQUAL and EQUAL.";

    MAREA_LexFixture thirdFixture;

    ASSERT_EQ(thirdFixture.MAREA_lex("<<<"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(thirdFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_SHIFT_LEFT, MAREA_TOKEN_KIND_LESS, MAREA_TOKEN_KIND_END_OF_FILE })) << "'<<<' must split into SHIFT_LEFT and LESS.";

    MAREA_LexFixture fourthFixture;

    ASSERT_EQ(fourthFixture.MAREA_lex("x=-1"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fourthFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_EQUAL, MAREA_TOKEN_KIND_MINUS, MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "'x=-1' must split into IDENTIFIER, EQUAL, MINUS; the digit is an unknown byte until the number scanner lands.";
}

TEST(MAREALexerOperators, DotDotVersusDot)
{
    GTEST_COUT << "Validating a single dot and a double dot each lex to their own, distinct kind." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("a.b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_DOT, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'a.b' must lex with a single DOT.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("a..b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_DOT_DOT, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'a..b' must lex with a single DOT_DOT.";

    MAREA_LexFixture thirdFixture;

    ASSERT_EQ(thirdFixture.MAREA_lex("..."), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(thirdFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_DOT_DOT, MAREA_TOKEN_KIND_DOT, MAREA_TOKEN_KIND_END_OF_FILE })) << "'...' must split into DOT_DOT and DOT.";
}

TEST(MAREALexerOperators, HashBracketForms)
{
    GTEST_COUT << "Validating #[ and #![ each lex as their own single separator token." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("#[inline]"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> FIRST_KINDS{ firstFixture.MAREA_kinds() };

    ASSERT_FALSE(FIRST_KINDS.empty()) << "There must be at least one token.";

    EXPECT_EQ(FIRST_KINDS[0], MAREA_TOKEN_KIND_HASH_BRACKET) << "#[inline] must start with HASH_BRACKET.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("#![file]"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> SECOND_KINDS{ secondFixture.MAREA_kinds() };

    ASSERT_FALSE(SECOND_KINDS.empty()) << "There must be at least one token.";

    EXPECT_EQ(SECOND_KINDS[0], MAREA_TOKEN_KIND_HASH_BANG_BRACKET) << "#![file] must start with HASH_BANG_BRACKET.";
}

TEST(MAREALexerOperators, ShiftTokensCarryExactSpan)
{
    GTEST_COUT << "Validating a >> token carries length 2 so a parser can split it into two > of length 1." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("array<array<float32>>"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_EQ(KINDS.back(), MAREA_TOKEN_KIND_END_OF_FILE) << "The source must still end with END_OF_FILE.";

    const std::size_t SHIFT_INDEX{ KINDS.size() - 2 };

    ASSERT_EQ(KINDS[SHIFT_INDEX], MAREA_TOKEN_KIND_SHIFT_RIGHT) << "The last real token must be SHIFT_RIGHT.";

    EXPECT_EQ(POSITIONS[SHIFT_INDEX].length, MAREA_UINTEGER_32(2)) << "The >> token must carry length 2.";

    MAREA_LexFixture shiftEqualFixture;

    ASSERT_EQ(shiftEqualFixture.MAREA_lex(">>="), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(shiftEqualFixture.MAREA_kinds()[0], MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL) << "'>>=' must lex as one SHIFT_RIGHT_EQUAL token.";

    MAREA_LexFixture greaterEqualFixture;

    ASSERT_EQ(greaterEqualFixture.MAREA_lex(">="), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(greaterEqualFixture.MAREA_kinds()[0], MAREA_TOKEN_KIND_GREATER_EQUAL) << "'>=' must lex as one GREATER_EQUAL token.";
}

TEST(MAREALexerOperators, MinusFamily)
{
    GTEST_COUT << "Validating every minus-prefixed operator lexes correctly and that -- always starts a comment." << std::endl;

    MAREA_LexFixture spacedFixture;

    ASSERT_EQ(spacedFixture.MAREA_lex("a - b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(spacedFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_MINUS, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'a - b' must lex with a single MINUS.";

    for (const MAREA_OperatorRow &row : { MAREA_OperatorRow{ "-%", MAREA_TOKEN_KIND_MINUS_PERCENT }, MAREA_OperatorRow{ "-|", MAREA_TOKEN_KIND_MINUS_PIPE }, MAREA_OperatorRow{ "-=", MAREA_TOKEN_KIND_MINUS_EQUAL } }) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(row.text), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing '" << row.text << "' must succeed.";

        EXPECT_EQ(fixture.MAREA_kinds()[0], row.kind) << "'" << row.text << "' must lex to its own kind.";
    }

    MAREA_LexFixture commentFixture;

    ASSERT_EQ(commentFixture.MAREA_lex("a--b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(commentFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'a--b' must lex as an identifier followed by a comment.";
}
