// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaLibraryMask.h"

#include <string.h>

typedef struct MareaLibraryMaskEntry
{
    const char *name;
    MAREA_LibraryFlag flag;
} MareaLibraryMaskEntry;

/* TAG: one entry per MAREA_KNOWN_LIBRARIES name of cmake/settings/ConfigureMareaFunctions.cmake, kept in
 * step with it by hand, since the CMake list and this table name the same five shipped libraries. */
static const MareaLibraryMaskEntry MAREA_LIBRARY_MASK_TABLE[] = {
    { "marea.array", MAREA_LIBRARY_FLAG_MAREA_ARRAY },
    { "marea.vector", MAREA_LIBRARY_FLAG_MAREA_VECTOR },
    { "marea.tuple", MAREA_LIBRARY_FLAG_MAREA_TUPLE },
    { "c.memory", MAREA_LIBRARY_FLAG_C_MEMORY },
    { "c.threads", MAREA_LIBRARY_FLAG_C_THREADS },
};

MAREA_Boolean MareaLibraryMaskParse(const MAREA_Character8 *text, MAREA_LibraryMask *outMask)
{
    MAREA_IntegerSize cursor;
    MAREA_IntegerSize length;
    MAREA_LibraryMask mask;

    if (outMask == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (text == NULL || text[0] == '\0') {
        *outMask = MAREA_LIBRARY_MASK_DEFAULT;
        return MAREA_BOOLEAN_TRUE;
    }

    mask = MAREA_LIBRARY_FLAG_NONE;
    length = (MAREA_IntegerSize)strlen((const char *)text);
    cursor = 0;

    while (cursor < length) {
        MAREA_IntegerSize start;
        MAREA_IntegerSize tokenLength;
        MAREA_IntegerSize tableIndex;
        MAREA_Boolean matched;

        start = cursor;

        while (cursor < length && text[cursor] != ',') {
            cursor = cursor + 1;
        }

        tokenLength = cursor - start;
        matched = MAREA_BOOLEAN_FALSE;

        for (tableIndex = 0; tableIndex < MAREA_ARRAY_SIZE(MAREA_LIBRARY_MASK_TABLE); tableIndex = tableIndex + 1) {
            MAREA_IntegerSize nameLength = (MAREA_IntegerSize)strlen(MAREA_LIBRARY_MASK_TABLE[tableIndex].name);

            if (nameLength == tokenLength && strncmp((const char *)text + start, MAREA_LIBRARY_MASK_TABLE[tableIndex].name, tokenLength) == 0) {
                mask = mask | MAREA_LIBRARY_MASK_TABLE[tableIndex].flag;
                matched = MAREA_BOOLEAN_TRUE;
                break;
            }
        }

        if (!matched) {
            return MAREA_BOOLEAN_FALSE;
        }

        /* Skip the comma separator, when one ended this token. */
        if (cursor < length) {
            cursor = cursor + 1;
        }
    }

    *outMask = mask;
    return MAREA_BOOLEAN_TRUE;
}
