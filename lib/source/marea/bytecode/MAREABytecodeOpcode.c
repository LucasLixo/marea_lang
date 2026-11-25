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
    { .name = "move", .opcode = MAREA_BYTECODE_OPCODE_MOVE },
    { .name = "load_imm", .opcode = MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE },
    { .name = "load_data", .opcode = MAREA_BYTECODE_OPCODE_LOAD_DATA },
    { .name = "add", .opcode = MAREA_BYTECODE_OPCODE_ADD },
    { .name = "sub", .opcode = MAREA_BYTECODE_OPCODE_SUB },
    { .name = "mul", .opcode = MAREA_BYTECODE_OPCODE_MUL },
    { .name = "div", .opcode = MAREA_BYTECODE_OPCODE_DIV },
    { .name = "rem", .opcode = MAREA_BYTECODE_OPCODE_REM },
    { .name = "pow", .opcode = MAREA_BYTECODE_OPCODE_POW },
    { .name = "sat_add", .opcode = MAREA_BYTECODE_OPCODE_SATURATING_ADD },
    { .name = "sat_sub", .opcode = MAREA_BYTECODE_OPCODE_SATURATING_SUB },
    { .name = "sat_mul", .opcode = MAREA_BYTECODE_OPCODE_SATURATING_MUL },
    { .name = "shl", .opcode = MAREA_BYTECODE_OPCODE_SHIFT_LEFT },
    { .name = "shr", .opcode = MAREA_BYTECODE_OPCODE_SHIFT_RIGHT },
    { .name = "neg", .opcode = MAREA_BYTECODE_OPCODE_NEGATE },
    { .name = "bit_not", .opcode = MAREA_BYTECODE_OPCODE_BIT_NOT },
    { .name = "convert", .opcode = MAREA_BYTECODE_OPCODE_CONVERT },
    { .name = "bit_and", .opcode = MAREA_BYTECODE_OPCODE_BIT_AND },
    { .name = "bit_or", .opcode = MAREA_BYTECODE_OPCODE_BIT_OR },
    { .name = "bit_xor", .opcode = MAREA_BYTECODE_OPCODE_BIT_XOR },
    { .name = "not", .opcode = MAREA_BYTECODE_OPCODE_NOT },
    { .name = "eq", .opcode = MAREA_BYTECODE_OPCODE_EQUAL },
    { .name = "ne", .opcode = MAREA_BYTECODE_OPCODE_NOT_EQUAL },
    { .name = "lt_s", .opcode = MAREA_BYTECODE_OPCODE_LESS_SIGNED },
    { .name = "le_s", .opcode = MAREA_BYTECODE_OPCODE_LESS_EQUAL_SIGNED },
    { .name = "lt_u", .opcode = MAREA_BYTECODE_OPCODE_LESS_UNSIGNED },
    { .name = "le_u", .opcode = MAREA_BYTECODE_OPCODE_LESS_EQUAL_UNSIGNED },
    { .name = "eq_f", .opcode = MAREA_BYTECODE_OPCODE_EQUAL_FLOAT },
    { .name = "ne_f", .opcode = MAREA_BYTECODE_OPCODE_NOT_EQUAL_FLOAT },
    { .name = "lt_f", .opcode = MAREA_BYTECODE_OPCODE_LESS_FLOAT },
    { .name = "le_f", .opcode = MAREA_BYTECODE_OPCODE_LESS_EQUAL_FLOAT },
    { .name = "jump", .opcode = MAREA_BYTECODE_OPCODE_JUMP },
    { .name = "jump_if", .opcode = MAREA_BYTECODE_OPCODE_JUMP_IF },
    { .name = "jump_if_not", .opcode = MAREA_BYTECODE_OPCODE_JUMP_IF_NOT },
    { .name = "call", .opcode = MAREA_BYTECODE_OPCODE_CALL },
    { .name = "return", .opcode = MAREA_BYTECODE_OPCODE_RETURN },
    { .name = "trap", .opcode = MAREA_BYTECODE_OPCODE_TRAP },
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
