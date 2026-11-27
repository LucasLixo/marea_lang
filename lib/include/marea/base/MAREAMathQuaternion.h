// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHQUATERNION_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHQUATERNION_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAMathMatrix4x4.h"
    #include "marea/base/MAREAMathVector3.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A quaternion of four float32 lanes, 16 bytes aligned to 16.
     * x, y and z are the vector part, w is the scalar part. Open, because it is the layout of
     * marea.math.QuaternionFloat32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathQuaternionFloat32
    {
        MAREA_Float32 x;
        MAREA_Float32 y;
        MAREA_Float32 z;
        MAREA_Float32 w;
    } MAREA_MathQuaternionFloat32;

    /**
     * @brief Gives the identity quaternion, which turns nothing.
     * @param result Receives the quaternion. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathIdentityQuaternionFloat32(MAREA_MathQuaternionFloat32 * result);

    /**
     * @brief Gives the quaternion of a turn by angle about axis.
     * @param axis Axis of the turn, of any length. Must not be null.
     * @param angle Angle in radians.
     * @param result Receives the quaternion. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathFromAxisAngleQuaternionFloat32(const MAREA_MathVector3Float32 *axis, MAREA_Float32 angle, MAREA_MathQuaternionFloat32 *result);

    /**
     * @brief Gives the quaternion of the Euler angles: yaw about z, then pitch about y, then roll about x.
     * @param yaw Angle about z in radians.
     * @param pitch Angle about y in radians.
     * @param roll Angle about x in radians.
     * @param result Receives the quaternion. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathFromEulerQuaternionFloat32(MAREA_Float32 yaw, MAREA_Float32 pitch, MAREA_Float32 roll, MAREA_MathQuaternionFloat32 * result);

    /**
     * @brief Gives the rotation matrix of a quaternion of length one.
     * @param value Quaternion to read. Must not be null.
     * @param result Receives the matrix, with zero in each lane that it does not set. Must not be null.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathToMatrix4x4QuaternionFloat32(const MAREA_MathQuaternionFloat32 *value, MAREA_MathMatrix4x4Float32 *result);

    /**
     * @brief Gives the Hamilton product left * right.
     * @param left Quaternion to read. Must not be null.
     * @param right Quaternion to read. Must not be null.
     * @param result Receives the quaternion. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_MathQuaternionFloat32 *result);

    /**
     * @brief Gives the conjugate, which has the opposite vector part.
     * @param value Quaternion to read. Must not be null.
     * @param result Receives the quaternion. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathConjugateQuaternionFloat32(const MAREA_MathQuaternionFloat32 *value, MAREA_MathQuaternionFloat32 *result);

    /**
     * @brief Gives the quaternion of length one. A quaternion of length zero gives the identity.
     * @param value Quaternion to read. Must not be null.
     * @param result Receives the quaternion. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathNormalizeQuaternionFloat32(const MAREA_MathQuaternionFloat32 *value, MAREA_MathQuaternionFloat32 *result);

    /**
     * @brief Gives the dot product, the sum of the products from x to w.
     * @param left Quaternion to read. Must not be null.
     * @param right Quaternion to read. Must not be null.
     * @return The dot product.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDotQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right);

    /**
     * @brief Gives the length, the square root of the dot product with itself.
     * @param value Quaternion to read. Must not be null.
     * @return The length.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathLengthQuaternionFloat32(const MAREA_MathQuaternionFloat32 *value);

    /**
     * @brief Gives the normalized linear interpolation.
     * @param left Quaternion to read. Must not be null.
     * @param right Quaternion to read. Must not be null.
     * @param amount Place between left (0) and right (1). It is not clamped.
     * @param result Receives the quaternion. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMixQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_Float32 amount, MAREA_MathQuaternionFloat32 *result);

    /**
     * @brief Gives the spherical interpolation along the shortest arc. Ends that are very near use the linear one.
     * @param left Quaternion to read. Must not be null.
     * @param right Quaternion to read. Must not be null.
     * @param amount Place between left (0) and right (1). It is not clamped.
     * @param result Receives the quaternion. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathSphericalMixQuaternionFloat32(const MAREA_MathQuaternionFloat32 *left, const MAREA_MathQuaternionFloat32 *right, MAREA_Float32 amount, MAREA_MathQuaternionFloat32 *result);

    /**
     * @brief Turns a vector by a quaternion of length one.
     * @param rotation Quaternion to read. Must not be null.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in the padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathRotateVector3QuaternionFloat32(const MAREA_MathQuaternionFloat32 *rotation, const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHQUATERNION_H */
