// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

TEST(MAREASemaContext, CreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_SemaContextCreate rejects null arguments." << std::endl;

    MAREA_SemaFixture fixture;

    MAREA_Arena *arena{ fixture.parser.lexer.arena };

    MAREA_DiagnosticSink *sink{ fixture.parser.lexer.sink };

    MAREA_SemaContext *context{ nullptr };

    EXPECT_EQ(MAREA_SemaContextCreate(nullptr, sink, fixture.resolver, &context), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_SemaContextCreate(arena, nullptr, fixture.resolver, &context), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null sink must be rejected.";

    EXPECT_EQ(MAREA_SemaContextCreate(arena, sink, nullptr, &context), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null resolver must be rejected.";

    EXPECT_EQ(MAREA_SemaContextCreate(arena, sink, fixture.resolver, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";
}

TEST(MAREASemaContext, GettersReturnWhatWasGiven)
{
    GTEST_COUT << "Validating the context getters return the arena, sink and resolver it was created with." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    EXPECT_EQ(MAREA_SemaContextGetArena(fixture.context), fixture.parser.lexer.arena) << "The arena must be the one given.";

    EXPECT_EQ(MAREA_SemaContextGetDiagnostics(fixture.context), fixture.parser.lexer.sink) << "The sink must be the one given.";

    EXPECT_EQ(MAREA_SemaContextGetImportResolver(fixture.context), fixture.resolver) << "The resolver must be the one given.";
}

TEST(MAREASemaContext, GettersOnNullReturnNull)
{
    GTEST_COUT << "Validating the context getters accept null." << std::endl;

    EXPECT_EQ(MAREA_SemaContextGetArena(nullptr), nullptr) << "A null context has no arena.";

    EXPECT_EQ(MAREA_SemaContextGetDiagnostics(nullptr), nullptr) << "A null context has no sink.";

    EXPECT_EQ(MAREA_SemaContextGetImportResolver(nullptr), nullptr) << "A null context has no resolver.";
}

TEST(MAREASemaContext, CheckRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_SemaCheckCompilationUnit rejects null arguments." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.parser.MAREA_parse("namespace main;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(MAREA_SemaCheckCompilationUnit(nullptr, fixture.parser.root), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_SemaCheckCompilationUnit(fixture.context, nullptr), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null root must be rejected.";
}

TEST(MAREASemaContext, ErrorNodeIsSkippedSilently)
{
    GTEST_COUT << "Validating a tree that holds an ERROR_NODE checks with no sema diagnostic." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; ???"), MAREA_SEMA_EXCEPT_SUCCESS) << "A broken tree must still check.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "No sema diagnostic may appear for a skipped ERROR_NODE.";
}

TEST(MAREASemaContext, EmptyNamespaceChecksClean)
{
    GTEST_COUT << "Validating a namespace-only file checks with no diagnostic." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_errorCount(), 0U) << "A namespace-only file has no error.";
}

TEST(MAREASemaContext, CallerInputsAreEmptyOnANewContext)
{
    GTEST_COUT << "Validating a new context holds no target, no settings and no include resolver." << std::endl;

    MAREA_SemaFixture fixture;

    EXPECT_EQ(MAREA_SemaContextGetTarget(fixture.context), nullptr) << "A new context must hold no target.";
    EXPECT_EQ(MAREA_SemaContextGetSettings(fixture.context), nullptr) << "A new context must hold no settings.";
    EXPECT_EQ(MAREA_SemaContextGetIncludeResolver(fixture.context), nullptr) << "A new context must hold no include resolver.";
}

TEST(MAREASemaContext, CallerInputsRoundTripAndClear)
{
    GTEST_COUT << "Validating the target, the settings and the include resolver are stored as given and a null clears them." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_SemaTarget TARGET{};

    const MAREA_SemaSettings SETTINGS{};

    const MAREA_SemaIncludeResolver INCLUDES{};

    EXPECT_EQ(MAREA_SemaContextSetTarget(fixture.context, &TARGET), MAREA_SEMA_EXCEPT_SUCCESS) << "Setting a target must succeed.";
    EXPECT_EQ(MAREA_SemaContextSetSettings(fixture.context, &SETTINGS), MAREA_SEMA_EXCEPT_SUCCESS) << "Setting the settings must succeed.";
    EXPECT_EQ(MAREA_SemaContextSetIncludeResolver(fixture.context, &INCLUDES), MAREA_SEMA_EXCEPT_SUCCESS) << "Setting an include resolver must succeed.";

    EXPECT_EQ(MAREA_SemaContextGetTarget(fixture.context), &TARGET) << "The target must be the one given.";
    EXPECT_EQ(MAREA_SemaContextGetSettings(fixture.context), &SETTINGS) << "The settings must be the ones given.";
    EXPECT_EQ(MAREA_SemaContextGetIncludeResolver(fixture.context), &INCLUDES) << "The include resolver must be the one given.";

    EXPECT_EQ(MAREA_SemaContextSetTarget(fixture.context, nullptr), MAREA_SEMA_EXCEPT_SUCCESS) << "A null target clears it.";
    EXPECT_EQ(MAREA_SemaContextSetSettings(fixture.context, nullptr), MAREA_SEMA_EXCEPT_SUCCESS) << "Null settings clear them.";
    EXPECT_EQ(MAREA_SemaContextSetIncludeResolver(fixture.context, nullptr), MAREA_SEMA_EXCEPT_SUCCESS) << "A null include resolver clears it.";

    EXPECT_EQ(MAREA_SemaContextGetTarget(fixture.context), nullptr) << "The target must be cleared.";
    EXPECT_EQ(MAREA_SemaContextGetSettings(fixture.context), nullptr) << "The settings must be cleared.";
    EXPECT_EQ(MAREA_SemaContextGetIncludeResolver(fixture.context), nullptr) << "The include resolver must be cleared.";
}

TEST(MAREASemaContext, CallerInputsRejectANullContext)
{
    GTEST_COUT << "Validating the caller input setters reject a null context and the getters give null." << std::endl;

    const MAREA_SemaTarget TARGET{};

    const MAREA_SemaSettings SETTINGS{};

    const MAREA_SemaIncludeResolver INCLUDES{};

    EXPECT_EQ(MAREA_SemaContextSetTarget(nullptr, &TARGET), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected for the target.";
    EXPECT_EQ(MAREA_SemaContextSetSettings(nullptr, &SETTINGS), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected for the settings.";
    EXPECT_EQ(MAREA_SemaContextSetIncludeResolver(nullptr, &INCLUDES), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected for the include resolver.";

    EXPECT_EQ(MAREA_SemaContextGetTarget(nullptr), nullptr) << "A null context has no target.";
    EXPECT_EQ(MAREA_SemaContextGetSettings(nullptr), nullptr) << "A null context has no settings.";
    EXPECT_EQ(MAREA_SemaContextGetIncludeResolver(nullptr), nullptr) << "A null context has no include resolver.";
}
