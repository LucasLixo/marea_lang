// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHMATRIX2X2_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHMATRIX2X2_H

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

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHMATRIX2X2_H */
