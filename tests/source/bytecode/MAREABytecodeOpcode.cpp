// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <string>
#include <vector>

TEST(MAREABytecodeOpcode, GetNameGivesTheSpelling)
{
    GTEST_COUT << "Validating MAREA_BytecodeOpcodeGetName gives the text name of each opcode." << std::endl;

    ASSERT_NE(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_NOP), nullptr) << "nop must have a name.";

    EXPECT_STREQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_NOP), "nop") << "The name of NOP must be nop.";

    ASSERT_NE(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_RETURN_VOID), nullptr) << "return_void must have a name.";

    EXPECT_STREQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_RETURN_VOID), "return_void") << "The name of RETURN_VOID must be return_void.";

    EXPECT_EQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_COUNT), nullptr) << "The first value after the table has no name.";

    EXPECT_EQ(MAREA_BytecodeOpcodeGetName(65535), nullptr) << "The last 16-bit value has no name.";
}

TEST(MAREABytecodeOpcode, FromNameFindsEveryOpcode)
{
    GTEST_COUT << "Validating MAREA_BytecodeOpcodeFromName finds an opcode by its name and rejects the rest." << std::endl;

    MAREA_Uinteger16 opcode{ 99 };

    EXPECT_TRUE(MAREA_BytecodeOpcodeFromName("nop", 3, &opcode)) << "nop must be found.";

    EXPECT_EQ(opcode, MAREA_BYTECODE_OPCODE_NOP) << "nop must give NOP.";

    EXPECT_TRUE(MAREA_BytecodeOpcodeFromName("return_void junk", 11, &opcode)) << "The length must cut the name.";

    EXPECT_EQ(opcode, MAREA_BYTECODE_OPCODE_RETURN_VOID) << "return_void must give RETURN_VOID.";

    EXPECT_TRUE(MAREA_BytecodeOpcodeFromName("nop", 3, nullptr)) << "A null out pointer must be accepted.";

    opcode = 99;

    EXPECT_FALSE(MAREA_BytecodeOpcodeFromName("no", 2, &opcode)) << "A prefix of a name is not a name.";

    EXPECT_FALSE(MAREA_BytecodeOpcodeFromName("nopx", 4, &opcode)) << "A name with an extra letter is not a name.";

    EXPECT_FALSE(MAREA_BytecodeOpcodeFromName("", 0, &opcode)) << "The empty name is not a name.";

    EXPECT_FALSE(MAREA_BytecodeOpcodeFromName(nullptr, 3, &opcode)) << "A null name must be rejected.";

    EXPECT_EQ(opcode, 99U) << "A failed lookup must not write the out value.";
}

TEST(MAREABytecodeOpcode, TableOfTheScalarSlicesHasTheExactNames)
{
    GTEST_COUT << "Validating the opcodes of the scalar slices keep their value and their spelling." << std::endl;

    const std::vector<std::string> NAMES{ "nop", "return_void", "move", "load_imm", "load_data", "add", "sub", "mul", "div", "rem", "pow", "sat_add", "sat_sub", "sat_mul", "shl", "shr", "neg", "bit_not", "convert", "bit_and", "bit_or", "bit_xor", "not", "eq", "ne", "lt_s", "le_s", "lt_u", "le_u", "eq_f", "ne_f", "lt_f", "le_f", "jump", "jump_if", "jump_if_not", "call", "return", "trap" };

    ASSERT_EQ(static_cast<std::size_t>(MAREA_BYTECODE_OPCODE_COUNT), NAMES.size()) << "The count must be the number of names.";

    for (std::size_t index{ 0 }; index < NAMES.size(); ++index) {
        const MAREA_Character8 *name{ MAREA_BytecodeOpcodeGetName(static_cast<MAREA_Uinteger16>(index)) };

        ASSERT_NE(name, nullptr) << "Every value below the count must have a name: " << index;

        EXPECT_EQ(std::string(name), NAMES[index]) << "The spelling of the value " << index << " is wrong.";

        MAREA_Uinteger16 opcode{ 0xFFFF };

        EXPECT_TRUE(MAREA_BytecodeOpcodeFromName(name, NAMES[index].size(), &opcode)) << "The name must be found again: " << NAMES[index];

        EXPECT_EQ(static_cast<std::size_t>(opcode), index) << "The name must give back its value: " << NAMES[index];
    }
}

TEST(MAREABytecodeOpcode, NumberKindsAndTrapCodesHaveFixedValues)
{
    GTEST_COUT << "Validating the number kinds of an operand and the trap codes keep their value." << std::endl;

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_SINTEGER_8, 0U) << "sinteger8 must be kind 0.";

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64, 3U) << "sinteger64 must be kind 3.";

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8, 4U) << "uinteger8 must be kind 4.";

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64, 7U) << "uinteger64 must be kind 7.";

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_FLOAT_32, 8U) << "float32 must be kind 8.";

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_FLOAT_64, 9U) << "float64 must be kind 9.";

    EXPECT_EQ(MAREA_BYTECODE_NUMBER_KIND_COUNT, 10U) << "There must be ten kinds.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_NONE, 0U) << "No trap must be 0.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO, 1U) << "Division by zero must be 1.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_SHIFT_OUT_OF_RANGE, 2U) << "A shift out of range must be 2.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_NEGATIVE_EXPONENT, 3U) << "A negative exponent must be 3.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_CONVERT_OUT_OF_RANGE, 4U) << "A conversion out of range must be 4.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_UNREACHABLE, 5U) << "An unreachable point must be 5.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_MISSING_RETURN, 6U) << "A missing return must be 6.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_STACK_OVERFLOW, 7U) << "A stack overflow must be 7.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_COUNT, 8U) << "There must be eight codes with the one for no trap.";
}
