// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREABytecodeHelpers.hpp"

namespace
{
    constexpr std::size_t HEADER_SIZE{ 16 };
    constexpr std::size_t ENTRY_SIZE{ 20 };
    constexpr std::size_t EMPTY_SIZE{ 132 };

    void MAREA_put32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value)
    {
        for (std::size_t index{ 0 }; index < 4; ++index) {
            bytes[(offset + index)] = static_cast<std::uint8_t>((value >> (8 * index)) & 0xFF);
        }
    }

    void MAREA_put64(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint64_t value)
    {
        for (std::size_t index{ 0 }; index < 8; ++index) {
            bytes[(offset + index)] = static_cast<std::uint8_t>((value >> (8 * index)) & 0xFF);
        }
    }
} // namespace

TEST(MAREABytecodeBinary, SerializeRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeSerialize rejects null arguments." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    EXPECT_EQ(MAREA_BytecodeSerialize(nullptr, &bytes, &size), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeSerialize(module, nullptr, &size), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null bytes pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeSerialize(module, &bytes, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null size pointer must be rejected.";
}

TEST(MAREABytecodeBinary, DeserializeRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeDeserialize rejects null arguments." << std::endl;

    MAREA_BytecodeFixture fixture;

    const MAREA_Byte BYTE{ 0 };

    MAREA_BytecodeModule *module{ nullptr };

    EXPECT_EQ(MAREA_BytecodeDeserialize(nullptr, &BYTE, 1, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_BytecodeDeserialize(fixture.context, nullptr, 1, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "Null bytes with a size must be rejected.";

    EXPECT_EQ(MAREA_BytecodeDeserialize(fixture.context, &BYTE, 1, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeDeserialize(fixture.context, nullptr, 0, &module), MAREA_BYTECODE_EXCEPT_SUCCESS) << "Null bytes with no size is an empty input, which is a diagnostic.";

    EXPECT_EQ(module, nullptr) << "An empty input gives no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED) << "An empty input is truncated.";
}

TEST(MAREABytecodeBinary, EmptyModuleHasTheExactCanonicalBytes)
{
    GTEST_COUT << "Validating the exact bytes of the empty module: magic, version, flags and the section table." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::vector<std::uint8_t> BYTES{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    ASSERT_EQ(BYTES.size(), EMPTY_SIZE) << "The empty module must be 132 bytes.";

    const std::vector<std::uint8_t> HEAD{ 'M', 'B', 'C', 0, 0, 0, 1, 0, 0, 0, 0, 0, 5, 0, 0, 0 };

    EXPECT_EQ(std::vector<std::uint8_t>(BYTES.begin(), (BYTES.begin() + HEADER_SIZE)), HEAD) << "The header must be the magic, the version 0.1, the flags 0 and 5 sections.";

    const std::uint32_t IDS[5]{ 1, 2, 4, 5, 6 };

    const std::uint64_t OFFSETS[5]{ 116, 120, 124, 128, 128 };

    const std::uint64_t SIZES[5]{ 4, 4, 4, 0, 4 };

    for (std::size_t index{ 0 }; index < 5; ++index) {
        std::vector<std::uint8_t> expected(ENTRY_SIZE, 0);

        MAREA_put32(expected, 0, IDS[index]);

        MAREA_put64(expected, 4, OFFSETS[index]);

        MAREA_put64(expected, 12, SIZES[index]);

        const auto BEGIN{ BYTES.begin() + static_cast<std::ptrdiff_t>(HEADER_SIZE + (index * ENTRY_SIZE)) };

        EXPECT_EQ(std::vector<std::uint8_t>(BEGIN, (BEGIN + ENTRY_SIZE)), expected) << "The section entry " << index << " must hold its id, offset and size.";
    }

    EXPECT_EQ(std::vector<std::uint8_t>(BYTES.begin() + 116, BYTES.end()), (std::vector<std::uint8_t>(16, 0))) << "Every section of the empty module is a zero count.";
}

TEST(MAREABytecodeBinary, SerializeIsDeterministic)
{
    GTEST_COUT << "Validating two serializations of the same module are byte-identical." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_richModule() };

    EXPECT_EQ(fixture.MAREA_serialize(module), fixture.MAREA_serialize(module)) << "The same module must give the same bytes.";

    EXPECT_EQ(fixture.MAREA_serialize(module), fixture.MAREA_serialize(fixture.MAREA_richModule())) << "Two modules built the same way must give the same bytes.";
}

TEST(MAREABytecodeBinary, EmptyModuleRoundTrips)
{
    GTEST_COUT << "Validating deserialize(serialize(m)) equals m for the empty module." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    MAREA_BytecodeModule *back{ fixture.MAREA_deserialize(fixture.MAREA_serialize(module)) };

    ASSERT_NE(back, nullptr) << "The empty module must be read back.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "The empty module must equal its round trip.";

    EXPECT_EQ(fixture.MAREA_errors(), 0U) << "A valid input gives no diagnostic.";
}

TEST(MAREABytecodeBinary, RichModuleRoundTripsBitExact)
{
    GTEST_COUT << "Validating a module with strings, types, functions and data is bit-exact after a round trip." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_richModule() };

    const std::vector<std::uint8_t> BYTES{ fixture.MAREA_serialize(module) };

    MAREA_BytecodeModule *back{ fixture.MAREA_deserialize(BYTES) };

    ASSERT_NE(back, nullptr) << "The module must be read back.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "The module must equal its round trip.";

    EXPECT_EQ(fixture.MAREA_serialize(back), BYTES) << "The bytes must be identical after a second write.";

    EXPECT_EQ(fixture.MAREA_errors(), 0U) << "A valid input gives no diagnostic.";
}

TEST(MAREABytecodeBinary, RandomModulesRoundTrip)
{
    GTEST_COUT << "Validating the binary round trip law over 200 generated modules." << std::endl;

    for (std::uint64_t seed{ 1 }; seed <= 200; ++seed) {
        MAREA_BytecodeFixture fixture;

        MAREA_BytecodeModule *module{ fixture.MAREA_randomModule(seed) };

        const std::vector<std::uint8_t> BYTES{ fixture.MAREA_serialize(module) };

        MAREA_BytecodeModule *back{ fixture.MAREA_deserialize(BYTES) };

        ASSERT_NE(back, nullptr) << "The module of seed " << seed << " must be read back.";

        EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "The module of seed " << seed << " must equal its round trip.";

        EXPECT_EQ(fixture.MAREA_serialize(back), BYTES) << "The bytes of seed " << seed << " must be identical after a second write.";
    }
}

TEST(MAREABytecodeBinary, EveryTruncationIsADiagnostic)
{
    GTEST_COUT << "Validating every proper prefix of a valid binary gives a diagnostic and no module." << std::endl;

    MAREA_BytecodeFixture source;

    const std::vector<std::uint8_t> BYTES{ source.MAREA_serialize(source.MAREA_richModule()) };

    for (std::size_t length{ 0 }; length < BYTES.size(); ++length) {
        MAREA_BytecodeFixture fixture;

        const std::vector<std::uint8_t> PREFIX(BYTES.begin(), (BYTES.begin() + static_cast<std::ptrdiff_t>(length)));

        EXPECT_EQ(fixture.MAREA_deserialize(PREFIX), nullptr) << "A prefix of " << length << " bytes must give no module.";

        EXPECT_GT(fixture.MAREA_errors(), 0U) << "A prefix of " << length << " bytes must give a diagnostic.";
    }
}

TEST(MAREABytecodeBinary, TruncatedHeaderIsTruncated)
{
    GTEST_COUT << "Validating a cut header and a cut section table give the truncated code." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::vector<std::uint8_t> BYTES{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    EXPECT_EQ(fixture.MAREA_deserialize(std::vector<std::uint8_t>(BYTES.begin(), (BYTES.begin() + 10))), nullptr) << "A cut header must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED) << "A cut header is truncated.";

    EXPECT_EQ(fixture.MAREA_deserialize(std::vector<std::uint8_t>(BYTES.begin(), (BYTES.begin() + 100))), nullptr) << "A cut section table must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED) << "A cut section table is truncated.";

    EXPECT_EQ(fixture.MAREA_deserialize(std::vector<std::uint8_t>(BYTES.begin(), (BYTES.end() - 1))), nullptr) << "A cut last section must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_TRUNCATED) << "A cut last section is truncated.";
}

TEST(MAREABytecodeBinary, BadMagicIsADiagnostic)
{
    GTEST_COUT << "Validating a wrong magic gives the bad magic code at offset 0." << std::endl;

    MAREA_BytecodeFixture fixture;

    std::vector<std::uint8_t> bytes{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    bytes[0] = 'X';

    EXPECT_EQ(fixture.MAREA_deserialize(bytes), nullptr) << "A wrong magic must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_MAGIC) << "The code must be bad magic.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sink, 0, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The diagnostic must exist.";

    EXPECT_EQ(MAREA_DiagnosticGetPosition(diagnostic).offset, 0U) << "The position must hold the byte offset.";

    EXPECT_EQ(MAREA_DiagnosticGetPosition(diagnostic).line, 0U) << "A binary diagnostic has line 0.";

    EXPECT_EQ(MAREA_DiagnosticGetSeverity(diagnostic), MAREA_DIAGNOSTIC_SEVERITY_ERROR) << "The severity must be error.";
}

TEST(MAREABytecodeBinary, UnknownVersionIsADiagnostic)
{
    GTEST_COUT << "Validating an unknown major version and a minor version above the known one are rejected." << std::endl;

    MAREA_BytecodeFixture fixture;

    std::vector<std::uint8_t> major{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    major[4] = 1;

    EXPECT_EQ(fixture.MAREA_deserialize(major), nullptr) << "An unknown major version must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION) << "The code must be unsupported version.";

    std::vector<std::uint8_t> minor{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    minor[6] = 2;

    EXPECT_EQ(fixture.MAREA_deserialize(minor), nullptr) << "A higher minor version must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_VERSION) << "The code must be unsupported version for a minor too.";
}

TEST(MAREABytecodeBinary, ReservedFlagsAreADiagnostic)
{
    GTEST_COUT << "Validating a flag bit that is not zero is rejected." << std::endl;

    MAREA_BytecodeFixture fixture;

    std::vector<std::uint8_t> bytes{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    bytes[8] = 1;

    EXPECT_EQ(fixture.MAREA_deserialize(bytes), nullptr) << "A flag bit must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_FLAGS) << "The code must be bad flags.";
}

TEST(MAREABytecodeBinary, BadSectionTableIsADiagnostic)
{
    GTEST_COUT << "Validating a wrong section count, id, offset, size and a trailing byte are rejected." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::vector<std::uint8_t> GOOD{ fixture.MAREA_serialize(fixture.MAREA_richModule()) };

    std::vector<std::uint8_t> count{ GOOD };

    count[12] = 4;

    EXPECT_EQ(fixture.MAREA_deserialize(count), nullptr) << "A wrong section count must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE) << "A wrong section count is a bad table.";

    std::vector<std::uint8_t> id{ GOOD };

    id[(HEADER_SIZE + ENTRY_SIZE)] = 9;

    EXPECT_EQ(fixture.MAREA_deserialize(id), nullptr) << "A wrong section id must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE) << "A wrong section id is a bad table.";

    std::vector<std::uint8_t> offset{ GOOD };

    MAREA_put64(offset, (HEADER_SIZE + ENTRY_SIZE + 4), 0x7FFFFFFFFFFFFFFFULL);

    EXPECT_EQ(fixture.MAREA_deserialize(offset), nullptr) << "A wrong section offset must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE) << "A wrong section offset is a bad table.";

    std::vector<std::uint8_t> gap{ GOOD };

    MAREA_put64(gap, (HEADER_SIZE + ENTRY_SIZE + 4), (static_cast<std::uint64_t>(HEADER_SIZE + (5 * ENTRY_SIZE)) + 1000));

    EXPECT_EQ(fixture.MAREA_deserialize(gap), nullptr) << "A gap between sections must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE) << "A gap between sections is a bad table.";

    std::vector<std::uint8_t> size{ GOOD };

    MAREA_put64(size, (HEADER_SIZE + 12), 0xFFFFFFFFFFFFFFFFULL);

    EXPECT_EQ(fixture.MAREA_deserialize(size), nullptr) << "A section size that overflows must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE) << "A section size that overflows is a bad table.";

    std::vector<std::uint8_t> trailing{ GOOD };

    trailing.push_back(0);

    EXPECT_EQ(fixture.MAREA_deserialize(trailing), nullptr) << "A trailing byte must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_SECTION_TABLE) << "A trailing byte is a bad table.";
}

TEST(MAREABytecodeBinary, HugeCountIsAMalformedSection)
{
    GTEST_COUT << "Validating a count larger than the bytes of the section gives a diagnostic without a huge allocation." << std::endl;

    MAREA_BytecodeFixture fixture;

    const std::vector<std::uint8_t> GOOD{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    const std::size_t COUNT_OFFSETS[4]{ 116, 120, 124, 128 };

    for (const std::size_t OFFSET : COUNT_OFFSETS) {
        std::vector<std::uint8_t> bytes{ GOOD };

        MAREA_put32(bytes, OFFSET, 0xFFFFFFFFU);

        EXPECT_EQ(fixture.MAREA_deserialize(bytes), nullptr) << "A huge count at " << OFFSET << " must give no module.";

        EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION) << "A huge count at " << OFFSET << " is a malformed section.";
    }
}

TEST(MAREABytecodeBinary, SectionWithLeftoverBytesIsMalformed)
{
    GTEST_COUT << "Validating a section that is longer than its content is rejected." << std::endl;

    MAREA_BytecodeFixture fixture;

    std::vector<std::uint8_t> bytes{ fixture.MAREA_serialize(fixture.MAREA_newModule()) };

    bytes.push_back(0);

    MAREA_put64(bytes, (HEADER_SIZE + (4 * ENTRY_SIZE) + 12), 5);

    EXPECT_EQ(fixture.MAREA_deserialize(bytes), nullptr) << "A data section with a leftover byte must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION) << "A leftover byte is a malformed section.";
}

TEST(MAREABytecodeBinary, BadReferencesAreADiagnostic)
{
    GTEST_COUT << "Validating a string or type index out of range is rejected by the reader." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *badName{ fixture.MAREA_newModule() };

    MAREA_IGNORE(fixture.MAREA_addType(badName, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, 0, 1, 1));

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badName)), nullptr) << "A type name past the pool must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE) << "A type name past the pool is a bad reference.";

    MAREA_BytecodeModule *badElement{ fixture.MAREA_newModule() };

    MAREA_IGNORE(fixture.MAREA_addType(badElement, MAREA_BYTECODE_TYPE_KIND_ARRAY, MAREA_BYTECODE_NONE, 1, 1, 5, 2));

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badElement)), nullptr) << "An element past the table must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE) << "An element past the table is a bad reference.";

    MAREA_BytecodeModule *badMember{ fixture.MAREA_newModule() };

    const std::uint32_t TYPE{ fixture.MAREA_addType(badMember, MAREA_BYTECODE_TYPE_KIND_STRUCTURE, MAREA_BYTECODE_NONE, 1, 1) };

    fixture.MAREA_addMember(badMember, TYPE, MAREA_BYTECODE_NONE, 9, 0);

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badMember)), nullptr) << "A member type past the table must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE) << "A member type past the table is a bad reference.";

    MAREA_BytecodeModule *badMemberName{ fixture.MAREA_newModule() };

    const std::uint32_t OTHER{ fixture.MAREA_addType(badMemberName, MAREA_BYTECODE_TYPE_KIND_STRUCTURE, MAREA_BYTECODE_NONE, 1, 1) };

    fixture.MAREA_addMember(badMemberName, OTHER, 3, OTHER, 0);

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badMemberName)), nullptr) << "A member name past the pool must give no module.";

    MAREA_BytecodeModule *badFunction{ fixture.MAREA_newModule() };

    MAREA_IGNORE(fixture.MAREA_addFunction(badFunction, 0, MAREA_BYTECODE_NONE, 0, 0, 0));

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badFunction)), nullptr) << "A function name past the pool must give no module.";

    MAREA_BytecodeModule *badResult{ fixture.MAREA_newModule() };

    MAREA_IGNORE(fixture.MAREA_addFunction(badResult, MAREA_BYTECODE_NONE, 0, 0, 0, 0));

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badResult)), nullptr) << "A result type past the table must give no module.";

    MAREA_BytecodeModule *badParameter{ fixture.MAREA_newModule() };

    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(badParameter, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 0, 0, 0) };

    fixture.MAREA_addParameter(badParameter, FUNCTION, 0, 0);

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(badParameter)), nullptr) << "A parameter type past the table must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_BAD_REFERENCE) << "A parameter type past the table is a bad reference.";
}

TEST(MAREABytecodeBinary, UnknownOpcodeIsADiagnostic)
{
    GTEST_COUT << "Validating an opcode that is not in the table is rejected by the reader." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 0, 0, 0) };

    fixture.MAREA_addInstruction(module, FUNCTION, 77, 0, 0, 0);

    EXPECT_EQ(fixture.MAREA_deserialize(fixture.MAREA_serialize(module)), nullptr) << "An unknown opcode must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_UNKNOWN_OPCODE) << "The code must be unknown opcode.";
}

TEST(MAREABytecodeBinary, WrongCodeRangeIsMalformed)
{
    GTEST_COUT << "Validating a function code range that does not follow the previous one is rejected." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t FIRST{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 0, 0, 0) };

    fixture.MAREA_addInstruction(module, FIRST, MAREA_BYTECODE_OPCODE_NOP, 0, 0, 0);

    const std::vector<std::uint8_t> GOOD{ fixture.MAREA_serialize(module) };

    const std::size_t FUNCTIONS_OFFSET{ 124 };

    const std::size_t CODE_OFFSET_FIELD{ (FUNCTIONS_OFFSET + 4 + 4 + 4 + 4 + 4 + 8 + 4) };

    std::vector<std::uint8_t> bytes{ GOOD };

    MAREA_put32(bytes, CODE_OFFSET_FIELD, 1);

    EXPECT_EQ(fixture.MAREA_deserialize(bytes), nullptr) << "A code offset that is not the running sum must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION) << "A wrong code offset is a malformed section.";

    std::vector<std::uint8_t> length{ GOOD };

    MAREA_put32(length, (CODE_OFFSET_FIELD + 4), 9);

    EXPECT_EQ(fixture.MAREA_deserialize(length), nullptr) << "A code length past the code section must give no module.";

    EXPECT_EQ(fixture.MAREA_lastCode(), MAREA_BYTECODE_DIAGNOSTIC_CODE_MALFORMED_SECTION) << "A code length past the code section is a malformed section.";
}

TEST(MAREABytecodeBinary, MutatedInputNeverCrashes)
{
    GTEST_COUT << "Validating random byte changes of a valid binary give a module or a diagnostic, never a crash." << std::endl;

    MAREA_BytecodeFixture source;

    const std::vector<std::uint8_t> GOOD{ source.MAREA_serialize(source.MAREA_richModule()) };

    std::uint64_t state{ 0x2545F4914F6CDD1DULL };

    for (std::size_t round{ 0 }; round < 3000; ++round) {
        MAREA_BytecodeFixture fixture;

        std::vector<std::uint8_t> bytes{ GOOD };

        const std::size_t CHANGES{ 1 + (round % 3) };

        for (std::size_t change{ 0 }; change < CHANGES; ++change) {
            state ^= (state << 13);
            state ^= (state >> 7);
            state ^= (state << 17);

            bytes[(state % bytes.size())] = static_cast<std::uint8_t>((state >> 40) & 0xFF);
        }

        MAREA_BytecodeModule *module{ fixture.MAREA_deserialize(bytes) };

        if (module == nullptr) {
            EXPECT_GT(fixture.MAREA_errors(), 0U) << "A rejected input must give a diagnostic in round " << round << ".";
        } else {
            MAREA_BytecodeModule *back{ fixture.MAREA_deserialize(fixture.MAREA_serialize(module)) };

            ASSERT_NE(back, nullptr) << "A module that was read must be written and read again in round " << round << ".";

            EXPECT_TRUE(MAREA_BytecodeModuleEquals(module, back)) << "A module that was read must keep its content in round " << round << ".";
        }
    }
}
