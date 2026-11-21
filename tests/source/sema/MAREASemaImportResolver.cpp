// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    void MAREA_CALL notFound(MAREA_Null, const MAREA_Character8 *, MAREA_IntegerSize, MAREA_SemaImportResult *outResult, const MAREA_AstNode **)
    {
        *outResult = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    }
} // namespace

TEST(MAREASemaImportResolver, CreateRejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_SemaImportResolverCreate rejects null arguments." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "The arena must be created.";

    MAREA_SemaImportResolver *resolver{ nullptr };

    EXPECT_EQ(MAREA_SemaImportResolverCreate(nullptr, &notFound, nullptr, &resolver), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_SemaImportResolverCreate(arena, nullptr, nullptr, &resolver), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER) << "A null function must be rejected.";

    EXPECT_EQ(MAREA_SemaImportResolverCreate(arena, &notFound, nullptr, nullptr), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER) << "A null out pointer must be rejected.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREASemaImportResolver, UserDataRoundTrips)
{
    GTEST_COUT << "Validating the resolver returns the user data it was created with." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "The arena must be created.";

    int token{ 0 };

    MAREA_SemaImportResolver *resolver{ nullptr };

    ASSERT_EQ(MAREA_SemaImportResolverCreate(arena, &notFound, &token, &resolver), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS) << "The resolver must be created.";

    EXPECT_EQ(MAREA_SemaImportResolverGetUserData(resolver), &token) << "The user data must be the pointer given.";

    EXPECT_EQ(MAREA_SemaImportResolverGetUserData(nullptr), nullptr) << "A null resolver has no user data.";

    MAREA_ArenaDestroy(arena);
}
