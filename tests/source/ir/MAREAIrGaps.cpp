// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
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

TEST(MAREAIrGaps, SwitchRaisesNothingAndKeepsTheFunction)
{
    GTEST_COUT << "Validating a switch statement is lowered: it raises no ir diagnostic and its function stays in the module." << std::endl;

    const std::string SOURCE{ "namespace main; function keep() end function lose(a: integer32) switch a case 1 then return; default then return; end end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), Codes{}) << "A switch must raise no ir diagnostic.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep lose") << "Both functions must remain, the one with the switch included.";
}

TEST(MAREAIrGaps, PlainIfRaisesNothing)
{
    GTEST_COUT << "Validating a plain if statement, the neighbor of a switch, raises nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function run(a: integer32) if a == 1 then return; else return; end end"), Codes{}) << "An if must raise no ir diagnostic.";
}

TEST(MAREAIrGaps, ForEachRaisesNothingAndKeepsTheFunction)
{
    GTEST_COUT << "Validating a for-each statement is lowered: it raises no ir diagnostic and its function stays in the module." << std::endl;

    const std::string SOURCE{ "namespace main; import collections.array; function run(values: array<integer32>) for each value in values do end end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), Codes{}) << "A for each must raise no ir diagnostic.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "run") << "The function with the for each must remain.";
}

TEST(MAREAIrGaps, ForRangeRaisesNothing)
{
    GTEST_COUT << "Validating a for-range loop, the neighbor of a for-each, raises nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function run() for index in 0 .. 3 do end end"), Codes{}) << "A for range must raise no ir diagnostic.";
}

TEST(MAREAIrGaps, CallWithoutGenericsRaisesNothing)
{
    GTEST_COUT << "Validating a plain call raises nothing." << std::endl;

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

TEST(MAREAIrGaps, LongNameIsCutAndKeepsItsClosingQuote)
{
    GTEST_COUT << "Validating a diagnostic that names a very long declaration is cut to fit and still closes its quote." << std::endl;

    for (const std::size_t SIZE : { 10U, 100U, 119U, 120U, 121U, 159U, 400U }) {
        MAREA_IrFixture fixture;

        ASSERT_EQ(fixture.MAREA_lower("namespace main; structure " + std::string(SIZE, 'a') + "<Item> value: Item; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower for a name of " << SIZE << " bytes.";

        const MAREA_Diagnostic *diagnostic{ nullptr };

        ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.sema.parser.lexer.sink, MAREA_DiagnosticSinkGetCount(fixture.sema.parser.lexer.sink) - 1, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The diagnostic must exist.";

        const std::string MESSAGE{ MAREA_DiagnosticGetMessage(diagnostic) };

        EXPECT_LT(MESSAGE.size(), 160U) << "The message must fit its buffer for a name of " << SIZE << " bytes.";

        EXPECT_EQ(MESSAGE.back(), '\'') << "The name must keep its closing quote for a name of " << SIZE << " bytes.";

        EXPECT_NE(MESSAGE.find(" 'a"), std::string::npos) << "The name must open after the message text for a name of " << SIZE << " bytes.";
    }
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

    EXPECT_EQ(MAREA_functionNamesOf("namespace main; function a() end function b() print!(\"hi\"); end function c() end"), "a c") << "The declarations around the gap must remain.";
}

TEST(MAREAIrGaps, EachGapDiagnosticIsRaisedOnce)
{
    GTEST_COUT << "Validating two gap functions raise two diagnostics, one each." << std::endl;

    const Codes CODES{ MAREA_irCodesOf("namespace main; function a() print!(\"a\"); end function b() print!(\"b\"); end") };

    EXPECT_EQ(CODES, (Codes{ MACRO, MACRO })) << "Each dropped function must report its own gap once.";
}

TEST(MAREAIrGaps, ExamplesWithGapsOnlyRaiseGapDiagnostics)
{
    GTEST_COUT << "Validating the examples that hold gap constructs lower without a crash and raise only ir diagnostics." << std::endl;

    const std::vector<std::string> FILES{ "parallel/scaling.marea", "types/generics.marea", "macros/builtin.marea", "attributes/layout.marea" };

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

TEST(MAREAIrGaps, RemainingCodesKeepTheirNumbers)
{
    GTEST_COUT << "Validating that the two codes still raised keep the numbers 4004 and 4005 after 4001 to 4003 were retired." << std::endl;

    EXPECT_EQ(GENERIC, MAREA_UINTEGER_32(4004)) << "UNINSTANTIATED_GENERIC must stay 4004.";
    EXPECT_EQ(MACRO, MAREA_UINTEGER_32(4005)) << "UNSUPPORTED_MACRO must stay 4005.";
}

TEST(MAREAIrGaps, CallOfADroppedFunctionRaisesAtTheCallSite)
{
    GTEST_COUT << "Validating a call to a function that was dropped from the module raises a diagnostic at the call site, and does not lower to a dangling name." << std::endl;

    const std::string SOURCE{ "namespace main; function noop<Item>(value: Item) end function run() noop(5); end function keep() end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), (Codes{ GENERIC, GENERIC })) << "The generic function and the call that targets it must each raise once.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "The caller must be dropped with its dangling call.";
}

TEST(MAREAIrGaps, CallOfADroppedFunctionDeclaredLaterRaisesAtTheCallSite)
{
    GTEST_COUT << "Validating a call to a dropped function that is declared after the caller raises at the call site too." << std::endl;

    const std::string SOURCE{ "namespace main; function run() noop(5); end function noop<Item>(value: Item) end function keep() end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), (Codes{ GENERIC, GENERIC })) << "The generic function and the call that targets it must each raise once.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "The caller must be dropped with its dangling call.";
}

TEST(MAREAIrGaps, CallOfAFunctionDroppedByItsOwnDanglingCallRaisesToo)
{
    GTEST_COUT << "Validating a function dropped for a dangling call is itself a dropped name, so its own callers raise." << std::endl;

    const std::string SOURCE{ "namespace main; function noop<Item>(value: Item) end function middle() noop(5); end function top() middle(); end function keep() end" };

    EXPECT_EQ(MAREA_irCodesOf(SOURCE), (Codes{ GENERIC, GENERIC, GENERIC })) << "The generic function, its caller and the caller of that caller must each raise once.";

    EXPECT_EQ(MAREA_functionNamesOf(SOURCE), "keep") << "Only the function with no dangling call may remain.";
}

TEST(MAREAIrGaps, CallOfAKeptFunctionStaysQuiet)
{
    GTEST_COUT << "Validating a call to a function that stays in the module raises nothing." << std::endl;

    EXPECT_EQ(MAREA_irCodesOf("namespace main; function noop() end function run() noop(); end"), Codes{}) << "A kept callee must raise nothing.";
}
