// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_SHARED_MAREAFILEIO_H)
    #define MAREA_APPS_SHARED_MAREAFILEIO_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAArena.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Reads the whole content of path into the arena, NUL terminated.
     * @param arena Arena that owns the returned buffer. Must not be null.
     * @param path NUL-terminated file path. Must not be null.
     * @param outData Receives the read, NUL-terminated buffer. Must not be null.
     * @param outLength Receives the byte count, not counting the terminator. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE when the file cannot be opened or read,
     * or an argument is null.
     */
    MAREA_Boolean MareaFileIoReadAll(MAREA_Arena *arena, const char *path, const MAREA_Character8 **outData, MAREA_IntegerSize *outLength);

    /**
     * @brief Writes length bytes of data to path, overwriting any existing file.
     * @param path NUL-terminated file path. Must not be null.
     * @param data Bytes to write. Must not be null when length is not 0.
     * @param length Byte count to write.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE when the file cannot be created or
     * written, or path is null.
     */
    MAREA_Boolean MareaFileIoWriteAll(const char *path, const void *data, MAREA_IntegerSize length);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_SHARED_MAREAFILEIO_H */
