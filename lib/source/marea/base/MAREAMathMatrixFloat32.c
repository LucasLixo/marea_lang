// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAMathMatrix2x2.h"
#include "marea/base/MAREAMathMatrix3x3.h"
#include "marea/base/MAREAMathMatrix4x4.h"
#include "marea/base/MAREAMathVector2.h"
#include "marea/base/MAREAMathVector3.h"
#include "marea/base/MAREAMathVector4.h"

#include <math.h>
#include <string.h>

// INFO: each product is rounded by itself, so a result does not depend on whether the processor has a fused multiply and add.
#if defined(__clang__)
    #pragma STDC FP_CONTRACT OFF
#endif

#define MAREA_G_LANE_COUNT       4
#define MAREA_G_MATRIX_LANES     16
#define MAREA_G_AT(column, row)  (((column) * MAREA_G_LANE_COUNT) + (row))
#define MAREA_G_E(column, row)   (m.lane[MAREA_G_AT(column, row)])
#define MAREA_G_OUT(column, row) (output.lane[MAREA_G_AT(column, row)])

// INFO: the bytes of a matrix as four columns of four lanes. A lane past the size of the matrix is padding and stays zero.
typedef struct MAREA_ALIGNED(4) MAREA_G_Matrix
{
    MAREA_Float32 lane[MAREA_G_MATRIX_LANES];
} MAREA_G_Matrix;

typedef struct MAREA_ALIGNED(4) MAREA_G_Column
{
    MAREA_Float32 lane[MAREA_G_LANE_COUNT];
} MAREA_G_Column;

MAREA_STATIC_ASSERT((sizeof(MAREA_G_Matrix) == sizeof(MAREA_MathMatrix4x4Float32)) && (sizeof(MAREA_G_Column) == sizeof(MAREA_MathVector4Float32)), "A matrix of float32 is columns of four lanes:");

static MAREA_G_Matrix MAREA_CALL MAREA_G_Load(const void *value, MAREA_IntegerSize size)
{
    MAREA_G_Matrix matrix = { 0 };

    memcpy(&matrix, value, (size * sizeof(MAREA_G_Column)));

    return matrix;
}

static void MAREA_CALL MAREA_G_Store(const MAREA_G_Matrix *matrix, MAREA_Null result, MAREA_IntegerSize size)
{
    memcpy(result, matrix, (size * sizeof(MAREA_G_Column)));
}

static void MAREA_CALL MAREA_G_Identity(MAREA_Null result, MAREA_IntegerSize size)
{
    MAREA_G_Matrix output = { 0 };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < size; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_G_OUT(index, index) = MAREA_FLOAT_32(1.0);
    }

    MAREA_G_Store(&output, result, size);
}

static void MAREA_CALL MAREA_G_Transpose(const void *value, MAREA_Null result, MAREA_IntegerSize size)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, size);

    MAREA_G_Matrix output = { 0 };

    for (MAREA_IntegerSize column = MAREA_INTEGER_SIZE(0); column < size; column = (column + MAREA_INTEGER_SIZE(1))) {
        for (MAREA_IntegerSize row = MAREA_INTEGER_SIZE(0); row < size; row = (row + MAREA_INTEGER_SIZE(1))) {
            MAREA_G_OUT(column, row) = MAREA_G_E(row, column);
        }
    }

    MAREA_G_Store(&output, result, size);
}

// INFO: the sum goes from the first column to the last. output must not be vector.
static void MAREA_CALL MAREA_G_Transform(const MAREA_G_Matrix *matrix, const MAREA_Float32 *MAREA_RESTRICT vector, MAREA_Float32 *MAREA_RESTRICT output, MAREA_IntegerSize size)
{
    for (MAREA_IntegerSize row = MAREA_INTEGER_SIZE(0); row < size; row = (row + MAREA_INTEGER_SIZE(1))) {
        MAREA_Float32 sum = (matrix->lane[row] * vector[0]);

        for (MAREA_IntegerSize column = MAREA_INTEGER_SIZE(1); column < size; column = (column + MAREA_INTEGER_SIZE(1))) {
            sum = (sum + (matrix->lane[MAREA_G_AT(column, row)] * vector[column]));
        }

        output[row] = sum;
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

    MAREA_G_Store(&output, result, size);
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
#define MAREA_G_DEFINE_MATRIX(size)                                                                                                                                                                                                 \
    void MAREA_CALL MAREA_MathIdentityMatrix##size##x##size##Float32(MAREA_MathMatrix##size##x##size##Float32 *result)                                                                                                              \
    {                                                                                                                                                                                                                               \
        MAREA_G_Identity(result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                         \
    }                                                                                                                                                                                                                               \
    void MAREA_CALL MAREA_MathTransposeMatrix##size##x##size##Float32(const MAREA_MathMatrix##size##x##size##Float32 *value, MAREA_MathMatrix##size##x##size##Float32 *result)                                                      \
    {                                                                                                                                                                                                                               \
        MAREA_G_Transpose(value, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                                 \
    }                                                                                                                                                                                                                               \
    void MAREA_CALL MAREA_MathMultiplyMatrix##size##x##size##Float32(const MAREA_MathMatrix##size##x##size##Float32 *left, const MAREA_MathMatrix##size##x##size##Float32 *right, MAREA_MathMatrix##size##x##size##Float32 *result) \
    {                                                                                                                                                                                                                               \
        MAREA_G_Multiply(left, right, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                            \
    }                                                                                                                                                                                                                               \
    void MAREA_CALL MAREA_MathMultiplyMatrix##size##x##size##Vector##size##Float32(const MAREA_MathMatrix##size##x##size##Float32 *matrix, const MAREA_MathVector##size##Float32 *vector, MAREA_MathVector##size##Float32 *result)  \
    {                                                                                                                                                                                                                               \
        MAREA_G_MultiplyVector(matrix, vector, result, MAREA_INTEGER_SIZE(size));                                                                                                                                                   \
    }

MAREA_G_DEFINE_MATRIX(2)
MAREA_G_DEFINE_MATRIX(3)
MAREA_G_DEFINE_MATRIX(4)

MAREA_Float32 MAREA_CALL MAREA_MathDeterminantMatrix2x2Float32(const MAREA_MathMatrix2x2Float32 *value)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, MAREA_INTEGER_SIZE(2));

    return ((MAREA_G_E(0, 0) * MAREA_G_E(1, 1)) - (MAREA_G_E(1, 0) * MAREA_G_E(0, 1)));
}

MAREA_Float32 MAREA_CALL MAREA_MathDeterminantMatrix3x3Float32(const MAREA_MathMatrix3x3Float32 *value)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, MAREA_INTEGER_SIZE(3));

    const MAREA_Float32 A = (MAREA_G_E(0, 0) * ((MAREA_G_E(1, 1) * MAREA_G_E(2, 2)) - (MAREA_G_E(2, 1) * MAREA_G_E(1, 2))));
    const MAREA_Float32 B = (MAREA_G_E(1, 0) * ((MAREA_G_E(0, 1) * MAREA_G_E(2, 2)) - (MAREA_G_E(2, 1) * MAREA_G_E(0, 2))));
    const MAREA_Float32 C = (MAREA_G_E(2, 0) * ((MAREA_G_E(0, 1) * MAREA_G_E(1, 2)) - (MAREA_G_E(1, 1) * MAREA_G_E(0, 2))));

    return ((A - B) + C);
}

MAREA_Float32 MAREA_CALL MAREA_MathDeterminantMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, MAREA_INTEGER_SIZE(4));

    const MAREA_Float32 S0 = ((MAREA_G_E(2, 2) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 2) * MAREA_G_E(2, 3)));
    const MAREA_Float32 S1 = ((MAREA_G_E(2, 1) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 1) * MAREA_G_E(2, 3)));
    const MAREA_Float32 S2 = ((MAREA_G_E(2, 1) * MAREA_G_E(3, 2)) - (MAREA_G_E(3, 1) * MAREA_G_E(2, 2)));
    const MAREA_Float32 S3 = ((MAREA_G_E(2, 0) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 0) * MAREA_G_E(2, 3)));
    const MAREA_Float32 S4 = ((MAREA_G_E(2, 0) * MAREA_G_E(3, 2)) - (MAREA_G_E(3, 0) * MAREA_G_E(2, 2)));
    const MAREA_Float32 S5 = ((MAREA_G_E(2, 0) * MAREA_G_E(3, 1)) - (MAREA_G_E(3, 0) * MAREA_G_E(2, 1)));

    const MAREA_Float32 C0 = (((MAREA_G_E(1, 1) * S0) - (MAREA_G_E(1, 2) * S1)) + (MAREA_G_E(1, 3) * S2));
    const MAREA_Float32 C1 = (((MAREA_G_E(1, 0) * S0) - (MAREA_G_E(1, 2) * S3)) + (MAREA_G_E(1, 3) * S4));
    const MAREA_Float32 C2 = (((MAREA_G_E(1, 0) * S1) - (MAREA_G_E(1, 1) * S3)) + (MAREA_G_E(1, 3) * S5));
    const MAREA_Float32 C3 = (((MAREA_G_E(1, 0) * S2) - (MAREA_G_E(1, 1) * S4)) + (MAREA_G_E(1, 2) * S5));

    return ((((MAREA_G_E(0, 0) * C0) - (MAREA_G_E(0, 1) * C1)) + (MAREA_G_E(0, 2) * C2)) - (MAREA_G_E(0, 3) * C3));
}

// INFO: a determinant that is equal to zero gives the zero matrix. A NaN determinant is not zero, so the NaN stays.
void MAREA_CALL MAREA_MathInverseMatrix2x2Float32(const MAREA_MathMatrix2x2Float32 *value, MAREA_MathMatrix2x2Float32 *result)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, MAREA_INTEGER_SIZE(2));

    const MAREA_Float32 DETERMINANT = MAREA_MathDeterminantMatrix2x2Float32(value);

    MAREA_G_Matrix output = { 0 };

    if (DETERMINANT != MAREA_FLOAT_32(0.0)) {
        const MAREA_Float32 RECIPROCAL = (MAREA_FLOAT_32(1.0) / DETERMINANT);

        MAREA_G_OUT(0, 0) = (MAREA_G_E(1, 1) * RECIPROCAL);
        MAREA_G_OUT(0, 1) = ((-MAREA_G_E(0, 1)) * RECIPROCAL);
        MAREA_G_OUT(1, 0) = ((-MAREA_G_E(1, 0)) * RECIPROCAL);
        MAREA_G_OUT(1, 1) = (MAREA_G_E(0, 0) * RECIPROCAL);
    }

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(2));
}

void MAREA_CALL MAREA_MathInverseMatrix3x3Float32(const MAREA_MathMatrix3x3Float32 *value, MAREA_MathMatrix3x3Float32 *result)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, MAREA_INTEGER_SIZE(3));

    const MAREA_Float32 DETERMINANT = MAREA_MathDeterminantMatrix3x3Float32(value);

    MAREA_G_Matrix output = { 0 };

    if (DETERMINANT != MAREA_FLOAT_32(0.0)) {
        const MAREA_Float32 RECIPROCAL = (MAREA_FLOAT_32(1.0) / DETERMINANT);

        MAREA_G_OUT(0, 0) = (((MAREA_G_E(1, 1) * MAREA_G_E(2, 2)) - (MAREA_G_E(2, 1) * MAREA_G_E(1, 2))) * RECIPROCAL);
        MAREA_G_OUT(0, 1) = (((MAREA_G_E(2, 1) * MAREA_G_E(0, 2)) - (MAREA_G_E(0, 1) * MAREA_G_E(2, 2))) * RECIPROCAL);
        MAREA_G_OUT(0, 2) = (((MAREA_G_E(0, 1) * MAREA_G_E(1, 2)) - (MAREA_G_E(1, 1) * MAREA_G_E(0, 2))) * RECIPROCAL);
        MAREA_G_OUT(1, 0) = (((MAREA_G_E(2, 0) * MAREA_G_E(1, 2)) - (MAREA_G_E(1, 0) * MAREA_G_E(2, 2))) * RECIPROCAL);
        MAREA_G_OUT(1, 1) = (((MAREA_G_E(0, 0) * MAREA_G_E(2, 2)) - (MAREA_G_E(2, 0) * MAREA_G_E(0, 2))) * RECIPROCAL);
        MAREA_G_OUT(1, 2) = (((MAREA_G_E(1, 0) * MAREA_G_E(0, 2)) - (MAREA_G_E(0, 0) * MAREA_G_E(1, 2))) * RECIPROCAL);
        MAREA_G_OUT(2, 0) = (((MAREA_G_E(1, 0) * MAREA_G_E(2, 1)) - (MAREA_G_E(2, 0) * MAREA_G_E(1, 1))) * RECIPROCAL);
        MAREA_G_OUT(2, 1) = (((MAREA_G_E(2, 0) * MAREA_G_E(0, 1)) - (MAREA_G_E(0, 0) * MAREA_G_E(2, 1))) * RECIPROCAL);
        MAREA_G_OUT(2, 2) = (((MAREA_G_E(0, 0) * MAREA_G_E(1, 1)) - (MAREA_G_E(1, 0) * MAREA_G_E(0, 1))) * RECIPROCAL);
    }

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(3));
}

void MAREA_CALL MAREA_MathInverseMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, MAREA_MathMatrix4x4Float32 *result)
{
    const MAREA_G_Matrix m = MAREA_G_Load(value, MAREA_INTEGER_SIZE(4));

    const MAREA_Float32 C00 = ((MAREA_G_E(2, 2) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 2) * MAREA_G_E(2, 3)));
    const MAREA_Float32 C02 = ((MAREA_G_E(1, 2) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 2) * MAREA_G_E(1, 3)));
    const MAREA_Float32 C03 = ((MAREA_G_E(1, 2) * MAREA_G_E(2, 3)) - (MAREA_G_E(2, 2) * MAREA_G_E(1, 3)));
    const MAREA_Float32 C04 = ((MAREA_G_E(2, 1) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 1) * MAREA_G_E(2, 3)));
    const MAREA_Float32 C06 = ((MAREA_G_E(1, 1) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 1) * MAREA_G_E(1, 3)));
    const MAREA_Float32 C07 = ((MAREA_G_E(1, 1) * MAREA_G_E(2, 3)) - (MAREA_G_E(2, 1) * MAREA_G_E(1, 3)));
    const MAREA_Float32 C08 = ((MAREA_G_E(2, 1) * MAREA_G_E(3, 2)) - (MAREA_G_E(3, 1) * MAREA_G_E(2, 2)));
    const MAREA_Float32 C10 = ((MAREA_G_E(1, 1) * MAREA_G_E(3, 2)) - (MAREA_G_E(3, 1) * MAREA_G_E(1, 2)));
    const MAREA_Float32 C11 = ((MAREA_G_E(1, 1) * MAREA_G_E(2, 2)) - (MAREA_G_E(2, 1) * MAREA_G_E(1, 2)));
    const MAREA_Float32 C12 = ((MAREA_G_E(2, 0) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 0) * MAREA_G_E(2, 3)));
    const MAREA_Float32 C14 = ((MAREA_G_E(1, 0) * MAREA_G_E(3, 3)) - (MAREA_G_E(3, 0) * MAREA_G_E(1, 3)));
    const MAREA_Float32 C15 = ((MAREA_G_E(1, 0) * MAREA_G_E(2, 3)) - (MAREA_G_E(2, 0) * MAREA_G_E(1, 3)));
    const MAREA_Float32 C16 = ((MAREA_G_E(2, 0) * MAREA_G_E(3, 2)) - (MAREA_G_E(3, 0) * MAREA_G_E(2, 2)));
    const MAREA_Float32 C18 = ((MAREA_G_E(1, 0) * MAREA_G_E(3, 2)) - (MAREA_G_E(3, 0) * MAREA_G_E(1, 2)));
    const MAREA_Float32 C19 = ((MAREA_G_E(1, 0) * MAREA_G_E(2, 2)) - (MAREA_G_E(2, 0) * MAREA_G_E(1, 2)));
    const MAREA_Float32 C20 = ((MAREA_G_E(2, 0) * MAREA_G_E(3, 1)) - (MAREA_G_E(3, 0) * MAREA_G_E(2, 1)));
    const MAREA_Float32 C22 = ((MAREA_G_E(1, 0) * MAREA_G_E(3, 1)) - (MAREA_G_E(3, 0) * MAREA_G_E(1, 1)));
    const MAREA_Float32 C23 = ((MAREA_G_E(1, 0) * MAREA_G_E(2, 1)) - (MAREA_G_E(2, 0) * MAREA_G_E(1, 1)));

    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = (((MAREA_G_E(1, 1) * C00) - (MAREA_G_E(1, 2) * C04)) + (MAREA_G_E(1, 3) * C08));
    MAREA_G_OUT(0, 1) = (-(((MAREA_G_E(0, 1) * C00) - (MAREA_G_E(0, 2) * C04)) + (MAREA_G_E(0, 3) * C08)));
    MAREA_G_OUT(0, 2) = (((MAREA_G_E(0, 1) * C02) - (MAREA_G_E(0, 2) * C06)) + (MAREA_G_E(0, 3) * C10));
    MAREA_G_OUT(0, 3) = (-(((MAREA_G_E(0, 1) * C03) - (MAREA_G_E(0, 2) * C07)) + (MAREA_G_E(0, 3) * C11)));

    MAREA_G_OUT(1, 0) = (-(((MAREA_G_E(1, 0) * C00) - (MAREA_G_E(1, 2) * C12)) + (MAREA_G_E(1, 3) * C16)));
    MAREA_G_OUT(1, 1) = (((MAREA_G_E(0, 0) * C00) - (MAREA_G_E(0, 2) * C12)) + (MAREA_G_E(0, 3) * C16));
    MAREA_G_OUT(1, 2) = (-(((MAREA_G_E(0, 0) * C02) - (MAREA_G_E(0, 2) * C14)) + (MAREA_G_E(0, 3) * C18)));
    MAREA_G_OUT(1, 3) = (((MAREA_G_E(0, 0) * C03) - (MAREA_G_E(0, 2) * C15)) + (MAREA_G_E(0, 3) * C19));

    MAREA_G_OUT(2, 0) = (((MAREA_G_E(1, 0) * C04) - (MAREA_G_E(1, 1) * C12)) + (MAREA_G_E(1, 3) * C20));
    MAREA_G_OUT(2, 1) = (-(((MAREA_G_E(0, 0) * C04) - (MAREA_G_E(0, 1) * C12)) + (MAREA_G_E(0, 3) * C20)));
    MAREA_G_OUT(2, 2) = (((MAREA_G_E(0, 0) * C06) - (MAREA_G_E(0, 1) * C14)) + (MAREA_G_E(0, 3) * C22));
    MAREA_G_OUT(2, 3) = (-(((MAREA_G_E(0, 0) * C07) - (MAREA_G_E(0, 1) * C15)) + (MAREA_G_E(0, 3) * C23)));

    MAREA_G_OUT(3, 0) = (-(((MAREA_G_E(1, 0) * C08) - (MAREA_G_E(1, 1) * C16)) + (MAREA_G_E(1, 2) * C20)));
    MAREA_G_OUT(3, 1) = (((MAREA_G_E(0, 0) * C08) - (MAREA_G_E(0, 1) * C16)) + (MAREA_G_E(0, 2) * C20));
    MAREA_G_OUT(3, 2) = (-(((MAREA_G_E(0, 0) * C10) - (MAREA_G_E(0, 1) * C18)) + (MAREA_G_E(0, 2) * C22)));
    MAREA_G_OUT(3, 3) = (((MAREA_G_E(0, 0) * C11) - (MAREA_G_E(0, 1) * C19)) + (MAREA_G_E(0, 2) * C23));

    const MAREA_Float32 DETERMINANT = ((((MAREA_G_E(0, 0) * MAREA_G_OUT(0, 0)) + (MAREA_G_E(0, 1) * MAREA_G_OUT(1, 0))) + (MAREA_G_E(0, 2) * MAREA_G_OUT(2, 0))) + (MAREA_G_E(0, 3) * MAREA_G_OUT(3, 0)));

    if (DETERMINANT == MAREA_FLOAT_32(0.0)) {
        memset(&output, 0, sizeof(output));

    } else {
        const MAREA_Float32 RECIPROCAL = (MAREA_FLOAT_32(1.0) / DETERMINANT);

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_INTEGER_SIZE(16); index = (index + MAREA_INTEGER_SIZE(1))) {
            output.lane[index] = (output.lane[index] * RECIPROCAL);
        }
    }

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(4));
}

void MAREA_CALL MAREA_MathTranslateMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, const MAREA_MathVector3Float32 *offset, MAREA_MathMatrix4x4Float32 *result)
{
    MAREA_MathMatrix4x4Float32 transform;

    MAREA_MathIdentityMatrix4x4Float32(&transform);

    transform.column3.x = offset->x;
    transform.column3.y = offset->y;
    transform.column3.z = offset->z;

    MAREA_MathMultiplyMatrix4x4Float32(value, &transform, result);
}

void MAREA_CALL MAREA_MathScaleMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, const MAREA_MathVector3Float32 *factor, MAREA_MathMatrix4x4Float32 *result)
{
    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = factor->x;
    MAREA_G_OUT(1, 1) = factor->y;
    MAREA_G_OUT(2, 2) = factor->z;
    MAREA_G_OUT(3, 3) = MAREA_FLOAT_32(1.0);

    MAREA_G_Multiply(value, &output, result, MAREA_INTEGER_SIZE(4));
}

void MAREA_CALL MAREA_MathRotateMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, MAREA_Float32 angle, const MAREA_MathVector3Float32 *axis, MAREA_MathMatrix4x4Float32 *result)
{
    MAREA_MathVector3Float32 a;

    MAREA_MathNormalizeVector3Float32(axis, &a);

    const MAREA_Float32 C = cosf(angle);
    const MAREA_Float32 S = sinf(angle);
    const MAREA_Float32 OC = (MAREA_FLOAT_32(1.0) - C);

    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = (C + ((a.x * a.x) * OC));
    MAREA_G_OUT(0, 1) = (((a.y * a.x) * OC) + (a.z * S));
    MAREA_G_OUT(0, 2) = (((a.z * a.x) * OC) - (a.y * S));
    MAREA_G_OUT(1, 0) = (((a.x * a.y) * OC) - (a.z * S));
    MAREA_G_OUT(1, 1) = (C + ((a.y * a.y) * OC));
    MAREA_G_OUT(1, 2) = (((a.z * a.y) * OC) + (a.x * S));
    MAREA_G_OUT(2, 0) = (((a.x * a.z) * OC) + (a.y * S));
    MAREA_G_OUT(2, 1) = (((a.y * a.z) * OC) - (a.x * S));
    MAREA_G_OUT(2, 2) = (C + ((a.z * a.z) * OC));
    MAREA_G_OUT(3, 3) = MAREA_FLOAT_32(1.0);

    MAREA_G_Multiply(value, &output, result, MAREA_INTEGER_SIZE(4));
}

void MAREA_CALL MAREA_MathLookAtMatrix4x4Float32(const MAREA_MathVector3Float32 *eye, const MAREA_MathVector3Float32 *center, const MAREA_MathVector3Float32 *up, MAREA_MathMatrix4x4Float32 *result)
{
    MAREA_MathVector3Float32 forward;
    MAREA_MathVector3Float32 side;
    MAREA_MathVector3Float32 upward;

    MAREA_MathSubtractVector3Float32(center, eye, &forward);
    MAREA_MathNormalizeVector3Float32(&forward, &forward);
    MAREA_MathCrossVector3Float32(&forward, up, &side);
    MAREA_MathNormalizeVector3Float32(&side, &side);
    MAREA_MathCrossVector3Float32(&side, &forward, &upward);

    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = side.x;
    MAREA_G_OUT(0, 1) = upward.x;
    MAREA_G_OUT(0, 2) = (-forward.x);
    MAREA_G_OUT(1, 0) = side.y;
    MAREA_G_OUT(1, 1) = upward.y;
    MAREA_G_OUT(1, 2) = (-forward.y);
    MAREA_G_OUT(2, 0) = side.z;
    MAREA_G_OUT(2, 1) = upward.z;
    MAREA_G_OUT(2, 2) = (-forward.z);
    MAREA_G_OUT(3, 0) = (-MAREA_MathDotVector3Float32(&side, eye));
    MAREA_G_OUT(3, 1) = (-MAREA_MathDotVector3Float32(&upward, eye));
    MAREA_G_OUT(3, 2) = MAREA_MathDotVector3Float32(&forward, eye);
    MAREA_G_OUT(3, 3) = MAREA_FLOAT_32(1.0);

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(4));
}

void MAREA_CALL MAREA_MathPerspectiveMatrix4x4Float32(MAREA_Float32 fieldOfViewY, MAREA_Float32 aspect, MAREA_Float32 nearPlane, MAREA_Float32 farPlane, MAREA_MathMatrix4x4Float32 *result)
{
    const MAREA_Float32 HALF_TANGENT = tanf(fieldOfViewY / MAREA_FLOAT_32(2.0));

    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = (MAREA_FLOAT_32(1.0) / (aspect * HALF_TANGENT));
    MAREA_G_OUT(1, 1) = (MAREA_FLOAT_32(1.0) / HALF_TANGENT);
    MAREA_G_OUT(2, 2) = (farPlane / (nearPlane - farPlane));
    MAREA_G_OUT(2, 3) = MAREA_FLOAT_32(-1.0);
    MAREA_G_OUT(3, 2) = (-((farPlane * nearPlane) / (farPlane - nearPlane)));

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(4));
}

void MAREA_CALL MAREA_MathOrthographicMatrix4x4Float32(MAREA_Float32 left, MAREA_Float32 right, MAREA_Float32 bottom, MAREA_Float32 top, MAREA_Float32 nearPlane, MAREA_Float32 farPlane, MAREA_MathMatrix4x4Float32 *result)
{
    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = (MAREA_FLOAT_32(2.0) / (right - left));
    MAREA_G_OUT(1, 1) = (MAREA_FLOAT_32(2.0) / (top - bottom));
    MAREA_G_OUT(2, 2) = (-(MAREA_FLOAT_32(1.0) / (farPlane - nearPlane)));
    MAREA_G_OUT(3, 0) = (-((right + left) / (right - left)));
    MAREA_G_OUT(3, 1) = (-((top + bottom) / (top - bottom)));
    MAREA_G_OUT(3, 2) = (-(nearPlane / (farPlane - nearPlane)));
    MAREA_G_OUT(3, 3) = MAREA_FLOAT_32(1.0);

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(4));
}

void MAREA_CALL MAREA_MathFrustumMatrix4x4Float32(MAREA_Float32 left, MAREA_Float32 right, MAREA_Float32 bottom, MAREA_Float32 top, MAREA_Float32 nearPlane, MAREA_Float32 farPlane, MAREA_MathMatrix4x4Float32 *result)
{
    MAREA_G_Matrix output = { 0 };

    MAREA_G_OUT(0, 0) = ((MAREA_FLOAT_32(2.0) * nearPlane) / (right - left));
    MAREA_G_OUT(1, 1) = ((MAREA_FLOAT_32(2.0) * nearPlane) / (top - bottom));
    MAREA_G_OUT(2, 0) = ((right + left) / (right - left));
    MAREA_G_OUT(2, 1) = ((top + bottom) / (top - bottom));
    MAREA_G_OUT(2, 2) = (farPlane / (nearPlane - farPlane));
    MAREA_G_OUT(2, 3) = MAREA_FLOAT_32(-1.0);
    MAREA_G_OUT(3, 2) = (-((farPlane * nearPlane) / (farPlane - nearPlane)));

    MAREA_G_Store(&output, result, MAREA_INTEGER_SIZE(4));
}
