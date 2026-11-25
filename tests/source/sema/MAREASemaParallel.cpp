// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 INVALID_SPAWN{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT };

    constexpr MAREA_Uinteger32 OUTSIDE_BLOCK{ MAREA_SEMA_DIAGNOSTIC_CODE_PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN };

    constexpr MAREA_Uinteger32 SPAWN_NEEDS_CALL{ MAREA_SEMA_DIAGNOSTIC_CODE_SPAWN_NEEDS_CALL };

    constexpr MAREA_Uinteger32 PARTITION_NOT_ARRAY{ MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_SOURCE_NOT_ARRAY };

    constexpr MAREA_Uinteger32 INVALID_JUMP{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_PARALLEL_JUMP };

    constexpr MAREA_Uinteger32 CONFLICT{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_PARALLEL_ACCESS };

    constexpr MAREA_Uinteger32 OUTER_WRITE{ MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_BODY_WRITES_OUTER_NAME };

    constexpr MAREA_Uinteger32 SOURCE_USED{ MAREA_SEMA_DIAGNOSTIC_CODE_PARTITION_SOURCE_USED_IN_BODY };

    const std::string SHARED{ "structure P x: sinteger32; y: sinteger32; end function w(#[semantics = reference_mutable] p: P) end function r(p: P): sinteger32 return p.x; end " };

    const std::string HEAD{ "constant WORKERS: sinteger32 = 4; function work(part: slice<float32>, factor: float32) end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; require c.array; require c.slice; require c.task; require c.pointer; require c.vector; require c.table; require c.owned; require vulkan; " + HEAD + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaParallel, PartitionPartIsAValidArgument)
{
    GTEST_COUT << "Validating a partition part is a valid spawn argument." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] values: c.array<float32>, k: float32) fork_join partition mutable values into 4 as part do spawn work(part, k); end end end").empty()) << "A part and a read-only parameter must raise nothing.";
}

TEST(MAREASemaParallel, LiteralsAndConstantsAreValidArguments)
{
    GTEST_COUT << "Validating a literal and a constant are valid spawn arguments." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn work(part, 2.0); end end end").empty()) << "A float literal must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32, b: string, c: boolean) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g(7, \"x\", true); end end end").empty()) << "Literals must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: character) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g('x'); end end end").empty()) << "A character literal must raise nothing.";

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
    GTEST_COUT << "Validating a write-borrowed parameter cannot be spawned over, and a default parameter (a read-only borrow) can." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: c.array<float32>) end function f(#[semantics = reference_mutable] m: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(m); end end end"), Codes{ INVALID_SPAWN }) << "An explicit write borrow must raise INVALID_SPAWN_ARGUMENT.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: c.array<float32>) end function f(m: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(m); end end end").empty()) << "A default parameter (the non-primitive default is a read-only borrow) must raise nothing.";
}

TEST(MAREASemaParallel, LocalShadowingAWriteBorrowIsAccepted)
{
    GTEST_COUT << "Validating a local that shadows a write-borrowed parameter is not a write borrow." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: c.array<float32>) end function f(#[semantics = reference_mutable] m: c.array<float32>, values: c.array<float32>) do local m: c.array<float32> = c.array<float32>{}; fork_join partition values into 4 as part do spawn g(m); end end end end").empty()) << "A shadowing local must raise nothing.";
}

TEST(MAREASemaParallel, SourceOfAnActiveMutablePartitionIsRejected)
{
    GTEST_COUT << "Validating the source of an active partition mutable cannot be spawned over." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: c.array<float32>) end function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 4 as part do spawn g(values); end end end"), Codes{ INVALID_SPAWN }) << "The source of a mutable partition must raise INVALID_SPAWN_ARGUMENT.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: c.array<float32>) end function f(#[semantics = reference_read_only] values: c.array<float32>) fork_join partition values into 4 as part do spawn g(values); end end end").empty()) << "The source of a read-only partition must raise nothing.";
}

TEST(MAREASemaParallel, OtherExpressionsAreRejected)
{
    GTEST_COUT << "Validating an argument that is not a name, a literal or a constant is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) end function f(values: c.array<float32>) fork_join partition values into 4 as part do spawn g(1 + 2); end end end"), Codes{ INVALID_SPAWN }) << "A computed argument must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, FieldOfASafeNameIsAValidArgument)
{
    GTEST_COUT << "Validating a field of a name that is safe to spawn over is accepted." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: float32) end function f(#[semantics = reference_read_only] p: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(p.size); end end end").empty()) << "A field of a read-only parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: float32) end function f(#[semantics = reference_read_only] p: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(p.a.b); end end end").empty()) << "A nested field of a read-only parameter must raise nothing.";
}

TEST(MAREASemaParallel, FieldOfAnUnsafeNameIsRejected)
{
    GTEST_COUT << "Validating a field of a write borrow or of a mutable partition source is still rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: float32) end function f(#[semantics = reference_mutable] m: c.array<float32>, values: c.array<float32>) fork_join partition values into 4 as part do spawn g(m.size); end end end"), Codes{ INVALID_SPAWN }) << "A field of a write borrow must raise INVALID_SPAWN_ARGUMENT.";

    EXPECT_EQ(MAREA_codesOf("function g(a: float32) end function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 4 as part do spawn g(values.size); end end end"), Codes{ INVALID_SPAWN }) << "A field of a mutable partition source must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, SpawnOutsideForkJoinIsRejected)
{
    GTEST_COUT << "Validating a spawn outside every fork_join block raises PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: c.array<float32>) end function f(m: c.array<float32>) spawn g(m); end"), Codes{ OUTSIDE_BLOCK }) << "A spawn in a plain function body must raise 3062.";

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) end function f(a: sinteger32) fork_join spawn g(a); end spawn g(a); end"), Codes{ OUTSIDE_BLOCK }) << "A spawn after the end of a block must raise 3062.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32) end function f(a: sinteger32) fork_join spawn g(a); end end").empty()) << "A spawn inside a block must raise nothing.";
}

TEST(MAREASemaParallel, SpawnOutsideForkJoinSkipsTheArgumentRules)
{
    GTEST_COUT << "Validating a spawn outside a block raises only 3062, not 3020 too." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) end function f(a: sinteger32) spawn g(1 + 2); end"), Codes{ OUTSIDE_BLOCK }) << "A computed argument outside a block must raise only 3062.";
}

TEST(MAREASemaParallel, PartitionOutsideForkJoinIsRejected)
{
    GTEST_COUT << "Validating a partition outside every fork_join block raises PARALLEL_CONSTRUCT_OUTSIDE_FORK_JOIN." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) partition values into 2 as part do end end"), Codes{ OUTSIDE_BLOCK }) << "A partition with no block must raise 3062.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do end end end").empty()) << "A partition inside a block must raise nothing.";
}

TEST(MAREASemaParallel, NestedForkJoinAcceptsSpawnAndPartition)
{
    GTEST_COUT << "Validating a spawn and a partition in a nested fork_join block raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: sinteger32) end function f(values: c.array<float32>, a: sinteger32) fork_join fork_join spawn g(a); partition values into 2 as part do end end end end").empty()) << "A nested block must raise nothing.";
}

TEST(MAREASemaParallel, ForkDepthIsPerFunction)
{
    GTEST_COUT << "Validating a spawn in a function declared after a function with a block raises 3062." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: sinteger32) end function f(a: sinteger32) fork_join spawn g(a); end end function h(a: sinteger32) spawn g(a); end"), Codes{ OUTSIDE_BLOCK }) << "The depth must not leak from one function to the next.";
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

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do if a then continue; end end end end end"), Codes{ INVALID_JUMP }) << "A continue to a while loop must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) for i in 0 .. 3 do fork_join partition values into 2 as part do continue; end end end end"), Codes{ INVALID_JUMP }) << "A continue to a for-range loop must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, others: c.array<float32>) for each other in others do fork_join partition values into 2 as part do continue; end end end end"), Codes{ INVALID_JUMP }) << "A continue to a for each loop must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, BreakOutOfAPartitionBodyIsRejected)
{
    GTEST_COUT << "Validating a break in a partition body that targets a loop outside the fork_join is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do break; end end end end"), Codes{ INVALID_JUMP }) << "A break to a while loop must raise INVALID_PARALLEL_JUMP.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] values: c.array<float32>) for i in 0 .. 3 do fork_join partition mutable values into 2 as part do break; end end end end"), Codes{ INVALID_JUMP }) << "A break of a mutable partition must raise INVALID_PARALLEL_JUMP.";
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

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: sinteger32) while true do fork_join partition values into 2 as part do switch a case 1 then continue; default then end end end end end"), Codes{ INVALID_JUMP }) << "A continue in a switch arm must raise INVALID_PARALLEL_JUMP.";
}

TEST(MAREASemaParallel, EveryJumpOfABodyIsReported)
{
    GTEST_COUT << "Validating each jump that leaves a partition body is reported once." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do fork_join partition values into 2 as part do if a then break; end continue; return; end end end end"), (Codes{ INVALID_JUMP, INVALID_JUMP, INVALID_JUMP })) << "Three jumps must raise three diagnostics.";
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

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) fork_join partition values into 2 as part do while a do if a then break; end continue; end end end end").empty()) << "A while loop of the body must raise nothing.";

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

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, a: boolean) while a do if a then break; end end end").empty()) << "A break in a loop must raise nothing.";
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

TEST(MAREASemaParallel, SpawnOfAValueThatIsNotACallIsRejected)
{
    GTEST_COUT << "Validating a spawn whose operand is not a call raises SPAWN_NEEDS_CALL." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Point x: sinteger32; end function f(point: Point) fork_join spawn point; end end"), Codes{ SPAWN_NEEDS_CALL }) << "A spawn of a name must raise 3063.";

    EXPECT_EQ(MAREA_codesOf("function f() fork_join spawn 1; end end"), Codes{ SPAWN_NEEDS_CALL }) << "A spawn of a literal must raise 3063.";

    EXPECT_EQ(MAREA_codesOf("function f() fork_join spawn print!(\"x\"); end end"), Codes{ SPAWN_NEEDS_CALL }) << "A spawn of a macro call must raise 3063.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>, k: float32) fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A spawn of a call must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(a: sinteger32) spawn a; end"), Codes{ OUTSIDE_BLOCK }) << "A spawn outside a block must raise only 3062.";
}

TEST(MAREASemaParallel, PartitionOfAValueThatIsNotAnArrayIsRejected)
{
    GTEST_COUT << "Validating a partition of a value that is known not to be an array raises PARTITION_SOURCE_NOT_ARRAY." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() local count: sinteger32 = 4; fork_join partition count into 2 as part do end end end"), Codes{ PARTITION_NOT_ARRAY }) << "A partition of a primitive must raise 3064.";

    EXPECT_EQ(MAREA_codesOf("structure Point x: sinteger32; end function f(point: Point) fork_join partition point into 2 as part do end end end"), Codes{ PARTITION_NOT_ARRAY }) << "A partition of a structure must raise 3064.";

    EXPECT_EQ(MAREA_codesOf("enum Kind A; B; end function f(kind: Kind) fork_join partition kind into 2 as part do end end end"), Codes{ PARTITION_NOT_ARRAY }) << "A partition of an enum must raise 3064.";

    EXPECT_EQ(MAREA_codesOf("function f() fork_join partition 4 into 2 as part do end end end"), Codes{ PARTITION_NOT_ARRAY }) << "A partition of a number must raise 3064.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do end end end").empty()) << "A partition of a library array must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32, 4>) fork_join partition values into 2 as part do end end end").empty()) << "A partition of an array must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: vector<float32>) fork_join partition values into 2 as part do end end end").empty()) << "A partition of a vector must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: slice<float32>) fork_join partition values into 2 as part do end end end").empty()) << "A partition of a slice must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: Unknown) fork_join partition values into 2 as part do end end end").empty()) << "A partition of an unclassified value must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(count: sinteger32) partition count into 2 as part do end end"), Codes{ OUTSIDE_BLOCK }) << "A partition outside a block must raise only 3062.";
}

TEST(MAREASemaParallel, TwoSpawnsThatWriteOneLocalAreRejected)
{
    GTEST_COUT << "Validating two spawns of one fork_join block cannot both write one local." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn w(a); spawn w(a); end end"), Codes{ CONFLICT }) << "Two workers that write one local must raise CONFLICTING_PARALLEL_ACCESS once.";

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn w(a); spawn r(a); end end"), Codes{ CONFLICT }) << "A worker that reads what another worker writes must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn r(a); spawn w(a); end end"), Codes{ CONFLICT }) << "A worker that writes what an earlier worker reads must raise CONFLICTING_PARALLEL_ACCESS.";
}

TEST(MAREASemaParallel, SpawnsThatOnlyReadOrUseOtherValuesAreAccepted)
{
    GTEST_COUT << "Validating workers that share only reads, or write different values, raise nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn r(a); spawn r(a); end end").empty()) << "Two readers of one local must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; local b: P = P{ x = 1, y = 2 }; fork_join spawn w(a); spawn w(b); end end").empty()) << "Two writers of two locals must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn w(a); end fork_join spawn w(a); end end").empty()) << "Two writers in two fork_join blocks, one after the other, must raise nothing.";
}

TEST(MAREASemaParallel, SpawnInALoopThatWritesAnOuterLocalIsRejected)
{
    GTEST_COUT << "Validating a spawn in a loop that writes a local declared before the block repeats the write." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join for i in 0 .. 3 do spawn w(a); end end end"), Codes{ CONFLICT }) << "A spawn in a loop that writes one outer local must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "function f() fork_join for i in 0 .. 3 do local a: P = P{ x = 1, y = 2 }; spawn w(a); end end end").empty()) << "A spawn in a loop that writes a local of the loop body must raise nothing.";
}

TEST(MAREASemaParallel, PartitionPartsAndSourcesAreCompared)
{
    GTEST_COUT << "Validating a part that two workers write, and a source that a worker writes while a partition reads it, are rejected." << std::endl;

    const std::string SCALE{ "function scale(#[semantics = reference_mutable] part: c.slice<float32>, factor: float32) end function sum(part: c.slice<float32>): float32 return 0.0; end " };

    EXPECT_EQ(MAREA_codesOf(SCALE + "function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do spawn scale(part, 2.0); spawn scale(part, 3.0); end end end"), Codes{ CONFLICT }) << "One part passed to two writers must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_EQ(MAREA_codesOf(SCALE + "function f(values: c.array<float32>) local copy_of: c.array<float32> = copy values; fork_join partition copy_of into 2 as part do spawn sum(part); spawn scale(copy_of, 2.0); end end end"), Codes{ CONFLICT }) << "A worker that writes the source of a partition must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_TRUE(MAREA_codesOf(SCALE + "function f(#[semantics = reference_mutable] values: c.array<float32>, #[semantics = reference_mutable] other: c.array<float32>) fork_join partition mutable values into 2 as part do spawn scale(part, 2.0); end partition mutable other into 2 as part do spawn scale(part, 2.0); end end end").empty()) << "Two partitions that each name their part part must raise nothing.";
}

TEST(MAREASemaParallel, FieldsOfOneValueAreComparedByPath)
{
    GTEST_COUT << "Validating a field and its owner overlap, and two other fields do not." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHARED + "structure H a: P; b: P; end function f() local h: H = H{ a = P{ x = 1, y = 2 }, b = P{ x = 1, y = 2 } }; fork_join spawn w(h.a); spawn r(h.a); end end"), Codes{ CONFLICT }) << "A write and a read of one field must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "structure H a: P; b: P; end function f() local h: H = H{ a = P{ x = 1, y = 2 }, b = P{ x = 1, y = 2 } }; fork_join spawn w(h.a); spawn w(h.b); end end").empty()) << "Writes of two fields must raise nothing.";
}

TEST(MAREASemaParallel, BlockBodyAndWorkerAreCompared)
{
    GTEST_COUT << "Validating the code of the fork_join block itself cannot use what a worker writes, or write what a worker reads." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn w(a); local x: sinteger32 = r(a); end end"), Codes{ CONFLICT }) << "A read in the block body of what a worker writes must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn w(a); a.y = 5; end end"), Codes{ CONFLICT }) << "A write in the block body of what a worker writes must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn r(a); a.y = 5; end end"), Codes{ CONFLICT }) << "A write in the block body of what a worker reads must raise CONFLICTING_PARALLEL_ACCESS.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn r(a); local x: sinteger32 = r(a); end end").empty()) << "A read in the block body of what a worker reads must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join a.y = 5; spawn w(a); end end").empty()) << "A write in the block body before the worker starts must raise nothing.";
}

TEST(MAREASemaParallel, NestedForkJoinIsComparedWithTheOuterBlock)
{
    GTEST_COUT << "Validating a nested fork_join block is compared with what the outer block captured." << std::endl;

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f() local a: P = P{ x = 1, y = 2 }; fork_join spawn w(a); fork_join spawn r(a); end end end"), Codes{ CONFLICT }) << "A nested worker that reads what an outer worker writes must raise CONFLICTING_PARALLEL_ACCESS.";
}

TEST(MAREASemaParallel, PartitionBodyThatWritesAnOuterNameIsRejected)
{
    GTEST_COUT << "Validating a partition body cannot write a local or a parameter declared outside it." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) local counter: sinteger32 = 0; fork_join partition values into 2 as part do counter = counter + 1; end end end"), Codes{ OUTER_WRITE }) << "An assignment to an outer local must raise 3075.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>) local counter: sinteger32 = 0; fork_join partition values into 2 as part do counter += 1; end end end"), Codes{ OUTER_WRITE }) << "A compound assignment to an outer local must raise 3075.";

    EXPECT_EQ(MAREA_codesOf(SHARED + "function f(values: c.array<float32>) local point: P = P{ x = 1, y = 2 }; fork_join partition values into 2 as part do point.x = 2; end end end"), Codes{ OUTER_WRITE }) << "A write to a field of an outer local must raise 3075.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, total: sinteger32) fork_join partition values into 2 as part do total = 1; end end end"), Codes{ OUTER_WRITE }) << "A write to an outer parameter must raise 3075.";

    EXPECT_EQ(MAREA_codesOf("function f(values: c.array<float32>, #[semantics = reference_mutable] other: c.array<float32>) fork_join partition values into 2 as part do other[0] = 1.0; end end end"), Codes{ OUTER_WRITE }) << "A write to an element of an outer array must raise 3075.";
}

TEST(MAREASemaParallel, PartitionBodyThatWritesItsOwnNamesIsAccepted)
{
    GTEST_COUT << "Validating a partition body may write a name it declares, its part and a file constant is not a local." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do local inner: sinteger32 = 0; inner = 1; end end end").empty()) << "A write to a local of the body must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do part[0] = 1.0; end end end").empty()) << "A write through the part must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) local counter: sinteger32 = 0; fork_join partition values into 2 as part do local first: sinteger32 = counter; end end end").empty()) << "A read of an outer local must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do for i in 0 .. 3 do i = 2; end end end end").empty()) << "A write to a loop variable of the body must raise nothing.";

    const Codes CONSTANT_CODES{ MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do WORKERS = 1; end end end") };

    EXPECT_EQ(std::count(CONSTANT_CODES.begin(), CONSTANT_CODES.end(), OUTER_WRITE), 0) << "A write to a file constant must raise nothing from this rule.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) local counter: sinteger32 = 0; fork_join partition values into 2 as part do local counter: sinteger32 = 0; counter = 1; end end end").empty()) << "A body local that hides an outer name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do partition part into 2 as inner do local x: sinteger32 = 0; x = 1; end end end end end").empty()) << "A nested partition that writes its own local must raise nothing.";
}

TEST(MAREASemaParallel, SourceOfAPartitionMutableUsedInItsBodyIsRejected)
{
    GTEST_COUT << "Validating the source of a partition mutable cannot be used in its body." << std::endl;

    const std::string SCALE{ "function scale(#[semantics = reference_mutable] part: c.slice<float32>, factor: float32) end " };

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do values[0] = 1.0; end end end"), Codes{ SOURCE_USED }) << "A write to the source must raise 3076.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do local first: float32 = values[0]; end end end"), Codes{ SOURCE_USED }) << "A read of the source must raise 3076.";

    EXPECT_EQ(MAREA_codesOf("function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do for each value in values do end end end end"), Codes{ SOURCE_USED }) << "A loop over the source must raise 3076.";

    EXPECT_EQ(MAREA_codesOf(SCALE + "function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do spawn scale(values, 2.0); end end end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_SPAWN_ARGUMENT }) << "The source as a spawn argument must raise 3020 and no 3076.";

    EXPECT_TRUE(MAREA_codesOf(SCALE + "function f(#[semantics = reference_mutable] values: c.array<float32>) fork_join partition mutable values into 2 as part do spawn scale(part, 2.0); part[0] = 1.0; end end end").empty()) << "The part must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: c.array<float32>) fork_join partition values into 2 as part do local first: float32 = values[0]; end end end").empty()) << "A partition that is not mutable has no 3076 rule.";

    EXPECT_TRUE(MAREA_codesOf(SHARED + "structure H values: sinteger32; end function f(#[semantics = reference_mutable] values: c.array<float32>, other: H) fork_join partition mutable values into 2 as part do local first: sinteger32 = other.values; end end end").empty()) << "A member named like the source must raise nothing.";
}
