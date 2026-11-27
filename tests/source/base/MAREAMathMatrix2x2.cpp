// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathMatrix2x2.h"

#include "tests/MAREAMathMatrixHelpers.hpp"

#include <cstddef>

namespace
{
    MAREA_MathMatrix2x2Float32 MAREA_float2x2(std::initializer_list<float> values)
    {
        return MAREA_matrixOf<MAREA_MathMatrix2x2Float32>(MAREA_matrixColumns(2, values));
    }

    MAREA_MathMatrix2x2Sinteger32 MAREA_integer2x2(std::initializer_list<std::int32_t> values)
    {
        return MAREA_matrixOf<MAREA_MathMatrix2x2Sinteger32>(MAREA_integerColumns(2, values));
    }
}

TEST(MAREAMathMatrix2x2, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the column places of the matrices of two by two." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathMatrix2x2Float32), 32U) << "Matrix2x2Float32 is 32 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix2x2Float32), 16U) << "Matrix2x2Float32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathMatrix2x2Sinteger32), 32U) << "Matrix2x2Sinteger32 is 32 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix2x2Sinteger32), 16U) << "Matrix2x2Sinteger32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathMatrix2x2Boolean), 8U) << "Matrix2x2Boolean is 8 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix2x2Boolean), 4U) << "Matrix2x2Boolean is aligned to 4 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix2x2Float32, column0), 0U) << "column0 of Matrix2x2Float32 is first.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix2x2Float32, column1), 16U) << "column1 of Matrix2x2Float32 is at 16.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix2x2Sinteger32, column1), 16U) << "column1 of Matrix2x2Sinteger32 is at 16.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix2x2Boolean, column1), 4U) << "column1 of Matrix2x2Boolean is at 4.";
}

TEST(MAREAMathMatrix2x2, IdentityFloat32)
{
    GTEST_COUT << "Validating the identity has one on the diagonal and clears each other lane." << std::endl;

    MAREA_MathMatrix2x2Float32 result{ MAREA_garbage<MAREA_MathMatrix2x2Float32>() };

    MAREA_MathIdentityMatrix2x2Float32(&result);

    MAREA_expectMatrixBits(result, MAREA_matrixColumns(2, { 1.0F, 0.0F, 0.0F, 1.0F }), "identity");
}

TEST(MAREAMathMatrix2x2, TransposeFloat32)
{
    GTEST_COUT << "Validating the transpose swaps rows and columns, also in place." << std::endl;

    MAREA_MathMatrix2x2Float32 value{ MAREA_float2x2({ 1.0F, 2.0F, 3.0F, 4.0F }) };

    MAREA_MathTransposeMatrix2x2Float32(&value, &value);

    MAREA_expectMatrixBits(value, MAREA_matrixColumns(2, { 1.0F, 3.0F, 2.0F, 4.0F }), "transpose");
}

TEST(MAREAMathMatrix2x2, DeterminantFloat32)
{
    GTEST_COUT << "Validating the determinant of a known matrix and of a singular one." << std::endl;

    const MAREA_MathMatrix2x2Float32 VALUE{ MAREA_float2x2({ 1.0F, 2.0F, 3.0F, 4.0F }) };

    const MAREA_MathMatrix2x2Float32 SINGULAR{ MAREA_float2x2({ 1.0F, 2.0F, 2.0F, 4.0F }) };

    EXPECT_EQ(MAREA_MathDeterminantMatrix2x2Float32(&VALUE), -2.0F) << "1 * 4 - 3 * 2.";

    EXPECT_EQ(MAREA_MathDeterminantMatrix2x2Float32(&SINGULAR), 0.0F) << "A repeated direction has no area.";
}

TEST(MAREAMathMatrix2x2, InverseFloat32)
{
    GTEST_COUT << "Validating the inverse of a known matrix, in place." << std::endl;

    MAREA_MathMatrix2x2Float32 value{ MAREA_float2x2({ 1.0F, 2.0F, 3.0F, 4.0F }) };

    MAREA_MathInverseMatrix2x2Float32(&value, &value);

    MAREA_expectMatrixNear(value, MAREA_matrixColumns(2, { -2.0F, 1.0F, 1.5F, -0.5F }), 1.0e-6F, "inverse");
}

TEST(MAREAMathMatrix2x2, InverseOfASingularMatrixIsZero)
{
    GTEST_COUT << "Validating a singular matrix gives the zero matrix." << std::endl;

    const MAREA_MathMatrix2x2Float32 SINGULAR{ MAREA_float2x2({ 1.0F, 2.0F, 2.0F, 4.0F }) };

    MAREA_MathMatrix2x2Float32 result{ MAREA_garbage<MAREA_MathMatrix2x2Float32>() };

    MAREA_MathInverseMatrix2x2Float32(&SINGULAR, &result);

    MAREA_expectMatrixBits(result, MAREA_MatrixLanes{}, "singular inverse");
}

TEST(MAREAMathMatrix2x2, MultiplyFloat32)
{
    GTEST_COUT << "Validating the matrix product and the product with a vector, also in place." << std::endl;

    MAREA_MathMatrix2x2Float32 left{ MAREA_float2x2({ 1.0F, 2.0F, 3.0F, 4.0F }) };

    const MAREA_MathMatrix2x2Float32 RIGHT{ MAREA_float2x2({ 5.0F, 6.0F, 7.0F, 8.0F }) };

    const MAREA_MathVector2Float32 VECTOR{ MAREA_vectorOf<MAREA_MathVector2Float32>(MAREA_FloatLanes{ 5.0F, 6.0F, 0.0F, 0.0F }, 2) };

    MAREA_MathVector2Float32 product{ MAREA_garbage<MAREA_MathVector2Float32>() };

    MAREA_MathMultiplyMatrix2x2Vector2Float32(&left, &VECTOR, &product);

    MAREA_expectFloatBits(product, { 23.0F, 34.0F, 0.0F, 0.0F }, 2, "matrix by vector");

    MAREA_MathMultiplyMatrix2x2Float32(&left, &RIGHT, &left);

    MAREA_expectMatrixBits(left, MAREA_matrixColumns(2, { 23.0F, 34.0F, 31.0F, 46.0F }), "matrix by matrix");
}

TEST(MAREAMathMatrix2x2, IdentityTransposeAndMultiplySinteger32)
{
    GTEST_COUT << "Validating the integer identity, transpose and products." << std::endl;

    MAREA_MathMatrix2x2Sinteger32 identity{ MAREA_garbage<MAREA_MathMatrix2x2Sinteger32>() };

    MAREA_MathIdentityMatrix2x2Sinteger32(&identity);

    MAREA_expectIntegerMatrix(identity, MAREA_integerColumns(2, { 1, 0, 0, 1 }), "identity");

    const MAREA_MathMatrix2x2Sinteger32 LEFT{ MAREA_integer2x2({ 1, 2, 3, 4 }) };

    const MAREA_MathMatrix2x2Sinteger32 RIGHT{ MAREA_integer2x2({ 5, 6, 7, 8 }) };

    MAREA_MathMatrix2x2Sinteger32 result{ MAREA_garbage<MAREA_MathMatrix2x2Sinteger32>() };

    MAREA_MathTransposeMatrix2x2Sinteger32(&LEFT, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(2, { 1, 3, 2, 4 }), "transpose");

    MAREA_MathMultiplyMatrix2x2Sinteger32(&LEFT, &RIGHT, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(2, { 23, 34, 31, 46 }), "matrix by matrix");

    const MAREA_MathVector2Sinteger32 VECTOR{ MAREA_vectorOf<MAREA_MathVector2Sinteger32>(MAREA_IntegerLanes{ 5, 6, 0, 0 }, 2) };

    MAREA_MathVector2Sinteger32 product{ MAREA_garbage<MAREA_MathVector2Sinteger32>() };

    MAREA_MathMultiplyMatrix2x2Vector2Sinteger32(&LEFT, &VECTOR, &product);

    MAREA_expectIntegers(product, { 23, 34, 0, 0 }, 2, "matrix by vector");
}

TEST(MAREAMathMatrix2x2, MultiplySinteger32Wraps)
{
    GTEST_COUT << "Validating the integer products and sums wrap." << std::endl;

    const MAREA_MathMatrix2x2Sinteger32 LEFT{ MAREA_integer2x2({ MAREA_MATH_LARGEST, 0, MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST }) };

    const MAREA_MathMatrix2x2Sinteger32 RIGHT{ MAREA_integer2x2({ 1, 1, 2, -1 }) };

    MAREA_MathMatrix2x2Sinteger32 result{ MAREA_garbage<MAREA_MathMatrix2x2Sinteger32>() };

    MAREA_MathMultiplyMatrix2x2Sinteger32(&LEFT, &RIGHT, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(2, { -2, MAREA_MATH_SMALLEST, MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST }), "wrapping product");
}
