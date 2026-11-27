// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHVECTOR3_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHVECTOR3_H

    #include "marea/base/MAREACTypes.h"
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

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHVECTOR3_H */
