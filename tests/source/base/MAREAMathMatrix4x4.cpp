// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathMatrix4x4.h"

#include "tests/MAREAMathMatrixHelpers.hpp"

#include <cstddef>

namespace
{
    constexpr float MAREA_QUARTER_TURN{ 1.57079632679489661923F };

    MAREA_MathMatrix4x4Float32 MAREA_float4x4(const MAREA_MatrixLanes &lanes)
    {
        return MAREA_matrixOf<MAREA_MathMatrix4x4Float32>(lanes);
    }

    MAREA_MathMatrix4x4Sinteger32 MAREA_integer4x4(std::initializer_list<std::int32_t> values)
    {
        return MAREA_matrixOf<MAREA_MathMatrix4x4Sinteger32>(MAREA_integerColumns(4, values));
    }

    MAREA_MathVector3Float32 MAREA_float3(float x, float y, float z)
    {
        return MAREA_vectorOf<MAREA_MathVector3Float32>(MAREA_FloatLanes{ x, y, z, 0.0F }, 3);
    }

    MAREA_MathVector4Float32 MAREA_float4(const MAREA_FloatLanes &lanes)
    {
        return MAREA_vectorOf<MAREA_MathVector4Float32>(lanes, 4);
    }

    MAREA_FloatLanes MAREA_transform(const MAREA_MathMatrix4x4Float32 &matrix, const MAREA_FloatLanes &point)
    {
        const MAREA_MathVector4Float32 VECTOR{ MAREA_float4(point) };

        MAREA_MathVector4Float32 result{ MAREA_garbage<MAREA_MathVector4Float32>() };

        MAREA_MathMultiplyMatrix4x4Vector4Float32(&matrix, &VECTOR, &result);

        return MAREA_lanesOf<MAREA_FloatLanes>(result);
    }

    void MAREA_expectLanesNear(const MAREA_FloatLanes &actual, const MAREA_FloatLanes &expected, float tolerance, const char *label)
    {
        for (std::size_t lane{ 0 }; lane < 4; ++lane) {
            EXPECT_NEAR(actual[lane], expected[lane], tolerance) << label << " lane " << lane;
        }
    }
}

TEST(MAREAMathMatrix4x4, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the column places of the matrices of four by four." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathMatrix4x4Float32), 64U) << "Matrix4x4Float32 is 64 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix4x4Float32), 16U) << "Matrix4x4Float32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathMatrix4x4Sinteger32), 64U) << "Matrix4x4Sinteger32 is 64 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix4x4Sinteger32), 16U) << "Matrix4x4Sinteger32 is aligned to 16 bytes.";

    EXPECT_EQ(sizeof(MAREA_MathMatrix4x4Boolean), 16U) << "Matrix4x4Boolean is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathMatrix4x4Boolean), 4U) << "Matrix4x4Boolean is aligned to 4 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix4x4Float32, column1), 16U) << "column1 of Matrix4x4Float32 is at 16.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix4x4Float32, column2), 32U) << "column2 of Matrix4x4Float32 is at 32.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix4x4Float32, column3), 48U) << "column3 of Matrix4x4Float32 is at 48.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix4x4Sinteger32, column3), 48U) << "column3 of Matrix4x4Sinteger32 is at 48.";

    EXPECT_EQ(offsetof(MAREA_MathMatrix4x4Boolean, column3), 12U) << "column3 of Matrix4x4Boolean is at 12.";
}

TEST(MAREAMathMatrix4x4, IdentityFloat32)
{
    GTEST_COUT << "Validating the identity has one on the diagonal and zero elsewhere." << std::endl;

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathIdentityMatrix4x4Float32(&result);

    MAREA_expectMatrixBits(result, MAREA_MATRIX_IDENTITY, "identity");
}

TEST(MAREAMathMatrix4x4, MultiplyVectorHasTheBitsOfTheOrderedSum)
{
    GTEST_COUT << "Validating the product with a vector sums the columns from left to right." << std::endl;

    const std::vector<MAREA_FloatLanes> &vectors{ MAREA_matrixVectorCases() };

    std::size_t index{ 0 };

    for (const MAREA_MatrixLanes &matrix : MAREA_matrixCases()) {
        const MAREA_FloatLanes &VECTOR{ vectors[index % vectors.size()] };

        const MAREA_FloatLanes ACTUAL{ MAREA_transform(MAREA_float4x4(matrix), VECTOR) };

        const MAREA_FloatLanes EXPECTED{ MAREA_referenceMultiplyVector(matrix, VECTOR) };

        for (std::size_t lane{ 0 }; lane < 4; ++lane) {
            ASSERT_TRUE(MAREA_sameFloat(ACTUAL[lane], EXPECTED[lane])) << "case " << index << " lane " << lane << ": " << ACTUAL[lane] << " against " << EXPECTED[lane];
        }

        ++index;
    }
}

TEST(MAREAMathMatrix4x4, MultiplyHasTheBitsOfTheOrderedSum)
{
    GTEST_COUT << "Validating the matrix product sends each column of the right side through the left one." << std::endl;

    const std::vector<MAREA_MatrixLanes> &matrices{ MAREA_matrixCases() };

    for (std::size_t index{ 0 }; index < matrices.size(); ++index) {
        const MAREA_MatrixLanes &SECOND{ matrices[(index + 7) % matrices.size()] };

        const MAREA_MathMatrix4x4Float32 RIGHT{ MAREA_float4x4(SECOND) };

        MAREA_MathMatrix4x4Float32 left{ MAREA_float4x4(matrices[index]) };

        MAREA_MathMultiplyMatrix4x4Float32(&left, &RIGHT, &left);

        MAREA_expectMatrixBits(left, MAREA_referenceMultiply(matrices[index], SECOND), "multiply", index);
    }
}

TEST(MAREAMathMatrix4x4, TransposeMovesEveryLaneBitForBit)
{
    GTEST_COUT << "Validating the transpose moves the sixteen lanes, also in place." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_MatrixLanes &matrix : MAREA_matrixCases()) {
        MAREA_MathMatrix4x4Float32 value{ MAREA_float4x4(matrix) };

        MAREA_MathTransposeMatrix4x4Float32(&value, &value);

        MAREA_expectMatrixBits(value, MAREA_referenceTranspose(matrix), "transpose", index);

        ++index;
    }
}

TEST(MAREAMathMatrix4x4, DeterminantHasTheBitsOfTheOrderedExpansion)
{
    GTEST_COUT << "Validating the determinant follows the scalar expansion." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_MatrixLanes &matrix : MAREA_matrixCases()) {
        const MAREA_MathMatrix4x4Float32 VALUE{ MAREA_float4x4(matrix) };

        const float ACTUAL{ MAREA_MathDeterminantMatrix4x4Float32(&VALUE) };

        const float EXPECTED{ MAREA_referenceDeterminant(matrix) };

        ASSERT_TRUE(MAREA_sameFloat(ACTUAL, EXPECTED)) << "case " << index << ": " << ACTUAL << " against " << EXPECTED;

        ++index;
    }
}

TEST(MAREAMathMatrix4x4, InverseHasTheBitsOfTheAdjugateOverTheDeterminant)
{
    GTEST_COUT << "Validating the inverse follows the scalar adjugate, also in place." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_MatrixLanes &matrix : MAREA_matrixCases()) {
        MAREA_MathMatrix4x4Float32 value{ MAREA_float4x4(matrix) };

        MAREA_MathInverseMatrix4x4Float32(&value, &value);

        MAREA_expectMatrixBits(value, MAREA_referenceInverse(matrix), "inverse", index);

        ++index;
    }
}

TEST(MAREAMathMatrix4x4, InverseOfASingularMatrixIsZero)
{
    GTEST_COUT << "Validating a zero matrix and a repeated column give the zero matrix." << std::endl;

    MAREA_MatrixLanes repeated{};

    for (std::size_t lane{ 0 }; lane < 16; ++lane) {
        repeated[lane] = 1.0F + static_cast<float>(lane % 4);
    }

    for (const MAREA_MatrixLanes &matrix : { MAREA_MatrixLanes{}, repeated }) {
        const MAREA_MathMatrix4x4Float32 VALUE{ MAREA_float4x4(matrix) };

        MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

        MAREA_MathInverseMatrix4x4Float32(&VALUE, &result);

        MAREA_expectMatrixBits(result, MAREA_MatrixLanes{}, "singular inverse");
    }
}

TEST(MAREAMathMatrix4x4, InverseOfANanMatrixKeepsTheNan)
{
    GTEST_COUT << "Validating a NaN lane is not read as a singular matrix." << std::endl;

    MAREA_MatrixLanes matrix{ MAREA_MATRIX_IDENTITY };

    matrix[5] = MAREA_MATH_NAN;

    MAREA_MathMatrix4x4Float32 value{ MAREA_float4x4(matrix) };

    MAREA_MathInverseMatrix4x4Float32(&value, &value);

    const MAREA_MatrixLanes ACTUAL{ MAREA_matrixLanesOf<MAREA_MatrixLanes>(value) };

    EXPECT_TRUE(std::any_of(ACTUAL.begin(), ACTUAL.end(), [](float lane) { return std::isnan(lane); })) << "The NaN must reach the result.";
}

TEST(MAREAMathMatrix4x4, IdentityIsNeutralAndItsOwnInverse)
{
    GTEST_COUT << "Validating the identity changes no matrix, is its own inverse and has determinant one." << std::endl;

    const MAREA_MathMatrix4x4Float32 IDENTITY{ MAREA_float4x4(MAREA_MATRIX_IDENTITY) };

    const MAREA_MatrixLanes &LAST{ MAREA_matrixCases().back() };

    const MAREA_MathMatrix4x4Float32 VALUE{ MAREA_float4x4(LAST) };

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathMultiplyMatrix4x4Float32(&IDENTITY, &VALUE, &result);

    MAREA_expectMatrixBits(result, LAST, "identity by matrix");

    MAREA_MathInverseMatrix4x4Float32(&IDENTITY, &result);

    MAREA_expectMatrixNear(result, MAREA_MATRIX_IDENTITY, 0.0F, "inverse of identity");

    EXPECT_EQ(MAREA_MathDeterminantMatrix4x4Float32(&IDENTITY), 1.0F) << "The determinant of the identity is one.";
}

TEST(MAREAMathMatrix4x4, IdentityTransposeAndMultiplySinteger32)
{
    GTEST_COUT << "Validating the integer identity, transpose and products." << std::endl;

    MAREA_MathMatrix4x4Sinteger32 identity{ MAREA_garbage<MAREA_MathMatrix4x4Sinteger32>() };

    MAREA_MathIdentityMatrix4x4Sinteger32(&identity);

    MAREA_expectIntegerMatrix(identity, MAREA_integerColumns(4, { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }), "identity");

    const MAREA_MathMatrix4x4Sinteger32 VALUE{ MAREA_integer4x4({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }) };

    MAREA_MathMatrix4x4Sinteger32 result{ MAREA_garbage<MAREA_MathMatrix4x4Sinteger32>() };

    MAREA_MathTransposeMatrix4x4Sinteger32(&VALUE, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(4, { 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16 }), "transpose");

    const MAREA_MathMatrix4x4Sinteger32 RIGHT{ MAREA_integer4x4({ 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0 }) };

    MAREA_MathMultiplyMatrix4x4Sinteger32(&VALUE, &RIGHT, &result);

    MAREA_expectIntegerMatrix(result, MAREA_integerColumns(4, { 28, 32, 36, 40, 1, 2, 3, 4, 5, 6, 7, 8, 2, 4, 6, 8 }), "matrix by matrix");

    const MAREA_MathVector4Sinteger32 VECTOR{ MAREA_vectorOf<MAREA_MathVector4Sinteger32>(MAREA_IntegerLanes{ 1, 1, 1, 1 }, 4) };

    MAREA_MathVector4Sinteger32 product{ MAREA_garbage<MAREA_MathVector4Sinteger32>() };

    MAREA_MathMultiplyMatrix4x4Vector4Sinteger32(&VALUE, &VECTOR, &product);

    MAREA_expectIntegers(product, { 28, 32, 36, 40 }, 4, "matrix by vector");
}

TEST(MAREAMathMatrix4x4, MultiplySinteger32Wraps)
{
    GTEST_COUT << "Validating the integer products and sums wrap." << std::endl;

    const MAREA_MathMatrix4x4Sinteger32 VALUE{ MAREA_integer4x4({ MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST, 0, 0, MAREA_MATH_LARGEST, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }) };

    const MAREA_MathVector4Sinteger32 VECTOR{ MAREA_vectorOf<MAREA_MathVector4Sinteger32>(MAREA_IntegerLanes{ 1, 1, 0, 0 }, 4) };

    MAREA_MathVector4Sinteger32 product{ MAREA_garbage<MAREA_MathVector4Sinteger32>() };

    MAREA_MathMultiplyMatrix4x4Vector4Sinteger32(&VALUE, &VECTOR, &product);

    MAREA_expectIntegers(product, { -2, MAREA_MATH_SMALLEST, 0, 0 }, 4, "wrapping product");
}

TEST(MAREAMathMatrix4x4, TranslateAndScaleApplyOnTheRight)
{
    GTEST_COUT << "Validating translate and scale multiply the matrix by the transform on the right, also in place." << std::endl;

    const MAREA_MathVector3Float32 OFFSET{ MAREA_float3(3.0F, 4.0F, 5.0F) };

    const MAREA_MathVector3Float32 FACTOR{ MAREA_float3(2.0F, 3.0F, 4.0F) };

    MAREA_MathMatrix4x4Float32 value{ MAREA_float4x4(MAREA_MATRIX_IDENTITY) };

    MAREA_MathTranslateMatrix4x4Float32(&value, &OFFSET, &value);

    MAREA_expectMatrixNear(value, MAREA_matrixColumns(4, { 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 3.0F, 4.0F, 5.0F, 1.0F }), 0.0F, "translate");

    MAREA_MathScaleMatrix4x4Float32(&value, &FACTOR, &value);

    MAREA_expectMatrixNear(value, MAREA_matrixColumns(4, { 2.0F, 0.0F, 0.0F, 0.0F, 0.0F, 3.0F, 0.0F, 0.0F, 0.0F, 0.0F, 4.0F, 0.0F, 3.0F, 4.0F, 5.0F, 1.0F }), 0.0F, "scale after translate");

    MAREA_MathTranslateMatrix4x4Float32(&value, &OFFSET, &value);

    MAREA_expectLanesNear(MAREA_transform(value, { 0.0F, 0.0F, 0.0F, 1.0F }), { 9.0F, 16.0F, 25.0F, 1.0F }, 0.0F, "translate after scale");
}

TEST(MAREAMathMatrix4x4, RotateTurnsAboutTheNormalizedAxis)
{
    GTEST_COUT << "Validating a quarter turn about z, with an axis that is not of length one." << std::endl;

    const MAREA_MathMatrix4x4Float32 IDENTITY{ MAREA_float4x4(MAREA_MATRIX_IDENTITY) };

    const MAREA_MathVector3Float32 AXIS{ MAREA_float3(0.0F, 0.0F, 5.0F) };

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathRotateMatrix4x4Float32(&IDENTITY, MAREA_QUARTER_TURN, &AXIS, &result);

    MAREA_expectLanesNear(MAREA_transform(result, { 1.0F, 0.0F, 0.0F, 1.0F }), { 0.0F, 1.0F, 0.0F, 1.0F }, 1.0e-5F, "x turns to y");

    MAREA_expectLanesNear(MAREA_transform(result, { 0.0F, 1.0F, 0.0F, 1.0F }), { -1.0F, 0.0F, 0.0F, 1.0F }, 1.0e-5F, "y turns to minus x");
}

TEST(MAREAMathMatrix4x4, LookAtMovesTheEyeToTheOrigin)
{
    GTEST_COUT << "Validating the view matrix of an eye on the z axis, and the degenerate views." << std::endl;

    const MAREA_MathVector3Float32 EYE{ MAREA_float3(0.0F, 0.0F, 5.0F) };

    const MAREA_MathVector3Float32 CENTER{ MAREA_float3(0.0F, 0.0F, 0.0F) };

    const MAREA_MathVector3Float32 UP{ MAREA_float3(0.0F, 1.0F, 0.0F) };

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathLookAtMatrix4x4Float32(&EYE, &CENTER, &UP, &result);

    MAREA_expectMatrixNear(result, MAREA_matrixColumns(4, { 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, -5.0F, 1.0F }), 1.0e-6F, "view");

    MAREA_expectLanesNear(MAREA_transform(result, { 0.0F, 0.0F, 5.0F, 1.0F }), { 0.0F, 0.0F, 0.0F, 1.0F }, 1.0e-5F, "eye");

    MAREA_MathLookAtMatrix4x4Float32(&EYE, &EYE, &UP, &result);

    MAREA_expectMatrixNear(result, MAREA_matrixColumns(4, { 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F }), 0.0F, "eye on the center");
}

TEST(MAREAMathMatrix4x4, PerspectiveMapsTheDepthFromZeroToOne)
{
    GTEST_COUT << "Validating the perspective matrix of a known view volume." << std::endl;

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathPerspectiveMatrix4x4Float32(MAREA_QUARTER_TURN, 2.0F, 1.0F, 3.0F, &result);

    MAREA_expectMatrixNear(result, MAREA_matrixColumns(4, { 0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, -1.5F, -1.0F, 0.0F, 0.0F, -1.5F, 0.0F }), 1.0e-6F, "perspective");

    const MAREA_FloatLanes NEAR_POINT{ MAREA_transform(result, { 0.0F, 0.0F, -1.0F, 1.0F }) };

    const MAREA_FloatLanes FAR_POINT{ MAREA_transform(result, { 0.0F, 0.0F, -3.0F, 1.0F }) };

    EXPECT_NEAR(NEAR_POINT[2] / NEAR_POINT[3], 0.0F, 1.0e-5F) << "The near plane has depth zero.";

    EXPECT_NEAR(FAR_POINT[2] / FAR_POINT[3], 1.0F, 1.0e-5F) << "The far plane has depth one.";

    MAREA_MathPerspectiveMatrix4x4Float32(MAREA_QUARTER_TURN, 2.0F, 5.0F, 5.0F, &result);

    const MAREA_MatrixLanes FLAT{ MAREA_matrixLanesOf<MAREA_MatrixLanes>(result) };

    EXPECT_EQ(FLAT[10], MAREA_MATH_INFINITY) << "Equal planes divide by zero and give infinity.";

    EXPECT_EQ(FLAT[14], -MAREA_MATH_INFINITY) << "Equal planes divide by zero and give minus infinity.";
}

TEST(MAREAMathMatrix4x4, OrthographicMapsTheBoxToTheUnitVolume)
{
    GTEST_COUT << "Validating the orthographic matrix of a known box." << std::endl;

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathOrthographicMatrix4x4Float32(0.0F, 4.0F, 0.0F, 2.0F, 1.0F, 3.0F, &result);

    MAREA_expectMatrixNear(result, MAREA_matrixColumns(4, { 0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, -0.5F, 0.0F, -1.0F, -1.0F, -0.5F, 1.0F }), 0.0F, "orthographic");

    MAREA_MathOrthographicMatrix4x4Float32(-1.0F, 1.0F, -1.0F, 1.0F, 0.0F, 2.0F, &result);

    MAREA_expectLanesNear(MAREA_transform(result, { 0.0F, 0.0F, -1.0F, 1.0F }), { 0.0F, 0.0F, 0.5F, 1.0F }, 1.0e-5F, "center of the box");

    MAREA_MathOrthographicMatrix4x4Float32(-2.0F, 3.0F, -1.5F, 2.5F, 4.0F, 4.0F, &result);

    EXPECT_EQ(MAREA_matrixLanesOf<MAREA_MatrixLanes>(result)[10], -MAREA_MATH_INFINITY) << "Equal planes divide by zero and give minus infinity.";
}

TEST(MAREAMathMatrix4x4, FrustumOfAnOffCenterVolume)
{
    GTEST_COUT << "Validating the frustum matrix of a known volume that is not centered." << std::endl;

    MAREA_MathMatrix4x4Float32 result{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathFrustumMatrix4x4Float32(-1.0F, 3.0F, -1.0F, 1.0F, 1.0F, 3.0F, &result);

    MAREA_expectMatrixNear(result, MAREA_matrixColumns(4, { 0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.5F, 0.0F, -1.5F, -1.0F, 0.0F, 0.0F, -1.5F, 0.0F }), 0.0F, "frustum");
}
