// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathQuaternion.h"

#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathVector3.h"

#include <math.h>
#include <string.h>

// INFO: each product is rounded by itself, so a result does not depend on whether the processor has a fused multiply and add.
#if defined(__clang__)
    #pragma STDC FP_CONTRACT OFF
#endif

// INFO: past this cosine the two ends are so near that the sine of the angle is not a safe divisor.
#define MAREA_G_SPHERICAL_COSINE_LIMIT MAREA_FLOAT_32(0.9995)

void MAREA_CALL MAREA_MathIdentityQuaternionFloat32(MAREA_MathQuaternionFloat32 *result)
{
    *result = (MAREA_MathQuaternionFloat32){ .w = MAREA_FLOAT_32(1.0) };
}

void MAREA_CALL MAREA_MathFromAxisAngleQuaternionFloat32(const MAREA_MathVector3Float32 *axis, MAREA_Float32 angle, MAREA_MathQuaternionFloat32 *result)
{
    MAREA_MathVector3Float32 a;

    MAREA_MathNormalizeVector3Float32(axis, &a);

    const MAREA_Float32 HALF = (angle * MAREA_FLOAT_32(0.5));
    const MAREA_Float32 S = sinf(HALF);

    *result = (MAREA_MathQuaternionFloat32){ .x = (a.x * S), .y = (a.y * S), .z = (a.z * S), .w = cosf(HALF) };
}

void MAREA_CALL MAREA_MathFromEulerQuaternionFloat32(MAREA_Float32 yaw, MAREA_Float32 pitch, MAREA_Float32 roll, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_Float32 CY = cosf(yaw * MAREA_FLOAT_32(0.5));
    const MAREA_Float32 SY = sinf(yaw * MAREA_FLOAT_32(0.5));
    const MAREA_Float32 CP = cosf(pitch * MAREA_FLOAT_32(0.5));
    const MAREA_Float32 SP = sinf(pitch * MAREA_FLOAT_32(0.5));
    const MAREA_Float32 CR = cosf(roll * MAREA_FLOAT_32(0.5));
    const MAREA_Float32 SR = sinf(roll * MAREA_FLOAT_32(0.5));

    *result = (MAREA_MathQuaternionFloat32){
        .x = (((SR * CP) * CY) - ((CR * SP) * SY)),
        .y = (((CR * SP) * CY) + ((SR * CP) * SY)),
        .z = (((CR * CP) * SY) - ((SR * SP) * CY)),
        .w = (((CR * CP) * CY) + ((SR * SP) * SY)),
    };
}

void MAREA_CALL MAREA_MathToMatrix4x4QuaternionFloat32(const MAREA_MathQuaternionFloat32 *value, MAREA_MathMatrix4x4Float32 *result)
{
    const MAREA_MathQuaternionFloat32 Q = *value;

    const MAREA_Float32 XX = (Q.x * Q.x);
    const MAREA_Float32 YY = (Q.y * Q.y);
    const MAREA_Float32 ZZ = (Q.z * Q.z);
    const MAREA_Float32 XY = (Q.x * Q.y);
    const MAREA_Float32 XZ = (Q.x * Q.z);
    const MAREA_Float32 YZ = (Q.y * Q.z);
    const MAREA_Float32 WX = (Q.w * Q.x);
    const MAREA_Float32 WY = (Q.w * Q.y);
    const MAREA_Float32 WZ = (Q.w * Q.z);

    const MAREA_Float32 ONE = MAREA_FLOAT_32(1.0);
    const MAREA_Float32 TWO = MAREA_FLOAT_32(2.0);

    memset(result, 0, sizeof(*result));

    result->column0.x = (ONE - (TWO * (YY + ZZ)));
    result->column0.y = (TWO * (XY + WZ));
    result->column0.z = (TWO * (XZ - WY));
    result->column1.x = (TWO * (XY - WZ));
    result->column1.y = (ONE - (TWO * (XX + ZZ)));
    result->column1.z = (TWO * (YZ + WX));
    result->column2.x = (TWO * (XZ + WY));
    result->column2.y = (TWO * (YZ - WX));
    result->column2.z = (ONE - (TWO * (XX + YY)));
    result->column3.w = ONE;
}

void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_MathQuaternionFloat32 A = *left;
    const MAREA_MathQuaternionFloat32 B = *right;

    *result = (MAREA_MathQuaternionFloat32){
        .x = (((A.w * B.x) + (A.x * B.w)) + ((A.y * B.z) - (A.z * B.y))),
        .y = (((A.w * B.y) - (A.x * B.z)) + ((A.y * B.w) + (A.z * B.x))),
        .z = (((A.w * B.z) + (A.x * B.y)) - ((A.y * B.x) - (A.z * B.w))),
        .w = (((A.w * B.w) - (A.x * B.x)) - ((A.y * B.y) + (A.z * B.z))),
    };
}

void MAREA_CALL MAREA_MathConjugateQuaternionFloat32(const MAREA_MathQuaternionFloat32 *value, MAREA_MathQuaternionFloat32 *result)
{
    *result = (MAREA_MathQuaternionFloat32){ .x = (-value->x), .y = (-value->y), .z = (-value->z), .w = value->w };
}

MAREA_Float32 MAREA_CALL MAREA_MathDotQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right)
{
    return ((((left->x * right->x) + (left->y * right->y)) + (left->z * right->z)) + (left->w * right->w));
}

MAREA_Float32 MAREA_CALL MAREA_MathLengthQuaternionFloat32(const MAREA_MathQuaternionFloat32 *value)
{
    return sqrtf(MAREA_MathDotQuaternionFloat32(value, value));
}

void MAREA_CALL MAREA_MathNormalizeQuaternionFloat32(const MAREA_MathQuaternionFloat32 *value, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_Float32 LENGTH = MAREA_MathLengthQuaternionFloat32(value);

    if (LENGTH == MAREA_FLOAT_32(0.0)) {
        MAREA_MathIdentityQuaternionFloat32(result);

        return;
    }

    const MAREA_Float32 INVERSE = (MAREA_FLOAT_32(1.0) / LENGTH);

    *result = (MAREA_MathQuaternionFloat32){ .x = (value->x * INVERSE), .y = (value->y * INVERSE), .z = (value->z * INVERSE), .w = (value->w * INVERSE) };
}

void MAREA_CALL MAREA_MathMixQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_Float32 amount, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_MathQuaternionFloat32 MIXED = {
        .x = (left->x + ((right->x - left->x) * amount)),
        .y = (left->y + ((right->y - left->y) * amount)),
        .z = (left->z + ((right->z - left->z) * amount)),
        .w = (left->w + ((right->w - left->w) * amount)),
    };

    MAREA_MathNormalizeQuaternionFloat32(&MIXED, result);
}

void MAREA_CALL MAREA_MathSphericalMixQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_Float32 amount, MAREA_MathQuaternionFloat32 *result)
{
    const MAREA_MathQuaternionFloat32 START = *left;

    MAREA_MathQuaternionFloat32 end = *right;

    MAREA_Float32 cosine = MAREA_MathDotQuaternionFloat32(left, right);

    // INFO: a quaternion and its negation are one rotation, so the nearer of the two ends gives the shortest arc.
    if (cosine < MAREA_FLOAT_32(0.0)) {
        end = (MAREA_MathQuaternionFloat32){ .x = (-end.x), .y = (-end.y), .z = (-end.z), .w = (-end.w) };

        cosine = (-cosine);
    }

    if (cosine > MAREA_G_SPHERICAL_COSINE_LIMIT) {
        MAREA_MathMixQuaternionFloat32(&START, &end, amount, result);

        return;
    }

    const MAREA_Float32 ANGLE = acosf(cosine);
    const MAREA_Float32 SINE = sinf(ANGLE);
    const MAREA_Float32 START_WEIGHT = (sinf((MAREA_FLOAT_32(1.0) - amount) * ANGLE) / SINE);
    const MAREA_Float32
END_WEIGHT
= (sinf(amount * ANGLE) / SINE);

*result = (MAREA_MathQuaternionFloat32){
    .x = ((START.x * START_WEIGHT) + (end.x * END_WEIGHT)),
    .y = ((START.y * START_WEIGHT) + (end.y * END_WEIGHT)),
    .z = ((START.z * START_WEIGHT) + (end.z * END_WEIGHT)),
    .w = ((START.w * START_WEIGHT) + (end.w * END_WEIGHT)),
};
}

// INFO: value + 2 * cross(q, cross(q, value) + value * w), with q the vector part of the quaternion.
void MAREA_CALL MAREA_MathRotateVector3QuaternionFloat32(const MAREA_MathQuaternionFloat32 *rotation, const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result)
{
    const MAREA_MathVector3Float32 AXIS = { .x = rotation->x, .y = rotation->y, .z = rotation->z };
    const MAREA_MathVector3Float32 VALUE = *value;

    MAREA_MathVector3Float32 first;
    MAREA_MathVector3Float32 second;

    MAREA_MathCrossVector3Float32(&AXIS, &VALUE, &first);

    first.x = (first.x + (VALUE.x * rotation->w));
    first.y = (first.y + (VALUE.y * rotation->w));
    first.z = (first.z + (VALUE.z * rotation->w));

    MAREA_MathCrossVector3Float32(&AXIS, &first, &second);

    *result = (MAREA_MathVector3Float32){
        .x = (VALUE.x + (MAREA_FLOAT_32(2.0) * second.x)),
        .y = (VALUE.y + (MAREA_FLOAT_32(2.0) * second.y)),
        .z = (VALUE.z + (MAREA_FLOAT_32(2.0) * second.z)),
    };
}
