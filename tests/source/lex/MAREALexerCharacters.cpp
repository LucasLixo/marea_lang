// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

TEST(MAREALexerCharacters, SimpleCharacter)
{
    GTEST_COUT << "Validating a simple character literal lexes as one token whose text includes both quotes." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("'a'"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_CHARACTER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The source must lex as a single character literal.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "'a'", "" })) << "The token's text must include both quotes.";

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "A simple character must not report any diagnostic.";
}

TEST(MAREALexerCharacters, DoubleQuoteInsideCharacter)
{
    GTEST_COUT << "Validating a double quote needs no escape inside a character literal." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("'\"'"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_CHARACTER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "A double quote must be a valid character.";

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "There must be no diagnostic.";
}

TEST(MAREALexerCharacters, EscapedCharacters)
{
    GTEST_COUT << "Validating every escape of a character literal lexes as one token without a diagnostic." << std::endl;

    const std::vector<std::string> SOURCES{ "'\\n'", "'\\t'", "'\\r'", "'\\0'", "'\\\\'", "'\\\"'", "'\\''", "'\\x41'" };

    for (const std::string &source : SOURCES) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(source), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

        EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_CHARACTER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "Source " << source << " must lex as one character literal.";

        EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "Source " << source << " must not report a diagnostic.";
    }
}

TEST(MAREALexerCharacters, EmptyCharacterReported)
{
    GTEST_COUT << "Validating an empty character literal reports EMPTY_CHARACTER and lexes as an ERROR token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("''"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "An empty literal must lex as one ERROR token.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes(), (std::vector<MAREA_Uinteger32>{ static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_EMPTY_CHARACTER) })) << "The diagnostic must be EMPTY_CHARACTER.";
}

TEST(MAREALexerCharacters, MultiCharacterReported)
{
    GTEST_COUT << "Validating a literal with more than one character reports TOO_MANY_CHARACTERS and lexes as an ERROR token." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("'ab'"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "A long literal must lex as one ERROR token.";

    EXPECT_EQ(fixture.MAREA_texts(), (std::vector<std::string>{ "'ab'", "" })) << "The ERROR token must cover the whole literal.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes(), (std::vector<MAREA_Uinteger32>{ static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_TOO_MANY_CHARACTERS) })) << "The diagnostic must be TOO_MANY_CHARACTERS.";
}

TEST(MAREALexerCharacters, EscapeCountsAsOneCharacter)
{
    GTEST_COUT << "Validating an escape followed by a character is two characters." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("'\\na'"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes(), (std::vector<MAREA_Uinteger32>{ static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_TOO_MANY_CHARACTERS) })) << "An escape plus a character must be too many.";
}

TEST(MAREALexerCharacters, UnterminatedCharacterReported)
{
    GTEST_COUT << "Validating a character literal cut off by the end of input or a line end reports UNTERMINATED_STRING." << std::endl;

    for (const std::string &source : std::vector<std::string>{ "'a", "'", "'a\nb" }) {
        MAREA_LexFixture fixture;

        ASSERT_EQ(fixture.MAREA_lex(source), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

        EXPECT_EQ(fixture.MAREA_kinds().front(), MAREA_TOKEN_KIND_ERROR) << "Source " << source << " must start with an ERROR token.";

        EXPECT_EQ(fixture.MAREA_diagnosticCodes().front(), static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING)) << "Source " << source << " must report an unterminated literal.";
    }
}

TEST(MAREALexerCharacters, InvalidEscapeReported)
{
    GTEST_COUT << "Validating a bad escape inside a character reports INVALID_ESCAPE and the literal still closes." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("'\\q'"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_CHARACTER_LITERAL, MAREA_TOKEN_KIND_END_OF_FILE })) << "The literal must still close.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes(), (std::vector<MAREA_Uinteger32>{ static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE) })) << "The diagnostic must be INVALID_ESCAPE.";
}

TEST(MAREALexerCharacters, LexingContinuesAfterCharacter)
{
    GTEST_COUT << "Validating tokens after a character literal are lexed normally." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("x = 'a';"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_EQUAL, MAREA_TOKEN_KIND_CHARACTER_LITERAL, MAREA_TOKEN_KIND_SEMICOLON, MAREA_TOKEN_KIND_END_OF_FILE })) << "The tokens around the literal must be unchanged.";
}
