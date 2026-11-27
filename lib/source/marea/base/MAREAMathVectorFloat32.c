// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"

#include <math.h>
#include <string.h>

// INFO: each product is rounded by itself, so a result does not depend on whether the processor has a fused multiply and add.
#if defined(__clang__)
    #pragma STDC FP_CONTRACT OFF
#endif

#define MAREA_G_LANE_COUNT         4
#define MAREA_G_RADIANS_PER_DEGREE MAREA_FLOAT_32(0.01745329251994329577)
#define MAREA_G_DEGREES_PER_RADIAN MAREA_FLOAT_32(57.2957795130823208768)

// INFO: the bytes of a vector of float32 as four lanes. The lanes past the size of the vector are its padding.
typedef struct MAREA_ALIGNED(4) MAREA_G_Lanes
{
    MAREA_Float32 lane[MAREA_G_LANE_COUNT];
} MAREA_G_Lanes;

MAREA_STATIC_ASSERT((sizeof(MAREA_G_Lanes) == sizeof(MAREA_MathVector2Float32)) && (sizeof(MAREA_G_Lanes) == sizeof(MAREA_MathVector3Float32)) && (sizeof(MAREA_G_Lanes) == sizeof(MAREA_MathVector4Float32)), "A vector of float32 is four lanes:");

typedef MAREA_Float32(MAREA_CALL *MAREA_G_UnaryOperation)(MAREA_Float32 value);
typedef MAREA_Float32(MAREA_CALL *MAREA_G_BinaryOperation)(MAREA_Float32 left, MAREA_Float32 right);

static MAREA_G_Lanes MAREA_CALL MAREA_G_Load(const void *value)
{
    MAREA_G_Lanes lanes;

    memcpy(&lanes, value, sizeof(lanes));

    return lanes;
}

static void MAREA_CALL MAREA_G_Unary(const void *value, MAREA_Null result, MAREA_IntegerSize count, MAREA_G_UnaryOperation operation)
{
    const MAREA_G_Lanes INPUT = MAREA_G_Load(value);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = operation(INPUT.lane[index]);
    }

    memcpy(result, &output, sizeof(output));
}

static void MAREA_CALL MAREA_G_Binary(const void *left, const void *right, MAREA_Null result, MAREA_IntegerSize count, MAREA_G_BinaryOperation operation)
{
    const MAREA_G_Lanes LEFT = MAREA_G_Load(left);
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = operation(LEFT.lane[index], RIGHT.lane[index]);
    }

    memcpy(result, &output, sizeof(output));
}

static MAREA_Float32 MAREA_CALL MAREA_G_Add(MAREA_Float32 left, MAREA_Float32 right)
{
    return (left + right);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Subtract(MAREA_Float32 left, MAREA_Float32 right)
{
    return (left - right);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Multiply(MAREA_Float32 left, MAREA_Float32 right)
{
    return (left * right);
}

// INFO: the sign of the NaN that a processor makes for 0 / 0 differs, so a quotient that is a NaN is the positive one.
static MAREA_Float32 MAREA_CALL MAREA_G_Divide(MAREA_Float32 left, MAREA_Float32 right)
{
    const MAREA_Float32 QUOTIENT = (left / right);

    return (isnan(QUOTIENT) ? (MAREA_Float32)NAN : QUOTIENT);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Minimum(MAREA_Float32 left, MAREA_Float32 right)
{
    return ((left < right) ? left : right);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Maximum(MAREA_Float32 left, MAREA_Float32 right)
{
    return ((left > right) ? left : right);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Power(MAREA_Float32 left, MAREA_Float32 right)
{
    return powf(left, right);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Absolute(MAREA_Float32 value)
{
    return fabsf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Sine(MAREA_Float32 value)
{
    return sinf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Cosine(MAREA_Float32 value)
{
    return cosf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Tangent(MAREA_Float32 value)
{
    return tanf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_ArcSine(MAREA_Float32 value)
{
    return asinf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_ArcCosine(MAREA_Float32 value)
{
    return acosf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_ArcTangent(MAREA_Float32 value)
{
    return atanf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Radians(MAREA_Float32 value)
{
    return (value * MAREA_G_RADIANS_PER_DEGREE);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Degrees(MAREA_Float32 value)
{
    return (value * MAREA_G_DEGREES_PER_RADIAN);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Exponential(MAREA_Float32 value)
{
    return expf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Logarithm(MAREA_Float32 value)
{
    return logf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_SquareRoot(MAREA_Float32 value)
{
    return sqrtf(value);
}

static MAREA_Float32 MAREA_CALL MAREA_G_InverseSquareRoot(MAREA_Float32 value)
{
    return ((value == MAREA_FLOAT_32(0.0)) ? MAREA_FLOAT_32(0.0) : (MAREA_FLOAT_32(1.0) / sqrtf(value)));
}

static MAREA_Float32 MAREA_CALL MAREA_G_DotOfLanes(const MAREA_G_Lanes *left, const MAREA_G_Lanes *right, MAREA_IntegerSize count)
{
    MAREA_Float32 sum = (left->lane[0] * right->lane[0]);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(1); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        sum = (sum + (left->lane[index] * right->lane[index]));
    }

    return sum;
}

static MAREA_Float32 MAREA_CALL MAREA_G_Dot(const void *left, const void *right, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes LEFT = MAREA_G_Load(left);
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    return MAREA_G_DotOfLanes(&LEFT, &RIGHT, count);
}

static MAREA_Float32 MAREA_CALL MAREA_G_Length(const void *value, MAREA_IntegerSize count)
{
    return sqrtf(MAREA_G_Dot(value, value, count));
}

static MAREA_Float32 MAREA_CALL MAREA_G_Distance(const void *left, const void *right, MAREA_IntegerSize count)
{
    MAREA_G_Lanes difference = { 0 };

    MAREA_G_Binary(left, right, &difference, count, &MAREA_G_Subtract);

    return MAREA_G_Length(&difference, count);
}

static void MAREA_CALL MAREA_G_Scale(const void *value, MAREA_Float32 factor, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes INPUT = MAREA_G_Load(value);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = (INPUT.lane[index] * factor);
    }

    memcpy(result, &output, sizeof(output));
}

static void MAREA_CALL MAREA_G_Normalize(const void *value, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_Float32 LENGTH = MAREA_G_Length(value, count);

    if (LENGTH == MAREA_FLOAT_32(0.0)) {
        memset(result, 0, sizeof(MAREA_G_Lanes));

        return;
    }

    MAREA_G_Scale(value, (MAREA_FLOAT_32(1.0) / LENGTH), result, count);
}

static void MAREA_CALL MAREA_G_Mix(const void *left, const void *right, MAREA_Float32 amount, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes LEFT = MAREA_G_Load(left);
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = (LEFT.lane[index] + ((RIGHT.lane[index] - LEFT.lane[index]) * amount));
    }

    memcpy(result, &output, sizeof(output));
}

static void MAREA_CALL MAREA_G_Clamp(const void *value, const void *low, const void *high, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes VALUE = MAREA_G_Load(value);
    const MAREA_G_Lanes LOW = MAREA_G_Load(low);
    const MAREA_G_Lanes HIGH = MAREA_G_Load(high);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = ((VALUE.lane[index] < LOW.lane[index]) ? LOW.lane[index] : ((VALUE.lane[index] > HIGH.lane[index]) ? HIGH.lane[index] : VALUE.lane[index]));
    }

    memcpy(result, &output, sizeof(output));
}

#define MAREA_G_DEFINE_UNARY(name, size)                                                                                                           \
    void MAREA_CALL MAREA_Math##name##Vector##size##Float32(const MAREA_MathVector##size##Float32 *value, MAREA_MathVector##size##Float32 *result) \
    {                                                                                                                                              \
        MAREA_G_Unary(value, result, MAREA_INTEGER_SIZE(size), &MAREA_G_##name);                                                                   \
    }

#define MAREA_G_DEFINE_BINARY(name, size)                                                                                                                                                       \
    void MAREA_CALL MAREA_Math##name##Vector##size##Float32(const MAREA_MathVector##size##Float32 *left, const MAREA_MathVector##size##Float32 *right, MAREA_MathVector##size##Float32 *result) \
    {                                                                                                                                                                                           \
        MAREA_G_Binary(left, right, result, MAREA_INTEGER_SIZE(size), &MAREA_G_##name);                                                                                                         \
    }

// INFO: the public functions of one size. Each one is the shared loop with the lane count of that size.
#define MAREA_G_DEFINE_VECTOR(size)                                                                                                                                                                                                      \
    MAREA_G_DEFINE_BINARY(Add, size)                                                                                                                                                                                                     \
    MAREA_G_DEFINE_BINARY(Subtract, size)                                                                                                                                                                                                \
    MAREA_G_DEFINE_BINARY(Multiply, size)                                                                                                                                                                                                \
    MAREA_G_DEFINE_BINARY(Divide, size)                                                                                                                                                                                                  \
    MAREA_G_DEFINE_BINARY(Minimum, size)                                                                                                                                                                                                 \
    MAREA_G_DEFINE_BINARY(Maximum, size)                                                                                                                                                                                                 \
    MAREA_G_DEFINE_BINARY(Power, size)                                                                                                                                                                                                   \
    MAREA_G_DEFINE_UNARY(Absolute, size)                                                                                                                                                                                                 \
    MAREA_G_DEFINE_UNARY(Sine, size)                                                                                                                                                                                                     \
    MAREA_G_DEFINE_UNARY(Cosine, size)                                                                                                                                                                                                   \
    MAREA_G_DEFINE_UNARY(Tangent, size)                                                                                                                                                                                                  \
    MAREA_G_DEFINE_UNARY(ArcSine, size)                                                                                                                                                                                                  \
    MAREA_G_DEFINE_UNARY(ArcCosine, size)                                                                                                                                                                                                \
    MAREA_G_DEFINE_UNARY(ArcTangent, size)                                                                                                                                                                                               \
    MAREA_G_DEFINE_UNARY(Radians, size)                                                                                                                                                                                                  \
    MAREA_G_DEFINE_UNARY(Degrees, size)                                                                                                                                                                                                  \
    MAREA_G_DEFINE_UNARY(Exponential, size)                                                                                                                                                                                              \
    MAREA_G_DEFINE_UNARY(Logarithm, size)                                                                                                                                                                                                \
    MAREA_G_DEFINE_UNARY(SquareRoot, size)                                                                                                                                                                                               \
    MAREA_G_DEFINE_UNARY(InverseSquareRoot, size)                                                                                                                                                                                        \
    void MAREA_CALL MAREA_MathScaleVector##size##Float32(const MAREA_MathVector##size##Float32 *value, MAREA_Float32 factor, MAREA_MathVector##size##Float32 *result)                                                                    \
    {                                                                                                                                                                                                                                    \
        MAREA_G_Scale(value, factor, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                  \
    }                                                                                                                                                                                                                                    \
    MAREA_Float32 MAREA_CALL MAREA_MathDotVector##size##Float32(const MAREA_MathVector##size##Float32 *left, const MAREA_MathVector##size##Float32 *right)                                                                               \
    {                                                                                                                                                                                                                                    \
        return MAREA_G_Dot(left, right, MAREA_INTEGER_SIZE(size));                                                                                                                                                                       \
    }                                                                                                                                                                                                                                    \
    MAREA_Float32 MAREA_CALL MAREA_MathLengthVector##size##Float32(const MAREA_MathVector##size##Float32 *value)                                                                                                                         \
    {                                                                                                                                                                                                                                    \
        return MAREA_G_Length(value, MAREA_INTEGER_SIZE(size));                                                                                                                                                                          \
    }                                                                                                                                                                                                                                    \
    MAREA_Float32 MAREA_CALL MAREA_MathDistanceVector##size##Float32(const MAREA_MathVector##size##Float32 *left, const MAREA_MathVector##size##Float32 *right)                                                                          \
    {                                                                                                                                                                                                                                    \
        return MAREA_G_Distance(left, right, MAREA_INTEGER_SIZE(size));                                                                                                                                                                  \
    }                                                                                                                                                                                                                                    \
    void MAREA_CALL MAREA_MathNormalizeVector##size##Float32(const MAREA_MathVector##size##Float32 *value, MAREA_MathVector##size##Float32 *result)                                                                                      \
    {                                                                                                                                                                                                                                    \
        MAREA_G_Normalize(value, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                      \
    }                                                                                                                                                                                                                                    \
    void MAREA_CALL MAREA_MathMixVector##size##Float32(const MAREA_MathVector##size##Float32 *left, const MAREA_MathVector##size##Float32 *right, MAREA_Float32 amount, MAREA_MathVector##size##Float32 *result)                         \
    {                                                                                                                                                                                                                                    \
        MAREA_G_Mix(left, right, amount, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                              \
    }                                                                                                                                                                                                                                    \
    void MAREA_CALL MAREA_MathClampVector##size##Float32(const MAREA_MathVector##size##Float32 *value, const MAREA_MathVector##size##Float32 *low, const MAREA_MathVector##size##Float32 *high, MAREA_MathVector##size##Float32 *result) \
    {                                                                                                                                                                                                                                    \
        MAREA_G_Clamp(value, low, high, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                               \
    }

MAREA_G_DEFINE_VECTOR(2)
MAREA_G_DEFINE_VECTOR(3)
MAREA_G_DEFINE_VECTOR(4)

void MAREA_CALL MAREA_MathCrossVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result)
{
    const MAREA_G_Lanes LEFT = MAREA_G_Load(left);
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    MAREA_G_Lanes output = { 0 };

    output.lane[0] = ((LEFT.lane[1] * RIGHT.lane[2]) - (LEFT.lane[2] * RIGHT.lane[1]));
    output.lane[1] = ((LEFT.lane[2] * RIGHT.lane[0]) - (LEFT.lane[0] * RIGHT.lane[2]));
    output.lane[2] = ((LEFT.lane[0] * RIGHT.lane[1]) - (LEFT.lane[1] * RIGHT.lane[0]));

    memcpy(result, &output, sizeof(output));
}
