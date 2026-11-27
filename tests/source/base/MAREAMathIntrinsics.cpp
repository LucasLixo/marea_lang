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

// WARNING: these functions do not exist yet - this file is the red state of the intrinsics backend.
// The declarations let the test compile; the call fails to link until a lib/source/marea/base file defines
// each one, registered the same way the scalar function of the same operation is registered. Whether this
// backend actually builds on this machine (needs sse2 on x86_64 or neon on aarch64) is decided by part 3/4,
// not by this test file.
extern "C" {
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAddVector2Float32Intrinsics(const MAREA_MathVector2Float32 *left, const MAREA_MathVector2Float32 *right, MAREA_MathVector2Float32 *result);
    MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32Intrinsics(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right);
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathNormalizeVector4Float32Intrinsics(const MAREA_MathVector4Float32 *value, MAREA_MathVector4Float32 *result);
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32Intrinsics(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result);
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32Intrinsics(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result);
}

// INFO: a bit-for-bit comparison, never '==' on the float, so a sign of zero or a NaN payload the two
// backends must agree on is caught too (adapted from the reference suite's float edge-case checks).
static bool MAREA_bitsEqual(const void *left, const void *right, std::size_t size)
{
    return std::memcmp(left, right, size) == 0;
}

TEST(MAREAMathIntrinsics, AddVector2Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the intrinsics add of Vector2Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector2Float32 LEFT{ 1.5F, -2.25F, 0.0F, 0.0F };
    const MAREA_MathVector2Float32 RIGHT{ -3.75F, 4.0F, 0.0F, 0.0F };

    MAREA_MathVector2Float32 scalarResult{};
    MAREA_MathAddVector2Float32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathVector2Float32 intrinsicsResult{};
    MAREA_MathAddVector2Float32Intrinsics(&LEFT, &RIGHT, &intrinsicsResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &intrinsicsResult, sizeof(scalarResult))) << "The intrinsics sum must be bit-identical to the scalar one.";
}

TEST(MAREAMathIntrinsics, AddVector2Float32MatchesScalarOnSignedZeroAndNan)
{
    GTEST_COUT << "Validating the intrinsics backend keeps the scalar sign of zero and NaN payload, never '=='." << std::endl;

    const MAREA_Float32 NAN_VALUE = std::numeric_limits<MAREA_Float32>::quiet_NaN();

    const MAREA_MathVector2Float32 LEFT{ -0.0F, NAN_VALUE, 0.0F, 0.0F };
    const MAREA_MathVector2Float32 RIGHT{ 0.0F, 1.0F, 0.0F, 0.0F };

    MAREA_MathVector2Float32 scalarResult{};
    MAREA_MathAddVector2Float32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathVector2Float32 intrinsicsResult{};
    MAREA_MathAddVector2Float32Intrinsics(&LEFT, &RIGHT, &intrinsicsResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &intrinsicsResult, sizeof(scalarResult))) << "The intrinsics backend must reproduce the scalar NaN payload and signed zero exactly.";
}

TEST(MAREAMathIntrinsics, DotVector3Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the intrinsics dot product of Vector3Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector3Float32 LEFT{ 1.0F, 2.0F, 3.0F, 0.0F };
    const MAREA_MathVector3Float32 RIGHT{ -4.0F, 5.0F, -6.0F, 0.0F };

    const MAREA_Float32 SCALAR_RESULT = MAREA_MathDotVector3Float32(&LEFT, &RIGHT);
    const MAREA_Float32 INTRINSICS_RESULT = MAREA_MathDotVector3Float32Intrinsics(&LEFT, &RIGHT);

    EXPECT_TRUE(MAREA_bitsEqual(&SCALAR_RESULT, &INTRINSICS_RESULT, sizeof(SCALAR_RESULT))) << "The intrinsics dot product must be bit-identical to the scalar one.";
}

TEST(MAREAMathIntrinsics, NormalizeVector4Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the intrinsics normalize of Vector4Float32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathVector4Float32 VALUE{ 3.0F, 0.0F, 4.0F, 0.0F };

    MAREA_MathVector4Float32 scalarResult{};
    MAREA_MathNormalizeVector4Float32(&VALUE, &scalarResult);

    MAREA_MathVector4Float32 intrinsicsResult{};
    MAREA_MathNormalizeVector4Float32Intrinsics(&VALUE, &intrinsicsResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &intrinsicsResult, sizeof(scalarResult))) << "The intrinsics normalize must be bit-identical to the scalar one.";
}

TEST(MAREAMathIntrinsics, MultiplyMatrix4x4Float32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the intrinsics multiply of Matrix4x4Float32 matches the scalar result bit for bit." << std::endl;

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

    MAREA_MathMatrix4x4Float32 intrinsicsResult{};
    MAREA_MathMultiplyMatrix4x4Float32Intrinsics(&left, &right, &intrinsicsResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &intrinsicsResult, sizeof(scalarResult))) << "The intrinsics matrix multiply must be bit-identical to the scalar one.";
}

TEST(MAREAMathIntrinsics, MultiplyQuaternionFloat32MatchesScalarBitForBit)
{
    GTEST_COUT << "Validating the intrinsics multiply of QuaternionFloat32 matches the scalar result bit for bit." << std::endl;

    const MAREA_MathQuaternionFloat32 LEFT{ 0.5F, 0.5F, 0.5F, 0.5F };
    const MAREA_MathQuaternionFloat32 RIGHT{ 0.0F, 0.0F, 0.0F, 1.0F };

    MAREA_MathQuaternionFloat32 scalarResult{};
    MAREA_MathMultiplyQuaternionFloat32(&LEFT, &RIGHT, &scalarResult);

    MAREA_MathQuaternionFloat32 intrinsicsResult{};
    MAREA_MathMultiplyQuaternionFloat32Intrinsics(&LEFT, &RIGHT, &intrinsicsResult);

    EXPECT_TRUE(MAREA_bitsEqual(&scalarResult, &intrinsicsResult, sizeof(scalarResult))) << "The intrinsics quaternion multiply must be bit-identical to the scalar one.";
}
