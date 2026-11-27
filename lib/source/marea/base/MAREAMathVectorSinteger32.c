// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"

#include "marea/base/MAREAHostContext.h"

#include <string.h>

#define MAREA_G_LANE_COUNT 4

// INFO: the bytes of a vector of sinteger32 as four lanes. The lanes past the size of the vector are its padding.
typedef struct MAREA_ALIGNED(4) MAREA_G_Lanes
{
    MAREA_Sinteger32 lane[MAREA_G_LANE_COUNT];
} MAREA_G_Lanes;

MAREA_STATIC_ASSERT((sizeof(MAREA_G_Lanes) == sizeof(MAREA_MathVector2Sinteger32)) && (sizeof(MAREA_G_Lanes) == sizeof(MAREA_MathVector3Sinteger32)) && (sizeof(MAREA_G_Lanes) == sizeof(MAREA_MathVector4Sinteger32)), "A vector of sinteger32 is four lanes:");

typedef MAREA_Sinteger32(MAREA_CALL *MAREA_G_BinaryOperation)(MAREA_Sinteger32 left, MAREA_Sinteger32 right);

static MAREA_G_Lanes MAREA_CALL MAREA_G_Load(const void *value)
{
    MAREA_G_Lanes lanes;

    memcpy(&lanes, value, sizeof(lanes));

    return lanes;
}

// INFO: the arithmetic runs on the unsigned type, whose overflow is defined, and gives the wrapped signed value.
static MAREA_Sinteger32 MAREA_CALL MAREA_G_Add(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return (MAREA_Sinteger32)((MAREA_Uinteger32)left + (MAREA_Uinteger32)right);
}

static MAREA_Sinteger32 MAREA_CALL MAREA_G_Subtract(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return (MAREA_Sinteger32)((MAREA_Uinteger32)left - (MAREA_Uinteger32)right);
}

static MAREA_Sinteger32 MAREA_CALL MAREA_G_Multiply(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return (MAREA_Sinteger32)((MAREA_Uinteger32)left * (MAREA_Uinteger32)right);
}

// INFO: the caller refuses a zero divisor first. A divisor of -1 is a negation, so the smallest value wraps to itself.
static MAREA_Sinteger32 MAREA_CALL MAREA_G_Quotient(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return ((right == MAREA_SINTEGER_32(-1)) ? MAREA_G_Subtract(MAREA_SINTEGER_32(0), left) : (left / right));
}

static MAREA_Sinteger32 MAREA_CALL MAREA_G_Minimum(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return ((left < right) ? left : right);
}

static MAREA_Sinteger32 MAREA_CALL MAREA_G_Maximum(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return ((left > right) ? left : right);
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

// INFO: only the lanes of the vector are divisors: the padding is zero and is never read as one.
static void MAREA_CALL MAREA_G_Divide(MAREA_HostContext *context, const void *left, const void *right, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (RIGHT.lane[index] == MAREA_SINTEGER_32(0)) {
            MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_DIVISION_BY_ZERO);

            memset(result, 0, sizeof(MAREA_G_Lanes));

            return;
        }
    }

    MAREA_G_Binary(left, right, result, count, &MAREA_G_Quotient);
}

static void MAREA_CALL MAREA_G_Scale(const void *value, MAREA_Sinteger32 factor, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes INPUT = MAREA_G_Load(value);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = MAREA_G_Multiply(INPUT.lane[index], factor);
    }

    memcpy(result, &output, sizeof(output));
}

static MAREA_Sinteger32 MAREA_CALL MAREA_G_Dot(const void *left, const void *right, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes LEFT = MAREA_G_Load(left);
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    MAREA_Sinteger32 sum = MAREA_SINTEGER_32(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        sum = MAREA_G_Add(sum, MAREA_G_Multiply(LEFT.lane[index], RIGHT.lane[index]));
    }

    return sum;
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

static void MAREA_CALL MAREA_G_Absolute(const void *value, MAREA_Null result, MAREA_IntegerSize count)
{
    const MAREA_G_Lanes INPUT = MAREA_G_Load(value);

    MAREA_G_Lanes output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[index] = ((INPUT.lane[index] < MAREA_SINTEGER_32(0)) ? MAREA_G_Subtract(MAREA_SINTEGER_32(0), INPUT.lane[index]) : INPUT.lane[index]);
    }

    memcpy(result, &output, sizeof(output));
}

#define MAREA_G_DEFINE_BINARY(name, size)                                                                                                                                                       \
    void MAREA_CALL MAREA_Math##name##Vector##size##Sinteger32(const MAREA_MathVector##size##Sinteger32 *left, const MAREA_MathVector##size##Sinteger32 *right, MAREA_MathVector##size##Sinteger32 *result) \
    {                                                                                                                                                                                           \
        MAREA_G_Binary(left, right, result, MAREA_INTEGER_SIZE(size), &MAREA_G_##name);                                                                                                         \
    }

// INFO: the public functions of one size. Each one is the shared loop with the lane count of that size.
#define MAREA_G_DEFINE_VECTOR(size)                                                                                                                                                                                                  \
    MAREA_G_DEFINE_BINARY(Add, size)                                                                                                                                                                                                 \
    MAREA_G_DEFINE_BINARY(Subtract, size)                                                                                                                                                                                            \
    MAREA_G_DEFINE_BINARY(Multiply, size)                                                                                                                                                                                            \
    MAREA_G_DEFINE_BINARY(Minimum, size)                                                                                                                                                                                             \
    MAREA_G_DEFINE_BINARY(Maximum, size)                                                                                                                                                                                             \
    void MAREA_CALL MAREA_MathDivideVector##size##Sinteger32(MAREA_HostContext *context, const MAREA_MathVector##size##Sinteger32 *left, const MAREA_MathVector##size##Sinteger32 *right, MAREA_MathVector##size##Sinteger32 *result) \
    {                                                                                                                                                                                                                                \
        MAREA_G_Divide(context, left, right, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                      \
    }                                                                                                                                                                                                                                \
    void MAREA_CALL MAREA_MathScaleVector##size##Sinteger32(const MAREA_MathVector##size##Sinteger32 *value, MAREA_Sinteger32 factor, MAREA_MathVector##size##Sinteger32 *result)                                                    \
    {                                                                                                                                                                                                                                \
        MAREA_G_Scale(value, factor, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                              \
    }                                                                                                                                                                                                                                \
    MAREA_Sinteger32 MAREA_CALL MAREA_MathDotVector##size##Sinteger32(const MAREA_MathVector##size##Sinteger32 *left, const MAREA_MathVector##size##Sinteger32 *right)                                                               \
    {                                                                                                                                                                                                                                \
        return MAREA_G_Dot(left, right, MAREA_INTEGER_SIZE(size));                                                                                                                                                                   \
    }                                                                                                                                                                                                                                \
    void MAREA_CALL MAREA_MathClampVector##size##Sinteger32(const MAREA_MathVector##size##Sinteger32 *value, const MAREA_MathVector##size##Sinteger32 *low, const MAREA_MathVector##size##Sinteger32 *high, MAREA_MathVector##size##Sinteger32 *result) \
    {                                                                                                                                                                                                                                \
        MAREA_G_Clamp(value, low, high, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                           \
    }                                                                                                                                                                                                                                \
    void MAREA_CALL MAREA_MathAbsoluteVector##size##Sinteger32(const MAREA_MathVector##size##Sinteger32 *value, MAREA_MathVector##size##Sinteger32 *result)                                                                          \
    {                                                                                                                                                                                                                                \
        MAREA_G_Absolute(value, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                   \
    }

MAREA_G_DEFINE_VECTOR(2)
MAREA_G_DEFINE_VECTOR(3)
MAREA_G_DEFINE_VECTOR(4)

void MAREA_CALL MAREA_MathCrossVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result)
{
    const MAREA_G_Lanes LEFT = MAREA_G_Load(left);
    const MAREA_G_Lanes RIGHT = MAREA_G_Load(right);

    MAREA_G_Lanes output = { 0 };

    output.lane[0] = MAREA_G_Subtract(MAREA_G_Multiply(LEFT.lane[1], RIGHT.lane[2]), MAREA_G_Multiply(LEFT.lane[2], RIGHT.lane[1]));
    output.lane[1] = MAREA_G_Subtract(MAREA_G_Multiply(LEFT.lane[2], RIGHT.lane[0]), MAREA_G_Multiply(LEFT.lane[0], RIGHT.lane[2]));
    output.lane[2] = MAREA_G_Subtract(MAREA_G_Multiply(LEFT.lane[0], RIGHT.lane[1]), MAREA_G_Multiply(LEFT.lane[1], RIGHT.lane[0]));

    memcpy(result, &output, sizeof(output));
}
