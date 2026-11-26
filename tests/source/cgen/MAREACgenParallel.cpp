// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    constexpr int MAREA_TRAP_INVALID_PARTITION_COUNT{ 11 };

    const std::string MAREA_DATA{ "constant DATA: demo.array<sinteger32, 4> = demo.array<sinteger32, 4>{ 1, 2, 3, 4 }; " };

    MAREA_ParityResult MAREA_run(const std::string &body, const std::vector<MAREA_ParityCall> &calls = {})
    {
        const MAREA_ParityHost HOST{ MAREA_parityHost() };

        return MAREA_parityCheck(MAREA_PARITY_HOST_PRELUDE + body, calls, &HOST);
    }

    std::vector<std::int64_t> MAREA_log(const std::vector<std::int64_t> &entries)
    {
        return entries;
    }
} // namespace

TEST(MAREACgenParallel, SpawnsRunInSourceOrderAndEndBeforeTheBlockDoes)
{
    GTEST_COUT << "Validating a fork_join runs each spawn where it stands, also nested ones, before the statement after it." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function chain(depth: sinteger32) if depth < 16 then host.record(depth); local next: sinteger32 = depth + 1; fork_join spawn chain(next); end end end function nothing() end function f(): sinteger32 host.record(100); fork_join spawn chain(0); host.record(101); spawn nothing(); end host.record(102); return 0; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.MAREA_outcome("f()").log, MAREA_log({ 100, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 101, 102 })) << "The order of the source.";
}

TEST(MAREACgenParallel, ASpawnPassesABorrowAndATrapStopsTheRun)
{
    GTEST_COUT << "Validating a spawned call gets the local itself, and a fault in it means no later worker starts." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_run("function divide(a: sinteger32, b: sinteger32): sinteger32 return a / b; end function f(): sinteger32 local a: Pair = Pair{ x = 1, y = 2 }; fork_join spawn host.bump(a); end return a.x; end function g(): sinteger32 fork_join spawn divide(1, 0); spawn host.record(9); end host.record(10); return 0; end") };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 2) << "The spawned call changed the local.";

    EXPECT_EQ(RESULT.MAREA_outcome("g()").trap, 1) << "The fault of the worker.";

    EXPECT_TRUE(RESULT.MAREA_outcome("g()").log.empty()) << "Nothing runs after it.";
}

TEST(MAREACgenParallel, APartitionRunsItsBodyOncePerPart)
{
    GTEST_COUT << "Validating the body of a partition runs once for each part, also nested and with more parts than elements." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(3) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(1) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(6) }),
        MAREA_parityCall("g", "sinteger32"),
    };

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_DATA + "function f(n: sinteger32): sinteger32 fork_join partition DATA into n as part do host.record(n); spawn host.record(1); end end return 0; end function g(): sinteger32 fork_join partition DATA into 2 as part do partition DATA into 3 as other do local i: sinteger32 = 0; while true do i += 1; if i == 2 then break; end end host.record(i); end end end return 0; end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(RESULT.generated[0].log, MAREA_log({ 3, 1, 3, 1, 3, 1 })) << "Three parts.";

    EXPECT_EQ(RESULT.generated[2].log.size(), 12U) << "An empty part still runs the body.";

    EXPECT_EQ(RESULT.generated[3].log, MAREA_log({ 2, 2, 2, 2, 2, 2 })) << "Two times three runs.";
}

TEST(MAREACgenParallel, APartitionIntoNoPartTraps)
{
    GTEST_COUT << "Validating a part count of zero or less is the trap of a bad part count and the body does not run." << std::endl;

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(0) }),
        MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(-1) }),
        MAREA_parityCall("g", "sinteger32", { MAREA_parityUnsigned("uinteger64", 0) }),
    };

    const MAREA_ParityResult RESULT{ MAREA_run(MAREA_DATA + "function f(n: sinteger32): sinteger32 fork_join partition DATA into n as part do host.record(1); end end return 0; end function g(n: uinteger64): sinteger32 fork_join partition DATA into n as part do host.record(1); end end return 0; end", CALLS) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    for (const MAREA_ParityOutcome &outcome : RESULT.generated) {
        EXPECT_EQ(outcome.trap, MAREA_TRAP_INVALID_PARTITION_COUNT) << "The trap of a bad part count.";

        EXPECT_TRUE(outcome.log.empty()) << "No body ran.";
    }
}

TEST(MAREACgenParallel, APartOrATaskHandleUsedAsAValueIsReported)
{
    GTEST_COUT << "Validating the part of a partition and the handle of a spawn have no form yet, and are diagnostics." << std::endl;

    const MAREA_ParityHost HOST{ MAREA_parityHost() };

    EXPECT_EQ(MAREA_parityRejected(MAREA_PARITY_HOST_PRELUDE + MAREA_DATA + "function f() fork_join partition DATA into 2 as part do host.record(part[0]); end end end", &HOST).count(MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE), 1U) << "The part is not a value.";

    EXPECT_EQ(MAREA_parityRejected("namespace main; function one(): sinteger32 return 1; end function f() fork_join local task: demo.task<sinteger32> = spawn one(); end end").empty(), false) << "A handle has no form.";
}
