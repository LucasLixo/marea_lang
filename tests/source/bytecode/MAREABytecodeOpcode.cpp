// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

TEST(MAREABytecodeOpcode, GetNameGivesTheSpelling)
{
    GTEST_COUT << "Validating MAREA_BytecodeOpcodeGetName gives the text name of each opcode." << std::endl;

    ASSERT_NE(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_NOP), nullptr) << "nop must have a name.";

    EXPECT_STREQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_NOP), "nop") << "The name of NOP must be nop.";

    ASSERT_NE(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_RETURN_VOID), nullptr) << "return_void must have a name.";

    EXPECT_STREQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_RETURN_VOID), "return_void") << "The name of RETURN_VOID must be return_void.";

    EXPECT_EQ(MAREA_BytecodeOpcodeGetName(2), nullptr) << "An opcode out of the table has no name.";

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
