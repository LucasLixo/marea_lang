// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

namespace
{

    const std::string MAREA_NAMESPACE_MAIN{ "COMPILATION_UNIT\n  NAMESPACE_DECLARATION\n    QUALIFIED_NAME\n      IDENTIFIER \"main\"\n" };

} // namespace

TEST(MAREAParserFile, NamespaceOnly)
{
    GTEST_COUT << "Validating a file with only a namespace." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_dump(), MAREA_NAMESPACE_MAIN) << "The tree must hold only the namespace.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "A clean file must give no diagnostic.";
}

TEST(MAREAParserFile, DottedNamespace)
{
    GTEST_COUT << "Validating a dotted namespace." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace a.b.c;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_dump(), "COMPILATION_UNIT\n  NAMESPACE_DECLARATION\n    QUALIFIED_NAME\n      IDENTIFIER \"a\"\n      IDENTIFIER \"b\"\n      IDENTIFIER \"c\"\n") << "Every part of the name must be an identifier child.";
}

TEST(MAREAParserFile, Imports)
{
    GTEST_COUT << "Validating imports with and without an alias." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; import a.b; import c as d;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const std::string EXPECTED{ MAREA_NAMESPACE_MAIN +
                                "  IMPORT_DECLARATION\n    QUALIFIED_NAME\n      IDENTIFIER \"a\"\n      IDENTIFIER \"b\"\n"
                                "  IMPORT_DECLARATION flags=8\n    QUALIFIED_NAME\n      IDENTIFIER \"c\"\n    IDENTIFIER \"d\"\n" };

    EXPECT_EQ(fixture.MAREA_dump(), EXPECTED) << "The alias must be the second child with HAS_ALIAS set.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "Clean imports must give no diagnostic.";
}

TEST(MAREAParserFile, FileAttribute)
{
    GTEST_COUT << "Validating a file attribute with every value kind." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; #![ build = debug, simd, flag = true, size = 4, name = \"x\" ]"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const std::string EXPECTED{ MAREA_NAMESPACE_MAIN +
                                "  FILE_ATTRIBUTE\n"
                                "    ATTRIBUTE_ITEM\n      IDENTIFIER \"build\"\n      QUALIFIED_NAME\n        IDENTIFIER \"debug\"\n"
                                "    ATTRIBUTE_ITEM\n      IDENTIFIER \"simd\"\n"
                                "    ATTRIBUTE_ITEM\n      IDENTIFIER \"flag\"\n      BOOLEAN_LITERAL \"true\"\n"
                                "    ATTRIBUTE_ITEM\n      IDENTIFIER \"size\"\n      INTEGER_LITERAL \"4\"\n"
                                "    ATTRIBUTE_ITEM\n      IDENTIFIER \"name\"\n      STRING_LITERAL \"\\\"x\\\"\"\n" };

    EXPECT_EQ(fixture.MAREA_dump(), EXPECTED) << "Every attribute item must carry its key and its optional value.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "A clean attribute must give no diagnostic.";
}

TEST(MAREAParserFile, EmptyInputMissesNamespace)
{
    GTEST_COUT << "Validating empty input gives MISSING_NAMESPACE and an empty root." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse(""), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_dump(), "COMPILATION_UNIT\n") << "The root must have no children.";

    ASSERT_EQ(fixture.MAREA_codes().size(), 1U) << "Exactly one diagnostic is expected.";

    EXPECT_EQ(fixture.MAREA_codes()[0], static_cast<MAREA_Uinteger32>(MAREA_PARSE_DIAGNOSTIC_CODE_MISSING_NAMESPACE)) << "The code must be MISSING_NAMESPACE.";
}

TEST(MAREAParserFile, ImportWithoutNamespaceKeepsParsing)
{
    GTEST_COUT << "Validating a file without a namespace still parses its imports." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("import a;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_dump(), "COMPILATION_UNIT\n  IMPORT_DECLARATION\n    QUALIFIED_NAME\n      IDENTIFIER \"a\"\n") << "The import must still be parsed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_MISSING_NAMESPACE }) << "Only MISSING_NAMESPACE is expected.";
}

TEST(MAREAParserFile, MissingSemicolonAfterNamespace)
{
    GTEST_COUT << "Validating a missing semicolon is reported at the next token." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_dump(), MAREA_NAMESPACE_MAIN) << "The namespace must still be in the tree.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "EXPECTED_TOKEN is expected.";

    EXPECT_EQ(fixture.MAREA_offsets(), std::vector<MAREA_Uinteger32>{ 14 }) << "The position must be the end of file, offset 14.";
}

TEST(MAREAParserFile, UnknownItemIsSkipped)
{
    GTEST_COUT << "Validating a bad top level token gives EXPECTED_DECLARATION and an ERROR_NODE." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; foo bar; import a;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const std::vector<MAREA_Uinteger32> EXPECTED{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_DECLARATION, MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_DECLARATION };

    EXPECT_EQ(fixture.MAREA_codes(), EXPECTED) << "The bad run and the import after the items give one diagnostic each.";

    EXPECT_NE(fixture.MAREA_dump().find("  ERROR_NODE\n"), std::string::npos) << "An ERROR_NODE must mark the skipped run.";
}

TEST(MAREAParserFile, StrayTerminatorIsUnexpected)
{
    GTEST_COUT << "Validating a stray end gives UNEXPECTED_TOKEN and the parser terminates." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_UNEXPECTED_TOKEN }) << "UNEXPECTED_TOKEN is expected.";
}

TEST(MAREAParserFile, LoneTokensTerminate)
{
    GTEST_COUT << "Validating end, a semicolon and a right parenthesis alone never loop." << std::endl;

    for (const char *source : { "end", ";", ")", "namespace main; ;", "namespace main; )", "namespace main; #[", "namespace main; #![ a", "import", "namespace" }) {
        MAREA_ParseFixture fixture;

        ASSERT_EQ(fixture.MAREA_parse(source), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing of '" << source << "' must succeed.";

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "Parsing of '" << source << "' must report at least one diagnostic.";
    }
}
