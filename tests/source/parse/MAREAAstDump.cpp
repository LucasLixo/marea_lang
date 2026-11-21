// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/internal/parse/MAREAParseInternal.h"

#include <deque>
#include <string>

namespace
{

    struct MAREA_DumpFixture
    {
        MAREA_Arena *arena{ nullptr };

        MAREA_DumpFixture()
        {
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
        }

        ~MAREA_DumpFixture()
        {
            MAREA_ArenaDestroy(arena);
        }

        MAREA_DumpFixture(const MAREA_DumpFixture &) = delete;
        MAREA_DumpFixture &operator=(const MAREA_DumpFixture &) = delete;

        MAREA_AstNode *MAREA_node(MAREA_AstKind kind, const std::string &text = std::string())
        {
            MAREA_AstNode *node{ nullptr };

            const MAREA_SourcePosition POSITION{ "<test>", 0, 1, 1, 0 };

            ownedTexts.push_back(text);

            const std::string &stored{ ownedTexts.back() };

            MAREA_IGNORE(MAREA_AstNodeCreate(arena, kind, POSITION, stored.empty() ? nullptr : stored.c_str(), stored.size(), &node));

            return node;
        }

        void MAREA_add(MAREA_AstNode *parent, MAREA_AstNode *child)
        {
            MAREA_IGNORE(MAREA_AstNodeAppendChild(arena, parent, child));
        }

        std::string MAREA_dump(const MAREA_AstNode *root)
        {
            const MAREA_Character8 *text{ nullptr };

            MAREA_IntegerSize length{ 0 };

            MAREA_IGNORE(MAREA_AstDump(arena, root, &text, &length));

            return std::string(text, length);
        }

        std::deque<std::string> ownedTexts;
    };

} // namespace

TEST(MAREAAstDump, NamespaceOnlyFile)
{
    GTEST_COUT << "Validating the documented dump of a file with only a namespace." << std::endl;

    MAREA_DumpFixture fixture;

    MAREA_AstNode *root{ fixture.MAREA_node(MAREA_AST_KIND_COMPILATION_UNIT) };
    MAREA_AstNode *declaration{ fixture.MAREA_node(MAREA_AST_KIND_NAMESPACE_DECLARATION) };
    MAREA_AstNode *name{ fixture.MAREA_node(MAREA_AST_KIND_QUALIFIED_NAME) };
    MAREA_AstNode *part{ fixture.MAREA_node(MAREA_AST_KIND_IDENTIFIER, "main") };

    fixture.MAREA_add(name, part);
    fixture.MAREA_add(declaration, name);
    fixture.MAREA_add(root, declaration);

    EXPECT_EQ(fixture.MAREA_dump(root), "COMPILATION_UNIT\n  NAMESPACE_DECLARATION\n    QUALIFIED_NAME\n      IDENTIFIER \"main\"\n") << "The dump must match the format of the apply index.";
}

TEST(MAREAAstDump, OperatorAndFlags)
{
    GTEST_COUT << "Validating op= and flags= are written only when set." << std::endl;

    MAREA_DumpFixture fixture;

    MAREA_AstNode *binary{ fixture.MAREA_node(MAREA_AST_KIND_BINARY_EXPRESSION) };

    MAREA_AstNodeSetOperator(binary, MAREA_TOKEN_KIND_STAR_STAR);

    MAREA_AstNode *parameter{ fixture.MAREA_node(MAREA_AST_KIND_PARAMETER) };

    MAREA_AstNodeSetFlags(parameter, MAREA_AST_FLAG_MUTABLE);

    EXPECT_EQ(fixture.MAREA_dump(binary), "BINARY_EXPRESSION op=STAR_STAR\n") << "An operator must be written as op=NAME.";

    EXPECT_EQ(fixture.MAREA_dump(parameter), "PARAMETER flags=1\n") << "Flags must be written as flags=N.";
}

TEST(MAREAAstDump, TextIsEscaped)
{
    GTEST_COUT << "Validating quote, backslash and non printable bytes are escaped." << std::endl;

    MAREA_DumpFixture fixture;

    MAREA_AstNode *literal{ fixture.MAREA_node(MAREA_AST_KIND_STRING_LITERAL, std::string("\"a\\\\b\"\t\x7f")) };

    const std::string EXPECTED{ std::string(R"(STRING_LITERAL "\"a\\\\b\"\x09\x7F")") + "\n" };

    EXPECT_EQ(fixture.MAREA_dump(literal), EXPECTED) << "The escaping rule of the index must apply.";
}

TEST(MAREAAstDump, RejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_AstDump rejects null arguments." << std::endl;

    MAREA_DumpFixture fixture;

    MAREA_AstNode *root{ fixture.MAREA_node(MAREA_AST_KIND_BLOCK) };

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_AstDump(nullptr, root, &text, &length), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_AstDump(fixture.arena, nullptr, &text, &length), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null root must be rejected.";

    EXPECT_EQ(MAREA_AstDump(fixture.arena, root, nullptr, &length), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null text pointer must be rejected.";

    EXPECT_EQ(MAREA_AstDump(fixture.arena, root, &text, nullptr), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null length pointer must be rejected.";
}

TEST(MAREAAstDump, ChildrenAreReadInOrder)
{
    GTEST_COUT << "Validating children keep their order through the public getters." << std::endl;

    MAREA_DumpFixture fixture;

    MAREA_AstNode *block{ fixture.MAREA_node(MAREA_AST_KIND_BLOCK) };

    for (MAREA_Sinteger32 index{ 0 }; index < 100; ++index) {
        fixture.MAREA_add(block, fixture.MAREA_node((index % 2 == 0) ? MAREA_AST_KIND_BREAK_STATEMENT : MAREA_AST_KIND_CONTINUE_STATEMENT));
    }

    ASSERT_EQ(MAREA_AstNodeGetChildCount(block), MAREA_INTEGER_SIZE(100)) << "The block must hold 100 children.";

    const MAREA_AstNode *child{ nullptr };

    ASSERT_EQ(MAREA_AstNodeGetChildAt(block, MAREA_INTEGER_SIZE(3), &child), MAREA_AST_EXCEPT_SUCCESS) << "Index 3 must exist.";

    EXPECT_EQ(MAREA_AstNodeGetKind(child), MAREA_AST_KIND_CONTINUE_STATEMENT) << "Child 3 must be the fourth appended node.";

    EXPECT_EQ(MAREA_AstNodeGetChildAt(block, MAREA_INTEGER_SIZE(100), &child), MAREA_AST_EXCEPT_ERROR_OUT_OF_RANGE) << "An index equal to the count must be rejected.";
}
