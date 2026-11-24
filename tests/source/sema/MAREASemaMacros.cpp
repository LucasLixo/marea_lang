// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 UNKNOWN_MACRO{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO };
    constexpr MAREA_Uinteger32 INVALID_ARGUMENT{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT };
    constexpr MAREA_Uinteger32 TYPE_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };

    Codes MAREA_codesOfProgram(const std::string &program)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema(program) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }

    // INFO: the statement runs in a function with a boolean parameter ok and an integer parameter count.
    Codes MAREA_codesOfStatement(const std::string &statement)
    {
        return MAREA_codesOfProgram("namespace main;\nfunction check(ok: boolean, count: sinteger32)\n" + statement + "\nend\n");
    }

    std::vector<std::string> MAREA_messagesOfStatement(const std::string &statement)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        std::vector<std::string> messages;

        if (fixture.MAREA_sema("namespace main;\nfunction check(ok: boolean, count: sinteger32)\n" + statement + "\nend\n") != MAREA_SEMA_EXCEPT_SUCCESS) {
            return messages;
        }

        for (MAREA_Uinteger32 index = 0; index < static_cast<MAREA_Uinteger32>(MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink)); ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            if ((MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, index, &diagnostic) == MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) && (MAREA_DiagnosticGetCode(diagnostic) >= 3000)) {
                messages.emplace_back(MAREA_DiagnosticGetMessage(diagnostic));
            }
        }

        return messages;
    }
} // namespace

TEST(MAREASemaMacros, UnknownMacroIsReportedAtTheCall)
{
    GTEST_COUT << "Validating that a macro name the compiler does not define raises UNKNOWN_MACRO." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("shout!(\"hello\");"), Codes{ UNKNOWN_MACRO });
}

TEST(MAREASemaMacros, UnknownMacroMessageNamesTheMacro)
{
    GTEST_COUT << "Validating that the UNKNOWN_MACRO message names the macro." << std::endl;

    const std::vector<std::string> MESSAGES{ MAREA_messagesOfStatement("shout!(\"hello\");") };

    ASSERT_EQ(MESSAGES.size(), 1U);
    EXPECT_NE(MESSAGES[0].find("'shout'"), std::string::npos);
}

TEST(MAREASemaMacros, UnknownMacroInAnExpressionIsReported)
{
    GTEST_COUT << "Validating that an unknown macro used as a value raises UNKNOWN_MACRO once." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("local text: string = shout!(\"hello\");"), Codes{ UNKNOWN_MACRO });
}

TEST(MAREASemaMacros, PrintNeedsAFormatText)
{
    GTEST_COUT << "Validating that print! with no argument or a number as the format text raises INVALID_MACRO_ARGUMENT." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("print!();"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("print!(1, 2);"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaMacros, PrintCountsTheBracesOfTheFormatText)
{
    GTEST_COUT << "Validating that the {} count of a literal format text must equal the value count." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("print!(\"{} {}\", 1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("print!(\"{}\", 1, 2);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("print!(\"{}\", count);"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("print!(\"x\");"), Codes{});
}

TEST(MAREASemaMacros, LoneBraceIsNotAPlaceholder)
{
    GTEST_COUT << "Validating that a lone { or } in the format text is skipped by the count." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("print!(\"a { b } c\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("print!(\"{} }\", 1);"), Codes{});
}

TEST(MAREASemaMacros, AssertTakesAConditionAndAMessage)
{
    GTEST_COUT << "Validating that assert! and debug_assert! take exactly a condition and a string literal." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("assert!(1, \"x\");"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("assert!(ok, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("assert!(ok);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("assert!(ok, \"x\", 1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("assert!(ok, 1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("debug_assert!(\"s\", \"x\");"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("debug_assert!(count > 0, \"x\");"), Codes{});
}

TEST(MAREASemaMacros, UnreachableTakesNoArgument)
{
    GTEST_COUT << "Validating that unreachable! takes no argument." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("unreachable!(1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("unreachable!();"), Codes{});
}

TEST(MAREASemaMacros, LogNeedsAKnownLevelAndAFormatText)
{
    GTEST_COUT << "Validating that log! takes a level name, a string literal and the values." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("log!(loud, \"x\");"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("log!(info, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("log!(error, \"{}\", 1);"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("log!(trace, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("log!(debug, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("log!(warning, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("log!(info);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("log!(info, 1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("log!(1, \"x\");"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("log!(info, \"{} {}\", 1);"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaMacros, IncludeMacrosTakeOneStringLiteral)
{
    GTEST_COUT << "Validating that include_text! and include_bytes! take exactly one string literal." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("local text: string = include_text!(1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = include_text!(\"a.txt\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = include_text!();"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = include_text!(\"a\", \"b\");"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local bytes: c.array<uinteger8> = include_bytes!(1.5);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local bytes: c.array<uinteger8> = include_bytes!(\"a.bin\");"), Codes{});
}

TEST(MAREASemaMacros, CompileErrorAndStaticAssertGetOnlyTheShapeCheck)
{
    GTEST_COUT << "Validating that compile_error! and static_assert! are checked for their argument shape only." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main;\nstatic_assert!(1, \"x\");\n"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfProgram("namespace main;\nstatic_assert!(true, \"x\");\n"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("compile_error!(1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("compile_error!();"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaMacros, FormatNeedsAFormatText)
{
    GTEST_COUT << "Validating that format! takes one or more arguments, the first a string literal." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("local text: string = format!();"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = format!(1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = format!(\"{}\", count);"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = format!(\"{} {}\", count);"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaMacros, PlaceMacrosTakeNoArgument)
{
    GTEST_COUT << "Validating that line!, file! and namespace! take no argument." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("local text: string = namespace!();"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = file!();"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local number: sinteger32 = line!();"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = namespace!(1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local number: sinteger32 = line!(1);"), Codes{ INVALID_ARGUMENT });
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = file!(\"x\");"), Codes{ INVALID_ARGUMENT });
}

TEST(MAREASemaMacros, ACallOfAMacroThatSemaCannotTypeIsAccepted)
{
    GTEST_COUT << "Validating that a non-literal argument of the wrong kind stays unknown and is accepted." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("assert!(count, \"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("print!(count);"), Codes{});
}

TEST(MAREASemaMacros, MacroValueMustFitItsTarget)
{
    GTEST_COUT << "Validating the value rules of the macros: none, string, or unknown." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("local number: sinteger32 = print!(\"x\");"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfStatement("local number: sinteger32 = format!(\"x\");"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = format!(\"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = include_text!(\"a.txt\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = namespace!();"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local text: string = file!();"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("local number: sinteger32 = line!();"), Codes{});
}

TEST(MAREASemaMacros, MacroResultReturnedMustFitTheReturnType)
{
    GTEST_COUT << "Validating that a returned macro value is checked against the result type." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main;\nfunction check(): sinteger32\n return format!(\"x\");\nend\n"), Codes{ TYPE_MISMATCH });
    EXPECT_EQ(MAREA_codesOfProgram("namespace main;\nfunction check(): string\n return format!(\"x\");\nend\n"), Codes{});
}

TEST(MAREASemaMacros, MacroWithNoValuePassedAsAnArgumentIsReported)
{
    GTEST_COUT << "Validating that print!(...) passed as a call argument raises TYPE_MISMATCH." << std::endl;

    EXPECT_EQ(MAREA_codesOfProgram("namespace main;\nfunction take(value: sinteger32)\nend\nfunction check()\n take(print!(\"x\"));\nend\n"), Codes{ TYPE_MISMATCH });
}

TEST(MAREASemaMacros, MacroStatementIsNotAValueUse)
{
    GTEST_COUT << "Validating that a macro statement with no value raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOfStatement("print!(\"x\");"), Codes{});
    EXPECT_EQ(MAREA_codesOfStatement("log!(info, \"x\");"), Codes{});
}
