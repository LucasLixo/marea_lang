// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <cstddef>

namespace
{
    void MAREA_CALL notFound(MAREA_Null, const MAREA_Character8 *, MAREA_IntegerSize, MAREA_SemaImportResult *outResult, const MAREA_AstNode **)
    {
        *outResult = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    }

    void MAREA_CALL signatureNotFound(MAREA_Null, const MAREA_Character8 *, MAREA_IntegerSize, const MAREA_Character8 *, MAREA_IntegerSize, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature)
    {
        *outResult = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        *outSignature = nullptr;
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

TEST(MAREASemaImportResolver, SetSignatureFunctionRejectsNullResolver)
{
    GTEST_COUT << "Validating MAREA_SemaImportResolverSetSignatureFunction rejects a null resolver." << std::endl;

    EXPECT_EQ(MAREA_SemaImportResolverSetSignatureFunction(nullptr, &signatureNotFound), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER) << "A null resolver must be rejected.";
}

TEST(MAREASemaImportResolver, SetSignatureFunctionAcceptsAResolverCreatedWithNone)
{
    GTEST_COUT << "Validating a resolver created with no signature function accepts one set afterwards, and accepts clearing it back to none." << std::endl;

    MAREA_Arena *arena{ nullptr };

    ASSERT_EQ(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena), MAREA_ARENA_EXCEPT_SUCCESS) << "The arena must be created.";

    MAREA_SemaImportResolver *resolver{ nullptr };

    ASSERT_EQ(MAREA_SemaImportResolverCreate(arena, &notFound, nullptr, &resolver), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS) << "The resolver must be created.";

    EXPECT_EQ(MAREA_SemaImportResolverSetSignatureFunction(resolver, &signatureNotFound), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS) << "Setting a signature function on a resolver created with none must succeed.";

    EXPECT_EQ(MAREA_SemaImportResolverSetSignatureFunction(resolver, nullptr), MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS) << "Clearing the signature function back to none must succeed.";

    MAREA_ArenaDestroy(arena);
}

TEST(MAREASemaSignatureParameter, FieldsKeepTheirOrder)
{
    GTEST_COUT << "Validating MAREA_SemaSignatureParameter keeps its fields in descending size order with no gaps." << std::endl;

    constexpr std::size_t TYPE_TEXT_OFFSET{ offsetof(MAREA_SemaSignatureParameter, typeText) };
    constexpr std::size_t TYPE_TEXT_LENGTH_OFFSET{ offsetof(MAREA_SemaSignatureParameter, typeTextLength) };

    EXPECT_LT(TYPE_TEXT_OFFSET, TYPE_TEXT_LENGTH_OFFSET) << "typeText must come before typeTextLength.";

    constexpr std::size_t FIELD_SIZE_SUM{ sizeof(MAREA_SemaSignatureParameter::typeText) + sizeof(MAREA_SemaSignatureParameter::typeTextLength) };

    EXPECT_GE(sizeof(MAREA_SemaSignatureParameter), FIELD_SIZE_SUM) << "sizeof must be at least the sum of the field sizes.";
}

TEST(MAREASemaSignature, FieldsKeepTheirOrder)
{
    GTEST_COUT << "Validating MAREA_SemaSignature keeps its fields in descending size order with no gaps." << std::endl;

    constexpr std::size_t PARAMETERS_OFFSET{ offsetof(MAREA_SemaSignature, parameters) };
    constexpr std::size_t PARAMETER_COUNT_OFFSET{ offsetof(MAREA_SemaSignature, parameterCount) };
    constexpr std::size_t RESULT_TYPE_TEXT_OFFSET{ offsetof(MAREA_SemaSignature, resultTypeText) };
    constexpr std::size_t RESULT_TYPE_TEXT_LENGTH_OFFSET{ offsetof(MAREA_SemaSignature, resultTypeTextLength) };

    EXPECT_LT(PARAMETERS_OFFSET, PARAMETER_COUNT_OFFSET) << "parameters must come before parameterCount.";

    EXPECT_LT(PARAMETER_COUNT_OFFSET, RESULT_TYPE_TEXT_OFFSET) << "parameterCount must come before resultTypeText.";

    EXPECT_LT(RESULT_TYPE_TEXT_OFFSET, RESULT_TYPE_TEXT_LENGTH_OFFSET) << "resultTypeText must come before resultTypeTextLength.";

    constexpr std::size_t FIELD_SIZE_SUM{ sizeof(MAREA_SemaSignature::parameters) + sizeof(MAREA_SemaSignature::parameterCount) + sizeof(MAREA_SemaSignature::resultTypeText) + sizeof(MAREA_SemaSignature::resultTypeTextLength) };

    EXPECT_GE(sizeof(MAREA_SemaSignature), FIELD_SIZE_SUM) << "sizeof must be at least the sum of the field sizes.";
}
