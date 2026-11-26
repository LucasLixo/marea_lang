// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_INDEX_OUT_OF_BOUNDS{ 8 };

    const std::string MAREA_PRELUDE{ "namespace main; require marea.array; " };

    const std::string MAREA_ARRAY4{ "marea.array.Array<sinteger32, 4>" };

    const std::string MAREA_FOUR{ "local a: " + MAREA_ARRAY4 + " = " + MAREA_ARRAY4 + "{ 1, 2, 3, 4 }; " };

    const std::string MAREA_NONE{ "local a: marea.array.Array<sinteger32, 0> = marea.array.Array<sinteger32, 0>{}; " };

    MAREA_ParityResult MAREA_run(const std::string &body, const std::vector<MAREA_ParityCall> &calls = {})
    {
        return MAREA_parityCheck(MAREA_PRELUDE + body, calls);
    }
} // namespace

TEST(MAREACgenArrayLibrary, MembersReadTheElementsAndTheCount)
{
    GTEST_COUT << "Validating [], front, back, size, max_size and empty give the same answers in both backends." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 " + MAREA_FOUR + "return (a.front() * 100) + (a.back() * 10) + a[1]; end function g(): uinteger64 " + MAREA_FOUR + "return (a.size() * 10) + a.max_size(); end function h(): boolean " + MAREA_FOUR + "return a.empty(); end function i(): boolean " + MAREA_NONE + "return a.empty() and (a.size() == 0); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 142) << "The first element, the last and the second.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 44) << "The size and the maximum size are the count.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("h()")), 0) << "A count of 4 is not empty.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("i()")), 1) << "A count of 0 is empty.";
}

TEST(MAREACgenArrayLibrary, FillFilledAndSwapWriteEveryElement)
{
    GTEST_COUT << "Validating fill, filled and swap change each element and nothing else." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 " + MAREA_FOUR + "a.fill(7); return a[0] + a[1] + a[2] + a[3]; end function g(): sinteger32 local a: marea.array.Array<sinteger32, 8> = marea.array.Array<sinteger32, 8>.filled(3); local total: sinteger32 = 0; for each value in a do total += value; end return total; end function h(): sinteger32 " + MAREA_FOUR + "local b: " + MAREA_ARRAY4 + " = " + MAREA_ARRAY4 + "{ 5, 6, 7, 8 }; a.swap(b); return (a[0] * 1000) + (a[3] * 100) + (b[0] * 10) + b[3]; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 28) << "Four times 7.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 24) << "Eight times 3.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("h()")), 5814) << "The two arrays exchanged their elements.";
}

TEST(MAREACgenArrayLibrary, AnIndexAtOrPastTheCountTraps)
{
    GTEST_COUT << "Validating [], a write and front and back of a count of zero trap with the index out of bounds code." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityUnsigned("uinteger64", 3) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityUnsigned("uinteger64", 4) }),
        MAREA_parityCall("g", "sinteger32", { MAREA_parityUnsigned("uinteger64", 4) }),
        MAREA_parityCall("h", "sinteger32"),
        MAREA_parityCall("i", "sinteger32"),
    };

    const MAREA_ParityResult RESULT{ MAREA_run("function f(i: uinteger64): sinteger32 " + MAREA_FOUR + "return a[i]; end function g(i: uinteger64): sinteger32 " + MAREA_FOUR + "a[i] = 9; return a[0]; end function h(): sinteger32 " + MAREA_NONE + "return a.front(); end function i(): sinteger32 " + MAREA_NONE + "return a.back(); end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 4) << "The last element.";

    for (std::size_t index{ 1 }; index < CALLS.size(); ++index) {
        EXPECT_EQ(RESULT.generated[index].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << MAREA_parityDescribe(CALLS[index]);
    }
}

TEST(MAREACgenArrayLibrary, NestedArraysAreReachedElementByElement)
{
    GTEST_COUT << "Validating an array of arrays holds each inner array in place and checks each index." << std::endl;

    const std::string ROW{ "marea.array.Array<sinteger32, 2>" };

    const std::string GRID{ "marea.array.Array<marea.array.Array<sinteger32, 2>, 2>" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityUnsigned("uinteger64", 1) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityUnsigned("uinteger64", 2) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_run("function f(i: uinteger64): sinteger32 local grid: " + GRID + " = " + GRID + "{ " + ROW + "{ 1, 2 }, " + ROW + "{ 3, 4 } }; grid[0][1] = 9; return (grid[0][1] * 10) + grid[1][i]; end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 94) << "The written element and the last element.";

    EXPECT_EQ(RESULT.generated[1].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << "The inner index is checked.";
}

TEST(MAREACgenArrayLibrary, EveryLoopWalksTheArray)
{
    GTEST_COUT << "Validating while, do while, for range and for each, with its key, walk the elements in order." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function w(): sinteger32 " + MAREA_FOUR + "local total: sinteger32 = 0; local index: uinteger64 = 0; while index < a.size() do total = (total * 10) + a[index]; index += 1; end return total; end function d(): sinteger32 " + MAREA_FOUR + "local total: sinteger32 = 0; local index: uinteger64 = 0; do total += a[index]; index += 1; while total < 6 and index < a.size() end return convert<sinteger32>(index); end function r(): sinteger32 " + MAREA_FOUR + "local total: sinteger32 = 0; for index in 0 .. 3 do total += a[index] * a[index]; end return total; end function k(): uinteger64 " + MAREA_FOUR + "for each index, value in a do if value == 3 then return index; end end return a.size(); end function m(): sinteger32 " + MAREA_FOUR + "for each value in mutable a do value *= 10; end local total: sinteger32 = 0; for each value in a do if value == 20 then continue; end if value == 40 then break; end total += value; end return total; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("w()")), 1234) << "The elements in order.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("d()")), 3) << "The total reaches 6 at the third element.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("r()")), 30) << "The sum of the squares.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("k()")), 2) << "The key of the element 3.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("m()")), 40) << "10 and 30: 20 is skipped and 40 stops.";
}

TEST(MAREACgenArrayLibrary, AWalkFromTheEndSeesTheReverseOrder)
{
    GTEST_COUT << "Validating a walk from size down to zero reads the last element first." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): sinteger32 " + MAREA_FOUR + "local total: sinteger32 = 0; local index: uinteger64 = a.size(); while index > 0 do index -= 1; total = (total * 10) + a[index]; end return total; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 4321) << "The elements from the last to the first.";
}

TEST(MAREACgenArrayLibrary, ACountOfZeroRunsNoLoopBodyAndDoWhileTraps)
{
    GTEST_COUT << "Validating for each over an empty array runs nothing and a do while reads past its end." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_parityCall("e", "sinteger32"), MAREA_parityCall("d", "sinteger32") };

    const MAREA_ParityResult RESULT{ MAREA_run("function e(): sinteger32 " + MAREA_NONE + "local count: sinteger32 = 0; for each value in a do count += 1; end return count; end function d(): sinteger32 " + MAREA_NONE + "local total: sinteger32 = 0; local index: uinteger64 = 0; do total += a[index]; index += 1; while index < a.size() end return total; end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 0) << "The body never runs.";

    EXPECT_EQ(RESULT.generated[1].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << "The body runs once and the first read traps.";
}

TEST(MAREACgenArrayLibrary, EachElementOfAnArrayIsDestroyedOnce)
{
    GTEST_COUT << "Validating an array that holds structures with a destroy operator destroys each element once, in the generated C." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_parityHost() };

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; require marea.array; require demo.host as host; structure Handle id: sinteger32; end implementation Handle #[operator = \"destroy\"] function close() host.record(-1); end end function f(): sinteger32 local a: marea.array.Array<Handle, 3> = marea.array.Array<Handle, 3>{ Handle{ id = 1 }, Handle{ id = 2 }, Handle{ id = 3 } }; host.record(9); return a[2].id; end", {}, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 3) << "The element is read before the destroys.";

    EXPECT_EQ(RESULT.generated[0].log, (std::vector<std::int64_t>{ 9, -1, -1, -1 })) << "Three destroys when the array ends.";
}
