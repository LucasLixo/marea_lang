// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 FAILED{ MAREA_SEMA_DIAGNOSTIC_CODE_COMPILE_TIME_ASSERTION_FAILED };

    Codes MAREA_codesOfProgram(const std::string &program)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(program) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }

    std::string MAREA_assertOf(const std::string &condition)
    {
        return "namespace main; static_assert!(" + condition + ", \"text\");";
    }

    std::string MAREA_lastMessage(const std::string &program)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(program) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return "<sema failed>";
        }

        const MAREA_Diagnostic *diagnostic{ nullptr };

        if (MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, (MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink) - 1), &diagnostic) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
            return "<no diagnostic>";
        }

        return MAREA_DiagnosticGetMessage(diagnostic);
    }
} // namespace

TEST(MAREASemaStaticAssert, CompileErrorAtTheTopLevelRaisesTheCode)
{
    GTEST_COUT << "Validating that compile_error at the top level raises COMPILE_TIME_ASSERTION_FAILED." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; compile_error!(\"stop\");"), Codes{ FAILED });
}

TEST(MAREASemaStaticAssert, CompileErrorInAFunctionBodyRaisesTheCode)
{
    GTEST_COUT << "Validating that compile_error in a function body raises COMPILE_TIME_ASSERTION_FAILED." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; function run(): sinteger32\n    compile_error!(\"stop\");\nend"), Codes{ FAILED });
}

TEST(MAREASemaStaticAssert, CompileErrorWithAnyArgumentsStillRaisesTheCode)
{
    GTEST_COUT << "Validating that compile_error raises the code whatever its arguments, and that a wrong shape adds the macro shape code." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; compile_error!(\"\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; compile_error!();"), (Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT, FAILED }));
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; compile_error!(1, 2);"), (Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT, FAILED }));
}

TEST(MAREASemaStaticAssert, CompileErrorIsReportedAtTheCall)
{
    GTEST_COUT << "Validating that the diagnostic of compile_error sits at the offset of the call." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    const std::string PREFIX{ "namespace main;\n" };

    ASSERT_EQ(fixture.MAREA_sema(PREFIX + "compile_error!(\"stop\");"), MAREA_SEMA_EXCEPT_SUCCESS);

    ASSERT_EQ(fixture.MAREA_semaCodes(), Codes{ FAILED });

    EXPECT_EQ(fixture.parser.MAREA_offsets().back(), PREFIX.size());
}

TEST(MAREASemaStaticAssert, FalseConstantConditionRaisesTheCode)
{
    GTEST_COUT << "Validating that a static_assert over a false constant raises COMPILE_TIME_ASSERTION_FAILED." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1 == 2")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("false")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("not true")), Codes{ FAILED });
}

TEST(MAREASemaStaticAssert, FalseConditionInAFunctionBodyRaisesTheCode)
{
    GTEST_COUT << "Validating that a false static_assert in a function body raises the code." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; function run(): sinteger32\n    static_assert!(1 > 2, \"text\");\n    return 0;\nend"), Codes{ FAILED });
}

TEST(MAREASemaStaticAssert, TrueConstantConditionRaisesNothing)
{
    GTEST_COUT << "Validating that a static_assert over a true constant raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("2 + 2 == 4")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("true")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("(1 < 2) and (3 >= 3)")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1 > 2 or 7 % 4 == 3")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("-3 + 5 == 2 * 1")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("10 / 3 != 4")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("0x10 == 16 and 0b101 == 5 and 1_000 == 1000")), Codes{});
}

TEST(MAREASemaStaticAssert, MultiplicationFoldsAtTheEdgeAndGivesUpOnOverflow)
{
    GTEST_COUT << "Validating that a product that fits 64 bits is folded and one that does not is unknown." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("-4611686018427387904 * 2 < 0")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("-4611686018427387904 * 2 > 0")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("3037000499 * 3037000499 > 0")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("3037000499 * 3037000499 < 0")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("3037000500 * 3037000500 < 0")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("4611686018427387904 * 2 < 0")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("-3037000500 * 3037000500 > 0")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("-3037000500 * -3037000500 < 0")), Codes{});
}

TEST(MAREASemaStaticAssert, FileConstantIsRead)
{
    GTEST_COUT << "Validating that a file constant is evaluated by the condition." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant N: sinteger32 = 4; static_assert!(N == 5, \"n\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant N: sinteger32 = 4; static_assert!(N == 4, \"n\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant A: sinteger32 = B + 1; constant B: sinteger32 = 2; static_assert!(A == 4, \"n\");"), Codes{ FAILED });
}

TEST(MAREASemaStaticAssert, ConditionThatCannotBeDecidedRaisesNothing)
{
    GTEST_COUT << "Validating that an undecided condition is accepted." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1 / 0 == 1")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1 % 0 == 1")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("9223372036854775807 + 1 == 0")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("unknown_name == 1")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("size_of<string>() == 9")), Codes{});
}

TEST(MAREASemaStaticAssert, ParameterShadowingAConstantIsUnknown)
{
    GTEST_COUT << "Validating that a parameter that shares the name of a file constant is not read as the constant." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant N: sinteger32 = 4; function run(N: sinteger32): sinteger32\n    static_assert!(N == 5, \"n\");\n    return 0;\nend"), Codes{});
}

TEST(MAREASemaStaticAssert, CyclicConstantsAreUnknown)
{
    GTEST_COUT << "Validating that constants that name each other end as unknown here." << std::endl;

    const Codes CODES{ MAREA_codesOfProgram("namespace main; constant A: sinteger32 = B; constant B: sinteger32 = A; static_assert!(A == 1, \"n\");") };

    EXPECT_EQ(std::count(CODES.begin(), CODES.end(), FAILED), 0);
}

TEST(MAREASemaStaticAssert, WrongArgumentCountIsLeftToTheMacroShapeRule)
{
    GTEST_COUT << "Validating that a static_assert with a wrong argument count is left to the macro shape rule." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; static_assert!(1 == 2);"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; static_assert!(1 == 2, \"a\", \"b\");"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT });
}

TEST(MAREASemaStaticAssert, OtherMacrosAreNotEvaluated)
{
    GTEST_COUT << "Validating that unreachable does not raise the code." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main; function run(): sinteger32\n    unreachable!();\nend"), Codes{});
}

TEST(MAREASemaStaticAssert, MessageCarriesTheTextOfTheFirstStringArgument)
{
    GTEST_COUT << "Validating the message of both diagnostics." << std::endl;

    EXPECT_EQ(MAREA_lastMessage("namespace main; compile_error!(\"stop now\");"), "compile_error: stop now");
    EXPECT_EQ(MAREA_lastMessage("namespace main; static_assert!(1 == 2, \"never\");"), "static assertion failed: never");
    EXPECT_EQ(MAREA_lastMessage("namespace main; compile_error!();"), "compile_error");
}

TEST(MAREASemaStaticAssert, SizeOfAndAlignOfAreReadFromTheLayout)
{
    GTEST_COUT << "Validating that size_of and align_of fold from the layout record of a type of the file and of a primitive." << std::endl;

    const std::string HEAD{ "namespace main; structure Pair first: sinteger32; second: sinteger32; end " };

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "static_assert!(size_of<Pair>() == 9, \"x\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "static_assert!(align_of<Pair>() == 8, \"x\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "static_assert!(size_of<sinteger64>() == 4, \"x\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "static_assert!(size_of<Pair>() == 8, \"x\"); static_assert!(align_of<Pair>() == 4, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; import other; static_assert!(size_of<other.Light>() == 9, \"x\");"), Codes{}) << "A type with no known layout stays unknown.";
}

TEST(MAREASemaStaticAssert, FloatsAndStringsFold)
{
    GTEST_COUT << "Validating that a float comparison and a string equality fold." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1.5 > 2.0")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1.5 + 1.0 == 3.0")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram("namespace main; constant NAME: string = \"a\"; static_assert!(NAME == \"b\", \"x\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("\"a\" != \"a\"")), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("1.5 + 1.0 == 2.5")), Codes{});
    EXPECT_EQ(MAREA_codesOfProgram(MAREA_assertOf("\"a\" == \"a\"")), Codes{});
}

TEST(MAREASemaStaticAssert, ANarrowConstantWrapsInItsOwnWidth)
{
    GTEST_COUT << "Validating that arithmetic on a constant of a narrow type wraps in that type." << std::endl;

    const std::string HEAD{ "namespace main; constant BIG: uinteger8 = 250; constant LOW: sinteger8 = 127; " };

    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "static_assert!(BIG + 10 == 260, \"x\");"), Codes{ FAILED });
    EXPECT_EQ(MAREA_codesOfProgram(HEAD + "static_assert!(BIG + 10 == 4, \"x\"); static_assert!(LOW + 1 == -128, \"x\"); static_assert!(BIG * BIG == 36, \"x\");"), Codes{});
}
