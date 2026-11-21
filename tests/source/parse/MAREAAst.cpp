// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <type_traits>

namespace
{

    template <typename T, typename = void>
    struct MAREA_IsComplete : std::false_type
    {
    };

    template <typename T>
    struct MAREA_IsComplete<T, std::void_t<decltype(sizeof(T))>> : std::true_type
    {
    };

} // namespace

TEST(MAREAAst, TypesAreOpaque)
{
    GTEST_COUT << "Validating MAREA_AstNode stays opaque to callers." << std::endl;

    EXPECT_FALSE(MAREA_IsComplete<MAREA_AstNode>::value) << "MAREA_AstNode must not be a complete type outside its implementation.";
}

TEST(MAREAAst, AccessorsOnNullReturnDefaults)
{
    GTEST_COUT << "Validating every node accessor returns a safe default for a null node." << std::endl;

    EXPECT_EQ(MAREA_AstNodeGetKind(nullptr), MAREA_AST_KIND_ERROR_NODE) << "MAREA_AstNodeGetKind must return ERROR_NODE for null.";

    EXPECT_EQ(MAREA_AstNodeGetText(nullptr), nullptr) << "MAREA_AstNodeGetText must return null for null.";

    EXPECT_EQ(MAREA_AstNodeGetTextLength(nullptr), MAREA_UINTEGER_32(0)) << "MAREA_AstNodeGetTextLength must return 0 for null.";

    EXPECT_EQ(MAREA_AstNodeGetOperator(nullptr), MAREA_TOKEN_KIND_ERROR) << "MAREA_AstNodeGetOperator must return ERROR for null.";

    EXPECT_EQ(MAREA_AstNodeGetFlags(nullptr), MAREA_UINTEGER_32(0)) << "MAREA_AstNodeGetFlags must return 0 for null.";

    EXPECT_EQ(MAREA_AstNodeGetChildCount(nullptr), MAREA_INTEGER_SIZE(0)) << "MAREA_AstNodeGetChildCount must return 0 for null.";

    EXPECT_EQ(MAREA_AstNodeGetPosition(nullptr).length, MAREA_UINTEGER_32(0)) << "MAREA_AstNodeGetPosition must return a zeroed position for null.";
}

TEST(MAREAAst, GetChildAtRejectsBadArguments)
{
    GTEST_COUT << "Validating MAREA_AstNodeGetChildAt on null arguments." << std::endl;

    const MAREA_AstNode *child{ nullptr };

    EXPECT_EQ(MAREA_AstNodeGetChildAt(nullptr, MAREA_INTEGER_SIZE(0), &child), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected.";
}
