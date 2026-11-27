// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathVector2.h"

#include "tests/MAREAMathVectorHelpers.hpp"

#include <cstddef>

TEST(MAREAMathVector2, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the lane places of the vectors of two lanes." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathVector2Float32), 16U) << "Vector2Float32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector2Float32), 16U) << "Vector2Float32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathVector2Sinteger32), 16U) << "Vector2Sinteger32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector2Sinteger32), 16U) << "Vector2Sinteger32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathVector2Boolean), 4U) << "Vector2Boolean is 4 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector2Boolean), 4U) << "Vector2Boolean is aligned to 4 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathVector2Float32, x), 0 * sizeof(MAREA_Float32)) << "x of Vector2Float32 is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector2Float32, y), 1 * sizeof(MAREA_Float32)) << "y of Vector2Float32 is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector2Sinteger32, x), 0 * sizeof(MAREA_Sinteger32)) << "x of Vector2Sinteger32 is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector2Sinteger32, y), 1 * sizeof(MAREA_Sinteger32)) << "y of Vector2Sinteger32 is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector2Boolean, x), 0 * sizeof(MAREA_Boolean)) << "x of Vector2Boolean is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector2Boolean, y), 1 * sizeof(MAREA_Boolean)) << "y of Vector2Boolean is lane 1.";
}

TEST(MAREAMathVector2, Dot)
{
    GTEST_COUT << "Validating the dot product sums the lanes from left to right." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkDot();
}

TEST(MAREAMathVector2, LengthAndDistance)
{
    GTEST_COUT << "Validating the length and the distance follow the scalar formula." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkLengthAndDistance();
}

TEST(MAREAMathVector2, NormalizeKeepsZeroAtZero)
{
    GTEST_COUT << "Validating normalize follows the scalar formula and a zero vector stays zero." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkNormalize();
}

TEST(MAREAMathVector2, HugeLengthOverflowsToInfinity)
{
    GTEST_COUT << "Validating the length of huge lanes overflows to infinity." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkHugeLength();
}

TEST(MAREAMathVector2, Mix)
{
    GTEST_COUT << "Validating mix follows the scalar formula inside, at the ends and outside." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkMix();
}

TEST(MAREAMathVector2, ArithmeticFloat32)
{
    GTEST_COUT << "Validating add, subtract, multiply, divide and scale work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkArithmetic();
}

TEST(MAREAMathVector2, ZeroByZeroIsAPositiveNan)
{
    GTEST_COUT << "Validating zero divided by zero is one NaN on every target." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkZeroByZero();
}

TEST(MAREAMathVector2, MinimumMaximumAbsoluteFloat32)
{
    GTEST_COUT << "Validating minimum, maximum and absolute work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkMinimumMaximumAbsolute();
}

TEST(MAREAMathVector2, ClampFloat32KeepsTheBoundOrder)
{
    GTEST_COUT << "Validating clamp looks at the lower bound before the upper bound." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkClamp();
}

TEST(MAREAMathVector2, Trigonometry)
{
    GTEST_COUT << "Validating the trigonometric operations and the angle conversions work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkTrigonometry();
}

TEST(MAREAMathVector2, ExponentialAndRoots)
{
    GTEST_COUT << "Validating the exponential, logarithm, power and root operations work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(2).MAREA_checkExponentialAndRoots();
}

TEST(MAREAMathVector2, ArithmeticSinteger32Wraps)
{
    GTEST_COUT << "Validating the integer arithmetic wraps and clears the padding." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(2).MAREA_checkWrapping();
}

TEST(MAREAMathVector2, DotSinteger32Wraps)
{
    GTEST_COUT << "Validating the integer dot product wraps." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(2).MAREA_checkDot();
}

TEST(MAREAMathVector2, MinimumMaximumClampAbsoluteSinteger32)
{
    GTEST_COUT << "Validating the integer minimum, maximum, clamp and absolute work lane by lane." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(2).MAREA_checkMinimumMaximumClampAbsolute();
}

TEST(MAREAMathVector2, DivisionSinteger32ByZeroGivesZero)
{
    GTEST_COUT << "Validating an integer lane divided by zero gives zero." << std::endl;

    const auto OPERATIONS{ MAREA_MATH_INTEGER_OPERATIONS(2) };

    MAREA_expectIntegers(OPERATIONS.MAREA_binary(OPERATIONS.divide, { 7, 9, 0, 0 }, { 0, 3, 0, 0 }), { 0, 3, 0, 0 }, 2, "divide by zero");
}

TEST(MAREAMathVector2, DivisionSinteger32SmallestByMinusOneWraps)
{
    GTEST_COUT << "Validating the smallest integer divided by minus one wraps." << std::endl;

    const auto OPERATIONS{ MAREA_MATH_INTEGER_OPERATIONS(2) };

    MAREA_expectIntegers(OPERATIONS.MAREA_binary(OPERATIONS.divide, { MAREA_MATH_SMALLEST, 7, 0, 0 }, { -1, 0, 0, 0 }), { MAREA_MATH_SMALLEST, 0, 0, 0 }, 2, "divide the smallest by minus one");
}

TEST(MAREAMathVector2, DivisionSinteger32TruncatesTowardZero)
{
    GTEST_COUT << "Validating the integer division truncates toward zero." << std::endl;

    const auto OPERATIONS{ MAREA_MATH_INTEGER_OPERATIONS(2) };

    MAREA_expectIntegers(OPERATIONS.MAREA_binary(OPERATIONS.divide, { 7, -7, 0, 0 }, { 2, 2, 0, 0 }), { 3, -3, 0, 0 }, 2, "divide with truncation");
}
