// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAHostFunction.h"
#include "marea/base/MAREAMacros.h"
#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathQuaternion.h"
#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"

#include <math.h>
#include <string.h>

// INFO: each product is rounded by itself, so a result does not depend on whether the processor has a fused multiply and add.
#if defined(__clang__)
    #pragma STDC FP_CONTRACT OFF
#endif

// INFO: the real-ISA selectors of this build (decision D8, file 16 of the plan): sse2 on x86_64, neon on aarch64,
// each only with gcc or clang, the same compilers the registered intrinsic headers support.
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(_M_X64))
    #define MAREA_MATH_INTRINSICS_X86_SSE2 1
#else
    #define MAREA_MATH_INTRINSICS_X86_SSE2 0
#endif

#if (defined(__GNUC__) || defined(__clang__)) && defined(__aarch64__)
    #define MAREA_MATH_INTRINSICS_AARCH64_NEON 1
#else
    #define MAREA_MATH_INTRINSICS_AARCH64_NEON 0
#endif

// INFO: a target with neither selector compiles none of this file, so the facade can never call an intrinsics
// function that was not built: cgen and the registry only ever see the functions this guard admits.
#if MAREA_MATH_INTRINSICS_X86_SSE2 || MAREA_MATH_INTRINSICS_AARCH64_NEON

    #if MAREA_MATH_INTRINSICS_X86_SSE2
        #include <immintrin.h>
    #elif MAREA_MATH_INTRINSICS_AARCH64_NEON
        #include <arm_neon.h>
    #endif

MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAddVector2Float32Intrinsics(const MAREA_MathVector2Float32 *left, const MAREA_MathVector2Float32 *right, MAREA_MathVector2Float32 *result);
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32Intrinsics(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathNormalizeVector4Float32Intrinsics(const MAREA_MathVector4Float32 *value, MAREA_MathVector4Float32 *result);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32Intrinsics(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32Intrinsics(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result);

    #if MAREA_MATH_INTRINSICS_X86_SSE2
typedef __m128 MAREA_G_Register4;
    #else
typedef float32x4_t MAREA_G_Register4;
    #endif

MAREA_STATIC_ASSERT(sizeof(MAREA_G_Register4) == sizeof(MAREA_MathVector4Float32), "The compiler register of four float32 lanes is the size of a vector of four float32:");

// INFO: one load, store, add, multiply and splat of four float32 lanes, named the same on both ISA so every
// operation below reads the same on x86_64 and on aarch64.
MAREA_FORCE_INLINE MAREA_G_Register4 MAREA_CALL MAREA_G_Load4(const void *source)
{
    #if MAREA_MATH_INTRINSICS_X86_SSE2
    return _mm_load_ps((const float *)source);
    #else
    return vld1q_f32((const float *)source);
    #endif
}

MAREA_FORCE_INLINE void MAREA_CALL MAREA_G_Store4(void *destination, MAREA_G_Register4 value)
{
    #if MAREA_MATH_INTRINSICS_X86_SSE2
    _mm_store_ps((float *)destination, value);
    #else
    vst1q_f32((float *)destination, value);
    #endif
}

MAREA_FORCE_INLINE MAREA_G_Register4 MAREA_CALL MAREA_G_Add4(MAREA_G_Register4 left, MAREA_G_Register4 right)
{
    #if MAREA_MATH_INTRINSICS_X86_SSE2
    return _mm_add_ps(left, right);
    #else
    return vaddq_f32(left, right);
    #endif
}

MAREA_FORCE_INLINE MAREA_G_Register4 MAREA_CALL MAREA_G_Multiply4(MAREA_G_Register4 left, MAREA_G_Register4 right)
{
    #if MAREA_MATH_INTRINSICS_X86_SSE2
    return _mm_mul_ps(left, right);
    #else
    return vmulq_f32(left, right);
    #endif
}

MAREA_FORCE_INLINE MAREA_G_Register4 MAREA_CALL MAREA_G_Splat4(MAREA_Float32 value)
{
    #if MAREA_MATH_INTRINSICS_X86_SSE2
    return _mm_set1_ps(value);
    #else
    return vdupq_n_f32(value);
    #endif
}

// INFO: one lane of a register, by a plain store to four lanes of memory: the same bits as a direct lane read,
// with no ISA-specific extract instruction to get wrong.
MAREA_FORCE_INLINE MAREA_Float32 MAREA_CALL MAREA_G_Lane4(MAREA_G_Register4 value, MAREA_IntegerSize index)
{
    MAREA_Float32 lanes[4];

    MAREA_G_Store4(lanes, value);

    return lanes[index];
}

// INFO: the four lanes of value, reordered by the given indices, by the same store-then-read as MAREA_G_Lane4:
// used only for the quaternion multiply, where each operand of a lane product reads a different input lane.
MAREA_FORCE_INLINE MAREA_G_Register4 MAREA_CALL MAREA_G_Shuffle4(MAREA_G_Register4 value, MAREA_IntegerSize i0, MAREA_IntegerSize i1, MAREA_IntegerSize i2, MAREA_IntegerSize i3)
{
    MAREA_Float32 lanes[4];

    MAREA_G_Store4(lanes, value);

    MAREA_Float32 shuffled[4] = { lanes[i0], lanes[i1], lanes[i2], lanes[i3] };

    return MAREA_G_Load4(shuffled);
}

// INFO: the sum of two lanes past the real width of Vector2Float32 (its padding) is never written to the result;
// the scalar and the vector-extension backend always give zero there too, whatever the two inputs carry in that
// padding (the padding of a valid instance is zero, so this never changes the sum itself).
void MAREA_CALL MAREA_MathAddVector2Float32Intrinsics(const MAREA_MathVector2Float32 *left, const MAREA_MathVector2Float32 *right, MAREA_MathVector2Float32 *result)
{
    const MAREA_G_Register4 LEFT = MAREA_G_Load4(left);
    const MAREA_G_Register4 RIGHT = MAREA_G_Load4(right);
    const MAREA_G_Register4 SUM = MAREA_G_Add4(LEFT, RIGHT);

    result->x = MAREA_G_Lane4(SUM, 0);
    result->y = MAREA_G_Lane4(SUM, 1);
    result->padding[0] = MAREA_FLOAT_32(0.0);
    result->padding[1] = MAREA_FLOAT_32(0.0);
}

// INFO: the fourth lane of the product (the padding of Vector3Float32) is never added to the sum, the same order
// the scalar and the vector-extension backend sum in: (p0 + p1) + p2, never (... + p3).
MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32Intrinsics(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right)
{
    const MAREA_G_Register4 LEFT = MAREA_G_Load4(left);
    const MAREA_G_Register4 RIGHT = MAREA_G_Load4(right);
    const MAREA_G_Register4 PRODUCT = MAREA_G_Multiply4(LEFT, RIGHT);

    const MAREA_Float32 SUM01 = (MAREA_G_Lane4(PRODUCT, 0) + MAREA_G_Lane4(PRODUCT, 1));

    return (SUM01 + MAREA_G_Lane4(PRODUCT, 2));
}

void MAREA_CALL MAREA_MathNormalizeVector4Float32Intrinsics(const MAREA_MathVector4Float32 *value, MAREA_MathVector4Float32 *result)
{
    const MAREA_G_Register4 VALUE = MAREA_G_Load4(value);
    const MAREA_G_Register4 SQUARE = MAREA_G_Multiply4(VALUE, VALUE);

    const MAREA_Float32 SUM01 = (MAREA_G_Lane4(SQUARE, 0) + MAREA_G_Lane4(SQUARE, 1));
    const MAREA_Float32 SUM012 = (SUM01 + MAREA_G_Lane4(SQUARE, 2));
    const MAREA_Float32 LENGTH = sqrtf(SUM012 + MAREA_G_Lane4(SQUARE, 3));

    if (LENGTH == MAREA_FLOAT_32(0.0)) {
        memset(result, 0, sizeof(*result));

        return;
    }

    const MAREA_Float32 INVERSE = (MAREA_FLOAT_32(1.0) / LENGTH);
    const MAREA_G_Register4 SCALED = MAREA_G_Multiply4(VALUE, MAREA_G_Splat4(INVERSE));

    MAREA_G_Store4(result, SCALED);
}

// INFO: one column of the product, the same four lane products and the same left-to-right sum order as the
// vector-extension and the scalar body: ((p0 + p1) + p2) + p3.
static MAREA_G_Register4 MAREA_CALL MAREA_G_MultiplyMatrix4x4Column(MAREA_G_Register4 columnOfLeft0, MAREA_G_Register4 columnOfLeft1, MAREA_G_Register4 columnOfLeft2, MAREA_G_Register4 columnOfLeft3, const MAREA_MathVector4Float32 *columnOfRight)
{
    const MAREA_G_Register4 LANE0 = MAREA_G_Multiply4(columnOfLeft0, MAREA_G_Splat4(columnOfRight->x));
    const MAREA_G_Register4 LANE1 = MAREA_G_Multiply4(columnOfLeft1, MAREA_G_Splat4(columnOfRight->y));
    const MAREA_G_Register4 LANE2 = MAREA_G_Multiply4(columnOfLeft2, MAREA_G_Splat4(columnOfRight->z));
    const MAREA_G_Register4 LANE3 = MAREA_G_Multiply4(columnOfLeft3, MAREA_G_Splat4(columnOfRight->w));

    const MAREA_G_Register4 SUM01 = MAREA_G_Add4(LANE0, LANE1);
    const MAREA_G_Register4 SUM012 = MAREA_G_Add4(SUM01, LANE2);

    return MAREA_G_Add4(SUM012, LANE3);
}

void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32Intrinsics(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result)
{
    const MAREA_G_Register4 COLUMN0 = MAREA_G_Load4(&left->column0);
    const MAREA_G_Register4 COLUMN1 = MAREA_G_Load4(&left->column1);
    const MAREA_G_Register4 COLUMN2 = MAREA_G_Load4(&left->column2);
    const MAREA_G_Register4 COLUMN3 = MAREA_G_Load4(&left->column3);

    MAREA_MathMatrix4x4Float32 output = { 0 };

    MAREA_G_Store4(&output.column0, MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column0));
    MAREA_G_Store4(&output.column1, MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column1));
    MAREA_G_Store4(&output.column2, MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column2));
    MAREA_G_Store4(&output.column3, MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column3));

    *result = output;
}

// INFO: the same sixteen lane products and the same four sums and differences, in the same grouping, as
// MAREA_MathMultiplyQuaternionFloat32VectorExtension: each result lane reads the two registers at the same
// index, so the bits are the same as that backend's, which already matches the scalar one.
void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32Intrinsics(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_MathQuaternionFloat32 A = *left;
    const MAREA_G_Register4 B = MAREA_G_Load4(right);

    const MAREA_G_Register4 B_WZYX = MAREA_G_Shuffle4(B, 3, 2, 1, 0);
    const MAREA_G_Register4 B_ZWXY = MAREA_G_Shuffle4(B, 2, 3, 0, 1);
    const MAREA_G_Register4 B_YXWZ = MAREA_G_Shuffle4(B, 1, 0, 3, 2);

    const MAREA_G_Register4 WB = MAREA_G_Multiply4(B, MAREA_G_Splat4(A.w));
    const MAREA_G_Register4 XB = MAREA_G_Multiply4(B_WZYX, MAREA_G_Splat4(A.x));
    const MAREA_G_Register4 YB = MAREA_G_Multiply4(B_ZWXY, MAREA_G_Splat4(A.y));
    const MAREA_G_Register4 ZB = MAREA_G_Multiply4(B_YXWZ, MAREA_G_Splat4(A.z));

    *result = (MAREA_MathQuaternionFloat32){
        .x = ((MAREA_G_Lane4(WB, 0) + MAREA_G_Lane4(XB, 0)) + (MAREA_G_Lane4(YB, 0) - MAREA_G_Lane4(ZB, 0))),
        .y = ((MAREA_G_Lane4(WB, 1) - MAREA_G_Lane4(XB, 1)) + (MAREA_G_Lane4(YB, 1) + MAREA_G_Lane4(ZB, 1))),
        .z = ((MAREA_G_Lane4(WB, 2) + MAREA_G_Lane4(XB, 2)) - (MAREA_G_Lane4(YB, 2) - MAREA_G_Lane4(ZB, 2))),
        .w = ((MAREA_G_Lane4(WB, 3) - MAREA_G_Lane4(XB, 3)) - (MAREA_G_Lane4(YB, 3) + MAREA_G_Lane4(ZB, 3))),
    };
}

// INFO: the one C macro that registers a function for the language runtime (lib/source/marea/base/MAREAMathLibrary.c
// registers the matching scalar function, MAREAMathVectorExtension.c the matching vector-extension one, the same
// way); the name-based registry entry that lets marea.math's facade pick this backend by the backend/when
// attributes is the facade's own job, not this file's.
MAREA_HOST_FUNCTION(MAREA_MathAddVector2Float32Intrinsics, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector3Float32Intrinsics, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathNormalizeVector4Float32Intrinsics, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix4x4Float32Intrinsics, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyQuaternionFloat32Intrinsics, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));

#endif /* MAREA_MATH_INTRINSICS_X86_SSE2 || MAREA_MATH_INTRINSICS_AARCH64_NEON */
