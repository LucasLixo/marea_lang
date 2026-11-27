// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHVECTOR4_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHVECTOR4_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A vector of four float32 lanes, 16 bytes aligned to 16.
     * Open, because it is the layout of marea.math.Vector4Float32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathVector4Float32
    {
        MAREA_Float32 x;
        MAREA_Float32 y;
        MAREA_Float32 z;
        MAREA_Float32 w;
    } MAREA_MathVector4Float32;

    /**
     * @brief A vector of four sinteger32 lanes, 16 bytes aligned to 16.
     * Open, because it is the layout of marea.math.Vector4Sinteger32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathVector4Sinteger32
    {
        MAREA_Sinteger32 x;
        MAREA_Sinteger32 y;
        MAREA_Sinteger32 z;
        MAREA_Sinteger32 w;
    } MAREA_MathVector4Sinteger32;

    /**
     * @brief A vector of four boolean lanes, 4 bytes aligned to 4.
     * Open, because it is the layout of marea.math.Vector4Boolean: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_MathVector4Boolean
    {
        MAREA_Boolean x;
        MAREA_Boolean y;
        MAREA_Boolean z;
        MAREA_Boolean w;
    } MAREA_MathVector4Boolean;

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHVECTOR4_H */
