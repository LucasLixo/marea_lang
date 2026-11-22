// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

#include <string>

TEST(MAREAAstDump, NamespaceOnlyFile)
{
    GTEST_COUT << "Validating the documented dump of a file with only a namespace." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_dump(), "COMPILATION_UNIT\n  NAMESPACE_DECLARATION\n    QUALIFIED_NAME\n      IDENTIFIER \"main\"\n") << "The dump must match the format of the docs.";
}

TEST(MAREAAstDump, OperatorAndFlags)
{
    GTEST_COUT << "Validating op= and flags= are written only when set." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; constant x: integer32 = a ** b; function f(p: mutable integer32) end"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const std::string DUMP{ fixture.MAREA_itemsDump() };

    EXPECT_NE(DUMP.find("    BINARY_EXPRESSION op=STAR_STAR\n"), std::string::npos) << "An operator must be written as op=NAME.";

    EXPECT_NE(DUMP.find("      PARAMETER flags=1\n"), std::string::npos) << "Flags must be written as flags=N.";

    EXPECT_EQ(DUMP.find("QUALIFIED_NAME op="), std::string::npos) << "A node without an operator must not write op=.";

    EXPECT_EQ(DUMP.find("QUALIFIED_NAME flags="), std::string::npos) << "A node without flags must not write flags=.";
}

TEST(MAREAAstDump, ImportMemberList)
{
    GTEST_COUT << "Validating the dump of an import member list names the kind and writes the flags." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; import a.b { f as g, T };"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const std::string EXPECTED{ MAREA_lines({ "  IMPORT_DECLARATION flags=32", "    QUALIFIED_NAME", "      IDENTIFIER \"a\"", "      IDENTIFIER \"b\"", "    IMPORT_MEMBER flags=8", "      QUALIFIED_NAME", "        IDENTIFIER \"f\"", "      IDENTIFIER \"g\"", "    IMPORT_MEMBER", "      QUALIFIED_NAME", "        IDENTIFIER \"T\"" }) };

    EXPECT_EQ(fixture.MAREA_itemsDump(), EXPECTED) << "The dump must show HAS_MEMBERS as flags=32 and HAS_ALIAS as flags=8.";
}

TEST(MAREAAstDump, TextIsEscaped)
{
    GTEST_COUT << "Validating quote, backslash and non printable bytes are escaped." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string SOURCE{ std::string("namespace main; constant s: string = \"a\\\\b\\\"") + "\x01" + "\x7f\";" };

    ASSERT_EQ(fixture.MAREA_parse(SOURCE), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed even with a bad byte in the string.";

    const std::string EXPECTED{ std::string(R"(    STRING_LITERAL "\"a\\\\b\\\"\x01\x7F\"")") + "\n" };

    EXPECT_NE(fixture.MAREA_itemsDump().find(EXPECTED), std::string::npos) << "The escaping rule of the docs must apply.";
}

TEST(MAREAAstDump, RejectsNullArguments)
{
    GTEST_COUT << "Validating MAREA_AstDump rejects null arguments." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_AstDump(nullptr, fixture.root, &text, &length), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_AstDump(fixture.lexer.arena, nullptr, &text, &length), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null root must be rejected.";

    EXPECT_EQ(MAREA_AstDump(fixture.lexer.arena, fixture.root, nullptr, &length), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null text pointer must be rejected.";

    EXPECT_EQ(MAREA_AstDump(fixture.lexer.arena, fixture.root, &text, nullptr), MAREA_AST_EXCEPT_ERROR_NULL_POINTER) << "A null length pointer must be rejected.";
}

TEST(MAREAAstDump, ChildrenAreReadInOrder)
{
    GTEST_COUT << "Validating 100 children keep their order through the public getters." << std::endl;

    MAREA_ParseFixture fixture;

    std::string source{ "namespace main; function f() " };

    for (int index{ 0 }; index < 100; ++index) {
        source += (index % 2 == 0) ? "break; " : "continue; ";
    }

    source += "end";

    ASSERT_EQ(fixture.MAREA_parse(source), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const MAREA_AstNode *function{ nullptr };

    const MAREA_AstNode *block{ nullptr };

    ASSERT_EQ(MAREA_AstNodeGetChildAt(fixture.root, 1, &function), MAREA_AST_EXCEPT_SUCCESS) << "The function must be the second child.";

    ASSERT_EQ(MAREA_AstNodeGetChildAt(function, 2, &block), MAREA_AST_EXCEPT_SUCCESS) << "The block must be the third child of the function.";

    ASSERT_EQ(MAREA_AstNodeGetChildCount(block), MAREA_INTEGER_SIZE(100)) << "The block must hold 100 children.";

    const MAREA_AstNode *child{ nullptr };

    ASSERT_EQ(MAREA_AstNodeGetChildAt(block, MAREA_INTEGER_SIZE(3), &child), MAREA_AST_EXCEPT_SUCCESS) << "Index 3 must exist.";

    EXPECT_EQ(MAREA_AstNodeGetKind(child), MAREA_AST_KIND_CONTINUE_STATEMENT) << "Child 3 must be the fourth statement.";

    EXPECT_EQ(MAREA_AstNodeGetChildAt(block, MAREA_INTEGER_SIZE(100), &child), MAREA_AST_EXCEPT_ERROR_OUT_OF_RANGE) << "An index equal to the count must be rejected.";
}
