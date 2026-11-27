// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREACatalogueHelpers.hpp"
#include "tests/MAREASemaHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    struct MAREA_MathTypeExpectation
    {
        const char *name;
        MAREA_Uinteger64 size;
        MAREA_Uinteger64 laneSize;
        std::size_t lanes;
    };

    const std::vector<MAREA_MathTypeExpectation> MATH_TYPES{
        { "Vector2Float32", 16, 4, 2 },
        { "Vector3Float32", 16, 4, 3 },
        { "Vector4Float32", 16, 4, 4 },
        { "Vector2Sinteger32", 16, 4, 2 },
        { "Vector3Sinteger32", 16, 4, 3 },
        { "Vector4Sinteger32", 16, 4, 4 },
        { "Vector2Boolean", 4, 1, 2 },
        { "Vector3Boolean", 4, 1, 3 },
        { "Vector4Boolean", 4, 1, 4 },
    };

    std::string MAREA_mathSource()
    {
        return MAREA_readFile(MAREA_mathSourcePath());
    }

    const MAREA_SemaTypedNode *MAREA_findStructure(const MAREA_SemaTypedNode *node, const std::string &name)
    {
        const MAREA_AstNode *syntax{ MAREA_SemaTypedNodeGetAstNode(node) };

        if ((MAREA_AstNodeGetKind(syntax) == MAREA_AST_KIND_STRUCTURE_DECLARATION) && (MAREA_render(syntax).find(" " + name + " ") != std::string::npos)) {
            return node;
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypedNodeGetChildCount(node); ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                continue;
            }

            const MAREA_SemaTypedNode *found{ MAREA_findStructure(child, name) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }

    std::string MAREA_implementationOf(const std::string &source, const std::string &name)
    {
        const std::string HEAD{ "\nimplementation " + name + "\n" };

        const std::size_t BEGIN{ source.find(HEAD) };

        if (BEGIN == std::string::npos) {
            return "";
        }

        const std::size_t
END
{ source.find("\nend\n", BEGIN + HEAD.size()) };

return source.substr(BEGIN, (END == std::string::npos) ? std::string::npos : END - BEGIN);
}
} // namespace

TEST(MAREASemaMathLibrary, ShippedSourcePassesSemaAndDeclaresTheNineVectorTypes)
{
    GTEST_COUT << "Validating the shipped source of marea.math passes sema and measures each vector type." << std::endl;

    const std::string SOURCE{ MAREA_mathSource() };

    ASSERT_FALSE(SOURCE.empty()) << "The shipped source must exist at " << MAREA_mathSourcePath();

    MAREA_SemaFixture fixture;

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(fixture.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(fixture.MAREA_sema(SOURCE, "marea/math.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The shipped source must give no diagnostic.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must be built.";

    for (const MAREA_MathTypeExpectation &expectation : MATH_TYPES) {
        const MAREA_SemaTypedNode *node{ MAREA_findStructure(root, expectation.name) };

        ASSERT_NE(node, nullptr) << "The shipped source must declare " << expectation.name;

        const MAREA_SemaTypeLayout *layout{ nullptr };

        ASSERT_EQ(MAREA_SemaTypedNodeGetLayout(node, &layout), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must have a layout.";

        MAREA_Uinteger64 size{ 0 };

        MAREA_Uinteger64 alignment{ 0 };

        ASSERT_EQ(MAREA_SemaTypeLayoutGetSize(layout, &size), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must be measured.";

        ASSERT_EQ(MAREA_SemaTypeLayoutGetAlignment(layout, &alignment), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must be measured.";

        EXPECT_EQ(size, expectation.size) << "The size of " << expectation.name;

        EXPECT_EQ(alignment, expectation.size) << "The alignment of " << expectation.name;

        for (std::size_t lane{ 0 }; lane < expectation.lanes; ++lane) {
            MAREA_Uinteger64 offset{ 0 };

            ASSERT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(layout, lane, &offset), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must have lane " << lane;

            EXPECT_EQ(offset, lane * expectation.laneSize) << "The place of lane " << lane << " of " << expectation.name;
        }
    }
}

TEST(MAREASemaMathLibrary, ShippedSourceDeclaresTheOperatorsOfEachVectorType)
{
    GTEST_COUT << "Validating each vector type of the shipped source has its operator functions." << std::endl;

    const std::string SOURCE{ MAREA_mathSource() };

    ASSERT_FALSE(SOURCE.empty()) << "The shipped source must exist at " << MAREA_mathSourcePath();

    for (const MAREA_MathTypeExpectation &expectation : MATH_TYPES) {
        const std::string BODY{ MAREA_implementationOf(SOURCE, expectation.name) };

        ASSERT_FALSE(BODY.empty()) << expectation.name << " must have an implementation block.";

        std::vector<std::string> tokens{ "==", "!=", "[]" };

        if (std::string(expectation.name).find("Boolean") == std::string::npos) {
            tokens.insert(tokens.end(), { "+", "-", "*", "/" });
        }

        for (const std::string &token : tokens) {
            EXPECT_NE(BODY.find("#[operator = \"" + token + "\"]"), std::string::npos) << expectation.name << " must declare the operator " << token;
        }
    }
}
