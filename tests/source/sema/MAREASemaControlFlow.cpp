// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISSING_RETURN{ MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_RETURN };
    constexpr MAREA_Uinteger32 NON_EXHAUSTIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_NON_EXHAUSTIVE_SWITCH };
    constexpr MAREA_Uinteger32 DUPLICATE_ARM{ MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_SWITCH_ARM };
    constexpr MAREA_Uinteger32 JUMP_OUTSIDE_LOOP{ MAREA_SEMA_DIAGNOSTIC_CODE_JUMP_OUTSIDE_LOOP };

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

TEST(MAREASemaControlFlow, WithBodyCanReturn)
{
    GTEST_COUT << "Validating a with statement whose body returns on every path completes the function." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: integer32): integer32 with a as view do return 1; end end").empty()) << "A with body that always returns must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(a: integer32): integer32 with a as view do local x: integer32 = view; end end"), Codes{ MISSING_RETURN }) << "A with body that does not return must still raise MISSING_RETURN.";
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

TEST(MAREASemaControlFlow, RepeatedDefaultArmIsRejected)
{
    GTEST_COUT << "Validating a second default arm raises DUPLICATE_SWITCH_ARM once for each extra default." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then return 1; default then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "Two default arms must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n default then return 1; default then return 2; default then return 3; end end"), (Codes{ DUPLICATE_ARM, DUPLICATE_ARM })) << "Three default arms must raise two diagnostics.";

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c default then return 1; default then return 2; end end"), Codes{ DUPLICATE_ARM }) << "A switch over an enum must also reject two default arms.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then return 1; default then return 2; end end").empty()) << "One default arm must raise nothing.";
}

TEST(MAREASemaControlFlow, DefaultArmBeforeACaseStaysAccepted)
{
    GTEST_COUT << "Validating a default arm that is not last is not changed by the duplicate check." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n default then return 2; case 1 then return 1; end end").empty()) << "The place of the default arm is a parser rule and must raise nothing here.";
}

TEST(MAREASemaControlFlow, RepeatedCaseValueIsRejected)
{
    GTEST_COUT << "Validating a case value that appears again raises DUPLICATE_SWITCH_ARM." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then return 1; case 1 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "The same integer in two arms must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1, 2, 1 then return 1; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "The same integer twice in one list must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1, 1 then return 1; case 1 then return 2; default then return 3; end end"), (Codes{ DUPLICATE_ARM, DUPLICATE_ARM })) << "Each repeat must raise one diagnostic, not the first use.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case -1 then return 1; case -1 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A negative literal written twice must raise DUPLICATE_SWITCH_ARM.";
}

TEST(MAREASemaControlFlow, RepeatedStringAndBooleanCaseValuesAreRejected)
{
    GTEST_COUT << "Validating a repeated string, boolean or float value is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(s: string): integer32 switch s case \"a\" then return 1; case \"a\" then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A repeated string must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(b: boolean): integer32 switch b case true then return 1; case true then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A repeated boolean must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(x: float32): integer32 switch x case 1.5 then return 1; case 1.5 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A repeated float must raise DUPLICATE_SWITCH_ARM.";
}

TEST(MAREASemaControlFlow, DistinctCaseValuesAreAccepted)
{
    GTEST_COUT << "Validating distinct case values raise nothing, also when the kinds differ in text only." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1, 2 then return 1; case 3 then return 2; default then return 4; end end").empty()) << "Distinct integers must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then return 1; case -1 then return 2; default then return 3; end end").empty()) << "A value and its negation are distinct.";

    EXPECT_TRUE(MAREA_codesOf("function f(s: string): integer32 switch s case \"1\" then return 1; case \"2\" then return 2; default then return 3; end end").empty()) << "Distinct strings must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n case a.LIMIT then return 1; case b.LIMIT then return 2; default then return 3; end end").empty()) << "Two constants of different namespaces with one last name are distinct.";
}

TEST(MAREASemaControlFlow, RepeatedConstantNameIsRejected)
{
    GTEST_COUT << "Validating a constant name written twice in a switch over a non-enum is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case LIMIT then return 1; case LIMIT then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A repeated bare constant name must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case a.LIMIT then return 1; case a.LIMIT then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A repeated qualified constant name must raise DUPLICATE_SWITCH_ARM.";
}

TEST(MAREASemaControlFlow, RepeatedIntegerValueInAnotherBaseIsRejected)
{
    GTEST_COUT << "Validating two integer literals of one value in different spellings are the same case value." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1, 0x1 then return 1; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "1 and 0x1 in one list must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 255 then return 1; case 0xFF then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "255 and 0xFF across arms must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 5 then return 1; case 0b101 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "5 and 0b101 must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1_000 then return 1; case 1000 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A digit separator must not change the value.";

    EXPECT_EQ(MAREA_codesOf("function f(n: integer32): integer32 switch n case -1 then return 1; case -0x1 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A negated literal must be compared by value.";

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32): integer32 switch n case 1 then return 1; case 0x2 then return 2; case -0x1 then return 4; default then return 3; end end").empty()) << "Literals of different values must raise nothing.";
}

TEST(MAREASemaControlFlow, ConstantAndItsLiteralAreTheSameCaseValue)
{
    GTEST_COUT << "Validating a named constant and the literal it holds are the same case value." << std::endl;

    const std::string MAX{ "constant MAX: integer32 = 100; " };

    EXPECT_EQ(MAREA_codesOf(MAX + "function f(n: integer32): integer32 switch n case MAX then return 1; case 100 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A constant, then its literal, must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf(MAX + "function f(n: integer32): integer32 switch n case 0x64 then return 1; case MAX then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A literal in another base, then the constant, must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("constant A: integer32 = 7; constant B: integer32 = A; function f(n: integer32): integer32 switch n case B then return 1; case 7 then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A constant built from another constant must be followed.";

    EXPECT_EQ(MAREA_codesOf("constant A: integer32 = 7; constant B: integer32 = 7; function f(n: integer32): integer32 switch n case A then return 1; case B then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "Two constants of one value must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf("constant NAME: string = \"a\"; function f(s: string): integer32 switch s case NAME then return 1; case \"a\" then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A string constant and its literal must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_TRUE(MAREA_codesOf(MAX + "function f(n: integer32): integer32 switch n case MAX then return 1; case 101 then return 2; default then return 3; end end").empty()) << "A constant and another literal must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf(MAX + "function f(MAX: integer32): integer32 switch MAX case MAX then return 1; case 100 then return 2; default then return 3; end end").empty()) << "A local that hides the constant must not be resolved to it.";

    EXPECT_TRUE(MAREA_codesOf("constant A: integer32 = B; constant B: integer32 = A; function f(n: integer32): integer32 switch n case A then return 1; case 1 then return 2; default then return 3; end end").empty()) << "A cycle of constants must end without a false repeat.";
}

TEST(MAREASemaControlFlow, QualifiedNamesAreNotResolvedToAConstantOfTheLastName)
{
    GTEST_COUT << "Validating a qualified name is compared by its path, never by a constant that shares its last name." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("constant LIMIT: integer32 = 5; function f(n: integer32): integer32 switch n case a.LIMIT then return 1; case b.LIMIT then return 2; default then return 3; end end").empty()) << "a.LIMIT and b.LIMIT must stay distinct when the file has its own LIMIT.";

    EXPECT_TRUE(MAREA_codesOf("constant LIMIT: integer32 = 5; constant MAX: integer32 = 5; function f(n: integer32): integer32 switch n case a.LIMIT then return 1; case a.MAX then return 2; default then return 3; end end").empty()) << "a.LIMIT and a.MAX must stay distinct even when the file has constants LIMIT and MAX of one value.";

    EXPECT_EQ(MAREA_codesOf("constant LIMIT: integer32 = 5; function f(n: integer32): integer32 switch n case a.LIMIT then return 1; case a.LIMIT then return 2; default then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A repeated qualified name must still raise DUPLICATE_SWITCH_ARM.";
}

TEST(MAREASemaControlFlow, RepeatedEnumVariantIsRejected)
{
    GTEST_COUT << "Validating an enum variant that appears again raises DUPLICATE_SWITCH_ARM." << std::endl;

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.red then return 2; case Color.green, Color.blue then return 3; end end"), Codes{ DUPLICATE_ARM }) << "The same variant in two arms must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red, Color.red then return 1; case Color.green, Color.blue then return 3; end end"), Codes{ DUPLICATE_ARM }) << "The same variant twice in one list must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case red then return 1; case Color.red then return 2; case green, blue then return 3; end end"), Codes{ DUPLICATE_ARM }) << "A bare and a qualified name of one variant must raise DUPLICATE_SWITCH_ARM.";

    EXPECT_TRUE(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.green then return 2; case Color.blue then return 3; end end").empty()) << "Each variant once must raise nothing.";
}

TEST(MAREASemaControlFlow, RepeatedVariantAndMissingVariantRaiseBoth)
{
    GTEST_COUT << "Validating a repeat does not hide the exhaustiveness check." << std::endl;

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.red then return 2; case Color.green then return 3; end end"), (Codes{ DUPLICATE_ARM, NON_EXHAUSTIVE })) << "A repeat and a missing variant must both be reported.";

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.green then return 2; end end"), Codes{ NON_EXHAUSTIVE }) << "A missing variant alone must still raise NON_EXHAUSTIVE_SWITCH only.";
}

TEST(MAREASemaControlFlow, NestedSwitchesKeepTheirOwnArms)
{
    GTEST_COUT << "Validating an inner switch does not share arms with the outer switch." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: integer32, b: integer32) switch a case 1 then switch b case 1 then return; default then return; end default then return; end end").empty()) << "One value and one default in each of two switches must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("function f(a: integer32, b: integer32) switch a case 1 then switch b case 2 then return; case 2 then return; end default then return; end end"), Codes{ DUPLICATE_ARM }) << "A repeat inside the inner switch must raise one diagnostic.";
}

TEST(MAREASemaControlFlow, BrokenCaseValuesAreNotCompared)
{
    GTEST_COUT << "Validating two broken case values are not taken for a repeat." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(n: integer32) switch n case ), ) then return; end end").empty()) << "Two error nodes must raise no sema diagnostic.";
}

TEST(MAREASemaControlFlow, BreakOrContinueWithNoEnclosingLoopIsRejected)
{
    GTEST_COUT << "Validating a break or continue with no enclosing loop raises JUMP_OUTSIDE_LOOP." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() break; end"), Codes{ JUMP_OUTSIDE_LOOP }) << "A bare break at function top level must raise JUMP_OUTSIDE_LOOP.";

    EXPECT_EQ(MAREA_codesOf("function f() continue; end"), Codes{ JUMP_OUTSIDE_LOOP }) << "A bare continue at function top level must raise JUMP_OUTSIDE_LOOP.";

    EXPECT_EQ(MAREA_codesOf("function f(a: boolean) if a then break; end end"), Codes{ JUMP_OUTSIDE_LOOP }) << "A break inside an if with no enclosing loop must raise JUMP_OUTSIDE_LOOP.";

    EXPECT_EQ(MAREA_codesOf("function f(a: integer32) with a as view do continue; end end"), Codes{ JUMP_OUTSIDE_LOOP }) << "A continue inside a with with no enclosing loop must raise JUMP_OUTSIDE_LOOP.";
}

TEST(MAREASemaControlFlow, BreakOrContinueInsideALoopIsAccepted)
{
    GTEST_COUT << "Validating a break or continue correctly inside a loop raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f(a: boolean) while a do break; end end").empty()) << "A break inside a while must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f() for i in 1 .. 3 do continue; end end").empty()) << "A continue inside a for-range must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("function f(values: array<integer32>) for each v in values do break; end end").empty()) << "A break inside a for-each must raise nothing.";
}

TEST(MAREASemaControlFlow, ReturnWithNoEnclosingLoopIsStillUnreported)
{
    GTEST_COUT << "Validating a return with no enclosing loop raises nothing from this check." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function f() return; end").empty()) << "A bare return at function top level must raise nothing.";
}

TEST(MAREASemaControlFlow, RepeatedArmIsReportedAtTheRepeat)
{
    GTEST_COUT << "Validating the diagnostic points at the repeated default and at the repeated value." << std::endl;

    const std::string PREFIX{ "namespace main; function f(n: integer32) switch n " };

    const std::string DEFAULT_SOURCE{ PREFIX + "default then return; default then return; end end" };

    MAREA_SemaFixture defaultFixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(defaultFixture.MAREA_sema(DEFAULT_SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    ASSERT_EQ(defaultFixture.parser.MAREA_offsets().size(), 2U) << "The parser now also flags the second default as not last, so two diagnostics are raised.";

    EXPECT_EQ(defaultFixture.parser.MAREA_offsets()[0], DEFAULT_SOURCE.rfind("default")) << "The parse diagnostic must sit on the second default arm.";

    EXPECT_EQ(defaultFixture.parser.MAREA_offsets()[1], DEFAULT_SOURCE.rfind("default")) << "The sema diagnostic must also sit on the second default arm.";

    const std::string CASE_SOURCE{ PREFIX + "case 7 then return; case 8, 7 then return; end end" };

    MAREA_SemaFixture caseFixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(caseFixture.MAREA_sema(CASE_SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The unit must check.";

    ASSERT_EQ(caseFixture.parser.MAREA_offsets().size(), 1U) << "One diagnostic must be raised.";

    EXPECT_EQ(caseFixture.parser.MAREA_offsets()[0], CASE_SOURCE.rfind('7')) << "The diagnostic must sit on the repeated value.";
}

TEST(MAREASemaControlFlow, SwitchOverAnImportedEnumChecksCoverage)
{
    GTEST_COUT << "Validating a switch over an enum imported from another file is checked for coverage." << std::endl;

    MAREA_SemaFixture library;

    ASSERT_EQ(library.parser.MAREA_parse("namespace a.b; #[public] enum Color red; green; blue; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The library must parse.";

    const auto RESOLVE = [&library](const std::string &name, const MAREA_AstNode **root) {
        *root = library.parser.root;

        return (name == "a.b") ? MAREA_SEMA_IMPORT_RESULT_FOUND : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    };

    const std::string PARTIAL{ "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.green then return 2; end end" };

    const std::string FULL{ "function f(c: Color): integer32 switch c case Color.red then return 1; case Color.green then return 2; case Color.blue then return 3; end end" };

    const std::string DEFAULTED{ "function f(c: Color): integer32 switch c case Color.red then return 1; default then return 2; end end" };

    MAREA_SemaFixture member(RESOLVE);

    ASSERT_EQ(member.MAREA_sema("namespace main; import a.b { Color }; " + PARTIAL), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(member.MAREA_semaCodes(), Codes{ NON_EXHAUSTIVE }) << "A missing variant of a member import must raise NON_EXHAUSTIVE_SWITCH.";

    MAREA_SemaFixture whole(RESOLVE);

    ASSERT_EQ(whole.MAREA_sema("namespace main; import a.b.Color; " + PARTIAL), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(whole.MAREA_semaCodes(), Codes{ NON_EXHAUSTIVE }) << "A missing variant of a whole-path import must raise NON_EXHAUSTIVE_SWITCH.";

    MAREA_SemaFixture covered(RESOLVE);

    ASSERT_EQ(covered.MAREA_sema("namespace main; import a.b { Color }; " + FULL), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(covered.MAREA_semaCodes().empty()) << "Every variant covered must raise nothing.";

    MAREA_SemaFixture defaulted(RESOLVE);

    ASSERT_EQ(defaulted.MAREA_sema("namespace main; import a.b { Color }; " + DEFAULTED), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(defaulted.MAREA_semaCodes().empty()) << "A default arm must raise nothing.";
}

TEST(MAREASemaControlFlow, SwitchOverAnAliasedImportedEnumChecksCoverage)
{
    GTEST_COUT << "Validating a switch over an enum imported under an alias is checked for coverage." << std::endl;

    MAREA_SemaFixture library;

    ASSERT_EQ(library.parser.MAREA_parse("namespace a.b; #[public] enum Color red; green; blue; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The library must parse.";

    MAREA_SemaFixture fixture([&library](const std::string &name, const MAREA_AstNode **root) {
        *root = library.parser.root;

        return (name == "a.b") ? MAREA_SEMA_IMPORT_RESULT_FOUND : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    });

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { Color as Hue }; function g(h: Hue): integer32 switch h case Hue.red then return 1; end end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), Codes{ NON_EXHAUSTIVE }) << "A missing variant behind an alias must raise NON_EXHAUSTIVE_SWITCH.";
}

TEST(MAREASemaControlFlow, SwitchOverAnExternalEnumIsStillExempt)
{
    GTEST_COUT << "Validating a switch over a type of an external namespace is still not checked." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { Color }; function f(c: Color): integer32 switch c case Color.red then return 1; end end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "An external type has no tree, so it must raise nothing.";
}
