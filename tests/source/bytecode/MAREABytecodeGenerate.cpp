// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREABytecodeGenerateHelpers.hpp"

namespace
{
    using Lines = std::vector<std::string>;

    struct KindCase
    {
        std::string type;
        int kind;
    };

    const std::vector<KindCase> KINDS{ { "sinteger8", 0 }, { "sinteger16", 1 }, { "sinteger32", 2 }, { "sinteger64", 3 }, { "uinteger8", 4 }, { "uinteger16", 5 }, { "uinteger32", 6 }, { "uinteger64", 7 }, { "float32", 8 }, { "float64", 9 } };

    Lines MAREA_lowerFunction(const std::string &signature, const std::string &body, std::uint32_t *outRegisters = nullptr, const std::string &prefix = "")
    {
        MAREA_GenerateFixture fixture;

        if (!fixture.MAREA_generate("namespace main; " + prefix + " function f" + signature + " " + body + " end")) {
            return { "<generate failed>" };
        }

        const std::int64_t INDEX{ fixture.MAREA_findFunction("main.f") };

        if (INDEX < 0) {
            return { "<no function>" };
        }

        if (outRegisters != nullptr) {
            *outRegisters = fixture.MAREA_registerCount(static_cast<std::uint32_t>(INDEX));
        }

        return fixture.MAREA_instructions(static_cast<std::uint32_t>(INDEX));
    }

    std::string MAREA_join(const Lines &lines)
    {
        std::string result;

        for (const std::string &line : lines) {
            result += line + "; ";
        }

        return result;
    }
} // namespace

TEST(MAREABytecodeGenerate, RejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_BytecodeGenerate rejects a null argument and a node that is not a module." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_EQ(fixture.ir.MAREA_lower("namespace main; function f() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    MAREA_BytecodeModule *module{ nullptr };

    EXPECT_EQ(MAREA_BytecodeGenerate(nullptr, fixture.ir.module, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_BytecodeGenerate(fixture.context, nullptr, &module), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null IR module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeGenerate(fixture.context, fixture.ir.module, nullptr), MAREA_BYTECODE_EXCEPT_ERROR_NULL_POINTER) << "A null out module must be rejected.";

    EXPECT_EQ(MAREA_BytecodeGenerate(fixture.context, MAREA_irChild(fixture.ir.module, 0), &module), MAREA_BYTECODE_EXCEPT_ERROR_OUT_OF_RANGE) << "A node that is not a MODULE must be rejected.";

    EXPECT_EQ(module, nullptr) << "A rejected call must not give a module.";
}

TEST(MAREABytecodeGenerate, EmptyFunctionReturnsVoid)
{
    GTEST_COUT << "Validating a function with no body ends with return_void and uses no register." << std::endl;

    std::uint32_t registers{ 99 };

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("()", "", &registers)), "return_void 0 0 0; ") << "A void function must end with return_void.";

    EXPECT_EQ(registers, 0U) << "A function with no parameter and no value uses no register.";
}

TEST(MAREABytecodeGenerate, FunctionRecordHasNameParametersAndResult)
{
    GTEST_COUT << "Validating the function record: the full name, the parameter types and the result type." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace a.b; function add(left: sinteger32, right: float64): sinteger32 return left; end function nothing() end", "a/b.marea")) << "The unit must generate.";

    ASSERT_EQ(MAREA_BytecodeModuleGetFunctionCount(fixture.module), 2U) << "Two functions must be in the table, in source order.";

    EXPECT_EQ(fixture.MAREA_functionName(0), "a.b.add") << "The name is the namespace and the name.";

    EXPECT_EQ(fixture.MAREA_functionName(1), "a.b.nothing") << "The second function keeps its place.";

    MAREA_BytecodeFunctionInfo info{};

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(fixture.module, 0, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The function must be readable.";

    EXPECT_EQ(info.parameterCount, 2U) << "The function has two parameters.";

    EXPECT_EQ(info.frameArea, 0U) << "A scalar function has no frame area.";

    MAREA_BytecodeTypeInfo type{};

    ASSERT_EQ(MAREA_BytecodeModuleGetType(fixture.module, info.result, &type), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The result type must be in the table.";

    EXPECT_EQ(type.kind, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE) << "The result is a primitive.";

    EXPECT_EQ(type.size, 4U) << "sinteger32 is 4 bytes.";

    EXPECT_EQ(type.alignment, 4U) << "sinteger32 is aligned to 4 bytes.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_BytecodeModuleGetString(fixture.module, type.name, &text, &length), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The type name must be in the pool.";

    EXPECT_EQ(std::string(text, length), "sinteger32") << "The type has its full name.";

    MAREA_BytecodeParameterInfo parameter{};

    ASSERT_EQ(MAREA_BytecodeModuleGetParameter(fixture.module, 0, 1, &parameter), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The second parameter must be readable.";

    ASSERT_EQ(MAREA_BytecodeModuleGetType(fixture.module, parameter.type, &type), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The parameter type must be in the table.";

    EXPECT_EQ(type.size, 8U) << "float64 is 8 bytes.";

    EXPECT_EQ(parameter.flags, 0U) << "A scalar parameter has no flag.";

    ASSERT_EQ(MAREA_BytecodeModuleGetFunction(fixture.module, 1, &info), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The second function must be readable.";

    EXPECT_EQ(info.result, MAREA_BYTECODE_NONE) << "A function with no result type has no result.";
}

TEST(MAREABytecodeGenerate, ReturnOfASum)
{
    GTEST_COUT << "Validating the lowering of a + b with a return value." << std::endl;

    std::uint32_t registers{ 0 };

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32, b: sinteger32): sinteger32", "return a + b;", &registers)), "move 2 0 0; add 2 1 2; return 2 0 0; trap 6 0 0; ") << "The left operand is copied, then the typed add runs in place.";

    EXPECT_EQ(registers, 3U) << "Two parameters and one temporary.";
}

TEST(MAREABytecodeGenerate, ReturnOfANameAndOfALiteral)
{
    GTEST_COUT << "Validating a return of a parameter and of a literal." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "return a;")), "return 0 0 0; trap 6 0 0; ") << "A parameter is returned from its own register.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): sinteger32", "return 5;")), "load_imm 0 5 0; return 0 0 0; trap 6 0 0; ") << "A small literal is a 32-bit immediate.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): sinteger32", "return -2;")), "load_imm 0 65534 65535; return 0 0 0; trap 6 0 0; ") << "A negative literal is folded into one immediate.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): sinteger8", "return -128;")), "load_imm 0 65408 65535; return 0 0 0; trap 6 0 0; ") << "The lowest sinteger8 must be folded with no overflow.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): boolean", "return true;")), "load_imm 0 1 0; return 0 0 0; trap 6 0 0; ") << "true is 1.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): boolean", "return false;")), "load_imm 0 0 0; return 0 0 0; trap 6 0 0; ") << "false is 0.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): uinteger32", "return 0xFF_00;")), "load_imm 0 65280 0; return 0 0 0; trap 6 0 0; ") << "A hexadecimal literal with a separator.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): uinteger8", "return 0b1010_0101;")), "load_imm 0 165 0; return 0 0 0; trap 6 0 0; ") << "A binary literal.";
}

TEST(MAREABytecodeGenerate, WideConstantsGoToTheDataSection)
{
    GTEST_COUT << "Validating a constant that does not fit 32 bits is an 8-byte blob and is shared." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(): sinteger64 local a: sinteger64 = 5000000000; local b: sinteger64 = 5000000000; local c: uinteger32 = 4294967295; return a + b + convert<sinteger64>(c); end")) << "The unit must generate.";

    ASSERT_EQ(MAREA_BytecodeModuleGetDataCount(fixture.module), 2U) << "Two different wide values need two blobs.";

    EXPECT_EQ(fixture.MAREA_data(0), (std::vector<std::uint8_t>{ 0x00, 0xF2, 0x05, 0x2A, 0x01, 0x00, 0x00, 0x00 })) << "5000000000 in little endian.";

    EXPECT_EQ(fixture.MAREA_data(1), (std::vector<std::uint8_t>{ 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00 })) << "A uinteger32 is zero extended.";

    const Lines LINES{ fixture.MAREA_instructions(0) };

    ASSERT_GE(LINES.size(), 3U) << "The function must have code.";

    EXPECT_EQ(LINES[0], "load_data 0 0 0") << "The first constant loads blob 0.";

    EXPECT_EQ(LINES[1], "load_data 1 0 0") << "The same value loads the same blob.";

    EXPECT_EQ(LINES[2], "load_data 2 1 0") << "The next value loads blob 1.";
}

TEST(MAREABytecodeGenerate, FloatConstantsAreBlobsOfTheBits)
{
    GTEST_COUT << "Validating a float literal is the bits of a float64 in a blob, also for float32." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: float64, b: float32): float64 local x: float32 = 0.5; local y: float64 = a - 0.5; return y; end")) << "The unit must generate.";

    ASSERT_EQ(MAREA_BytecodeModuleGetDataCount(fixture.module), 1U) << "Both uses of 0.5 share one blob.";

    EXPECT_EQ(fixture.MAREA_data(0), (std::vector<std::uint8_t>{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3F })) << "0.5 as float64 bits.";

    EXPECT_EQ(MAREA_join(fixture.MAREA_instructions(0)), "load_data 2 0 0; move 3 0 0; load_data 4 0 0; sub 3 4 9; return 3 0 0; trap 6 0 0; ") << "A float32 local holds a float64 value that float32 can hold.";
}

TEST(MAREABytecodeGenerate, LocalsAssignmentsAndCompoundAssignments)
{
    GTEST_COUT << "Validating locals take a register, and an assignment changes the register of the name." << std::endl;

    std::uint32_t registers{ 0 };

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "local x: sinteger32 = a * 2; x += 1; x = x - a; return x;", &registers)), "move 1 0 0; load_imm 2 2 0; mul 1 2 2; load_imm 2 1 0; add 1 2 2; move 2 1 0; sub 2 0 2; move 1 2 0; return 1 0 0; trap 6 0 0; ") << "A local initializer is built in the register of the local.";

    EXPECT_EQ(registers, 3U) << "One parameter, one local and one temporary.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "local x: sinteger32 = a; local y: sinteger32 = x; return y;")), "move 1 0 0; move 2 1 0; return 2 0 0; trap 6 0 0; ") << "A name is copied by value.";
}

TEST(MAREABytecodeGenerate, EveryBinaryOperatorHasItsOpcodeForEveryKind)
{
    GTEST_COUT << "Validating each arithmetic operator against each number kind." << std::endl;

    struct Operator
    {
        std::string text;
        std::string opcode;
        bool integerOnly;
        bool floatOnly;
    };

    const std::vector<Operator> OPERATORS{ { "+", "add", false, false }, { "-", "sub", false, false }, { "*", "mul", false, false }, { "/", "div", false, false }, { "%", "rem", false, false }, { "**", "pow", false, false }, { "+%", "add", true, false }, { "-%", "sub", true, false }, { "*%", "mul", true, false }, { "+|", "sat_add", true, false }, { "-|", "sat_sub", true, false }, { "*|", "sat_mul", true, false } };

    std::size_t checked{ 0 };

    for (const Operator &op : OPERATORS) {
        for (const KindCase &kind : KINDS) {
            const bool FLOAT{ kind.kind >= 8 };

            if (op.integerOnly && FLOAT) {
                continue;
            }

            MAREA_GenerateFixture fixture;

            if (!fixture.MAREA_generate("namespace main; function f(a: " + kind.type + ", b: " + kind.type + "): " + kind.type + " return a " + op.text + " b; end")) {
                continue;
            }

            EXPECT_EQ(MAREA_join(fixture.MAREA_instructions(0)), "move 2 0 0; " + op.opcode + " 2 1 " + std::to_string(kind.kind) + "; return 2 0 0; trap 6 0 0; ") << op.text << " on " << kind.type;

            ++checked;
        }
    }

    EXPECT_GE(checked, 90U) << "Most pairs must be accepted by sema and checked.";
}

TEST(MAREABytecodeGenerate, BitwiseAndShiftOperators)
{
    GTEST_COUT << "Validating bitwise operators are three-register opcodes and shifts keep the kind of the left side." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: uinteger8, b: uinteger8): uinteger8", "return a & b;")), "move 2 0 0; bit_and 2 2 1; return 2 0 0; trap 6 0 0; ") << "and";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: uinteger8, b: uinteger8): uinteger8", "return a | b;")), "move 2 0 0; bit_or 2 2 1; return 2 0 0; trap 6 0 0; ") << "or";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: uinteger8, b: uinteger8): uinteger8", "return a ^ b;")), "move 2 0 0; bit_xor 2 2 1; return 2 0 0; trap 6 0 0; ") << "xor";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger16, b: uinteger8): sinteger16", "return a << b;")), "move 2 0 0; shl 2 1 1; return 2 0 0; trap 6 0 0; ") << "A left shift uses the kind of the left side.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: uinteger64): uinteger64", "return a >> 3;")), "move 1 0 0; load_imm 2 3 0; shr 1 2 7; return 1 0 0; trap 6 0 0; ") << "A right shift by a literal.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: uinteger64): uinteger64", "local x: uinteger64 = a; x <<= 2; x >>= 1; x &= 255; x |= 1; x ^= 3; return x;")), "move 1 0 0; load_imm 2 2 0; shl 1 2 7; load_imm 2 1 0; shr 1 2 7; load_imm 2 255 0; bit_and 1 1 2; load_imm 2 1 0; bit_or 1 1 2; load_imm 2 3 0; bit_xor 1 1 2; return 1 0 0; trap 6 0 0; ") << "Compound bit and shift assignments.";
}

TEST(MAREABytecodeGenerate, CompoundArithmeticAssignments)
{
    GTEST_COUT << "Validating each compound arithmetic assignment uses the opcode of its operator." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "local x: sinteger32 = a; x -= 1; x *= 2; x /= 3; x %= 4; x **= 2; return x;")), "move 1 0 0; load_imm 2 1 0; sub 1 2 2; load_imm 2 2 0; mul 1 2 2; load_imm 2 3 0; div 1 2 2; load_imm 2 4 0; rem 1 2 2; load_imm 2 2 0; pow 1 2 2; return 1 0 0; trap 6 0 0; ") << "-=, *=, /=, %= and **=.";
}

TEST(MAREABytecodeGenerate, ComparisonsPickTheOpcodeOfTheKind)
{
    GTEST_COUT << "Validating each comparison against signed, unsigned and float operands." << std::endl;

    struct Case
    {
        std::string type;
        std::string less;
        std::string lessEqual;
        std::string equal;
        std::string notEqual;
    };

    const std::vector<Case> CASES{ { "sinteger32", "lt_s", "le_s", "eq", "ne" }, { "uinteger16", "lt_u", "le_u", "eq", "ne" }, { "float64", "lt_f", "le_f", "eq_f", "ne_f" }, { "float32", "lt_f", "le_f", "eq_f", "ne_f" } };

    for (const Case &c : CASES) {
        const std::string SIGNATURE{ "(a: " + c.type + ", b: " + c.type + "): boolean" };

        EXPECT_EQ(MAREA_join(MAREA_lowerFunction(SIGNATURE, "return a < b;")), c.less + " 2 0 1; return 2 0 0; trap 6 0 0; ") << c.type << " <";

        EXPECT_EQ(MAREA_join(MAREA_lowerFunction(SIGNATURE, "return a > b;")), c.less + " 2 1 0; return 2 0 0; trap 6 0 0; ") << c.type << " > swaps the operands";

        EXPECT_EQ(MAREA_join(MAREA_lowerFunction(SIGNATURE, "return a <= b;")), c.lessEqual + " 2 0 1; return 2 0 0; trap 6 0 0; ") << c.type << " <=";

        EXPECT_EQ(MAREA_join(MAREA_lowerFunction(SIGNATURE, "return a >= b;")), c.lessEqual + " 2 1 0; return 2 0 0; trap 6 0 0; ") << c.type << " >= swaps the operands";

        EXPECT_EQ(MAREA_join(MAREA_lowerFunction(SIGNATURE, "return a == b;")), c.equal + " 2 0 1; return 2 0 0; trap 6 0 0; ") << c.type << " ==";

        EXPECT_EQ(MAREA_join(MAREA_lowerFunction(SIGNATURE, "return a != b;")), c.notEqual + " 2 0 1; return 2 0 0; trap 6 0 0; ") << c.type << " !=";
    }
}

TEST(MAREABytecodeGenerate, UnaryOperators)
{
    GTEST_COUT << "Validating minus, bit not, not, copy and move." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger16): sinteger16", "return -a;")), "move 1 0 0; neg 1 1 1; return 1 0 0; trap 6 0 0; ") << "neg";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: float32): float32", "return -a;")), "move 1 0 0; neg 1 1 8; return 1 0 0; trap 6 0 0; ") << "neg on a float";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: uinteger32): uinteger32", "return ~a;")), "move 1 0 0; bit_not 1 1 6; return 1 0 0; trap 6 0 0; ") << "bit not";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: boolean): boolean", "return not a;")), "move 1 0 0; not 1 1 0; return 1 0 0; trap 6 0 0; ") << "not";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "return copy a;")), "move 1 0 0; return 1 0 0; trap 6 0 0; ") << "copy of a scalar is a move";
}

TEST(MAREABytecodeGenerate, LogicalOperatorsShortCircuit)
{
    GTEST_COUT << "Validating and and or jump over the right side." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: boolean, b: boolean): boolean", "return a and b;")), "move 2 0 0; jump_if_not 2 1 0; move 2 1 0; return 2 0 0; trap 6 0 0; ") << "and";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: boolean, b: boolean): boolean", "return a or b;")), "move 2 0 0; jump_if 2 1 0; move 2 1 0; return 2 0 0; trap 6 0 0; ") << "or";
}

TEST(MAREABytecodeGenerate, ConvertBuiltinAndSameKind)
{
    GTEST_COUT << "Validating convert gives the source and the target kind, and a same-kind convert is a move." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(b: sinteger32): float64", "return convert<float64>(b);")), "convert 1 0 2306; return 1 0 0; trap 6 0 0; ") << "sinteger32 to float64: kind 2 in the low byte, kind 9 in the high byte.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(b: float64): sinteger8", "return convert<sinteger8>(b);")), "convert 1 0 9; return 1 0 0; trap 6 0 0; ") << "float64 to sinteger8: kind 9 and kind 0.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(b: sinteger32): sinteger32", "return convert<sinteger32>(b);")), "move 1 0 0; return 1 0 0; trap 6 0 0; ") << "The same kind is a move.";
}

TEST(MAREABytecodeGenerate, ConstantsAreInlinedAndBuildValueIsItsDefault)
{
    GTEST_COUT << "Validating a file constant is lowered where it is read, and build_value! is its default." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): sinteger32", "return LIMIT + 1;", nullptr, "constant LIMIT: sinteger32 = 7;")), "load_imm 0 7 0; load_imm 1 1 0; add 0 1 2; return 0 0 0; trap 6 0 0; ") << "A constant is read as its initializer.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): sinteger32", "return LIMIT;", nullptr, "constant BASE: sinteger32 = 4; constant LIMIT: sinteger32 = BASE * 2;")), "load_imm 0 4 0; load_imm 1 2 0; mul 0 1 2; return 0 0 0; trap 6 0 0; ") << "A constant that reads a constant.";

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(): sinteger32", "local x: sinteger32 = build_value!(\"MODE\", sinteger32, 3); return x;")), "load_imm 0 3 0; return 0 0 0; trap 6 0 0; ") << "The bytecode stage has no build settings, so the default is the value.";
}

TEST(MAREABytecodeGenerate, ParameterShadowedByALocalAndNestedScopes)
{
    GTEST_COUT << "Validating a name resolves to the innermost declaration and a scope frees its registers." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "local x: sinteger32 = 1; do local x: sinteger32 = 2; a = x; end return x;")), "load_imm 1 1 0; load_imm 2 2 0; move 3 2 0; move 0 3 0; return 1 0 0; trap 6 0 0; ") << "The inner x is a new register that is freed at the end of the scope.";
}

TEST(MAREABytecodeGenerate, UnreachableStatementTraps)
{
    GTEST_COUT << "Validating unreachable!() is a trap with its code." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("()", "unreachable!();")), "trap 5 0 0; return_void 0 0 0; ") << "unreachable traps with code 5.";
}

TEST(MAREABytecodeGenerate, UnsupportedTypeGivesADiagnosticAndNoModule)
{
    GTEST_COUT << "Validating a type that is not a scalar gives 5033 and no module." << std::endl;

    MAREA_GenerateFixture fixture;

    EXPECT_FALSE(fixture.MAREA_generate("namespace main; function f(text: string) end")) << "A string parameter cannot be lowered yet.";

    EXPECT_EQ(fixture.module, nullptr) << "No module is given when a function fails.";

    EXPECT_EQ(fixture.except, MAREA_BYTECODE_EXCEPT_SUCCESS) << "A diagnostic is not an exception.";

    ASSERT_EQ(fixture.MAREA_codes().size(), 1U) << "One diagnostic for the function.";

    EXPECT_EQ(fixture.MAREA_codes()[0], MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE) << "The code must be 5033.";
}

TEST(MAREABytecodeGenerate, UnsupportedConstructGivesADiagnostic)
{
    GTEST_COUT << "Validating a construct of a later slice gives 5030 and no module." << std::endl;

    MAREA_GenerateFixture fixture;

    EXPECT_FALSE(fixture.MAREA_generate("namespace main; structure Point x: sinteger32; end function f() local p: Point = Point{ x = 1 }; p.x = 2; end")) << "An assignment to a field is not lowered yet.";

    EXPECT_EQ(fixture.module, nullptr) << "No module is given.";

    ASSERT_EQ(fixture.MAREA_codes().size(), 1U) << "One diagnostic.";

    EXPECT_EQ(fixture.MAREA_codes()[0], MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT) << "The code must be 5030.";
}

TEST(MAREABytecodeGenerate, ForEachIsRefusedAsAnUnwalkableCollection)
{
    GTEST_COUT << "Validating a for each gives 5034 because no collection type can be walked yet." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_EQ(fixture.ir.MAREA_lower("namespace main; function f(n: sinteger32) for each v in n do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_BytecodeGenerate(fixture.context, fixture.ir.module, &fixture.module), MAREA_BYTECODE_EXCEPT_SUCCESS) << "A diagnostic is not an exception.";

    EXPECT_EQ(fixture.module, nullptr) << "No module is given.";

    ASSERT_EQ(fixture.MAREA_codes().size(), 1U) << "One diagnostic.";

    EXPECT_EQ(fixture.MAREA_codes()[0], MAREA_BYTECODE_DIAGNOSTIC_CODE_UNWALKABLE_COLLECTION) << "The code must be 5034.";
}

TEST(MAREABytecodeGenerate, EveryCaseOfAFunctionIsReportedOnceAndTheOthersStillRun)
{
    GTEST_COUT << "Validating two bad functions give two diagnostics, one for each." << std::endl;

    MAREA_GenerateFixture fixture;

    EXPECT_FALSE(fixture.MAREA_generate("namespace main; function a(text: string) end function b(): sinteger32 return 1; end function c(text: string) end")) << "Two functions are bad.";

    EXPECT_EQ(fixture.MAREA_codes().size(), 2U) << "One diagnostic for each bad function, none for the good one.";
}

TEST(MAREABytecodeGenerate, GeneratedModulesKeepTheRoundTripLaws)
{
    GTEST_COUT << "Validating a generated module survives the binary and the text round trip." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: float64, b: sinteger64): float64 local x: float64 = a * 2.5; local y: sinteger64 = b + 5000000000; return x; end")) << "The unit must generate.";

    const MAREA_Byte *bytes{ nullptr };

    MAREA_IntegerSize size{ 0 };

    ASSERT_EQ(MAREA_BytecodeSerialize(fixture.module, &bytes, &size), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The module must serialize.";

    MAREA_BytecodeModule *binary{ nullptr };

    ASSERT_EQ(MAREA_BytecodeDeserialize(fixture.context, bytes, size, &binary), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The bytes must read.";

    ASSERT_NE(binary, nullptr) << "A generated module must be accepted by the reader.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(fixture.module, binary)) << "The binary round trip must keep the module.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_BytecodeDisassemble(fixture.module, &text, &length), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The module must disassemble.";

    MAREA_BytecodeModule *assembled{ nullptr };

    ASSERT_EQ(MAREA_BytecodeAssemble(fixture.context, text, length, &assembled), MAREA_BYTECODE_EXCEPT_SUCCESS) << "The text must read.";

    ASSERT_NE(assembled, nullptr) << "The text of a generated module must be accepted.";

    EXPECT_TRUE(MAREA_BytecodeModuleEquals(fixture.module, assembled)) << "The text round trip must keep the module.";
}

namespace
{
    bool MAREA_jumpsStayInTheFunction(const MAREA_GenerateFixture &fixture, std::uint32_t functionIndex)
    {
        MAREA_BytecodeFunctionInfo info{};

        if (MAREA_BytecodeModuleGetFunction(fixture.module, functionIndex, &info) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return false;
        }

        for (std::uint32_t index{ 0 }; index < info.instructionCount; ++index) {
            MAREA_BytecodeInstruction instruction{};

            MAREA_IGNORE(MAREA_BytecodeModuleGetInstruction(fixture.module, functionIndex, index, &instruction));

            if ((instruction.opcode == MAREA_BYTECODE_OPCODE_JUMP) || (instruction.opcode == MAREA_BYTECODE_OPCODE_JUMP_IF) || (instruction.opcode == MAREA_BYTECODE_OPCODE_JUMP_IF_NOT)) {
                const std::int64_t TARGET{ static_cast<std::int64_t>(index) + 1 + static_cast<std::int16_t>(instruction.b) };

                if ((TARGET < 0) || (TARGET >= static_cast<std::int64_t>(info.instructionCount))) {
                    return false;
                }
            }
        }

        return true;
    }
} // namespace

TEST(MAREABytecodeGenerate, IfJumpsOverItsBlock)
{
    GTEST_COUT << "Validating an if without else jumps over the block when the condition is false." << std::endl;

    std::uint32_t registers{ 0 };

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: sinteger32): sinteger32", "if a < 0 then return 1; end return 2;", &registers)), "load_imm 2 0 0; lt_s 1 0 2; jump_if_not 1 2 0; load_imm 1 1 0; return 1 0 0; load_imm 1 2 0; return 1 0 0; trap 6 0 0; ") << "The jump lands on the first instruction after the block.";

    EXPECT_EQ(registers, 3U) << "The temporaries of the condition are freed before the block.";
}

TEST(MAREABytecodeGenerate, IfWithElseJumpsOverTheElse)
{
    GTEST_COUT << "Validating an if with an else: the block ends with a jump over the else." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(a: boolean): sinteger32", "if a then return 1; else return 2; end")), "jump_if_not 0 3 0; load_imm 1 1 0; return 1 0 0; jump 0 2 0; load_imm 1 2 0; return 1 0 0; trap 6 0 0; ") << "A boolean name is tested in its own register.";
}

TEST(MAREABytecodeGenerate, ElseIfChainJumpsToTheEnd)
{
    GTEST_COUT << "Validating an elseif chain: each arm that is taken jumps to the end of the chain." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(a: boolean, b: boolean): sinteger32 local x: sinteger32 = 0; if a then x = 1; elseif b then x = 2; else x = 3; end return x; end")) << "The unit must generate.";

    EXPECT_EQ(MAREA_join(fixture.MAREA_instructions(0)), "load_imm 2 0 0; jump_if_not 0 3 0; load_imm 3 1 0; move 2 3 0; jump 0 6 0; jump_if_not 1 3 0; load_imm 3 2 0; move 2 3 0; jump 0 2 0; load_imm 3 3 0; move 2 3 0; return 2 0 0; trap 6 0 0; ") << "Two jumps to the end, one for each arm that is not the last.";

    EXPECT_TRUE(MAREA_jumpsStayInTheFunction(fixture, 0)) << "Every jump must land inside the function.";
}

TEST(MAREABytecodeGenerate, WhileLoopsBackToItsCondition)
{
    GTEST_COUT << "Validating a while: the test, the exit jump, the body and the jump back." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(n: sinteger32): sinteger32", "local i: sinteger32 = 0; while i < n do i += 1; end return i;")), "load_imm 1 0 0; lt_s 2 1 0; jump_if_not 2 3 0; load_imm 2 1 0; add 1 2 2; jump 0 65531 0; return 1 0 0; trap 6 0 0; ") << "The jump back is a negative offset.";
}

TEST(MAREABytecodeGenerate, BreakAndContinue)
{
    GTEST_COUT << "Validating break leaves the loop and continue goes back to the start, also from an inner block." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(n: sinteger32): sinteger32", "local i: sinteger32 = 0; while true do i += 1; if i > n then break; end continue; end return i;")), "load_imm 1 0 0; load_imm 2 1 0; add 1 2 2; lt_s 2 0 1; jump_if_not 2 1 0; jump 0 2 0; jump 0 65530 0; jump 0 65529 0; return 1 0 0; trap 6 0 0; ") << "A while true has no test, and a break jumps to the instruction after the jump back.";
}

TEST(MAREABytecodeGenerate, NestedLoopsKeepTheirOwnBreaks)
{
    GTEST_COUT << "Validating a break of an inner loop leaves the inner loop only." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(n: sinteger32): sinteger32 local t: sinteger32 = 0; local i: sinteger32 = 0; while i < n do local j: sinteger32 = 0; while true do j += 1; if j > 3 then break; end t += j; end i += 1; if i > 100 then break; end end return t; end")) << "The unit must generate.";

    EXPECT_TRUE(MAREA_jumpsStayInTheFunction(fixture, 0)) << "Every jump must land inside the function.";
}

TEST(MAREABytecodeGenerate, DoWhileSwitchAndForRangeUseOnlyTheBasicJumps)
{
    GTEST_COUT << "Validating a do while, a switch and a for-range loop lower to jumps that stay in the function." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function f(n: sinteger32): sinteger32 local t: sinteger32 = 0; do t += 1; while t < n end for k in 1 .. n do if k == 3 then continue; end t += k; end switch n case 1, 2 then t += 10; case 3 then t += 20; default then t += 30; end return t; end")) << "The unit must generate.";

    EXPECT_TRUE(MAREA_jumpsStayInTheFunction(fixture, 0)) << "Every jump must land inside the function.";
}

TEST(MAREABytecodeGenerate, EnumLocalIsRefusedUntilTheNextSlice)
{
    GTEST_COUT << "Validating a local of an enum type gives 5033 because enums are a type of the next slice." << std::endl;

    MAREA_GenerateFixture fixture;

    const std::string SOURCE{ "namespace main; enum Light : sinteger32 red; green; end function f(n: sinteger32): sinteger32 local light: Light = Light.red; switch light case Light.red then return 1; case Light.green then return 2; end end" };

    EXPECT_FALSE(fixture.MAREA_generate(SOURCE)) << "A local of an enum type is not lowered yet.";

    ASSERT_FALSE(fixture.MAREA_codes().empty()) << "A diagnostic must be raised.";

    EXPECT_EQ(fixture.MAREA_codes()[0], MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE) << "An enum is a type of the next slice.";
}

TEST(MAREABytecodeGenerate, CallsAFunctionWithArgumentsAndUsesTheResult)
{
    GTEST_COUT << "Validating a call fills the window after the result register, then reads the result back." << std::endl;

    MAREA_GenerateFixture fixture;

    ASSERT_TRUE(fixture.MAREA_generate("namespace main; function add(a: sinteger32, b: sinteger32): sinteger32 return a + b; end function f(): sinteger32 return add(2, 3) + 1; end")) << "The unit must generate.";

    const std::int64_t ADD{ fixture.MAREA_findFunction("main.add") };
    const std::int64_t F{ fixture.MAREA_findFunction("main.f") };

    ASSERT_GE(ADD, 0) << "add must be in the function table.";
    ASSERT_GE(F, 0) << "f must be in the function table.";

    EXPECT_EQ(MAREA_join(fixture.MAREA_instructions(static_cast<std::uint32_t>(F))), "load_imm 2 2 0; load_imm 3 3 0; call 1 " + std::to_string(ADD) + " 2; move 0 1 0; load_imm 1 1 0; add 0 1 2; return 0 0 0; trap 6 0 0; ") << "The arguments fill the window, the call reads the function by its index, and the result is moved out.";

    EXPECT_EQ(fixture.MAREA_registerCount(static_cast<std::uint32_t>(F)), 4U) << "The window of the call is the widest point reached.";
}

TEST(MAREABytecodeGenerate, CallOfAVoidFunctionAsAStatementAndRecursion)
{
    GTEST_COUT << "Validating a void call as a statement moves nothing back, and a function may call itself." << std::endl;

    EXPECT_EQ(MAREA_join(MAREA_lowerFunction("(n: sinteger32)", "if n > 0 then f(n - 1); end")), "load_imm 2 0 0; lt_s 1 2 0; jump_if_not 1 4 0; move 3 0 0; load_imm 4 1 0; sub 3 4 2; call 2 0 1; return_void 0 0 0; ") << "A self call passes its own function index and no move follows a void result.";
}

TEST(MAREABytecodeGenerate, JumpOverTheLimitOfSixteenBitsIsADiagnostic)
{
    GTEST_COUT << "Validating a loop body that is longer than 32767 instructions gives 5032." << std::endl;

    std::string body;

    for (int index{ 0 }; index < 17000; ++index) {
        body += "t += 1; ";
    }

    MAREA_GenerateFixture fixture;

    EXPECT_FALSE(fixture.MAREA_generate("namespace main; function f(n: sinteger32): sinteger32 local t: sinteger32 = 0; while t < n do " + body + " end return t; end")) << "The exit jump of the loop is too far.";

    EXPECT_EQ(fixture.module, nullptr) << "No module is given.";

    ASSERT_EQ(fixture.MAREA_codes().size(), 1U) << "One diagnostic.";

    EXPECT_EQ(fixture.MAREA_codes()[0], MAREA_BYTECODE_DIAGNOSTIC_CODE_OPERAND_OVERFLOW) << "The code must be 5032.";
}
