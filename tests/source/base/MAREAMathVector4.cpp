// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathVector4.h"

#include "tests/MAREAMathVectorHelpers.hpp"

#include <cstddef>

TEST(MAREAMathVector4, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the lane places of the vectors of four lanes." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathVector4Float32), 16U) << "Vector4Float32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector4Float32), 16U) << "Vector4Float32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathVector4Sinteger32), 16U) << "Vector4Sinteger32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector4Sinteger32), 16U) << "Vector4Sinteger32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathVector4Boolean), 4U) << "Vector4Boolean is 4 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector4Boolean), 4U) << "Vector4Boolean is aligned to 4 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Float32, x), 0 * sizeof(MAREA_Float32)) << "x of Vector4Float32 is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Float32, y), 1 * sizeof(MAREA_Float32)) << "y of Vector4Float32 is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Float32, z), 2 * sizeof(MAREA_Float32)) << "z of Vector4Float32 is lane 2.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Float32, w), 3 * sizeof(MAREA_Float32)) << "w of Vector4Float32 is lane 3.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Sinteger32, x), 0 * sizeof(MAREA_Sinteger32)) << "x of Vector4Sinteger32 is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Sinteger32, y), 1 * sizeof(MAREA_Sinteger32)) << "y of Vector4Sinteger32 is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Sinteger32, z), 2 * sizeof(MAREA_Sinteger32)) << "z of Vector4Sinteger32 is lane 2.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Sinteger32, w), 3 * sizeof(MAREA_Sinteger32)) << "w of Vector4Sinteger32 is lane 3.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Boolean, x), 0 * sizeof(MAREA_Boolean)) << "x of Vector4Boolean is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Boolean, y), 1 * sizeof(MAREA_Boolean)) << "y of Vector4Boolean is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Boolean, z), 2 * sizeof(MAREA_Boolean)) << "z of Vector4Boolean is lane 2.";

    EXPECT_EQ(offsetof(MAREA_MathVector4Boolean, w), 3 * sizeof(MAREA_Boolean)) << "w of Vector4Boolean is lane 3.";
}

TEST(MAREAMathVector4, Dot)
{
    GTEST_COUT << "Validating the dot product sums the lanes from left to right." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkDot();
}

TEST(MAREAMathVector4, LengthAndDistance)
{
    GTEST_COUT << "Validating the length and the distance follow the scalar formula." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkLengthAndDistance();
}

TEST(MAREAMathVector4, NormalizeKeepsZeroAtZero)
{
    GTEST_COUT << "Validating normalize follows the scalar formula and a zero vector stays zero." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkNormalize();
}

TEST(MAREAMathVector4, HugeLengthOverflowsToInfinity)
{
    GTEST_COUT << "Validating the length of huge lanes overflows to infinity." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkHugeLength();
}

TEST(MAREAMathVector4, Mix)
{
    GTEST_COUT << "Validating mix follows the scalar formula inside, at the ends and outside." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkMix();
}

TEST(MAREAMathVector4, ArithmeticFloat32)
{
    GTEST_COUT << "Validating add, subtract, multiply, divide and scale work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkArithmetic();
}

TEST(MAREAMathVector4, ZeroByZeroIsAPositiveNan)
{
    GTEST_COUT << "Validating zero divided by zero is one NaN on every target." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkZeroByZero();
}

TEST(MAREAMathVector4, MinimumMaximumAbsoluteFloat32)
{
    GTEST_COUT << "Validating minimum, maximum and absolute work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkMinimumMaximumAbsolute();
}

TEST(MAREAMathVector4, ClampFloat32KeepsTheBoundOrder)
{
    GTEST_COUT << "Validating clamp looks at the lower bound before the upper bound." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkClamp();
}

TEST(MAREAMathVector4, Trigonometry)
{
    GTEST_COUT << "Validating the trigonometric operations and the angle conversions work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkTrigonometry();
}

TEST(MAREAMathVector4, ExponentialAndRoots)
{
    GTEST_COUT << "Validating the exponential, logarithm, power and root operations work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(4).MAREA_checkExponentialAndRoots();
}

TEST(MAREAMathVector4, ArithmeticSinteger32Wraps)
{
    GTEST_COUT << "Validating the integer arithmetic wraps and clears the padding." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(4).MAREA_checkWrapping();
}

TEST(MAREAMathVector4, DotSinteger32Wraps)
{
    GTEST_COUT << "Validating the integer dot product wraps." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(4).MAREA_checkDot();
}

TEST(MAREAMathVector4, MinimumMaximumClampAbsoluteSinteger32)
{
    GTEST_COUT << "Validating the integer minimum, maximum, clamp and absolute work lane by lane." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(4).MAREA_checkMinimumMaximumClampAbsolute();
}

TEST(MAREAMathVector4, DivisionSinteger32ByZeroRaisesTheTrap)
{
    GTEST_COUT << "Validating an integer lane divided by zero raises the trap of the language." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(4).MAREA_checkDivisionByZero();
}

TEST(MAREAMathVector4, DivisionSinteger32SmallestByMinusOneWraps)
{
    GTEST_COUT << "Validating the smallest integer divided by minus one wraps." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(4).MAREA_expectQuotient({ MAREA_MATH_SMALLEST, 7, 1, MAREA_MATH_SMALLEST }, { -1, 1, 1, -1 }, { MAREA_MATH_SMALLEST, 7, 1, MAREA_MATH_SMALLEST }, "divide the smallest by minus one");
}

TEST(MAREAMathVector4, DivisionSinteger32TruncatesTowardZero)
{
    GTEST_COUT << "Validating the integer division truncates toward zero." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(4).MAREA_expectQuotient({ 7, -7, 9, -9 }, { 2, 2, -2, -2 }, { 3, -3, -4, 4 }, "divide with truncation");
}
