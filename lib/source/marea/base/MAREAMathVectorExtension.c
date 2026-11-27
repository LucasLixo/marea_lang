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

// INFO: the GCC/clang vector_size extension on the 16-byte structures of marea.math, the one exception to "C11
// only" that c-style records for this file (decision D8): a lane past the real width of a type is read or
// written here too, never left to carry whatever the caller put there.
typedef MAREA_Float32 MAREA_G_Lane4 __attribute__((vector_size(16)));

MAREA_STATIC_ASSERT(sizeof(MAREA_G_Lane4) == sizeof(MAREA_MathVector4Float32), "A lane of four float32 is the size of a vector of four float32:");

MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAddVector2Float32VectorExtension(const MAREA_MathVector2Float32 *left, const MAREA_MathVector2Float32 *right, MAREA_MathVector2Float32 *result);
MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32VectorExtension(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathNormalizeVector4Float32VectorExtension(const MAREA_MathVector4Float32 *value, MAREA_MathVector4Float32 *result);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32VectorExtension(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result);
MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32VectorExtension(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result);

// INFO: the sum of two lanes past the real width of Vector2Float32 (its padding) is never written to the result;
// the scalar backend always gives zero there too, whatever the two inputs carry in that padding.
void MAREA_CALL MAREA_MathAddVector2Float32VectorExtension(const MAREA_MathVector2Float32 *left, const MAREA_MathVector2Float32 *right, MAREA_MathVector2Float32 *result)
{
    const MAREA_G_Lane4 LEFT = *(const MAREA_G_Lane4 *)left;
    const MAREA_G_Lane4 RIGHT = *(const MAREA_G_Lane4 *)right;
    const MAREA_G_Lane4 SUM = (LEFT + RIGHT);

    result->x = SUM[0];
    result->y = SUM[1];
    result->padding[0] = MAREA_FLOAT_32(0.0);
    result->padding[1] = MAREA_FLOAT_32(0.0);
}

// INFO: the fourth lane of the product (the padding of Vector3Float32) is never added to the sum, the same order
// the scalar backend sums in: (p0 + p1) + p2, never (... + p3).
MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32VectorExtension(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right)
{
    const MAREA_G_Lane4 LEFT = *(const MAREA_G_Lane4 *)left;
    const MAREA_G_Lane4 RIGHT = *(const MAREA_G_Lane4 *)right;
    const MAREA_G_Lane4 PRODUCT = (LEFT * RIGHT);

    const MAREA_Float32 SUM01 = (PRODUCT[0] + PRODUCT[1]);

    return (SUM01 + PRODUCT[2]);
}

void MAREA_CALL MAREA_MathNormalizeVector4Float32VectorExtension(const MAREA_MathVector4Float32 *value, MAREA_MathVector4Float32 *result)
{
    const MAREA_G_Lane4 VALUE = *(const MAREA_G_Lane4 *)value;
    const MAREA_G_Lane4 SQUARE = (VALUE * VALUE);

    const MAREA_Float32 SUM01 = (SQUARE[0] + SQUARE[1]);
    const MAREA_Float32 SUM012 = (SUM01 + SQUARE[2]);
    const MAREA_Float32 LENGTH = sqrtf(SUM012 + SQUARE[3]);

    if (LENGTH == MAREA_FLOAT_32(0.0)) {
        memset(result, 0, sizeof(*result));

        return;
    }

    const MAREA_Float32 INVERSE = (MAREA_FLOAT_32(1.0) / LENGTH);
    const MAREA_G_Lane4 FACTOR = { INVERSE, INVERSE, INVERSE, INVERSE };
    const MAREA_G_Lane4 SCALED = (VALUE * FACTOR);

    result->x = SCALED[0];
    result->y = SCALED[1];
    result->z = SCALED[2];
    result->w = SCALED[3];
}

// INFO: one column of the product, the same four lane products and the same left-to-right sum order as the
// scalar backend's MAREA_G_Transform: ((p0 + p1) + p2) + p3.
static MAREA_G_Lane4 MAREA_CALL MAREA_G_MultiplyMatrix4x4Column(MAREA_G_Lane4 columnOfLeft0, MAREA_G_Lane4 columnOfLeft1, MAREA_G_Lane4 columnOfLeft2, MAREA_G_Lane4 columnOfLeft3, const MAREA_MathVector4Float32 *columnOfRight)
{
    const MAREA_G_Lane4 LANE0 = (columnOfLeft0 * (MAREA_G_Lane4){ columnOfRight->x, columnOfRight->x, columnOfRight->x, columnOfRight->x });
    const MAREA_G_Lane4 LANE1 = (columnOfLeft1 * (MAREA_G_Lane4){ columnOfRight->y, columnOfRight->y, columnOfRight->y, columnOfRight->y });
    const MAREA_G_Lane4 LANE2 = (columnOfLeft2 * (MAREA_G_Lane4){ columnOfRight->z, columnOfRight->z, columnOfRight->z, columnOfRight->z });
    const MAREA_G_Lane4 LANE3 = (columnOfLeft3 * (MAREA_G_Lane4){ columnOfRight->w, columnOfRight->w, columnOfRight->w, columnOfRight->w });

    const MAREA_G_Lane4 SUM01 = (LANE0 + LANE1);
    const MAREA_G_Lane4 SUM012 = (SUM01 + LANE2);

    return (SUM012 + LANE3);
}

static void MAREA_CALL MAREA_G_StoreColumn(MAREA_G_Lane4 value, MAREA_MathVector4Float32 *result)
{
    result->x = value[0];
    result->y = value[1];
    result->z = value[2];
    result->w = value[3];
}

void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32VectorExtension(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result)
{
    const MAREA_G_Lane4 COLUMN0 = *(const MAREA_G_Lane4 *)&left->column0;
    const MAREA_G_Lane4 COLUMN1 = *(const MAREA_G_Lane4 *)&left->column1;
    const MAREA_G_Lane4 COLUMN2 = *(const MAREA_G_Lane4 *)&left->column2;
    const MAREA_G_Lane4 COLUMN3 = *(const MAREA_G_Lane4 *)&left->column3;

    MAREA_MathMatrix4x4Float32 output = { 0 };

    MAREA_G_StoreColumn(MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column0), &output.column0);
    MAREA_G_StoreColumn(MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column1), &output.column1);
    MAREA_G_StoreColumn(MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column2), &output.column2);
    MAREA_G_StoreColumn(MAREA_G_MultiplyMatrix4x4Column(COLUMN0, COLUMN1, COLUMN2, COLUMN3, &right->column3), &output.column3);

    *result = output;
}

// INFO: sixteen lane products as four vector multiplies (WB, XB, YB, ZB), then the same four sums and
// differences, in the same grouping, as MAREA_MathMultiplyQuaternionFloat32's scalar body. Multiplication of
// two finite float32 is exactly commutative, so b * a here gives the same bits as the scalar body's a * b.
void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32VectorExtension(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_MathQuaternionFloat32 A = *left;
    const MAREA_MathQuaternionFloat32 B = *right;

    const MAREA_G_Lane4 B_XYZW = { B.x, B.y, B.z, B.w };
    const MAREA_G_Lane4 B_WZYX = { B.w, B.z, B.y, B.x };
    const MAREA_G_Lane4 B_ZWXY = { B.z, B.w, B.x, B.y };
    const MAREA_G_Lane4 B_YXWZ = { B.y, B.x, B.w, B.z };

    const MAREA_G_Lane4 WB = (B_XYZW * (MAREA_G_Lane4){ A.w, A.w, A.w, A.w });
    const MAREA_G_Lane4 XB = (B_WZYX * (MAREA_G_Lane4){ A.x, A.x, A.x, A.x });
    const MAREA_G_Lane4 YB = (B_ZWXY * (MAREA_G_Lane4){ A.y, A.y, A.y, A.y });
    const MAREA_G_Lane4 ZB = (B_YXWZ * (MAREA_G_Lane4){ A.z, A.z, A.z, A.z });

    *result = (MAREA_MathQuaternionFloat32){
        .x = ((WB[0] + XB[0]) + (YB[0] - ZB[0])),
        .y = ((WB[1] - XB[1]) + (YB[1] + ZB[1])),
        .z = ((WB[2] + XB[2]) - (YB[2] - ZB[2])),
        .w = ((WB[3] - XB[3]) - (YB[3] + ZB[3])),
    };
}

// INFO: the one C macro that registers a function for the language runtime (lib/source/marea/base/MAREAMathLibrary.c
// registers the matching scalar function the same way); the name-based registry entry that lets marea.math's
// facade pick this backend by the backend/when attributes is the facade's own job (apply lib-math-simd-backends,
// part 4), not this file's.
MAREA_HOST_FUNCTION(MAREA_MathAddVector2Float32VectorExtension, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32), MAREA_HOST_BORROW(MAREA_MathVector2Float32));
MAREA_HOST_FUNCTION(MAREA_MathDotVector3Float32VectorExtension, MAREA_HOST_RETURNS(float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32), MAREA_HOST_BORROW(MAREA_MathVector3Float32));
MAREA_HOST_FUNCTION(MAREA_MathNormalizeVector4Float32VectorExtension, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathVector4Float32), MAREA_HOST_BORROW(MAREA_MathVector4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyMatrix4x4Float32VectorExtension, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32), MAREA_HOST_BORROW(MAREA_MathMatrix4x4Float32));
MAREA_HOST_FUNCTION(MAREA_MathMultiplyQuaternionFloat32VectorExtension, MAREA_HOST_RETURNS_STRUCTURE(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32), MAREA_HOST_BORROW(MAREA_MathQuaternionFloat32));
