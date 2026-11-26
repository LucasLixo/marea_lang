// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_DIVISION_BY_ZERO{ 1 };
    constexpr int MAREA_TRAP_SHIFT_OUT_OF_RANGE{ 2 };
    constexpr int MAREA_TRAP_NEGATIVE_EXPONENT{ 3 };
    constexpr int MAREA_TRAP_CONVERT_OUT_OF_RANGE{ 4 };
    constexpr int MAREA_TRAP_UNREACHABLE{ 5 };

    MAREA_ParityCall MAREA_two(const std::string &function, const std::string &result, const std::string &type, std::int64_t left, std::int64_t right)
    {
        return MAREA_parityCall(function, result, { MAREA_paritySigned(type, left), MAREA_paritySigned(type, right) });
    }

    MAREA_ParityCall MAREA_twoUnsigned(const std::string &function, const std::string &type, std::uint64_t left, std::uint64_t right)
    {
        return MAREA_parityCall(function, type, { MAREA_parityUnsigned(type, left), MAREA_parityUnsigned(type, right) });
    }
} // namespace

TEST(MAREACgenScalar, ModuleConstantsAreReadAsInTheVm)
{
    GTEST_COUT << "Validating a constant of the module, also one that reads another, has the same value in both backends." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityCheck("namespace main; constant LIMIT: sinteger32 = 10; constant DOUBLE: sinteger32 = LIMIT * 2; constant HALF: float32 = 0.5; function f(): sinteger32 return LIMIT + DOUBLE; end function g(): float32 return HALF + HALF; end", {}) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 30) << "10 + 20.";
}

TEST(MAREACgenScalar, ConstantsOfAnImplementationAreReadThroughTheType)
{
    GTEST_COUT << "Validating a constant declared in an implementation block is read through the name of the type." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; structure Gauge value: sinteger32; end implementation Gauge constant OFFSET: sinteger32 = 3; end function f(): sinteger32 return Gauge.OFFSET + 1; end", {}) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 4) << "3 + 1.";
}

TEST(MAREACgenScalar, ConvertTrapsExactlyWhereTheVmTraps)
{
    GTEST_COUT << "Validating convert traps for a value that the target cannot hold, in both backends, at the same limits." << std::endl;

    const std::string SOURCE{ "namespace main; function narrow(x: sinteger32): sinteger8 return convert<sinteger8>(x); end function shrink(x: sinteger32): uinteger16 return convert<uinteger16>(x); end function wide(x: uinteger64): sinteger64 return convert<sinteger64>(x); end function cut(x: float64): sinteger32 return convert<sinteger32>(x); end function single(x: sinteger64): float32 return convert<float32>(x); end function same(x: sinteger32): sinteger32 return convert<sinteger32>(x); end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("narrow", "sinteger8", { MAREA_parityInt(-128) }),
        MAREA_parityCall("narrow", "sinteger8", { MAREA_parityInt(127) }),
        MAREA_parityCall("narrow", "sinteger8", { MAREA_parityInt(128) }),
        MAREA_parityCall("narrow", "sinteger8", { MAREA_parityInt(-129) }),
        MAREA_parityCall("shrink", "uinteger16", { MAREA_parityInt(-1) }),
        MAREA_parityCall("shrink", "uinteger16", { MAREA_parityInt(65535) }),
        MAREA_parityCall("wide", "sinteger64", { MAREA_parityUnsigned("uinteger64", 9223372036854775807ULL) }),
        MAREA_parityCall("wide", "sinteger64", { MAREA_parityUnsigned("uinteger64", 9223372036854775808ULL) }),
        MAREA_parityCall("cut", "sinteger32", { MAREA_parityFloat("float64", -2.9) }),
        MAREA_parityCall("cut", "sinteger32", { MAREA_parityFloat("float64", 1.0e20) }),
        MAREA_parityCall("cut", "sinteger32", { MAREA_parityFloat("float64", 2147483647.5) }),
        MAREA_parityCall("cut", "sinteger32", { MAREA_parityFloat("float64", -2147483648.0) }),
        MAREA_parityCall("single", "float32", { MAREA_paritySigned("sinteger64", 16777217) }),
        MAREA_parityCall("same", "sinteger32", { MAREA_parityInt(7) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), -128) << "The lowest sinteger8 fits.";

    EXPECT_EQ(RESULT.generated[2].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "128 does not fit a sinteger8.";

    EXPECT_EQ(RESULT.generated[3].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "-129 does not fit a sinteger8.";

    EXPECT_EQ(RESULT.generated[4].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "-1 does not fit an unsigned type.";

    EXPECT_EQ(RESULT.generated[7].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "2^63 does not fit a sinteger64.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[8]), -2) << "A float is truncated toward zero.";

    EXPECT_EQ(RESULT.generated[9].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "1e20 does not fit a sinteger32.";

    EXPECT_EQ(MAREA_parityDouble(RESULT.generated[12]), 16777216.0) << "A float32 rounds the integer.";
}

TEST(MAREACgenScalar, ConvertToTheWidestIntegersTrapsAtTheLimit)
{
    GTEST_COUT << "Validating a float of exactly 2^63 or 2^64 does not fit a 64-bit integer, and the largest value below fits." << std::endl;

    const std::string SOURCE{ "namespace main; function s(x: float64): sinteger64 return convert<sinteger64>(x); end function u(x: float64): uinteger64 return convert<uinteger64>(x); end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("s", "sinteger64", { MAREA_parityFloat("float64", 9223372036854775808.0) }),
        MAREA_parityCall("u", "uinteger64", { MAREA_parityFloat("float64", 18446744073709551616.0) }),
        MAREA_parityCall("s", "sinteger64", { MAREA_parityFloat("float64", 9223372036854774784.0) }),
        MAREA_parityCall("u", "uinteger64", { MAREA_parityFloat("float64", 18446744073709549568.0) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "2^63 does not fit a sinteger64.";

    EXPECT_EQ(RESULT.generated[1].trap, MAREA_TRAP_CONVERT_OUT_OF_RANGE) << "2^64 does not fit a uinteger64.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[2]), 9223372036854774784) << "The largest double below 2^63.";

    EXPECT_EQ(RESULT.generated[3].bits, 18446744073709549568ULL) << "The largest double below 2^64.";
}

TEST(MAREACgenScalar, PowerTrapsOnANegativeExponentAsInTheVm)
{
    GTEST_COUT << "Validating a power wraps, traps on a negative exponent and works on floats as the vm does." << std::endl;

    const std::string SOURCE{ "namespace main; function power(a: sinteger32, b: sinteger32): sinteger32 return a ** b; end function upower(a: uinteger8, b: uinteger8): uinteger8 return a ** b; end function fpower(a: float64, b: float64): float64 return a ** b; end function spower(a: float32, b: float32): float32 return a ** b; end function compound(a: sinteger32): sinteger32 local x: sinteger32 = a; x **= 3; return x; end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_two("power", "sinteger32", "sinteger32", 2, 10),
        MAREA_two("power", "sinteger32", "sinteger32", -3, 3),
        MAREA_two("power", "sinteger32", "sinteger32", 2, -1),
        MAREA_two("power", "sinteger32", "sinteger32", 7, 100),
        MAREA_twoUnsigned("upower", "uinteger8", 3, 7),
        MAREA_parityCall("fpower", "float64", { MAREA_parityFloat("float64", 2.0), MAREA_parityFloat("float64", 0.5) }),
        MAREA_parityCall("spower", "float32", { MAREA_parityFloat("float32", 2.0), MAREA_parityFloat("float32", 0.5) }),
        MAREA_parityCall("compound", "sinteger32", { MAREA_parityInt(5) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 1024) << "2 to the 10th.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[1]), -27) << "A negative base with an odd exponent.";

    EXPECT_EQ(RESULT.generated[2].trap, MAREA_TRAP_NEGATIVE_EXPONENT) << "A negative exponent traps.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[4]), 139) << "2187 wraps to 139 in a uinteger8.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[7]), 125) << "The compound form.";
}

TEST(MAREACgenScalar, WrappingAndSaturatingOperatorsMatchTheVm)
{
    GTEST_COUT << "Validating the wrapping and the saturating forms give the vm results at the limits of each width." << std::endl;

    const std::string SOURCE{ "namespace main; function wrap(a: uinteger8, b: uinteger8): uinteger8 return (a +% b) -% (a *% b); end function narrow(a: sinteger8, b: sinteger8): sinteger8 return (a +| b) -| (a *| b); end function byte(a: uinteger8, b: uinteger8): uinteger8 return (a +| b) *| (a -| b); end function wide(a: sinteger64, b: sinteger64): sinteger64 return (a +| b) -| (a *| b); end function huge(a: uinteger64, b: uinteger64): uinteger64 return (a +| b) *| (a -| b); end function half(a: sinteger16, b: sinteger16): sinteger16 return a *| b; end function real(a: float64, b: float64): float64 return (a +| b) *| (a -| b); end" };

    const std::int64_t LOWEST{ std::numeric_limits<std::int64_t>::min() };
    const std::int64_t HIGHEST{ std::numeric_limits<std::int64_t>::max() };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_twoUnsigned("wrap", "uinteger8", 200, 100),
        MAREA_two("narrow", "sinteger8", "sinteger8", 127, 127),
        MAREA_two("narrow", "sinteger8", "sinteger8", -128, -128),
        MAREA_two("narrow", "sinteger8", "sinteger8", -128, 127),
        MAREA_twoUnsigned("byte", "uinteger8", 255, 255),
        MAREA_twoUnsigned("byte", "uinteger8", 3, 200),
        MAREA_two("wide", "sinteger64", "sinteger64", HIGHEST, 2),
        MAREA_two("wide", "sinteger64", "sinteger64", LOWEST, -1),
        MAREA_two("wide", "sinteger64", "sinteger64", LOWEST, 2),
        MAREA_two("wide", "sinteger64", "sinteger64", -3, 5),
        MAREA_twoUnsigned("huge", "uinteger64", 18446744073709551615ULL, 2),
        MAREA_twoUnsigned("huge", "uinteger64", 5, 7),
        MAREA_two("half", "sinteger16", "sinteger16", -32768, -32768),
        MAREA_two("half", "sinteger16", "sinteger16", -32768, 32767),
        MAREA_parityCall("real", "float64", { MAREA_parityFloat("float64", 3.5), MAREA_parityFloat("float64", 1.25) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();
}

TEST(MAREACgenScalar, SaturatingMultiplyOfUnsigned32StaysAtTheMaximum)
{
    GTEST_COUT << "Validating the saturating multiply of two large uinteger32 values stays at the maximum of the type." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; function f(a: uinteger32, b: uinteger32): uinteger32 return a *| b; end", { MAREA_twoUnsigned("f", "uinteger32", 4294967295ULL, 4294967295ULL) }) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].bits, 4294967295ULL) << "The product is far above the maximum.";
}

TEST(MAREACgenScalar, FloatRemainderMatchesTheVm)
{
    GTEST_COUT << "Validating the remainder of two floats, also in the compound form, is the vm result." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: float64, b: float64): float64 return a % b; end function g(a: float32, b: float32): float32 local x: float32 = a; x %= b; return x; end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "float64", { MAREA_parityFloat("float64", 7.5), MAREA_parityFloat("float64", 2.0) }),
        MAREA_parityCall("f", "float64", { MAREA_parityFloat("float64", -7.5), MAREA_parityFloat("float64", 2.0) }),
        MAREA_parityCall("f", "float64", { MAREA_parityFloat("float64", 1.0), MAREA_parityFloat("float64", 0.0) }),
        MAREA_parityCall("g", "float32", { MAREA_parityFloat("float32", 100.25), MAREA_parityFloat("float32", 0.3) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();
}

TEST(MAREACgenScalar, ShiftCountOfAnotherTypeTrapsAsInTheVm)
{
    GTEST_COUT << "Validating a shift reads the whole count, also when its type is wider than the shifted value." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: uinteger8, b: sinteger32): uinteger8 return a << b; end function g(a: sinteger32, b: uinteger64): sinteger32 local x: sinteger32 = a; x >>= b; return x; end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "uinteger8", { MAREA_parityUnsigned("uinteger8", 1), MAREA_parityInt(256) }),
        MAREA_parityCall("f", "uinteger8", { MAREA_parityUnsigned("uinteger8", 1), MAREA_parityInt(7) }),
        MAREA_parityCall("f", "uinteger8", { MAREA_parityUnsigned("uinteger8", 1), MAREA_parityInt(-1) }),
        MAREA_parityCall("g", "sinteger32", { MAREA_parityInt(-64), MAREA_parityUnsigned("uinteger64", 4) }),
        MAREA_parityCall("g", "sinteger32", { MAREA_parityInt(-64), MAREA_parityUnsigned("uinteger64", 4294967296ULL) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, MAREA_TRAP_SHIFT_OUT_OF_RANGE) << "256 is past the width of 8 bits.";

    EXPECT_EQ(RESULT.generated[4].trap, MAREA_TRAP_SHIFT_OUT_OF_RANGE) << "2^32 is past the width of 32 bits.";
}

TEST(MAREACgenScalar, OperandsRunInTheOrderOfTheSource)
{
    GTEST_COUT << "Validating the left operand runs first, so the first fault is the trap of the run in both compilers." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return (10 / a) + (1 << b); end function g(a: sinteger32, b: sinteger32): sinteger32 return (1 << b) + (10 / a); end function two(x: sinteger32, y: sinteger32): sinteger32 return x - y; end function h(a: sinteger32, b: sinteger32): sinteger32 return two(10 / a, 1 << b); end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_two("f", "sinteger32", "sinteger32", 0, 40),
        MAREA_two("g", "sinteger32", "sinteger32", 0, 40),
        MAREA_two("h", "sinteger32", "sinteger32", 0, 40),
        MAREA_two("h", "sinteger32", "sinteger32", 2, 2),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, MAREA_TRAP_DIVISION_BY_ZERO) << "The division is written first.";

    EXPECT_EQ(RESULT.generated[1].trap, MAREA_TRAP_SHIFT_OUT_OF_RANGE) << "The shift is written first.";

    EXPECT_EQ(RESULT.generated[2].trap, MAREA_TRAP_DIVISION_BY_ZERO) << "The first argument runs first.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[3]), 1) << "10 / 2 minus 1 << 2.";
}

TEST(MAREACgenScalar, ATrapInAConditionStopsTheFunction)
{
    GTEST_COUT << "Validating a fault in the test of an if, an elseif or a while stops the run, as in the vm." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: sinteger32): sinteger32 local n: sinteger32 = 0; if (10 / a) > 0 then n = 1; elseif (10 / (a - 1)) > 0 then n = 2; else n = 3; end while (10 / a) > n do n += 1; end return n; end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(0) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(1) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(-5) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(2) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, MAREA_TRAP_DIVISION_BY_ZERO) << "The test of the if traps.";

    EXPECT_EQ(RESULT.generated[2].trap, 0) << "No fault for -5.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[3]), 5) << "10 / 2 turns of the loop.";
}

TEST(MAREACgenScalar, ShortCircuitRunsTheRightOperandOnlyWhenNeeded)
{
    GTEST_COUT << "Validating and and or do not run their right operand when the left one decides, so no fault happens there." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: sinteger32): boolean return a != 0 and (10 / a) > 1; end function g(a: sinteger32): boolean return a == 0 or (10 / a) > 1; end" };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, {}) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();
}

TEST(MAREACgenScalar, UnreachableIsATrapAndNotACrash)
{
    GTEST_COUT << "Validating unreachable!() gives the unreachable trap of the vm instead of stopping the program." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: sinteger32): sinteger32 if a > 0 then return a; end unreachable!(); end function g(a: sinteger32) if a == 0 then unreachable!(); end end function h(a: sinteger32): sinteger32 g(a); return 9; end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(0) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(3) }),
        MAREA_parityCall("h", "sinteger32", { MAREA_parityInt(0) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, MAREA_TRAP_UNREACHABLE) << "The end of the function is reached.";

    EXPECT_EQ(RESULT.generated[2].trap, MAREA_TRAP_UNREACHABLE) << "A trap of a callee stops the caller.";
}

TEST(MAREACgenScalar, ProgramsThatSemaAcceptsCompileWithoutAWarning)
{
    GTEST_COUT << "Validating unused names and tests that are always true compile with every warning as an error." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; function f(unused: sinteger32, x: uinteger32): boolean local y: sinteger32 = 1; local z: sinteger32 = 2; z = 3; local w: boolean = x >= 0; return x < 0 or w; end", {}) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();
}

TEST(MAREACgenScalar, FloatLiteralsAndTheEdgeIntegersAreValidC)
{
    GTEST_COUT << "Validating literals at the edges of their types are written as valid C with the same value." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: float32): float32 return (a * 2.0) + 1.0; end function g(): sinteger32 local m: sinteger32 = -2147483648; return m; end function h(): sinteger64 return -9223372036854775808; end function i(): uinteger64 return 18446744073709551615; end function j(): float64 return 100000000000000000000.0; end function k(): sinteger8 return -128; end function l(): float32 return 0.1; end" };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, {}) };

    EXPECT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();
}

TEST(MAREACgenScalar, ANewLocalReadsTheOuterNameThatItHides)
{
    GTEST_COUT << "Validating the value of a new local reads the outer local of the same name, never itself." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityCheck("namespace main; function f(): sinteger32 local x: sinteger32 = 5; local r: sinteger32 = 0; do local x: sinteger32 = x + 1; r = x; end return (r * 10) + x; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 65) << "The inner x is 6, the outer one stays 5.";
}

TEST(MAREACgenScalar, MemberAndPlainFunctionNamesNeverCollide)
{
    GTEST_COUT << "Validating a function of a type and a plain function whose name joins the two stay two functions in C." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; structure Point x: sinteger32; end implementation Point function sum(self: Point): sinteger32 return self.x + 1; end end function Point_sum(p: Point): sinteger32 return p.x + 100; end function f(): sinteger32 local p: Point = Point{ x = 1 }; return (p.sum() * 1000) + Point_sum(p); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 2101) << "2 * 1000 + 101.";
}

TEST(MAREACgenScalar, AFunctionThatCannotBeWrittenIsLeftOutWhole)
{
    GTEST_COUT << "Validating a function that cgen cannot lower is not written at all, and the rest of the module is." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function bad(): sinteger32 local c: character = 'a'; return 1; end function good(): sinteger32 return 2; end"), MAREA_IR_EXCEPT_SUCCESS) << "The module must lower.";

    const MAREA_ParityModules MODULES{ MAREA_parityGenerate(fixture, nullptr) };

    EXPECT_FALSE(MODULES.cgenCodes.empty()) << "The character local must be reported.";

    EXPECT_EQ(MODULES.cText.find("f_bad("), MODULES.cText.rfind("f_bad(")) << "Only the declaration of bad may stay.";

    EXPECT_EQ(MAREA_parityCompileOnly(MODULES.cText), "") << "The rest of the module must be valid C.";
}

TEST(MAREACgenScalar, SameModuleWithCompositesGivesTheSameText)
{
    GTEST_COUT << "Validating two generations of one module with every composite give the same bytes." << std::endl;

    MAREA_IrFixture fixture(MAREA_parityLibraries());

    ASSERT_EQ(fixture.MAREA_lower("namespace main; require marea.array as array; require marea.array.Array; enum Shape Circle(radius: float64 = 1.0); Empty; end structure Point x: sinteger32; end structure Box points: marea.array.Array<sinteger32, 2>; shape: Shape; end function f(): float64 local p: marea.array.Array<Point, 2> = array.Array<Point, 2>{}; local b: Box = Box{ points = array.Array<sinteger32, 2>{}, shape = Shape.Circle(2.0) }; return b.shape.radius + convert<float64>(p[1].x); end"), MAREA_IR_EXCEPT_SUCCESS) << "The module must lower.";

    const MAREA_ParityModules FIRST{ MAREA_parityGenerate(fixture, nullptr) };

    const MAREA_ParityModules SECOND{ MAREA_parityGenerate(fixture, nullptr) };

    ASSERT_TRUE(FIRST.cgenOk) << "The module must generate: " << MAREA_parityCodes(FIRST.cgenCodes);

    EXPECT_EQ(FIRST.cText, SECOND.cText) << "The same module must give the same text.";
}
