// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREABytecodeHelpers.hpp"
#include "tests/MAREAHostSignatureHelpers.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::string MAREA_stringAt(const MAREA_BytecodeModule *module, std::uint32_t index)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_BytecodeModuleGetString(module, index, &text, &length) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return "<none>";
        }

        return std::string(text, length);
    }

    std::string MAREA_typeName(const MAREA_BytecodeModule *module, std::uint32_t index)
    {
        MAREA_BytecodeTypeInfo type{};

        if (MAREA_BytecodeModuleGetType(module, index, &type) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return "<none>";
        }

        return MAREA_stringAt(module, type.name);
    }

    std::vector<std::string> MAREA_parameters(const MAREA_BytecodeModule *module, std::uint32_t function)
    {
        std::vector<std::string> result;

        MAREA_BytecodeFunctionInfo info{};

        MAREA_IGNORE(MAREA_BytecodeModuleGetFunction(module, function, &info));

        for (std::uint32_t index{ 0 }; index < info.parameterCount; ++index) {
            MAREA_BytecodeParameterInfo parameter{};

            MAREA_IGNORE(MAREA_BytecodeModuleGetParameter(module, function, index, &parameter));

            result.push_back(MAREA_typeName(module, parameter.type) + (((parameter.flags & MAREA_BYTECODE_PARAMETER_FLAG_BORROW) != 0) ? " borrow" : ""));
        }

        return result;
    }

    std::vector<std::string> MAREA_names(const std::vector<std::string> &instructions)
    {
        std::vector<std::string> result;

        for (const std::string &instruction : instructions) {
            result.push_back(instruction.substr(0, instruction.find(' ')));
        }

        return result;
    }

    struct MAREA_ProjectGenerateFixture
    {
        MAREA_ParseFixture unit;
        std::unique_ptr<MAREA_GenerateFixture> generate;

        MAREA_ProjectGenerateFixture()
        {
            MAREA_IGNORE(unit.MAREA_parse("namespace other; function value(): sinteger32 return 7; end"));

            const MAREA_AstNode *root{ unit.root };

            generate = std::make_unique<MAREA_GenerateFixture>([root](const std::string &text, const MAREA_AstNode **outRoot) {
                if (text == "other") {
                    *outRoot = root;

                    return MAREA_SEMA_IMPORT_RESULT_FOUND;
                }

                return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
            });
        }
    };
} // namespace

TEST(MAREABytecodeHostCalls, NewValuesAreFixed)
{
    GTEST_COUT << "Validating the new opcodes, trap codes, flags and the diagnostic code keep their value and spelling." << std::endl;

    EXPECT_EQ(MAREA_BYTECODE_OPCODE_ALLOCATE, 46) << "alloc is 46.";

    EXPECT_EQ(MAREA_BYTECODE_OPCODE_FREE, 47) << "free is 47.";

    EXPECT_STREQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_ALLOCATE), "alloc") << "The spelling of alloc.";

    EXPECT_STREQ(MAREA_BytecodeOpcodeGetName(MAREA_BYTECODE_OPCODE_FREE), "free") << "The spelling of free.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_OUT_OF_MEMORY, 9) << "Out of memory is 9.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_HOST_FUNCTION_MISSING, 10) << "A missing host function is 10.";

    EXPECT_EQ(MAREA_BYTECODE_TRAP_CODE_INVALID_PARTITION_COUNT, 11) << "A bad part count is 11.";

    EXPECT_EQ(MAREA_BYTECODE_FUNCTION_FLAG_HOST, 1) << "The host flag is bit 0.";

    EXPECT_EQ(MAREA_BYTECODE_PARAMETER_FLAG_BORROW, 1) << "The borrow flag is bit 0.";

    EXPECT_EQ(MAREA_BYTECODE_DIAGNOSTIC_CODE_UNTYPED_OPERAND, 5035U) << "An untyped operand is 5035.";
}

TEST(MAREABytecodeHostCalls, AHostCallBecomesAnImportOfTheFunctionTable)
{
    GTEST_COUT << "Validating a host call adds one import after the functions of the unit and calls it by its index." << std::endl;

    MAREA_HostGenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32): sinteger32 return host.add(a, a) + host.add(a, a); end function g() end")) << "The unit must generate.";

    ASSERT_EQ(MAREA_BytecodeModuleGetFunctionCount(fixture.module), 3U) << "f, g and one import for the two calls.";

    MAREA_BytecodeFunctionInfo info{};

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(fixture.module, 2, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The import must be there.";

    EXPECT_EQ(fixture.MAREA_functionName(2), "demo.host.add") << "The name is the full path of the registration.";

    EXPECT_EQ(info.flags, static_cast<MAREA_Uinteger32>(MAREA_BYTECODE_FUNCTION_FLAG_HOST)) << "The import has the host flag.";

    EXPECT_EQ(info.instructionCount, 0U) << "An import has no code.";

    EXPECT_EQ(info.registerCount, 0U) << "An import has no register.";

    EXPECT_EQ(MAREA_typeName(fixture.module, info.result), "sinteger32") << "The result type of the signature.";

    EXPECT_EQ(MAREA_parameters(fixture.module, 2), (std::vector<std::string>{ "sinteger32", "sinteger32" })) << "Two scalars.";

    const std::vector<std::string> CODE{ fixture.MAREA_instructions(0) };

    EXPECT_EQ(std::count_if(CODE.begin(), CODE.end(), [](const std::string &line) { return (line.rfind("call ", 0) == 0) && (line.find(" 2 2") != std::string::npos); }), 2) << "Both calls name the import 2 with two arguments.";
}

TEST(MAREABytecodeHostCalls, ABorrowIsFlaggedAndAStructureResultGetsFrameStorage)
{
    GTEST_COUT << "Validating a structure argument is a borrow and a structure result is written to zeroed frame storage." << std::endl;

    MAREA_HostGenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; host.sum(a, a); return 0; end")) << "The unit must generate.";

    EXPECT_EQ(MAREA_parameters(fixture.module, 1), (std::vector<std::string>{ "Pair borrow", "Pair borrow" })) << "Both are borrows.";

    MAREA_BytecodeFunctionInfo info{};

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(fixture.module, 1, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The import must be there.";

    EXPECT_EQ(MAREA_typeName(fixture.module, info.result), "Pair") << "The result is the structure.";

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(fixture.module, 0, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The function must be there.";

    EXPECT_EQ(info.frameArea, 16U) << "The local and the discarded result take 8 bytes each.";

    const std::vector<std::string> NAMES{ MAREA_names(fixture.MAREA_instructions(0)) };

    const std::vector<std::string> CALL{ "addr_of", "zero", "move", "move", "call" };

    EXPECT_NE(std::search(NAMES.begin(), NAMES.end(), CALL.begin(), CALL.end()), NAMES.end()) << "The result storage is ready before the arguments and the call.";
}

TEST(MAREABytecodeHostCalls, AnUntypedValueIsRefused)
{
    GTEST_COUT << "Validating a call that sema gave no type to cannot be a value." << std::endl;

    for (const char *body : { "function f(): sinteger32 local v: sinteger32 = host.unknown(); return v; end", "function f(): sinteger32 return host.add(host.unknown(), 1); end", "function f() if host.unknown() then end end", "function f() host.record(host.unknown()); end" }) {
        MAREA_HostGenerateFixture fixture;

        EXPECT_FALSE(fixture.MAREA_build(body)) << body;

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ 5035 }) << body;
    }
}

TEST(MAREABytecodeHostCalls, AnUntypedStatementCallIsAnImportWithNoResult)
{
    GTEST_COUT << "Validating a call statement sema gave no type to becomes an import with no result." << std::endl;

    MAREA_HostGenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32) host.unknown(a); end")) << "The unit must generate.";

    MAREA_BytecodeFunctionInfo info{};

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(fixture.module, 1, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The import must be there.";

    EXPECT_EQ(info.result, MAREA_BYTECODE_NONE) << "There is no result.";
}

TEST(MAREABytecodeHostCalls, AnArgumentWithNoValueFormIsRefused)
{
    GTEST_COUT << "Validating a string argument is refused, because a string is not a bytecode value yet." << std::endl;

    MAREA_HostGenerateFixture fixture;

    EXPECT_FALSE(fixture.MAREA_build("function f(): sinteger32 return host.text(\"a\"); end")) << "A string cannot cross.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ 5033 }) << "The type is not supported.";
}

TEST(MAREABytecodeHostCalls, ACallIntoAnotherUnitOfTheProjectIsAnUnresolvedName)
{
    GTEST_COUT << "Validating a call into another unit of the project, in every way of writing it, is a compile error and no import." << std::endl;

    for (const char *source : { "namespace main; import other; function f(): sinteger32 return other.value(); end", "namespace main; import other; function f() other.value(); end", "namespace main; import other as o; function f(): sinteger32 return o.value(); end", "namespace main; import other.value as v; function f(): sinteger32 return v(); end", "namespace main; import other { value as v }; function f() v(); end" }) {
        MAREA_ProjectGenerateFixture fixture;

        EXPECT_FALSE(fixture.generate->MAREA_generate(source)) << source;

        EXPECT_EQ(fixture.generate->MAREA_codes(), std::vector<MAREA_Uinteger32>{ 5031 }) << source;
    }
}

TEST(MAREABytecodeHostCalls, ACallThroughSelfOrALocalTypeAliasIsAnUnresolvedName)
{
    GTEST_COUT << "Validating a call through Self or a local type alias, as a value or a statement, is a compile error and no import." << std::endl;

    for (const char *body : { "implementation Pair function one(): sinteger32 return 1; end function two(): sinteger32 return Self.one(); end end", "implementation Pair function one() end function two() Self.one(); end end", "implementation Pair function one(): sinteger32 return 1; end end type Q = Pair; function f(): sinteger32 return Q.one(); end", "implementation Pair function one() end end type Q = Pair; function f() Q.one(); end", "function f() main.missing(); end" }) {
        MAREA_HostGenerateFixture fixture;

        EXPECT_FALSE(fixture.MAREA_build(body)) << body;

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ 5031 }) << body;
    }
}

TEST(MAREABytecodeHostCalls, ARegisteredHostOrLibraryNameIsStillAnImport)
{
    GTEST_COUT << "Validating a host or library name, in every way of writing it, becomes an import of its full path." << std::endl;

    const std::vector<std::pair<const char *, const char *>> CASES{
        { "namespace main; require demo.host; function f(): sinteger32 return demo.host.add(1, 2); end", "demo.host.add" },
        { "namespace main; require demo.host as host; function f(): sinteger32 return host.add(1, 2); end", "demo.host.add" },
        { "namespace main; require demo.host.add as plus; function f(): sinteger32 return plus(1, 2); end", "demo.host.add" },
        { "namespace main; require demo.host { add as plus }; function f(): sinteger32 return plus(1, 2); end", "demo.host.add" },
        { "namespace main; require c.math; function f() c.math.log(); end", "c.math.log" },
        { "namespace main; require demo.other; function f() demo.other.thing(); end", "demo.other.thing" },
    };

    for (const auto &[source, import] : CASES) {
        MAREA_HostGenerateFixture fixture;

        ASSERT_TRUE(fixture.MAREA_generate(source)) << source;

        ASSERT_EQ(MAREA_BytecodeModuleGetFunctionCount(fixture.module), 2U) << source;

        EXPECT_EQ(fixture.MAREA_functionName(1), import) << source;
    }
}

TEST(MAREABytecodeHostCalls, ASpawnLowersAsThePlainCall)
{
    GTEST_COUT << "Validating fork_join and spawn add no instruction: the loop of the parts is sequential." << std::endl;

    MAREA_HostGenerateFixture parallel;
    MAREA_HostGenerateFixture plain;

    ASSERT_TRUE(parallel.MAREA_build("function w(a: sinteger32): sinteger32 return a; end function f() fork_join spawn w(1); spawn host.record(2); end end")) << "The unit must generate.";

    ASSERT_TRUE(plain.MAREA_build("function w(a: sinteger32): sinteger32 return a; end function f() w(1); host.record(2); end")) << "The unit must generate.";

    EXPECT_EQ(parallel.MAREA_instructions(1), plain.MAREA_instructions(1)) << "The same code.";
}

TEST(MAREABytecodeHostCalls, ATaskHandleAndAPartAreNotValuesYet)
{
    GTEST_COUT << "Validating the handle of a spawn and the part of a partition are refused where a value is needed." << std::endl;

    for (const char *body : { "function w(): sinteger32 return 1; end function f() fork_join local t: demo.task<sinteger32> = spawn w(); end end", "function w(): sinteger32 return 1; end function f() fork_join host.record(spawn w()); end end", "constant DATA: demo.array<sinteger32, 4> = demo.array<sinteger32, 4>{ 1, 2, 3, 4 }; function f() fork_join partition DATA into 2 as part do host.unknown(part); end end end" }) {
        MAREA_HostGenerateFixture fixture;

        EXPECT_FALSE(fixture.MAREA_build(body)) << body;

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ 5033 }) << body;
    }
}

TEST(MAREABytecodeHostCalls, AModuleWithImportsAndAllocationSurvivesBothForms)
{
    GTEST_COUT << "Validating imports, borrow flags, alloc and free go through the binary and the text form." << std::endl;

    MAREA_HostGenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f() local a: Pair = Pair{ x = 1, y = 2 }; host.sum(a, a); end")) << "The unit must generate.";

    MAREA_BytecodeInstruction instruction{ MAREA_BYTECODE_OPCODE_ALLOCATE, 1, 0, 8 };

    ASSERT_EQ(MAREA_BytecodeModuleAddInstruction(fixture.module, 0, &instruction), MAREA_BYTECODE_EXCEPT_SUCCESS) << "alloc must be added.";

    instruction = { MAREA_BYTECODE_OPCODE_FREE, 1, 0, 0 };

    ASSERT_EQ(MAREA_BytecodeModuleAddInstruction(fixture.module, 0, &instruction), MAREA_BYTECODE_EXCEPT_SUCCESS) << "free must be added.";

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    ASSERT_EQ(MAREA_BytecodeSerialize(fixture.module, &bytes, &size), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The module must serialize.";

    MAREA_BytecodeModule *binary{ nullptr };

    ASSERT_EQ(MAREA_BytecodeDeserialize(fixture.context, bytes, size, &binary), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The bytes must be read.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(fixture.module, binary)) << "The binary form keeps the module.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_BytecodeDisassemble(fixture.module, &text, &length), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The module must disassemble.";

    MAREA_BytecodeModule *assembled{ nullptr };

    ASSERT_EQ(MAREA_BytecodeAssemble(fixture.context, text, length, &assembled), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The text must be read.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(fixture.module, assembled)) << "The text form keeps the module.";
}
