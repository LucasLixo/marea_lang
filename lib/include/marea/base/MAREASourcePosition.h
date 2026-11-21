// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREASOURCEPOSITION_H)
    #define MAREA_INCLUDE_BASE_MAREASOURCEPOSITION_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Position of a span of source text, passed by value. The only public struct of base and
     * lex: later stages pass it into a diagnostic sink and read it back by value.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_SourcePosition
    {
        const MAREA_Character8 *file;
        MAREA_Uinteger32 offset;
        MAREA_Uinteger32 line;
        MAREA_Uinteger32 column;
        MAREA_Uinteger32 length;
    } MAREA_SourcePosition;

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREASOURCEPOSITION_H */
