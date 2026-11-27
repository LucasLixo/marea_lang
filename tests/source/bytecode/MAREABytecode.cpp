// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstddef>

#include "tests/MAREABytecodeHelpers.hpp"

TEST(MAREABytecode, ContextCreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeContextCreate rejects null arguments." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeContext *context{ nullptr };

    EXPECT_EQ(MAREA_BytecodeContextCreate(nullptr, fixture.sink, &context), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_BytecodeContextCreate(fixture.arena, nullptr, &context), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null sink must be rejected.";

    EXPECT_EQ(MAREA_BytecodeContextCreate(fixture.arena, fixture.sink, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";
}

TEST(MAREABytecode, ContextGettersReturnWhatWasGiven)
{
    GTEST_COUT << "Validating the context getters return the arena and the sink." << std::endl;

    MAREA_BytecodeFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    EXPECT_EQ(MAREA_BytecodeContextGetArena(fixture.context), fixture.arena) << "The arena must be the one given.";

    EXPECT_EQ(MAREA_BytecodeContextGetDiagnostics(fixture.context), fixture.sink) << "The sink must be the one given.";

    EXPECT_EQ(MAREA_BytecodeContextGetArena(nullptr), nullptr) << "A null context must give a null arena.";

    EXPECT_EQ(MAREA_BytecodeContextGetDiagnostics(nullptr), nullptr) << "A null context must give a null sink.";
}

TEST(MAREABytecode, ModuleCreateIsEmptyAndRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeModuleCreate gives an empty module and rejects null arguments." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ nullptr };

    EXPECT_EQ(MAREA_BytecodeModuleCreate(nullptr, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleCreate(fixture.context, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    module = fixture.MAREA_newModule();

    ASSERT_NE(module, nullptr) << "A module must be created.";

    EXPECT_EQ(MAREA_BytecodeModuleGetContext(module), fixture.context) << "The module must keep its context.";

    EXPECT_EQ(MAREA_BytecodeModuleGetContext(nullptr), nullptr) << "A null module must give a null context.";

    EXPECT_EQ(MAREA_BytecodeModuleGetStringCount(module), 0U) << "A new module has no string.";

    EXPECT_EQ(MAREA_BytecodeModuleGetTypeCount(module), 0U) << "A new module has no type.";

    EXPECT_EQ(MAREA_BytecodeModuleGetFunctionCount(module), 0U) << "A new module has no function.";

    EXPECT_EQ(MAREA_BytecodeModuleGetDataCount(module), 0U) << "A new module has no blob.";

    EXPECT_EQ(MAREA_BytecodeModuleGetStringCount(nullptr), 0U) << "A null module has no string.";

    EXPECT_EQ(MAREA_BytecodeModuleGetTypeCount(nullptr), 0U) << "A null module has no type.";

    EXPECT_EQ(MAREA_BytecodeModuleGetFunctionCount(nullptr), 0U) << "A null module has no function.";

    EXPECT_EQ(MAREA_BytecodeModuleGetDataCount(nullptr), 0U) << "A null module has no blob.";
}

TEST(MAREABytecode, StringPoolDeduplicatesAndKeepsEveryByte)
{
    GTEST_COUT << "Validating the string pool deduplicates, keeps any byte and ends each string with a NUL." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t FIRST{ fixture.MAREA_addString(module, "alpha") };

    const std::uint32_t SECOND{ fixture.MAREA_addString(module, std::string("a\0b", 3)) };

    const std::uint32_t EMPTY{ fixture.MAREA_addString(module, "") };

    EXPECT_EQ(fixture.MAREA_addString(module, "alpha"), FIRST) << "The same string must give the same index.";

    EXPECT_EQ(fixture.MAREA_addString(module, ""), EMPTY) << "The empty string must be deduplicated.";

    EXPECT_NE(FIRST, SECOND) << "Different strings must have different indexes.";

    EXPECT_EQ(MAREA_BytecodeModuleGetStringCount(module), 3U) << "Three different strings are in the pool.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_BytecodeModuleGetString(module, SECOND, &text, &length), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid index must be read.";

    EXPECT_EQ(length, 3U) << "A string keeps its NUL byte inside.";

    EXPECT_EQ(std::string(text, length), std::string("a\0b", 3)) << "The bytes must be kept.";

    EXPECT_EQ(text[length], '\0') << "A NUL follows the last byte.";
}

TEST(MAREABytecode, StringPoolRejectsBadArguments)
{
    GTEST_COUT << "Validating the string pool rejects null arguments and an index out of range." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    std::uint32_t index{ 0 };

    EXPECT_EQ(MAREA_BytecodeModuleAddString(nullptr, "a", 1, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddString(module, nullptr, 1, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null text with a length must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddString(module, "a", 1, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddString(module, nullptr, 0, &index), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A null text with no length is the empty string.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_BytecodeModuleGetString(nullptr, 0, &text, &length), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetString(module, 0, nullptr, &length), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null text pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetString(module, 0, &text, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null length pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetString(module, 1, &text, &length), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "An index past the count must be rejected.";
}

TEST(MAREABytecode, StringPoolReportsOutOfMemory)
{
    GTEST_COUT << "Validating a string too large for the arena gives out of memory." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    std::uint32_t index{ 0 };

    const MAREA_IntegerSize HUGE_LENGTH{ (MAREA_INTEGER_SIZE_MAXIMUM / 2) };

    EXPECT_EQ(MAREA_BytecodeModuleAddString(module, "a", HUGE_LENGTH, &index), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY) << "A huge length must give out of memory.";

    EXPECT_EQ(MAREA_BytecodeModuleGetStringCount(module), 0U) << "A failed add must not change the pool.";
}

TEST(MAREABytecode, TypeTableKeepsEveryField)
{
    GTEST_COUT << "Validating the type table keeps every field and every member." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t NAME{ fixture.MAREA_addString(module, "Point") };

    const std::uint32_t INTEGER{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_BYTECODE_NONE, 4, 4) };

    const std::uint32_t ARRAY{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_ARRAY, NAME, 0x123456789ULL, 8, INTEGER, 0x9876543210ULL) };

    fixture.MAREA_addMember(module, ARRAY, NAME, INTEGER, 0xABCDEF0123ULL);

    fixture.MAREA_addMember(module, ARRAY, MAREA_BYTECODE_NONE, INTEGER, 7);

    EXPECT_EQ(INTEGER, 0U) << "The first type has index 0.";

    EXPECT_EQ(ARRAY, 1U) << "The second type has index 1.";

    EXPECT_EQ(MAREA_BytecodeModuleGetTypeCount(module), 2U) << "Two types were added.";

    MAREA_BytecodeTypeInfo info{};

    ASSERT_EQ(MAREA_BytecodeModuleGetType(module, ARRAY, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid index must be read.";

    EXPECT_EQ(info.kind, MAREA_BYTECODE_TYPE_KIND_ARRAY) << "The kind must be kept.";

    EXPECT_EQ(info.name, NAME) << "The name must be kept.";

    EXPECT_EQ(info.size, 0x123456789ULL) << "The size must be kept over 32 bits.";

    EXPECT_EQ(info.alignment, 8U) << "The alignment must be kept.";

    EXPECT_EQ(info.element, INTEGER) << "The element must be kept.";

    EXPECT_EQ(info.count, 0x9876543210ULL) << "The count must be kept over 32 bits.";

    EXPECT_EQ(info.memberCount, 2U) << "The member count must be set on read.";

    MAREA_BytecodeMemberInfo member{};

    ASSERT_EQ(MAREA_BytecodeModuleGetMember(module, ARRAY, 0, &member), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid member must be read.";

    EXPECT_EQ(member.name, NAME) << "The member name must be kept.";

    EXPECT_EQ(member.type, INTEGER) << "The member type must be kept.";

    EXPECT_EQ(member.value, 0xABCDEF0123ULL) << "The member value must be kept over 32 bits.";

    ASSERT_EQ(MAREA_BytecodeModuleGetMember(module, ARRAY, 1, &member), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The second member must be read.";

    EXPECT_EQ(member.name, MAREA_BYTECODE_NONE) << "An absent name must be kept.";

    ASSERT_EQ(MAREA_BytecodeModuleGetType(module, INTEGER, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The first type must be read.";

    EXPECT_EQ(info.memberCount, 0U) << "A type with no member has a count of 0.";
}

TEST(MAREABytecode, TypeTableRejectsBadArguments)
{
    GTEST_COUT << "Validating the type table rejects null arguments and an index out of range." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t TYPE{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_BYTECODE_NONE, 1, 1) };

    MAREA_BytecodeTypeInfo info{};

    MAREA_BytecodeMemberInfo member{};

    std::uint32_t index{ 0 };

    EXPECT_EQ(MAREA_BytecodeModuleAddType(nullptr, &info, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddType(module, nullptr, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null type must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddType(module, &info, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetType(nullptr, 0, &info), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on read.";

    EXPECT_EQ(MAREA_BytecodeModuleGetType(module, 0, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out type must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetType(module, 1, &info), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "An index past the count must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddMember(nullptr, TYPE, &member), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on add member.";

    EXPECT_EQ(MAREA_BytecodeModuleAddMember(module, TYPE, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null member must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddMember(module, 1, &member), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A type index past the count must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetMember(nullptr, TYPE, 0, &member), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on get member.";

    EXPECT_EQ(MAREA_BytecodeModuleGetMember(module, TYPE, 0, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out member must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetMember(module, 1, 0, &member), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A type index past the count must be rejected on read.";

    EXPECT_EQ(MAREA_BytecodeModuleGetMember(module, TYPE, 0, &member), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A member index past the count must be rejected.";
}

TEST(MAREABytecode, FunctionTableKeepsEveryField)
{
    GTEST_COUT << "Validating the function table keeps the header, the parameters and the instructions." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t TYPE{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_BYTECODE_NONE, 4, 4) };

    const std::uint32_t NAME{ fixture.MAREA_addString(module, "run") };

    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, NAME, TYPE, 70000, 0x123456789ULL, 5) };

    fixture.MAREA_addParameter(module, FUNCTION, TYPE, 1);

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_NOP, 1, 2, 3);

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN_VOID, 65535, 0, 0);

    MAREA_BytecodeFunctionInfo info{};

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(module, FUNCTION, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid function must be read.";

    EXPECT_EQ(info.name, NAME) << "The name must be kept.";

    EXPECT_EQ(info.result, TYPE) << "The result must be kept.";

    EXPECT_EQ(info.registerCount, 70000U) << "The register count must be kept.";

    EXPECT_EQ(info.frameArea, 0x123456789ULL) << "The frame area must be kept over 32 bits.";

    EXPECT_EQ(info.flags, 5U) << "The flags must be kept.";

    EXPECT_EQ(info.parameterCount, 1U) << "The parameter count must be set on read.";

    EXPECT_EQ(info.instructionCount, 2U) << "The instruction count must be set on read.";

    MAREA_BytecodeParameterInfo parameter{};

    ASSERT_EQ(MAREA_BytecodeModuleGetParameter(module, FUNCTION, 0, &parameter), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid parameter must be read.";

    EXPECT_EQ(parameter.type, TYPE) << "The parameter type must be kept.";

    EXPECT_EQ(parameter.flags, 1U) << "The parameter flags must be kept.";

    MAREA_BytecodeInstruction instruction{};

    ASSERT_EQ(MAREA_BytecodeModuleGetInstruction(module, FUNCTION, 1, &instruction), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid instruction must be read.";

    EXPECT_EQ(instruction.opcode, MAREA_BYTECODE_OPCODE_RETURN_VOID) << "The opcode must be kept.";

    EXPECT_EQ(instruction.a, 65535U) << "The operand a must be kept.";

    EXPECT_EQ(MAREA_BytecodeModuleGetFunctionCount(module), 1U) << "One function was added.";
}

TEST(MAREABytecode, FunctionTableRejectsBadArguments)
{
    GTEST_COUT << "Validating the function table rejects null arguments and an index out of range." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 0, 0, 0) };

    MAREA_BytecodeFunctionInfo info{};

    MAREA_BytecodeParameterInfo parameter{};

    MAREA_BytecodeInstruction instruction{};

    std::uint32_t index{ 0 };

    EXPECT_EQ(MAREA_BytecodeModuleAddFunction(nullptr, &info, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddFunction(module, nullptr, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null function must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddFunction(module, &info, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetFunction(nullptr, 0, &info), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on read.";

    EXPECT_EQ(MAREA_BytecodeModuleGetFunction(module, 0, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out function must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetFunction(module, 1, &info), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "An index past the count must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddParameter(nullptr, FUNCTION, &parameter), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on add parameter.";

    EXPECT_EQ(MAREA_BytecodeModuleAddParameter(module, FUNCTION, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null parameter must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddParameter(module, 1, &parameter), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A function index past the count must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetParameter(nullptr, FUNCTION, 0, &parameter), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on get parameter.";

    EXPECT_EQ(MAREA_BytecodeModuleGetParameter(module, FUNCTION, 0, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out parameter must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetParameter(module, FUNCTION, 0, &parameter), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A parameter index past the count must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetParameter(module, 1, 0, &parameter), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A function index past the count must be rejected on get parameter.";

    EXPECT_EQ(MAREA_BytecodeModuleAddInstruction(nullptr, FUNCTION, &instruction), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on add instruction.";

    EXPECT_EQ(MAREA_BytecodeModuleAddInstruction(module, FUNCTION, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null instruction must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddInstruction(module, 1, &instruction), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A function index past the count must be rejected on add instruction.";

    EXPECT_EQ(MAREA_BytecodeModuleGetInstruction(nullptr, FUNCTION, 0, &instruction), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on get instruction.";

    EXPECT_EQ(MAREA_BytecodeModuleGetInstruction(module, FUNCTION, 0, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out instruction must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetInstruction(module, FUNCTION, 0, &instruction), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "An instruction index past the count must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetInstruction(module, 1, 0, &instruction), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A function index past the count must be rejected on get instruction.";
}

TEST(MAREABytecode, DataSectionKeepsBlobsApart)
{
    GTEST_COUT << "Validating the read-only data section keeps each blob, merges none and accepts an empty blob." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t FIRST{ fixture.MAREA_addData(module, { 1, 2, 3 }) };

    const std::uint32_t SECOND{ fixture.MAREA_addData(module, { 1, 2, 3 }) };

    const std::uint32_t EMPTY{ fixture.MAREA_addData(module, {}) };

    EXPECT_NE(FIRST, SECOND) << "Two equal blobs keep two indexes.";

    EXPECT_EQ(MAREA_BytecodeModuleGetDataCount(module), 3U) << "Three blobs were added.";

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    ASSERT_EQ(MAREA_BytecodeModuleGetData(module, FIRST, &bytes, &size), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A valid blob must be read.";

    ASSERT_EQ(size, 3U) << "The size must be kept.";

    EXPECT_EQ(bytes[2], 3U) << "The bytes must be kept.";

    ASSERT_EQ(MAREA_BytecodeModuleGetData(module, EMPTY, &bytes, &size), MAREA_BYTECODE_EXCEPT_SUCCESS) << "An empty blob must be read.";

    EXPECT_EQ(size, 0U) << "An empty blob has no byte.";
}

TEST(MAREABytecode, DataSectionRejectsBadArguments)
{
    GTEST_COUT << "Validating the data section rejects null arguments, an index out of range and a huge blob." << std::endl;

    MAREA_BytecodeFixture fixture;

    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const MAREA_Byte BYTE{ 1 };

    std::uint32_t index{ 0 };

    EXPECT_EQ(MAREA_BytecodeModuleAddData(nullptr, &BYTE, 1, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddData(module, nullptr, 1, &index), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "Null bytes with a size must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddData(module, &BYTE, 1, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleAddData(module, &BYTE, (MAREA_INTEGER_SIZE_MAXIMUM / 2), &index), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_MEMORY) << "A huge size must give out of memory.";

    EXPECT_EQ(MAREA_BytecodeModuleGetDataCount(module), 0U) << "A failed add must not change the section.";

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    EXPECT_EQ(MAREA_BytecodeModuleGetData(nullptr, 0, &bytes, &size), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected on read.";

    EXPECT_EQ(MAREA_BytecodeModuleGetData(module, 0, nullptr, &size), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null bytes pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetData(module, 0, &bytes, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null size pointer must be rejected.";

    EXPECT_EQ(MAREA_BytecodeModuleGetData(module, 0, &bytes, &size), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "An index past the count must be rejected.";
}

TEST(MAREABytecode, EqualsComparesEveryPart)
{
    GTEST_COUT << "Validating MAREA_BytecodeModuleEquals compares every part of a module." << std::endl;

    MAREA_BytecodeFixture fixture;

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(fixture.MAREA_richModule(), fixture.MAREA_richModule())) << "Two modules built the same way are equal.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(fixture.MAREA_newModule(), fixture.MAREA_newModule())) << "Two empty modules are equal.";

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(nullptr, nullptr)) << "Two null modules are not equal.";

    MAREA_BytecodeModule *base{ fixture.MAREA_richModule() };

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, nullptr)) << "A module is not equal to null.";

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(nullptr, base)) << "Null is not equal to a module.";

    MAREA_BytecodeModule *moreStrings{ fixture.MAREA_richModule() };

    MAREA_IGNORE(fixture.MAREA_addString(moreStrings, "more"));

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreStrings)) << "A different string pool must differ.";

    MAREA_BytecodeModule *moreTypes{ fixture.MAREA_richModule() };

    MAREA_IGNORE(fixture.MAREA_addType(moreTypes, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, MAREA_BYTECODE_NONE, 1, 1));

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreTypes)) << "A different type table must differ.";

    MAREA_BytecodeModule *moreMembers{ fixture.MAREA_richModule() };

    fixture.MAREA_addMember(moreMembers, 1, MAREA_BYTECODE_NONE, 0, 9);

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreMembers)) << "A different member list must differ.";

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, fixture.MAREA_newModule())) << "A module with nothing must differ from a full one.";

    MAREA_BytecodeModule *moreFunctions{ fixture.MAREA_richModule() };

    MAREA_IGNORE(fixture.MAREA_addFunction(moreFunctions, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 1, 0, 0));

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreFunctions)) << "A different function table must differ.";

    MAREA_BytecodeModule *moreInstructions{ fixture.MAREA_richModule() };

    fixture.MAREA_addInstruction(moreInstructions, 0, MAREA_BYTECODE_OPCODE_NOP, 0, 0, 0);

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreInstructions)) << "A different code must differ.";

    MAREA_BytecodeModule *moreParameters{ fixture.MAREA_richModule() };

    fixture.MAREA_addParameter(moreParameters, 0, 0, 0);

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreParameters)) << "A different parameter list must differ.";

    MAREA_BytecodeModule *moreData{ fixture.MAREA_richModule() };

    MAREA_IGNORE(fixture.MAREA_addData(moreData, { 1 }));

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(base, moreData)) << "A different data section must differ.";

    MAREA_BytecodeModule *firstOne{ fixture.MAREA_newModule() };

    MAREA_BytecodeModule *secondOne{ fixture.MAREA_newModule() };

    MAREA_IGNORE(fixture.MAREA_addData(firstOne, { 1 }));

    MAREA_IGNORE(fixture.MAREA_addData(secondOne, { 2 }));

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(firstOne, secondOne)) << "A blob with another byte must differ.";

    MAREA_BytecodeModule *fieldOne{ fixture.MAREA_newModule() };

    MAREA_BytecodeModule *fieldTwo{ fixture.MAREA_newModule() };

    MAREA_IGNORE(fixture.MAREA_addFunction(fieldOne, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 1, 0, 0));

    MAREA_IGNORE(fixture.MAREA_addFunction(fieldTwo, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 2, 0, 0));

    EXPECT_FALSE(MAREA_BytecodeModuleEquals(fieldOne, fieldTwo)) << "A function with another register count must differ.";
}

TEST(MAREABytecode, InfoStructuresHaveAFixedLayout)
{
    GTEST_COUT << "Validating the size and the field offsets of the open structures." << std::endl;

    EXPECT_EQ(sizeof(MAREA_BytecodeTypeInfo), 40U) << "The type info must be 40 bytes.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, size), 0U) << "size must be at 0.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, count), 8U) << "count must be at 8.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, name), 16U) << "name must be at 16.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, element), 20U) << "element must be at 20.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, alignment), 24U) << "alignment must be at 24.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, memberCount), 28U) << "memberCount must be at 28.";

    EXPECT_EQ(offsetof(MAREA_BytecodeTypeInfo, kind), 32U) << "kind must be at 32.";

    EXPECT_EQ(sizeof(MAREA_BytecodeMemberInfo), 16U) << "The member info must be 16 bytes.";

    EXPECT_EQ(offsetof(MAREA_BytecodeMemberInfo, value), 0U) << "value must be at 0.";

    EXPECT_EQ(offsetof(MAREA_BytecodeMemberInfo, name), 8U) << "name must be at 8.";

    EXPECT_EQ(offsetof(MAREA_BytecodeMemberInfo, type), 12U) << "type must be at 12.";

    EXPECT_EQ(sizeof(MAREA_BytecodeFunctionInfo), 32U) << "The function info must be 32 bytes.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, frameArea), 0U) << "frameArea must be at 0.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, name), 8U) << "name must be at 8.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, result), 12U) << "result must be at 12.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, registerCount), 16U) << "registerCount must be at 16.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, flags), 20U) << "flags must be at 20.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, parameterCount), 24U) << "parameterCount must be at 24.";

    EXPECT_EQ(offsetof(MAREA_BytecodeFunctionInfo, instructionCount), 28U) << "instructionCount must be at 28.";

    EXPECT_EQ(sizeof(MAREA_BytecodeParameterInfo), 8U) << "The parameter info must be 8 bytes.";

    EXPECT_EQ(offsetof(MAREA_BytecodeParameterInfo, type), 0U) << "type must be at 0.";

    EXPECT_EQ(offsetof(MAREA_BytecodeParameterInfo, flags), 4U) << "flags must be at 4.";

    EXPECT_EQ(sizeof(MAREA_BytecodeInstruction), 8U) << "An instruction must be 64 bits.";

    EXPECT_EQ(offsetof(MAREA_BytecodeInstruction, opcode), 0U) << "opcode must be at 0.";

    EXPECT_EQ(offsetof(MAREA_BytecodeInstruction, a), 2U) << "a must be at 2.";

    EXPECT_EQ(offsetof(MAREA_BytecodeInstruction, b), 4U) << "b must be at 4.";

    EXPECT_EQ(offsetof(MAREA_BytecodeInstruction, c), 6U) << "c must be at 6.";
}
