// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/bytecode/MAREABytecodeOpcode.h"

#include <string.h>

typedef struct MAREA_G_OpcodeRow
{
    const MAREA_Character8 *name;
    MAREA_Uinteger16 opcode;
} MAREA_G_OpcodeRow;

// INFO: a later slice adds its opcode as one more row here and one more value in the public enum.
static const MAREA_G_OpcodeRow MAREA_G_OPCODE_TABLE[] = {
    { .name = "nop", .opcode = MAREA_BYTECODE_OPCODE_NOP },
    { .name = "return_void", .opcode = MAREA_BYTECODE_OPCODE_RETURN_VOID },
};

const MAREA_Character8 *MAREA_CALL MAREA_BytecodeOpcodeGetName(MAREA_Uinteger16 opcode)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(MAREA_G_OPCODE_TABLE); index += MAREA_INTEGER_SIZE(1)) {
        if (MAREA_G_OPCODE_TABLE[index].opcode == opcode) {
            return MAREA_G_OPCODE_TABLE[index].name;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_Boolean MAREA_CALL MAREA_BytecodeOpcodeFromName(const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger16 *outOpcode)
{
    if (name == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(MAREA_G_OPCODE_TABLE); index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_G_OpcodeRow *row = &MAREA_G_OPCODE_TABLE[index];

        if ((strlen(row->name) == length) && (memcmp(row->name, name, length) == 0)) {
            if (outOpcode != MAREA_NULLPTR) {
                *outOpcode = row->opcode;
            }

            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}
