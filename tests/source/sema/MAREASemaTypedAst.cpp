// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    const MAREA_SemaTypedNode *MAREA_findTypedNodeByKind(const MAREA_SemaTypedNode *node, MAREA_AstKind kind)
    {
        if (node == nullptr) {
            return nullptr;
        }

        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            return node;
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                continue;
            }

            const MAREA_SemaTypedNode *found{ MAREA_findTypedNodeByKind(child, kind) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }
} // namespace

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

TEST(MAREASemaTypedAst, ImportMemberListIsCarriedIntoTheShadowTree)
{
    GTEST_COUT << "Validating an import with a member list keeps its IMPORT_MEMBER nodes in the shadow tree." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { f as g, T };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The source must raise no error.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with a member list must build.";

    const MAREA_SemaTypedNode *import{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_IMPORT_DECLARATION) };

    ASSERT_NE(import, nullptr) << "The import must be in the shadow tree.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetChildCount(import), MAREA_INTEGER_SIZE(3)) << "The import must keep its namespace and its two members.";

    EXPECT_NE(MAREA_findTypedNodeByKind(import, MAREA_AST_KIND_IMPORT_MEMBER), nullptr) << "A member must be in the shadow tree.";
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

TEST(MAREASemaTypedAst, PrimitiveReturnedIdentifierIsTaggedPrimitive)
{
    GTEST_COUT << "Validating a primitive by-value identifier is tagged PRIMITIVE." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(n: integer32): integer32 return n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *returnStatement{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_RETURN_STATEMENT) };

    ASSERT_NE(returnStatement, nullptr) << "The typed tree must carry a RETURN_STATEMENT node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(returnStatement, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The returned expression must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(returned), MAREA_SEMA_TYPE_CLASS_PRIMITIVE) << "A primitive identifier must classify as PRIMITIVE.";
}

TEST(MAREASemaTypedAst, UserStructureReturnedIdentifierIsTaggedUser)
{
    GTEST_COUT << "Validating a user structure identifier is tagged USER with its declaration." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(s: Shape): Shape return copy s; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *structureNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_STRUCTURE_DECLARATION) };

    ASSERT_NE(structureNode, nullptr) << "The typed tree must carry the structure declaration node.";

    const MAREA_SemaTypedNode *copyExpression{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(copyExpression, nullptr) << "The typed tree must carry the copy expression node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(copyExpression, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The copied identifier must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(returned), MAREA_SEMA_TYPE_CLASS_USER) << "A structure-typed identifier must classify as USER.";

    const MAREA_AstNode *declaration{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeDeclaration(returned, &declaration), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The declaration must be reachable.";

    EXPECT_EQ(declaration, MAREA_SemaTypedNodeGetAstNode(structureNode)) << "The USER declaration must point at the Shape structure declaration.";
}

TEST(MAREASemaTypedAst, ExternalReturnedIdentifierIsTaggedExternal)
{
    GTEST_COUT << "Validating a resolver-reported external identifier is tagged EXTERNAL with its name." << std::endl;

    MAREA_ResolveCallback resolve{ [](const std::string &text, const MAREA_AstNode **) { return (text == "array") ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; } };

    MAREA_SemaFixture fixture(resolve);

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(a: array<integer32>): array<integer32> return copy a; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *copyExpression{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(copyExpression, nullptr) << "The typed tree must carry the copy expression node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(copyExpression, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The copied identifier must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(returned), MAREA_SEMA_TYPE_CLASS_EXTERNAL) << "A resolver-external identifier must classify as EXTERNAL.";

    const MAREA_Character8 *outText{ nullptr };

    MAREA_IntegerSize outLength{ 0 };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeName(returned, &outText, &outLength), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The type name must be reachable.";

    EXPECT_EQ(std::string(outText, outLength), "array") << "The EXTERNAL name must be the type name text.";
}

TEST(MAREASemaTypedAst, NonGenericCallResultCarriesTheCalleeReturnType)
{
    GTEST_COUT << "Validating a non-generic call result carries the callee's real return type." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; structure Shape x: float32; end function make(): Shape return Shape{ x = 1.0 }; end function f(): Shape return make(); end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *structureNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_STRUCTURE_DECLARATION) };

    ASSERT_NE(structureNode, nullptr) << "The typed tree must carry the structure declaration node.";

    const MAREA_SemaTypedNode *callNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(callNode, nullptr) << "The typed tree must carry the make() call node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(callNode), MAREA_SEMA_TYPE_CLASS_USER) << "The call result must classify as USER.";

    const MAREA_AstNode *declaration{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeDeclaration(callNode, &declaration), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The declaration must be reachable.";

    EXPECT_EQ(declaration, MAREA_SemaTypedNodeGetAstNode(structureNode)) << "The call result must carry the real Shape declaration, not a guess.";
}

TEST(MAREASemaTypedAst, GenericCallResultIsTaggedGenericWithoutSubstitution)
{
    GTEST_COUT << "Validating a generic call result is tagged GENERIC with the raw type kept, never substituted." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; function same<Item>(a: Item): Item return copy a; end function f(): integer32 return same(5); end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *callNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(callNode, nullptr) << "The typed tree must carry the same() call node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(callNode), MAREA_SEMA_TYPE_CLASS_GENERIC) << "An unbound generic call result must classify as GENERIC.";

    const MAREA_Character8 *outText{ nullptr };

    MAREA_IntegerSize outLength{ 0 };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeName(callNode, &outText, &outLength), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The type name must be reachable.";

    EXPECT_EQ(std::string(outText, outLength), "Item") << "The raw, unsubstituted generic parameter name must be kept.";
}

TEST(MAREASemaTypedAst, PrimitiveIdentifierOwnershipIsValue)
{
    GTEST_COUT << "Validating a primitive by-value identifier has ownership mode VALUE." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(n: integer32): integer32 return n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *returnStatement{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_RETURN_STATEMENT) };

    ASSERT_NE(returnStatement, nullptr) << "The typed tree must carry a RETURN_STATEMENT node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(returnStatement, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The returned expression must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(returned), MAREA_SEMA_OWNERSHIP_MODE_VALUE) << "A primitive identifier must carry ownership mode VALUE.";
}

TEST(MAREASemaTypedAst, ReadOnlyBorrowParameterOwnershipIsBorrow)
{
    GTEST_COUT << "Validating a read-only structure parameter has ownership mode BORROW." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(s: Shape) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The typed tree must carry the parameter node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(parameter), MAREA_SEMA_OWNERSHIP_MODE_BORROW) << "A read-only structure parameter must carry ownership mode BORROW.";
}

TEST(MAREASemaTypedAst, MutableBorrowParameterOwnershipIsMutableBorrow)
{
    GTEST_COUT << "Validating a mutable structure parameter has ownership mode MUTABLE_BORROW." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(s: mutable Shape) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The typed tree must carry the parameter node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(parameter), MAREA_SEMA_OWNERSHIP_MODE_MUTABLE_BORROW) << "A mutable structure parameter must carry ownership mode MUTABLE_BORROW.";
}

TEST(MAREASemaTypedAst, OwnedParameterOwnershipIsMove)
{
    GTEST_COUT << "Validating an owned<T> parameter has ownership mode MOVE." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(o: owned<Shape>) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The typed tree must carry the parameter node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(parameter), MAREA_SEMA_OWNERSHIP_MODE_MOVE) << "An owned<T> parameter must carry ownership mode MOVE.";
}

TEST(MAREASemaTypedAst, CopyExpressionOwnershipIsCopy)
{
    GTEST_COUT << "Validating a copy expression has ownership mode COPY." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(s: Shape): Shape return copy s; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *copyExpression{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_UNARY_EXPRESSION) };

    ASSERT_NE(copyExpression, nullptr) << "The typed tree must carry the copy expression node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(copyExpression), MAREA_SEMA_OWNERSHIP_MODE_COPY) << "A copy expression must carry ownership mode COPY.";
}
