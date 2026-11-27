// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHMATRIX4X4_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHMATRIX4X4_H

    #include "marea/base/MAREAMacros.h"
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

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHMATRIX4X4_H */
