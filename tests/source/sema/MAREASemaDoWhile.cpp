// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 NON_BOOLEAN{ MAREA_SEMA_DIAGNOSTIC_CODE_NON_BOOLEAN_CONDITION };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };
    constexpr MAREA_Uinteger32 JUMP_OUTSIDE{ MAREA_SEMA_DIAGNOSTIC_CODE_JUMP_OUTSIDE_LOOP };
    constexpr MAREA_Uinteger32 MISSING_RETURN{ MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_RETURN };

    // INFO: the body runs in a function with a boolean parameter flag and an integer parameter count.
    Codes MAREA_codesOfBody(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main;\nfunction check(flag: boolean, count: sinteger32)\n" + body + "\nend\n") != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaDoWhile, ABooleanConditionIsAccepted)
{
    GTEST_COUT << "Validating a do-while with a boolean condition raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    local index: sinteger32 = 0;\n    do\n        index += 1;\n    while index < count end"), Codes{});
    EXPECT_EQ(MAREA_codesOfBody("    do\n    while flag end"), Codes{});
    EXPECT_EQ(MAREA_codesOfBody("    do\n    while true end"), Codes{});
}

TEST(MAREASemaDoWhile, ANonBooleanConditionIsRejected)
{
    GTEST_COUT << "Validating a do-while whose condition is a literal or a value of another type raises NON_BOOLEAN_CONDITION." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    do\n    while 1 end"), Codes{ NON_BOOLEAN });
    EXPECT_EQ(MAREA_codesOfBody("    do\n    while \"text\" end"), Codes{ NON_BOOLEAN });
    EXPECT_EQ(MAREA_codesOfBody("    do\n    while count end"), Codes{ NON_BOOLEAN });
}

TEST(MAREASemaDoWhile, ANameOfTheBodyIsNotVisibleInTheCondition)
{
    GTEST_COUT << "Validating a name declared in the body ends at the while, so the condition cannot read it (decision 4)." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    do\n        local inner: sinteger32 = 0;\n    while inner < 3 end"), Codes{ UNDECLARED });
}

TEST(MAREASemaDoWhile, ANameOfTheOuterScopeIsVisibleInTheCondition)
{
    GTEST_COUT << "Validating a name declared before the loop is read by the condition." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    local outer: sinteger32 = 0;\n    do\n        outer += 1;\n    while outer < 3 end"), Codes{});
}

TEST(MAREASemaDoWhile, ANameOfTheBodyMayBeDeclaredAgainAfterTheLoop)
{
    GTEST_COUT << "Validating the scope of the body ends with the statement, so the name can be declared again after it." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    do\n        local inner: sinteger32 = 0;\n    while flag end\n    local inner: sinteger32 = 1;"), Codes{});
}

TEST(MAREASemaDoWhile, BreakAndContinueAreValidInTheBody)
{
    GTEST_COUT << "Validating break and continue in the body of a do-while are accepted, and a break outside every loop still is not." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    do\n        if flag then\n            continue;\n        end\n        break;\n    while flag end"), Codes{});
    EXPECT_EQ(MAREA_codesOfBody("    do\n    while flag end\n    break;"), Codes{ JUMP_OUTSIDE });
}

TEST(MAREASemaDoWhile, TheLoopDepthEndsWithTheStatement)
{
    GTEST_COUT << "Validating a break after the do-while is outside a loop, and a break inside a nested while is accepted." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    do\n        while flag do\n            break;\n        end\n    while flag end"), Codes{});
    EXPECT_EQ(MAREA_codesOfBody("    do\n    while flag end\n    continue;"), Codes{ JUMP_OUTSIDE });
}

TEST(MAREASemaDoWhile, ADoWhileDoesNotEndAPathThatMustReturn)
{
    GTEST_COUT << "Validating a function with a result still needs a return after a do-while, because the loop does not end a path by itself." << std::endl;

    MAREA_SemaFixture missing([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(missing.MAREA_sema("namespace main;\nfunction value(flag: boolean): sinteger32\n    do\n        return 1;\n    while flag end\nend\n"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(missing.MAREA_semaCodes(), Codes{ MISSING_RETURN });

    MAREA_SemaFixture present([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(present.MAREA_sema("namespace main;\nfunction value(flag: boolean): sinteger32\n    do\n        return 1;\n    while flag end\n    return 2;\nend\n"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(present.MAREA_semaCodes(), Codes{});
}

TEST(MAREASemaDoWhile, TheNamesOfTheBodyAreCheckedLikeAnyBlock)
{
    GTEST_COUT << "Validating an undeclared name in the body of a do-while is reported once." << std::endl;

    EXPECT_EQ(MAREA_codesOfBody("    do\n        local x: sinteger32 = missing;\n    while flag end"), Codes{ UNDECLARED });
}

TEST(MAREASemaDoWhile, TheTypedTreeIsBuiltForADoWhile)
{
    GTEST_COUT << "Validating a function with a do-while loop still builds the typed tree." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema("namespace main;\nfunction run(flag: boolean)\n    do\n        break;\n    while flag end\nend\n"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";
    ASSERT_EQ(fixture.MAREA_semaCodes(), Codes{}) << "The program must be accepted.";

    const MAREA_SemaTypedNode *root{ nullptr };

    EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must be built.";
    EXPECT_NE(root, nullptr) << "The typed tree must have a root.";
}
