// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREALexHelpers.hpp"

TEST(MAREALexerDiagnostics, CodeValuesAreStable)
{
    GTEST_COUT << "Validating every lexer diagnostic code has its documented stable value." << std::endl;

    EXPECT_EQ(static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE), MAREA_UINTEGER_32(1001)) << "UNKNOWN_BYTE must be 1001.";

    EXPECT_EQ(static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNTERMINATED_STRING), MAREA_UINTEGER_32(1002)) << "UNTERMINATED_STRING must be 1002.";

    EXPECT_EQ(static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_INVALID_ESCAPE), MAREA_UINTEGER_32(1003)) << "INVALID_ESCAPE must be 1003.";

    EXPECT_EQ(static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_MALFORMED_NUMBER), MAREA_UINTEGER_32(1004)) << "MALFORMED_NUMBER must be 1004.";
}

TEST(MAREALexerDiagnostics, MessagesAreNonEmptyAscii)
{
    GTEST_COUT << "Validating every diagnostic message is a non-empty, ASCII-only sentence." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("@ \"abc\n 0x"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.sink) };

    ASSERT_GT(COUNT, MAREA_INTEGER_SIZE(0)) << "There must be at least one diagnostic.";

    for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
        const MAREA_Diagnostic *diagnostic{ nullptr };

        ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sink, index, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkGetAt must succeed.";

        const MAREA_Character8 *message{ MAREA_DiagnosticGetMessage(diagnostic) };

        ASSERT_NE(message, nullptr) << "A message must not be null.";

        const std::string MESSAGE{ message };

        EXPECT_FALSE(MESSAGE.empty()) << "A message must not be empty.";

        for (const unsigned char BYTE : MESSAGE) {
            EXPECT_LT(BYTE, 128U) << "A message must be ASCII only.";
        }
    }
}

TEST(MAREALexerDiagnostics, CleanSourceHasNoDiagnostics)
{
    GTEST_COUT << "Validating a well-formed source reports no diagnostic at all." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("namespace example.basics;\n\nfunction main() end"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    EXPECT_TRUE(fixture.MAREA_diagnosticCodes().empty()) << "A clean source must report no diagnostic.";
}

TEST(MAREALexerDiagnostics, AppendToExistingSink)
{
    GTEST_COUT << "Validating the lexer appends to a sink that already holds an earlier diagnostic, never clearing it." << std::endl;

    MAREA_LexFixture fixture;

    const MAREA_SourcePosition EARLIER_POSITION{ "earlier.marea", MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(1), MAREA_UINTEGER_32(1), MAREA_UINTEGER_32(1) };

    ASSERT_EQ(MAREA_DiagnosticSinkAdd(fixture.sink, MAREA_UINTEGER_32(1), MAREA_DIAGNOSTIC_SEVERITY_WARNING, EARLIER_POSITION, "an earlier phase diagnostic"), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkAdd must succeed.";

    ASSERT_EQ(fixture.MAREA_lex("@"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_diagnosticCodes() };

    ASSERT_EQ(CODES.size(), 2U) << "The sink must hold the earlier diagnostic plus the lexer's own one.";

    EXPECT_EQ(CODES[0], MAREA_UINTEGER_32(1)) << "The earlier diagnostic must still be first.";

    EXPECT_EQ(CODES[1], static_cast<MAREA_Uinteger32>(MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE)) << "The lexer's diagnostic must be appended after it.";
}

TEST(MAREALexerDiagnostics, SeverityIsError)
{
    GTEST_COUT << "Validating every diagnostic the lexer itself reports carries ERROR severity." << std::endl;

    MAREA_LexFixture fixture;

    ASSERT_EQ(fixture.MAREA_lex("@ \"abc\n 0x"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.sink) };

    ASSERT_GT(COUNT, MAREA_INTEGER_SIZE(0)) << "There must be at least one diagnostic.";

    for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
        const MAREA_Diagnostic *diagnostic{ nullptr };

        ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sink, index, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkGetAt must succeed.";

        EXPECT_EQ(MAREA_DiagnosticGetSeverity(diagnostic), MAREA_DIAGNOSTIC_SEVERITY_ERROR) << "Every lexer diagnostic must carry ERROR severity.";
    }
}
