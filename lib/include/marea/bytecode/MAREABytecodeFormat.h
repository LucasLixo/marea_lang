// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BYTECODE_MAREABYTECODEFORMAT_H)
    #define MAREA_INCLUDE_BYTECODE_MAREABYTECODEFORMAT_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/bytecode/MAREABytecode.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Major version of the binary format and of the text form. A reader rejects any other major version.
     */
    #if !defined(MAREA_BYTECODE_FORMAT_VERSION_MAJOR)
        #define MAREA_BYTECODE_FORMAT_VERSION_MAJOR MAREA_UINTEGER_16(0)
    #endif

    /**
     * @brief Minor version of the binary format and of the text form. A reader rejects a minor version that is
     * higher than its own.
     */
    #if !defined(MAREA_BYTECODE_FORMAT_VERSION_MINOR)
        #define MAREA_BYTECODE_FORMAT_VERSION_MINOR MAREA_UINTEGER_16(1)
    #endif

    /**
     * @brief Writes the canonical binary form of a module: little endian, fixed width, no padding, a fixed section
     * order. The same module always gives the same bytes.
     * @param module Module to write. Must not be null.
     * @param outBytes Receives the bytes, valid for the arena lifetime. Must not be null.
     * @param outSize Receives the number of bytes. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeSerialize(const MAREA_BytecodeModule *module, const MAREA_Byte **outBytes, MAREA_IntegerSize *outSize);

    /**
     * @brief Reads a module from its canonical binary form. A malformed input is a diagnostic in the sink of the
     * context, with the byte offset in the position, and the out module is null. Nothing crashes on any input.
     * @param context Context that owns the module. Must not be null.
     * @param bytes Bytes to read. Must not be null when size is not 0.
     * @param size Number of bytes.
     * @param outModule Receives the module, or null when the input is malformed. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS (also for a malformed input), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER
     * when an argument is null, or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeDeserialize(MAREA_BytecodeContext * context, const MAREA_Byte *bytes, MAREA_IntegerSize size, MAREA_BytecodeModule **outModule);

    /**
     * @brief Writes the canonical text form of a module: ASCII, one line for each item, without a comment.
     * @param module Module to write. Must not be null.
     * @param outText Receives the text with a NUL after the last character, valid for the arena lifetime. Must not
     * be null.
     * @param outLength Receives the number of characters, without the NUL. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS, MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER when an argument is null, or
     * MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeDisassemble(const MAREA_BytecodeModule *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength);

    /**
     * @brief Reads a module from its text form. A comment starts with a semicolon and runs to the end of the line.
     * A malformed input is a diagnostic in the sink of the context, with the line and the column in the position,
     * and the out module is null. Nothing crashes on any input.
     * @param context Context that owns the module. Must not be null.
     * @param text Text to read. It does not need a terminating NUL. Must not be null when length is not 0.
     * @param length Number of characters.
     * @param outModule Receives the module, or null when the input is malformed. Must not be null.
     * @return MAREA_BYTECODE_EXCEPT_SUCCESS (also for a malformed input), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER
     * when an argument is null, or MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeAssemble(MAREA_BytecodeContext * context, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_BytecodeModule **outModule);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BYTECODE_MAREABYTECODEFORMAT_H */
