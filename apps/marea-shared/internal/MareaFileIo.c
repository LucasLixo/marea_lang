// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaFileIo.h"

#include <stdio.h>

MAREA_Boolean MareaFileIoReadAll(MAREA_Arena *arena, const char *path, const MAREA_Character8 **outData, MAREA_IntegerSize *outLength)
{
    FILE *file;
    long size;
    MAREA_Null memory;

    if (arena == NULL || path == NULL || outData == NULL || outLength == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    file = fopen(path, "rb");

    if (file == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return MAREA_BOOLEAN_FALSE;
    }

    size = ftell(file);

    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_ArenaAllocate(arena, (MAREA_IntegerSize)size + 1, 1, &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        fclose(file);
        return MAREA_BOOLEAN_FALSE;
    }

    if (size > 0 && fread(memory, 1, (size_t)size, file) != (size_t)size) {
        fclose(file);
        return MAREA_BOOLEAN_FALSE;
    }

    fclose(file);
    ((char *)memory)[size] = '\0';

    *outData = (const MAREA_Character8 *)memory;
    *outLength = (MAREA_IntegerSize)size;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MareaFileIoWriteAll(const char *path, const void *data, MAREA_IntegerSize length)
{
    FILE *file;

    if (path == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    file = fopen(path, "wb");

    if (file == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (length > 0 && fwrite(data, 1, (size_t)length, file) != (size_t)length) {
        fclose(file);
        return MAREA_BOOLEAN_FALSE;
    }

    fclose(file);
    return MAREA_BOOLEAN_TRUE;
}
