// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstring>

#include "MareaImportResolver.h"

namespace
{
    class MareaImportResolverTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_EQ(MAREA_ArenaCreate(0, &arena), MAREA_ARENA_EXCEPT_SUCCESS);
            ASSERT_EQ(MAREA_DiagnosticSinkCreate(arena, &sink), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS);
        }

        void TearDown() override { MAREA_ArenaDestroy(arena); }

        MAREA_Arena *arena{ nullptr };
        MAREA_DiagnosticSink *sink{ nullptr };
    };
} // namespace

TEST_F(MareaImportResolverTest, FindsAnExistingNamespaceUnderThePrimaryDirectory)
{
    MareaImportResolverState state{};

    MareaImportResolverInit(&state, arena, sink, nullptr, 0, reinterpret_cast<const MAREA_Character8 *>(MAREA_TESTS_EXAMPLES_DIRECTORY));

    MAREA_SemaImportResult result{};
    const MAREA_AstNode *root{ nullptr };
    const char *namespaceText = "basics.operators";

    MareaImportResolverResolve(&state, reinterpret_cast<const MAREA_Character8 *>(namespaceText), static_cast<MAREA_IntegerSize>(strlen(namespaceText)), &result, &root);

    ASSERT_EQ(result, MAREA_SEMA_IMPORT_RESULT_FOUND);
    ASSERT_NE(root, nullptr);
}

TEST_F(MareaImportResolverTest, AnswersNotFoundWhenNoFileMatches)
{
    MareaImportResolverState state{};

    MareaImportResolverInit(&state, arena, sink, nullptr, 0, reinterpret_cast<const MAREA_Character8 *>(MAREA_TESTS_EXAMPLES_DIRECTORY));

    MAREA_SemaImportResult result{};
    const MAREA_AstNode *root{ nullptr };
    const char *namespaceText = "no.such.namespace";

    MareaImportResolverResolve(&state, reinterpret_cast<const MAREA_Character8 *>(namespaceText), static_cast<MAREA_IntegerSize>(strlen(namespaceText)), &result, &root);

    ASSERT_EQ(result, MAREA_SEMA_IMPORT_RESULT_NOT_FOUND);
}

TEST_F(MareaImportResolverTest, SearchesIncludeDirectoriesWhenThePrimaryDoesNotMatch)
{
    MareaImportResolverState state{};
    const MAREA_Character8 *includeDirectories[1] = { reinterpret_cast<const MAREA_Character8 *>(MAREA_TESTS_EXAMPLES_DIRECTORY) };

    MareaImportResolverInit(&state, arena, sink, includeDirectories, 1, reinterpret_cast<const MAREA_Character8 *>("/does/not/exist"));

    MAREA_SemaImportResult result{};
    const MAREA_AstNode *root{ nullptr };
    const char *namespaceText = "basics.operators";

    MareaImportResolverResolve(&state, reinterpret_cast<const MAREA_Character8 *>(namespaceText), static_cast<MAREA_IntegerSize>(strlen(namespaceText)), &result, &root);

    ASSERT_EQ(result, MAREA_SEMA_IMPORT_RESULT_FOUND);
    ASSERT_NE(root, nullptr);
}
