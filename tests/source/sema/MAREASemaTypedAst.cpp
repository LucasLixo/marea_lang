// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

TEST(MAREASemaTypedAst, NullPointerIsRejected)
{
    GTEST_COUT << "Validating a null context, root or outRoot is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    EXPECT_EQ(MAREA_SemaBuildTypedAst(nullptr, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";

    EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, nullptr, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null root must be rejected.";

    EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null outRoot must be rejected.";
}

TEST(MAREASemaTypedAst, NeverCheckedRootIsRejected)
{
    GTEST_COUT << "Validating a root that was never checked is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.parser.MAREA_parse("namespace main; structure S a: integer32; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The source must parse.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED) << "A root sema never checked must be rejected.";
}

TEST(MAREASemaTypedAst, CheckedWithErrorsIsRejected)
{
    GTEST_COUT << "Validating a root checked with errors present is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    MAREA_IGNORE(fixture.MAREA_sema("namespace main; structure S a: unknown_type; end"));

    ASSERT_GT(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The source must raise at least one error.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED) << "A root checked with errors must be rejected.";
}

TEST(MAREASemaTypedAst, CheckedRootBuildsAMatchingShadowTree)
{
    GTEST_COUT << "Validating a checked root with zero errors builds a 1 to 1 shadow tree." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure S a: integer32; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The source must raise no error.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    ASSERT_NE(outRoot, nullptr) << "A successful build must produce a typed root.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetAstNode(outRoot), fixture.parser.root) << "The typed root must shadow the original root.";

    const MAREA_IntegerSize ORIGINAL_COUNT{ MAREA_AstNodeGetChildCount(fixture.parser.root) };

    EXPECT_EQ(MAREA_SemaTypedNodeGetChildCount(outRoot), ORIGINAL_COUNT) << "The typed root must have the same child count as the original root.";

    for (MAREA_IntegerSize index{ 0 }; index < ORIGINAL_COUNT; ++index) {
        const MAREA_AstNode *originalChild{ nullptr };

        ASSERT_EQ(MAREA_AstNodeGetChildAt(fixture.parser.root, index, &originalChild), MAREA_AST_EXCEPT_SUCCESS) << "The original child must be reachable.";

        const MAREA_SemaTypedNode *typedChild{ nullptr };

        EXPECT_EQ(MAREA_SemaTypedNodeGetChildAt(outRoot, index, &typedChild), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed child must be reachable at the same index.";

        EXPECT_EQ(MAREA_SemaTypedNodeGetAstNode(typedChild), originalChild) << "The typed child must shadow the original child in the same order.";
    }
}

TEST(MAREASemaTypedAst, OutOfRangeChildIsRejected)
{
    GTEST_COUT << "Validating an out of range child index is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure S a: integer32; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(outRoot) };

    const MAREA_SemaTypedNode *out{ nullptr };

    EXPECT_EQ(MAREA_SemaTypedNodeGetChildAt(outRoot, COUNT, &out), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "An index not less than the child count must be rejected.";
}
