// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISSING_RETURN{ MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_RETURN };
    constexpr MAREA_Uinteger32 NON_EXHAUSTIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_NON_EXHAUSTIVE_SWITCH };

    const std::string COLOR{ "enum Color red; green; blue; end " };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaControlFlow, EveryIfArmReturns)
{
    GTEST_COUT << "Validating an if with every arm returning is a complete function." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: boolean, b: boolean): integer32 if a then return 1; elseif b then return 2; else return 3; end end").empty()) << "Every arm returning must raise nothing.";
}

TEST(MAREASemaControlFlow, MissingReturnInOneArm)
{
    GTEST_COUT << "Validating an if with an arm that does not return is missing a return." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(a: boolean): integer32 if a then return 1; else local x: integer32 = 2; end end"), Codes{ MISSING_RETURN }) << "A silent else arm must raise MISSING_RETURN.";

    EXPECT_EQ(MAREA_codesOf("function f(a: boolean): integer32 if a then return 1; end end"), Codes{ MISSING_RETURN }) << "An if with no else must raise MISSING_RETURN.";

    EXPECT_EQ(MAREA_codesOf("function f(a: boolean, b: boolean): integer32 if a then return 1; elseif b then local x: integer32 = 2; else return 3; end end"), Codes{ MISSING_RETURN }) << "A silent elseif arm must raise MISSING_RETURN.";
}

TEST(MAREASemaControlFlow, ReturnAfterIfCompletes)
{
    GTEST_COUT << "Validating a return after an if completes the function." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: boolean): integer32 if a then return 1; end return 2; end").empty()) << "A trailing return must raise nothing.";
}

TEST(MAREASemaControlFlow, FunctionWithoutReturnTypeNeedsNoReturn)
{
    GTEST_COUT << "Validating a function with no return type needs no return." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: boolean) if a then return; end end").empty()) << "A function without a return type must raise nothing.";
}

TEST(MAREASemaControlFlow, EmptyBodyIsMissingAReturn)
{
    GTEST_COUT << "Validating an empty body of a function with a return type is missing a return." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(): integer32 end"), Codes{ MISSING_RETURN }) << "An empty body must raise MISSING_RETURN.";
}

TEST(MAREASemaControlFlow, NestedBlocksCanReturn)
{
    GTEST_COUT << "Validating a return inside a scope block completes the function." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(): integer32 do return 1; end end").empty()) << "A scope block that returns must raise nothing.";
}

TEST(MAREASemaControlFlow, LoopBodyIsNeverATerminatingPath)
{
    GTEST_COUT << "Validating a loop whose body returns is still missing a return after it." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(): integer32 while true do return 1; end end"), Codes{ MISSING_RETURN }) << "A while must raise MISSING_RETURN.";

    EXPECT_EQ(MAREA_codesOf("function f(): integer32 for i in 1 .. 3 do return 1; end end"), Codes{ MISSING_RETURN }) << "A for must raise MISSING_RETURN.";

    EXPECT_TRUE(MAREA_codesOf("function f(): integer32 while true do local x: integer32 = 1; end return 1; end").empty()) << "A return after the loop must raise nothing.";
}

TEST(MAREASemaControlFlow, SwitchOverEnumCoveringEveryVariant)
{
    GTEST_COUT << "Validating a switch over an enum that covers every variant is exhaustive." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.green, Color.blue then return 2; end end").empty()) << "A full switch with returning arms must raise nothing.";
}

TEST(MAREASemaControlFlow, SwitchOverEnumMissingAVariant)
{
    GTEST_COUT << "Validating a switch over an enum that misses a variant is not exhaustive." << std::endl;

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.green then return 2; end end"), Codes{ NON_EXHAUSTIVE }) << "A missing variant must raise NON_EXHAUSTIVE_SWITCH.";
}

TEST(MAREASemaControlFlow, DefaultArmCoversTheMissingVariant)
{
    GTEST_COUT << "Validating a default arm makes a switch over an enum exhaustive." << std::endl;

    EXPECT_TRUE(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; default then return 2; end end").empty()) << "A default arm must raise nothing.";
}

TEST(MAREASemaControlFlow, SwitchOverANonEnumIsExempt)
{
    GTEST_COUT << "Validating a switch over a non-enum with only a default arm is accepted." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n default then return 1; end end").empty()) << "A default-only switch must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32) switch n case 1 then return; end end").empty()) << "A switch over an integer with no default must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then return 1; end end"), Codes{ MISSING_RETURN }) << "A switch over an integer with no default cannot complete a function.";
}

TEST(MAREASemaControlFlow, SwitchArmThatDoesNotReturnIsMissingAReturn)
{
    GTEST_COUT << "Validating a default switch with one silent arm is missing a return." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then local x: integer32 = 1; default then return 2; end end"), Codes{ MISSING_RETURN }) << "A silent arm must raise MISSING_RETURN.";
}

TEST(MAREASemaControlFlow, UnknownScrutineeTypeIsExempt)
{
    GTEST_COUT << "Validating a switch over a value of unknown type is not checked for exhaustiveness." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(c: Unknown) switch c case Other.red then return; end end").empty()) << "An unknown scrutinee must raise nothing from exhaustiveness.";
}

TEST(MAREASemaControlFlow, ErrorNodeInBodyIsSkipped)
{
    GTEST_COUT << "Validating a body with a broken statement raises no missing-return diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(): integer32 ; ) end").empty()) << "A broken body must raise no sema diagnostic.";
}

TEST(MAREASemaControlFlow, DivergingMacroEndsAPath)
{
    GTEST_COUT << "Validating unreachable!() and compile_error!() end a path and other macros do not." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: boolean): integer32 if a then return 1; end unreachable!(); end").empty()) << "unreachable must end a path.";

    EXPECT_TRUE(MAREA_codesOf("function f(): integer32 compile_error!(\"no\"); end").empty()) << "compile_error must end a path.";

    EXPECT_EQ(MAREA_codesOf("function f(): integer32 print!(\"x\"); end"), Codes{ MISSING_RETURN }) << "A printing macro must not end a path.";
}
