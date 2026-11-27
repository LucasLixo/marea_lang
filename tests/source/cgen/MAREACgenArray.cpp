// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_INDEX_OUT_OF_BOUNDS{ 8 };

    const std::string MAREA_ARRAYS{ "namespace main; require marea.array as array; require marea.array.Array; structure Point x: sinteger32; y: sinteger32; end " };
} // namespace

TEST(MAREACgenArray, LiteralsAndElementsAreReadAndWritten)
{
    GTEST_COUT << "Validating positional and indexed literals fill the rest with zeros and an element is read, assigned and combined." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "function f(): sinteger32 local a: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{ 1, 2, 3, 4 }; local b: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{ [2] = 7 }; local c: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{}; a[1] = 5; a[2] += 1; return (a[0] * 1000) + (a[1] * 100) + (a[2] * 10) + b[2] + b[0] + a[3] + c[3]; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 1551) << "a is 1, 5, 4, 4 and b is 0, 0, 7, 0.";
}

TEST(MAREACgenArray, AnIndexPastTheCountTraps)
{
    GTEST_COUT << "Validating an index past the count, also a negative one, is the index out of bounds trap." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(3) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(4) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(-1) }),
        MAREA_parityCall("g", "sinteger32", { MAREA_parityUnsigned("uinteger64", 4) }),
        MAREA_parityCall("g", "sinteger32", { MAREA_parityUnsigned("uinteger64", 0) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "function f(i: sinteger32): sinteger32 local a: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{ 1, 2, 3, 4 }; return a[i]; end function g(i: uinteger64): sinteger32 local a: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{ 1, 2, 3, 4 }; a[i] = 9; return a[0]; end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 4) << "The last element.";

    EXPECT_EQ(RESULT.generated[1].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << "The index 4 is past the count.";

    EXPECT_EQ(RESULT.generated[2].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << "A negative index is past the count.";

    EXPECT_EQ(RESULT.generated[3].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << "A write past the count traps before it writes.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[4]), 9) << "A write inside the count.";
}

TEST(MAREACgenArray, CopiesAreIndependentAndAResultComesBackByValue)
{
    GTEST_COUT << "Validating a copy of an array is a new value and a function returns an array." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "function make(): marea.array.Array<sinteger32, 2> return array.Array<sinteger32, 2>{ 8, 9 }; end function first(values: marea.array.Array<sinteger32, 2>): sinteger32 return values[0]; end function f(): sinteger32 local a: marea.array.Array<sinteger32, 2> = array.Array<sinteger32, 2>{ 1, 2 }; local b: marea.array.Array<sinteger32, 2> = copy a; b[1] = 7; local c: marea.array.Array<sinteger32, 2> = a; c[0] = 3; local d: marea.array.Array<sinteger32, 2> = make(); d = make(); return (a[0] * 100000) + (a[1] * 10000) + (b[1] * 1000) + (c[0] * 100) + (d[1] * 10) + first(make()); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 127398) << "a keeps 1 and 2.";
}

TEST(MAREACgenArray, AWriteThroughAnArrayWriteBorrowReachesTheCaller)
{
    GTEST_COUT << "Validating an array parameter is a borrow, so a function with a write borrow changes the caller's array." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "function change(#[semantics = reference_mutable] values: marea.array.Array<sinteger32, 2>) values[0] = 100; end function f(): sinteger32 local a: marea.array.Array<sinteger32, 2> = array.Array<sinteger32, 2>{ 1, 2 }; change(a); return a[0] + a[1]; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 102) << "The write is seen by the caller.";
}

TEST(MAREACgenArray, ArraysOfStructuresAndOfArraysAreStored)
{
    GTEST_COUT << "Validating an array of structures, an array of arrays and arrays in a structure keep the layout of sema." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "structure Holder values: marea.array.Array<sinteger32, 2>; grid: marea.array.Array<marea.array.Array<sinteger32, 2>, 2>; count: sinteger32; end function f(): sinteger32 local points: marea.array.Array<Point, 2> = array.Array<Point, 2>{ Point{ x = 1, y = 2 }, Point{ x = 3, y = 4 } }; points[1].x = 30; local holder: Holder = Holder{ values = array.Array<sinteger32, 2>{ 7, 8 }, grid = array.Array<marea.array.Array<sinteger32, 2>, 2>{}, count = 2 }; holder.grid[1][0] = 4; local grid: marea.array.Array<marea.array.Array<sinteger32, 2>, 3> = array.Array<marea.array.Array<sinteger32, 2>, 3>{}; grid[2][1] = 5; return (points[1].x * 100) + (points[0].y * 10) + grid[2][1] + grid[0][0] + holder.count + (holder.values[1] * 10000) + (holder.grid[1][0] * 100000); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 483027) << "400000 + 80000 + 3000 + 20 + 5 + 0 + 2.";

    EXPECT_NE(RESULT.cText.find("_Static_assert(sizeof(marea_s_Holder) == 28"), std::string::npos) << "The size of sema must be asserted.";
}

TEST(MAREACgenArray, AStructureThatHoldsAnArrayOfStructuresIsMeasuredAndWritten)
{
    GTEST_COUT << "Validating a structure that holds an array of structures is written with the layout that sema measures." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; require demo.array; structure Point x: sinteger32; end structure Holder points: demo.array<Point, 2>; end function f(): sinteger32 local h: Holder = Holder{ points = demo.array<Point, 2>{ Point{ x = 3 }, Point{ x = 4 } } }; return h.points[1].x; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 4) << "The second point of the array.";

    EXPECT_NE(RESULT.cText.find("_Static_assert(sizeof(marea_s_Holder) == 8"), std::string::npos) << "Two points of four bytes.";
}

TEST(MAREACgenArray, ForEachWalksEveryElementWithItsKey)
{
    GTEST_COUT << "Validating a for each visits every element in order, with its key, and a mutable one writes the elements." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "function sum(values: marea.array.Array<sinteger32, 4>): sinteger32 local total: sinteger32 = 0; for each index, value in values do total += value * (convert<sinteger32>(index) + 1); end return total; end function f(): sinteger32 local a: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{ 1, 2, 3, 4 }; for each value in mutable a do value *= 10; end local seen: sinteger32 = 0; for each value in a do if value == 20 then continue; end if value == 40 then break; end seen += value; end return (sum(a) * 100) + seen; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 30040) << "sum is 300 and seen is 40.";
}

TEST(MAREACgenArray, WithoutARegistryAnArrayIsReported)
{
    GTEST_COUT << "Validating cgen reads the shape of a library type from the registry only, and reports a type it cannot read." << std::endl;

    MAREA_IrFixture fixture(MAREA_parityLibraries());

    ASSERT_EQ(fixture.MAREA_lower(MAREA_ARRAYS + "function f(): sinteger32 local a: marea.array.Array<sinteger32, 4> = array.Array<sinteger32, 4>{}; return a[0]; end"), MAREA_IR_EXCEPT_SUCCESS) << "The module must lower.";

    MAREA_CgenContext *context{ nullptr };

    ASSERT_EQ(MAREA_CgenContextCreate(fixture.sema.parser.lexer.arena, fixture.sema.parser.lexer.sink, &context), MAREA_CGEN_EXCEPT_SUCCESS) << "The context must be created.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_CgenGenerateModule(context, fixture.module, &text, &length), MAREA_CGEN_EXCEPT_SUCCESS) << "The generation reports through the sink.";

    EXPECT_EQ(MAREA_parityNewCodes(fixture, 0).count(MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE), 1U) << "The array has no shape without a registry.";
}

TEST(MAREACgenArray, TheContextKeepsTheRegistry)
{
    GTEST_COUT << "Validating the context keeps the registry it is given and rejects a null context." << std::endl;

    MAREA_IrFixture fixture;

    MAREA_Arena *arena{ fixture.sema.parser.lexer.arena };

    MAREA_CgenContext *context{ nullptr };

    ASSERT_EQ(MAREA_CgenContextCreate(arena, fixture.sema.parser.lexer.sink, &context), MAREA_CGEN_EXCEPT_SUCCESS) << "The context must be created.";

    EXPECT_EQ(MAREA_CgenContextGetTypeRegistry(context), nullptr) << "A new context has no registry.";

    MAREA_TypeRegistry *registry{ MAREA_buildLibraryRegistry(arena) };

    ASSERT_EQ(MAREA_CgenContextSetTypeRegistry(context, registry), MAREA_CGEN_EXCEPT_SUCCESS) << "The registry must be taken.";

    EXPECT_EQ(MAREA_CgenContextGetTypeRegistry(context), registry) << "The context must keep it.";

    EXPECT_EQ(MAREA_CgenContextSetTypeRegistry(nullptr, registry), MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER) << "A null context must be refused.";

    EXPECT_EQ(MAREA_CgenContextGetTypeRegistry(nullptr), nullptr) << "A null context has no registry.";
}

TEST(MAREACgenArray, WritesThroughElementsOfStructuresReachTheArray)
{
    GTEST_COUT << "Validating a mutable for each and a method with a write borrow of self change the element in the array." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_ARRAYS + "implementation Point function bump(#[semantics = reference_mutable] self: Point, by: sinteger32) self.x += by; end end function f(): sinteger32 local points: marea.array.Array<Point, 2> = array.Array<Point, 2>{ Point{ x = 1, y = 2 }, Point{ x = 3, y = 4 } }; for each point in mutable points do point.y *= 10; end points[1].bump(100); return (points[0].x * 1000000) + (points[0].y * 10000) + (points[1].x * 100) + points[1].y; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 1210340) << "points are (1, 20) and (103, 40).";
}
