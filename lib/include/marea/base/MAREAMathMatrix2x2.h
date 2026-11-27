// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHMATRIX2X2_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHMATRIX2X2_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAMathVector2.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A matrix of two columns of two float32 lanes, 32 bytes aligned to 16.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix2x2Float32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathMatrix2x2Float32
    {
        MAREA_MathVector2Float32 column0;
        MAREA_MathVector2Float32 column1;
    } MAREA_MathMatrix2x2Float32;

    /**
     * @brief A matrix of two columns of two sinteger32 lanes, 32 bytes aligned to 16.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix2x2Sinteger32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathMatrix2x2Sinteger32
    {
        MAREA_MathVector2Sinteger32 column0;
        MAREA_MathVector2Sinteger32 column1;
    } MAREA_MathMatrix2x2Sinteger32;

    /**
     * @brief A matrix of two columns of two boolean lanes, 8 bytes aligned to 4.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix2x2Boolean: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_MathMatrix2x2Boolean
    {
        MAREA_MathVector2Boolean column0;
        MAREA_MathVector2Boolean column1;
    } MAREA_MathMatrix2x2Boolean;

    /**
     * @brief Gives the identity matrix of float32.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityMatrix2x2Float32(MAREA_MathMatrix2x2Float32 * result);

    /**
     * @brief Gives the transpose, whose columns are the rows of value.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTransposeMatrix2x2Float32(const MAREA_MathMatrix2x2Float32 *value, MAREA_MathMatrix2x2Float32 *result);

    /**
     * @brief Gives the matrix product left * right.
     * @param left Matrix to read. Must not be null.
     * @param right Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix2x2Float32(const MAREA_MathMatrix2x2Float32 *left, const MAREA_MathMatrix2x2Float32 *right, MAREA_MathMatrix2x2Float32 *result);

    /**
     * @brief Gives the product of a matrix and a column vector.
     * @param matrix Matrix to read. Must not be null.
     * @param vector Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix2x2Vector2Float32(const MAREA_MathMatrix2x2Float32 *matrix, const MAREA_MathVector2Float32 *vector, MAREA_MathVector2Float32 *result);

    /**
     * @brief Gives the identity matrix of sinteger32.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityMatrix2x2Sinteger32(MAREA_MathMatrix2x2Sinteger32 * result);

    /**
     * @brief Gives the transpose, whose columns are the rows of value.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTransposeMatrix2x2Sinteger32(const MAREA_MathMatrix2x2Sinteger32 *value, MAREA_MathMatrix2x2Sinteger32 *result);

    /**
     * @brief Gives the matrix product left * right. The integers wrap.
     * @param left Matrix to read. Must not be null.
     * @param right Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix2x2Sinteger32(const MAREA_MathMatrix2x2Sinteger32 *left, const MAREA_MathMatrix2x2Sinteger32 *right, MAREA_MathMatrix2x2Sinteger32 *result);

    /**
     * @brief Gives the product of a matrix and a column vector. The integers wrap.
     * @param matrix Matrix to read. Must not be null.
     * @param vector Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix2x2Vector2Sinteger32(const MAREA_MathMatrix2x2Sinteger32 *matrix, const MAREA_MathVector2Sinteger32 *vector, MAREA_MathVector2Sinteger32 *result);

    /**
     * @brief Gives the determinant.
     * @param value Matrix to read. Must not be null.
     * @return The determinant.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDeterminantMatrix2x2Float32(const MAREA_MathMatrix2x2Float32 *value);

    /**
     * @brief Gives the inverse. A matrix whose determinant is equal to zero gives the zero matrix.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathInverseMatrix2x2Float32(const MAREA_MathMatrix2x2Float32 *value, MAREA_MathMatrix2x2Float32 *result);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHMATRIX2X2_H */
