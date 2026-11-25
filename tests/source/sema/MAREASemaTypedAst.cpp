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

    ASSERT_EQ(fixture.parser.MAREA_parse("namespace main; structure S a: sinteger32; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The source must parse.";

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

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { f as g, T as T };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check.";

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

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure S a: sinteger32; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

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

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure S a: sinteger32; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

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

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(n: sinteger32): sinteger32 return n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

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

    MAREA_ResolveCallback resolve{ [](const std::string &text, const MAREA_AstNode **) { return (text == "list") ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; } };

    MAREA_SemaFixture fixture(resolve);

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(a: list<sinteger32>): list<sinteger32> return copy a; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

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

    EXPECT_EQ(std::string(outText, outLength), "list") << "The EXTERNAL name must be the type name text.";
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

TEST(MAREASemaTypedAst, ConstructionCallResultCarriesTheConstructedTypeDeclaration)
{
    GTEST_COUT << "Validating Type(args) is typed USER with the real structure declaration, not the construct function." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; structure Point x: float32; y: float32; end implementation Point #[operator = \"construct\"] function make(x: float32 = 0.0, y: float32 = 0.0) end end function f(): Point return Point(1.0, 2.0); end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *structureNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_STRUCTURE_DECLARATION) };

    ASSERT_NE(structureNode, nullptr) << "The typed tree must carry the Point structure declaration.";

    const MAREA_SemaTypedNode *callNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(callNode, nullptr) << "The typed tree must carry the Point(1.0, 2.0) call node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(callNode), MAREA_SEMA_TYPE_CLASS_USER) << "The construction call result must classify as USER.";

    const MAREA_AstNode *declaration{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeDeclaration(callNode, &declaration), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The declaration must be reachable.";

    EXPECT_EQ(declaration, MAREA_SemaTypedNodeGetAstNode(structureNode)) << "The construction call result must carry the Point structure declaration, not the construct function.";
}

TEST(MAREASemaTypedAst, GenericCallResultIsTaggedGenericWithoutSubstitution)
{
    GTEST_COUT << "Validating a generic call result is tagged GENERIC with the raw type kept, never substituted." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; function same<Item>(a: Item): Item return copy a; end function f(): sinteger32 return same(5); end" };

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

TEST(MAREASemaTypedAst, ConvertCallResultCarriesTheTargetType)
{
    GTEST_COUT << "Validating a convert<T>(x) call is typed T in the typed-AST hand-off." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; function f(x: sinteger32): sinteger64 return convert<sinteger64>(x); end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *callNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(callNode, nullptr) << "The typed tree must carry the convert call node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(callNode), MAREA_SEMA_TYPE_CLASS_PRIMITIVE) << "The conversion result must classify as PRIMITIVE.";

    const MAREA_Character8 *outText{ nullptr };

    MAREA_IntegerSize outLength{ 0 };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeName(callNode, &outText, &outLength), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The type name must be reachable.";

    ASSERT_NE(outText, nullptr) << "The conversion call must carry a type name.";

    EXPECT_EQ(std::string(outText, outLength), "sinteger64") << "The conversion result must be the target type.";
}

TEST(MAREASemaTypedAst, ConvertToAStructureCarriesTheStructureDeclaration)
{
    GTEST_COUT << "Validating convert<T>(x) to a structure is typed USER with the real declaration." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; structure Meters value: float32; end structure Feet value: float32; end function f(x: Feet): Meters return convert<Meters>(x); end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *callNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(callNode, nullptr) << "The typed tree must carry the convert call node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(callNode), MAREA_SEMA_TYPE_CLASS_USER) << "The conversion result must classify as USER.";

    const MAREA_AstNode *declaration{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeDeclaration(callNode, &declaration), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The declaration must be reachable.";

    ASSERT_NE(declaration, nullptr) << "The conversion call must carry the target declaration.";

    const MAREA_AstNode *firstStructure{ MAREA_SemaTypedNodeGetAstNode(MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_STRUCTURE_DECLARATION)) };

    EXPECT_EQ(declaration, firstStructure) << "The declaration must be that of Meters, the first structure.";
}

TEST(MAREASemaTypedAst, GenericCallWithExplicitTypeArgumentIsSubstituted)
{
    GTEST_COUT << "Validating a generic call with an explicit type argument gets the substituted result type." << std::endl;

    MAREA_SemaFixture fixture;

    const std::string SOURCE{ "namespace main; function same<Item>(a: Item): Item return copy a; end function f(): sinteger32 return same<sinteger32>(5); end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *callNode{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(callNode, nullptr) << "The typed tree must carry the same<sinteger32>() call node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetTypeClass(callNode), MAREA_SEMA_TYPE_CLASS_PRIMITIVE) << "The substituted result must classify as PRIMITIVE, not GENERIC.";

    const MAREA_Character8 *outText{ nullptr };

    MAREA_IntegerSize outLength{ 0 };

    ASSERT_EQ(MAREA_SemaTypedNodeGetTypeName(callNode, &outText, &outLength), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The type name must be reachable.";

    ASSERT_NE(outText, nullptr) << "The generic call must carry a type name.";

    EXPECT_EQ(std::string(outText, outLength), "sinteger32") << "The generic parameter must be replaced by the type argument.";
}

TEST(MAREASemaTypedAst, PrimitiveIdentifierOwnershipIsValue)
{
    GTEST_COUT << "Validating a primitive by-value identifier has ownership mode VALUE." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(n: sinteger32): sinteger32 return n; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *returnStatement{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_RETURN_STATEMENT) };

    ASSERT_NE(returnStatement, nullptr) << "The typed tree must carry a RETURN_STATEMENT node.";

    const MAREA_SemaTypedNode *returned{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(returnStatement, 0, &returned), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The returned expression must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(returned), MAREA_SEMA_OWNERSHIP_MODE_VALUE) << "A primitive identifier must carry ownership mode VALUE.";
}

TEST(MAREASemaTypedAst, EnumParameterOwnershipIsValue)
{
    GTEST_COUT << "Validating an enum parameter has ownership mode VALUE, the documented intentional exception; #[value] no longer exists." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; enum Color red; end function f(c: Color) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The typed tree must carry the parameter node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(parameter), MAREA_SEMA_OWNERSHIP_MODE_VALUE) << "An enum parameter must carry ownership mode VALUE.";
}

TEST(MAREASemaTypedAst, ReadOnlyBorrowParameterOwnershipIsBorrow)
{
    GTEST_COUT << "Validating a #[semantics = reference_read_only] structure parameter has ownership mode BORROW." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(#[semantics = reference_read_only] s: Shape) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The typed tree must carry the parameter node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(parameter), MAREA_SEMA_OWNERSHIP_MODE_BORROW) << "A reference_read_only structure parameter must carry ownership mode BORROW.";
}

TEST(MAREASemaTypedAst, DefaultParameterOwnershipIsBorrow)
{
    GTEST_COUT << "Validating a plain structure parameter with no attribute defaults to ownership mode BORROW, since a non-primitive defaults to reference_read_only." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(s: Shape) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

    const MAREA_SemaTypedNode *outRoot{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &outRoot), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A checked root with zero errors must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(outRoot, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The typed tree must carry the parameter node.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetOwnershipMode(parameter), MAREA_SEMA_OWNERSHIP_MODE_BORROW) << "A structure parameter with no attribute must carry ownership mode BORROW.";
}

TEST(MAREASemaTypedAst, MutableBorrowParameterOwnershipIsMutableBorrow)
{
    GTEST_COUT << "Validating a plain structure parameter with an explicit reference_mutable has ownership mode MUTABLE_BORROW." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Shape x: float32; end function f(#[semantics = reference_mutable] s: Shape) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check with zero errors.";

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

namespace
{
    // Builds the typed tree of a checked source and reads the resolved type of the first node of a kind.
    struct MAREA_ResolvedTypeResult
    {
        MAREA_SemaTypedAstExcept except{ MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER };
        std::string text;
        bool found{ false };
    };

    MAREA_ResolvedTypeResult MAREA_resolvedTypeOf(MAREA_SemaFixture &fixture, const std::string &source, MAREA_AstKind kind)
    {
        MAREA_ResolvedTypeResult result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_SemaTypedNode *node{ MAREA_findTypedNodeByKind(root, kind) };

        if (node == nullptr) {
            return result;
        }

        result.found = true;

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        result.except = MAREA_SemaTypedNodeGetResolvedType(node, &text, &length);

        if (result.except == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            result.text = std::string(text, length);
        }

        return result;
    }

    const MAREA_SemaImportResult MAREA_EXTERNAL_RESULT{ MAREA_SEMA_IMPORT_RESULT_EXTERNAL };
} // namespace

TEST(MAREASemaTypedAst, ResolvedTypeOfParameterKeepsTypeArgumentsWithoutSpaces)
{
    GTEST_COUT << "Validating a parameter with a size argument gives the canonical text with no space." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function f(a: array<sinteger32, 4>) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_TRUE(RESULT.found) << "The parameter must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A parameter has a resolved type.";

    EXPECT_EQ(RESULT.text, "array<sinteger32,4>") << "Type arguments must be joined by a comma with no space.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfLocalKeepsTypeArguments)
{
    GTEST_COUT << "Validating a local declaration gives the canonical text of its written type." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function f() local a: array<sinteger32, 4> = array<sinteger32, 4>{ 0, 0, 0, 0 }; end", MAREA_AST_KIND_LOCAL_DECLARATION) };

    ASSERT_TRUE(RESULT.found) << "The local must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A local has a resolved type.";

    EXPECT_EQ(RESULT.text, "array<sinteger32,4>") << "A local must give array<sinteger32,4>.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfFieldIsItsWrittenType)
{
    GTEST_COUT << "Validating a structure field gives the canonical text of its type." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; structure Point x: float32; end structure Line points: vector<Point>; end", MAREA_AST_KIND_FIELD_DECLARATION) };

    ASSERT_TRUE(RESULT.found) << "The field must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A field has a resolved type.";

    EXPECT_EQ(RESULT.text, "float32") << "The first field in tree order must give float32.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfFieldWithTypeArgumentIsCanonical)
{
    GTEST_COUT << "Validating a field whose type has a user type argument gives vector<Point>." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; structure Line points: vector<Point>; end structure Point x: float32; end", MAREA_AST_KIND_FIELD_DECLARATION) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A field has a resolved type.";

    EXPECT_EQ(RESULT.text, "vector<Point>") << "A user type argument must be kept.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfVariantFieldIsItsWrittenType)
{
    GTEST_COUT << "Validating an enum variant field gives the canonical text of its type." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; enum Shape Dot; Circle(radius: float32 = 1.0); end", MAREA_AST_KIND_VARIANT_FIELD) };

    ASSERT_TRUE(RESULT.found) << "The variant field must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A variant field has a resolved type.";

    EXPECT_EQ(RESULT.text, "float32") << "The variant field must give float32.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfConstantIsItsWrittenType)
{
    GTEST_COUT << "Validating a file constant gives the canonical text of its type." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; constant MAX: sinteger32 = 100;", MAREA_AST_KIND_CONSTANT_DECLARATION) };

    ASSERT_TRUE(RESULT.found) << "The constant must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A constant has a resolved type.";

    EXPECT_EQ(RESULT.text, "sinteger32") << "The constant must give sinteger32.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfFunctionIsItsResult)
{
    GTEST_COUT << "Validating a function declaration gives the canonical text of its result type." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function f(n: sinteger32): array<sinteger32, 4> local a: array<sinteger32, 4> = array<sinteger32, 4>{ 0, 0, 0, 0 }; return a; end", MAREA_AST_KIND_FUNCTION_DECLARATION) };

    ASSERT_TRUE(RESULT.found) << "The function must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A function with a result has a resolved type.";

    EXPECT_EQ(RESULT.text, "array<sinteger32,4>") << "The function must give the type of its result, not of a parameter.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfFunctionWithoutResultHasNoType)
{
    GTEST_COUT << "Validating a function with no result type returns ERROR_NO_TYPE." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function f(n: sinteger32) end", MAREA_AST_KIND_FUNCTION_DECLARATION) };

    ASSERT_TRUE(RESULT.found) << "The function must be in the typed tree.";

    EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "A function with no result must give ERROR_NO_TYPE.";

    EXPECT_TRUE(RESULT.text.empty()) << "No text is given for a function with no result.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfNodeThatDeclaresNoTypeIsNoType)
{
    GTEST_COUT << "Validating a node that does not declare a type returns ERROR_NO_TYPE and clears the outputs." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function f(n: sinteger32) return; end", MAREA_AST_KIND_BLOCK) };

    ASSERT_TRUE(RESULT.found) << "The block must be in the typed tree.";

    EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "A block must give ERROR_NO_TYPE.";
}

TEST(MAREASemaTypedAst, ResolvedTypeClearsTheOutputsOnNoType)
{
    GTEST_COUT << "Validating the outputs are cleared when a node has no type." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(n: sinteger32) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The root must build.";

    const MAREA_Character8 *text{ reinterpret_cast<const MAREA_Character8 *>(0x1) };

    MAREA_IntegerSize length{ 99 };

    EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(root, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "A compilation unit declares no type.";

    EXPECT_EQ(text, nullptr) << "The text must be cleared.";

    EXPECT_EQ(length, MAREA_INTEGER_SIZE(0)) << "The length must be cleared.";
}

TEST(MAREASemaTypedAst, ResolvedTypeIsNullSafe)
{
    GTEST_COUT << "Validating a null node, text or length is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(n: sinteger32) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The root must build.";

    const MAREA_SemaTypedNode *parameter{ MAREA_findTypedNodeByKind(root, MAREA_AST_KIND_PARAMETER) };

    ASSERT_NE(parameter, nullptr) << "The parameter must be in the typed tree.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(nullptr, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null node must be rejected.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(parameter, nullptr, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null text pointer must be rejected.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(parameter, &text, nullptr), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER) << "A null length pointer must be rejected.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfGenericParameterIsItsName)
{
    GTEST_COUT << "Validating a parameter typed by a generic parameter gives the generic name." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function same<Item>(a: Item): Item return copy a; end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_TRUE(RESULT.found) << "The parameter must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A generic parameter has a resolved type.";

    EXPECT_EQ(RESULT.text, "Item") << "A generic parameter must give its own name.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfFunctionTypeParameterUsesTheFunctionSpelling)
{
    GTEST_COUT << "Validating a parameter of a function type gives function(a,b):r with no space and no parameter name." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function apply(op: function(left: sinteger32, right: sinteger32): sinteger32) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_TRUE(RESULT.found) << "The parameter must be in the typed tree.";

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A function type parameter has a resolved type.";

    EXPECT_EQ(RESULT.text, "function(sinteger32,sinteger32):sinteger32") << "A function type must use the function(...) spelling.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfFunctionTypeWithoutResultOmitsTheColon)
{
    GTEST_COUT << "Validating a function type with no result has no colon." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; function run(op: function(text: string)) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A function type parameter has a resolved type.";

    EXPECT_EQ(RESULT.text, "function(string)") << "A function type with no result must end after the parenthesis.";
}

TEST(MAREASemaTypedAst, ResolvedTypeOfNestedArgumentsIsCanonical)
{
    GTEST_COUT << "Validating nested type arguments and a function type argument are rendered whole." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult NESTED{ MAREA_resolvedTypeOf(fixture, "namespace main; function f(a: vector<array<sinteger32, 4>>) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(NESTED.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A nested type has a resolved type.";

    EXPECT_EQ(NESTED.text, "vector<array<sinteger32,4>>") << "Nested arguments must keep every level.";
}

TEST(MAREASemaTypedAst, ResolvedTypeKeepsADottedNameWhole)
{
    GTEST_COUT << "Validating a dotted type name is kept whole, with its arguments." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_EXTERNAL_RESULT; });

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; require c.array; function f(a: c.array<string>) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A dotted type has a resolved type.";

    EXPECT_EQ(RESULT.text, "c.array<string>") << "A dotted name must be kept whole.";
}

TEST(MAREASemaTypedAst, ResolvedTypeReplacesAWholePathImportAlias)
{
    GTEST_COUT << "Validating an import alias is replaced by the last segment of its path, with the written arguments." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_EXTERNAL_RESULT; });

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; import c.collections.list as lst; function f(a: lst<sinteger32>) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "An aliased type has a resolved type.";

    EXPECT_EQ(RESULT.text, "list<sinteger32>") << "A whole-path alias must stand for the last segment of its path.";
}

TEST(MAREASemaTypedAst, ResolvedTypeReplacesAMemberImportAlias)
{
    GTEST_COUT << "Validating a member alias is replaced by the member path as written." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_EXTERNAL_RESULT; });

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; import a.b { Color as Hue }; function f(h: Hue) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "An aliased member has a resolved type.";

    EXPECT_EQ(RESULT.text, "Color") << "A member alias must give the member path.";
}

TEST(MAREASemaTypedAst, ResolvedTypeReplacesTheHeadOfADottedNameThatIsANamespaceAlias)
{
    GTEST_COUT << "Validating the head of a dotted name that is an import alias is replaced and the tail kept." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_EXTERNAL_RESULT; });

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; import c.collections as lst; function f(a: lst.Item) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A dotted aliased type has a resolved type.";

    EXPECT_EQ(RESULT.text, "collections.Item") << "A namespace alias head must be replaced.";
}

TEST(MAREASemaTypedAst, ResolvedTypeFollowsALocalTypeAlias)
{
    GTEST_COUT << "Validating a local type alias is replaced by the type it names, with its arguments, and a chain is followed." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ResolvedTypeResult RESULT{ MAREA_resolvedTypeOf(fixture, "namespace main; structure Point x: float32; end type Grid = array<Point, 4>; type Board = Grid; function f(b: Board) end", MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "An aliased type has a resolved type.";

    EXPECT_EQ(RESULT.text, "array<Point,4>") << "A chain of local aliases must be followed to the full type.";
}

TEST(MAREASemaTypedAst, ResolvedTypeIsTheSameOnTwoRuns)
{
    GTEST_COUT << "Validating two builds of one source give the same text." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: vector<array<sinteger32, 4>>, op: function(x: sinteger32): boolean) end" };

    MAREA_SemaFixture first;

    MAREA_SemaFixture second;

    const MAREA_ResolvedTypeResult ONE{ MAREA_resolvedTypeOf(first, SOURCE, MAREA_AST_KIND_PARAMETER) };

    const MAREA_ResolvedTypeResult TWO{ MAREA_resolvedTypeOf(second, SOURCE, MAREA_AST_KIND_PARAMETER) };

    ASSERT_EQ(ONE.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The first run must resolve.";

    EXPECT_EQ(ONE.text, TWO.text) << "Two runs must give the same text.";
}
