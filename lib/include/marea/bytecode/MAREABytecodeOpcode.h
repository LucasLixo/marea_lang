// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BYTECODE_MAREABYTECODEOPCODE_H)
    #define MAREA_INCLUDE_BYTECODE_MAREABYTECODEOPCODE_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Opcode of one 64-bit instruction. A register holds 64 bits with no tag, so the opcode, and the kind
     * operand of a typed opcode, say how to read it. A value that is narrower than 64 bits is always kept
     * normalized: sign extended for a signed integer, zero extended for an unsigned one, and a float32 as a
     * float64 value that float32 can hold. A boolean is 0 or 1.
     *
     * The operands a, b and c are, for each opcode:
     * - nop: none. return_void: none, the call gives no value. return: a is the register of the value.
     * - move: a is the destination, b the source. load_imm: a is the destination, b and c are the low and the high
     *   16 bits of a signed 32-bit value, sign extended. load_data: a is the destination, b is the index of an
     *   8-byte blob of the data section, read as the 64 bits.
     * - add, sub, mul, div, rem, pow, sat_add, sat_sub, sat_mul, shl, shr: a is the left operand and the
     *   destination, b is the right operand, c is a MAREA_BytecodeNumberKind that says how to read both and how
     *   to normalize the result. A signed or unsigned integer add, sub, mul and neg wrap. div and rem trap on a
     *   zero divisor, and a signed division of the lowest value by -1 wraps. pow traps on a negative exponent. A
     *   shift traps when the count, read as an unsigned 64-bit value, is not less than the width of the kind. The
     *   sat_ opcodes clamp to the limits of the kind, and are the plain operation on a float.
     * - neg, bit_not: a is the destination, b the source, c the kind. not: a is the destination, b the source,
     *   read as a boolean.
     * - convert: a is the destination, b the source, c holds the kind of the source in the low 8 bits and the
     *   kind of the target in the high 8 bits. A conversion that does not fit the target traps.
     * - bit_and, bit_or, bit_xor, eq, ne, lt_s, le_s, lt_u, le_u, eq_f, ne_f, lt_f, le_f: a is the destination, b
     *   and c are the two sources. A comparison gives a boolean. The suffixes mean signed, unsigned and float.
     * - jump: b is a signed 16-bit offset in instructions, relative to the next instruction. jump_if and
     *   jump_if_not: a is the register that holds the condition, b is the offset.
     * - call: a is the first register of a window of 1 + c registers, b is the index of the function, c is the
     *   number of arguments. The arguments are in a + 1 up, and they become the first registers of the new
     *   frame. The result comes back in a.
     * - trap: a is a MAREA_BytecodeTrapCode.
     */
    typedef enum MAREA_BytecodeOpcode
    {
        MAREA_BYTECODE_OPCODE_NOP = MAREA_UINTEGER_16(0),
        MAREA_BYTECODE_OPCODE_RETURN_VOID = MAREA_UINTEGER_16(1),
        MAREA_BYTECODE_OPCODE_MOVE = MAREA_UINTEGER_16(2),
        MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE = MAREA_UINTEGER_16(3),
        MAREA_BYTECODE_OPCODE_LOAD_DATA = MAREA_UINTEGER_16(4),
        MAREA_BYTECODE_OPCODE_ADD = MAREA_UINTEGER_16(5),
        MAREA_BYTECODE_OPCODE_SUB = MAREA_UINTEGER_16(6),
        MAREA_BYTECODE_OPCODE_MUL = MAREA_UINTEGER_16(7),
        MAREA_BYTECODE_OPCODE_DIV = MAREA_UINTEGER_16(8),
        MAREA_BYTECODE_OPCODE_REM = MAREA_UINTEGER_16(9),
        MAREA_BYTECODE_OPCODE_POW = MAREA_UINTEGER_16(10),
        MAREA_BYTECODE_OPCODE_SATURATING_ADD = MAREA_UINTEGER_16(11),
        MAREA_BYTECODE_OPCODE_SATURATING_SUB = MAREA_UINTEGER_16(12),
        MAREA_BYTECODE_OPCODE_SATURATING_MUL = MAREA_UINTEGER_16(13),
        MAREA_BYTECODE_OPCODE_SHIFT_LEFT = MAREA_UINTEGER_16(14),
        MAREA_BYTECODE_OPCODE_SHIFT_RIGHT = MAREA_UINTEGER_16(15),
        MAREA_BYTECODE_OPCODE_NEGATE = MAREA_UINTEGER_16(16),
        MAREA_BYTECODE_OPCODE_BIT_NOT = MAREA_UINTEGER_16(17),
        MAREA_BYTECODE_OPCODE_CONVERT = MAREA_UINTEGER_16(18),
        MAREA_BYTECODE_OPCODE_BIT_AND = MAREA_UINTEGER_16(19),
        MAREA_BYTECODE_OPCODE_BIT_OR = MAREA_UINTEGER_16(20),
        MAREA_BYTECODE_OPCODE_BIT_XOR = MAREA_UINTEGER_16(21),
        MAREA_BYTECODE_OPCODE_NOT = MAREA_UINTEGER_16(22),
        MAREA_BYTECODE_OPCODE_EQUAL = MAREA_UINTEGER_16(23),
        MAREA_BYTECODE_OPCODE_NOT_EQUAL = MAREA_UINTEGER_16(24),
        MAREA_BYTECODE_OPCODE_LESS_SIGNED = MAREA_UINTEGER_16(25),
        MAREA_BYTECODE_OPCODE_LESS_EQUAL_SIGNED = MAREA_UINTEGER_16(26),
        MAREA_BYTECODE_OPCODE_LESS_UNSIGNED = MAREA_UINTEGER_16(27),
        MAREA_BYTECODE_OPCODE_LESS_EQUAL_UNSIGNED = MAREA_UINTEGER_16(28),
        MAREA_BYTECODE_OPCODE_EQUAL_FLOAT = MAREA_UINTEGER_16(29),
        MAREA_BYTECODE_OPCODE_NOT_EQUAL_FLOAT = MAREA_UINTEGER_16(30),
        MAREA_BYTECODE_OPCODE_LESS_FLOAT = MAREA_UINTEGER_16(31),
        MAREA_BYTECODE_OPCODE_LESS_EQUAL_FLOAT = MAREA_UINTEGER_16(32),
        MAREA_BYTECODE_OPCODE_JUMP = MAREA_UINTEGER_16(33),
        MAREA_BYTECODE_OPCODE_JUMP_IF = MAREA_UINTEGER_16(34),
        MAREA_BYTECODE_OPCODE_JUMP_IF_NOT = MAREA_UINTEGER_16(35),
        MAREA_BYTECODE_OPCODE_CALL = MAREA_UINTEGER_16(36),
        MAREA_BYTECODE_OPCODE_RETURN = MAREA_UINTEGER_16(37),
        MAREA_BYTECODE_OPCODE_TRAP = MAREA_UINTEGER_16(38),
        MAREA_BYTECODE_OPCODE_COUNT = MAREA_UINTEGER_16(39),
    } MAREA_BytecodeOpcode;

    /**
     * @brief How a register is read by a typed opcode. Four widths of signed and of unsigned integer, then the two
     * floats. A boolean needs no kind.
     */
    typedef enum MAREA_BytecodeNumberKind
    {
        MAREA_BYTECODE_NUMBER_KIND_SINTEGER_8 = MAREA_UINTEGER_8(0),
        MAREA_BYTECODE_NUMBER_KIND_SINTEGER_16 = MAREA_UINTEGER_8(1),
        MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32 = MAREA_UINTEGER_8(2),
        MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64 = MAREA_UINTEGER_8(3),
        MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 = MAREA_UINTEGER_8(4),
        MAREA_BYTECODE_NUMBER_KIND_UINTEGER_16 = MAREA_UINTEGER_8(5),
        MAREA_BYTECODE_NUMBER_KIND_UINTEGER_32 = MAREA_UINTEGER_8(6),
        MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64 = MAREA_UINTEGER_8(7),
        MAREA_BYTECODE_NUMBER_KIND_FLOAT_32 = MAREA_UINTEGER_8(8),
        MAREA_BYTECODE_NUMBER_KIND_FLOAT_64 = MAREA_UINTEGER_8(9),
        MAREA_BYTECODE_NUMBER_KIND_COUNT = MAREA_UINTEGER_8(10),
    } MAREA_BytecodeNumberKind;

    /**
     * @brief Why a run stops with no value. The operand of the trap opcode is one of these, and so is the reason
     * of a trap that an opcode raises itself. The same code is the same fault in every backend.
     */
    typedef enum MAREA_BytecodeTrapCode
    {
        MAREA_BYTECODE_TRAP_CODE_NONE = MAREA_UINTEGER_8(0),
        MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO = MAREA_UINTEGER_8(1),
        MAREA_BYTECODE_TRAP_CODE_SHIFT_OUT_OF_RANGE = MAREA_UINTEGER_8(2),
        MAREA_BYTECODE_TRAP_CODE_NEGATIVE_EXPONENT = MAREA_UINTEGER_8(3),
        MAREA_BYTECODE_TRAP_CODE_CONVERT_OUT_OF_RANGE = MAREA_UINTEGER_8(4),
        MAREA_BYTECODE_TRAP_CODE_UNREACHABLE = MAREA_UINTEGER_8(5),
        MAREA_BYTECODE_TRAP_CODE_MISSING_RETURN = MAREA_UINTEGER_8(6),
        MAREA_BYTECODE_TRAP_CODE_STACK_OVERFLOW = MAREA_UINTEGER_8(7),
        MAREA_BYTECODE_TRAP_CODE_COUNT = MAREA_UINTEGER_8(8),
    } MAREA_BytecodeTrapCode;

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
