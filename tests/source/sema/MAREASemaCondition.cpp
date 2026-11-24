// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 CONDITION{ MAREA_SEMA_DIAGNOSTIC_CODE_NON_BOOLEAN_CONDITION };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }

    std::string MAREA_function(const std::string &statement)
    {
        return "function f(i: sinteger32, a: float64, s: string, c: character, p: boolean, q: boolean) " + statement + " end";
    }
} // namespace

TEST(MAREASemaCondition, WhileConditionNeedsBoolean)
{
    GTEST_COUT << "Validating a while condition that is not a boolean raises NON_BOOLEAN_CONDITION." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while i do return; end")), Codes{ CONDITION }) << "A sinteger32 name must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while a do return; end")), Codes{ CONDITION }) << "A float64 name must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while s do return; end")), Codes{ CONDITION }) << "A string name must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while c do return; end")), Codes{ CONDITION }) << "A character name must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while 1 do return; end")), Codes{ CONDITION }) << "An integer literal must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while 1.5 do return; end")), Codes{ CONDITION }) << "A float literal must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while \"x\" do return; end")), Codes{ CONDITION }) << "A string literal must raise 3043.";
}

TEST(MAREASemaCondition, IfAndElseifConditionNeedBoolean)
{
    GTEST_COUT << "Validating an if or elseif condition that is not a boolean raises NON_BOOLEAN_CONDITION." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("if i then return; end")), Codes{ CONDITION }) << "An integer if condition must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("if p then return; elseif i then return; end")), Codes{ CONDITION }) << "An integer elseif condition must raise 3043.";

    EXPECT_EQ(MAREA_codesOf(MAREA_function("if s then return; end")), Codes{ CONDITION }) << "A string if condition must raise 3043.";
}

TEST(MAREASemaCondition, BooleanConditionIsSilent)
{
    GTEST_COUT << "Validating a boolean condition raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while true do return; end")).empty()) << "true must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while false do return; end")).empty()) << "false must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while p do return; end")).empty()) << "A boolean name must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while i < 3 do return; end")).empty()) << "A comparison must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while p and q do return; end")).empty()) << "and over booleans must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while p or q do return; end")).empty()) << "or over booleans must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while not p do return; end")).empty()) << "not over a boolean must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("while (p) do return; end")).empty()) << "A parenthesized boolean must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("if p then return; elseif q then return; end")).empty()) << "Boolean if and elseif conditions must raise nothing.";
}

TEST(MAREASemaCondition, UnknownTypeIsSilent)
{
    GTEST_COUT << "Validating a condition of an unknown type raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(x: pointer<sinteger32>) while x do return; end end").empty()) << "A name of an unknown type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function g(): boolean return true; end function f() while g() do return; end end").empty()) << "A call must raise nothing.";
}

TEST(MAREASemaCondition, SwitchSubjectIsNotACondition)
{
    GTEST_COUT << "Validating a switch subject of an integer type raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("switch i case 1 then return; end")).empty()) << "An integer switch subject must raise nothing.";
}
