// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaCallerHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 UNKNOWN_MACRO{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO };
    constexpr MAREA_Uinteger32 INVALID_ARGUMENT{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT };
    constexpr MAREA_Uinteger32 TYPE_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 ARGUMENT_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH };
    constexpr MAREA_Uinteger32 UNIMPORTED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNIMPORTED_NAMESPACE };

    // INFO: the fixture macro include_bytes is registered by the fixture library array of the root marea, and by the
    // neutral fixture root demo, both under the owner flag of marea.array.
    MAREA_CallerInputs MAREA_withFiles()
    {
        MAREA_CallerInputs inputs;

        inputs.hasIncludes = true;
        inputs.files = { { "logo.bin", 12 }, { "empty.bin", 0 } };

        return inputs;
    }

    std::string MAREA_function(const std::string &imports, const std::string &body)
    {
        return "namespace main;\n" + imports + "function check()\n" + body + "\nend\n";
    }
} // namespace

TEST(MAREASemaRegisteredMacros, AQualifiedCallThroughAnAliasIsTypedByTheDescriptor)
{
    GTEST_COUT << "Validating a macro called through the alias of its library is accepted and typed as the library type with the file size." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array as array;\n", "    local data: marea.array.Array<uinteger8, 12> = array.include_bytes!(\"logo.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{});
}

TEST(MAREASemaRegisteredMacros, AQualifiedCallThroughTheFullPathIsAccepted)
{
    GTEST_COUT << "Validating a macro called through the full path of its library is accepted." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    local data: marea.array.Array<uinteger8, 12> = marea.array.include_bytes!(\"logo.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{});
}

TEST(MAREASemaRegisteredMacros, TheFixtureRootIsReachedTheSameWay)
{
    GTEST_COUT << "Validating a macro of a fixture library under the neutral root demo is called through its path." << std::endl;

    const std::string PROGRAM{ "namespace main;\nfunction check()\n    local data: demo.array<uinteger8, 12> = demo.array.include_bytes!(\"logo.bin\");\nend\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{});
}

TEST(MAREASemaRegisteredMacros, AnEmptyFileGivesACountOfZero)
{
    GTEST_COUT << "Validating the count that the expander gives is the one the type holds, even zero." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    local data: marea.array.Array<uinteger8, 0> = marea.array.include_bytes!(\"empty.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{});
}

TEST(MAREASemaRegisteredMacros, ADeclaredCountThatIsNotTheFileSizeIsRejected)
{
    GTEST_COUT << "Validating a declared count that differs from the size of the file raises GENERIC_ARGUMENT_MISMATCH." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    local data: marea.array.Array<uinteger8, 3> = marea.array.include_bytes!(\"logo.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{ ARGUMENT_MISMATCH });
}

TEST(MAREASemaRegisteredMacros, ADeclaredElementTypeThatIsNotTheResultIsRejected)
{
    GTEST_COUT << "Validating a declared element type that differs from the element type of the descriptor raises GENERIC_ARGUMENT_MISMATCH." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    local data: marea.array.Array<sinteger16, 12> = marea.array.include_bytes!(\"logo.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{ ARGUMENT_MISMATCH });
}

TEST(MAREASemaRegisteredMacros, TheResultFitsNoPrimitiveType)
{
    GTEST_COUT << "Validating the value of a registered macro is a library type, so a primitive target raises TYPE_MISMATCH." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    local text: string = marea.array.include_bytes!(\"logo.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{ TYPE_MISMATCH });
}

TEST(MAREASemaRegisteredMacros, AFileTheResolverDoesNotKnowIsRejected)
{
    GTEST_COUT << "Validating a file that the include resolver reports as missing raises INVALID_MACRO_ARGUMENT." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    marea.array.include_bytes!(\"missing.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_withFiles()), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaRegisteredMacros, WithNoIncludeResolverEveryFileIsMissing)
{
    GTEST_COUT << "Validating a context with no include resolver reports every file as missing." << std::endl;

    const std::string PROGRAM{ MAREA_function("require marea.array;\n", "    marea.array.include_bytes!(\"logo.bin\");") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaRegisteredMacros, TheArgumentKindIsCheckedBeforeTheExpanderRuns)
{
    GTEST_COUT << "Validating a call whose arguments are not one string literal raises INVALID_MACRO_ARGUMENT." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    marea.array.include_bytes!(1);"), MAREA_withFiles()), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    marea.array.include_bytes!();"), MAREA_withFiles()), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    marea.array.include_bytes!(\"logo.bin\", \"logo.bin\");"), MAREA_withFiles()), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    marea.array.include_bytes!(\"logo.bin\");"), MAREA_withFiles()), Codes{});
}

TEST(MAREASemaRegisteredMacros, ANameThatTheLibraryDoesNotRegisterIsUnknown)
{
    GTEST_COUT << "Validating a qualified call of a name that its library does not register raises UNKNOWN_MACRO." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    marea.array.include_nothing!(\"x\");"), MAREA_withFiles()), Codes{ UNKNOWN_MACRO });
    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array as array;\n", "    array.print!(\"x\");"), MAREA_withFiles()), Codes{ UNKNOWN_MACRO });
}

TEST(MAREASemaRegisteredMacros, TheMacroOfALibraryThatIsOffIsUnknown)
{
    GTEST_COUT << "Validating a registry built with the library off has no macro of it, so the name is unknown." << std::endl;

    MAREA_CallerInputs inputs{ MAREA_withFiles() };

    inputs.libraries = MAREA_LIBRARY_FLAG_MAREA_VECTOR;

    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    marea.array.include_bytes!(\"logo.bin\");"), inputs), Codes{ UNKNOWN_MACRO });
}

TEST(MAREASemaRegisteredMacros, ALibraryThatWasNotRequiredIsNotReachable)
{
    GTEST_COUT << "Validating a macro called by the full path of a library that no require covers raises UNIMPORTED_NAMESPACE." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_function("", "    marea.array.include_bytes!(\"logo.bin\");"), MAREA_withFiles()), Codes{ UNIMPORTED });
}

TEST(MAREASemaRegisteredMacros, AQualifiedNameThatNoNamespaceOwnsIsUnknown)
{
    GTEST_COUT << "Validating a qualified call whose qualifier is no namespace raises UNKNOWN_MACRO." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_function("", "    nothing.here!(\"x\");")), Codes{ UNKNOWN_MACRO });
}

TEST(MAREASemaRegisteredMacros, TheUnqualifiedNameOfARegisteredMacroIsNotALanguageMacro)
{
    GTEST_COUT << "Validating a macro that a library registers is reached only through its path: the bare name of an unregistered macro is UNKNOWN_MACRO." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_function("require marea.array;\n", "    read_bytes!(\"logo.bin\");"), MAREA_withFiles()), Codes{ UNKNOWN_MACRO });
}

TEST(MAREASemaRegisteredMacros, TheTypedTreeIsBuiltForAProgramWithAQualifiedMacro)
{
    GTEST_COUT << "Validating a program that calls a library macro through its qualified name still builds the typed tree." << std::endl;

    MAREA_CallerRun run(MAREA_function("require marea.array;\n", "    local data: marea.array.Array<uinteger8, 12> = marea.array.include_bytes!(\"logo.bin\");"), MAREA_withFiles());

    ASSERT_EQ(run.MAREA_codes(), Codes{}) << "The program must be accepted.";

    const MAREA_SemaTypedNode *root{ nullptr };

    EXPECT_EQ(MAREA_SemaBuildTypedAst(run.fixture.context, run.fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must be built.";
    EXPECT_NE(root, nullptr) << "The typed tree must have a root.";
}
