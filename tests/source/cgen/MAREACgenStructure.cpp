// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    const std::string MAREA_POINTS{ "namespace main; structure Point x: sinteger32; y: sinteger32; end structure Box inner: Point; count: sinteger32; end " };
} // namespace

TEST(MAREACgenStructure, LocalsLiteralsFieldsCopiesAndMovesMatchTheVm)
{
    GTEST_COUT << "Validating a structure local, its literal, its field reads, copy, move and assignment are the same in both backends." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_POINTS + "function f(a: sinteger32): sinteger32 local p: Point = Point{ x = a, y = 2 }; local b: Box = Box{ inner = Point{ x = 3, y = 4 }, count = 5 }; local q: Point = copy p; p = Point{ x = p.y, y = p.x }; local r: Point = copy b.inner; local m: Point = move q; return (p.x * 10000) + (p.y * 1000) + (b.inner.x * 100) + (r.y * 10) + m.x; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f((int32_t)(1LL))")), 21341) << "The assignment reads the old value of p.";
}

TEST(MAREACgenStructure, FieldsAreAssignedAndCombined)
{
    GTEST_COUT << "Validating a field, also a field of a field, is assigned and combined in place." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; structure Point x: float64; y: float64; end structure Box inner: Point; count: sinteger32; end function f(): float64 local p: Point = Point{ x = 0.0, y = 0.0 }; p.x = 5.0; p.y += 1.0; local b: Box = Box{ inner = Point{ x = 1.0, y = 1.0 }, count = 1 }; b.inner.x = p.x * 2.0; b.inner.y -= p.y; return (b.inner.x * 100.0) + b.inner.y + p.x + p.y; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityDouble(RESULT.MAREA_outcome("f()")), 1006.0) << "1000 + 0 + 5 + 1.";
}

TEST(MAREACgenStructure, ParametersAreBorrowsAndResultsComeBackByValue)
{
    GTEST_COUT << "Validating a read-only and a write borrow of a structure and a structure result between functions of the unit." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_POINTS + "function read(#[semantics = reference_read_only] p: Point): sinteger32 return p.x + p.y; end function write(#[semantics = reference_mutable] p: Point) p.x = p.x + 10; end function forward(p: Point): sinteger32 return read(p); end function make(a: sinteger32): Point return Point{ x = a, y = a * 2 }; end function f(): sinteger32 local p: Point = make(1); write(p); write(p); local s: sinteger32 = forward(p); local q: Point = make(read(p)); return (s * 1000) + read(Point{ x = 1, y = 1 }) + read(make(3)) + q.y; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 23057) << "23 * 1000 + 2 + 9 + 46.";
}

TEST(MAREACgenStructure, MethodsStaticFunctionsAndSelfAreCalled)
{
    GTEST_COUT << "Validating a method, a static function, a write borrow of self and Self in an implementation block." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; structure Point x: sinteger32; y: sinteger32; end implementation Point constant ORIGIN_X: sinteger32 = 3; function make(x: sinteger32 = 1): Self return Self{ x = x, y = Point.ORIGIN_X }; end function sum(self: Point): sinteger32 return self.x + self.y; end function bump(#[semantics = reference_mutable] self: Point, by: sinteger32) self.x += by; end function twice(self: Point): sinteger32 return self.sum() * 2; end end function f(): sinteger32 local p: Point = Point.make(); p.bump(2); Point.bump(p, 1); local q: Point = Point.make(2); return (p.sum() * 10000) + (q.twice() * 100) + p.x; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 71004) << "p is (4, 3) and q is (2, 3).";
}

TEST(MAREACgenStructure, ConstantsAreBuiltWhereTheyAreRead)
{
    GTEST_COUT << "Validating a constant of a structure type gives a fresh value, so a copy can be changed." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_POINTS + "constant ORIGIN: Point = Point{ x = 5, y = 6 }; function f(): sinteger32 local p: Point = copy ORIGIN; p.x = 100; return (p.x * 100) + ORIGIN.x + ORIGIN.y; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 10011) << "The constant keeps 5 and 6.";
}

TEST(MAREACgenStructure, PackAndAlignKeepTheLayoutOfSema)
{
    GTEST_COUT << "Validating a packed and an aligned structure are written with the layout of sema, which C asserts." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_POINTS + "#[layout = c, pack = 1] structure Packed a: uinteger8; b: uinteger32; inner: Point; end #[align = 16] structure Wide a: uinteger8; end function read(p: Point): sinteger32 return p.x + p.y; end function f(): sinteger32 local s: Packed = Packed{ a = 1, b = 2, inner = Point{ x = 3, y = 4 } }; s.inner.y = 40; local w: Wide = Wide{ a = 7 }; return (s.inner.y * 100) + convert<sinteger32>(s.b) + convert<sinteger32>(w.a) + read(s.inner); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 4052) << "4000 + 2 + 7 + 43.";

    EXPECT_NE(RESULT.cText.find("_Static_assert(sizeof(marea_s_Packed) == 13"), std::string::npos) << "The size of sema must be asserted.";

    EXPECT_NE(RESULT.cText.find("_Static_assert(_Alignof(marea_s_Wide) == 16"), std::string::npos) << "The alignment of sema must be asserted.";
}

TEST(MAREACgenStructure, ATrapInAFunctionWithAStructureResultStopsTheCaller)
{
    GTEST_COUT << "Validating a fault inside a function that returns a structure stops the run like any other call." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_POINTS + "function make(a: sinteger32): Point return Point{ x = 10 / a, y = 1 }; end function f(a: sinteger32): sinteger32 local p: Point = make(a); return p.x; end", { MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(0) }), MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(5) }) }) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].trap, 1) << "The division by zero is the trap of the run.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[1]), 2) << "10 / 5.";
}

TEST(MAREACgenStructure, AStructureWithAnUndecidedLayoutIsReported)
{
    GTEST_COUT << "Validating a structure whose layout sema cannot measure is a diagnostic and not a wrong layout." << std::endl;

    EXPECT_EQ(MAREA_parityRejected("namespace main; structure S text: string; end function f(): sinteger32 local a: S = S{ text = \"x\" }; return 1; end").count(MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE), 1U) << "A string field has no layout.";
}

TEST(MAREACgenStructure, AWriteBorrowOfAMemberOfAPackedStructureReachesIt)
{
    GTEST_COUT << "Validating a write borrow of a misaligned member passes an aligned copy and writes it back." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_POINTS + "#[layout = c, pack = 1] structure Packed a: uinteger8; inner: Point; end function bump(#[semantics = reference_mutable] p: Point) p.x += 10; p.y += 1; end function f(): sinteger32 local s: Packed = Packed{ a = 1, inner = Point{ x = 3, y = 4 } }; bump(s.inner); bump(s.inner); return (s.inner.x * 100) + s.inner.y + convert<sinteger32>(s.a); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 2307) << "x is 23, y is 6 and a is 1.";
}
