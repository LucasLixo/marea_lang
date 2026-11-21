#include "tests/dependencies.hpp"

TEST(MAREALexContext, CreateStoresArenaAndSink)
{
    GTEST_COUT << "Validating MAREA_LexContextCreate stores the given arena and sink." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    MAREA_LexContext *context{ nullptr };

    ASSERT_EQ(MAREA_LexContextCreate(arena, sink, &context), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_LexContextCreate must succeed.";

    EXPECT_EQ(MAREA_LexContextGetArena(context), arena) << "MAREA_LexContextGetArena must return the given arena.";

    EXPECT_EQ(MAREA_LexContextGetDiagnostics(context), sink) << "MAREA_LexContextGetDiagnostics must return the given sink.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREALexContext, CreateRejectsNull)
{
    GTEST_COUT << "Validating MAREA_LexContextCreate rejects every null argument." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    MAREA_LexContext *context{ nullptr };

    EXPECT_EQ(MAREA_LexContextCreate(nullptr, sink, &context), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "MAREA_LexContextCreate must reject a null arena.";

    EXPECT_EQ(MAREA_LexContextCreate(arena, nullptr, &context), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "MAREA_LexContextCreate must reject a null sink.";

    EXPECT_EQ(MAREA_LexContextCreate(arena, sink, nullptr), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "MAREA_LexContextCreate must reject a null outContext.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREALexContext, SetFileNameRoundTrip)
{
    GTEST_COUT << "Validating MAREA_LexContextSetFileName round-trips through MAREA_LexContextGetFileName." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    MAREA_LexContext *context{ nullptr };

    ASSERT_EQ(MAREA_LexContextCreate(arena, sink, &context), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_LexContextCreate must succeed.";

    ASSERT_EQ(MAREA_LexContextSetFileName(context, "example.marea"), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_LexContextSetFileName must succeed.";

    EXPECT_STREQ(MAREA_LexContextGetFileName(context), "example.marea") << "MAREA_LexContextGetFileName must return the name that was set.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREALexContext, DefaultFileName)
{
    GTEST_COUT << "Validating a fresh context defaults its file name to \"<source>\"." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "MAREA_ArenaCreate must succeed.";

    MAREA_DiagnosticSink *sink{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "MAREA_DiagnosticSinkCreate must succeed.";

    MAREA_LexContext *context{ nullptr };

    ASSERT_EQ(MAREA_LexContextCreate(arena, sink, &context), MAREA_LEX_EXCEPT_SUCCESS) << "MAREA_LexContextCreate must succeed.";

    EXPECT_STREQ(MAREA_LexContextGetFileName(context), "<source>") << "A fresh context must default its file name to \"<source>\".";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREALexContext, SetFileNameRejectsNullContext)
{
    GTEST_COUT << "Validating MAREA_LexContextSetFileName rejects a null context." << std::endl;

    EXPECT_EQ(MAREA_LexContextSetFileName(nullptr, "name.marea"), MAREA_LEX_EXCEPT_ERROR_NULL_POINTER) << "MAREA_LexContextSetFileName must reject a null context.";
}
