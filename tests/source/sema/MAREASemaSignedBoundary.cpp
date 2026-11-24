// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 NEGATE_UNSIGNED{ MAREA_SEMA_DIAGNOSTIC_CODE_NEGATE_UNSIGNED };
    constexpr MAREA_Uinteger32 DIVISION_OVERFLOW{ MAREA_SEMA_DIAGNOSTIC_CODE_SIGNED_DIVISION_OVERFLOW };

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
} // namespace

TEST(MAREASemaSignedBoundary, NegationOfAnUnsignedValueIsRejected)
{
    GTEST_COUT << "Validating that unary minus over an unsigned value raises NEGATE_UNSIGNED." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(value: uinteger32) local other: uinteger32 = -value; end"), Codes{ NEGATE_UNSIGNED }) << "A negated unsigned name must raise NEGATE_UNSIGNED.";

    EXPECT_EQ(MAREA_codesOf("function g(a: uinteger8, b: uinteger8) local other: uinteger8 = -(a + b); end"), Codes{ NEGATE_UNSIGNED }) << "A negated unsigned sum must raise NEGATE_UNSIGNED.";

    EXPECT_EQ(MAREA_codesOf("structure Point count: uinteger64; end function g(point: Point) local other: uinteger64 = -point.count; end"), Codes{ NEGATE_UNSIGNED }) << "A negated unsigned field must raise NEGATE_UNSIGNED.";
}

TEST(MAREASemaSignedBoundary, NegationThatIsNotUnsignedStaysSilent)
{
    GTEST_COUT << "Validating that unary minus over a signed value or a literal raises no NEGATE_UNSIGNED." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(value: sinteger32) local other: sinteger32 = -value; end").empty()) << "A negated signed name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(value: float64) local other: float64 = -value; end").empty()) << "A negated float name must raise nothing.";

    for (MAREA_Uinteger32 code : MAREA_codesOf("function g() local other: uinteger32 = -1; end")) {
        EXPECT_NE(code, NEGATE_UNSIGNED) << "A negated literal for an unsigned local belongs to the literal range rule.";
    }
}

TEST(MAREASemaSignedBoundary, MinimumDividedByMinusOneIsRejected)
{
    GTEST_COUT << "Validating that the lowest signed value divided by minus one raises SIGNED_DIVISION_OVERFLOW." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g() local quotient: sinteger32 = -2147483648 / -1; end"), Codes{ DIVISION_OVERFLOW }) << "sinteger32 minimum divided by -1 must raise SIGNED_DIVISION_OVERFLOW.";

    EXPECT_EQ(MAREA_codesOf("function g() local quotient: sinteger8 = -128 / -1; end"), Codes{ DIVISION_OVERFLOW }) << "sinteger8 minimum divided by -1 must raise SIGNED_DIVISION_OVERFLOW.";

    EXPECT_EQ(MAREA_codesOf("function g() local quotient: sinteger16 = -32768 / -1; end"), Codes{ DIVISION_OVERFLOW }) << "sinteger16 minimum divided by -1 must raise SIGNED_DIVISION_OVERFLOW.";

    EXPECT_EQ(MAREA_codesOf("function g() local quotient: sinteger64 = -9223372036854775808 / -1; end"), Codes{ DIVISION_OVERFLOW }) << "sinteger64 minimum divided by -1 must raise SIGNED_DIVISION_OVERFLOW.";

    EXPECT_EQ(MAREA_codesOf("function g() local rest: sinteger32 = -2147483648 % -1; end"), Codes{ DIVISION_OVERFLOW }) << "A modulo of the minimum by -1 must raise SIGNED_DIVISION_OVERFLOW.";

    EXPECT_EQ(MAREA_codesOf("function g() local quotient: sinteger32 = (-2147483648) / (-1); end"), Codes{ DIVISION_OVERFLOW }) << "Parentheses must not hide the shape.";

    EXPECT_EQ(MAREA_codesOf("constant QUOTIENT: sinteger32 = -2147483648 / -1;"), Codes{ DIVISION_OVERFLOW }) << "A constant must be checked too.";

    EXPECT_EQ(MAREA_codesOf("function g(): sinteger32 return -2147483648 / -1; end"), Codes{ DIVISION_OVERFLOW }) << "A return value must be checked too.";
}

TEST(MAREASemaSignedBoundary, DivisionThatHasADefinedResultStaysSilent)
{
    GTEST_COUT << "Validating that a division with a defined result raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g() local quotient: sinteger32 = -2147483648 / 2; end").empty()) << "A minimum divided by 2 must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g() local quotient: sinteger64 = -2147483648 / -1; end").empty()) << "The sinteger32 minimum is not the sinteger64 minimum.";

    EXPECT_TRUE(MAREA_codesOf("function g() local quotient: sinteger32 = 7 / -1; end").empty()) << "A division of 7 by -1 must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(value: sinteger32) local quotient: sinteger32 = value / -1; end").empty()) << "A name on the left is not folded and must raise nothing.";
}
