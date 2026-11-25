// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return ((text.compare(0, 12, "collections.") == 0) || ((text.compare(0, 2, "c.") == 0) || (text.compare(0, 6, "marea.") == 0) || (text.compare(0, 5, "demo.") == 0))) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    const std::string BOX{ "structure Box x: sinteger32; end implementation Box #[operator = \"destroy\"] function drop() end end " };

    // The body of the function named run, as a render.
    std::string MAREA_lowerBody(const std::string &prefix, const std::string &signature, const std::string &body, const std::string &result = "")
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower("namespace main; " + prefix + " function run(" + signature + ")" + result + " " + body + " end") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((function == nullptr) || (MAREA_IrNodeGetChildCount(function) < 2)) {
            return "<no body>";
        }

        return MAREA_irRender(MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1));
    }
} // namespace

TEST(MAREAIrLanguageUpdate, NestedSwitchesGetTwoNumberedNames)
{
    GTEST_COUT << "Validating two nested switch statements declare two different hidden names, the outer one first." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody("", "a: sinteger32, b: sinteger32", "switch a case 1 then switch b case 2 then return; end default then return; end") };

EXPECT_NE(RENDERED.find("(LOCAL_DECLARATION $switch0 (IDENTIFIER a))"), std::string::npos) << "The outer switch must be $switch0: " <<
RENDERED
;

EXPECT_NE(RENDERED.find("(LOCAL_DECLARATION $switch1 (IDENTIFIER b))"), std::string::npos) << "The inner switch must be $switch1: " <<
RENDERED
;

EXPECT_NE(RENDERED.find("(IDENTIFIER $switch1) (INTEGER_LITERAL 2)"), std::string::npos) << "The test of the inner case must read $switch1: " <<
RENDERED
;
}

TEST(MAREAIrLanguageUpdate, SwitchNumbersRestartInEachFunction)
{
    GTEST_COUT << "Validating the counter of the hidden switch names restarts at zero for each function." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function one(a: sinteger32) switch a case 1 then return; end end function two(a: sinteger32) switch a case 1 then return; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_NE(MAREA_irRender(MAREA_irChild(fixture.module, 1)).find("$switch0"), std::string::npos) << "The second function must start again at zero.";
}

TEST(MAREAIrLanguageUpdate, TwoHoistedReturnsGetTwoNumberedNames)
{
    GTEST_COUT << "Validating two value returns of one function hoist two different hidden names." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(BOX, "flag: boolean", "local a: Box = Box(); if flag then return 1; end return 2;", ": sinteger32") };

EXPECT_NE(RENDERED.find("(LOCAL_DECLARATION $return0 (INTEGER_LITERAL 1))"), std::string::npos) << "The first hoisted return must be $return0: " <<
RENDERED
;

EXPECT_NE(RENDERED.find("(LOCAL_DECLARATION $return1 (INTEGER_LITERAL 2))"), std::string::npos) << "The second hoisted return must be $return1: " <<
RENDERED
;

EXPECT_NE(RENDERED.find("(RETURN_STATEMENT (IDENTIFIER $return1))"), std::string::npos) << "The second return must read $return1: " <<
RENDERED
;
}

TEST(MAREAIrLanguageUpdate, NestedHoistedReturnKeepsOneNumberedName)
{
    GTEST_COUT << "Validating a return that two destructible scopes wrap is hoisted once, under one numbered name." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(BOX, "flag: boolean", "local a: Box = Box(); if flag then local b: Box = Box(); return a.x; end unreachable!();", ": sinteger32") };

EXPECT_NE(RENDERED.find("(LOCAL_DECLARATION $return0 "), std::string::npos) << "The hoisted name must be numbered: " <<
RENDERED
;

EXPECT_EQ(RENDERED.find("$return1"), std::string::npos) << "The outer wrap must not hoist the value again: " <<
RENDERED
;
}
