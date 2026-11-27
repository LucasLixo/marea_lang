// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_INDEX_OUT_OF_BOUNDS{ 8 };

    constexpr int MAREA_TRAP_OUT_OF_MEMORY{ 9 };

    const std::string MAREA_PRELUDE{ "namespace main; require marea.vector; " };

    const std::string MAREA_V{ "marea.vector.Vector<sinteger32>" };

    const std::string MAREA_EMPTY{ "local v: " + MAREA_V + " = " + MAREA_V + "{}; " };

    const std::string MAREA_THREE{ "local v: " + MAREA_V + " = " + MAREA_V + "{ 1, 2, 3 }; " };

    const std::string MAREA_DIGITS{ "function digits(#[semantics = reference_read_only] v: " + MAREA_V + "): sinteger32 local total: sinteger32 = 0; for each value in v do total = (total * 10) + value; end return total; end " };

    MAREA_ParityResult MAREA_run(const std::string &body, const std::vector<MAREA_ParityCall> &calls = {})
    {
        return MAREA_parityCheck(MAREA_PRELUDE + body, calls);
    }

    std::vector<MAREA_ParityCall> MAREA_countCalls(const std::string &function, const std::vector<std::uint64_t> &counts)
    {
        std::vector<MAREA_ParityCall> calls;

        for (const std::uint64_t count : counts) {
            calls.push_back(MAREA_parityCall(function, "uinteger64", { MAREA_parityUnsigned("uinteger64", count) }));
        }

        return calls;
    }

    std::vector<std::int64_t> MAREA_values(const MAREA_ParityResult &result)
    {
        std::vector<std::int64_t> values;

        for (const MAREA_ParityOutcome &outcome : result.generated) {
            values.push_back(MAREA_parityValue(outcome));
        }

        return values;
    }
} // namespace

TEST(MAREACgenVectorLibrary, AnEmptyVectorHoldsNothingAndALiteralHoldsItsValues)
{
    GTEST_COUT << "Validating an empty vector has size and capacity zero and a literal has both equal to its count." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): uinteger64 " + MAREA_EMPTY + "return (v.size() * 10) + v.capacity(); end function g(): boolean " + MAREA_EMPTY + "return v.empty(); end function h(): uinteger64 " + MAREA_THREE + "return (v.size() * 10) + v.capacity(); end function i(): sinteger32 " + MAREA_THREE + "return (v.front() * 100) + (v[1] * 10) + v.back(); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 0) << "No element and no storage.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 1) << "An empty vector is empty.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("h()")), 33) << "A literal of 3 values.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("i()")), 123) << "The first, the second and the last element.";
}

TEST(MAREACgenVectorLibrary, TheCapacityGrowsByTheSameRuleInBothBackends)
{
    GTEST_COUT << "Validating a full vector grows to the larger of 4 and twice its capacity." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_countCalls("f", { 0, 1, 4, 5, 8, 9, 16, 17, 33 }) };

    const MAREA_ParityResult RESULT{ MAREA_run("function f(n: uinteger64): uinteger64 " + MAREA_EMPTY + "local i: uinteger64 = 0; while i < n do v.append(1); i += 1; end return v.capacity(); end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_values(RESULT), (std::vector<std::int64_t>{ 0, 4, 4, 8, 8, 16, 16, 32, 64 })) << "The sequence of the growth rule.";
}

TEST(MAREACgenVectorLibrary, AFullLiteralGrowsOnTheNextAppend)
{
    GTEST_COUT << "Validating an append or an insert on a full literal asks for the larger of 4 and twice the capacity." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function f(): uinteger64 local v: " + MAREA_V + " = " + MAREA_V + "{ 1 }; v.append(2); return v.capacity(); end function g(): uinteger64 " + MAREA_THREE + "v.insert(0, 0); return v.capacity(); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 4) << "4 is larger than 2.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("g()")), 6) << "Twice 3.";
}

TEST(MAREACgenVectorLibrary, ReserveAndShrinkSetTheCapacity)
{
    GTEST_COUT << "Validating reserve asks for exactly the request when it is larger, and shrink_to_fit drops the rest." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_countCalls("f", { 0, 2, 3, 10, 100 }) };

    const MAREA_ParityResult RESULT{ MAREA_run("function f(n: uinteger64): uinteger64 " + MAREA_THREE + "v.reserve(n); return v.capacity(); end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_values(RESULT), (std::vector<std::int64_t>{ 3, 3, 3, 10, 100 })) << "A smaller request does nothing.";

    const MAREA_ParityResult SHRUNK{ MAREA_run(MAREA_DIGITS + "function g(): uinteger64 " + MAREA_THREE + "v.reserve(32); v.shrink_to_fit(); return (v.size() * 10) + v.capacity(); end function h(): sinteger32 " + MAREA_THREE + "v.reserve(32); v.shrink_to_fit(); v.append(4); return digits(v); end function e(): uinteger64 " + MAREA_THREE + "v.clear(); v.shrink_to_fit(); return v.capacity(); end") };

    ASSERT_TRUE(SHRUNK.failures.empty()) << SHRUNK.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(SHRUNK.MAREA_outcome("g()")), 33) << "The capacity is the size.";

    EXPECT_EQ(MAREA_parityValue(SHRUNK.MAREA_outcome("h()")), 1234) << "The elements are kept.";

    EXPECT_EQ(MAREA_parityValue(SHRUNK.MAREA_outcome("e()")), 0) << "An empty vector shrinks to no storage.";
}

TEST(MAREACgenVectorLibrary, TheModifiersKeepTheOrderOfTheElements)
{
    GTEST_COUT << "Validating append, pop, insert, erase, clear, resize and fill move the elements as they say." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_DIGITS + "function a(): sinteger32 " + MAREA_EMPTY + "for i in 1 .. 9 do v.append(i); end return digits(v); end function p(): sinteger32 " + MAREA_THREE + "local last: sinteger32 = v.pop(); return (last * 1000) + digits(v); end function i(): sinteger32 " + MAREA_THREE + "v.insert(1, 7); v.insert(0, 8); v.insert(v.size(), 9); return digits(v); end function e(): sinteger32 " + MAREA_THREE + "local gone: sinteger32 = v.erase(1); return (gone * 1000) + digits(v); end function c(): uinteger64 " + MAREA_THREE + "v.clear(); local kept: uinteger64 = v.capacity(); v.append(5); return (kept * 100) + (v.size() * 10) + convert<uinteger64>(v[0]); end function r(): sinteger32 " + MAREA_THREE + "v.resize(5, 7); return digits(v); end function t(): sinteger32 " + MAREA_THREE + "v.resize(1, 7); return digits(v); end function z(): sinteger32 " + MAREA_EMPTY + "v.resize(3, 1); return digits(v); end function l(): sinteger32 " + MAREA_THREE + "v.fill(4); return digits(v); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("a()")), 123456789) << "Appends past two growths keep the order.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("p()")), 3012) << "pop gives the last element.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("i()")), 817239) << "insert shifts the tail up.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("e()")), 2013) << "erase gives the element and shifts the tail down.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("c()")), 315) << "clear keeps the capacity.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("r()")), 12377) << "resize adds copies of the value.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("t()")), 1) << "resize drops the tail.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("z()")), 111) << "resize of an empty vector repeats the value.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("l()")), 444) << "fill keeps the size.";
}

TEST(MAREACgenVectorLibrary, SwapCopyAndMoveExchangeOrDuplicateTheStorage)
{
    GTEST_COUT << "Validating swap exchanges the headers, copy is independent and move and return hand the owner on." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_DIGITS + "function make(): " + MAREA_V + " " + MAREA_THREE + "return v; end function s(): uinteger64 " + MAREA_THREE + "local w: " + MAREA_V + " = " + MAREA_V + "{ 9 }; w.reserve(8); v.swap(w); return (convert<uinteger64>(digits(v)) * 100000) + (v.capacity() * 10000) + (convert<uinteger64>(digits(w)) * 10) + w.capacity(); end function c(): sinteger32 " + MAREA_THREE + "local w: " + MAREA_V + " = copy v; w[0] = 9; return (digits(v) * 1000) + digits(w); end function m(): sinteger32 " + MAREA_THREE + "local w: " + MAREA_V + " = move v; return digits(w); end function r(): sinteger32 local w: " + MAREA_V + " = make(); w = make(); return digits(w); end function over(): sinteger32 " + MAREA_THREE + "local w: " + MAREA_V + " = " + MAREA_V + "{ 7 }; w = copy v; v = copy w; return (digits(v) * 1000) + digits(w); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("s()")), 981233) << "Each vector took the elements and the capacity of the other.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("c()")), 123923) << "A write to the copy leaves the source.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("m()")), 123) << "The elements moved.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("r()")), 123) << "A returned vector reaches the caller.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("over()")), 123123) << "A copy over a vector that holds elements replaces them.";
}

TEST(MAREACgenVectorLibrary, EveryFaultOfAnIndexIsTheSameTrap)
{
    GTEST_COUT << "Validating each member that checks an index traps with the index out of bounds code in both backends." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("g", "sinteger32", { MAREA_parityUnsigned("uinteger64", 3) }),
        MAREA_parityCall("w", "sinteger32", { MAREA_parityUnsigned("uinteger64", 3) }),
        MAREA_parityCall("front", "sinteger32"),
        MAREA_parityCall("back", "sinteger32"),
        MAREA_parityCall("pop", "sinteger32"),
        MAREA_parityCall("ins", "sinteger32", { MAREA_parityUnsigned("uinteger64", 4) }),
        MAREA_parityCall("ins", "sinteger32", { MAREA_parityUnsigned("uinteger64", UINT64_MAX) }),
        MAREA_parityCall("era", "sinteger32", { MAREA_parityUnsigned("uinteger64", 3) }),
        MAREA_parityCall("era", "sinteger32", { MAREA_parityUnsigned("uinteger64", UINT64_MAX) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_run("function g(i: uinteger64): sinteger32 " + MAREA_THREE + "return v[i]; end function w(i: uinteger64): sinteger32 " + MAREA_THREE + "v[i] = 1; return v[0]; end function front(): sinteger32 " + MAREA_EMPTY + "return v.front(); end function back(): sinteger32 " + MAREA_EMPTY + "return v.back(); end function pop(): sinteger32 " + MAREA_EMPTY + "return v.pop(); end function ins(i: uinteger64): sinteger32 " + MAREA_THREE + "v.insert(i, 1); return v[0]; end function era(i: uinteger64): sinteger32 " + MAREA_THREE + "return v.erase(i); end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    for (std::size_t index{ 0 }; index < CALLS.size(); ++index) {
        EXPECT_EQ(RESULT.generated[index].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << MAREA_parityDescribe(CALLS[index]);
    }
}

TEST(MAREACgenVectorLibrary, MaxSizeBoundsAReserve)
{
    GTEST_COUT << "Validating max_size is the largest count that fits, and a reserve past it is out of memory." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("m", "uinteger64"),
        MAREA_parityCall("e", "uinteger64"),
        MAREA_parityCall("past", "uinteger64"),
        MAREA_parityCall("all", "uinteger64"),
    };

    const MAREA_ParityResult RESULT{ MAREA_run("structure Nothing end function m(): uinteger64 " + MAREA_EMPTY + "return v.max_size(); end function e(): uinteger64 local v: marea.vector.Vector<Nothing> = marea.vector.Vector<Nothing>{}; return v.max_size(); end function past(): uinteger64 " + MAREA_EMPTY + "v.reserve(v.max_size() + 1); return v.capacity(); end function all(): uinteger64 " + MAREA_EMPTY + "v.reserve(18446744073709551615); return v.capacity(); end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 2305843009213693951LL) << "The largest signed 64 bit count of bytes, over 4.";

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[1]), INT64_MAX) << "An element of no size.";

    EXPECT_EQ(RESULT.generated[2].trap, MAREA_TRAP_OUT_OF_MEMORY) << "No allocation can hold one more.";

    EXPECT_EQ(RESULT.generated[3].trap, MAREA_TRAP_OUT_OF_MEMORY) << "A request whose bytes overflow is refused.";
}

TEST(MAREACgenVectorLibrary, AFailedAllocationIsTheOutOfMemoryTrap)
{
    GTEST_COUT << "Validating a host allocator that gives no block stops the run with out of memory in both backends." << std::endl;

    MAREA_ParityHost host{ MAREA_parityHost() };

    host.vmAllocate = &marea_parity_no_block;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_no_block";
    host.cRelease = "marea_parity_release";

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_parityCall("append", "uinteger64"), MAREA_parityCall("literal", "uinteger64"), MAREA_parityCall("reserve", "uinteger64") };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck("namespace main; require marea.vector; require demo.host as host; function append(): uinteger64 " + MAREA_EMPTY + "host.record(1); v.append(1); host.record(2); return v.size(); end function literal(): uinteger64 " + MAREA_THREE + "host.record(2); return v.size(); end function reserve(): uinteger64 " + MAREA_EMPTY + "v.reserve(8); host.record(2); return v.size(); end", CALLS, &host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    for (std::size_t index{ 0 }; index < CALLS.size(); ++index) {
        EXPECT_EQ(RESULT.generated[index].trap, MAREA_TRAP_OUT_OF_MEMORY) << MAREA_parityDescribe(CALLS[index]);

        EXPECT_EQ(std::count(RESULT.generated[index].log.begin(), RESULT.generated[index].log.end(), 2), 0) << "Nothing after the failed allocation runs: " << MAREA_parityDescribe(CALLS[index]);
    }
}

TEST(MAREACgenVectorLibrary, EveryBlockOfTheHostAllocatorIsGivenBack)
{
    GTEST_COUT << "Validating the allocator of the host sees one free for each block, after growth, copy and swap." << std::endl;

    MAREA_ParityHost host{ MAREA_parityHost() };

    host.vmAllocate = &marea_parity_allocate;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_allocate";
    host.cRelease = "marea_parity_release";

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(MAREA_PRELUDE + "function f(): uinteger64 " + MAREA_EMPTY + "for i in 1 .. 20 do v.append(i); end local w: " + MAREA_V + " = copy v; w.shrink_to_fit(); v.swap(w); local u: " + MAREA_V + " = move v; return u.size(); end", {}, &host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    const std::vector<std::int64_t> LOG{ RESULT.MAREA_outcome("f()").log };

    const auto BLOCKS{ std::count_if(LOG.begin(), LOG.end(), [](std::int64_t entry) { return (entry >= 1000) && (entry < 2000); }) };

    const auto FREES{ std::count(LOG.begin(), LOG.end(), 2000) };

    EXPECT_GT(BLOCKS, 0) << "The vector takes its storage from the host.";

    EXPECT_EQ(FREES, BLOCKS) << "No block is lost and none is freed twice.";
}

TEST(MAREACgenVectorLibrary, ElementsAreDestroyedOnceWhenTheyLeaveTheVector)
{
    GTEST_COUT << "Validating clear, a shorter resize and the end of the owner destroy each element once, in the generated C." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_parityHost() };

    const std::string HANDLE{ "namespace main; require marea.vector; require demo.host as host; structure Handle id: sinteger32; end implementation Handle #[operator = \"destroy\"] function close() host.record(-1); end end " };

    const std::string HANDLES{ "local v: marea.vector.Vector<Handle> = marea.vector.Vector<Handle>{}; v.append(Handle{ id = 1 }); v.append(Handle{ id = 2 }); v.append(Handle{ id = 3 }); host.record(9); " };

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(HANDLE + "function end_of_owner(): uinteger64 " + HANDLES + "return v.size(); end function clear(): uinteger64 " + HANDLES + "v.clear(); host.record(8); return v.size(); end function popped(): uinteger64 " + HANDLES + "if true then local last: Handle = v.pop(); host.record(last.id); end host.record(8); return v.size(); end", {}, &HOST) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.MAREA_outcome("end_of_owner()").log, (std::vector<std::int64_t>{ 9, -1, -1, -1 })) << "Each element once.";

    EXPECT_EQ(RESULT.MAREA_outcome("clear()").log, (std::vector<std::int64_t>{ 9, -1, -1, -1, 8 })) << "clear destroys at once and the end destroys nothing more.";

    EXPECT_EQ(RESULT.MAREA_outcome("popped()").log, (std::vector<std::int64_t>{ 9, 3, -1, 8, -1, -1 })) << "The popped element belongs to the caller.";
}

TEST(MAREACgenVectorLibrary, EveryLoopWalksTheVector)
{
    GTEST_COUT << "Validating the loops walk the current size, a mutable for each writes in place and another vector grows in the body." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_parityCall("w", "sinteger32"), MAREA_parityCall("r", "sinteger32"), MAREA_parityCall("m", "sinteger32"), MAREA_parityCall("evens", "sinteger32"), MAREA_parityCall("none", "sinteger32"), MAREA_parityCall("d", "sinteger32") };

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_DIGITS + "function w(): sinteger32 " + MAREA_THREE + "local total: sinteger32 = 0; local index: uinteger64 = 0; while index < v.size() do total = (total * 10) + v[index]; index += 1; end return total; end function r(): sinteger32 " + MAREA_THREE + "local total: sinteger32 = 0; for index in 0 .. 2 do total += v[index]; end return total; end function m(): sinteger32 " + MAREA_THREE + "for each value in mutable v do value *= 2; end local total: sinteger32 = 0; for each index, value in v do total += value * (convert<sinteger32>(index) + 1); end return total; end function evens(): sinteger32 local v: " + MAREA_V + " = " + MAREA_V + "{ 1, 2, 3, 4, 5, 6 }; " + "local result: " + MAREA_V + " = " + MAREA_V + "{}; for each value in v do if value % 2 == 0 then result.append(value); end end return digits(result); end function none(): sinteger32 " + MAREA_EMPTY + "local count: sinteger32 = 0; for each value in v do count += 1; end return count; end function d(): sinteger32 " + MAREA_EMPTY + "local total: sinteger32 = 0; local index: uinteger64 = 0; do total += v[index]; index += 1; while index < v.size() end return total; end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_values(RESULT), (std::vector<std::int64_t>{ 123, 6, 28, 246, 0, 0 })) << "The loops in order.";

    EXPECT_EQ(RESULT.generated[5].trap, MAREA_TRAP_INDEX_OUT_OF_BOUNDS) << "A do while over an empty vector traps at the first read.";
}

TEST(MAREACgenVectorLibrary, AVectorHoldsStructuresVectorsAndElementsOfNoSize)
{
    GTEST_COUT << "Validating a vector of a structure, of vectors moved in and of a structure with no field." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("structure Point x: sinteger32; y: sinteger32; end structure Nothing end function p(): sinteger32 local v: marea.vector.Vector<Point> = marea.vector.Vector<Point>{}; v.append(Point{ x = 1, y = 2 }); v.append(Point{ x = 3, y = 4 }); v[0].x = 9; local total: sinteger32 = 0; for each point in v do total = (total * 100) + (point.x * 10) + point.y; end return total; end function n(): sinteger32 local outer: marea.vector.Vector<" + MAREA_V + "> = marea.vector.Vector<" + MAREA_V + ">{}; " + MAREA_THREE + "outer.append(move v); outer.append(" + MAREA_V + "{ 4 }); outer[1].append(5); return (outer[0][2] * 100) + (outer[1][0] * 10) + outer[1][1]; end function z(): uinteger64 local v: marea.vector.Vector<Nothing> = marea.vector.Vector<Nothing>{}; v.append(Nothing{}); v.append(Nothing{}); local first: Nothing = v.pop(); v.append(Nothing{}); return v.size(); end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("p()")), 9234) << "The structures are stored and written in place.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("n()")), 345) << "The inner vectors keep their own storage.";

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("z()")), 2) << "The size counts elements of no size.";
}

TEST(MAREACgenVectorLibrary, TemporariesLoopsJumpsAndContainersGiveEveryBlockBack)
{
    GTEST_COUT << "Validating no block is lost or freed twice for temporaries, jumps out of loops, replaced values and vectors held in other values." << std::endl;

    MAREA_ParityHost host{ MAREA_parityHost() };

    host.vmAllocate = &marea_parity_allocate;
    host.vmRelease = &marea_parity_release;
    host.cAllocate = "marea_parity_allocate";
    host.cRelease = "marea_parity_release";

    const std::string BAG{ "structure Bag items: " + MAREA_V + "; extra: sinteger32; end " };

    const std::string MAKE{ "function make(): " + MAREA_V + " " + MAREA_THREE + "return v; end function first(#[semantics = reference_read_only] v: " + MAREA_V + "): sinteger32 return v[0]; end " };

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_parityCall("borrowed", "sinteger32"), MAREA_parityCall("discarded", "sinteger32"), MAREA_parityCall("loops", "sinteger32"), MAREA_parityCall("replaced", "sinteger32"), MAREA_parityCall("bag", "sinteger32"), MAREA_parityCall("row", "sinteger32"), MAREA_parityCall("nested", "sinteger32"), MAREA_parityCall("walked", "sinteger32"), MAREA_parityCall("early", "sinteger32") };

    const std::string BODY{ "function borrowed(): sinteger32 local total: sinteger32 = 0; local i: sinteger32 = 0; while i < 3 do total += first(make()); i += 1; end return total; end "
                            "function discarded(): sinteger32 make(); make(); return 1; end "
                            "function loops(): sinteger32 local total: sinteger32 = 0; for i in 1 .. 5 do " +
                            MAREA_THREE + "if i == 2 then continue; end if i == 4 then break; end total += v[0]; end return total; end "
                                          "function replaced(): sinteger32 " +
                            MAREA_THREE + "v = make(); v = " + MAREA_V + "{ 8 }; v = move v; return v[0]; end "
                                                                         "function bag(): sinteger32 local b: Bag = Bag{ items = make(), extra = 1 }; local c: Bag = copy b; c.items[0] = 7; c.items = make(); b.items.append(4); return (b.items[3] * 10) + c.items[0]; end "
                                                                         "function row(): sinteger32 local r: marea.array.Array<" +
                            MAREA_V + ", 2> = marea.array.Array<" + MAREA_V + ", 2>{ make(), make() }; r[1].append(4); return r[1][3]; end "
                                                                              "function nested(): sinteger32 local outer: marea.vector.Vector<" +
                            MAREA_V + "> = marea.vector.Vector<" + MAREA_V + ">{ make(), make() }; local other: marea.vector.Vector<" + MAREA_V + "> = marea.vector.Vector<" + MAREA_V + ">{}; other.append(outer.pop()); outer.append(make()); outer.erase(0); outer.insert(0, make()); outer.clear(); outer.append(make()); return other[0][2] + outer[0][1]; end "
                                                                                                                                                                                         "function walked(): sinteger32 local total: sinteger32 = 0; for value in 1 .. 2 do for each item in make() do total += item; end end return total; end "
                                                                                                                                                                                         "function early(): sinteger32 " +
                            MAREA_THREE + "while true do local w: " + MAREA_V + " = make(); return w[2] + v[0]; end return 0; end" };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck("namespace main; require marea.vector; require marea.array; " + BAG + MAKE + BODY, CALLS, &host) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_values(RESULT), (std::vector<std::int64_t>{ 3, 1, 2, 8, 41, 4, 5, 12, 4 })) << "The values of the calls.";

    for (std::size_t index{ 0 }; index < CALLS.size(); ++index) {
        const std::vector<std::int64_t> &log{ RESULT.generated[index].log };

        const auto BLOCKS{ std::count_if(log.begin(), log.end(), [](std::int64_t entry) { return (entry >= 1000) && (entry < 2000); }) };

        EXPECT_EQ(std::count(log.begin(), log.end(), 2000), BLOCKS) << MAREA_parityDescribe(CALLS[index]);
    }
}
