// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathMatrix2x2.h"
#include "marea/base/MAREAMathMatrix3x3.h"
#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"

#include <string.h>

#define MAREA_G_LANE_COUNT      4
#define MAREA_G_MATRIX_LANES    16
#define MAREA_G_AT(column, row) (((column) * MAREA_G_LANE_COUNT) + (row))

// INFO: the bytes of a matrix as four columns of four lanes. A lane past the size of the matrix is padding and stays zero.
typedef struct MAREA_ALIGNED(4) MAREA_G_Matrix
{
    MAREA_Sinteger32 lane[MAREA_G_MATRIX_LANES];
} MAREA_G_Matrix;

typedef struct MAREA_ALIGNED(4) MAREA_G_Column
{
    MAREA_Sinteger32 lane[MAREA_G_LANE_COUNT];
} MAREA_G_Column;

MAREA_STATIC_ASSERT((sizeof(MAREA_G_Matrix) == sizeof(MAREA_MathMatrix4x4Sinteger32)) && (sizeof(MAREA_G_Column) == sizeof(MAREA_MathVector4Sinteger32)), "A matrix of sinteger32 is columns of four lanes:");

static MAREA_G_Matrix MAREA_CALL MAREA_G_Load(const void *value, MAREA_IntegerSize size)
{
    MAREA_G_Matrix matrix = { 0 };

    memcpy(&matrix, value, (size * sizeof(MAREA_G_Column)));

    return matrix;
}

static void MAREA_CALL MAREA_G_Identity(MAREA_Null result, MAREA_IntegerSize size)
{
    MAREA_G_Matrix output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < size; index = (index + MAREA_INTEGER_SIZE(1))) {
        output.lane[MAREA_G_AT(index, index)] = MAREA_SINTEGER_32(1);
    }

    memcpy(result, &output, (size * sizeof(MAREA_G_Column)));
}

static void MAREA_CALL MAREA_G_Transpose(const void *value, MAREA_Null result, MAREA_IntegerSize size)
{
    const MAREA_G_Matrix INPUT = MAREA_G_Load(value, size);

    MAREA_G_Matrix output = { 0 };

    for (MAREA_IntegerSize column = MAREA_INTEGER_SIZE(0); column < size; column = (column + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize row = MAREA_INTEGER_SIZE(0); row < size; row = (row + MAREA_INTEGER_SIZE(1))) {
            output.lane[MAREA_G_AT(column, row)] = INPUT.lane[MAREA_G_AT(row, column)];
        }
    }

    memcpy(result, &output, (size * sizeof(MAREA_G_Column)));
}

// INFO: the arithmetic runs on the unsigned type, whose overflow is defined, and gives the wrapped signed value. output must not be vector.
static void MAREA_CALL MAREA_G_Transform(const MAREA_G_Matrix *matrix, const MAREA_Sinteger32 *MAREA_RESTRICT vector, MAREA_Sinteger32 *MAREA_RESTRICT output, MAREA_IntegerSize size)
{
    for (MAREA_IntegerSize row = MAREA_INTEGER_SIZE(0); row < size; row = (row + MAREA_INTEGER_SIZE(1))) {
        MAREA_Uinteger32 sum = MAREA_UINTEGER_32(0);

        for (MAREA_IntegerSize column = MAREA_INTEGER_SIZE(0); column < size; column = (column + MAREA_INTEGER_SIZE(1))) {
            sum = (sum + ((MAREA_Uinteger32)matrix->lane[MAREA_G_AT(column, row)] * (MAREA_Uinteger32)vector[column]));
        }

        output[row] = (MAREA_Sinteger32)sum;
    }
}

static void MAREA_CALL MAREA_G_Multiply(const void *left, const void *right, MAREA_Null result, MAREA_IntegerSize size)
{
    const MAREA_G_Matrix LEFT = MAREA_G_Load(left, size);
    const MAREA_G_Matrix RIGHT = MAREA_G_Load(right, size);

    MAREA_G_Matrix output = { 0 };

    for (MAREA_IntegerSize column = MAREA_INTEGER_SIZE(0); column < size; column = (column + MAREA_INTEGER_SIZE(1))) {
        MAREA_G_Transform(&LEFT, &RIGHT.lane[MAREA_G_AT(column, 0)], &output.lane[MAREA_G_AT(column, 0)], size);
    }

    memcpy(result, &output, (size * sizeof(MAREA_G_Column)));
}

static void MAREA_CALL MAREA_G_MultiplyVector(const void *matrix, const void *vector, MAREA_Null result, MAREA_IntegerSize size)
{
    const MAREA_G_Matrix MATRIX = MAREA_G_Load(matrix, size);

    MAREA_G_Column input;

    memcpy(&input, vector, sizeof(input));

    MAREA_G_Column output = { 0 };

    MAREA_G_Transform(&MATRIX, input.lane, output.lane, size);

    memcpy(result, &output, sizeof(output));
}

// INFO: the public functions of one size. Each one is the shared loop with the size of that matrix.
#define MAREA_G_DEFINE_MATRIX(size)                                                                                                                                                                                                             \
    void MAREA_CALL MAREA_MathIdentityMatrix##size##x##size##Sinteger32(MAREA_MathMatrix##size##x##size##Sinteger32 *result)                                                                                                                    \
    {                                                                                                                                                                                                                                           \
        MAREA_G_Identity(result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                                     \
    }                                                                                                                                                                                                                                           \
    void MAREA_CALL MAREA_MathTransposeMatrix##size##x##size##Sinteger32(const MAREA_MathMatrix##size##x##size##Sinteger32 *value, MAREA_MathMatrix##size##x##size##Sinteger32 *result)                                                         \
    {                                                                                                                                                                                                                                           \
        MAREA_G_Transpose(value, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                             \
    }                                                                                                                                                                                                                                           \
    void MAREA_CALL MAREA_MathMultiplyMatrix##size##x##size##Sinteger32(const MAREA_MathMatrix##size##x##size##Sinteger32 *left, const MAREA_MathMatrix##size##x##size##Sinteger32 *right, MAREA_MathMatrix##size##x##size##Sinteger32 *result) \
    {                                                                                                                                                                                                                                           \
        MAREA_G_Multiply(left, right, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                        \
    }                                                                                                                                                                                                                                           \
    void MAREA_CALL MAREA_MathMultiplyMatrix##size##x##size##Vector##size##Sinteger32(const MAREA_MathMatrix##size##x##size##Sinteger32 *matrix, const MAREA_MathVector##size##Sinteger32 *vector, MAREA_MathVector##size##Sinteger32 *result)  \
    {                                                                                                                                                                                                                                           \
        MAREA_G_MultiplyVector(matrix, vector, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                               \
    }

MAREA_G_DEFINE_MATRIX(2)
MAREA_G_DEFINE_MATRIX(3)
MAREA_G_DEFINE_MATRIX(4)
