// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 ZERO{ MAREA_SEMA_DIAGNOSTIC_CODE_DIVISION_BY_ZERO };
    constexpr MAREA_Uinteger32 TOO_LARGE{ MAREA_SEMA_DIAGNOSTIC_CODE_SHIFT_COUNT_TOO_LARGE };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }

    Codes MAREA_codesOfFunction(const std::string &parameters, const std::string &statements)
    {
        return MAREA_codesOf("function f(" + parameters + ") " + statements + " end");
    }
} // namespace

TEST(MAREASemaConstantOperand, DivisionByConstantZeroIsReported)
{
    GTEST_COUT << "Validating that / and % by a constant zero raise DIVISION_BY_ZERO." << std::endl;

    for (const char *zero : { "0", "0x0", "0b0", "-0", "(0)", "0_0" }) {
        const std::string Z{ zero };

        EXPECT_EQ(MAREA_codesOfFunction("x: sinteger32", "local a: sinteger32 = x / " + Z + ";"), Codes{ ZERO }) << "Division by " << Z;

        EXPECT_EQ(MAREA_codesOfFunction("x: sinteger32", "local a: sinteger32 = x % " + Z + ";"), Codes{ ZERO }) << "Remainder by " << Z;
    }

    EXPECT_EQ(MAREA_codesOfFunction("", "local a: sinteger32 = 1 / 0;"), Codes{ ZERO }) << "Two literals must raise too.";

    EXPECT_EQ(MAREA_codesOfFunction("x: uinteger8", "local a: uinteger8 = x / 0;"), Codes{ ZERO }) << "An unsigned operand must raise too.";
}

TEST(MAREASemaConstantOperand, CompoundDivisionByConstantZeroIsReported)
{
    GTEST_COUT << "Validating that /= and %= by a constant zero raise DIVISION_BY_ZERO." << std::endl;

    EXPECT_EQ(MAREA_codesOfFunction("x: sinteger32", "x /= 0;"), Codes{ ZERO }) << "/= by zero.";

    EXPECT_EQ(MAREA_codesOfFunction("x: sinteger32", "x %= 0x0;"), Codes{ ZERO }) << "%= by zero.";
}

TEST(MAREASemaConstantOperand, DivisionSilentCases)
{
    GTEST_COUT << "Validating that a non-zero or non-constant divisor and a float divisor raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOfFunction("x: sinteger32", "local a: sinteger32 = x / 2;").empty()) << "A divisor of 2 is fine.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: sinteger32, y: sinteger32", "local a: sinteger32 = x / y;").empty()) << "A divisor that is a name is not folded.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: sinteger32", "local a: sinteger32 = x % 10;").empty()) << "A divisor of 10 is fine.";

    EXPECT_TRUE(MAREA_codesOfFunction("", "local a: float64 = 1.0 / 0.0;").empty()) << "A float divisor is out of scope.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: float64", "local a: float64 = x / 0.0;").empty()) << "A float divisor is out of scope.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: sinteger32", "x /= 2;").empty()) << "/= by 2 is fine.";
}

TEST(MAREASemaConstantOperand, ShiftCountAtOrAboveWidthIsReported)
{
    GTEST_COUT << "Validating that a shift count not below the width raises SHIFT_COUNT_TOO_LARGE." << std::endl;

    EXPECT_EQ(MAREA_codesOfFunction("x: uinteger32", "local a: uinteger32 = x << 32;"), Codes{ TOO_LARGE }) << "uinteger32 << 32.";

    EXPECT_EQ(MAREA_codesOfFunction("x: sinteger64", "local a: sinteger64 = x >> 64;"), Codes{ TOO_LARGE }) << "sinteger64 >> 64.";

    EXPECT_EQ(MAREA_codesOfFunction("x: uinteger8", "x <<= 8;"), Codes{ TOO_LARGE }) << "uinteger8 <<= 8.";

    EXPECT_EQ(MAREA_codesOfFunction("x: sinteger32", "x >>= 0x20;"), Codes{ TOO_LARGE }) << "sinteger32 >>= 32 as hexadecimal.";

    EXPECT_EQ(MAREA_codesOfFunction("x: uinteger64", "local a: uinteger64 = x << 100;"), Codes{ TOO_LARGE }) << "uinteger64 << 100.";

    EXPECT_EQ(MAREA_codesOfFunction("x: uinteger32", "local a: uinteger32 = x << (32);"), Codes{ TOO_LARGE }) << "A parenthesized count.";

    EXPECT_EQ(MAREA_codesOfFunction("x: uinteger32", "local a: uinteger32 = x << -1;"), Codes{ TOO_LARGE }) << "A negative count.";
}

TEST(MAREASemaConstantOperand, ShiftCountBelowWidthIsSilent)
{
    GTEST_COUT << "Validating that a shift count below the width raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOfFunction("x: uinteger32", "local a: uinteger32 = x << 31;").empty()) << "uinteger32 << 31.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: uinteger8", "x <<= 7;").empty()) << "uinteger8 <<= 7.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: uinteger64", "local a: uinteger64 = x >> 63;").empty()) << "uinteger64 >> 63.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: sinteger32", "local a: sinteger32 = x >> 0;").empty()) << "A count of 0.";

    EXPECT_TRUE(MAREA_codesOfFunction("x: uinteger32, n: uinteger32", "local a: uinteger32 = x << n;").empty()) << "A count that is a name is not folded.";

    EXPECT_TRUE(MAREA_codesOfFunction("", "local a: sinteger32 = 1 << 40;").empty()) << "A literal left side has no width.";
}
