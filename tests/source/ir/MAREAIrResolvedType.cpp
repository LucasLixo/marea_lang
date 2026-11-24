// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

#include "marea/internal/sema/MAREASemaInternal.h"

namespace
{
    const MAREA_ResolveCallback RESOLVE_C{ [](const std::string &text, const MAREA_AstNode **) {
        return (text.compare(0, 2, "c.") == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::string MAREA_resolved(const MAREA_IrNode *node)
    {
        if (MAREA_IrNodeGetResolvedType(node) == nullptr) {
            return "";
        }

        return std::string(MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node));
    }

    std::string MAREA_dumpOf(MAREA_IrFixture &fixture)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<dump failed>";
        }

        return std::string(text, length);
    }

    // The first node of a kind in depth-first order, or null.
    const MAREA_IrNode *MAREA_findKind(const MAREA_IrNode *node, MAREA_IrKind kind)
    {
        if (MAREA_IrNodeGetKind(node) == kind) {
            return node;
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(node); ++index) {
            const MAREA_IrNode *found{ MAREA_findKind(MAREA_irChild(node, index), kind) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }

    bool MAREA_isExpressionKind(MAREA_IrKind kind)
    {
        return (kind == MAREA_IR_KIND_BINARY_EXPRESSION) || (kind == MAREA_IR_KIND_UNARY_EXPRESSION) || (kind == MAREA_IR_KIND_MEMBER_ACCESS) || (kind == MAREA_IR_KIND_INDEX_ACCESS) || (kind == MAREA_IR_KIND_CALL_EXPRESSION) || (kind == MAREA_IR_KIND_INTEGER_LITERAL) || (kind == MAREA_IR_KIND_FLOAT_LITERAL) || (kind == MAREA_IR_KIND_STRING_LITERAL) || (kind == MAREA_IR_KIND_BOOLEAN_LITERAL) || (kind == MAREA_IR_KIND_CHARACTER_LITERAL);
    }

    // The ir invents a local named with a leading dollar sign; it must carry a type like the expressions do.
    bool MAREA_isHiddenLocal(const MAREA_IrNode *node)
    {
        return (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_LOCAL_DECLARATION) && (MAREA_irText(node).compare(0, 1, "$") == 0);
    }

    // An expression that sema leaves untyped on purpose (see the closed list in the sema doc): a callee, a call of an untyped callee, an operation with an untyped operand, or a spawn of a void call.
    bool MAREA_untypedOnPurpose(const MAREA_IrNode *node, const MAREA_IrNode *parent, MAREA_IntegerSize index)
    {
        if ((parent != nullptr) && (MAREA_IrNodeGetKind(parent) == MAREA_IR_KIND_CALL_EXPRESSION) && (index == 0)) {
            return true;
        }

        if ((MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_CALL_EXPRESSION) && (MAREA_IrNodeGetResolvedType(MAREA_irChild(node, 0)) == nullptr)) {
            return true;
        }

        if ((MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_UNARY_EXPRESSION) && (MAREA_IrNodeGetOperator(node) == MAREA_TOKEN_KIND_KEYWORD_SPAWN) && (MAREA_resolved(MAREA_irChild(node, 0)) == "void")) {
            return true;
        }

        for (MAREA_IntegerSize child{ 0 }; child < MAREA_IrNodeGetChildCount(node); ++child) {
            const MAREA_IrNode *operand{ MAREA_irChild(node, child) };

            if ((MAREA_isExpressionKind(MAREA_IrNodeGetKind(operand)) || (MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_IDENTIFIER)) && (MAREA_IrNodeGetResolvedType(operand) == nullptr)) {
                return true;
            }
        }

        return false;
    }

    // Counts the expression nodes with no resolved type that are not explained, and the ones that are, in a lowered tree.
    void MAREA_collectMissing(const MAREA_IrNode *node, const MAREA_IrNode *parent, MAREA_IntegerSize index, const std::string &file, std::string &missing, std::size_t &onPurpose)
    {
        const MAREA_IrKind KIND{ MAREA_IrNodeGetKind(node) };

        if ((MAREA_isExpressionKind(KIND) || (KIND == MAREA_IR_KIND_ZERO_VALUE) || MAREA_isHiddenLocal(node)) && (MAREA_IrNodeGetResolvedType(node) == nullptr)) {
            if (MAREA_untypedOnPurpose(node, parent, index)) {
                ++onPurpose;
            } else {
                missing += file + ": " + MAREA_IrKindGetName(KIND) + " " + MAREA_irText(node) + " at line " + std::to_string(MAREA_IrNodeGetPosition(node).line) + "\n";
            }
        }

        for (MAREA_IntegerSize child{ 0 }; child < MAREA_IrNodeGetChildCount(node); ++child) {
            MAREA_collectMissing(MAREA_irChild(node, child), node, child, file, missing, onPurpose);
        }
    }
} // namespace

TEST(MAREAIrResolvedType, NullNodeGivesNoText)
{
    GTEST_COUT << "Validating the resolved type accessors of a null node give null and zero." << std::endl;

    EXPECT_EQ(MAREA_IrNodeGetResolvedType(nullptr), nullptr) << "A null node has no resolved type.";

    EXPECT_EQ(MAREA_IrNodeGetResolvedTypeLength(nullptr), MAREA_INTEGER_SIZE(0)) << "A null node has a zero length.";
}

TEST(MAREAIrResolvedType, BinaryExpressionCarriesItsType)
{
    GTEST_COUT << "Validating x + 1 with x: sinteger32 carries resolved sinteger32 and dumps it before the operator." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f(x: sinteger32): sinteger32 return x + 1; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *binary{ MAREA_findKind(fixture.module, MAREA_IR_KIND_BINARY_EXPRESSION) };

    ASSERT_NE(binary, nullptr) << "The tree must hold a BINARY_EXPRESSION.";

    EXPECT_EQ(MAREA_resolved(binary), "sinteger32") << "The binary expression must carry its resolved type.";

    EXPECT_EQ(MAREA_resolved(MAREA_irChild(binary, 0)), "sinteger32") << "The identifier x must carry its resolved type.";

    EXPECT_EQ(MAREA_resolved(MAREA_irChild(binary, 1)), "sinteger32") << "The literal 1 must carry its resolved type.";

    EXPECT_NE(MAREA_dumpOf(fixture).find("BINARY_EXPRESSION resolved=sinteger32 op=PLUS"), std::string::npos) << "The dump must print resolved= before op=: " << MAREA_dumpOf(fixture);
}

TEST(MAREAIrResolvedType, HeadOnlyTypeNameIsUnchanged)
{
    GTEST_COUT << "Validating the head-only typeName stays the head name while resolved keeps the type arguments." << std::endl;

    MAREA_IrFixture fixture{ RESOLVE_C };

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import c.array; function f(a: array<sinteger32, 4>) end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *parameter{ MAREA_findKind(fixture.module, MAREA_IR_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The tree must hold a PARAMETER.";

    EXPECT_EQ(MAREA_irTypeName(parameter), "array") << "The typeName must stay the head name.";

    EXPECT_EQ(MAREA_resolved(parameter), "array<sinteger32,4>") << "The resolved type must keep the type arguments.";
}

TEST(MAREAIrResolvedType, DeclarationsCarryTheirType)
{
    GTEST_COUT << "Validating every declaration kind that has a type carries it in the dump." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant LIMIT: uinteger8 = 7; structure Point x: float64; end function f(a: sinteger32): sinteger32 local b: sinteger32 = a; return b; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_NE(DUMP.find("CONSTANT_DECLARATION \"LIMIT\" type=uinteger8 resolved=uinteger8"), std::string::npos) << DUMP;

    EXPECT_NE(DUMP.find("FIELD_DECLARATION \"x\" type=float64 resolved=float64"), std::string::npos) << DUMP;

    EXPECT_NE(DUMP.find("FUNCTION_DECLARATION \"f\" type=sinteger32 resolved=sinteger32"), std::string::npos) << DUMP;

    EXPECT_NE(DUMP.find("PARAMETER \"a\" type=sinteger32 resolved=sinteger32"), std::string::npos) << DUMP;

    EXPECT_NE(DUMP.find("LOCAL_DECLARATION \"b\" type=sinteger32 resolved=sinteger32"), std::string::npos) << DUMP;
}

TEST(MAREAIrResolvedType, UntypedExpressionIsLeftOutOnce)
{
    GTEST_COUT << "Validating an expression sema cannot type raises 4006 once and leaves its function out." << std::endl;

    MAREA_IrFixture fixture{ RESOLVE_C };

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import c.stdio; function f() local x: sinteger32 = stdio.value; end function g() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNTYPED_EXPRESSION }) << "One untyped expression must raise 4006 once.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(DUMP.find("\"f\""), std::string::npos) << "The function with the untyped expression must be left out: " << DUMP;

    EXPECT_NE(DUMP.find("FUNCTION_DECLARATION \"g\""), std::string::npos) << "The other function must stay: " << DUMP;
}

TEST(MAREAIrResolvedType, UntypedOnPurposeStaysInTheTree)
{
    GTEST_COUT << "Validating a call of an external function with no signature stays in the tree with no resolved type and raises nothing." << std::endl;

    MAREA_IrFixture fixture{ RESOLVE_C };

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import c.stdio; function f() stdio.puts(\"a\"); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_TRUE(fixture.MAREA_irCodes().empty()) << "A call that sema leaves untyped on purpose must raise nothing.";

    const MAREA_IrNode *call{ MAREA_findKind(fixture.module, MAREA_IR_KIND_CALL_EXPRESSION) };

    ASSERT_NE(call, nullptr) << "The call must stay in the tree.";

    EXPECT_EQ(MAREA_IrNodeGetResolvedType(call), nullptr) << "The call must have no resolved type.";

    EXPECT_EQ(MAREA_IrNodeGetResolvedTypeLength(call), MAREA_INTEGER_SIZE(0)) << "The length must be zero.";
}

TEST(MAREAIrResolvedType, UntypedExpressionCodeIs4006)
{
    GTEST_COUT << "Validating UNTYPED_EXPRESSION keeps the number 4006." << std::endl;

    EXPECT_EQ(MAREA_IR_DIAGNOSTIC_CODE_UNTYPED_EXPRESSION, MAREA_UINTEGER_32(4006)) << "UNTYPED_EXPRESSION must be 4006.";
}

TEST(MAREAIrResolvedType, QualifierNameIsNotUntyped)
{
    GTEST_COUT << "Validating the qualifier of an enum member is not an untyped expression." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Color Red; Green; end function f(): Color return Color.Red; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_TRUE(fixture.MAREA_irCodes().empty()) << "A qualifier name must raise no ir diagnostic.";
}

TEST(MAREAIrResolvedType, DumpOfTheSameUnitTwiceIsIdentical)
{
    GTEST_COUT << "Validating the dump with resolved= is byte-identical on two runs." << std::endl;

    std::string dumps[2];

    for (std::string &dump : dumps) {
        MAREA_IrFixture fixture;

        ASSERT_EQ(fixture.MAREA_lower("namespace main; function f(x: sinteger32): sinteger32 return x * 2 + 1; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

        dump = MAREA_dumpOf(fixture);
    }

    EXPECT_EQ(dumps[0], dumps[1]) << "Two dumps of one unit must match.";
}

TEST(MAREAIrResolvedType, ExamplesTypeEveryLoweredExpression)
{
    GTEST_COUT << "Validating every expression node that stays in a lowered example carries a resolved type." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    std::string missing;

    std::size_t onPurpose{ 0 };

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.MAREA_lower(file.source, file.relativePath), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must lower.";

        MAREA_collectMissing(fixture.module, nullptr, 0, file.relativePath, missing, onPurpose);
    }

    GTEST_COUT << "Expression nodes left untyped on purpose in the examples: " << onPurpose << std::endl;

    EXPECT_TRUE(missing.empty()) << "Every expression node of a lowered example must carry a resolved type or be untyped on purpose:\n"
                                 << missing;
}

namespace
{
    // The text of the first dump line that starts with a prefix, or an empty string.
    std::string MAREA_lineOf(const std::string &dump, const std::string &prefix)
    {
        const std::size_t START{ dump.find(prefix) };

        if (START == std::string::npos) {
            return "";
        }

        return dump.substr(START, dump.find('\n', START) - START);
    }

    std::size_t MAREA_countOf(const std::string &dump, const std::string &part)
    {
        std::size_t count{ 0 };

        for (std::size_t at{ dump.find(part) }; at != std::string::npos; at = dump.find(part, at + 1)) {
            ++count;
        }

        return count;
    }

    const std::string BOX{ "structure Box x: sinteger32; end implementation Box #[operator = \"destroy\"] function drop() end end " };
} // namespace

// DISABLED: an omitted field is now MISSING_FIELD (3077); the ir ZERO_VALUE for fields is dead until the follow-up apply removes it
TEST(MAREAIrResolvedType, DISABLED_WrittenInitializerNeverGetsAZeroNextToIt)
{
    GTEST_COUT << "Validating a local with = or a literal holds exactly one value child and no ZERO_VALUE." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: sinteger32; y: float64; end function f() local a: sinteger32 = 1; local b: Point = Point{ x = 1 }; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *body{ MAREA_irChild(MAREA_irChild(fixture.module, 1), 1) };

    ASSERT_EQ(MAREA_IrNodeGetChildCount(body), MAREA_INTEGER_SIZE(2)) << "The body must hold the two locals.";

    const MAREA_IrKind EXPECTED[]{ MAREA_IR_KIND_INTEGER_LITERAL, MAREA_IR_KIND_LITERAL_VALUE };

    for (MAREA_IntegerSize index{ 0 }; index < 2; ++index) {
        const MAREA_IrNode *local{ MAREA_irChild(body, index) };

        ASSERT_EQ(MAREA_IrNodeGetChildCount(local), MAREA_INTEGER_SIZE(1)) << "Each local must hold exactly one value child, local " << index;

        EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(local, 0)), EXPECTED[index]) << "The only child of local " << index << " has the wrong kind.";
    }
}

TEST(MAREAIrResolvedType, LocalWithNoInitializerLowersWithNoValueChild)
{
    GTEST_COUT << "Validating a local with no value, which sema rejects, lowers with no child and no ZERO_VALUE." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.sema.MAREA_sema("namespace main; function f() local a: sinteger32; end", "main.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "Sema must run.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.sema.parser.MAREA_codes() };

    EXPECT_NE(std::find(CODES.begin(), CODES.end(), MAREA_Uinteger32{ MAREA_SEMA_DIAGNOSTIC_CODE_LOCAL_WITHOUT_INITIAL_VALUE }), CODES.end()) << "Sema must reject the local with 3065.";

    // INFO: the fixture refuses a unit with a sema error and so does the typed AST, so a clean sink is swapped in after the check.
    MAREA_DiagnosticSink *cleanSink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(fixture.sema.parser.lexer.arena, &cleanSink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The clean sink must be created.";

    fixture.sema.context->sink = cleanSink;

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &fixture.typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed AST must build.";

    ASSERT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, fixture.typedRoot, &fixture.module), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *local{ MAREA_findKind(fixture.module, MAREA_IR_KIND_LOCAL_DECLARATION) };

    ASSERT_NE(local, nullptr) << "The local must be lowered.";

    EXPECT_EQ(MAREA_IrNodeGetChildCount(local), MAREA_INTEGER_SIZE(0)) << "A local with no value must hold no child.";

    EXPECT_EQ(MAREA_findKind(fixture.module, MAREA_IR_KIND_ZERO_VALUE), nullptr) << "No ZERO_VALUE may be invented for a local.";
}

// DISABLED: an omitted field is now MISSING_FIELD (3077); the ir ZERO_VALUE for fields is dead until the follow-up apply removes it
TEST(MAREAIrResolvedType, DISABLED_ZeroValueOfAnOmittedFieldCarriesTheFieldType)
{
    GTEST_COUT << "Validating the ZERO_VALUE of an omitted struct field carries the type of that field." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: sinteger32; y: float64; end function f() local p: Point = Point{ x = 1 }; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(MAREA_countOf(DUMP, "ZERO_VALUE resolved=float64"), std::size_t{ 1 }) << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "ZERO_VALUE resolved=sinteger32"), std::size_t{ 0 }) << "The named field x must not be zeroed: " << DUMP;
}

// DISABLED: an omitted field is now MISSING_FIELD (3077); the ir ZERO_VALUE for fields is dead until the follow-up apply removes it
TEST(MAREAIrResolvedType, DISABLED_EmptyLiteralZeroesEveryFieldWithItsOwnType)
{
    GTEST_COUT << "Validating an empty struct literal gives each ZERO_VALUE the type of its own field." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: sinteger32; y: float64; end function f() local p: Point = Point{}; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(MAREA_countOf(DUMP, "ZERO_VALUE resolved=sinteger32"), std::size_t{ 1 }) << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "ZERO_VALUE resolved=float64"), std::size_t{ 1 }) << DUMP;
}

TEST(MAREAIrResolvedType, ForRangeHiddenBoundAndStepAreTyped)
{
    GTEST_COUT << "Validating the for-range desugaring types the loop variable, $last, every reference to them, the tests and the step literal." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f(n: uinteger8) for index in 0 .. n do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(MAREA_lineOf(DUMP, "LOCAL_DECLARATION \"$last0\""), "LOCAL_DECLARATION \"$last0\" resolved=uinteger8") << "$last0 must have the type of the bound: " << DUMP;

    EXPECT_EQ(MAREA_lineOf(DUMP, "LOCAL_DECLARATION \"index\""), "LOCAL_DECLARATION \"index\" resolved=uinteger8 flags=1") << "The loop variable must be typed: " << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "IDENTIFIER \"$last0\" resolved=uinteger8"), std::size_t{ 2 }) << "Both reads of $last0 must be typed: " << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "IDENTIFIER \"index\" resolved=uinteger8"), std::size_t{ 3 }) << "Every read of the loop variable must be typed: " << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "BINARY_EXPRESSION resolved=boolean op="), std::size_t{ 2 }) << "The loop test and the leave test must be boolean: " << DUMP;

    EXPECT_NE(DUMP.find("INTEGER_LITERAL \"1\" resolved=uinteger8"), std::string::npos) << "The step literal must have the type of the variable: " << DUMP;
}

TEST(MAREAIrResolvedType, SwitchHiddenLocalAndTestsAreTyped)
{
    GTEST_COUT << "Validating the switch desugaring types $switch, its references and the tests built from it." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f(v: uinteger8) switch v case 1, 2 then return; case 3 then return; default then return; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(MAREA_lineOf(DUMP, "LOCAL_DECLARATION \"$switch\""), "LOCAL_DECLARATION \"$switch\" resolved=uinteger8") << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "IDENTIFIER \"$switch\" resolved=uinteger8"), std::size_t{ 3 }) << "Every read of $switch must be typed: " << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "BINARY_EXPRESSION resolved=boolean op=EQUAL_EQUAL"), std::size_t{ 3 }) << "Every case test must be boolean: " << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "BINARY_EXPRESSION resolved=boolean op=KEYWORD_OR"), std::size_t{ 1 }) << "The or of a case must be boolean: " << DUMP;
}

TEST(MAREAIrResolvedType, ReturnHoistedThroughADestroyIsTyped)
{
    GTEST_COUT << "Validating $return and its read carry the type of the returned value." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; " + BOX + "function f(): sinteger32 local a: Box = Box(); return a.x; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string DUMP{ MAREA_dumpOf(fixture) };

    EXPECT_EQ(MAREA_lineOf(DUMP, "LOCAL_DECLARATION \"$return\""), "LOCAL_DECLARATION \"$return\" resolved=sinteger32") << DUMP;

    EXPECT_EQ(MAREA_countOf(DUMP, "IDENTIFIER \"$return\" resolved=sinteger32"), std::size_t{ 1 }) << DUMP;

    EXPECT_EQ(MAREA_lineOf(DUMP, "CONSTRUCT_CALL"), "CONSTRUCT_CALL type=Box resolved=Box") << DUMP;

    EXPECT_EQ(MAREA_lineOf(DUMP, "DESTROY_CALL"), "DESTROY_CALL \"Box.drop\"") << "A DESTROY_CALL stays untyped: " << DUMP;
}
