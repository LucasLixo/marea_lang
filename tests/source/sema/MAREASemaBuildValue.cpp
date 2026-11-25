// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaCallerHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 INVALID_ARGUMENT{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT };
    constexpr MAREA_Uinteger32 INVALID_VALUE{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BUILD_VALUE };
    constexpr MAREA_Uinteger32 TYPE_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };
    constexpr MAREA_Uinteger32 ASSERTION_FAILED{ MAREA_SEMA_DIAGNOSTIC_CODE_COMPILE_TIME_ASSERTION_FAILED };

    const std::string HEADER{ "namespace main;\n" };

    const std::string MODE{ "enum Mode\n    Fast;\n    Safe;\nend\n" };

    MAREA_CallerInputs MAREA_given(const std::string &name, const std::string &text)
    {
        MAREA_CallerInputs inputs;

        inputs.hasSettings = true;
        inputs.settings.emplace_back(name, text);

        return inputs;
    }

    Codes MAREA_codesOfConstant(const std::string &type, const std::string &call, const MAREA_CallerInputs &inputs = MAREA_CallerInputs{})
    {
        return MAREA_codesWith(HEADER + MODE + "constant VALUE: " + type + " = " + call + ";\n", inputs);
    }
} // namespace

TEST(MAREASemaBuildValue, EveryAcceptedTypeTakesItsDefaultWhenTheBuildGivesNothing)
{
    GTEST_COUT << "Validating build_value! with no settings accepts every primitive type and an enum, using the default." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("boolean", "build_value!(\"FAST\", boolean, false)"), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"LOG_LEVEL\", sinteger32, 2)"), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("uinteger8", "build_value!(\"LEVEL\", uinteger8, 7)"), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("float64", "build_value!(\"SCALE\", float64, 1.5)"), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("string", "build_value!(\"TITLE\", string, \"Marea\")"), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("character", "build_value!(\"MARK\", character, 'x')"), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("Mode", "build_value!(\"RENDER_MODE\", Mode, Mode.Fast)"), Codes{});
}

TEST(MAREASemaBuildValue, AGivenTextThatFitsTheTypeIsAccepted)
{
    GTEST_COUT << "Validating a text from the build that fits the type of the call raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("boolean", "build_value!(\"X\", boolean, false)", MAREA_given("X", "true")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "3")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "-5")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "0x1F")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("uinteger8", "build_value!(\"X\", uinteger8, 1)", MAREA_given("X", "255")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("float64", "build_value!(\"X\", float64, 1.5)", MAREA_given("X", "2.5")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("float64", "build_value!(\"X\", float64, 1.5)", MAREA_given("X", "1e3")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("string", "build_value!(\"X\", string, \"a\")", MAREA_given("X", "any text at all")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("character", "build_value!(\"X\", character, 'a')", MAREA_given("X", "z")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("character", "build_value!(\"X\", character, 'a')", MAREA_given("X", "'z'")), Codes{});
    EXPECT_EQ(MAREA_codesOfConstant("Mode", "build_value!(\"X\", Mode, Mode.Fast)", MAREA_given("X", "Safe")), Codes{});
}

TEST(MAREASemaBuildValue, ASettingForAnotherNameIsNotRead)
{
    GTEST_COUT << "Validating a setting whose name differs from the name of the call is ignored, so the default stands." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("OTHER", "not a number")), Codes{});
}

TEST(MAREASemaBuildValue, ADataCarryingVariantIsMatchedByNameWhenEveryFieldHasADefault)
{
    GTEST_COUT << "Validating an enum with a data-carrying variant is accepted when every field of every variant has a default (decision 53)." << std::endl;

    const std::string SHAPE{ "enum Step\n    Stay;\n    Move(x: sinteger32 = 0, y: sinteger32 = 0);\nend\n" };

    const std::string PROGRAM{ HEADER + SHAPE + "constant VALUE: Step = build_value!(\"STEP\", Step, Step.Stay);\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM), Codes{});
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("STEP", "Move")), Codes{});
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("STEP", "Stay")), Codes{});
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("STEP", "Jump")), Codes{ INVALID_VALUE });
}

TEST(MAREASemaBuildValue, AnEnumWithAFieldThatHasNoDefaultIsRejectedAtTheTypeArgument)
{
    GTEST_COUT << "Validating an enum with a variant field that has no default raises INVALID_MACRO_ARGUMENT once (decision 53)." << std::endl;

    const std::string SHAPE{ "enum Step\n    Stay;\n    Move(x: sinteger32);\nend\n" };

    const std::string PROGRAM{ HEADER + SHAPE + "constant VALUE: Step = build_value!(\"STEP\", Step, Step.Stay);\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("STEP", "Stay")), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaBuildValue, TheCallNeedsExactlyThreeArguments)
{
    GTEST_COUT << "Validating build_value! with fewer or more than three arguments raises INVALID_MACRO_ARGUMENT." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32)"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\")"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!()"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 1, 2)"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaBuildValue, TheNameMustBeAStringLiteral)
{
    GTEST_COUT << "Validating a first argument that is not a string literal raises INVALID_MACRO_ARGUMENT." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(7, sinteger32, 1)"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(X, sinteger32, 1)"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaBuildValue, TheTypeArgumentMustBeAPrimitiveOrAnEnum)
{
    GTEST_COUT << "Validating a type argument that is a literal, an unknown name or a structure raises INVALID_MACRO_ARGUMENT." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", 3, 1)"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", Missing, 1)"), Codes{ INVALID_ARGUMENT });

    const std::string PROGRAM{ HEADER + "structure Point\n    x: sinteger32;\nend\nconstant VALUE: sinteger32 = build_value!(\"X\", Point, 1);\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaBuildValue, TheDefaultMustFitTheType)
{
    GTEST_COUT << "Validating a default of another type raises TYPE_MISMATCH at the argument." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, \"text\")"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfConstant("boolean", "build_value!(\"X\", boolean, 1)"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfConstant("string", "build_value!(\"X\", string, 1)"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfConstant("Mode", "build_value!(\"X\", Mode, 1)"), Codes{ TYPE_MISMATCH });
}

TEST(MAREASemaBuildValue, TheValueTypeMustFitTheTarget)
{
    GTEST_COUT << "Validating the value has the type of the second argument, so another declared type raises TYPE_MISMATCH." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("string", "build_value!(\"X\", sinteger32, 1)"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 1)"), Codes{});

    const std::string LOCAL{ HEADER + "function check()\n    local name: string = build_value!(\"X\", sinteger32, 1);\nend\n" };

    EXPECT_EQ(MAREA_codesWith(LOCAL), Codes{ TYPE_MISMATCH });
}

TEST(MAREASemaBuildValue, AGivenTextThatDoesNotFitTheTypeIsInvalidBuildValue)
{
    GTEST_COUT << "Validating a text from the build that is not a value of the type raises INVALID_BUILD_VALUE and is never replaced by the default." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "high")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "2.5")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_value!(\"X\", sinteger32, 2)", MAREA_given("X", "2147483648")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("uinteger8", "build_value!(\"X\", uinteger8, 2)", MAREA_given("X", "256")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("uinteger8", "build_value!(\"X\", uinteger8, 2)", MAREA_given("X", "-1")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("boolean", "build_value!(\"X\", boolean, false)", MAREA_given("X", "yes")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("boolean", "build_value!(\"X\", boolean, false)", MAREA_given("X", "1")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("float64", "build_value!(\"X\", float64, 1.5)", MAREA_given("X", "abc")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("character", "build_value!(\"X\", character, 'a')", MAREA_given("X", "ab")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("character", "build_value!(\"X\", character, 'a')", MAREA_given("X", "")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("Mode", "build_value!(\"X\", Mode, Mode.Fast)", MAREA_given("X", "Slow")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesOfConstant("Mode", "build_value!(\"X\", Mode, Mode.Fast)", MAREA_given("X", "0")), Codes{ INVALID_VALUE });
}

TEST(MAREASemaBuildValue, TheMessageNamesTheSettingTheTextAndTheType)
{
    GTEST_COUT << "Validating the INVALID_BUILD_VALUE message holds the name, the text given and the expected type." << std::endl;

    MAREA_CallerRun run(HEADER + "constant VALUE: sinteger32 = build_value!(\"LOG_LEVEL\", sinteger32, 2);\n", MAREA_given("LOG_LEVEL", "high"));

    const std::vector<std::string> MESSAGES{ run.MAREA_messages() };

    ASSERT_EQ(MESSAGES.size(), 1U) << "One diagnostic is expected.";
    EXPECT_NE(MESSAGES[0].find("LOG_LEVEL"), std::string::npos) << "The message must name the setting.";
    EXPECT_NE(MESSAGES[0].find("high"), std::string::npos) << "The message must hold the text given.";
    EXPECT_NE(MESSAGES[0].find("sinteger32"), std::string::npos) << "The message must name the type.";
}

TEST(MAREASemaBuildValue, TheValueFromTheBuildFeedsACompileTimeAssertion)
{
    GTEST_COUT << "Validating a static_assert! over a build_value! constant reads the value the build gave, or the default." << std::endl;

    const std::string PROGRAM{ HEADER + "constant LOG_LEVEL: sinteger32 = build_value!(\"LOG_LEVEL\", sinteger32, 2);\nstatic_assert!(LOG_LEVEL >= 0 and LOG_LEVEL <= 4, \"LOG_LEVEL is 0 to 4\");\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM), Codes{}) << "The default 2 holds.";
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("LOG_LEVEL", "3")), Codes{}) << "A given 3 holds.";
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("LOG_LEVEL", "9")), Codes{ ASSERTION_FAILED }) << "A given 9 breaks the assertion.";
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_given("LOG_LEVEL", "0x4")), Codes{}) << "A hexadecimal 4 holds.";

    const std::string FLAG{ HEADER + "constant FAST: boolean = build_value!(\"FAST\", boolean, false);\nstatic_assert!(FAST, \"FAST must be on\");\n" };

    EXPECT_EQ(MAREA_codesWith(FLAG), Codes{ ASSERTION_FAILED }) << "The default false breaks the assertion.";
    EXPECT_EQ(MAREA_codesWith(FLAG, MAREA_given("FAST", "true")), Codes{}) << "A given true holds.";
}

TEST(MAREASemaBuildValue, BuildValueIsAReservedMacroName)
{
    GTEST_COUT << "Validating build_value is a reserved macro name, so a user declaration of it raises RESERVED_MACRO_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesWith(HEADER + "function build_value()\nend\n"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_MACRO_NAME });
}

TEST(MAREASemaBuildValue, AnUnqualifiedMacroOfALibraryIsStillUnknown)
{
    GTEST_COUT << "Validating a macro name that no language macro has raises UNKNOWN_MACRO, build_value included when the name is misspelled." << std::endl;

    EXPECT_EQ(MAREA_codesOfConstant("sinteger32", "build_values!(\"X\", sinteger32, 1)"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO });
}

TEST(MAREASemaBuildValue, TheTypedTreeIsBuiltForAProgramWithBuildValueConstants)
{
    GTEST_COUT << "Validating a program with build_value! constants still builds the typed tree." << std::endl;

    MAREA_CallerRun run(HEADER + MODE + "constant LOG_LEVEL: sinteger32 = build_value!(\"LOG_LEVEL\", sinteger32, 2);\nconstant RENDER_MODE: Mode = build_value!(\"RENDER_MODE\", Mode, Mode.Fast);\n", MAREA_given("LOG_LEVEL", "3"));

    ASSERT_EQ(run.MAREA_codes(), Codes{}) << "The program must be accepted.";

    const MAREA_SemaTypedNode *root{ nullptr };

    EXPECT_EQ(MAREA_SemaBuildTypedAst(run.fixture.context, run.fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must be built.";
    EXPECT_NE(root, nullptr) << "The typed tree must have a root.";
}
