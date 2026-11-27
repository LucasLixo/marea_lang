// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };

    std::vector<MAREA_Uinteger32> MAREA_codesOf(const std::string &source)
    {
        MAREA_SemaFixture fixture;

        MAREA_IGNORE(fixture.MAREA_sema("namespace main; " + source));

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaFunctionOrder, CallOfALaterFunctionIsUndeclared)
{
    GTEST_COUT << "Validating a call of a function that is declared later raises UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function first(): sinteger32 return second(); end function second(): sinteger32 return 1; end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "A later function must raise one UNDECLARED_NAME.";
}

TEST(MAREASemaFunctionOrder, CallOfAnEarlierFunctionIsFine)
{
    GTEST_COUT << "Validating a call of an earlier function raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function second(): sinteger32 return 1; end function first(): sinteger32 return second(); end").empty()) << "An earlier function must raise nothing.";
}

TEST(MAREASemaFunctionOrder, RecursiveCallIsFine)
{
    GTEST_COUT << "Validating a recursive call raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function again(count: sinteger32): sinteger32 return again(count); end").empty()) << "A function may call itself.";
}

TEST(MAREASemaFunctionOrder, FunctionNameAsValueBeforeItsDeclarationIsUndeclared)
{
    GTEST_COUT << "Validating a function name used as a value before its declaration raises UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function first(): sinteger32 local value: function() = second; return 1; end function second() end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "A later function name as a value must raise one UNDECLARED_NAME.";
}

TEST(MAREASemaFunctionOrder, ImplementationFunctionCallOfALaterTopLevelFunctionIsUndeclared)
{
    GTEST_COUT << "Validating a call from an implementation function to a later top-level function raises UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure Planet mass: float64; end implementation Planet function pull(self: Planet): sinteger32 return later(); end end function later(): sinteger32 return 1; end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "A later top-level function must raise one UNDECLARED_NAME.";
}

TEST(MAREASemaFunctionOrder, ImportedFunctionIsFine)
{
    GTEST_COUT << "Validating a call of a function that a c. import brings raises nothing." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    MAREA_IGNORE(fixture.MAREA_sema("namespace main; import c.lib { work as work }; function first(): sinteger32 return work(); end"));

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "An imported function must raise nothing.";
}
