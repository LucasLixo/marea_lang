// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHVECTOR2_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHVECTOR2_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief A vector of two float32 lanes, 16 bytes aligned to 16.
     * The padding lanes hold zero. Open, because it is the layout of
     * marea.math.Vector2Float32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathVector2Float32
    {
        MAREA_Float32 x;
        MAREA_Float32 y;
        MAREA_Float32 padding[2];
    } MAREA_MathVector2Float32;

    /**
     * @brief A vector of two sinteger32 lanes, 16 bytes aligned to 16.
     * The padding lanes hold zero. Open, because it is the layout of
     * marea.math.Vector2Sinteger32: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(16) MAREA_MathVector2Sinteger32
    {
        MAREA_Sinteger32 x;
        MAREA_Sinteger32 y;
        MAREA_Sinteger32 padding[2];
    } MAREA_MathVector2Sinteger32;

    /**
     * @brief A vector of two boolean lanes, 4 bytes aligned to 4.
     * The padding lanes hold zero. Open, because it is the layout of
     * marea.math.Vector2Boolean: a C caller and the language read the same bytes.
     */
    typedef struct MAREA_ALIGNED(4) MAREA_MathVector2Boolean
    {
        MAREA_Boolean x;
        MAREA_Boolean y;
        MAREA_Boolean padding[2];
    } MAREA_MathVector2Boolean;

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHVECTOR2_H */
