// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathMatrix3x3.h"

#include "tests/MAREAMathMatrixHelpers.hpp"

#include <cstddef>

namespace
{
    MAREA_MathMatrix3x3Float32 MAREA_float3x3(std::initializer_list<float> values)
    {
        return MAREA_matrixOf<MAREA_MathMatrix3x3Float32>(MAREA_matrixColumns(3, values));
    }

    MAREA_MathMatrix3x3Sinteger32 MAREA_integer3x3(std::initializer_list<std::int32_t> values)
    {
        return MAREA_matrixOf<MAREA_MathMatrix3x3Sinteger32>(MAREA_integerColumns(3, values));
    }
}

TEST(MAREAMathMatrix3x3, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the column places of the matrices of three by three." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathMatrix3x3Float32), 48U) << "Matrix3x3Float32 is 48 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix3x3Float32), 16U) << "Matrix3x3Float32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathMatrix3x3Sinteger32), 48U) << "Matrix3x3Sinteger32 is 48 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix3x3Sinteger32), 16U) << "Matrix3x3Sinteger32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathMatrix3x3Boolean), 12U) << "Matrix3x3Boolean is 12 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix3x3Boolean), 4U) << "Matrix3x3Boolean is aligned to 4 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix3x3Float32, column1), 16U) << "column1 of Matrix3x3Float32 is at 16.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix3x3Float32, column2), 32U) << "column2 of Matrix3x3Float32 is at 32.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix3x3Sinteger32, column2), 32U) << "column2 of Matrix3x3Sinteger32 is at 32.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix3x3Boolean, column2), 8U) << "column2 of Matrix3x3Boolean is at 8.";
}

TEST(MAREAMathMatrix3x3, IdentityFloat32)
{
    GTEST_COUT << "Validating the identity has one on the diagonal and clears each other lane." << std::endl;

    MAREA_MathMatrix3x3Float32 result{ MAREA_garbage<MAREA_MathMatrix3x3Float32>() };

    MAREA_MathIdentityMatrix3x3Float32(&result);

    MAREA_expectMatrixBits(result, MAREA_matrixColumns(3, { 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F }), "identity");
}

TEST(MAREAMathMatrix3x3, TransposeFloat32)
{
    GTEST_COUT << "Validating the transpose swaps rows and columns, also in place." << std::endl;

    MAREA_MathMatrix3x3Float32 value{ MAREA_float3x3({ 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F }) };

    MAREA_MathTransposeMatrix3x3Float32(&value, &value);

    MAREA_expectMatrixBits(value, MAREA_matrixColumns(3, { 1.0F, 4.0F, 7.0F, 2.0F, 5.0F, 8.0F, 3.0F, 6.0F, 9.0F }), "transpose");
}

TEST(MAREAMathMatrix3x3, DeterminantFloat32)
{
    GTEST_COUT << "Validating the determinant of a known matrix and of a singular one." << std::endl;

    const MAREA_MathMatrix3x3Float32 VALUE{ MAREA_float3x3({ 1.0F, 0.0F, 5.0F, 2.0F, 1.0F, 6.0F, 3.0F, 4.0F, 0.0F }) };

    const MAREA_MathMatrix3x3Float32 SINGULAR{ MAREA_float3x3({ 1.0F, 2.0F, 3.0F, 2.0F, 4.0F, 6.0F, 7.0F, 8.0F, 9.0F }) };

    EXPECT_EQ(MAREA_MathDeterminantMatrix3x3Float32(&VALUE), 1.0F) << "The determinant of the known matrix is one.";

    EXPECT_EQ(MAREA_MathDeterminantMatrix3x3Float32(&SINGULAR), 0.0F) << "A repeated direction has no volume.";
}

TEST(MAREAMathMatrix3x3, InverseFloat32)
{
    GTEST_COUT << "Validating the inverse of a known matrix, in place." << std::endl;

    MAREA_MathMatrix3x3Float32 value{ MAREA_float3x3({ 1.0F, 0.0F, 5.0F, 2.0F, 1.0F, 6.0F, 3.0F, 4.0F, 0.0F }) };

    MAREA_MathInverseMatrix3x3Float32(&value, &value);

    MAREA_expectMatrixBits(value, MAREA_matrixColumns(3, { -24.0F, 20.0F, -5.0F, 18.0F, -15.0F, 4.0F, 5.0F, -4.0F, 1.0F }), "inverse");
}

TEST(MAREAMathMatrix3x3, InverseOfASingularMatrixIsZero)
{
    GTEST_COUT << "Validating a singular matrix gives the zero matrix." << std::endl;

    const MAREA_MathMatrix3x3Float32 SINGULAR{ MAREA_float3x3({ 1.0F, 2.0F, 3.0F, 2.0F, 4.0F, 6.0F, 7.0F, 8.0F, 9.0F }) };

    MAREA_MathMatrix3x3Float32 result{ MAREA_garbage<MAREA_MathMatrix3x3Float32>() };

    MAREA_MathInverseMatrix3x3Float32(&SINGULAR, &result);

    MAREA_expectMatrixBits(result, MAREA_MatrixLanes{}, "singular inverse");
}

TEST(MAREAMathMatrix3x3, MultiplyFloat32)
{
    GTEST_COUT << "Validating the matrix product and the product with a vector, also in place." << std::endl;

    MAREA_MathMatrix3x3Float32 left{ MAREA_float3x3({ 1.0F, 0.0F, 5.0F, 2.0F, 1.0F, 6.0F, 3.0F, 4.0F, 0.0F }) };

    const MAREA_MathMatrix3x3Float32 RIGHT{ MAREA_float3x3({ 1.0F, 1.0F, 1.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.0F }) };

    const MAREA_MathVector3Float32 VECTOR{ MAREA_vectorOf<MAREA_MathVector3Float32>(MAREA_FloatLanes{ 1.0F, 2.0F, 3.0F, 0.0F }, 3) };

    MAREA_MathVector3Float32 product{ MAREA_garbage<MAREA_MathVector3Float32>() };

    MAREA_MathMultiplyMatrix3x3Vector3Float32(&left, &VECTOR, &product);

    MAREA_expectFloatBits(product, { 14.0F, 14.0F, 17.0F, 0.0F }, 3, "matrix by vector");

    MAREA_MathMultiplyMatrix3x3Float32(&left, &RIGHT, &left);

    MAREA_expectMatrixBits(left, MAREA_matrixColumns(3, { 6.0F, 5.0F, 11.0F, 1.0F, 0.0F, 5.0F, 6.0F, 8.0F, 0.0F }), "matrix by matrix");
}

TEST(MAREAMathMatrix3x3, IdentityTransposeAndMultiplySinteger32)
{
    GTEST_COUT << "Validating the integer identity, transpose and products." << std::endl;

    MAREA_MathMatrix3x3Sinteger32 identity{ MAREA_garbage<MAREA_MathMatrix3x3Sinteger32>() };

    MAREA_MathIdentityMatrix3x3Sinteger32(&identity);

    MAREA_expectIntegerMatrix(identity, MAREA_integerColumns(3, { 1, 0, 0, 0, 1, 0, 0, 0, 1 }), "identity");

    const MAREA_MathMatrix3x3Sinteger32 VALUE{ MAREA_integer3x3({ 1, 0, 5, 2, 1, 6, 3, 4, 0 }) };

    MAREA_MathMatrix3x3Sinteger32 result{ MAREA_garbage<MAREA_MathMatrix3x3Sinteger32>() };

    MAREA_MathMultiplyMatrix3x3Sinteger32(&identity, &VALUE, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(3, { 1, 0, 5, 2, 1, 6, 3, 4, 0 }), "identity by matrix");

    MAREA_MathTransposeMatrix3x3Sinteger32(&VALUE, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(3, { 1, 2, 3, 0, 1, 4, 5, 6, 0 }), "transpose");

    const MAREA_MathMatrix3x3Sinteger32 RIGHT{ MAREA_integer3x3({ 1, 1, 1, 1, 0, 0, 0, 0, 2 }) };

    MAREA_MathMultiplyMatrix3x3Sinteger32(&VALUE, &RIGHT, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(3, { 6, 5, 11, 1, 0, 5, 6, 8, 0 }), "matrix by matrix");

    const MAREA_MathVector3Sinteger32 VECTOR{ MAREA_vectorOf<MAREA_MathVector3Sinteger32>(MAREA_IntegerLanes{ 1, 2, 3, 0 }, 3) };

    MAREA_MathVector3Sinteger32 product{ MAREA_garbage<MAREA_MathVector3Sinteger32>() };

    MAREA_MathMultiplyMatrix3x3Vector3Sinteger32(&VALUE, &VECTOR, &product);

    MAREA_expectIntegers(product, { 14, 14, 17, 0 }, 3, "matrix by vector");
}

TEST(MAREAMathMatrix3x3, MultiplySinteger32Wraps)
{
    GTEST_COUT << "Validating the integer products and sums wrap." << std::endl;

    const MAREA_MathMatrix3x3Sinteger32 VALUE{ MAREA_integer3x3({ MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST, 0, MAREA_MATH_LARGEST, 0, 0, 0, 0, 1 }) };

    const MAREA_MathVector3Sinteger32 VECTOR{ MAREA_vectorOf<MAREA_MathVector3Sinteger32>(MAREA_IntegerLanes{ 1, 1, 0, 0 }, 3) };

    MAREA_MathVector3Sinteger32 product{ MAREA_garbage<MAREA_MathVector3Sinteger32>() };

    MAREA_MathMultiplyMatrix3x3Vector3Sinteger32(&VALUE, &VECTOR, &product);

    MAREA_expectIntegers(product, { -2, MAREA_MATH_SMALLEST, 0, 0 }, 3, "wrapping product");
}
