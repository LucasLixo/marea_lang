// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHMATRIX4X4_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHMATRIX4X4_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAMathVector3.h"
    #include "marea/base/MAREAMathVector4.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A matrix of four columns of four float32 lanes, 64 bytes aligned to 16.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix4x4Float32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathMatrix4x4Float32
    {
        MAREA_MathVector4Float32 column0;
        MAREA_MathVector4Float32 column1;
        MAREA_MathVector4Float32 column2;
        MAREA_MathVector4Float32 column3;
    } MAREA_MathMatrix4x4Float32;

    /**
     * @brief A matrix of four columns of four sinteger32 lanes, 64 bytes aligned to 16.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix4x4Sinteger32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathMatrix4x4Sinteger32
    {
        MAREA_MathVector4Sinteger32 column0;
        MAREA_MathVector4Sinteger32 column1;
        MAREA_MathVector4Sinteger32 column2;
        MAREA_MathVector4Sinteger32 column3;
    } MAREA_MathMatrix4x4Sinteger32;

    /**
     * @brief A matrix of four columns of four boolean lanes, 16 bytes aligned to 4.
     * The matrix is column major, and the padding lanes of each column hold zero. Open, because it is the layout
     * of marea.math.Matrix4x4Boolean: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_MathMatrix4x4Boolean
    {
        MAREA_MathVector4Boolean column0;
        MAREA_MathVector4Boolean column1;
        MAREA_MathVector4Boolean column2;
        MAREA_MathVector4Boolean column3;
    } MAREA_MathMatrix4x4Boolean;

    /**
     * @brief Gives the identity matrix of float32.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityMatrix4x4Float32(MAREA_MathMatrix4x4Float32 * result);

    /**
     * @brief Gives the transpose, whose columns are the rows of value.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTransposeMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives the matrix product left * right.
     * @param left Matrix to read. Must not be null.
     * @param right Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *left, const MAREA_MathMatrix4x4Float32 *right, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives the product of a matrix and a column vector.
     * @param matrix Matrix to read. Must not be null.
     * @param vector Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Vector4Float32(const MAREA_MathMatrix4x4Float32 *matrix, const MAREA_MathVector4Float32 *vector, MAREA_MathVector4Float32 *result);

    /**
     * @brief Gives the identity matrix of sinteger32.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityMatrix4x4Sinteger32(MAREA_MathMatrix4x4Sinteger32 * result);

    /**
     * @brief Gives the transpose, whose columns are the rows of value.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTransposeMatrix4x4Sinteger32(const MAREA_MathMatrix4x4Sinteger32 *value, MAREA_MathMatrix4x4Sinteger32 *result);

    /**
     * @brief Gives the matrix product left * right. The integers wrap.
     * @param left Matrix to read. Must not be null.
     * @param right Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Sinteger32(const MAREA_MathMatrix4x4Sinteger32 *left, const MAREA_MathMatrix4x4Sinteger32 *right, MAREA_MathMatrix4x4Sinteger32 *result);

    /**
     * @brief Gives the product of a matrix and a column vector. The integers wrap.
     * @param matrix Matrix to read. Must not be null.
     * @param vector Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyMatrix4x4Vector4Sinteger32(const MAREA_MathMatrix4x4Sinteger32 *matrix, const MAREA_MathVector4Sinteger32 *vector, MAREA_MathVector4Sinteger32 *result);

    /**
     * @brief Gives the determinant.
     * @param value Matrix to read. Must not be null.
     * @return The determinant.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDeterminantMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value);

    /**
     * @brief Gives the inverse. A matrix whose determinant is equal to zero gives the zero matrix.
     * @param value Matrix to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathInverseMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives value * T, where T moves a point by offset.
     * @param value Matrix to read. Must not be null.
     * @param offset Offset to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTranslateMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, const MAREA_MathVector3Float32 *offset, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives value * S, where S scales each axis by its factor.
     * @param value Matrix to read. Must not be null.
     * @param factor Factors to read. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathScaleMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, const MAREA_MathVector3Float32 *factor, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives value * R, where R turns by angle about axis.
     * @param value Matrix to read. Must not be null.
     * @param angle Angle in radians.
     * @param axis Axis of the turn, of any length. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathRotateMatrix4x4Float32(const MAREA_MathMatrix4x4Float32 *value, MAREA_Float32 angle, const MAREA_MathVector3Float32 *axis, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives the right-handed view matrix of an eye that looks at a point.
     * @param eye Position of the eye. Must not be null.
     * @param center Point to look at. Must not be null.
     * @param up Upward direction. Must not be null.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathLookAtMatrix4x4Float32(const MAREA_MathVector3Float32 *eye, const MAREA_MathVector3Float32 *center, const MAREA_MathVector3Float32 *up, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives the right-handed perspective projection, with depth from zero to one. Equal planes give infinite lanes.
     * @param fieldOfViewY Vertical field of view in radians.
     * @param aspect Width over height.
     * @param nearPlane Distance of the near plane.
     * @param farPlane Distance of the far plane.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathPerspectiveMatrix4x4Float32(MAREA_Float32 fieldOfViewY, MAREA_Float32 aspect, MAREA_Float32 nearPlane, MAREA_Float32 farPlane, MAREA_MathMatrix4x4Float32 * result);

    /**
     * @brief Gives the right-handed orthographic projection of a box, with depth from zero to one.
     * @param left Left limit.
     * @param right Right limit.
     * @param bottom Lower limit.
     * @param top Upper limit.
     * @param nearPlane Distance of the near plane.
     * @param farPlane Distance of the far plane.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathOrthographicMatrix4x4Float32(MAREA_Float32 left, MAREA_Float32 right, MAREA_Float32 bottom, MAREA_Float32 top, MAREA_Float32 nearPlane, MAREA_Float32 farPlane, MAREA_MathMatrix4x4Float32 * result);

    /**
     * @brief Gives the right-handed perspective projection of a volume that need not be centered, with depth from zero to one.
     * @param left Left limit.
     * @param right Right limit.
     * @param bottom Lower limit.
     * @param top Upper limit.
     * @param nearPlane Distance of the near plane.
     * @param farPlane Distance of the far plane.
     * @param result Receives the matrix, with zero in each padding lane. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathFrustumMatrix4x4Float32(MAREA_Float32 left, MAREA_Float32 right, MAREA_Float32 bottom, MAREA_Float32 top, MAREA_Float32 nearPlane, MAREA_Float32 farPlane, MAREA_MathMatrix4x4Float32 * result);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHMATRIX4X4_H */
