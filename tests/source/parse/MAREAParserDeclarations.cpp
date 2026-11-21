// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

namespace
{

    std::string MAREA_itemsOf(MAREA_ParseFixture &fixture, const std::string &body)
    {
        EXPECT_EQ(fixture.MAREA_parse("namespace main; " + body), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

        return fixture.MAREA_itemsDump();
    }

} // namespace

TEST(MAREAParserDeclarations, Constant)
{
    GTEST_COUT << "Validating a constant declaration." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  CONSTANT_DECLARATION", "    IDENTIFIER \"MAX\"", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"integer32\"", "    INTEGER_LITERAL \"10\"" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "constant MAX: integer32 = 10;"), EXPECTED) << "A constant holds a name, a type and a value.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserDeclarations, TypeAlias)
{
    GTEST_COUT << "Validating a generic type alias." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  TYPE_DECLARATION", "    IDENTIFIER \"Ints\"", "    GENERIC_PARAMETERS", "      GENERIC_PARAMETER", "        IDENTIFIER \"T\"", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"array\"", "      TYPE_ARGUMENTS", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"T\"" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "type Ints<T> = array<T>;"), EXPECTED) << "An alias holds a name, generic parameters and a type.";
}

TEST(MAREAParserDeclarations, StructureWithAttributes)
{
    GTEST_COUT << "Validating a structure with an attribute and a field attribute." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  STRUCTURE_DECLARATION", "    ATTRIBUTE", "      ATTRIBUTE_ITEM", "        IDENTIFIER \"value\"", "    IDENTIFIER \"Point\"", "    FIELD_DECLARATION", "      IDENTIFIER \"x\"", "      NAMED_TYPE", "        QUALIFIED_NAME", "          IDENTIFIER \"float32\"", "    FIELD_DECLARATION", "      ATTRIBUTE", "        ATTRIBUTE_ITEM", "          IDENTIFIER \"hidden\"", "      IDENTIFIER \"y\"", "      NAMED_TYPE", "        QUALIFIED_NAME", "          IDENTIFIER \"float32\"" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "#[value] structure Point x: float32; #[hidden] y: float32; end"), EXPECTED) << "Attributes lead the node that owns them.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserDeclarations, EmptyStructureAndOpaque)
{
    GTEST_COUT << "Validating an empty structure and an opaque structure." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  STRUCTURE_DECLARATION", "    IDENTIFIER \"Empty\"", "  OPAQUE_DECLARATION", "    IDENTIFIER \"Handle\"" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "structure Empty end opaque structure Handle;"), EXPECTED) << "Both must parse.";
}

TEST(MAREAParserDeclarations, Enumeration)
{
    GTEST_COUT << "Validating an enum with plain and field variants." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  ENUM_DECLARATION", "    IDENTIFIER \"Shape\"", "    VARIANT_DECLARATION", "      IDENTIFIER \"Dot\"", "    VARIANT_DECLARATION", "      IDENTIFIER \"Circle\"", "      VARIANT_FIELD", "        IDENTIFIER \"radius\"", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"float32\"", "        FLOAT_LITERAL \"1.0\"" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "enum Shape Dot; Circle(radius: float32 = 1.0); end"), EXPECTED) << "A variant field may have a default.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserDeclarations, Contract)
{
    GTEST_COUT << "Validating a contract with functions with and without a result." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  CONTRACT_DECLARATION", "    IDENTIFIER \"Shape\"", "    CONTRACT_FUNCTION", "      IDENTIFIER \"area\"", "      PARAMETER_LIST", "        PARAMETER", "          IDENTIFIER \"self\"", "          NAMED_TYPE", "            QUALIFIED_NAME", "              IDENTIFIER \"Shape\"", "      NAMED_TYPE", "        QUALIFIED_NAME", "          IDENTIFIER \"float32\"", "    CONTRACT_FUNCTION", "      IDENTIFIER \"reset\"", "      PARAMETER_LIST" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "contract Shape function area(self: Shape): float32; function reset(); end"), EXPECTED) << "The parameter list is always present.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserDeclarations, Implementation)
{
    GTEST_COUT << "Validating an implementation with generics, a function and a constant." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  IMPLEMENTATION_DECLARATION", "    GENERIC_PARAMETERS", "      GENERIC_PARAMETER", "        IDENTIFIER \"T\"", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"array\"", "      TYPE_ARGUMENTS", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"T\"", "    FUNCTION_DECLARATION", "      ATTRIBUTE", "        ATTRIBUTE_ITEM", "          IDENTIFIER \"inline\"", "          QUALIFIED_NAME", "            IDENTIFIER \"force\"", "      IDENTIFIER \"len\"", "      PARAMETER_LIST", "      NAMED_TYPE", "        QUALIFIED_NAME", "          IDENTIFIER \"integer32\"", "      BLOCK", "    CONSTANT_DECLARATION", "      IDENTIFIER \"ZERO\"", "      NAMED_TYPE", "        QUALIFIED_NAME", "          IDENTIFIER \"integer32\"", "      INTEGER_LITERAL \"0\"" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "implementation<T> array<T> #[inline = force] function len(): integer32 end constant ZERO: integer32 = 0; end"), EXPECTED) << "Members keep their own attributes.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserDeclarations, FunctionWithGenericsAndWhere)
{
    GTEST_COUT << "Validating a function with generics, flags, a result and a where clause." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string EXPECTED{ MAREA_lines({ "  FUNCTION_DECLARATION", "    IDENTIFIER \"max\"", "    GENERIC_PARAMETERS", "      GENERIC_PARAMETER", "        IDENTIFIER \"T\"", "      GENERIC_PARAMETER flags=4", "        IDENTIFIER \"N\"", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"integer32\"", "    PARAMETER_LIST", "      PARAMETER", "        IDENTIFIER \"a\"", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"T\"", "      PARAMETER flags=1", "        IDENTIFIER \"b\"", "        NAMED_TYPE", "          QUALIFIED_NAME", "            IDENTIFIER \"T\"", "    NAMED_TYPE", "      QUALIFIED_NAME", "        IDENTIFIER \"T\"", "    WHERE_CLAUSE", "      CONSTRAINT", "        IDENTIFIER \"T\"", "        QUALIFIED_NAME", "          IDENTIFIER \"Comparable\"", "        QUALIFIED_NAME", "          IDENTIFIER \"Printable\"", "      CONSTRAINT", "        IDENTIFIER \"N\"", "        QUALIFIED_NAME", "          IDENTIFIER \"Sized\"", "    BLOCK" }) };

    EXPECT_EQ(MAREA_itemsOf(fixture, "function max<T, constant N: integer32>(a: T, b: mutable T): T where T: Comparable, Printable and N: Sized end"), EXPECTED) << "The function must hold every part in grammar order.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";
}

TEST(MAREAParserDeclarations, AttributesBeforeEveryKind)
{
    GTEST_COUT << "Validating an attribute list before every kind of item." << std::endl;

    const std::vector<std::pair<std::string, std::string>> CASES{
        { "#[a] constant x: integer32 = 1;", "  CONSTANT_DECLARATION" },
        { "#[a] type T = integer32;", "  TYPE_DECLARATION" },
        { "#[a] structure S end", "  STRUCTURE_DECLARATION" },
        { "#[a] opaque structure O;", "  OPAQUE_DECLARATION" },
        { "#[a] enum E end", "  ENUM_DECLARATION" },
        { "#[a] contract C end", "  CONTRACT_DECLARATION" },
        { "#[a] implementation S end", "  IMPLEMENTATION_DECLARATION" },
        { "#[a] function f() end", "  FUNCTION_DECLARATION" },
    };

    for (const std::pair<std::string, std::string> &entry : CASES) {
        MAREA_ParseFixture fixture;

        const std::string DUMP{ MAREA_itemsOf(fixture, entry.first) };

        EXPECT_EQ(DUMP.compare(0, entry.second.size() + 1, entry.second + "\n"), 0) << "The item of '" << entry.first << "' must be " << entry.second << ".";

        EXPECT_NE(DUMP.find("    ATTRIBUTE\n"), std::string::npos) << "The attribute of '" << entry.first << "' must lead the item.";

        EXPECT_TRUE(fixture.MAREA_codes().empty()) << "'" << entry.first << "' must give no diagnostic.";
    }
}

TEST(MAREAParserDeclarations, MissingEndAtEndOfFile)
{
    GTEST_COUT << "Validating a missing end gives UNTERMINATED_BLOCK." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; structure P x: float32;"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK }) << "UNTERMINATED_BLOCK is expected.";
}

TEST(MAREAParserDeclarations, MissingSemicolonAfterFieldKeepsParsing)
{
    GTEST_COUT << "Validating a missing semicolon after a field does not hide the next field." << std::endl;

    MAREA_ParseFixture fixture;

    const std::string DUMP{ MAREA_itemsOf(fixture, "structure P x: float32 y: float32; end") };

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "One EXPECTED_TOKEN is expected.";

    EXPECT_NE(DUMP.find("      IDENTIFIER \"y\"\n"), std::string::npos) << "The second field must still be parsed.";
}

TEST(MAREAParserDeclarations, BadMemberInsideBlocksIsSkipped)
{
    GTEST_COUT << "Validating a bad token inside a structure and an implementation is skipped." << std::endl;

    {
        MAREA_ParseFixture fixture;

        const std::string DUMP{ MAREA_itemsOf(fixture, "structure P 42; x: float32; end") };

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "The bad member must be reported.";

        EXPECT_NE(DUMP.find("IDENTIFIER \"x\""), std::string::npos) << "The field after the bad token must be parsed.";
    }

    {
        MAREA_ParseFixture fixture;

        std::string ignored{ MAREA_itemsOf(fixture, "implementation S 42; function f() end end") };

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "The bad member must be reported.";

        EXPECT_NE(ignored.find("FUNCTION_DECLARATION"), std::string::npos) << "The function after the bad token must be parsed.";
    }
}
