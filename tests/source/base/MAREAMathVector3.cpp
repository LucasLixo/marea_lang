// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathVector3.h"

#include "tests/MAREAMathVectorHelpers.hpp"

#include <cstddef>

TEST(MAREAMathVector3, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the lane places of the vectors of three lanes." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathVector3Float32), 16U) << "Vector3Float32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector3Float32), 16U) << "Vector3Float32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathVector3Sinteger32), 16U) << "Vector3Sinteger32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector3Sinteger32), 16U) << "Vector3Sinteger32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathVector3Boolean), 4U) << "Vector3Boolean is 4 bytes.";

    EXPECT_EQ(alignof(MAREA_MathVector3Boolean), 4U) << "Vector3Boolean is aligned to 4 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Float32, x), 0 * sizeof(MAREA_Float32)) << "x of Vector3Float32 is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Float32, y), 1 * sizeof(MAREA_Float32)) << "y of Vector3Float32 is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Float32, z), 2 * sizeof(MAREA_Float32)) << "z of Vector3Float32 is lane 2.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Sinteger32, x), 0 * sizeof(MAREA_Sinteger32)) << "x of Vector3Sinteger32 is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Sinteger32, y), 1 * sizeof(MAREA_Sinteger32)) << "y of Vector3Sinteger32 is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Sinteger32, z), 2 * sizeof(MAREA_Sinteger32)) << "z of Vector3Sinteger32 is lane 2.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Boolean, x), 0 * sizeof(MAREA_Boolean)) << "x of Vector3Boolean is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Boolean, y), 1 * sizeof(MAREA_Boolean)) << "y of Vector3Boolean is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathVector3Boolean, z), 2 * sizeof(MAREA_Boolean)) << "z of Vector3Boolean is lane 2.";
}

TEST(MAREAMathVector3, Dot)
{
    GTEST_COUT << "Validating the dot product sums the lanes from left to right." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkDot();
}

TEST(MAREAMathVector3, LengthAndDistance)
{
    GTEST_COUT << "Validating the length and the distance follow the scalar formula." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkLengthAndDistance();
}

TEST(MAREAMathVector3, NormalizeKeepsZeroAtZero)
{
    GTEST_COUT << "Validating normalize follows the scalar formula and a zero vector stays zero." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkNormalize();
}

TEST(MAREAMathVector3, HugeLengthOverflowsToInfinity)
{
    GTEST_COUT << "Validating the length of huge lanes overflows to infinity." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkHugeLength();
}

TEST(MAREAMathVector3, Mix)
{
    GTEST_COUT << "Validating mix follows the scalar formula inside, at the ends and outside." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkMix();
}

TEST(MAREAMathVector3, ArithmeticFloat32)
{
    GTEST_COUT << "Validating add, subtract, multiply, divide and scale work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkArithmetic();
}

TEST(MAREAMathVector3, ZeroByZeroIsAPositiveNan)
{
    GTEST_COUT << "Validating zero divided by zero is one NaN on every target." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkZeroByZero();
}

TEST(MAREAMathVector3, MinimumMaximumAbsoluteFloat32)
{
    GTEST_COUT << "Validating minimum, maximum and absolute work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkMinimumMaximumAbsolute();
}

TEST(MAREAMathVector3, ClampFloat32KeepsTheBoundOrder)
{
    GTEST_COUT << "Validating clamp looks at the lower bound before the upper bound." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkClamp();
}

TEST(MAREAMathVector3, Trigonometry)
{
    GTEST_COUT << "Validating the trigonometric operations and the angle conversions work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkTrigonometry();
}

TEST(MAREAMathVector3, ExponentialAndRoots)
{
    GTEST_COUT << "Validating the exponential, logarithm, power and root operations work lane by lane." << std::endl;

    MAREA_MATH_FLOAT_OPERATIONS(3).MAREA_checkExponentialAndRoots();
}

TEST(MAREAMathVector3, ArithmeticSinteger32Wraps)
{
    GTEST_COUT << "Validating the integer arithmetic wraps and clears the padding." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(3).MAREA_checkWrapping();
}

TEST(MAREAMathVector3, DotSinteger32Wraps)
{
    GTEST_COUT << "Validating the integer dot product wraps." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(3).MAREA_checkDot();
}

TEST(MAREAMathVector3, MinimumMaximumClampAbsoluteSinteger32)
{
    GTEST_COUT << "Validating the integer minimum, maximum, clamp and absolute work lane by lane." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(3).MAREA_checkMinimumMaximumClampAbsolute();
}

TEST(MAREAMathVector3, DivisionSinteger32ByZeroRaisesTheTrap)
{
    GTEST_COUT << "Validating an integer lane divided by zero raises the trap of the language." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(3).MAREA_checkDivisionByZero();
}

TEST(MAREAMathVector3, DivisionSinteger32SmallestByMinusOneWraps)
{
    GTEST_COUT << "Validating the smallest integer divided by minus one wraps." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(3).MAREA_expectQuotient({ MAREA_MATH_SMALLEST, 7, MAREA_MATH_SMALLEST, 0 }, { -1, 1, -1, 0 }, { MAREA_MATH_SMALLEST, 7, MAREA_MATH_SMALLEST, 0 }, "divide the smallest by minus one");
}

TEST(MAREAMathVector3, DivisionSinteger32TruncatesTowardZero)
{
    GTEST_COUT << "Validating the integer division truncates toward zero." << std::endl;

    MAREA_MATH_INTEGER_OPERATIONS(3).MAREA_expectQuotient({ 7, -7, 9, 0 }, { 2, 2, -2, 0 }, { 3, -3, -4, 0 }, "divide with truncation");
}

TEST(MAREAMathVector3, CrossFloat32)
{
    GTEST_COUT << "Validating the cross product follows the scalar formula." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        const MAREA_MathVector3Float32 LEFT{ MAREA_vectorOf<MAREA_MathVector3Float32>(pair.first, 3) };

        const MAREA_MathVector3Float32 RIGHT{ MAREA_vectorOf<MAREA_MathVector3Float32>(pair.second, 3) };

        MAREA_MathVector3Float32 result{ MAREA_garbage<MAREA_MathVector3Float32>() };

        MAREA_MathCrossVector3Float32(&LEFT, &RIGHT, &result);

        MAREA_expectFloats(result, MAREA_referenceCross(pair.first, pair.second), 3, "cross", index);

        ++index;
    }
}

TEST(MAREAMathVector3, CrossSinteger32)
{
    GTEST_COUT << "Validating the integer cross product." << std::endl;

    const MAREA_MathVector3Sinteger32 LEFT{ MAREA_vectorOf<MAREA_MathVector3Sinteger32>(MAREA_IntegerLanes{ 1, 2, 3, 0 }, 3) };

    const MAREA_MathVector3Sinteger32 RIGHT{ MAREA_vectorOf<MAREA_MathVector3Sinteger32>(MAREA_IntegerLanes{ 4, 5, 6, 0 }, 3) };

    MAREA_MathVector3Sinteger32 result{ MAREA_garbage<MAREA_MathVector3Sinteger32>() };

    MAREA_MathCrossVector3Sinteger32(&LEFT, &RIGHT, &result);

    MAREA_expectIntegers(result, { -3, 6, -3, 0 }, 3, "cross");
}
