// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_MAREAVERSION_H)
    #define MAREA_INCLUDE_MAREAVERSION_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

    #define MAREA_VERSION_MAJOR 0
    #define MAREA_VERSION_MINOR 1
    #define MAREA_VERSION_PATCH 0

    #define MAREA_VERSION                    \
        MAREA_STRINGIFY(MAREA_VERSION_MAJOR) \
        "." MAREA_STRINGIFY(MAREA_VERSION_MINOR) "." MAREA_STRINGIFY(MAREA_VERSION_PATCH)

MAREA_BEGIN_EXTERN_C

    const MAREA_Character8 *MAREA_GetVersion(void);

MAREA_END_EXTERN_C

#endif
