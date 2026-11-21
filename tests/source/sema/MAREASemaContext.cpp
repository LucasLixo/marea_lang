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
