// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

TEST(MAREAIrContext, CreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_IrContextCreate rejects null arguments." << std::endl;

    MAREA_IrFixture fixture;

    MAREA_Arena *arena{ fixture.sema.parser.lexer.arena };

    MAREA_DiagnosticSink *sink{ fixture.sema.parser.lexer.sink };

    MAREA_IrContext *context{ nullptr };

    EXPECT_EQ(MAREA_IrContextCreate(nullptr, sink, &context), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_IrContextCreate(arena, nullptr, &context), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null sink must be rejected.";

    EXPECT_EQ(MAREA_IrContextCreate(arena, sink, nullptr), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";
}

TEST(MAREAIrContext, GettersReturnWhatWasGiven)
{
    GTEST_COUT << "Validating the context getters return the arena and sink it was created with." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    EXPECT_EQ(MAREA_IrContextGetArena(fixture.context), fixture.sema.parser.lexer.arena) << "The arena must be the one given.";

    EXPECT_EQ(MAREA_IrContextGetDiagnostics(fixture.context), fixture.sema.parser.lexer.sink) << "The sink must be the one given.";

    EXPECT_EQ(MAREA_IrContextGetArena(nullptr), nullptr) << "A null context must give a null arena.";

    EXPECT_EQ(MAREA_IrContextGetDiagnostics(nullptr), nullptr) << "A null context must give a null sink.";
}

TEST(MAREAIrContext, LowerRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_IrLowerCompilationUnit rejects null arguments." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "An empty unit must lower.";

    const MAREA_IrNode *module{ nullptr };

    EXPECT_EQ(MAREA_IrLowerCompilationUnit(nullptr, fixture.typedRoot, &module), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, nullptr, &module), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null typed root must be rejected.";

    EXPECT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, fixture.typedRoot, nullptr), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";
}

TEST(MAREAIrContext, LowerRejectsANodeThatIsNotACompilationUnit)
{
    GTEST_COUT << "Validating a typed node that is not a COMPILATION_UNIT is rejected." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "An empty unit must lower.";

    const MAREA_SemaTypedNode *child{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(fixture.typedRoot, 0, &child), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The unit must have a namespace child.";

    const MAREA_IrNode *module{ nullptr };

    EXPECT_EQ(MAREA_IrLowerCompilationUnit(fixture.context, child, &module), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A non-root typed node must be rejected.";
}

TEST(MAREAIrContext, EmptyNamespaceLowersToAnEmptyModule)
{
    GTEST_COUT << "Validating an empty namespace lowers to a MODULE node with zero children." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "An empty unit must lower.";

    ASSERT_NE(fixture.module, nullptr) << "A module must be produced.";

    EXPECT_EQ(MAREA_IrNodeGetKind(fixture.module), MAREA_IR_KIND_MODULE) << "The root kind must be MODULE.";

    EXPECT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 0U) << "An empty unit must give zero children.";

    EXPECT_EQ(fixture.MAREA_irCodes().size(), 0U) << "No ir diagnostic must be raised.";
}

TEST(MAREAIrContext, NodeAccessorsAreNullSafe)
{
    GTEST_COUT << "Validating the node accessors tolerate a null node." << std::endl;

    const MAREA_IrNode *child{ nullptr };

    EXPECT_EQ(MAREA_IrNodeGetKind(nullptr), MAREA_IR_KIND_COUNT) << "A null node must give COUNT.";

    EXPECT_EQ(MAREA_IrNodeGetPosition(nullptr).length, 0U) << "A null node must give a zeroed position.";

    EXPECT_EQ(MAREA_IrNodeGetText(nullptr), nullptr) << "A null node must give null text.";

    EXPECT_EQ(MAREA_IrNodeGetTextLength(nullptr), 0U) << "A null node must give zero text length.";

    EXPECT_EQ(MAREA_IrNodeGetOperator(nullptr), MAREA_TOKEN_KIND_ERROR) << "A null node must give no operator.";

    EXPECT_EQ(MAREA_IrNodeGetFlags(nullptr), 0U) << "A null node must give no flags.";

    EXPECT_EQ(MAREA_IrNodeGetChildCount(nullptr), 0U) << "A null node must give zero children.";

    EXPECT_EQ(MAREA_IrNodeGetTypeName(nullptr), nullptr) << "A null node must give a null type name.";

    EXPECT_EQ(MAREA_IrNodeGetTypeNameLength(nullptr), 0U) << "A null node must give a zero type name length.";

    EXPECT_EQ(MAREA_IrNodeGetChildAt(nullptr, 0, &child), MAREA_IR_NODE_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected.";
}

TEST(MAREAIrContext, ChildAtChecksItsArguments)
{
    GTEST_COUT << "Validating MAREA_IrNodeGetChildAt rejects a null out pointer and an index out of range." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "An empty unit must lower.";

    const MAREA_IrNode *child{ nullptr };

    EXPECT_EQ(MAREA_IrNodeGetChildAt(fixture.module, 0, nullptr), MAREA_IR_NODE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    EXPECT_EQ(MAREA_IrNodeGetChildAt(fixture.module, 0, &child), MAREA_IR_NODE_EXCEPT_ERROR_OUT_OF_RANGE) << "An index past the end must be rejected.";
}

TEST(MAREAIrContext, KindNamesAreStable)
{
    GTEST_COUT << "Validating MAREA_IrKindGetName names every kind and rejects an out-of-range value." << std::endl;

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_MODULE), "MODULE") << "The first kind must be MODULE.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_UNREACHABLE_STATEMENT), "UNREACHABLE_STATEMENT") << "The kind before the parallel ones must stay UNREACHABLE_STATEMENT.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_FORK_JOIN_BLOCK), "FORK_JOIN_BLOCK") << "The fork_join kind must be named.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_PARTITION_STATEMENT), "PARTITION_STATEMENT") << "The partition kind must be named.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_WITH_STATEMENT), "WITH_STATEMENT") << "The with kind must be named.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_FOR_EACH_STATEMENT), "FOR_EACH_STATEMENT") << "The for-each kind must be named.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_TYPE_NAME), "TYPE_NAME") << "The type operand kind must be named.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_BUILTIN_NAME), "BUILTIN_NAME") << "The last kind must be BUILTIN_NAME.";

    EXPECT_EQ(static_cast<int>(MAREA_IR_KIND_FOR_EACH_STATEMENT), static_cast<int>(MAREA_IR_KIND_WITH_STATEMENT) + 1) << "FOR_EACH_STATEMENT must come after the parallel kinds, so no value moves.";

    EXPECT_EQ(static_cast<int>(MAREA_IR_KIND_FORK_JOIN_BLOCK), static_cast<int>(MAREA_IR_KIND_UNREACHABLE_STATEMENT) + 1) << "The new kinds must come after the old ones, so no value moves.";

    EXPECT_EQ(static_cast<int>(MAREA_IR_KIND_TYPE_NAME), static_cast<int>(MAREA_IR_KIND_FOR_EACH_STATEMENT) + 1) << "TYPE_NAME must come after the for-each kind, so no value moves.";

    EXPECT_EQ(static_cast<int>(MAREA_IR_KIND_BUILTIN_NAME), static_cast<int>(MAREA_IR_KIND_TYPE_NAME) + 1) << "BUILTIN_NAME must follow TYPE_NAME.";

    EXPECT_EQ(static_cast<int>(MAREA_IR_KIND_COUNT), static_cast<int>(MAREA_IR_KIND_BUILTIN_NAME) + 1) << "BUILTIN_NAME must be the last kind before COUNT.";

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_COUNT), "UNKNOWN") << "COUNT must be unknown.";

    for (int kind{ 0 }; kind < static_cast<int>(MAREA_IR_KIND_COUNT); ++kind) {
        EXPECT_STRNE(MAREA_IrKindGetName(static_cast<MAREA_IrKind>(kind)), "UNKNOWN") << "Kind " << kind << " must have a name.";
    }
}
