// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAMATHQUATERNION_H)
    #define MAREA_INCLUDE_BASE_MAREAMATHQUATERNION_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

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

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAMATHQUATERNION_H */
