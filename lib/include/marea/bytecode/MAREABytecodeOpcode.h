// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BYTECODE_MAREABYTECODEOPCODE_H)
    #define MAREA_INCLUDE_BYTECODE_MAREABYTECODEOPCODE_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Opcode of one 64-bit instruction. The table holds only what the skeleton of the format needs. A
     * later slice adds a value at the end and one row in the name table, and nothing else changes.
     */
    typedef enum MAREA_BytecodeOpcode
    {
        MAREA_BYTECODE_OPCODE_NOP = MAREA_UINTEGER_16(0),
        MAREA_BYTECODE_OPCODE_RETURN_VOID = MAREA_UINTEGER_16(1),
    } MAREA_BytecodeOpcode;

    /**
     * @brief Returns the text name of an opcode, the spelling that the text form uses.
     * @param opcode Opcode to query.
     * @return A NUL-terminated name with static storage, or null when the opcode is not in the table.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_Character8 *MAREA_CALL MAREA_BytecodeOpcodeGetName(MAREA_Uinteger16 opcode);

    /**
     * @brief Finds the opcode that has the given text name.
     * @param name Name to look up. It does not need a terminating NUL. Null gives MAREA_BOOLEAN_FALSE.
     * @param length Number of characters of name.
     * @param outOpcode Receives the opcode when the name is known. Null is accepted and nothing is stored.
     * @return MAREA_BOOLEAN_TRUE when the name is in the table, otherwise MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_BytecodeOpcodeFromName(const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger16 *outOpcode);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BYTECODE_MAREABYTECODEOPCODE_H */
