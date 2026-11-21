// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_Uinteger32 FORK_JOIN{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_FORK_JOIN };
    const MAREA_Uinteger32 SWITCH{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_SWITCH };
    const MAREA_Uinteger32 FOR_EACH{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_FOR_EACH };
    const MAREA_Uinteger32 GENERIC{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC };
    const MAREA_Uinteger32 MACRO{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO };

    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        const bool EXTERNAL{ (text.compare(0, 12, "collections.") == 0) || (text.compare(0, 12, "concurrency.") == 0) || (text.compare(0, 7, "arenas.") == 0) || (text == "text") || (text.compare(0, 2, "c.") == 0) };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::vector<MAREA_Uinteger32> MAREA_irCodesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_irCodes();
    }

    std::string MAREA_functionNamesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        std::string names;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
            names += (names.empty() ? "" : " ") + MAREA_irText(MAREA_irChild(fixture.module, index));
        }

        return names;
    }

    using Codes = std::vector<MAREA_Uinteger32>;
} // namespace

TEST(MAREAIrGaps, SwitchRaisesItsDiagnosticAndDropsTheFunction)
{
    GTEST_COUT << "Validating a switch statement raises 4002 and its function is left out." << std::endl;

    const std::string SOURCE{ "namespace main; function keep() end function lose(a: integer32) switch a case 1 then return; default then return; end end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), Codes{ SWITCH }) << "A switch must raise UNSUPPORTED_SWITCH once.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "Only the function without the gap must remain.";
}

TEST(MAREAIrGaps, PlainIfRaisesNothing)
{
    GTEST_COUT << "Validating a plain if statement, the neighbor of a switch, raises nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function run(a: integer32) if a == 1 then return; else return; end end"), Codes{}) << "An if must raise no ir diagnostic.";
}

TEST(MAREAIrGaps, ForEachRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a for-each statement raises 4003." << std::endl;

    const std::string SOURCE{ "namespace main; import collections.array; function run(values: array<integer32>) for each value in values do end end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), Codes{ FOR_EACH }) << "A for each must raise UNSUPPORTED_FOR_EACH.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "") << "The function must be left out.";
}

TEST(MAREAIrGaps, ForRangeRaisesNothing)
{
    GTEST_COUT << "Validating a for-range loop, the neighbor of a for-each, raises nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function run() for index in 0 .. 3 do end end"), Codes{}) << "A for range must raise no ir diagnostic.";
}

TEST(MAREAIrGaps, ForkJoinRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a fork_join block raises 4001." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function run() fork_join end end"), Codes{ FORK_JOIN }) << "A fork_join must raise UNSUPPORTED_FORK_JOIN.";
}

TEST(MAREAIrGaps, SpawnRaisesItsDiagnosticInsideAnExpression)
{
    GTEST_COUT << "Validating a spawn expression raises 4001 once and the function is left out." << std::endl;

    const std::string SOURCE{ "namespace main; function work(a: integer32): integer32 return a; end function run() fork_join local task: integer32 = spawn work(1); end end" };

    const Codes CODES{ MAREA_irCodesOf(SOURCE) };

    ASSERT_FALSE(CODES.empty()) << "A fork_join with a spawn must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, FORK_JOIN) << "Only UNSUPPORTED_FORK_JOIN may be raised.";
    }
}

TEST(MAREAIrGaps, WithAndPartitionRaiseTheForkJoinDiagnostic)
{
    GTEST_COUT << "Validating a with statement raises 4001." << std::endl;

    const Codes CODES{ MAREA_irCodesOf("namespace main; import arenas.arena; function run(scratch: mutable arena) with scratch as local_arena do end end") };

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, FORK_JOIN) << "A with statement must raise UNSUPPORTED_FORK_JOIN.";
    }
}

TEST(MAREAIrGaps, CallWithoutGenericsRaisesNothing)
{
    GTEST_COUT << "Validating a plain call and a plain spawn-free body, the neighbors of the fork_join gap, raise nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function twice(a: integer32): integer32 return a + a; end function run(): integer32 return twice(2); end"), Codes{}) << "A plain call must raise no ir diagnostic.";
}

TEST(MAREAIrGaps, GenericFunctionRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a generic function raises 4004 and produces no node." << std::endl;

    const std::string SOURCE{ "namespace main; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function keep() end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), Codes{ GENERIC }) << "A generic function must raise UNINSTANTIATED_GENERIC.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "The generic function must leave no node.";
}

TEST(MAREAIrGaps, GenericStructureRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a generic structure raises 4004 and produces no node." << std::endl;

    const std::string SOURCE{ "namespace main; structure Box<Item> value: Item; end structure Plain x: integer32; end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), Codes{ GENERIC }) << "A generic structure must raise UNINSTANTIATED_GENERIC.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "Plain") << "The generic structure must leave no node.";
}

TEST(MAREAIrGaps, GenericImplementationRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating an implementation with generic parameters raises 4004 and produces no member." << std::endl;

    const std::string SOURCE{ "namespace main; structure Box<Item> value: Item; end implementation<Item> Box<Item> function get(self: Box<Item>): Item return self.value; end end" };

    const Codes CODES{ MAREA_irCodesOf(SOURCE) };

    ASSERT_FALSE(CODES.empty()) << "A generic implementation must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, GENERIC) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "") << "No member of a generic implementation may be lowered.";
}

TEST(MAREAIrGaps, CallOfAGenericFunctionRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a call that yields an unbound generic type raises 4004." << std::endl;

    const std::string SOURCE{ "namespace main; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function run(): integer32 return identity(5); end" };

    const Codes CODES{ MAREA_irCodesOf(SOURCE) };

    ASSERT_FALSE(CODES.empty()) << "The generic function and its call must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, GENERIC) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }
}

TEST(MAREAIrGaps, CallWithExplicitTypeArgumentsRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a call with explicit type arguments raises 4004 for the instantiation." << std::endl;

    const std::string SOURCE{ "namespace main; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function run(): integer32 return identity<integer32>(5); end" };

    const Codes CODES{ MAREA_irCodesOf(SOURCE) };

    ASSERT_FALSE(CODES.empty()) << "An explicit instantiation must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, GENERIC) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }
}

TEST(MAREAIrGaps, MacroOtherThanUnreachableRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a macro statement other than unreachable!() raises 4005 and names the macro." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run() print!(\"hi\"); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(fixture.MAREA_irCodes(), Codes{ MACRO }) << "A print! must raise UNSUPPORTED_MACRO.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sema.parser.lexer.sink, MAREA_DiagnosticSinkGetCount(fixture.sema.parser.lexer.sink) - 1, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The diagnostic must exist.";

    EXPECT_NE(std::string(MAREA_DiagnosticGetMessage(diagnostic)).find("print"), std::string::npos) << "The message must name the macro.";

    EXPECT_EQ(MAREA_DiagnosticGetSeverity(diagnostic), MAREA_DIAGNOSTIC_SEVERITY_ERROR) << "Every ir diagnostic is an error.";

    EXPECT_GT(MAREA_DiagnosticGetPosition(diagnostic).line, 0U) << "The diagnostic must carry a position.";
}

TEST(MAREAIrGaps, UnreachableMacroRaisesNothing)
{
    GTEST_COUT << "Validating unreachable!(), the neighbor of the macro gap, raises nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function run() unreachable!(); end"), Codes{}) << "unreachable!() must raise no ir diagnostic.";
}

TEST(MAREAIrGaps, ModuleLevelMacroRaisesItsDiagnostic)
{
    GTEST_COUT << "Validating a module-level macro statement raises 4005." << std::endl;

    const Codes CODES{ MAREA_irCodesOf("namespace main; static_assert!(true); constant A: integer32 = 1;") };

    EXPECT_EQ(CODES, Codes{ MACRO }) << "A module-level macro must raise UNSUPPORTED_MACRO.";
}

TEST(MAREAIrGaps, OnlyTheGapDeclarationIsDropped)
{
    GTEST_COUT << "Validating lowering continues with the next declaration after a gap." << std::endl;

    EXPECT_EQ(MAREA_functionNamesOf("namespace main; function a() end function b(x: integer32) switch x case 1 then return; default then return; end end function c() end"), "a c") << "The declarations around the gap must remain.";
}

TEST(MAREAIrGaps, EachGapDiagnosticIsRaisedOnce)
{
    GTEST_COUT << "Validating two gap functions raise two diagnostics, one each." << std::endl;

    const Codes CODES{ MAREA_irCodesOf("namespace main; function a(x: integer32) switch x case 1 then return; default then return; end end function b(y: integer32) switch y case 1 then return; default then return; end end") };

    EXPECT_EQ(CODES, (Codes{ SWITCH, SWITCH })) << "Each dropped function must report its own gap once.";
}

TEST(MAREAIrGaps, ExamplesWithGapsOnlyRaiseGapDiagnostics)
{
    GTEST_COUT << "Validating the examples that hold gap constructs lower without a crash and raise only ir diagnostics." << std::endl;

    const std::vector<std::string> FILES{ "parallel/scaling.marea", "basics/control_flow.marea", "types/enum_methods.marea", "types/generics.marea", "macros/builtin.marea", "attributes/layout.marea" };

    const MAREA_IrExampleSet EXAMPLES;

    for (const std::string &file : FILES) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        const std::string SOURCE{ MAREA_irReadExample(file) };

        ASSERT_EQ(fixture.sema.MAREA_sema(SOURCE, file), MAREA_SEMA_EXCEPT_SUCCESS) << "The example " << file << " must check.";

        ASSERT_EQ(fixture.sema.MAREA_errorCount(), 0U) << "The example " << file << " must check without an error.";

        const MAREA_SemaTypedNode *typedRoot{ nullptr };

        ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree of " << file << " must build.";

        const MAREA_IrNode *module{ nullptr };

        ASSERT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, typedRoot, &module), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file << " must lower.";

        EXPECT_EQ(MAREA_DiagnosticSinkGetCount(fixture.sema.parser.lexer.sink), fixture.MAREA_irCodes().size()) << "The example " << file << " may only raise ir diagnostics.";

        EXPECT_EQ(MAREA_IrNodeGetKind(module), MAREA_IR_KIND_MODULE) << "The root of " << file << " must be a MODULE.";
    }
}

TEST(MAREAIrGaps, ExamplesRaiseTheGapsTheyContain)
{
    GTEST_COUT << "Validating the examples raise the gap codes of the constructs they hold, and only those." << std::endl;

    const std::vector<std::pair<std::string, MAREA_Uinteger32>> EXPECTED{
        { "parallel/scaling.marea", FORK_JOIN },
        { "basics/control_flow.marea", FOR_EACH },
        { "types/enum_methods.marea", SWITCH },
        { "types/generics.marea", GENERIC },
        { "macros/builtin.marea", MACRO },
    };

    const MAREA_IrExampleSet EXAMPLES;

    for (const auto &[file, code] : EXPECTED) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample(file), file), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file << " must lower.";

        const Codes CODES{ fixture.MAREA_irCodes() };

        EXPECT_NE(std::find(CODES.begin(), CODES.end(), code), CODES.end()) << "The example " << file << " must raise code " << code << ".";
    }
}
