// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHMATRIX3X3_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHMATRIX3X3_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAMathVector3.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A matrix of three columns of three float32 lanes, 48 bytes aligned to 16.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix3x3Float32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathMatrix3x3Float32
    {
        MAREA_MathVector3Float32 column0;
        MAREA_MathVector3Float32 column1;
        MAREA_MathVector3Float32 column2;
    } MAREA_MathMatrix3x3Float32;

    /**
     * @brief A matrix of three columns of three sinteger32 lanes, 48 bytes aligned to 16.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix3x3Sinteger32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathMatrix3x3Sinteger32
    {
        MAREA_MathVector3Sinteger32 column0;
        MAREA_MathVector3Sinteger32 column1;
        MAREA_MathVector3Sinteger32 column2;
    } MAREA_MathMatrix3x3Sinteger32;

    /**
     * @brief A matrix of three columns of three boolean lanes, 12 bytes aligned to 4.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix3x3Boolean: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_MathMatrix3x3Boolean
    {
        MAREA_MathVector3Boolean column0;
        MAREA_MathVector3Boolean column1;
        MAREA_MathVector3Boolean column2;
    } MAREA_MathMatrix3x3Boolean;

    /**
     * @brief Gives the identity matrix of float32.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityMatrix3x3Float32(MAREA_MathMatrix3x3Float32 * result);

    /**
     * @brief Gives the transpose, whose columns are the rows of value.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTransposeMatrix3x3Float32(const MAREA_MathMatrix3x3Float32 *value, MAREA_MathMatrix3x3Float32 *result);

    /**
     * @brief Gives the matrix product left * right.
     * @param left Matrix to read. Must not be null.
     * @param right Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix3x3Float32(const MAREA_MathMatrix3x3Float32 *left, const MAREA_MathMatrix3x3Float32 *right, MAREA_MathMatrix3x3Float32 *result);

    /**
     * @brief Gives the product of a matrix and a column vector.
     * @param matrix Matrix to read. Must not be null.
     * @param vector Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix3x3Vector3Float32(const MAREA_MathMatrix3x3Float32 *matrix, const MAREA_MathVector3Float32 *vector, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the identity matrix of sinteger32.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityMatrix3x3Sinteger32(MAREA_MathMatrix3x3Sinteger32 * result);

    /**
     * @brief Gives the transpose, whose columns are the rows of value.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTransposeMatrix3x3Sinteger32(const MAREA_MathMatrix3x3Sinteger32 *value, MAREA_MathMatrix3x3Sinteger32 *result);

    /**
     * @brief Gives the matrix product left * right. The integers wrap.
     * @param left Matrix to read. Must not be null.
     * @param right Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix3x3Sinteger32(const MAREA_MathMatrix3x3Sinteger32 *left, const MAREA_MathMatrix3x3Sinteger32 *right, MAREA_MathMatrix3x3Sinteger32 *result);

    /**
     * @brief Gives the product of a matrix and a column vector. The integers wrap.
     * @param matrix Matrix to read. Must not be null.
     * @param vector Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix3x3Vector3Sinteger32(const MAREA_MathMatrix3x3Sinteger32 *matrix, const MAREA_MathVector3Sinteger32 *vector, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Gives the determinant.
     * @param value Matrix to read. Must not be null.
     * @return The determinant.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDeterminantMatrix3x3Float32(const MAREA_MathMatrix3x3Float32 *value);

    /**
     * @brief Gives the inverse. A matrix whose determinant is equal to zero gives the zero matrix.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathInverseMatrix3x3Float32(const MAREA_MathMatrix3x3Float32 *value, MAREA_MathMatrix3x3Float32 *result);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHMATRIX3X3_H */
