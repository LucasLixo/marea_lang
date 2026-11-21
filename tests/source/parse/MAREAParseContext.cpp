// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

TEST(MAREAParseContext, CreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_ParseContextCreate rejects null arguments." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "The arena must be created.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The sink must be created.";

    MAREA_ParseContext *context{ nullptr };

    EXPECT_EQ(MAREA_ParseContextCreate(nullptr, sink, &context), MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_ParseContextCreate(arena, nullptr, &context), MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER) << "A null sink must be rejected.";

    EXPECT_EQ(MAREA_ParseContextCreate(arena, sink, nullptr), MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREAParseContext, GettersReturnWhatWasGiven)
{
    GTEST_COUT << "Validating the context getters return the arena and the sink it was created with." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_NE(fixture.context, nullptr) << "The fixture must create a context.";

    EXPECT_EQ(MAREA_ParseContextGetArena(fixture.context), fixture.lexer.arena) << "The arena must be the one given.";

    EXPECT_EQ(MAREA_ParseContextGetDiagnostics(fixture.context), fixture.lexer.sink) << "The sink must be the one given.";
}

TEST(MAREAParseContext, GettersOnNullReturnNull)
{
    GTEST_COUT << "Validating the context getters accept null." << std::endl;

    EXPECT_EQ(MAREA_ParseContextGetArena(nullptr), nullptr) << "A null context has no arena.";

    EXPECT_EQ(MAREA_ParseContextGetDiagnostics(nullptr), nullptr) << "A null context has no sink.";
}

TEST(MAREAParseContext, ParseRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_ParseCompilationUnit rejects null arguments." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.lexer.MAREA_lex("namespace main;"), MAREA_LEX_EXCEPT_SUCCESS) << "Lexing must succeed.";

    const MAREA_AstNode *root{ nullptr };

    EXPECT_EQ(MAREA_ParseCompilationUnit(nullptr, fixture.lexer.list, &root), MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_ParseCompilationUnit(fixture.context, nullptr, &root), MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER) << "A null list must be rejected.";

    EXPECT_EQ(MAREA_ParseCompilationUnit(fixture.context, fixture.lexer.list, nullptr), MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";
}
