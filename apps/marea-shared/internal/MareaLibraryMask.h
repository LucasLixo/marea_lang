// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_SHARED_MAREALIBRARYMASK_H)
    #define MAREA_APPS_SHARED_MAREALIBRARYMASK_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Parses a comma-separated list of shipped-library names (the spelling
     * cmake/settings/ConfigureMareaFunctions.cmake's own MAREA_KNOWN_LIBRARIES list uses: "marea.array",
     * "marea.vector", "marea.tuple", "c.memory", "c.threads",
     * "marea.memory") into a MAREA_LibraryMask.
     * @param text The comma-separated list, NUL terminated. Null or empty means
     * MAREA_LIBRARY_MASK_DEFAULT (every shipped library on).
     * @param outMask Receives the parsed mask. Must not be null. Left unchanged on failure.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE when text names an unknown library or
     * outMask is null.
     */
    MAREA_Boolean MareaLibraryMaskParse(const MAREA_Character8 *text, MAREA_LibraryMask *outMask);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_SHARED_MAREALIBRARYMASK_H */
