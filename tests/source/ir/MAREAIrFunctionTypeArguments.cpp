// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    // The render of the first statement of the function named run, or a marker.
    std::string MAREA_firstStatement(const std::string &body, MAREA_IrFixture &fixture)
    {
        if (fixture.MAREA_lower("namespace main; function run() " + body + " end") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        const MAREA_IrNode *block{ MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1) };

        return (MAREA_IrNodeGetChildCount(block) == 0) ? "<empty>" : MAREA_irRender(MAREA_irChild(block, 0));
    }
} // namespace

TEST(MAREAIrFunctionTypeArguments, FunctionTypeIsATypeArgumentOfABuiltin)
{
    GTEST_COUT << "Validating size_of with a function type as its type argument lowers with the parameters and the result in the type." << std::endl;

    MAREA_IrFixture fixture;

    EXPECT_EQ(MAREA_firstStatement("local size: uinteger64 = size_of<function(n: sinteger32): sinteger32>();", fixture), "(LOCAL_DECLARATION size :uinteger64 (CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME function (PARAMETER_LIST (TYPE_NAME sinteger32)) (TYPE_NAME sinteger32))) (ARGUMENT_LIST)))") << "The function type must hold its parameter types, then its result.";

    EXPECT_TRUE(fixture.MAREA_irCodes().empty()) << "No IR diagnostic may be raised.";
}

TEST(MAREAIrFunctionTypeArguments, FunctionTypeWithNoParameterAndNoResultHoldsAnEmptyList)
{
    GTEST_COUT << "Validating a function type with no parameter and no result keeps one empty PARAMETER_LIST and no result." << std::endl;

    MAREA_IrFixture fixture;

    EXPECT_NE(MAREA_firstStatement("local size: uinteger64 = size_of<function()>();", fixture).find("(TYPE_NAME function (PARAMETER_LIST))"), std::string::npos) << "The empty list must be there so a result is always the second child.";
}

TEST(MAREAIrFunctionTypeArguments, FunctionTypeAsAParameterOfAFunctionTypeLowers)
{
    GTEST_COUT << "Validating a function type nested as the parameter of a function type lowers in full." << std::endl;

    MAREA_IrFixture fixture;

    EXPECT_NE(MAREA_firstStatement("local size: uinteger64 = size_of<function(f: function(a: boolean): boolean): sinteger32>();", fixture).find("(TYPE_NAME function (PARAMETER_LIST (TYPE_NAME function (PARAMETER_LIST (TYPE_NAME boolean)) (TYPE_NAME boolean))) (TYPE_NAME sinteger32))"), std::string::npos) << "A function type as a parameter must lower in full.";
}

TEST(MAREAIrFunctionTypeArguments, FunctionTypeInTheDumpIsStable)
{
    GTEST_COUT << "Validating the text dump prints the function type argument one node per line." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run() local size: uinteger64 = size_of<function(n: sinteger32): boolean>(); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length), MAREA_IR_EXCEPT_SUCCESS) << "The dump must succeed.";

    const std::string DUMP{ text, length };

    EXPECT_NE(DUMP.find("BUILTIN_NAME \"size_of\"\n            TYPE_NAME \"function\"\n              PARAMETER_LIST\n                TYPE_NAME \"sinteger32\"\n              TYPE_NAME \"boolean\"\n"), std::string::npos) << DUMP;
}
