// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathQuaternion.h"
#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"

#include <cstring>
#include <limits>

// WARNING: these functions do not exist yet - this file is the red state of the vector-extension backend.
// The declarations let the test compile; the call fails to link until a lib/source/marea/base file defines
// each one, registered the same way the scalar function of the same operation is registered.
extern "C" {
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAddVector2Float32VectorExtension(const MAREA_MathVector2Float32 *left, const MAREA_MathVector2Float32 *right, MAREA_MathVector2Float32 *result);
MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32VectorExtension(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathNormalizeVector4Float32VectorExtension(const MAREA_MathVector4Float32 *value, MAREA_MathVector4Float32 *result);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32VectorExtension(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32VectorExtension(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result);
}

// INFO: a bit-for-bit comparison, never '==' on the float, so a sign of zero or a NaN payload the two
// backends must agree on is caught too (adapted from the reference suite's float edge-case checks).
static bool MAREA_bitsEqual(const void *left, const void *right, std::size_t size)
{
    return std::memcmp(left, right, size) == 0;
}

TEST(MAREAMathVectorExtension, AddVector2Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the vector-extension add of Vector2Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector2Float32 LEFT{ 1.5F, -2.25F, 0.0F, 0.0F };
    const MAREA_MathVector2Float32 RIGHT{ -3.75F, 4.0F, 0.0F, 0.0F };

    MAREA_MathVector2Float32 scalarResult{};
    MAREA_MathAddVector2Float32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathVector2Float32 vectorExtensionResult{};
    MAREA_MathAddVector2Float32VectorExtension(&LEFT, &RIGHT, &vectorExtensionResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &vectorExtensionResult, sizeof(scalarResult))) << "The vector-extension sum must be bit-identical to the scalar one.";
}

TEST(MAREAMathVectorExtension, AddVector2Float32MatchesScalarOnSignedZeroAndNan)
{
    GTEST_COUT << "Validating the vector-extension backend keeps the scalar sign of zero and NaN payload, never '=='." << std::endl;

    const MAREA_Float32 NAN_VALUE = std::numeric_limits<MAREA_Float32>::quiet_NaN();

    const MAREA_MathVector2Float32 LEFT{ -0.0F, NAN_VALUE, 0.0F, 0.0F };
    const MAREA_MathVector2Float32 RIGHT{ 0.0F, 1.0F, 0.0F, 0.0F };

    MAREA_MathVector2Float32 scalarResult{};
    MAREA_MathAddVector2Float32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathVector2Float32 vectorExtensionResult{};
    MAREA_MathAddVector2Float32VectorExtension(&LEFT, &RIGHT, &vectorExtensionResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &vectorExtensionResult, sizeof(scalarResult))) << "The vector-extension backend must reproduce the scalar NaN payload and signed zero exactly.";
}

TEST(MAREAMathVectorExtension, DotVector3Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the vector-extension dot product of Vector3Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector3Float32 LEFT{ 1.0F, 2.0F, 3.0F, 0.0F };
    const MAREA_MathVector3Float32 RIGHT{ -4.0F, 5.0F, -6.0F, 0.0F };

    const MAREA_Float32 SCALAR_RESULT = MAREA_MathDotVector3Float32(&LEFT, &RIGHT);
    const MAREA_Float32 VECTOR_EXTENSION_RESULT = MAREA_MathDotVector3Float32VectorExtension(&LEFT, &RIGHT);

    EXPECT_TRUE(MAREA_bitsEqual(&SCALAR_RESULT, &VECTOR_EXTENSION_RESULT, sizeof(SCALAR_RESULT))) << "The vector-extension dot product must be bit-identical to the scalar one.";
}

TEST(MAREAMathVectorExtension, NormalizeVector4Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the vector-extension normalize of Vector4Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector4Float32 VALUE{ 3.0F, 0.0F, 4.0F, 0.0F };

    MAREA_MathVector4Float32 scalarResult{};
    MAREA_MathNormalizeVector4Float32(&VALUE, &scalarResult);

    MAREA_MathVector4Float32 vectorExtensionResult{};
    MAREA_MathNormalizeVector4Float32VectorExtension(&VALUE, &vectorExtensionResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &vectorExtensionResult, sizeof(scalarResult))) << "The vector-extension normalize must be bit-identical to the scalar one.";
}

TEST(MAREAMathVectorExtension, MultiplyMatrix4x4Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the vector-extension multiply of Matrix4x4Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathMatrix4x4Float32 left{
        MAREA_MathVector4Float32{ 1.0F, 2.0F, 3.0F, 4.0F },
        MAREA_MathVector4Float32{ 5.0F, 6.0F, 7.0F, 8.0F },
        MAREA_MathVector4Float32{ 9.0F, 10.0F, 11.0F, 12.0F },
        MAREA_MathVector4Float32{ 13.0F, 14.0F, 15.0F, 16.0F },
    };
    const MAREA_MathMatrix4x4Float32 right{
        MAREA_MathVector4Float32{ 16.0F, 15.0F, 14.0F, 13.0F },
        MAREA_MathVector4Float32{ 12.0F, 11.0F, 10.0F, 9.0F },
        MAREA_MathVector4Float32{ 8.0F, 7.0F, 6.0F, 5.0F },
        MAREA_MathVector4Float32{ 4.0F, 3.0F, 2.0F, 1.0F },
    };

    MAREA_MathMatrix4x4Float32 scalarResult{};
    MAREA_MathMultiplyMatrix4x4Float32(&left, &right, &scalarResult);

    MAREA_MathMatrix4x4Float32 vectorExtensionResult{};
    MAREA_MathMultiplyMatrix4x4Float32VectorExtension(&left, &right, &vectorExtensionResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &vectorExtensionResult, sizeof(scalarResult))) << "The vector-extension matrix multiply must be bit-identical to the scalar one.";
}

TEST(MAREAMathVectorExtension, MultiplyQuaternionFloat32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the vector-extension multiply of QuaternionFloat32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathQuaternionFloat32 LEFT{ 0.5F, 0.5F, 0.5F, 0.5F };
    const MAREA_MathQuaternionFloat32 RIGHT{ 0.0F, 0.0F, 0.0F, 1.0F };

    MAREA_MathQuaternionFloat32 scalarResult{};
    MAREA_MathMultiplyQuaternionFloat32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathQuaternionFloat32 vectorExtensionResult{};
    MAREA_MathMultiplyQuaternionFloat32VectorExtension(&LEFT, &RIGHT, &vectorExtensionResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &vectorExtensionResult, sizeof(scalarResult))) << "The vector-extension quaternion multiply must be bit-identical to the scalar one.";
}

TEST(MAREAMathVectorExtension, AutoBackendOnThisMachineMatchesScalarBitForBit)
{
    GTEST_COUT << "Validating that on this build machine (x86_64, sse2 available) the best backend auto would pick (vector_extension, since no neon/intrinsics applies) still matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector2Float32 LEFT{ 10.0F, -20.0F, 0.0F, 0.0F };
    const MAREA_MathVector2Float32 RIGHT{ 1.0F, 2.0F, 0.0F, 0.0F };

    MAREA_MathVector2Float32 scalarResult{};
    MAREA_MathAddVector2Float32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathVector2Float32 autoCandidateResult{};
    MAREA_MathAddVector2Float32VectorExtension(&LEFT, &RIGHT, &autoCandidateResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &autoCandidateResult, sizeof(scalarResult))) << "Whatever backend auto picks on this machine must agree with scalar bit for bit.";
}
