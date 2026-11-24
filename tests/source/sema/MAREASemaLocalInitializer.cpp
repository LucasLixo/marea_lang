// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 NO_VALUE{ MAREA_SEMA_DIAGNOSTIC_CODE_LOCAL_WITHOUT_INITIAL_VALUE };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; import c.lib; structure Point x: sinteger32; end " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }

    std::string MAREA_function(const std::string &statement)
    {
        return "function g(flag: boolean) " + statement + " end";
    }
} // namespace

TEST(MAREASemaLocalInitializer, LocalWithoutValueIsRejectedForEveryKindOfType)
{
    GTEST_COUT << "Validating a local with no value raises LOCAL_WITHOUT_INITIAL_VALUE for a primitive, a structure, a pointer and an array." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local a: sinteger32;")), Codes{ NO_VALUE }) << "A primitive local with no value must raise 3065.";
    EXPECT_EQ(MAREA_codesOf(MAREA_function("local a: Point;")), Codes{ NO_VALUE }) << "A structure local with no value must raise 3065.";
    EXPECT_EQ(MAREA_codesOf(MAREA_function("unsafe local a: pointer<sinteger32>; end")), Codes{ NO_VALUE }) << "A pointer local with no value must raise 3065.";
    EXPECT_EQ(MAREA_codesOf(MAREA_function("local a: array<sinteger32, 4>;")), Codes{ NO_VALUE }) << "An array local with no value must raise 3065.";
}

TEST(MAREASemaLocalInitializer, LocalWithValueIsSilent)
{
    GTEST_COUT << "Validating a local with a written value, also a literal in braces, raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local a: sinteger32 = 0;")).empty()) << "A local with = 0 must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf(MAREA_function("local a: Point = Point{ x = 1 };")).empty()) << "A local with a literal in braces must raise nothing.";
}

TEST(MAREASemaLocalInitializer, EachLocalWithoutValueRaisesOnce)
{
    GTEST_COUT << "Validating two locals with no value in one function raise two diagnostics." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("local a: sinteger32; local b: float64;")), (Codes{ NO_VALUE, NO_VALUE })) << "Two locals with no value must raise 3065 twice.";
}

TEST(MAREASemaLocalInitializer, NestedBlocksAreChecked)
{
    GTEST_COUT << "Validating a local with no value in an if, a switch arm and a loop raises 3065 for each." << std::endl;

    EXPECT_EQ(MAREA_codesOf(MAREA_function("while flag do local a: sinteger32; end")), Codes{ NO_VALUE }) << "A local in a loop must raise 3065.";
    EXPECT_EQ(MAREA_codesOf(MAREA_function("if flag then local a: sinteger32; end")), Codes{ NO_VALUE }) << "A local in an if must raise 3065.";
    EXPECT_EQ(MAREA_codesOf("function s(n: sinteger32) switch n case 1 then local a: sinteger32; end end"), Codes{ NO_VALUE }) << "A local in a switch arm must raise 3065.";
}
