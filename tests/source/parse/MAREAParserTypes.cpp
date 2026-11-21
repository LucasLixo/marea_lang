// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

namespace
{

    std::string MAREA_typeDump(MAREA_ParseFixture &fixture, const std::string &typeText)
    {
        EXPECT_EQ(fixture.MAREA_parse("namespace main; constant x: " + typeText + " = 1;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

        std::istringstream stream(fixture.MAREA_itemsDump());

        std::vector<std::string> lines;

        std::string line;

        while (std::getline(stream, line)) {
            lines.push_back(line);
        }

        if (lines.size() < 4) {
            return fixture.MAREA_itemsDump();
        }

        std::string result;

        for (size_t index{ 2 }; index + 1 < lines.size(); ++index) {
            result += lines[index].substr(std::min<size_t>(4, lines[index].size())) + "\n";
        }

        return result;
    }

} // namespace

TEST(MAREAParserTypes, PrimitiveName)
{
    GTEST_COUT << "Validating a primitive type name." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(MAREA_typeDump(fixture, "integer32"), MAREA_lines({ "NAMED_TYPE", "  QUALIFIED_NAME", "    IDENTIFIER \"integer32\"" })) << "A plain name must be a NAMED_TYPE.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserTypes, QualifiedName)
{
    GTEST_COUT << "Validating a qualified type name." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(MAREA_typeDump(fixture, "a.b.Point"), MAREA_lines({ "NAMED_TYPE", "  QUALIFIED_NAME", "    IDENTIFIER \"a\"", "    IDENTIFIER \"b\"", "    IDENTIFIER \"Point\"" })) << "Every part must be an identifier.";
}

TEST(MAREAParserTypes, TypeArguments)
{
    GTEST_COUT << "Validating type arguments with a type, two types and an integer." << std::endl;

    {
        MAREA_ParseFixture fixture;

        EXPECT_EQ(MAREA_typeDump(fixture, "array<float32>"), MAREA_lines({ "NAMED_TYPE", "  QUALIFIED_NAME", "    IDENTIFIER \"array\"", "  TYPE_ARGUMENTS", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"float32\"" })) << "One type argument.";
    }

    {
        MAREA_ParseFixture fixture;

        EXPECT_EQ(MAREA_typeDump(fixture, "vector<float32, 4>"), MAREA_lines({ "NAMED_TYPE", "  QUALIFIED_NAME", "    IDENTIFIER \"vector\"", "  TYPE_ARGUMENTS", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"float32\"", "    INTEGER_LITERAL \"4\"" })) << "An integer type argument.";
    }

    {
        MAREA_ParseFixture fixture;

        const std::string DUMP{ MAREA_typeDump(fixture, "table<string, integer32>") };

        EXPECT_NE(DUMP.find("IDENTIFIER \"string\""), std::string::npos) << "The key type must be present.";

        EXPECT_NE(DUMP.find("IDENTIFIER \"integer32\""), std::string::npos) << "The value type must be present.";

        EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
    }
}

TEST(MAREAParserTypes, NestedTypeArgumentsSplitShiftRight)
{
    GTEST_COUT << "Validating a shift right token closes two type argument lists." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "NAMED_TYPE", "  QUALIFIED_NAME", "    IDENTIFIER \"array\"", "  TYPE_ARGUMENTS", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"array\"", "      TYPE_ARGUMENTS", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"float32\"" }) };

    EXPECT_EQ(MAREA_typeDump(fixture, "array<array<float32>>"), EXPECTED) << "The two lists must nest.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected for the split.";
}

TEST(MAREAParserTypes, FunctionTypes)
{
    GTEST_COUT << "Validating function types with and without a result, and parameter flags." << std::endl;

    {
        MAREA_ParseFixture fixture;

        EXPECT_EQ(MAREA_typeDump(fixture, "function()"), MAREA_lines({ "FUNCTION_TYPE", "  PARAMETER_LIST" })) << "An empty function type.";
    }

    {
        MAREA_ParseFixture fixture;

        const std::string EXPECTED{ MAREA_lines({ "FUNCTION_TYPE", "  PARAMETER_LIST", "    PARAMETER", "      IDENTIFIER \"a\"", "      NAMED_TYPE", "        QUALIFIED_NAME", "          IDENTIFIER \"integer32\"", "  NAMED_TYPE", "    QUALIFIED_NAME", "      IDENTIFIER \"integer32\"" }) };

        EXPECT_EQ(MAREA_typeDump(fixture, "function(a: integer32): integer32"), EXPECTED) << "A function type with a result.";
    }

    {
        MAREA_ParseFixture fixture;

        const std::string DUMP{ MAREA_typeDump(fixture, "function(a: mutable integer32, b: reference integer32)") };

        EXPECT_NE(DUMP.find("PARAMETER flags=1\n"), std::string::npos) << "mutable must set the MUTABLE flag.";

        EXPECT_NE(DUMP.find("PARAMETER flags=2\n"), std::string::npos) << "reference must set the REFERENCE flag.";
    }
}

TEST(MAREAParserTypes, MissingClosingAngleBracket)
{
    GTEST_COUT << "Validating a missing closing angle bracket." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; constant x: array<float32 = 1;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "EXPECTED_TOKEN is expected and nothing more.";
}

TEST(MAREAParserTypes, MissingType)
{
    GTEST_COUT << "Validating a missing type gives EXPECTED_TYPE." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; constant x: = 1;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TYPE }) << "EXPECTED_TYPE is expected and nothing more.";
}
