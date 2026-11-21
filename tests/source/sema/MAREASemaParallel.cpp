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

    const std::string HEAD{ "constant WORKERS: integer32 = 4; function work(part: mutable slice<float32>, factor: float32) end " };

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

    EXPECT_TRUE(MAREA_codesOf("function f(values: mutable array<float32>, k: float32) fork_join partition mutable values into 4 as part do spawn work(part, k); end end end").empty()) << "A part and a read-only parameter must raise nothing.";
}

TEST(MAREASemaParallel, LiteralsAndConstantsAreValidArguments)
{
    GTEST_COUT << "Validating a literal and a constant are valid spawn arguments." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32>) fork_join partition values into 4 as part do spawn work(part, 2.0); end end end").empty()) << "A float literal must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32, b: string, c: boolean) end function f(values: array<float32>) fork_join partition values into 4 as part do spawn g(7, \"x\", true); end end end").empty()) << "Literals must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: integer32) end function f(values: array<float32>) fork_join partition values into 4 as part do spawn g(WORKERS); end end end").empty()) << "A constant must raise nothing.";
}

TEST(MAREASemaParallel, ReadOnlyIdentifierIsAValidArgument)
{
    GTEST_COUT << "Validating a plain identifier that is not a write borrow is a valid spawn argument." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32>, k: float32) fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A read-only parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32>) local k: float32 = 2.0; fork_join partition values into 4 as part do spawn work(part, k); end end end").empty()) << "A local must raise nothing.";
}

TEST(MAREASemaParallel, WriteBorrowedParameterIsRejected)
{
    GTEST_COUT << "Validating a write-borrowed parameter cannot be spawned over." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: mutable array<float32>) end function f(m: mutable array<float32>, values: array<float32>) fork_join partition values into 4 as part do spawn g(m); end end end"), Codes{ INVALID_SPAWN }) << "A write borrow must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, LocalShadowingAWriteBorrowIsAccepted)
{
    GTEST_COUT << "Validating a local that shadows a write-borrowed parameter is not a write borrow." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: array<float32>) end function f(m: mutable array<float32>, values: array<float32>) local m: array<float32> = array<float32>{}; fork_join partition values into 4 as part do spawn g(m); end end end").empty()) << "A shadowing local must raise nothing.";
}

TEST(MAREASemaParallel, SourceOfAnActiveMutablePartitionIsRejected)
{
    GTEST_COUT << "Validating the source of an active partition mutable cannot be spawned over." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: array<float32>) end function f(values: mutable array<float32>) fork_join partition mutable values into 4 as part do spawn g(values); end end end"), Codes{ INVALID_SPAWN }) << "The source of a mutable partition must raise INVALID_SPAWN_ARGUMENT.";

    EXPECT_TRUE(MAREA_codesOf("function g(a: array<float32>) end function f(values: array<float32>) fork_join partition values into 4 as part do spawn g(values); end end end").empty()) << "The source of a read-only partition must raise nothing.";
}

TEST(MAREASemaParallel, OtherExpressionsAreRejected)
{
    GTEST_COUT << "Validating an argument that is not a name, a literal or a constant is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(a: integer32) end function f(values: array<float32>) fork_join partition values into 4 as part do spawn g(1 + 2); end end end"), Codes{ INVALID_SPAWN }) << "A computed argument must raise INVALID_SPAWN_ARGUMENT.";
}

TEST(MAREASemaParallel, SpawnOutsideForkJoinIsNotChecked)
{
    GTEST_COUT << "Validating a spawn outside a fork_join block raises no spawn diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function g(a: mutable array<float32>) end function f(m: mutable array<float32>) spawn g(m); end").empty()) << "A spawn outside fork_join must raise nothing here.";
}

TEST(MAREASemaParallel, ErrorNodesAreSkipped)
{
    GTEST_COUT << "Validating a broken fork_join, partition or spawn argument raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32>) fork_join").empty()) << "A broken fork_join must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32>) fork_join partition values into as part do end end end").empty()) << "A broken partition must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<float32>) fork_join partition values into 4 as part do spawn work(part, ); end end end").empty()) << "A broken argument must raise no sema diagnostic.";
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
