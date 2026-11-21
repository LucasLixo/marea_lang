#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

TEST(MAREALexerStrings, SimpleString)
{
    GTEST_COUT << "Validating a simple string literal's text includes both quotes." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\"hello\""), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must lex as a single string literal.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "\"hello\"", "" })) << "The token's text must include both quotes.";

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "A simple string must not report any diagnostic.";
}

TEST(MAREALexerStrings, EmptyString)
{
    GTEST_COUT << "Validating an empty string literal lexes as a single two-byte token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\"\""), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "\"\"", "" })) << "An empty string must lex as the two-byte token \"\".";
}

TEST(MAREALexerStrings, StringKeepsCommentMarker)
{
    GTEST_COUT << "Validating a comment marker inside a string is ordinary string content, not a comment." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\"-- x\""), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "\"-- x\"", "" })) << "The whole string, comment marker included, must be the token's text.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must lex as a single string literal.";
}

TEST(MAREALexerStrings, AllEscapesAccepted)
{
    GTEST_COUT << "Validating every accepted escape sequence lexes without a diagnostic." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex(R"("\n\t\r\0\\\"\x41")"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must lex as a single string literal.";

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Every accepted escape must report no diagnostic.";
}

TEST(MAREALexerStrings, EscapedQuoteDoesNotEnd)
{
    GTEST_COUT << "Validating an escaped quote does not close the string." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex(R"("a\"b")"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The whole source, escaped quote included, must be a single string literal.";

    EXPECT_EQ(fixture.MAREA_texts()[0], R"("a\"b")") << "The token's text must span the escaped quote.";
}

TEST(MAREALexerStrings, AdjacentStrings)
{
    GTEST_COUT << "Validating two adjacent strings lex as two separate tokens." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\"a\"\"b\""), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must lex as two string literals.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "\"a\"", "\"b\"", "" })) << "Each string must keep its own quotes.";
}

TEST(MAREALexerStrings, UnterminatedAtNewline)
{
    GTEST_COUT << "Validating a string cut off by a line end is reported as unterminated and lexing resumes on the next line." << std::endl;

    MAREA_LexFixture lfFixture;

    ASSERT_EQ(lfFixture.MAREA_lex("\"abc\ndef"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(lfFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "The unterminated string must be one ERROR token, and lexing must resume with the next line's identifier.";

    const std::vector<MAREA_Uinteger32> LF_CODES{ lfFixture.MAREA_diagnosticCodes() };

    ASSERT_EQ(LF_CODES.size(), 1U) << "There must be exactly one diagnostic.";

    EXPECT_EQ(LF_CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING)) << "The diagnostic must be UNTERMINATED_STRING.";

    MAREA_LexFixture crlfFixture;

    ASSERT_EQ(crlfFixture.MAREA_lex("\"abc\r\ndef"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(crlfFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "A CRLF line end must also stop the string, and lexing must resume with the next line's identifier.";

    EXPECT_EQ(crlfFixture.MAREA_diagnosticCodes().size(), 1U) << "There must be exactly one diagnostic for the CRLF case too.";
}

TEST(MAREALexerStrings, UnterminatedAtEndOfFile)
{
    GTEST_COUT << "Validating a string cut off by end of input is reported as unterminated." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\"abc"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "The unterminated string must be a single ERROR token.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

    ASSERT_EQ(CODES.size(), 1U) << "There must be exactly one diagnostic.";

    EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING)) << "The diagnostic must be UNTERMINATED_STRING.";
}

TEST(MAREALexerStrings, InvalidEscape)
{
    GTEST_COUT << "Validating a bad escape reports INVALID_ESCAPE while the string stays one token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex(R"("a\qb")"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must still lex as a single string literal token.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

    ASSERT_EQ(CODES.size(), 1U) << "There must be exactly one diagnostic.";

    EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE)) << "The diagnostic must be INVALID_ESCAPE.";
}

TEST(MAREALexerStrings, BadHexEscape)
{
    GTEST_COUT << "Validating a malformed \\x escape reports INVALID_ESCAPE while the string stays one token." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex(R"("a\xZ1b")"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must still lex as a single string literal token.";

    EXPECT_EQ(firstFixture.MAREA_diagnosticCodes(), (std::vector<MAREA_Uinteger32>{ static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE) })) << "'\\xZ1' must report exactly one INVALID_ESCAPE diagnostic.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex(R"("a\x4")"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must still lex as a single string literal token.";

    EXPECT_EQ(secondFixture.MAREA_diagnosticCodes(), (std::vector<MAREA_Uinteger32>{ static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE) })) << "'\\x4' with only one hex digit before the closing quote must report exactly one INVALID_ESCAPE diagnostic.";
}

TEST(MAREALexerStrings, BackslashAtEndOfFile)
{
    GTEST_COUT << "Validating a trailing backslash with nothing after it is reported as unterminated." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("\"abc\\"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "The unterminated string must be a single ERROR token.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

    ASSERT_EQ(CODES.size(), 1U) << "There must be exactly one diagnostic.";

    EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING)) << "The diagnostic must be UNTERMINATED_STRING.";
}

TEST(MAREALexerStrings, StringTokenPositions)
{
    GTEST_COUT << "Validating a string token's offset and length cover exactly its quotes and content." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("  \"abc\""), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_FALSE(POSITIONS.empty()) << "There must be at least one token.";

    EXPECT_EQ(POSITIONS[0].offset, MAREA_UINTEGER_32(2)) << "The string must start right after the leading spaces.";

    EXPECT_EQ(POSITIONS[0].length, MAREA_UINTEGER_32(5)) << "The string's length must cover both quotes and the 3 content bytes.";
}

TEST(MAREALexerStrings, NonAsciiInString)
{
    GTEST_COUT << "Validating a non-ASCII byte inside a string reports UNKNOWN_BYTE while the string continues." << std::endl;

    MAREA_LexFixture fixture;

    const std::string SOURCE{ "\"a\xC3\xA9z\"" };

    ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_STRING_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must still lex as a single string literal token.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

    ASSERT_EQ(CODES.size(), 2U) << "Each non-ASCII byte must report its own diagnostic.";

    EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE)) << "The diagnostic must be UNKNOWN_BYTE.";

    EXPECT_EQ(CODES[1], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE)) << "The diagnostic must be UNKNOWN_BYTE.";
}
