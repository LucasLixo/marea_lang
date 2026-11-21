// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        const bool EXTERNAL{ (text.compare(0, 12, "collections.") == 0) || (text.compare(0, 12, "concurrency.") == 0) || (text.compare(0, 7, "arenas.") == 0) };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    // INFO: the source is a namespace, the prefix declarations, then function run with the signature and the body.
    std::string MAREA_sourceOf(const std::string &prefix, const std::string &signature, const std::string &body)
    {
        return "namespace main; import collections.array; import concurrency.task; import arenas.arena; " + prefix + " function run(" + signature + ") " + body + " end";
    }

    std::string MAREA_lowerBody(const std::string &prefix, const std::string &signature, const std::string &body)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(MAREA_sourceOf(prefix, signature, body)) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((function == nullptr) || (MAREA_IrNodeGetChildCount(function) < 2)) {
            return "<no body>";
        }

        return MAREA_irRender(MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1));
    }

    std::vector<MAREA_Uinteger32> MAREA_codesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_irCodes();
    }

    std::string MAREA_functionNamesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        std::string names;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
            names += (names.empty() ? "" : " ") + MAREA_irText(MAREA_irChild(fixture.module, index));
        }

        return names;
    }

    const std::string WORK{ "function work(part: array<float32>) end function scale(part: mutable array<float32>, factor: float32) end" };

    using Codes = std::vector<MAREA_Uinteger32>;
} // namespace

TEST(MAREAIrParallel, EmptyForkJoinKeepsOnlyItsBlock)
{
    GTEST_COUT << "Validating an empty fork_join lowers to a FORK_JOIN_BLOCK with one empty BLOCK." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "fork_join end"), "(BLOCK (FORK_JOIN_BLOCK (BLOCK)))") << "The join point must be its own node with the body as its only child.";
}

TEST(MAREAIrParallel, ForkJoinKeepsItsStatementsInOrder)
{
    GTEST_COUT << "Validating a fork_join keeps each statement of its body in source order." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function ping() end", "", "fork_join local first: integer32 = 1; ping(); end"), "(BLOCK (FORK_JOIN_BLOCK (BLOCK (LOCAL_DECLARATION first :integer32 (INTEGER_LITERAL 1)) (EXPRESSION_STATEMENT (CALL_EXPRESSION (IDENTIFIER ping) (ARGUMENT_LIST))))))") << "The body must keep its order.";
}

TEST(MAREAIrParallel, SpawnStatementIsAUnaryExpressionWithTheSpawnOperator)
{
    GTEST_COUT << "Validating a spawn statement lowers to a UNARY_EXPRESSION with op KEYWORD_SPAWN over the call, with no new kind." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function ping() end", "", "fork_join spawn ping(); end"), "(BLOCK (FORK_JOIN_BLOCK (BLOCK (EXPRESSION_STATEMENT (UNARY_EXPRESSION op=KEYWORD_SPAWN (CALL_EXPRESSION (IDENTIFIER ping) (ARGUMENT_LIST)))))))") << "The spawn must keep its operator and its call.";
}

TEST(MAREAIrParallel, SpawnResultBoundToALocalKeepsItsTypeName)
{
    GTEST_COUT << "Validating a spawn used as the initializer of a local keeps the local type name and the spawn." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function work(a: integer32): integer32 return a; end", "", "fork_join local result: task<integer32> = spawn work(1); end"), "(BLOCK (FORK_JOIN_BLOCK (BLOCK (LOCAL_DECLARATION result :task (UNARY_EXPRESSION op=KEYWORD_SPAWN (CALL_EXPRESSION :integer32 (IDENTIFIER work) (ARGUMENT_LIST (INTEGER_LITERAL 1))))))))") << "The task local must hold the spawn.";
}

TEST(MAREAIrParallel, SpawnResultAsAnArgumentStaysInTheCall)
{
    GTEST_COUT << "Validating a spawn inside a call argument, as in tasks.append(spawn f(part)), lowers in place." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody("function work(a: integer32): integer32 return a; end", "tasks: mutable array<task<integer32>>", "fork_join tasks.append(spawn work(1)); end") };

EXPECT_EQ(RENDERED, "(BLOCK (FORK_JOIN_BLOCK (BLOCK (EXPRESSION_STATEMENT (CALL_EXPRESSION (MEMBER_ACCESS (IDENTIFIER tasks) (IDENTIFIER append)) (ARGUMENT_LIST (UNARY_EXPRESSION op=KEYWORD_SPAWN (CALL_EXPRESSION :integer32 (IDENTIFIER work) (ARGUMENT_LIST (INTEGER_LITERAL 1))))))))))") << "The spawn must stay an argument.";
}

TEST(MAREAIrParallel, PartitionKeepsNameValueCountAndBody)
{
    GTEST_COUT << "Validating a read-only partition keeps the part name as text, then the value, the count and the body, with no flag." << std::endl;

    EXPECT_EQ(MAREA_lowerBody(WORK, "values: array<float32>", "fork_join partition values into 4 as part do spawn work(part); end end"), "(BLOCK (FORK_JOIN_BLOCK (BLOCK (PARTITION_STATEMENT part (IDENTIFIER values) (INTEGER_LITERAL 4) (BLOCK (EXPRESSION_STATEMENT (UNARY_EXPRESSION op=KEYWORD_SPAWN (CALL_EXPRESSION (IDENTIFIER work) (ARGUMENT_LIST (IDENTIFIER part))))))))))") << "The partition must keep its layout.";
}

TEST(MAREAIrParallel, PartitionMutableKeepsTheMutableFlag)
{
    GTEST_COUT << "Validating a mutable partition carries the MUTABLE flag on its node." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(WORK, "values: mutable array<float32>", "fork_join partition mutable values into 4 as part do spawn scale(part, 2.0); end end") };

EXPECT_NE(RENDERED.find("(PARTITION_STATEMENT part flags=1 (IDENTIFIER values) (INTEGER_LITERAL 4) (BLOCK"), std::string::npos) << "The flag must sit on the partition node: " <<
RENDERED
;
}

TEST(MAREAIrParallel, PartitionCountIsAnyLoweredExpression)
{
    GTEST_COUT << "Validating the part count of a partition is lowered like any expression, a constant name here." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody("constant WORKERS: integer32 = 4; " + WORK, "values: array<float32>", "fork_join partition values into WORKERS as part do spawn work(part); end end") };

EXPECT_NE(RENDERED.find("(PARTITION_STATEMENT part (IDENTIFIER values) (IDENTIFIER WORKERS) (BLOCK"), std::string::npos) << "The count must be an IDENTIFIER child: " <<
RENDERED
;
}

TEST(MAREAIrParallel, WithKeepsNameValueAndBody)
{
    GTEST_COUT << "Validating a with statement keeps the bound name as text, then the value and the body." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "scratch: mutable arena", "with scratch as inner do end"), "(BLOCK (WITH_STATEMENT inner (IDENTIFIER scratch) (BLOCK)))") << "The with must keep its layout.";
}

TEST(MAREAIrParallel, WithValueIsALoweredCallAndTheBodyKeepsItsStatements)
{
    GTEST_COUT << "Validating the value of a with statement may be a call and its body keeps its statements." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody("function open_buffer(a: integer32): integer32 return a; end function use(a: integer32) end", "", "with open_buffer(1) as buffer do use(buffer); end") };

EXPECT_EQ(RENDERED, "(BLOCK (WITH_STATEMENT buffer (CALL_EXPRESSION :integer32 (IDENTIFIER open_buffer) (ARGUMENT_LIST (INTEGER_LITERAL 1))) (BLOCK (EXPRESSION_STATEMENT (CALL_EXPRESSION (IDENTIFIER use) (ARGUMENT_LIST (IDENTIFIER buffer)))))))") << "The with must lower its value and its body.";
}

TEST(MAREAIrParallel, ForkJoinNestsInsideWithAndWithInsideForkJoin)
{
    GTEST_COUT << "Validating a fork_join inside a with, and a with inside a fork_join, nest in source order." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "scratch: mutable arena", "with scratch as inner do fork_join with inner as again do end end end"), "(BLOCK (WITH_STATEMENT inner (IDENTIFIER scratch) (BLOCK (FORK_JOIN_BLOCK (BLOCK (WITH_STATEMENT again (IDENTIFIER inner) (BLOCK)))))))") << "The nesting must follow the source.";
}

TEST(MAREAIrParallel, ContinueInsideWithTargetsTheEnclosingForRangeAndGetsItsStep)
{
    GTEST_COUT << "Validating a continue inside a with body inside a for-range loop still runs the step of the loop first." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody("", "scratch: mutable arena, a: boolean", "for index in 0 .. 3 do with scratch as inner do if a then continue; end end end") };

EXPECT_NE(RENDERED.find("(WITH_STATEMENT inner (IDENTIFIER scratch) (BLOCK (IF_STATEMENT (IDENTIFIER a) (BLOCK (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))))))"), std::string::npos) << "The continue must carry the step: " <<
RENDERED
;
}

TEST(MAREAIrParallel, ForkJoinRaisesNothingAndKeepsItsFunction)
{
    GTEST_COUT << "Validating a fork_join, a partition and a spawn raise no ir diagnostic and their function stays in the module." << std::endl;

    const std::string SOURCE{ MAREA_sourceOf("function keep() end " + WORK, "values: array<float32>", "fork_join partition values into 2 as part do spawn work(part); end end") };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{}) << "No ir diagnostic may be raised.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep work scale run") << "Every function, the one with the fork_join included, must remain.";
}

TEST(MAREAIrParallel, WithRaisesNothingAndKeepsItsFunction)
{
    GTEST_COUT << "Validating a with statement raises no ir diagnostic and its function stays in the module." << std::endl;

    const std::string SOURCE{ MAREA_sourceOf("", "scratch: mutable arena", "with scratch as inner do end") };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{}) << "No ir diagnostic may be raised.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "run") << "The function with the with must remain.";
}

TEST(MAREAIrParallel, GapInsideAForkJoinBodyDropsTheFunctionAndReportsOnlyThatGap)
{
    GTEST_COUT << "Validating a macro gap inside a fork_join body raises only its own code and drops the function whole." << std::endl;

    const std::string SOURCE{ MAREA_sourceOf("function keep() end", "", "fork_join print!(\"hi\"); end") };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "Only the macro gap may be raised.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "The function with the gap must be left out, the other one kept.";
}

TEST(MAREAIrParallel, GapInsideAWithValueDropsTheFunction)
{
    GTEST_COUT << "Validating a macro gap in the value of a with statement raises its own code and drops the function." << std::endl;

    const std::string SOURCE{ MAREA_sourceOf("function keep() end", "", "with print!(\"hi\") as inner do end") };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "Only the macro gap may be raised.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "The function with the gap must be left out.";
}

TEST(MAREAIrParallel, DumpWritesTheThreeNewKindsWithTheirNameAndFlag)
{
    GTEST_COUT << "Validating the text dump of a fork_join, a mutable partition and a with is exact." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower(MAREA_sourceOf(WORK, "values: mutable array<float32>, scratch: mutable arena", "fork_join partition mutable values into 2 as part do spawn scale(part, 1.0); end end with scratch as inner do end")), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length), MAREA_IR_EXCEPT_SUCCESS) << "The dump must succeed.";

    const std::string DUMP(text, length);

    const std::string WANTED{
        "    BLOCK\n"
        "      FORK_JOIN_BLOCK\n"
        "        BLOCK\n"
        "          PARTITION_STATEMENT \"part\" flags=1\n"
        "            IDENTIFIER \"values\"\n"
        "            INTEGER_LITERAL \"2\"\n"
        "            BLOCK\n"
        "              EXPRESSION_STATEMENT\n"
        "                UNARY_EXPRESSION op=KEYWORD_SPAWN\n"
        "                  CALL_EXPRESSION\n"
        "                    IDENTIFIER \"scale\"\n"
        "                    ARGUMENT_LIST\n"
        "                      IDENTIFIER \"part\"\n"
        "                      FLOAT_LITERAL \"1.0\"\n"
        "      WITH_STATEMENT \"inner\"\n"
        "        IDENTIFIER \"scratch\"\n"
        "        BLOCK\n"
    };

    EXPECT_NE(DUMP.find(WANTED), std::string::npos) << "The dump must hold the exact lines:\n"
                                                    << DUMP;
}

TEST(MAREAIrParallel, ScalingExampleKeepsItsThreeParallelFunctions)
{
    GTEST_COUT << "Validating examples/parallel/scaling.marea lowers whole: every function stays and no ir diagnostic is raised." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("parallel/scaling.marea"), "parallel/scaling.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    EXPECT_EQ(fixture.MAREA_irCodes(), Codes{}) << "The example must raise no ir diagnostic.";

    std::string names;

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
        names += " " + MAREA_irText(MAREA_irChild(fixture.module, index));
    }

    EXPECT_EQ(names, " WORKER_COUNT scale_slice sum_slice scale_all sum_all") << "The constant and the four functions must stay in source order.";

    const std::string
RENDERED
{ MAREA_irRender(fixture.module) };

EXPECT_NE(RENDERED.find("(PARTITION_STATEMENT part flags=1 (IDENTIFIER values) (IDENTIFIER WORKER_COUNT)"), std::string::npos) << "The mutable partition of scale_all must be present.";

EXPECT_NE(RENDERED.find("(PARTITION_STATEMENT part (IDENTIFIER values) (IDENTIFIER WORKER_COUNT)"), std::string::npos) << "The read-only partition of sum_all must be present.";
}
