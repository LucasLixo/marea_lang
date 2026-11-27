// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>

namespace
{
    const std::string MAREA_PRELUDE{ "namespace main; require marea.tuple; require marea.array; require marea.vector; structure Point x: sinteger32; y: sinteger32; end " };

    const std::string MAREA_PAIR{ "marea.tuple.Tuple<sinteger32, boolean>" };

    MAREA_ParityResult MAREA_run(const std::string &body)
    {
        return MAREA_parityCheck(MAREA_PRELUDE + body, {});
    }
} // namespace

TEST(MAREACgenTupleLibrary, ElementsAreReadAndWrittenByAConstantIndex)
{
    GTEST_COUT << "Validating each element keeps its own type and a write changes only that element." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 local t: " + MAREA_PAIR + " = " + MAREA_PAIR + "{ 4, true }; t[0] += 3; t[1] = false; if t[1] then return 0; end return t[0]; end function g(): uinteger64 local t: marea.tuple.Tuple<uinteger8, uinteger64, uinteger16> = marea.tuple.Tuple<uinteger8, uinteger64, uinteger16>{ 255, 5000000000, 65535 }; return convert<uinteger64>(t[0]) + t[1] + convert<uinteger64>(t[2]); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 7) << "The written elements.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 5000065790LL) << "Each width is kept.";
}

TEST(MAREACgenTupleLibrary, EqualityComparesElementByElement)
{
    GTEST_COUT << "Validating == and != compare each element in order, also a float that is not a number." << std::endl;

    const std::string TRIPLE{ "marea.tuple.Tuple<sinteger32, sinteger32, sinteger32>" };

    const std::string SMALL{ TRIPLE + "{ 1, 2, 3 }" };

    const std::string BIG{ TRIPLE + "{ 3, 2, 1 }" };

    const std::string FLOATS{ "marea.tuple.Tuple<float64, float64>" };

    const MAREA_ParityResult RESULT{ MAREA_run("function same(): boolean local a: " + TRIPLE + " = " + SMALL + "; local b: " + TRIPLE + " = " + SMALL + "; return a == b and not (a != b); end function other(): boolean local a: " + TRIPLE + " = " + SMALL + "; local b: " + TRIPLE + " = " + BIG + "; return a != b and not (a == b); end function last(): boolean local a: " + TRIPLE + " = " + SMALL + "; local b: " + TRIPLE + " = " + SMALL + "; b[2] = 4; return a != b; end function nan(): boolean local zero: float64 = 0.0; local a: " + FLOATS + " = " + FLOATS + "{ 1.0, zero / zero }; local b: " + FLOATS + " = copy a; return a != b and not (a == b); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("same()")), 1) << "Equal tuples.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("other()")), 1) << "Different tuples.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("last()")), 1) << "The last element counts.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("nan()")), 1) << "A float that is not a number is not equal to itself.";
}

TEST(MAREACgenTupleLibrary, ATupleGoesThroughCallsAndHoldsOtherTypes)
{
    GTEST_COUT << "Validating a returned tuple, a nested tuple and a tuple of a structure and an array." << std::endl;

    const std::string RANGE{ "marea.tuple.Tuple<sinteger32, sinteger32>" };

    const std::string INNER{ "marea.tuple.Tuple<Point, marea.array.Array<sinteger32, 2>>" };

    const std::string OUTER{ "marea.tuple.Tuple<" + RANGE + ", " + INNER + ">" };

    const MAREA_ParityResult RESULT{ MAREA_run("function bounds(a: sinteger32, b: sinteger32): " + RANGE + " if a < b then return " + RANGE + "{ a, b }; end return " + RANGE + "{ b, a }; end function f(): sinteger32 local t: " + RANGE + " = bounds(9, 4); return (t[0] * 10) + t[1]; end function g(): sinteger32 local t: " + OUTER + " = " + OUTER + "{ bounds(2, 1), " + INNER + "{ Point{ x = 3, y = 4 }, marea.array.Array<sinteger32, 2>{ 5, 6 } } }; t[1][0].y = 7; t[1][1][0] = 8; return (t[0][0] * 10000) + (t[0][1] * 1000) + (t[1][0].x * 100) + (t[1][0].y * 10) + t[1][1][0]; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 49) << "The two results of one function.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 12378) << "Nested elements are reached in place.";
}

TEST(MAREACgenTupleLibrary, SwapCopyAndMoveExchangeOrDuplicateTheElements)
{
    GTEST_COUT << "Validating swap, copy, move and a vector moved out of a tuple." << std::endl;

    const std::string RANGE{ "marea.tuple.Tuple<sinteger32, sinteger32>" };

    const std::string V{ "marea.vector.Vector<sinteger32>" };

    const std::string HELD{ "marea.tuple.Tuple<" + V + ", uinteger64>" };

    const MAREA_ParityResult RESULT{ MAREA_run("function s(): sinteger32 local a: " + RANGE + " = " + RANGE + "{ 1, 2 }; local b: " + RANGE + " = " + RANGE + "{ 3, 4 }; a.swap(b); return (a[0] * 1000) + (a[1] * 100) + (b[0] * 10) + b[1]; end function c(): sinteger32 local a: " + RANGE + " = " + RANGE + "{ 1, 2 }; local b: " + RANGE + " = copy a; b[0] = 9; local d: " + RANGE + " = move b; return (a[0] * 100) + (d[0] * 10) + d[1]; end function v(): uinteger64 local t: " + HELD + " = " + HELD + "{ " + V + "{ 7, 8 }, 2 }; local bytes: " + V + " = move t[0]; bytes.append(9); return (bytes.size() * 10) + t[1]; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("s()")), 3412) << "The two tuples exchanged their elements.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("c()")), 192) << "The copy is independent.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("v()")), 32) << "The vector moved out and the other element stays.";
}

TEST(MAREACgenTupleLibrary, TuplesPassToCallsAndLiveInStructuresAndVectors)
{
    GTEST_COUT << "Validating a read and a write borrow of a tuple, a tuple in a structure field and in a vector." << std::endl;

    const std::string RANGE{ "marea.tuple.Tuple<sinteger32, sinteger32>" };

    const std::string V{ "marea.vector.Vector<" + RANGE + ">" };

    const MAREA_ParityResult RESULT{ MAREA_run("structure Box tag: uinteger8; value: " + RANGE + "; end function sum(t: " + RANGE + "): sinteger32 return t[0] + t[1]; end function bump(#[semantics = reference_mutable] t: " + RANGE + ") t[1] += 10; end function p(): sinteger32 local t: " + RANGE + " = " + RANGE + "{ 1, 2 }; bump(t); return (sum(t) * 100) + t[1]; end function s(): sinteger32 local b: Box = Box{ tag = 1, value = " + RANGE + "{ 3, 4 } }; b.value[0] = 5; local same: boolean = b.value == " + RANGE + "{ 5, 4 }; if same then return sum(b.value); end return 0; end function w(): sinteger32 local v: " + V + " = " + V + "{ " + RANGE + "{ 1, 2 } }; v.append(" + RANGE + "{ 3, 4 }); v[0][1] = 7; return (v[0][1] * 10) + v[1][0]; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("p()")), 1312) << "The write borrow changed the tuple of the caller.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("s()")), 9) << "A tuple in a structure field.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("w()")), 73) << "Tuples in a vector.";
}

TEST(MAREACgenTupleLibrary, TuplesThatOwnStorageAreReturnedReplacedAndCopiedWithTheirStructure)
{
    GTEST_COUT << "Validating the storage of a vector held by a tuple has one owner in every path." << std::endl;

    const std::string V{ "marea.vector.Vector<sinteger32>" };

    const std::string HELD{ "marea.tuple.Tuple<" + V + ", sinteger32>" };

    const MAREA_ParityResult RESULT{ MAREA_run("structure Box value: " + HELD + "; end function make(n: sinteger32): " + HELD + " return " + HELD + "{ " + V + "{ n, n }, n }; end function r(): sinteger32 local t: " + HELD + " = make(1); t = make(2); t = make(3); return t[0][1] + t[1]; end function c(): sinteger32 local a: Box = Box{ value = make(4) }; local b: Box = copy a; b.value[0][0] = 9; return (a.value[0][0] * 10) + b.value[0][0]; end function d(): sinteger32 local total: sinteger32 = 0; for i in 0..3 do local t: " + HELD + " = make(i); total += t[0][0]; end local kept: " + HELD + " = make(5); local other: " + HELD + " = make(6); kept.swap(other); return total + kept[0][0] + other[1]; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("r()")), 6) << "A returned tuple replaces the old one.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("c()")), 49) << "The copy has its own storage.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("d()")), 17) << "A tuple of a loop and a swap of two owners.";
}

TEST(MAREACgenTupleLibrary, ACallOnEitherSideOfEqualityAndATypeAliasWork)
{
    GTEST_COUT << "Validating a tuple that a call gives is compared on either side, and an alias of a tuple is indexed." << std::endl;

    const std::string RANGE{ "marea.tuple.Tuple<sinteger32, sinteger32>" };

    const MAREA_ParityResult RESULT{ MAREA_run("type Range = " + RANGE + "; function m(): " + RANGE + " return " + RANGE + "{ 1, 2 }; end function e(): boolean local a: " + RANGE + " = m(); return m() == a and m() == m() and a == m(); end function a(): sinteger32 local t: Range = " + RANGE + "{ 3, 4 }; t[0] += 1; return (t[0] * 10) + t[1]; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("e()")), 1) << "The tuples are equal.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("a()")), 44) << "The alias is read as its tuple.";
}
