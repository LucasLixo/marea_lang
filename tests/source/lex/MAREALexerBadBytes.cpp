// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

#include <string>

TEST(MAREALexerBadBytes, UnknownByteKinds)
{
    GTEST_COUT << "Validating every unknown byte lexes as one ERROR token with one UNKNOWN_BYTE diagnostic." << std::endl;

    const std::string BAD_BYTES{ "@$?`\\'" };

    for (const char BYTE : BAD_BYTES) {
        MAREA_LexFixture fixture;

        const std::string SOURCE(1, BYTE);

        ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

        EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "Byte '" << BYTE << "' must lex as a single ERROR token.";

        const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

        ASSERT_EQ(CODES.size(), 1U) << "Byte '" << BYTE << "' must report exactly one diagnostic.";

        EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE)) << "The diagnostic must be UNKNOWN_BYTE.";
    }
}

TEST(MAREALexerBadBytes, UnknownByteDoesNotStopLexing)
{
    GTEST_COUT << "Validating an unknown byte does not stop lexing of the rest of the source." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("a @ b"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "Lexing must continue past the unknown byte.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes().size(), 1U) << "There must be exactly one diagnostic for the one unknown byte.";
}

TEST(MAREALexerBadBytes, NonAsciiByteReported)
{
    GTEST_COUT << "Validating a non-ASCII byte sequence is reported as two unknown byte diagnostics, one per byte." << std::endl;

    MAREA_LexFixture fixture;

    const std::string SOURCE{ "\xC3\xA9" };

    ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "Each byte of the non-ASCII sequence must lex as its own ERROR token.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes().size(), 2U) << "There must be exactly two diagnostics, one per bad byte.";
}

TEST(MAREALexerBadBytes, NulByteReported)
{
    GTEST_COUT << "Validating a NUL byte inside the length-based source is reported as an unknown byte, not treated as a terminator." << std::endl;

    MAREA_LexFixture fixture;

    const std::string SOURCE{ "a\0b", 3 };

    ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_IDENTIFIER, MAREA_TOKEN_KIND_END_OF_FILE })) << "The NUL byte must lex as its own ERROR token between the two identifiers.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes().size(), 1U) << "There must be exactly one diagnostic for the NUL byte.";
}

TEST(MAREALexerBadBytes, ControlCharacterReported)
{
    GTEST_COUT << "Validating every tested control character is reported as an unknown byte." << std::endl;

    const std::string CONTROL_BYTES{ "\x01\x0B\x0C\x7F" };

    for (const char BYTE : CONTROL_BYTES) {
        MAREA_LexFixture fixture;

        const std::string SOURCE(1, BYTE);

        ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

        EXPECT_EQ(fixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "Control byte must lex as a single ERROR token.";

        const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

        ASSERT_EQ(CODES.size(), 1U) << "Control byte must report exactly one diagnostic.";

        EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE)) << "The diagnostic must be UNKNOWN_BYTE.";
    }
}

TEST(MAREALexerBadBytes, LoneHashReported)
{
    GTEST_COUT << "Validating a lone # that does not start #[ or #![ is reported as an unknown byte." << std::endl;

    MAREA_LexFixture firstFixture;

    ASSERT_EQ(firstFixture.MAREA_lex("#"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(firstFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_END_OF_FILE })) << "A lone '#' must lex as a single ERROR token.";

    MAREA_LexFixture secondFixture;

    ASSERT_EQ(secondFixture.MAREA_lex("#!"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(secondFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_BANG, MAREA_TOKEN_KIND_END_OF_FILE })) << "'#!' without '[' must lex as a bad '#' followed by a plain BANG.";

    MAREA_LexFixture thirdFixture;

    ASSERT_EQ(thirdFixture.MAREA_lex("# ["), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(thirdFixture.MAREA_kinds(), (std::vector<MAREA_TokenKind>{ MAREA_TOKEN_KIND_ERROR, MAREA_TOKEN_KIND_LEFT_BRACKET, MAREA_TOKEN_KIND_END_OF_FILE })) << "'# [' must lex as a bad '#' followed by a plain LEFT_BRACKET.";
}

TEST(MAREALexerBadBytes, PositionPointsAtBadByte)
{
    GTEST_COUT << "Validating an unknown byte's diagnostic position and token position both point exactly at that byte." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("ab@cd"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_SourcePosition> POSITIONS{ fixture.MAREA_positions() };

    ASSERT_GE(POSITIONS.size(), 2U) << "There must be at least an identifier and the bad byte token.";

    EXPECT_EQ(POSITIONS[1].offset, MAREA_UINTEGER_32(2)) << "The bad byte token must start at offset 2.";

    EXPECT_EQ(POSITIONS[1].length, MAREA_UINTEGER_32(1)) << "The bad byte token must have length 1.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sink, MAREA_INTEGER_SIZE(0), &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "There must be a diagnostic.";

    EXPECT_EQ(MAREA_DiagnosticGetPosition(diagnostic).offset, MAREA_UINTEGER_32(2)) << "The diagnostic must point at offset 2, the bad byte.";
}

TEST(MAREALexerBadBytes, ManyBadBytesAllReported)
{
    GTEST_COUT << "Validating 100 unknown bytes in a row produce exactly 100 diagnostics." << std::endl;

    MAREA_LexFixture fixture;

    const std::string SOURCE(100, '@');

    ASSERT_EQ(fixture.MAREA_lex(SOURCE), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_EQ(fixture.MAREA_diagnosticCodes().size(), 100U) << "There must be exactly 100 diagnostics, one per bad byte.";

    const std::vector<MAREA_TokenKind> KINDS{ fixture.MAREA_kinds() };

    ASSERT_EQ(KINDS.size(), 101U) << "There must be 100 ERROR tokens plus END_OF_FILE.";

    EXPECT_EQ(KINDS.back(), MAREA_TOKEN_KIND_END_OF_FILE) << "The list must end with END_OF_FILE.";
}
