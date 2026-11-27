// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathQuaternion.h"

#include "tests/MAREAMathMatrixHelpers.hpp"

#include <cstddef>

namespace
{
    constexpr float MAREA_QUARTER_TURN{ 1.57079632679489661923F };

    constexpr float MAREA_HALF_ROOT_TWO{ 0.70710678118654752440F };

    constexpr MAREA_FloatLanes MAREA_QUATERNION_IDENTITY{ 0.0F, 0.0F, 0.0F, 1.0F };

    MAREA_MathQuaternionFloat32 MAREA_quaternion(const MAREA_FloatLanes &lanes)
    {
        return MAREA_vectorOf<MAREA_MathQuaternionFloat32>(lanes, 4);
    }

    MAREA_MathVector3Float32 MAREA_float3(float x, float y, float z)
    {
        return MAREA_vectorOf<MAREA_MathVector3Float32>(MAREA_FloatLanes{ x, y, z, 0.0F }, 3);
    }

    MAREA_FloatLanes MAREA_lanes(const MAREA_MathQuaternionFloat32 &value)
    {
        return MAREA_lanesOf<MAREA_FloatLanes>(value);
    }

    void MAREA_expectNear(const MAREA_MathQuaternionFloat32 &value, const MAREA_FloatLanes &expected, float tolerance, const char *label)
    {
        const MAREA_FloatLanes ACTUAL{ MAREA_lanes(value) };

        for (std::size_t lane{ 0 }; lane < 4; ++lane) {
            EXPECT_NEAR(ACTUAL[lane], expected[lane], tolerance) << label << " lane " << lane;
        }
    }

    MAREA_FloatLanes MAREA_referenceQuaternionNormalize(const MAREA_FloatLanes &value)
    {
        return (MAREA_referenceLength(value, 4) == 0.0F) ? MAREA_QUATERNION_IDENTITY : MAREA_referenceNormalize(value, 4);
    }

    MAREA_FloatLanes MAREA_referenceQuaternionMix(const MAREA_FloatLanes &left, const MAREA_FloatLanes &right, float amount)
    {
        return MAREA_referenceQuaternionNormalize(MAREA_referenceMix(left, right, amount, 4));
    }

    MAREA_FloatLanes MAREA_referenceSphericalMix(const MAREA_FloatLanes &left, const MAREA_FloatLanes &right, float amount)
    {
        float cosine{ MAREA_referenceDot(left, right, 4) };

        MAREA_FloatLanes end{ right };

        if (cosine < 0.0F) {
            end = { -right[0], -right[1], -right[2], -right[3] };

            cosine = -cosine;
        }

        if (cosine > 0.9995F) {
            return MAREA_referenceQuaternionMix(left, end, amount);
        }

        const float ANGLE{ std::acos(cosine) };

        const float SINE{ std::sin(ANGLE) };

        const float LEFT_WEIGHT{ std::sin((1.0F - amount) * ANGLE) / SINE };

        const float RIGHT_WEIGHT{ std::sin(amount * ANGLE) / SINE };

        MAREA_FloatLanes result{};

        for (std::size_t lane{ 0 }; lane < 4; ++lane) {
            result[lane] = MAREA_product(left[lane], LEFT_WEIGHT) + MAREA_product(end[lane], RIGHT_WEIGHT);
        }

        return result;
    }

    MAREA_FloatLanes MAREA_referenceQuaternionMultiply(const MAREA_FloatLanes &a, const MAREA_FloatLanes &b)
    {
        const auto P{ [&a, &b](std::size_t left, std::size_t right) { return MAREA_product(a[left], b[right]); } };

        return {
            (P(3, 0) + P(0, 3)) + (P(1, 2) - P(2, 1)),
            (P(3, 1) - P(0, 2)) + (P(1, 3) + P(2, 0)),
            (P(3, 2) + P(0, 1)) - (P(1, 0) - P(2, 3)),
            (P(3, 3) - P(0, 0)) - (P(1, 1) + P(2, 2))
        };
    }
}

TEST(MAREAMathQuaternion, Layout)
{
    GTEST_COUT << "Validating the size, the alignment and the lane places of the quaternion." << std::endl;

    EXPECT_EQ(sizeof(MAREA_MathQuaternionFloat32), 16U) << "QuaternionFloat32 is 16 bytes.";

    EXPECT_EQ(alignof(MAREA_MathQuaternionFloat32), 16U) << "QuaternionFloat32 is aligned to 16 bytes.";

    EXPECT_EQ(offsetof(MAREA_MathQuaternionFloat32, x), 0U) << "x is lane 0.";

    EXPECT_EQ(offsetof(MAREA_MathQuaternionFloat32, y), 4U) << "y is lane 1.";

    EXPECT_EQ(offsetof(MAREA_MathQuaternionFloat32, z), 8U) << "z is lane 2.";

    EXPECT_EQ(offsetof(MAREA_MathQuaternionFloat32, w), 12U) << "w is lane 3.";
}

TEST(MAREAMathQuaternion, Identity)
{
    GTEST_COUT << "Validating the identity quaternion is zero with w one." << std::endl;

    MAREA_MathQuaternionFloat32 result{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

    MAREA_MathIdentityQuaternionFloat32(&result);

    MAREA_expectFloatBits(result, MAREA_QUATERNION_IDENTITY, 4, "identity");
}

TEST(MAREAMathQuaternion, ConjugateChangesOnlyTheSignOfTheImaginaryPart)
{
    GTEST_COUT << "Validating conjugate flips the sign bit of x, y and z and copies w, in place." << std::endl;

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        MAREA_MathQuaternionFloat32 value{ MAREA_quaternion(pair.first) };

        MAREA_MathConjugateQuaternionFloat32(&value, &value);

        const MAREA_FloatLanes ACTUAL{ MAREA_lanes(value) };

        for (std::size_t lane{ 0 }; lane < 4; ++lane) {
            const std::uint32_t SIGN{ (lane < 3) ? 0x80000000U : 0U };

            ASSERT_EQ(MAREA_floatBits(ACTUAL[lane]), MAREA_floatBits(pair.first[lane]) ^ SIGN) << "conjugate lane " << lane;
        }
    }
}

TEST(MAREAMathQuaternion, Multiply)
{
    GTEST_COUT << "Validating the Hamilton product, its order and the scalar sum order." << std::endl;

    const MAREA_MathQuaternionFloat32 I{ MAREA_quaternion({ 1.0F, 0.0F, 0.0F, 0.0F }) };

    const MAREA_MathQuaternionFloat32 J{ MAREA_quaternion({ 0.0F, 1.0F, 0.0F, 0.0F }) };

    const MAREA_MathQuaternionFloat32 IDENTITY{ MAREA_quaternion(MAREA_QUATERNION_IDENTITY) };

    MAREA_MathQuaternionFloat32 result{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

    MAREA_MathMultiplyQuaternionFloat32(&I, &J, &result);

    MAREA_expectNear(result, { 0.0F, 0.0F, 1.0F, 0.0F }, 0.0F, "i by j");

    MAREA_MathMultiplyQuaternionFloat32(&J, &I, &result);

    MAREA_expectNear(result, { 0.0F, 0.0F, -1.0F, 0.0F }, 0.0F, "j by i");

    const MAREA_MathQuaternionFloat32 VALUE{ MAREA_quaternion({ 0.5F, 0.5F, 0.5F, 0.5F }) };

    MAREA_MathMultiplyQuaternionFloat32(&VALUE, &IDENTITY, &result);

    MAREA_expectNear(result, { 0.5F, 0.5F, 0.5F, 0.5F }, 0.0F, "by identity");

    std::size_t index{ 0 };

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        MAREA_MathQuaternionFloat32 left{ MAREA_quaternion(pair.first) };

        const MAREA_MathQuaternionFloat32 RIGHT{ MAREA_quaternion(pair.second) };

        MAREA_MathMultiplyQuaternionFloat32(&left, &RIGHT, &left);

        const MAREA_FloatLanes ACTUAL{ MAREA_lanes(left) };

        const MAREA_FloatLanes EXPECTED{ MAREA_referenceQuaternionMultiply(pair.first, pair.second) };

        for (std::size_t lane{ 0 }; lane < 4; ++lane) {
            ASSERT_TRUE(MAREA_sameFloat(ACTUAL[lane], EXPECTED[lane])) << "case " << index << " lane " << lane << ": " << ACTUAL[lane] << " against " << EXPECTED[lane];
        }

        ++index;
    }
}

TEST(MAREAMathQuaternion, DotAndLength)
{
    GTEST_COUT << "Validating the dot product and the length follow the scalar formula." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        const MAREA_MathQuaternionFloat32 LEFT{ MAREA_quaternion(pair.first) };

        const MAREA_MathQuaternionFloat32 RIGHT{ MAREA_quaternion(pair.second) };

        ASSERT_LE(MAREA_ulpDistance(MAREA_MathDotQuaternionFloat32(&LEFT, &RIGHT), MAREA_referenceDot(pair.first, pair.second, 4)), MAREA_MATH_MAXIMUM_ULP) << "dot case " << index;

        ASSERT_LE(MAREA_ulpDistance(MAREA_MathLengthQuaternionFloat32(&LEFT), MAREA_referenceLength(pair.first, 4)), MAREA_MATH_MAXIMUM_ULP) << "length case " << index;

        ++index;
    }
}

TEST(MAREAMathQuaternion, NormalizeGivesTheIdentityForAZeroLength)
{
    GTEST_COUT << "Validating normalize follows the scalar formula and a zero quaternion gives the identity." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        MAREA_MathQuaternionFloat32 value{ MAREA_quaternion(pair.first) };

        MAREA_MathNormalizeQuaternionFloat32(&value, &value);

        MAREA_expectFloats(value, MAREA_referenceQuaternionNormalize(pair.first), 4, "normalize", index);

        ++index;
    }

    MAREA_MathQuaternionFloat32 zero{ MAREA_quaternion({ 0.0F, 0.0F, 0.0F, 0.0F }) };

    MAREA_MathNormalizeQuaternionFloat32(&zero, &zero);

    MAREA_expectFloatBits(zero, MAREA_QUATERNION_IDENTITY, 4, "normalize of zero");
}

TEST(MAREAMathQuaternion, MixIsANormalizedLinearInterpolation)
{
    GTEST_COUT << "Validating mix is the normalized linear interpolation, inside, at the ends and outside." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        const MAREA_MathQuaternionFloat32 LEFT{ MAREA_quaternion(pair.first) };

        const MAREA_MathQuaternionFloat32 RIGHT{ MAREA_quaternion(pair.second) };

        MAREA_MathQuaternionFloat32 result{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

        MAREA_MathMixQuaternionFloat32(&LEFT, &RIGHT, MAREA_mixParameter(index), &result);

        MAREA_expectFloats(result, MAREA_referenceQuaternionMix(pair.first, pair.second, MAREA_mixParameter(index)), 4, "mix", index);

        ++index;
    }
}

TEST(MAREAMathQuaternion, SphericalMixTakesTheShortestArc)
{
    GTEST_COUT << "Validating the spherical interpolation follows the scalar formula." << std::endl;

    std::size_t index{ 0 };

    for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
        const MAREA_FloatLanes START{ MAREA_referenceQuaternionNormalize(pair.first) };

        const MAREA_FloatLanes END{ MAREA_referenceQuaternionNormalize(pair.second) };

        const MAREA_MathQuaternionFloat32 LEFT{ MAREA_quaternion(START) };

        const MAREA_MathQuaternionFloat32 RIGHT{ MAREA_quaternion(END) };

        MAREA_MathQuaternionFloat32 result{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

        MAREA_MathSphericalMixQuaternionFloat32(&LEFT, &RIGHT, MAREA_mixParameter(index), &result);

        MAREA_expectFloats(result, MAREA_referenceSphericalMix(START, END, MAREA_mixParameter(index)), 4, "spherical mix", index);

        ++index;
    }
}

TEST(MAREAMathQuaternion, SphericalMixAtTheEndsAndOnTheOtherHemisphere)
{
    GTEST_COUT << "Validating the ends, the middle, equal ends and a negated end of the spherical interpolation." << std::endl;

    const MAREA_FloatLanes TURN{ 0.0F, MAREA_HALF_ROOT_TWO, 0.0F, MAREA_HALF_ROOT_TWO };

    const MAREA_MathQuaternionFloat32 START{ MAREA_quaternion(MAREA_QUATERNION_IDENTITY) };

    const MAREA_MathQuaternionFloat32 END{ MAREA_quaternion(TURN) };

    const MAREA_MathQuaternionFloat32 NEGATED{ MAREA_quaternion({ -0.0F, -TURN[1], -0.0F, -TURN[3] }) };

    MAREA_MathQuaternionFloat32 result{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

    MAREA_MathSphericalMixQuaternionFloat32(&START, &END, 0.0F, &result);

    MAREA_expectNear(result, MAREA_QUATERNION_IDENTITY, 1.0e-5F, "start");

    MAREA_MathSphericalMixQuaternionFloat32(&START, &END, 1.0F, &result);

    MAREA_expectNear(result, TURN, 1.0e-5F, "end");

    MAREA_MathSphericalMixQuaternionFloat32(&START, &NEGATED, 0.5F, &result);

    MAREA_expectNear(result, { 0.0F, 0.38268343236508977F, 0.0F, 0.92387953251128676F }, 1.0e-5F, "middle through the negated end");

    MAREA_MathSphericalMixQuaternionFloat32(&START, &START, 0.5F, &result);

    MAREA_expectNear(result, MAREA_QUATERNION_IDENTITY, 1.0e-5F, "equal ends");
}

TEST(MAREAMathQuaternion, FromAxisAngleAndRotateVector3)
{
    GTEST_COUT << "Validating a quarter turn about z made from an axis that is not of length one." << std::endl;

    const MAREA_MathVector3Float32 AXIS{ MAREA_float3(0.0F, 0.0F, 3.0F) };

    MAREA_MathQuaternionFloat32 turn{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

    MAREA_MathFromAxisAngleQuaternionFloat32(&AXIS, MAREA_QUARTER_TURN, &turn);

    MAREA_expectNear(turn, { 0.0F, 0.0F, MAREA_HALF_ROOT_TWO, MAREA_HALF_ROOT_TWO }, 1.0e-6F, "axis and angle");

    MAREA_MathVector3Float32 value{ MAREA_float3(1.0F, 0.0F, 0.0F) };

    MAREA_MathRotateVector3QuaternionFloat32(&turn, &value, &value);

    EXPECT_NEAR(value.x, 0.0F, 1.0e-5F) << "rotated x.";

    EXPECT_NEAR(value.y, 1.0F, 1.0e-5F) << "rotated y.";

    EXPECT_NEAR(value.z, 0.0F, 1.0e-5F) << "rotated z.";

    EXPECT_EQ(MAREA_floatBits(MAREA_lanesOf<MAREA_FloatLanes>(value)[3]), 0U) << "The padding lane must be zero.";
}

TEST(MAREAMathQuaternion, FromEulerTurnsYawAboutZPitchAboutYAndRollAboutX)
{
    GTEST_COUT << "Validating each Euler angle alone." << std::endl;

    MAREA_MathQuaternionFloat32 result{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

    MAREA_MathFromEulerQuaternionFloat32(MAREA_QUARTER_TURN, 0.0F, 0.0F, &result);

    MAREA_expectNear(result, { 0.0F, 0.0F, MAREA_HALF_ROOT_TWO, MAREA_HALF_ROOT_TWO }, 1.0e-6F, "yaw");

    MAREA_MathFromEulerQuaternionFloat32(0.0F, MAREA_QUARTER_TURN, 0.0F, &result);

    MAREA_expectNear(result, { 0.0F, MAREA_HALF_ROOT_TWO, 0.0F, MAREA_HALF_ROOT_TWO }, 1.0e-6F, "pitch");

    MAREA_MathFromEulerQuaternionFloat32(0.0F, 0.0F, MAREA_QUARTER_TURN, &result);

    MAREA_expectNear(result, { MAREA_HALF_ROOT_TWO, 0.0F, 0.0F, MAREA_HALF_ROOT_TWO }, 1.0e-6F, "roll");

    MAREA_MathFromEulerQuaternionFloat32(MAREA_QUARTER_TURN, 0.0F, MAREA_QUARTER_TURN, &result);

    MAREA_expectNear(result, { 0.5F, 0.5F, 0.5F, 0.5F }, 1.0e-6F, "yaw and roll");
}

TEST(MAREAMathQuaternion, ToMatrix4x4IsTheSameTurnAsRotate)
{
    GTEST_COUT << "Validating the matrix of a quaternion matches the rotation matrix of the same axis and angle." << std::endl;

    const MAREA_MathVector3Float32 AXIS{ MAREA_float3(1.0F, 2.0F, 3.0F) };

    const MAREA_MathMatrix4x4Float32 IDENTITY{ MAREA_matrixOf<MAREA_MathMatrix4x4Float32>(MAREA_MATRIX_IDENTITY) };

    MAREA_MathQuaternionFloat32 turn{ MAREA_garbage<MAREA_MathQuaternionFloat32>() };

    MAREA_MathMatrix4x4Float32 fromQuaternion{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathMatrix4x4Float32 rotation{ MAREA_garbage<MAREA_MathMatrix4x4Float32>() };

    MAREA_MathFromAxisAngleQuaternionFloat32(&AXIS, 1.0F, &turn);

    MAREA_MathToMatrix4x4QuaternionFloat32(&turn, &fromQuaternion);

    MAREA_MathRotateMatrix4x4Float32(&IDENTITY, 1.0F, &AXIS, &rotation);

    MAREA_expectMatrixNear(fromQuaternion, MAREA_matrixLanesOf<MAREA_MatrixLanes>(rotation), 1.0e-5F, "matrix of the quaternion");
}
