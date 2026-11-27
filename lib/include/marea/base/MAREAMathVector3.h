// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHVECTOR3_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHVECTOR3_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAHostContext.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A vector of three float32 lanes, 16 bytes aligned to 16.
     * The padding lanes hold zero. Open, because it is the layout of
     * marea.math.Vector3Float32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathVector3Float32
    {
        MAREA_Float32 x;
        MAREA_Float32 y;
        MAREA_Float32 z;
        MAREA_Float32 padding[1];
    } MAREA_MathVector3Float32;

    /**
     * @brief A vector of three sinteger32 lanes, 16 bytes aligned to 16.
     * The padding lanes hold zero. Open, because it is the layout of
     * marea.math.Vector3Sinteger32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathVector3Sinteger32
    {
        MAREA_Sinteger32 x;
        MAREA_Sinteger32 y;
        MAREA_Sinteger32 z;
        MAREA_Sinteger32 padding[1];
    } MAREA_MathVector3Sinteger32;

    /**
     * @brief A vector of three boolean lanes, 4 bytes aligned to 4.
     * The padding lanes hold zero. Open, because it is the layout of
     * marea.math.Vector3Boolean: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_MathVector3Boolean
    {
        MAREA_Boolean x;
        MAREA_Boolean y;
        MAREA_Boolean z;
        MAREA_Boolean padding[1];
    } MAREA_MathVector3Boolean;

    /**
     * @brief Adds two vectors lane by lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAddVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Subtracts right from left lane by lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathSubtractVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Multiplies two vectors lane by lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Divides left by right lane by lane. A lane that has no number as its quotient (0 / 0) is the positive NaN.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathDivideVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Multiplies each lane by one factor.
     * @param value Vector to read. Must not be null.
     * @param factor Factor of each lane.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathScaleVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_Float32 factor, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the dot product, the sum of the products from the first lane to the last.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @return The dot product.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDotVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right);

    /**
     * @brief Gives the length, the square root of the dot product of the vector with itself.
     * @param value Vector to read. Must not be null.
     * @return The length, infinity when the square overflows.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathLengthVector3Float32(const MAREA_MathVector3Float32 *value);

    /**
     * @brief Gives the distance between two points.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @return The length of left minus right.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Float32 MAREA_CALL MAREA_MathDistanceVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right);

    /**
     * @brief Gives the vector of length one with the same direction. A vector of length zero gives zero.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathNormalizeVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives left + (right - left) * amount in each lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param amount Place between left (0) and right (1). It is not clamped.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMixVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_Float32 amount, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the smaller value of each lane: left when left < right, else right.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMinimumVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the larger value of each lane: left when left > right, else right.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMaximumVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives low when value < low, else high when value > high, else value, in each lane.
     * @param value Vector to read. Must not be null.
     * @param low Vector to read. Must not be null.
     * @param high Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathClampVector3Float32(const MAREA_MathVector3Float32 *value, const MAREA_MathVector3Float32 *low, const MAREA_MathVector3Float32 *high, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the absolute value of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAbsoluteVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the sine of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathSineVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the cosine of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathCosineVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the tangent of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathTangentVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the arc sine of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathArcSineVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the arc cosine of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathArcCosineVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the arc tangent of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathArcTangentVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the angle in radians of an angle in degrees of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathRadiansVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the angle in degrees of an angle in radians of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathDegreesVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the natural exponential of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathExponentialVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the natural logarithm of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathLogarithmVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the square root of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathSquareRootVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the inverse of the square root (zero for a zero lane) of each lane.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathInverseSquareRootVector3Float32(const MAREA_MathVector3Float32 *value, MAREA_MathVector3Float32 *result);

    /**
     * @brief Raises each lane of left to the lane of right.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathPowerVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Gives the cross product.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathCrossVector3Float32(const MAREA_MathVector3Float32 *left, const MAREA_MathVector3Float32 *right, MAREA_MathVector3Float32 *result);

    /**
     * @brief Adds two vectors lane by lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAddVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Subtracts right from left lane by lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathSubtractVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Multiplies two vectors lane by lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMultiplyVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Divides left by right lane by lane, toward zero. The smallest value divided by -1 wraps to itself. A zero divisor in a lane raises MAREA_HOST_TRAP_DIVISION_BY_ZERO, as the division of the language does, and the result is then zero.
     * @param context Context of the call, which receives the trap. Null raises nothing.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathDivideVector3Sinteger32(MAREA_HostContext * context, const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Multiplies each lane by one factor. The product wraps.
     * @param value Vector to read. Must not be null.
     * @param factor Factor of each lane.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathScaleVector3Sinteger32(const MAREA_MathVector3Sinteger32 *value, MAREA_Sinteger32 factor, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Gives the dot product. The products and the sum wrap.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @return The dot product.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Sinteger32 MAREA_CALL MAREA_MathDotVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right);

    /**
     * @brief Gives the smaller value of each lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMinimumVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Gives the larger value of each lane.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathMaximumVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Gives low when value < low, else high when value > high, else value, in each lane.
     * @param value Vector to read. Must not be null.
     * @param low Vector to read. Must not be null.
     * @param high Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathClampVector3Sinteger32(const MAREA_MathVector3Sinteger32 *value, const MAREA_MathVector3Sinteger32 *low, const MAREA_MathVector3Sinteger32 *high, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Gives the absolute value of each lane. The smallest value stays itself.
     * @param value Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathAbsoluteVector3Sinteger32(const MAREA_MathVector3Sinteger32 *value, MAREA_MathVector3Sinteger32 *result);

    /**
     * @brief Gives the cross product. The products and the differences wrap.
     * @param left Vector to read. Must not be null.
     * @param right Vector to read. Must not be null.
     * @param result Receives the vector, with zero in each padding lane. Must not be null. May be an input.
     */
    MAREA_EXTERN_DECL void MAREA_CALL MAREA_MathCrossVector3Sinteger32(const MAREA_MathVector3Sinteger32 *left, const MAREA_MathVector3Sinteger32 *right, MAREA_MathVector3Sinteger32 *result);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHVECTOR3_H */
