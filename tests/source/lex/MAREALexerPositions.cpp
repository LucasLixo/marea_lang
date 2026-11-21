#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

TEST(MAREALexerPositions, LineAndColumnAreOneBased)
{
    GTEST_COUT << "Validating the first token of a source starts at line 1, column 1." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("abc"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_FALSE(POSITIONS.empty()) << "There must be at least one token.";

    EXPECT_EQ(POSITIONS[0].line, MAREA_UINTEGER_32(1)) << "The first token must start at line 1.";

    EXPECT_EQ(POSITIONS[0].column, MAREA_UINTEGER_32(1)) << "The first token must start at column 1.";
}

TEST(MAREALexerPositions, LineAndColumnAdvanceAcrossNewlines)
{
    GTEST_COUT << "Validating line and column advance correctly across several lines." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a\nbc\nd"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_GE(POSITIONS.size(), 3U) << "There must be at least 3 identifier tokens.";

    EXPECT_EQ(POSITIONS[0].line, MAREA_UINTEGER_32(1)) << "'a' must be on line 1.";

    EXPECT_EQ(POSITIONS[0].column, MAREA_UINTEGER_32(1)) << "'a' must be at column 1.";

    EXPECT_EQ(POSITIONS[1].line, MAREA_UINTEGER_32(2)) << "'bc' must be on line 2.";

    EXPECT_EQ(POSITIONS[1].column, MAREA_UINTEGER_32(1)) << "'bc' must be at column 1.";

    EXPECT_EQ(POSITIONS[2].line, MAREA_UINTEGER_32(3)) << "'d' must be on line 3.";

    EXPECT_EQ(POSITIONS[2].column, MAREA_UINTEGER_32(1)) << "'d' must be at column 1.";
}

TEST(MAREALexerPositions, CrLfTreatsCrAsWhitespace)
{
    GTEST_COUT << "Validating a lone CR is whitespace and only LF starts a new line." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a\r\nb"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_GE(POSITIONS.size(), 2U) << "There must be at least 2 identifier tokens.";

    EXPECT_EQ(POSITIONS[0].line, MAREA_UINTEGER_32(1)) << "'a' must be on line 1.";

    EXPECT_EQ(POSITIONS[1].line, MAREA_UINTEGER_32(2)) << "'b' must be on line 2, since only the LF of CRLF starts a new line.";

    EXPECT_EQ(POSITIONS[1].column, MAREA_UINTEGER_32(1)) << "'b' must be at column 1.";
}

TEST(MAREALexerPositions, TabCountsAsOneColumn)
{
    GTEST_COUT << "Validating a tab advances the column by exactly one." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\tb"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_FALSE(POSITIONS.empty()) << "There must be at least one token.";

    EXPECT_EQ(POSITIONS[0].column, MAREA_UINTEGER_32(2)) << "A tab must count as exactly one column, so 'b' starts at column 2.";
}

TEST(MAREALexerPositions, FileNameAppearsOnEveryToken)
{
    GTEST_COUT << "Validating every token carries the file name set through MAREA_LexContextSetFileName." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(MAREA_LexContextSetFileName(fixture.context, "example.marea"), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_LexContextSetFileName must succeed.";

    ASSERT_EQ(fixture.MAREA_lex("a b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    for (const MAREA_SourcePosition &position : fixture.MAREA_positions()) {
        EXPECT_STREQ(position.file, "example.marea") << "Every token must carry the set file name.";
    }
}

TEST(MAREALexerPositions, EndOfFilePosition)
{
    GTEST_COUT << "Validating the END_OF_FILE token's position sits right after the last byte of source." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("ab"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_FALSE(POSITIONS.empty()) << "There must be at least one token.";

    const MAREA_SourcePosition &endPosition{ POSITIONS.back() };

    EXPECT_EQ(endPosition.offset, MAREA_UINTEGER_32(2)) << "END_OF_FILE must sit at the byte offset right after the source.";

    EXPECT_EQ(endPosition.length, MAREA_UINTEGER_32(0)) << "END_OF_FILE must have a zero length.";
}

TEST(MAREALexerPositions, OffsetLengthAndTextPointIntoSource)
{
    GTEST_COUT << "Validating a token's offset, length and text all describe the exact same source span." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("  abc  "), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_Token *token{ nullptr };

    ASSERT_EQ(MAREA_TokenListGetAt(fixture.list, MAREA_INTEGER_SIZE(0), &token), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_TokenListGetAt must succeed.";

    const MAREA_SourcePosition POSITION{ MAREA_TokenGetPosition(token) };

    EXPECT_EQ(POSITION.offset, MAREA_UINTEGER_32(2)) << "The identifier must start at offset 2.";

    EXPECT_EQ(POSITION.length, MAREA_UINTEGER_32(3)) << "The identifier must have length 3.";

    EXPECT_EQ(MAREA_TokenGetText(token), (fixture.ownedSource.c_str() + 2)) << "The token's text must point exactly 2 bytes into the source buffer.";
}
