// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

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

    // A node the ir invents (a hidden local, or an operation made from one) is typed by the next apply, so the sweep skips it.
    bool MAREA_mentionsHiddenLocal(const MAREA_IrNode *node)
    {
        if (MAREA_irText(node).compare(0, 1, "$") == 0) {
            return true;
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(node); ++index) {
            if (MAREA_mentionsHiddenLocal(MAREA_irChild(node, index))) {
                return true;
            }
        }

        return false;
    }

    bool MAREA_samePosition(const MAREA_IrNode *left, const MAREA_IrNode *right)
    {
        return (MAREA_IrNodeGetPosition(left).offset == MAREA_IrNodeGetPosition(right).offset) && (MAREA_IrNodeGetPosition(left).line == MAREA_IrNodeGetPosition(right).line);
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

            if (MAREA_isExpressionKind(MAREA_IrNodeGetKind(operand)) && (MAREA_IrNodeGetResolvedType(operand) == nullptr)) {
                return true;
            }
        }

        return false;
    }

    // Counts the expression nodes with no resolved type that are not explained, and the ones that are, in a lowered tree.
    void MAREA_collectMissing(const MAREA_IrNode *node, const MAREA_IrNode *parent, MAREA_IntegerSize index, const std::string &file, std::string &missing, std::size_t &onPurpose)
    {
        const MAREA_IrKind KIND{ MAREA_IrNodeGetKind(node) };

        // The ir invents the literal 1 of a loop step at the position of the loop, so it takes its type from the next apply.
        const bool INVENTED{ MAREA_mentionsHiddenLocal(node) || ((parent != nullptr) && (MAREA_IrNodeGetKind(parent) == MAREA_IR_KIND_ASSIGNMENT) && MAREA_samePosition(node, parent)) };

        if (MAREA_isExpressionKind(KIND) && (MAREA_IrNodeGetResolvedType(node) == nullptr) && !INVENTED) {
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
