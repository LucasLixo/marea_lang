// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAVmHostHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    const std::string MAREA_DATA{ "constant DATA: demo.array<sinteger32, 4> = demo.array<sinteger32, 4>{ 1, 2, 3, 4 }; " };
} // namespace

TEST(MAREAVmParallel, RunsSpawnsInSourceOrderAndJoinsAtTheEnd)
{
    GTEST_COUT << "Validating a fork_join runs each spawn in source order and the statement after the block sees all of them." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 host.record(1); fork_join spawn host.record(2); host.record(3); spawn host.record(4); end host.record(5); return 0; end")) << "The unit must generate.";

    EXPECT_FALSE(fixture.MAREA_call("f").trapped) << "No trap.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 1, 2, 3, 4, 5 })) << "The order of the source.";
}

TEST(MAREAVmParallel, RunsASpawnedFunctionOfTheUnitAndAnEmptyOne)
{
    GTEST_COUT << "Validating a spawned function of the unit runs, also one that does nothing." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function nothing() end function note(value: sinteger32): sinteger32 host.record(value); return value; end function f() fork_join spawn nothing(); spawn note(7); end end")) << "The unit must generate.";

    EXPECT_FALSE(fixture.MAREA_call("f").trapped) << "No trap.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 7 })) << "The spawned function ran.";
}

TEST(MAREAVmParallel, RunsAChainOfSpawnsToItsEnd)
{
    GTEST_COUT << "Validating a spawned function that spawns again runs sixteen generations before the first block ends." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function chain(depth: sinteger32) if depth < 16 then host.record(depth); local next: sinteger32 = depth + 1; fork_join spawn chain(next); end end end function f() fork_join spawn chain(0); end host.record(100); end")) << "The unit must generate.";

    EXPECT_FALSE(fixture.MAREA_call("f").trapped) << "No trap.";

    std::vector<std::int64_t> expected{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 100 };

    EXPECT_EQ(MAREA_hostLog(), expected) << "Every generation ran, then the statement after the outer block.";
}

TEST(MAREAVmParallel, PassesABorrowToASpawnedCallAndNotACopy)
{
    GTEST_COUT << "Validating a spawned call gets the local itself and its write is there after the join." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; fork_join spawn host.bump(a); end return a.x; end")) << "The unit must generate.";

    EXPECT_EQ(static_cast<std::int32_t>(fixture.MAREA_call("f").value), 2) << "The spawned call changed the local.";
}

TEST(MAREAVmParallel, ATrapInASpawnStopsTheRun)
{
    GTEST_COUT << "Validating a trap inside a spawned call ends the run, so a later spawn does not start." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("function divide(a: sinteger32, b: sinteger32): sinteger32 return a / b; end function f() fork_join spawn divide(1, 0); spawn host.record(9); end host.record(10); end")) << "The unit must generate.";

    const MAREA_VmResult RESULT{ fixture.MAREA_call("f") };

    ASSERT_TRUE(RESULT.trapped) << "The run must trap.";

    EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO) << "The trap of the spawned call.";

    EXPECT_TRUE(MAREA_hostLog().empty()) << "Nothing runs after the trap.";
}

TEST(MAREAVmParallel, RunsTheBodyOfAPartitionOncePerPart)
{
    GTEST_COUT << "Validating the body of a partition runs once for each part, also for more parts than elements." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build(MAREA_DATA + "function f(n: sinteger32) fork_join partition DATA into n as part do host.record(n); spawn host.record(1); end end end")) << "The unit must generate.";

    EXPECT_FALSE(fixture.MAREA_call("f", { 3 }).trapped) << "Three parts.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 3, 1, 3, 1, 3, 1 })) << "Three runs of the body, one after the other.";

    MAREA_hostLog().clear();

    EXPECT_FALSE(fixture.MAREA_call("f", { 1 }).trapped) << "One part.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 1, 1 })) << "One run.";

    MAREA_hostLog().clear();

    EXPECT_FALSE(fixture.MAREA_call("f", { 6 }).trapped) << "Six parts of four elements.";

    EXPECT_EQ(MAREA_hostLog().size(), 12U) << "An empty part still runs the body.";
}

TEST(MAREAVmParallel, APartitionIntoNoPartTraps)
{
    GTEST_COUT << "Validating a part count of zero or less is the trap INVALID_PARTITION_COUNT and the body does not run." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build(MAREA_DATA + "function f(n: sinteger32) fork_join partition DATA into n as part do host.record(1); end end end function g(n: uinteger64) fork_join partition DATA into n as part do host.record(1); end end end")) << "The unit must generate.";

    for (const std::int64_t COUNT : { std::int64_t{ 0 }, std::int64_t{ -1 } }) {
        const MAREA_VmResult RESULT{ fixture.MAREA_call("f", { static_cast<std::uint64_t>(COUNT) }) };

        ASSERT_TRUE(RESULT.trapped) << "The count " << COUNT << " must trap.";

        EXPECT_EQ(RESULT.trap, MAREA_BYTECODE_TRAP_CODE_INVALID_PARTITION_COUNT) << "The trap code must say a bad part count.";
    }

    EXPECT_EQ(fixture.MAREA_call("g", { 0 }).trap, MAREA_BYTECODE_TRAP_CODE_INVALID_PARTITION_COUNT) << "An unsigned zero traps too.";

    EXPECT_TRUE(MAREA_hostLog().empty()) << "No body ran.";
}

TEST(MAREAVmParallel, RunsNestedPartitionsAndALoopInsideAPartitionBody)
{
    GTEST_COUT << "Validating a partition inside a partition, with a loop that breaks inside the inner body." << std::endl;

    MAREA_VmHostFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build(MAREA_DATA + "function f() fork_join partition DATA into 2 as part do partition DATA into 3 as other do local i: sinteger32 = 0; while true do i += 1; if i == 2 then break; end end host.record(i); end end end end")) << "The unit must generate.";

    EXPECT_FALSE(fixture.MAREA_call("f").trapped) << "No trap.";

    EXPECT_EQ(MAREA_hostLog(), (std::vector<std::int64_t>{ 2, 2, 2, 2, 2, 2 })) << "Two times three runs.";
}
