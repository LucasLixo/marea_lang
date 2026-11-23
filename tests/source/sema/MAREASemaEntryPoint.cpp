// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

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
    GTEST_COUT << "Validating the former fixed main signature raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: array<string>): sinteger32 return 0; end").empty()) << "The former signature must raise nothing.";
}

TEST(MAREASemaEntryPoint, SameSignatureInAnotherNamespaceIsNotChecked)
{
    GTEST_COUT << "Validating a function named main outside the main namespace is not checked." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace tools.runner; function main(x: sinteger32): boolean return true; end", "tools/runner.marea").empty()) << "Another namespace must raise nothing.";
}

TEST(MAREASemaEntryPoint, AnyParameterTypeIsAccepted)
{
    GTEST_COUT << "Validating a main with any parameter type raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: array<sinteger32>): sinteger32 return 0; end").empty()) << "Another element type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: string): sinteger32 return 0; end").empty()) << "Another type must raise nothing.";
}

TEST(MAREASemaEntryPoint, ParameterFlagIsAccepted)
{
    GTEST_COUT << "Validating a mutable or reference flag on a main parameter raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: mutable array<string>): sinteger32 return 0; end").empty()) << "A mutable flag must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: reference array<string>): sinteger32 return 0; end").empty()) << "A reference flag must raise nothing.";
}

TEST(MAREASemaEntryPoint, AnyReturnTypeIsAccepted)
{
    GTEST_COUT << "Validating a main with another or no return type raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: array<string>): sinteger64 return 0; end").empty()) << "Another return type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: array<string>) end").empty()) << "A missing return type must raise nothing.";
}

TEST(MAREASemaEntryPoint, AnyParameterCountOrNameIsAccepted)
{
    GTEST_COUT << "Validating a main with no parameter, two parameters or another name raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(): sinteger32 return 0; end").empty()) << "No parameter must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(arguments: array<string>, extra: sinteger32): sinteger32 return 0; end").empty()) << "Two parameters must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; function main(args: array<string>): sinteger32 return 0; end").empty()) << "Another parameter name must raise nothing.";
}

TEST(MAREASemaEntryPoint, NamespaceWithoutMainIsAccepted)
{
    GTEST_COUT << "Validating a main namespace with no main function raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function helper() end").empty()) << "A missing main must raise nothing.";
}

TEST(MAREASemaEntryPoint, OtherFunctionsInMainNamespaceAreNotChecked)
{
    GTEST_COUT << "Validating only the function named main is checked." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; function other(x: sinteger32): boolean return true; end").empty()) << "Another function must raise nothing.";
}
