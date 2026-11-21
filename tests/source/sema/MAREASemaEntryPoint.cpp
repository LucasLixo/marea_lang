// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 INVALID_MAIN{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MAIN_SIGNATURE };

    Codes MAREA_codesOf(const std::string &source, const std::string &path = "main.marea")
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(source, path) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaEntryPoint, ExactSignatureIsAccepted)
{
    GTEST_COUT << "Validating the exact main signature raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: array<string>): integer32 return 0; end").empty()) << "The exact signature must raise nothing.";
}

TEST(MAREASemaEntryPoint, SameSignatureInAnotherNamespaceIsNotChecked)
{
    GTEST_COUT << "Validating a function named main outside the main namespace is not checked." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace tools.runner; function main(x: integer32): boolean return true; end", "tools/runner.marea").empty()) << "Another namespace must raise nothing.";
}

TEST(MAREASemaEntryPoint, WrongParameterTypeIsRejected)
{
    GTEST_COUT << "Validating a wrong parameter type of main is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: array<integer32>): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "A wrong element type must raise INVALID_MAIN_SIGNATURE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: string): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "A wrong type must raise INVALID_MAIN_SIGNATURE.";
}

TEST(MAREASemaEntryPoint, ParameterFlagIsRejected)
{
    GTEST_COUT << "Validating a mutable or reference flag on the arguments is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: mutable array<string>): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "A mutable flag must raise INVALID_MAIN_SIGNATURE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: reference array<string>): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "A reference flag must raise INVALID_MAIN_SIGNATURE.";
}

TEST(MAREASemaEntryPoint, WrongReturnTypeIsRejected)
{
    GTEST_COUT << "Validating a wrong or missing return type of main is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: array<string>): integer64 return 0; end"), Codes{ INVALID_MAIN }) << "A wrong return type must raise INVALID_MAIN_SIGNATURE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: array<string>) end"), Codes{ INVALID_MAIN }) << "A missing return type must raise INVALID_MAIN_SIGNATURE.";
}

TEST(MAREASemaEntryPoint, WrongParameterCountOrNameIsRejected)
{
    GTEST_COUT << "Validating a main with no parameter, two parameters or another name is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "No parameter must raise INVALID_MAIN_SIGNATURE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(arguments: array<string>, extra: integer32): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "Two parameters must raise INVALID_MAIN_SIGNATURE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; function main(args: array<string>): integer32 return 0; end"), Codes{ INVALID_MAIN }) << "Another parameter name must raise INVALID_MAIN_SIGNATURE.";
}

TEST(MAREASemaEntryPoint, NamespaceWithoutMainIsAccepted)
{
    GTEST_COUT << "Validating a main namespace with no main function raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function helper() end").empty()) << "A missing main must raise nothing.";
}

TEST(MAREASemaEntryPoint, OtherFunctionsInMainNamespaceAreNotChecked)
{
    GTEST_COUT << "Validating only the function named main is checked." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function other(x: integer32): boolean return true; end").empty()) << "Another function must raise nothing.";
}
