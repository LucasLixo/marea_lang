#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

TEST(MAREALexerNumbers, DecimalIntegers)
{
    GTEST_COUT << "Validating plain decimal integers lex as a single INTEGER_LITERAL token each." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("0 7 1234567890"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "0", "7", "1234567890", "" })) << "Each number must lex as its own token.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_INTEGER_LITERAL) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every number here must be an INTEGER_LITERAL.";
    }

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Plain decimal integers must not report any diagnostic.";
}

TEST(MAREALexerNumbers, IntegerWithUnderscores)
{
    GTEST_COUT << "Validating underscores anywhere after the first digit are accepted, even doubled or trailing." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("1_000 1__0 1_"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "1_000", "1__0", "1_", "" })) << "Each underscore form must lex as a single token.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_INTEGER_LITERAL) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every token here must be an INTEGER_LITERAL.";
    }

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Underscored integers must not report any diagnostic.";
}

TEST(MAREALexerNumbers, HexIntegers)
{
    GTEST_COUT << "Validating 0x hexadecimal integers lex as a single INTEGER_LITERAL token each." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("0x0 0xFF 0xff 0xDeAd_BEEF 0xFF00FF00"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "0x0", "0xFF", "0xff", "0xDeAd_BEEF", "0xFF00FF00", "" })) << "Each hexadecimal form must lex as a single token.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_INTEGER_LITERAL) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every token here must be an INTEGER_LITERAL.";
    }

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Valid hexadecimal integers must not report any diagnostic.";
}

TEST(MAREALexerNumbers, BinaryIntegers)
{
    GTEST_COUT << "Validating 0b binary integers lex as a single INTEGER_LITERAL token each." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("0b0 0b1010_0101"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "0b0", "0b1010_0101", "" })) << "Each binary form must lex as a single token.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_INTEGER_LITERAL) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every token here must be an INTEGER_LITERAL.";
    }

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Valid binary integers must not report any diagnostic.";
}

TEST(MAREALexerNumbers, FloatForms)
{
    GTEST_COUT << "Validating every accepted float form lexes as a single FLOAT_LITERAL token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("1.0 0.5 3.14159 1_0.0_1 2.4397e6 1e10 1E+5 1.5e-3 6.674e-11"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "1.0", "0.5", "3.14159", "1_0.0_1", "2.4397e6", "1e10", "1E+5", "1.5e-3", "6.674e-11", "" })) << "Each float form must lex as a single token.";

    for (const MAREA_TokenKind KIND : fixture.MAREA_kinds()) {
        EXPECT_TRUE((KIND == MAREA_TOKEN_KIND_FLOAT_LITERAL) || (KIND == MAREA_TOKEN_KIND_END_OF_FILE)) << "Every number here must be a FLOAT_LITERAL.";
    }

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Valid float forms must not report any diagnostic.";
}

TEST(MAREALexerNumbers, FloatNeedsDigitAfterPoint)
{
    GTEST_COUT << "Validating a dot not immediately followed by a digit never starts a float fraction." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("1."), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_DOT, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1.' must lex as an integer then a dot.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("1.a"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_DOT, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1.a' must lex as an integer, a dot and an identifier.";

    MAREA_LexFixture thirdFixture;

    ASSERT_EQ(thirdFixture.MAREA_lex("1.e5"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(thirdFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_DOT, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1.e5' must lex as an integer, a dot and the identifier e5.";
}

TEST(MAREALexerNumbers, RangeIsNotFloat)
{
    GTEST_COUT << "Validating a range's two dots never start a float fraction." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("1..2"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_DOT_DOT, MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1..2' must lex as 1, '..', 2.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("0..10"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_DOT_DOT, MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "'0..10' must lex as 0, '..', 10.";

    MAREA_LexFixture thirdFixture;

    ASSERT_EQ(thirdFixture.MAREA_lex("a[0..n]"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(thirdFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_LEFT_BRACKET, MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_DOT_DOT, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_RIGHT_BRACKET, MAREA_TOKEN_KIND_END_OF_FILE })) << "'a[0..n]' must keep the range as 0, '..', n.";
}

TEST(MAREALexerNumbers, FloatThenDot)
{
    GTEST_COUT << "Validating a float followed by a dot does not merge the extra dot into the float." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("1.5.3"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_FLOAT_LITERAL, MAREA_TOKEN_KIND_DOT, MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1.5.3' must lex as the float 1.5, a dot, then the integer 3.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "1.5", ".", "3", "" })) << "The float must stop exactly at '1.5'.";
}

TEST(MAREALexerNumbers, ExponentNeedsDigits)
{
    GTEST_COUT << "Validating an exponent marker with no digits after it never starts a float exponent." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("1e"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1e' must lex as the integer 1 followed by the identifier e.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("1e+"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_PLUS, MAREA_TOKEN_KIND_END_OF_FILE })) << "'1e+' must lex as the integer 1, the identifier e, then a plus.";
}

TEST(MAREALexerNumbers, NumberGluedToLetters)
{
    GTEST_COUT << "Validating a number immediately followed by letters splits into two tokens with no diagnostic." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("12abc"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "'12abc' must split into the integer 12 and the identifier abc.";

    EXPECT_TRUE(firstFixture.MAREA_diagnosticCodes().empty()) << "Splitting a number from trailing letters is not a lexer diagnostic.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("0b102"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_texts(), (std::vector<std::string>{ "0b10", "2", "" })) << "'0b102' must split into the binary integer 0b10 and the decimal integer 2.";

    EXPECT_TRUE(secondFixture.MAREA_diagnosticCodes().empty()) << "Splitting a binary literal from a trailing non-binary digit is not a lexer diagnostic.";
}

TEST(MAREALexerNumbers, MalformedPrefixReportsDiagnostic)
{
    GTEST_COUT << "Validating a 0x or 0b prefix with no valid digit reports MALFORMED_NUMBER and an ERROR token, then keeps lexing." << std::endl;

    const char *malformedSources[]{ "0x", "0b", "0xg", "0b2" };

    for (const char *source : malformedSources) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(source), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing '" << source << "' must succeed.";

        const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

        ASSERT_FALSE(KINDS.empty()) << "There must be at least one token.";

        EXPECT_EQ(KINDS[0], MAREA_TOKEN_KIND_ERROR) << "'" << source << "' must start with an ERROR token.";

        const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

        ASSERT_FALSE(CODES.empty()) << "'" << source << "' must report a diagnostic.";

        EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_MALFORMED_NUMBER)) << "The diagnostic must be MALFORMED_NUMBER.";
    }
}

TEST(MAREALexerNumbers, NegativeIsUnaryAndZerosKept)
{
    GTEST_COUT << "Validating a leading minus is its own token and leading zeros stay inside one integer token." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("-5"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_MINUS, MAREA_TOKEN_KIND_INTEGER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "'-5' must lex as MINUS then the integer 5.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("007"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_texts(), (std::vector<std::string>{ "007", "" })) << "'007' must lex as a single integer token with its leading zeros kept.";
}
