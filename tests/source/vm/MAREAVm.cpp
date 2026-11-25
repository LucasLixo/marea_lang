// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREABytecodeHelpers.hpp"
#include "tests/MAREAVmHelpers.hpp"

#include <cmath>
#include <cstring>

namespace
{
    // INFO: a fixture that hand-builds a module through the public MAREA_BytecodeModule API (the way
    // MAREA_BytecodeFixture already does for the bytecode stage's own tests), then runs it on a real
    // MAREA_VmContext. Used for the composite opcodes and shapes no accepted program can write yet: a tuple or
    // an array has no shipped library to name one with (fixture descriptors stand in for them, as the apply's own
    // context says), and a tagged-union enum variant with data has no construction syntax lowered yet either.
    struct MAREA_RawVmFixture : MAREA_BytecodeFixture
    {
        MAREA_VmContext *context{ nullptr };

        MAREA_RawVmFixture()
        {
            MAREA_IGNORE(MAREA_VmContextCreate(arena, sink, &context));
        }

        MAREA_VmResult MAREA_run(MAREA_BytecodeModule *module, std::uint32_t functionIndex, const std::vector<std::uint64_t> &arguments = {})
        {
            MAREA_VmResult result{};

            MAREA_IGNORE(MAREA_VmCall(context, module, functionIndex, (arguments.empty() ? nullptr : arguments.data()), static_cast<std::uint32_t>(arguments.size()), &result));

            return result;
        }
    };
} // namespace

namespace
{
    double MAREA_asFloat64(std::uint64_t bits)
    {
        double value{ 0.0 };

        std::memcpy(&value, &bits, sizeof(value));

        return value;
    }

    std::uint64_t MAREA_fromFloat64(double value)
    {
        std::uint64_t bits{ 0 };

        std::memcpy(&bits, &value, sizeof(bits));

        return bits;
    }
} // namespace

TEST(MAREAVm, ContextCreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_VmContextCreate rejects a null arena and a null sink." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(static_cast<MAREA_IntegerSize>(4096), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "The arena must be created for the probe.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The sink must be created for the probe.";

    MAREA_VmContext *context{ nullptr };

    EXPECT_EQ(MAREA_VmContextCreate(nullptr, sink, &context), MAREA_VM_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_VmContextCreate(arena, nullptr, &context), MAREA_VM_EXCEPT_ERROR_NULL_POINTER) << "A null sink must be rejected.";

    EXPECT_EQ(MAREA_VmContextCreate(arena, sink, nullptr), MAREA_VM_EXCEPT_ERROR_NULL_POINTER) << "A null out context must be rejected.";

    ASSERT_EQ(MAREA_VmContextCreate(arena, sink, &context), MAREA_VM_EXCEPT_SUCCESS) << "A good call must succeed.";

    EXPECT_EQ(MAREA_VmContextGetArena(context), arena) << "The context must keep its arena.";

    EXPECT_EQ(MAREA_VmContextGetDiagnostics(context), sink) << "The context must keep its sink.";

    EXPECT_EQ(MAREA_VmContextGetArena(nullptr), nullptr) << "A null context gives a null arena.";

    EXPECT_EQ(MAREA_VmContextGetDiagnostics(nullptr), nullptr) << "A null context gives a null sink.";
}

TEST(MAREAVm, CallOfAnUnknownNameIsReported)
{
    GTEST_COUT << "Validating a call of a name no function of the module has gives MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 return 1; end")) << "The unit must generate.";

    MAREA_VmResult result{};

    EXPECT_EQ(MAREA_VmCallByName(fixture.context, fixture.generate.module, "main.missing", 12, nullptr, 0, &result), MAREA_VM_EXCEPT_ERROR_UNKNOWN_FUNCTION) << "No function has this name.";
}

TEST(MAREAVm, CallWithTheWrongArgumentCountIsReported)
{
    GTEST_COUT << "Validating a call that does not give one argument per parameter gives MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function add(a: sinteger32, b: sinteger32): sinteger32 return a + b; end")) << "The unit must generate.";

    MAREA_VmResult result{};

    const std::string NAME{ "main.add" };

    EXPECT_EQ(MAREA_VmCallByName(fixture.context, fixture.generate.module, NAME.c_str(), static_cast<MAREA_IntegerSize>(NAME.size()), nullptr, 0, &result), MAREA_VM_EXCEPT_ERROR_ARGUMENT_COUNT) << "add takes two arguments, not zero.";
}

TEST(MAREAVm, RunsIntegerExpressionsLocalsAndReturn)
{
    GTEST_COUT << "Validating integer expressions, a local and return give the expected value." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32, b: sinteger32): sinteger32 local x: sinteger32 = a * 2; return x + b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 3, 4 }) };

    EXPECT_FALSE(RESULT.trapped) << "A plain arithmetic function never traps.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 10) << "3 * 2 + 4 is 10.";
}

TEST(MAREAVm, IntegerAddWrapsOnOverflow)
{
    GTEST_COUT << "Validating a signed 8-bit add wraps instead of trapping, the C11 fixed width rule." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger8, b: sinteger8): sinteger8 return a + b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { static_cast<std::uint64_t>(static_cast<std::int64_t>(std::int8_t{ 100 })), static_cast<std::uint64_t>(static_cast<std::int64_t>(std::int8_t{ 100 })) }) };

    EXPECT_FALSE(RESULT.trapped) << "Overflow of a fixed-width integer wraps, it never traps.";

    EXPECT_EQ(static_cast<std::int8_t>(RESULT.value), static_cast<std::int8_t>(200)) << "100 + 100 wraps to -56 in sinteger8.";
}

TEST(MAREAVm, RunsFloatExpressions)
{
    GTEST_COUT << "Validating a float64 expression runs with the expected IEEE 754 result." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: float64, b: float64): float64 return (a + b) * 2.0; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { MAREA_fromFloat64(1.5), MAREA_fromFloat64(2.5) }) };

    EXPECT_FALSE(RESULT.trapped) << "No trap.";

    EXPECT_DOUBLE_EQ(MAREA_asFloat64(RESULT.value), 8.0) << "(1.5 + 2.5) * 2.0 is 8.0.";
}

TEST(MAREAVm, RunsIfAndElse)
{
    GTEST_COUT << "Validating if and else pick the right branch." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32): sinteger32 if a < 0 then return -1; else return 1; end end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { static_cast<std::uint64_t>(static_cast<std::int64_t>(-5)) }).value), -1) << "A negative value takes the if branch.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 5 }).value), 1) << "A positive value takes the else branch.";
}

TEST(MAREAVm, RunsAWhileLoopWithBreakAndContinue)
{
    GTEST_COUT << "Validating a while loop with break and continue sums only the even values below n." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(n: sinteger32): sinteger32 local i: sinteger32 = 0; local sum: sinteger32 = 0; while true do if i >= n then break; end i += 1; if (i % 2) != 0 then continue; end sum += i; end return sum; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 7 }).value), 12) << "2 + 4 + 6 is 12.";
}

TEST(MAREAVm, RunsADoWhileLoopAtLeastOnce)
{
    GTEST_COUT << "Validating a do while loop runs its body once even when the condition starts false." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(n: sinteger32): sinteger32 local count: sinteger32 = 0; do count += 1; while count < n end return count; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 0 }).value), 1) << "A false condition from the start still runs the body once.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 4 }).value), 4) << "The loop keeps running while the condition holds.";
}

TEST(MAREAVm, RunsASwitchWithDefault)
{
    GTEST_COUT << "Validating a switch picks the matching arm, or the default." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(n: sinteger32): sinteger32 switch n case 1, 2 then return 10; case 3 then return 20; default then return 30; end end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 1 }).value), 10) << "1 matches the first arm.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 3 }).value), 20) << "3 matches the second arm.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 9 }).value), 30) << "Anything else falls to the default.";
}

TEST(MAREAVm, RunsAForRangeLoop)
{
    GTEST_COUT << "Validating a for-range loop sums an inclusive range." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(last: sinteger32): sinteger32 local sum: sinteger32 = 0; for i in 1 .. last do sum += i; end return sum; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 5 }).value), 15) << "1 + 2 + 3 + 4 + 5 is 15, the range is inclusive.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 0 }).value), 0) << "first greater than last runs zero times.";
}

TEST(MAREAVm, CallsAnotherFunctionAndItself)
{
    GTEST_COUT << "Validating a call to another function and a recursive call give the expected result." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function double_it(a: sinteger32): sinteger32 return a * 2; end function factorial(n: sinteger32): sinteger32 if n <= 1 then return 1; else return n * factorial(n - 1); end end function sum_of_doubled(a: sinteger32, b: sinteger32): sinteger32 return double_it(a) + double_it(b); end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("sum_of_doubled", { 3, 4 }).value), 14) << "double_it(3) + double_it(4) is 6 + 8.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("factorial", { 6 }).value), 720) << "6! is 720, a recursive call of the same function.";
}

TEST(MAREAVm, CallWithABorrowedScalarParameterPassesByValue)
{
    GTEST_COUT << "Validating a read-only parameter of a scalar type, the only kind this slice can lower, is still passed by value." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function add_one(a: sinteger32): sinteger32 return a + 1; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("add_one", { 41 }).value), 42) << "A scalar parameter, borrowed or not in the language's own rule, is a value in a register.";
}

TEST(MAREAVm, DivisionByZeroTraps)
{
    GTEST_COUT << "Validating an integer division by zero is a defined trap, not a crash." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32, b: sinteger32): sinteger32 return a / b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 10, 0 }) };

    ASSERT_TRUE(RESULT.trapped) << "Division by zero must trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO) << "The trap code must say division by zero.";
}

TEST(MAREAVm, RemainderByZeroTraps)
{
    GTEST_COUT << "Validating a remainder by zero is also a division-by-zero trap." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: uinteger32, b: uinteger32): uinteger32 return a % b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 10, 0 }) };

    ASSERT_TRUE(RESULT.trapped) << "Remainder by zero must trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO) << "The trap code must say division by zero.";
}

TEST(MAREAVm, SignedDivisionOfTheLowestValueByMinusOneWraps)
{
    GTEST_COUT << "Validating the one signed division that would overflow wraps back to the same value instead of trapping." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger8, b: sinteger8): sinteger8 return a / b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { static_cast<std::uint64_t>(static_cast<std::int64_t>(std::int8_t{ -128 })), static_cast<std::uint64_t>(static_cast<std::int64_t>(std::int8_t{ -1 })) }) };

    EXPECT_FALSE(RESULT.trapped) << "The lowest sinteger8 divided by -1 wraps, it does not trap.";

    EXPECT_EQ(static_cast<std::int8_t>(RESULT.value), static_cast<std::int8_t>(-128)) << "-128 / -1 wraps back to -128 in sinteger8.";
}

TEST(MAREAVm, ShiftOutOfRangeTraps)
{
    GTEST_COUT << "Validating a shift whose count is not less than the width traps." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: uinteger8, b: uinteger8): uinteger8 return a << b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 1, 8 }) };

    ASSERT_TRUE(RESULT.trapped) << "A shift by 8 of an 8-bit kind must trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_SHIFT_OUT_OF_RANGE) << "The trap code must say shift out of range.";
}

TEST(MAREAVm, NegativeExponentTraps)
{
    GTEST_COUT << "Validating an integer power with a negative exponent traps." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32, b: sinteger32): sinteger32 return a ** b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 2, static_cast<std::uint64_t>(static_cast<std::int64_t>(-1)) }) };

    ASSERT_TRUE(RESULT.trapped) << "A negative exponent must trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_NEGATIVE_EXPONENT) << "The trap code must say negative exponent.";
}

TEST(MAREAVm, IntegerPowerWraps)
{
    GTEST_COUT << "Validating a non-saturating integer power wraps like a chain of multiplies." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32, b: sinteger32): sinteger32 return a ** b; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f", { 2, 10 }).value), 1024) << "2 ** 10 is 1024, well inside sinteger32.";
}

TEST(MAREAVm, ConvertOutOfRangeTraps)
{
    GTEST_COUT << "Validating a convert that does not fit the target type traps instead of truncating silently." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger32): sinteger8 return convert<sinteger8>(a); end")) << "The unit must generate.";

    const MAREA_VmResult GOOD{ fixture.MAREA_call("f", { 100 }) };

    EXPECT_FALSE(GOOD.trapped) << "100 fits a sinteger8.";

    EXPECT_EQ(static_cast<std::int8_t>(GOOD.value), 100) << "The value must survive the convert.";

    const MAREA_VmResult BAD{ fixture.MAREA_call("f", { 1000 }) };

    ASSERT_TRUE(BAD.trapped) << "1000 does not fit a sinteger8.";

    EXPECT_EQ(BAD.trap, MAREA_BYTECODE_TRAP_CODE_CONVERT_OUT_OF_RANGE) << "The trap code must say convert out of range.";
}

TEST(MAREAVm, UnreachableTraps)
{
    GTEST_COUT << "Validating unreachable!() is a defined trap, used by a switch with no arm that still returns through every case." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f() unreachable!(); end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    ASSERT_TRUE(RESULT.trapped) << "unreachable!() must trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_UNREACHABLE) << "The trap code must say unreachable.";
}

TEST(MAREAVm, SaturatingArithmeticClampsInsteadOfWrapping)
{
    GTEST_COUT << "Validating the saturating opcodes clamp to the limits of the kind." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger8, b: sinteger8): sinteger8 return a +| b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { static_cast<std::uint64_t>(static_cast<std::int64_t>(std::int8_t{ 100 })), static_cast<std::uint64_t>(static_cast<std::int64_t>(std::int8_t{ 100 })) }) };

    EXPECT_FALSE(RESULT.trapped) << "A saturating add never traps.";

    EXPECT_EQ(static_cast<std::int8_t>(RESULT.value), 127) << "100 +| 100 clamps to the maximum of sinteger8.";
}

TEST(MAREAVm, SaturatingMultiplyOfWideIntegersClamps)
{
    GTEST_COUT << "Validating a saturating multiply of two sinteger64 values clamps without the undefined behavior a plain 64-bit multiply would risk." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: sinteger64, b: sinteger64): sinteger64 return a *| b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { static_cast<std::uint64_t>(4000000000LL), static_cast<std::uint64_t>(4000000000LL) }) };

    EXPECT_FALSE(RESULT.trapped) << "A saturating multiply never traps.";

    EXPECT_EQ(static_cast<std::int64_t>(RESULT.value), MAREA_SINTEGER_64_MAXIMUM) << "4000000000 * 4000000000 overflows sinteger64 and clamps to its maximum.";
}

TEST(MAREAVm, UnsignedSaturatingMultiplyOfWideIntegersClamps)
{
    GTEST_COUT << "Validating a saturating multiply of two uinteger64 values clamps to the maximum." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(a: uinteger64, b: uinteger64): uinteger64 return a *| b; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { static_cast<std::uint64_t>(8000000000ULL), static_cast<std::uint64_t>(8000000000ULL) }) };

    EXPECT_FALSE(RESULT.trapped) << "A saturating multiply never traps.";

    EXPECT_EQ(RESULT.value, MAREA_UINTEGER_64_MAXIMUM) << "8000000000 * 8000000000 overflows uinteger64 and clamps to its maximum.";
}

TEST(MAREAVm, DeepRecursionTrapsWithStackOverflowInsteadOfCrashing)
{
    GTEST_COUT << "Validating a call chain deeper than the vm's own limit is a defined trap, never a crash." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(n: sinteger32): sinteger32 if n <= 0 then return 0; else return 1 + f(n - 1); end end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { 100000 }) };

    ASSERT_TRUE(RESULT.trapped) << "A call chain this deep must be a defined trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_STACK_OVERFLOW) << "The trap code must say stack overflow.";
}

namespace
{
    struct MAREA_VmConstructCase
    {
        std::string name;
        std::string source;
        std::vector<std::uint64_t> arguments;
        std::int64_t expected;
    };

    const std::vector<MAREA_VmConstructCase> MAREA_VM_CONSTRUCT_CASES{
        { "integer_expressions", "function g(a: sinteger32, b: sinteger32): sinteger32 return (a + b) * 2 - 1; end", { 3, 4 }, 13 },
        { "float_expressions", "function g(a: float64): sinteger32 return convert<sinteger32>(a * 2.0); end", { MAREA_fromFloat64(3.5) }, 7 },
        { "locals_and_return", "function g(a: sinteger32): sinteger32 local doubled: sinteger32 = a * 2; return doubled; end", { 5 }, 10 },
        { "if_statement", "function g(a: sinteger32): sinteger32 if a > 0 then return 1; end return 0; end", { 9 }, 1 },
        { "while_statement", "function g(n: sinteger32): sinteger32 local i: sinteger32 = 0; local total: sinteger32 = 0; while i < n do total += i; i += 1; end return total; end", { 5 }, 10 },
        { "do_while_statement", "function g(n: sinteger32): sinteger32 local i: sinteger32 = 0; do i += 1; while i < n end return i; end", { 3 }, 3 },
        { "switch_statement", "function g(a: sinteger32): sinteger32 switch a case 1 then return 100; default then return 200; end end", { 1 }, 100 },
        { "for_range_statement", "function g(last: sinteger32): sinteger32 local total: sinteger32 = 0; for i in 1 .. last do total += i; end return total; end", { 4 }, 10 },
        { "call_with_parameters", "function helper(a: sinteger32, b: sinteger32): sinteger32 return a - b; end function g(a: sinteger32, b: sinteger32): sinteger32 return helper(a, b); end", { 9, 4 }, 5 },
    };
} // namespace

TEST(MAREAVm, OneFunctionPerLanguageConstructOfTheSlicesRunsWithTheExpectedResult)
{
    GTEST_COUT << "Validating a table-driven run of at least one function per construct of slices 3 to 5." << std::endl;

    std::size_t checked{ 0 };

    for (const MAREA_VmConstructCase &vmCase : MAREA_VM_CONSTRUCT_CASES) {
        MAREA_VmFixture fixture;

        ASSERT_TRUE(fixture.MAREA_build(vmCase.source)) << vmCase.name << " must generate.";

        const MAREA_VmResult RESULT{ fixture.MAREA_call("g", vmCase.arguments) };

        EXPECT_FALSE(RESULT.trapped) << vmCase.name << " must not trap.";

        EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), vmCase.expected) << vmCase.name << " gave the wrong result.";

        ++checked;
    }

    EXPECT_EQ(checked, MAREA_VM_CONSTRUCT_CASES.size()) << "Every construct of the table must have run.";
}

TEST(MAREAVm, ZeroOpcodeClearsFrameMemoryOrARegister)
{
    GTEST_COUT << "Validating zero clears a byte range of the frame's own memory area, or the register itself when the byte count is 0." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };

    const std::uint32_t MEMORY_FORM{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 3, 8, 0) };

    fixture.MAREA_addInstruction(module, MEMORY_FORM, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 0, 0, 0);
    fixture.MAREA_addInstruction(module, MEMORY_FORM, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 1, 99, 0);
    fixture.MAREA_addInstruction(module, MEMORY_FORM, MAREA_BYTECODE_OPCODE_STORE, 0, 1, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64);
    fixture.MAREA_addInstruction(module, MEMORY_FORM, MAREA_BYTECODE_OPCODE_ZERO, 0, 8, 0);
    fixture.MAREA_addInstruction(module, MEMORY_FORM, MAREA_BYTECODE_OPCODE_LOAD, 2, 0, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64);
    fixture.MAREA_addInstruction(module, MEMORY_FORM, MAREA_BYTECODE_OPCODE_RETURN, 2, 0, 0);

    const MAREA_VmResult MEMORY_RESULT{ fixture.MAREA_run(module, MEMORY_FORM) };

    EXPECT_FALSE(MEMORY_RESULT.trapped) << "Clearing a byte range must not trap.";

    EXPECT_EQ(MEMORY_RESULT.value, 0U) << "The 8 bytes that held 99 must read back as 0.";

    const std::uint32_t REGISTER_FORM{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 1, 0, 0) };

    fixture.MAREA_addInstruction(module, REGISTER_FORM, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 0, 42, 0);
    fixture.MAREA_addInstruction(module, REGISTER_FORM, MAREA_BYTECODE_OPCODE_ZERO, 0, 0, 0);
    fixture.MAREA_addInstruction(module, REGISTER_FORM, MAREA_BYTECODE_OPCODE_RETURN, 0, 0, 0);

    const MAREA_VmResult REGISTER_RESULT{ fixture.MAREA_run(module, REGISTER_FORM) };

    EXPECT_FALSE(REGISTER_RESULT.trapped) << "Clearing a register must not trap.";

    EXPECT_EQ(REGISTER_RESULT.value, 0U) << "A byte count of 0 must clear the register itself, not memory.";
}

TEST(MAREAVm, AddressOfFieldLoadStoreRoundTripAStructure)
{
    GTEST_COUT << "Validating addr_of, field, load and store build and read a two-field structure in the frame's own memory." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 6, 8, 0) };

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 0, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 1, 10, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 0, 1, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_FIELD, 2, 0, 4);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 3, 20, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 2, 3, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 4, 0, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 5, 2, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADD, 4, 5, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 4, 0, 0);

    const MAREA_VmResult RESULT{ fixture.MAREA_run(module, FUNCTION) };

    EXPECT_FALSE(RESULT.trapped) << "A plain field round trip must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 30) << "x (10) plus y (20), each stored then loaded back through its own field address, must be 30.";
}

TEST(MAREAVm, CopyBytesDuplicatesAStructureIndependently)
{
    GTEST_COUT << "Validating copy_bytes makes an independent duplicate: a later store into the source never reaches the copy." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 4, 16, 0) };

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 0, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 1, 8, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 2, 5, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 0, 2, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_COPY_BYTES, 1, 0, 8);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 2, 99, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 0, 2, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 3, 1, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 3, 0, 0);

    const MAREA_VmResult RESULT{ fixture.MAREA_run(module, FUNCTION) };

    EXPECT_FALSE(RESULT.trapped) << "A copy of a structure must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 5) << "The copy must keep the value of the field as it was at copy time (5), unaffected by the later store of 99 into the source.";
}

TEST(MAREAVm, MoveSemanticsZeroTheSourceAfterCopyBytes)
{
    GTEST_COUT << "Validating the copy_bytes then zero pair of move leaves the destination with the value and the source at all zero." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 6, 16, 0) };

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 0, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 1, 8, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 2, 7, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 0, 2, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_COPY_BYTES, 1, 0, 8);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ZERO, 0, 8, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 3, 0, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 4, 1, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 5, 100, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_MUL, 3, 5, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADD, 3, 4, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 3, 0, 0);

    const MAREA_VmResult RESULT{ fixture.MAREA_run(module, FUNCTION) };

    EXPECT_FALSE(RESULT.trapped) << "A move must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 7) << "source*100 + destination must be 7: the source reads as 0 after the move zeroes it, and the destination keeps the moved value (7).";
}

TEST(MAREAVm, IndexOutOfRangeTrapsOnABoundsCheckedArray)
{
    GTEST_COUT << "Validating a bounds check built from lt_u, jump_if_not and trap gives a defined index-out-of-range trap for an index at or past the length, and no trap for one inside it." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t INTEGER{ fixture.MAREA_addType(module, MAREA_BYTECODE_TYPE_KIND_PRIMITIVE, fixture.MAREA_addString(module, "sinteger32"), 4, 4) };
    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 10, 16, 0) };

    fixture.MAREA_addParameter(module, FUNCTION, INTEGER, 0);

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 3, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 1, 4, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LESS_UNSIGNED, 2, 0, 1);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, 2, 2, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 9, 3, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 9, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_TRAP, MAREA_BYTECODE_TRAP_CODE_INDEX_OUT_OF_RANGE, 0, 0);

    const MAREA_VmResult INSIDE{ fixture.MAREA_run(module, FUNCTION, { 2 }) };

    EXPECT_FALSE(INSIDE.trapped) << "An index inside the length (4) must not trap.";

    const MAREA_VmResult OUTSIDE{ fixture.MAREA_run(module, FUNCTION, { 10 }) };

    ASSERT_TRUE(OUTSIDE.trapped) << "An index at or past the length must trap.";

    EXPECT_EQ(OUTSIDE.trap, MAREA_BYTECODE_TRAP_CODE_INDEX_OUT_OF_RANGE) << "The trap code must say index out of range.";
}

TEST(MAREAVm, LoadDataBlockCopiesABlobIntoFrameMemory)
{
    GTEST_COUT << "Validating load_data_block copies a blob of the read-only data section into the frame's own memory, byte for byte." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t BLOB{ fixture.MAREA_addData(module, { 0x01, 0x02, 0x03, 0x04 }) };
    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 2, 4, 0) };

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 0, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_DATA_BLOCK, 0, static_cast<std::uint16_t>(BLOB), 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 1, 0, MAREA_BYTECODE_NUMBER_KIND_UINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 1, 0, 0);

    const MAREA_VmResult RESULT{ fixture.MAREA_run(module, FUNCTION) };

    EXPECT_FALSE(RESULT.trapped) << "Reading a blob into memory must not trap.";

    EXPECT_EQ(static_cast<std::uint32_t>(RESULT.value), 0x04030201U) << "The 4 bytes of the blob, little-endian, must read back as 0x04030201.";
}

TEST(MAREAVm, CallingTheSameFunctionTwiceRunsBothCallsToCompletion)
{
    GTEST_COUT << "Validating that calling the same function twice in one body (the shape of two destroy calls on the same kind of value) runs both to completion deterministically, never a crash or an undefined second run." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t UNIT{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 1, 0, 0) };

    fixture.MAREA_addInstruction(module, UNIT, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 0, 1, 0);
    fixture.MAREA_addInstruction(module, UNIT, MAREA_BYTECODE_OPCODE_RETURN, 0, 0, 0);

    const std::uint32_t MAIN{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 3, 0, 0) };

    fixture.MAREA_addInstruction(module, MAIN, MAREA_BYTECODE_OPCODE_CALL, 0, static_cast<std::uint16_t>(UNIT), 0);
    fixture.MAREA_addInstruction(module, MAIN, MAREA_BYTECODE_OPCODE_MOVE, 1, 0, 0);
    fixture.MAREA_addInstruction(module, MAIN, MAREA_BYTECODE_OPCODE_CALL, 0, static_cast<std::uint16_t>(UNIT), 0);
    fixture.MAREA_addInstruction(module, MAIN, MAREA_BYTECODE_OPCODE_ADD, 1, 0, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, MAIN, MAREA_BYTECODE_OPCODE_RETURN, 1, 0, 0);

    const MAREA_VmResult RESULT{ fixture.MAREA_run(module, MAIN) };

    EXPECT_FALSE(RESULT.trapped) << "Two calls of the same function must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 2) << "Each of the two calls must run to completion once: 1 + 1 is 2, never a crash or a skipped second run.";
}

TEST(MAREAVm, EnumTaggedUnionConstructReadAndSwitchDispatch)
{
    GTEST_COUT << "Validating the tag-plus-union shape of an enum with data: the zero rule, a tag store, a payload store at the payload offset, a tag read and a dispatch on it, and a payload read of the active variant." << std::endl;

    MAREA_RawVmFixture fixture;
    MAREA_BytecodeModule *module{ fixture.MAREA_newModule() };
    const std::uint32_t FUNCTION{ fixture.MAREA_addFunction(module, MAREA_BYTECODE_NONE, MAREA_BYTECODE_NONE, 7, 8, 0) };

    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ADDRESS_OF, 0, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_ZERO, 0, 8, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 1, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 0, 1, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_FIELD, 2, 0, 4);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 3, 7, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_STORE, 2, 3, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 4, 0, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_EQUAL, 5, 4, 1);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_JUMP_IF_NOT, 5, 2, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD, 6, 2, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 6, 0, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE, 6, 999, 0);
    fixture.MAREA_addInstruction(module, FUNCTION, MAREA_BYTECODE_OPCODE_RETURN, 6, 0, 0);

    const MAREA_VmResult RESULT{ fixture.MAREA_run(module, FUNCTION) };

    EXPECT_FALSE(RESULT.trapped) << "Building and reading a tagged-union value must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 7) << "The tag (0) must match the first arm, so the payload field (7) of the active variant, read at the payload offset, must come back.";
}

TEST(MAREAVm, BuildsAStructureLiteralAndReadsItsFields)
{
    GTEST_COUT << "Validating a structure literal with no construct operator zero-fills, stores each named field, and that a field read gives the stored value back, from real language source." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Point x: sinteger32; y: sinteger32; end function f(): sinteger32 local p: Point = Point{ x = 3, y = 4 }; return p.x + p.y; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    EXPECT_FALSE(RESULT.trapped) << "Building and reading a plain structure must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 7) << "x (3) plus y (4) must be 7.";
}

TEST(MAREAVm, CopiesAStructureLocalByValue)
{
    GTEST_COUT << "Validating `local b: Point = copy a;` copies the bytes of a structure local into a new one, from real language source." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Point x: sinteger32; y: sinteger32; end function f(): sinteger32 local a: Point = Point{ x = 1, y = 2 }; local b: Point = copy a; return (b.x * 10) + b.y; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    EXPECT_FALSE(RESULT.trapped) << "Copying a structure must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 12) << "The copy must read x (1) and y (2) back: 1 * 10 + 2 is 12.";
}

TEST(MAREAVm, MovesAStructureLocalIntoANewOne)
{
    GTEST_COUT << "Validating `local b: Point = move a;` builds the new local with the value of a, from real language source." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Point x: sinteger32; y: sinteger32; end function f(): sinteger32 local a: Point = Point{ x = 7, y = 8 }; local b: Point = move a; return b.x + b.y; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    EXPECT_FALSE(RESULT.trapped) << "Moving a structure must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 15) << "The moved-to local must read x (7) and y (8) back: 7 + 8 is 15.";
}

TEST(MAREAVm, NestedStructureFieldsConstructAndReadThroughAChain)
{
    GTEST_COUT << "Validating a structure field that is itself a structure is zero-filled and field-stored in the same pass as the outer one, and that a chain of field reads (bx.inner.x) reaches it, from real language source." << std::endl;

    MAREA_VmFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("structure Point x: sinteger32; y: sinteger32; end structure Box inner: Point; count: sinteger32; end function f(): sinteger32 local bx: Box = Box{ inner = Point{ x = 5, y = 6 }, count = 9 }; return bx.inner.x + bx.inner.y + bx.count; end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    EXPECT_FALSE(RESULT.trapped) << "Building and reading a structure nested in another must not trap.";

    EXPECT_EQ(static_cast<std::int32_t>(RESULT.value), 20) << "inner.x (5) plus inner.y (6) plus count (9) must be 20.";
}
