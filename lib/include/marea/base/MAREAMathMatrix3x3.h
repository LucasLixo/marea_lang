// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHMATRIX3X3_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHMATRIX3X3_H

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

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHMATRIX3X3_H */
