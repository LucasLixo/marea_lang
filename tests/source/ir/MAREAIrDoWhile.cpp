// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const std::string BOX{ "structure Box x: sinteger32; end implementation Box #[operator = \"destroy\"] function drop() end end " };

    // The body of the function named run, as a render, or a marker.
    std::string MAREA_lowerBody(const std::string &prefix, const std::string &body, MAREA_IrFixture &fixture)
    {
        if (fixture.MAREA_lower("namespace main; " + prefix + " function run(flag: boolean) local x: sinteger32 = 0; " + body + " end") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((function == nullptr) || (MAREA_IrNodeGetChildCount(function) < 2) || (MAREA_irText(function) != "run")) {
            return "<no function>";
        }

        return MAREA_irRender(MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1));
    }

    const std::string LEAVE{ "(IF_STATEMENT (UNARY_EXPRESSION op=KEYWORD_NOT (BINARY_EXPRESSION op=LESS (IDENTIFIER x) (INTEGER_LITERAL 3))) (BLOCK (BREAK_STATEMENT)))" };
} // namespace

TEST(MAREAIrDoWhile, PlainDoWhileLowersToAnEndlessWhileThatLeavesAtTheTest)
{
    GTEST_COUT << "Validating do ... while cond end lowers to while true with the body, then the test that leaves." << std::endl;

    MAREA_IrFixture fixture;

    EXPECT_EQ(MAREA_lowerBody("", "do x += 1; while x < 3 end", fixture), "(BLOCK (LOCAL_DECLARATION x :sinteger32 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BOOLEAN_LITERAL true) (BLOCK (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER x) (INTEGER_LITERAL 1))) " + LEAVE + ")))") << "The body must come first, in its own scope, then the test.";

    EXPECT_TRUE(fixture.MAREA_irCodes().empty()) << "A do-while must raise no IR diagnostic.";
}

TEST(MAREAIrDoWhile, ContinueGoesToTheTest)
{
    GTEST_COUT << "Validating a continue of the body is replaced by the test followed by the continue." << std::endl;

    MAREA_IrFixture fixture;

    EXPECT_EQ(MAREA_lowerBody("", "do if flag then continue; end x += 1; while x < 3 end", fixture), "(BLOCK (LOCAL_DECLARATION x :sinteger32 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BOOLEAN_LITERAL true) (BLOCK (BLOCK (IF_STATEMENT (IDENTIFIER flag) (BLOCK (BLOCK " + LEAVE + " (CONTINUE_STATEMENT)))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER x) (INTEGER_LITERAL 1))) " + LEAVE + ")))") << "A continue must run the test first.";
}

TEST(MAREAIrDoWhile, BreakStaysABreak)
{
    GTEST_COUT << "Validating a break of the body is kept as it is." << std::endl;

    MAREA_IrFixture fixture;

    const std::string
RENDERED
{ MAREA_lowerBody("", "do if flag then break; end while x < 3 end", fixture) };

EXPECT_NE(RENDERED.find("(IF_STATEMENT (IDENTIFIER flag) (BLOCK (BREAK_STATEMENT)))"), std::string::npos) <<
RENDERED
;
}

TEST(MAREAIrDoWhile, ContinueOfAnInnerLoopIsNotChanged)
{
    GTEST_COUT << "Validating a continue inside a nested while targets that loop, so the test is not copied there." << std::endl;

    MAREA_IrFixture fixture;

    const std::string
RENDERED
{ MAREA_lowerBody("", "do while flag do continue; end while x < 3 end", fixture) };

EXPECT_NE(RENDERED.find("(WHILE_STATEMENT (IDENTIFIER flag) (BLOCK (CONTINUE_STATEMENT)))"), std::string::npos) <<
RENDERED
;
}

TEST(MAREAIrDoWhile, NestedDoWhileKeepsEachContinueWithItsOwnTest)
{
    GTEST_COUT << "Validating the continue of an inner do-while goes to the test of the inner loop, and the outer one to its own." << std::endl;

    MAREA_IrFixture fixture;

    const std::string
RENDERED
{ MAREA_lowerBody("", "do do continue; while flag end continue; while x < 3 end", fixture) };

EXPECT_NE(RENDERED.find("(BLOCK (IF_STATEMENT (UNARY_EXPRESSION op=KEYWORD_NOT (IDENTIFIER flag)) (BLOCK (BREAK_STATEMENT))) (CONTINUE_STATEMENT))"), std::string::npos) << "The inner continue must run the inner test: " <<
RENDERED
;

EXPECT_NE(RENDERED.find("(BLOCK " + LEAVE + " (CONTINUE_STATEMENT))"), std::string::npos) << "The outer continue must run the outer test: " <<
RENDERED
;
}

TEST(MAREAIrDoWhile, TheConditionIsLoweredOnceForEachUse)
{
    GTEST_COUT << "Validating the test is a new node at each place, never one node used twice." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run(flag: boolean) do if flag then continue; end while flag end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *loop{ MAREA_irChild(MAREA_irChild(MAREA_irChild(fixture.module, 0), 1), 0) };

    const MAREA_IrNode *body{ MAREA_irChild(loop, 1) };

    const MAREA_IrNode *endTest{ MAREA_irChild(body, 1) };

    const MAREA_IrNode *innerBlock{ MAREA_irChild(MAREA_irChild(MAREA_irChild(MAREA_irChild(body, 0), 0), 1), 0) };

    EXPECT_NE(MAREA_irChild(MAREA_irChild(endTest, 0), 0), MAREA_irChild(MAREA_irChild(MAREA_irChild(innerBlock, 0), 0), 0)) << "The two tests must be two nodes.";
}

TEST(MAREAIrDoWhile, ContinueDestroysTheBodyLocalBeforeTheTest)
{
    GTEST_COUT << "Validating a continue destroys a destructible local of the body first, then runs the test." << std::endl;

    MAREA_IrFixture fixture;

    const std::string
RENDERED
{ MAREA_lowerBody(BOX, "do local b: Box = Box(); if flag then continue; end while x < 3 end", fixture) };

EXPECT_NE(RENDERED.find("(BLOCK (DESTROY_CALL Box.drop (IDENTIFIER b)) (BLOCK " + LEAVE + " (CONTINUE_STATEMENT)))"), std::string::npos) <<
RENDERED
;
}

TEST(MAREAIrDoWhile, FunctionWithADoWhileIsKeptInTheModule)
{
    GTEST_COUT << "Validating a function whose body holds a do-while is no longer dropped without a diagnostic." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run(flag: boolean) local x: sinteger32 = 0; do x += 1; while flag end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "The function must stay in the module.";
}
