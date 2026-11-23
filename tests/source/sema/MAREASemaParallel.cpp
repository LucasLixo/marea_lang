// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <fstream>
#include <sstream>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 INVALID_SPAWN{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT };

    constexpr MAREA_Uinteger32 INVALID_JUMP{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP };

    const std::string HEAD{ "constant WORKERS: sinteger32 = 4; function work(part: mutable slice<float32>, factor: float32) end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + HEAD + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaParallel, PartitionPartIsAValidArgument)
{
    GTEST_COUT << "Validating a partition part is a valid spawn argument." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: mutable c.array<float32>, k: float32) fork_join partition mutable values into 4 as part do spawn work(part, k); end end end").empty()) << "A part and a read-only parameter must raise nothing.";
}

TEST(MAREASemaParallel, LiteralsAndConstantsAreValidArguments)
{
    GTEST_COUT << "Validating a literal and a constant are valid spawn arguments." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn work(part, 2.0); end end end").empty()) << "A float literal must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32, b: string, c: boolean) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g(7, \"x\", true); end end end").empty()) << "Literals must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g(WORKERS); end end end").empty()) << "A constant must raise nothing.";
}

TEST(MAREASemaParallel, ReadOnlyIdentifierIsAValidArgument)
{
    GTEST_COUT << "Validating a plain identifier that is not a write borrow is a valid spawn argument." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, k: float32) fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A read-only parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) local k: float32 = 2.0; fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A local must raise nothing.";
}

TEST(MAREASemaParallel, WriteBorrowedParameterIsRejected)
{
    GTEST_COUT << "Validating a write-borrowed parameter cannot be spawned over." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: mutable c.array<float32>) end function f(m: mutable c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(m); end end end"), Codes{ INVALID_SPAWN }) << "A write borrow must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, LocalShadowingAWriteBorrowIsAccepted)
{
    GTEST_COUT << "Validating a local that shadows a write-borrowed parameter is not a write borrow." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: c.array<float32>) end function f(m: mutable c.array<float32>, values: c.array<float32>) local m: c.array<float32> = c.array<float32>{}; fork_join partition values into 4 as part do spawn g(m); end end end").empty()) << "A shadowing local must raise nothing.";
}

TEST(MAREASemaParallel, SourceOfAnActiveMutablePartitionIsRejected)
{
    GTEST_COUT << "Validating the source of an active partition mutable cannot be spawned over." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: c.array<float32>) end function f(values: mutable c.array<float32>) fork_join partition mutable values into 4 as part do spawn g(values); end end end"), Codes{ INVALID_SPAWN }) << "The source of a mutable partition must raise INVALID_SPAWN_ARGUMENT.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: c.array<float32>) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g(values); end end end").empty()) << "The source of a read-only partition must raise nothing.";
}

TEST(MAREASemaParallel, OtherExpressionsAreRejected)
{
    GTEST_COUT << "Validating an argument that is not a name, a literal or a constant is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g(1 + 2); end end end"), Codes{ INVALID_SPAWN }) << "A computed argument must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, FieldOfASafeNameIsAValidArgument)
{
    GTEST_COUT << "Validating a field of a name that is safe to spawn over is accepted." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: float32) end function f(p: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(p.size); end end end").empty()) << "A field of a read-only parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: float32) end function f(p: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(p.a.b); end end end").empty()) << "A nested field of a read-only parameter must raise nothing.";
}

TEST(MAREASemaParallel, FieldOfAnUnsafeNameIsRejected)
{
    GTEST_COUT << "Validating a field of a write borrow or of a mutable partition source is still rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: float32) end function f(m: mutable c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(m.size); end end end"), Codes{ INVALID_SPAWN }) << "A field of a write borrow must raise INVALID_SPAWN_ARGUMENT.";

    EXPECT_EQ(MAREA_codesOf("function g(a: float32) end function f(values: mutable c.array<float32>) fork_join partition mutable values into 4 as part do spawn g(values.size); end end end"), Codes{ INVALID_SPAWN }) << "A field of a mutable partition source must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, SpawnOutsideForkJoinIsNotChecked)
{
    GTEST_COUT << "Validating a spawn outside a fork_join block raises no spawn diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: mutable c.array<float32>) end function f(m: mutable c.array<float32>) spawn g(m); end").empty()) << "A spawn outside fork_join must raise nothing here.";
}

TEST(MAREASemaParallel, ErrorNodesAreSkipped)
{
    GTEST_COUT << "Validating a broken fork_join, partition or spawn argument raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join").empty()) << "A broken fork_join must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into as part do end end end").empty()) << "A broken partition must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn work(part, ); end end end").empty()) << "A broken argument must raise no sema diagnostic.";
}

TEST(MAREASemaParallel, ScalingExampleSpawnsPass)
{
    GTEST_COUT << "Validating the spawn calls of examples/parallel/scaling.marea raise no spawn diagnostic." << std::endl;

    std::ifstream stream(std::string(MAREA_TESTS_EXAMPLES_DIRECTORY) + "/parallel/scaling.marea", std::ios::binary);

    std::ostringstream buffer;

    buffer << stream.rdbuf();

    ASSERT_FALSE(buffer.str().empty()) << "The example must be readable.";

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema(buffer.str(), "parallel/scaling.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    for (MAREA_Uinteger32 code : fixture.MAREA_semaCodes()) {
        EXPECT_NE(code, INVALID_SPAWN) << "No spawn argument of the example may be rejected.";
    }
}

TEST(MAREASemaParallel, JumpCodeIsPinned)
{
    GTEST_COUT << "Validating the diagnostic code of a jump out of a partition body." << std::endl;

    EXPECT_EQ(INVALID_JUMP, 3024U) << "INVALID_PARALLEL_JUMP must be 3024.";
}

TEST(MAREASemaParallel, ContinueOutOfAPartitionBodyIsRejected)
{
    GTEST_COUT << "Validating a continue in a partition body that targets a loop outside the fork_join is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do if a then continue; end end end end end end"), Codes{ INVALID_JUMP }) << "A continue to a while loop must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) for i in 0 .. 3 do fork_join partition values into 2 as part do continue; end end end end"), Codes{ INVALID_JUMP }) << "A continue to a for-range loop must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, others: c.array<float32>) for each other in others do fork_join partition values into 2 as part do continue; end end end end"), Codes{ INVALID_JUMP }) << "A continue to a for each loop must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, BreakOutOfAPartitionBodyIsRejected)
{
    GTEST_COUT << "Validating a break in a partition body that targets a loop outside the fork_join is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do break; end end end end"), Codes{ INVALID_JUMP }) << "A break to a while loop must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) for i in 0 .. 3 do fork_join partition mutable values into 2 as part do break; end end end end"), Codes{ INVALID_JUMP }) << "A break of a mutable partition must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, ReturnInAPartitionBodyIsRejected)
{
    GTEST_COUT << "Validating a return in a partition body is rejected, with or without a value." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do return; end end end"), Codes{ INVALID_JUMP }) << "A return with no value must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>): sinteger32 fork_join partition values into 2 as part do return 1; end end return 0; end"), Codes{ INVALID_JUMP }) << "A return with a value must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as part do while a do return; end end end end"), Codes{ INVALID_JUMP }) << "A return inside a loop of the body must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, JumpThroughAWithOrSwitchInABodyIsRejected)
{
    GTEST_COUT << "Validating a jump that leaves a partition body through a with, an if or a switch is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do with values as view do break; end end end end end"), Codes{ INVALID_JUMP }) << "A break through a with must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: sinteger32) while true do fork_join partition values into 2 as part do switch a case 1 then continue; end default then end end end end end end"), Codes{ INVALID_JUMP }) << "A continue in a switch arm must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, EveryJumpOfABodyIsReported)
{
    GTEST_COUT << "Validating each jump that leaves a partition body is reported once." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do if a then break; end end continue; return; end end end end"), (Codes{ INVALID_JUMP, INVALID_JUMP, INVALID_JUMP })) << "Three jumps must raise three diagnostics.";
}

TEST(MAREASemaParallel, JumpInANestedPartitionIsRejectedOnce)
{
    GTEST_COUT << "Validating the nearest partition body is the barrier of a jump." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as outer do while a do fork_join partition values into 2 as inner do continue; end end end end end end"), Codes{ INVALID_JUMP }) << "The inner body must be rejected for a continue that targets a loop of the outer body.";
}

TEST(MAREASemaParallel, NestedPartitionRestoresTheStateOfTheOuterBody)
{
    GTEST_COUT << "Validating the loop count and the partition count come back after a nested partition ends." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as outer do while a do fork_join partition values into 2 as inner do end end break; end end end end").empty()) << "A break in a loop of the outer body after the inner partition must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as outer do fork_join partition values into 2 as inner do end end return; end end end"), Codes{ INVALID_JUMP }) << "A return in the outer body after the inner partition must still raise INVALID_PARALLEL_JUMP.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as part do while a do while a do end break; end end end end").empty()) << "A break after an inner loop of the body must raise nothing.";
}

TEST(MAREASemaParallel, LoopInsideAPartitionBodyIsAccepted)
{
    GTEST_COUT << "Validating a loop that lives inside a partition body can use break and continue." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as part do while a do if a then break; end end continue; end end end end").empty()) << "A while loop of the body must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do for i in 0 .. 3 do continue; end end end end").empty()) << "A for-range loop of the body must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, others: c.array<float32>) fork_join partition values into 2 as part do for each other in others do break; end end end end").empty()) << "A for each loop of the body must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as part do while a do with values as view do break; end end end end end").empty()) << "A break through a with inside a loop of the body must raise nothing.";
}

TEST(MAREASemaParallel, LoopOutsideAPartitionBodyIsStillChecked)
{
    GTEST_COUT << "Validating the barrier is lifted after the partition body ends." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as part do end while a do break; end end end").empty()) << "A loop that follows the partition must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do while a do break; end end end end end"), Codes{}) << "A loop inside the body of a loop must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as one do end partition values into 2 as two do while a do break; end end end end").empty()) << "A second partition after a first one must still lift the barrier.";
}

TEST(MAREASemaParallel, JumpOutsideAnyPartitionBodyIsUnchanged)
{
    GTEST_COUT << "Validating a jump in a with, a loop or a plain function body raises nothing here." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do with values as view do continue; end end end").empty()) << "A continue through a with outside a partition must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean): sinteger32 with values as view do return 1; end return 0; end").empty()) << "A return inside a with outside a partition must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do if a then break; end end end end").empty()) << "A break in a loop must raise nothing.";
}

TEST(MAREASemaParallel, JumpNextToABrokenStatementIsStillReported)
{
    GTEST_COUT << "Validating a jump that follows a broken statement in a partition body is reported once." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do 1 + ; continue; end end end end"), Codes{ INVALID_JUMP }) << "A broken sibling must not hide the jump, and the broken statement must raise no sema diagnostic.";
}

TEST(MAREASemaParallel, JumpDirectlyInAForkJoinBlockIsRejected)
{
    GTEST_COUT << "Validating a jump directly in a fork_join block, outside every partition body, is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) fork_join continue; end end"), Codes{ INVALID_JUMP }) << "A continue directly in a fork_join block must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) fork_join break; end end"), Codes{ INVALID_JUMP }) << "A break directly in a fork_join block must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) fork_join return; end end"), Codes{ INVALID_JUMP }) << "A return directly in a fork_join block must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, JumpAfterASpawnDirectlyInAForkJoinBlockIsRejected)
{
    GTEST_COUT << "Validating a jump that follows a spawn directly in a fork_join block is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g() end function f(values: c.array<float32>) fork_join spawn g(); return; end end"), Codes{ INVALID_JUMP }) << "A return right after a spawn, both directly in the fork_join block, must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, LoopDirectlyInAForkJoinBlockIsAccepted)
{
    GTEST_COUT << "Validating a loop that lives directly in a fork_join block, outside any partition, can use break and continue." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join while a do break; end end end").empty()) << "A while loop directly in the fork_join block must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join for i in 0 .. 3 do continue; end end end").empty()) << "A for-range loop directly in the fork_join block must raise nothing.";
}

TEST(MAREASemaParallel, LoopOutsideTheWholeForkJoinBlockDoesNotValidateAJump)
{
    GTEST_COUT << "Validating a loop outside the whole fork_join block is not a target of a jump directly in the block." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join break; end end end"), Codes{ INVALID_JUMP }) << "A break directly in the fork_join block must not reach an outer loop; it must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join continue; end end end"), Codes{ INVALID_JUMP }) << "A continue directly in the fork_join block must not reach an outer loop; it must raise INVALID_PARALLEL_JUMP.";
}
