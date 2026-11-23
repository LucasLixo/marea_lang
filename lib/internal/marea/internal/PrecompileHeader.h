// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_MAREA_INTERNAL_PRECOMPILE_HEADER_H)
    #define MAREA_INTERNAL_MAREA_INTERNAL_PRECOMPILE_HEADER_H

    // The headers nearly every translation unit needs. Angle brackets on purpose: this file
    // uses no symbol itself, so a check of unused quoted includes would report every entry.
    #if defined(SETTING_PRECOMPILE_HEADER)
        #include <marea/base/MAREAMacros.h> // NOLINT
        #include <marea/base/MAREACTypes.h> // NOLINT
    #endif

#endif // MAREA_INTERNAL_MAREA_INTERNAL_PRECOMPILE_HEADER_H
