// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

#include <utility>

namespace
{

    using MAREA_Case = std::pair<const char *, const char *>;

    void MAREA_expectCases(const std::vector<MAREA_Case> &cases)
    {
        for (const MAREA_Case &entry : cases) {
            MAREA_ParseFixture fixture;

            EXPECT_EQ(fixture.MAREA_expression(entry.first), entry.second) << "The expression '" << entry.first << "' must have this shape.";

            EXPECT_TRUE(fixture.MAREA_codes().empty()) << "The expression '" << entry.first << "' must give no diagnostic.";
        }
    }

} // namespace

TEST(MAREAParserExpressions, Literals)
{
    GTEST_COUT << "Validating every literal kind, a name and parentheses." << std::endl;

    MAREA_expectCases({
        { "1", "1" },
        { "0xFF", "0xFF" },
        { "1.5", "1.5" },
        { "\"s\"", "\"s\"" },
        { "'a'", "'a'" },
        { "'\\n'", "'\\n'" },
        { "true", "true" },
        { "false", "false" },
        { "a", "a" },
        { "(a)", "(parenthesized a)" },
    });
}

TEST(MAREAParserExpressions, EveryBinaryOperator)
{
    GTEST_COUT << "Validating every binary operator builds a binary node with its token kind." << std::endl;

    MAREA_expectCases({
        { "a or b", "(binary:KEYWORD_OR a b)" },
        { "a and b", "(binary:KEYWORD_AND a b)" },
        { "a == b", "(binary:EQUAL_EQUAL a b)" },
        { "a != b", "(binary:BANG_EQUAL a b)" },
        { "a < b", "(binary:LESS a b)" },
        { "a <= b", "(binary:LESS_EQUAL a b)" },
        { "a > b", "(binary:GREATER a b)" },
        { "a >= b", "(binary:GREATER_EQUAL a b)" },
        { "a | b", "(binary:PIPE a b)" },
        { "a ^ b", "(binary:CARET a b)" },
        { "a & b", "(binary:AMPERSAND a b)" },
        { "a << b", "(binary:SHIFT_LEFT a b)" },
        { "a >> b", "(binary:SHIFT_RIGHT a b)" },
        { "a + b", "(binary:PLUS a b)" },
        { "a - b", "(binary:MINUS a b)" },
        { "a +% b", "(binary:PLUS_PERCENT a b)" },
        { "a -% b", "(binary:MINUS_PERCENT a b)" },
        { "a +| b", "(binary:PLUS_PIPE a b)" },
        { "a -| b", "(binary:MINUS_PIPE a b)" },
        { "a * b", "(binary:STAR a b)" },
        { "a / b", "(binary:SLASH a b)" },
        { "a % b", "(binary:PERCENT a b)" },
        { "a *% b", "(binary:STAR_PERCENT a b)" },
        { "a *| b", "(binary:STAR_PIPE a b)" },
        { "a ** b", "(binary:STAR_STAR a b)" },
    });
}

TEST(MAREAParserExpressions, Precedence)
{
    GTEST_COUT << "Validating precedence and association of the binary levels." << std::endl;

    MAREA_expectCases({
        { "a + b * c", "(binary:PLUS a (binary:STAR b c))" },
        { "a * b + c", "(binary:PLUS (binary:STAR a b) c)" },
        { "a or b and c", "(binary:KEYWORD_OR a (binary:KEYWORD_AND b c))" },
        { "a | b ^ c & d", "(binary:PIPE a (binary:CARET b (binary:AMPERSAND c d)))" },
        { "a << b + c", "(binary:SHIFT_LEFT a (binary:PLUS b c))" },
        { "a == b and c", "(binary:KEYWORD_AND (binary:EQUAL_EQUAL a b) c)" },
        { "a < b | c", "(binary:LESS a (binary:PIPE b c))" },
        { "a - b - c", "(binary:MINUS (binary:MINUS a b) c)" },
        { "a * b ** c", "(binary:STAR a (binary:STAR_STAR b c))" },
    });
}

TEST(MAREAParserExpressions, PowerAndUnary)
{
    GTEST_COUT << "Validating the power is right to left and stronger than the unary operators." << std::endl;

    MAREA_expectCases({
        { "a ** b ** c", "(binary:STAR_STAR a (binary:STAR_STAR b c))" },
        { "-a ** 2", "(unary:MINUS (binary:STAR_STAR a 2))" },
        { "2 ** -1", "(binary:STAR_STAR 2 (unary:MINUS 1))" },
        { "not a and b", "(binary:KEYWORD_AND (unary:KEYWORD_NOT a) b)" },
        { "~a", "(unary:TILDE a)" },
        { "- - a", "(unary:MINUS (unary:MINUS a))" },
    });
}

TEST(MAREAParserExpressions, CopyMoveSpawn)
{
    GTEST_COUT << "Validating copy, move and spawn take a postfix operand." << std::endl;

    MAREA_expectCases({
        { "copy a.b", "(unary:KEYWORD_COPY (member_access a b))" },
        { "move a", "(unary:KEYWORD_MOVE a)" },
        { "spawn f(1)", "(unary:KEYWORD_SPAWN (call f (argument_list 1)))" },
    });
}

TEST(MAREAParserExpressions, PostfixChains)
{
    GTEST_COUT << "Validating member access, index access and calls." << std::endl;

    MAREA_expectCases({
        { "a.b[1].c", "(member_access (index_access (member_access a b) 1) c)" },
        { "f()", "(call f (argument_list))" },
        { "f(1, 2)", "(call f (argument_list 1 2))" },
        { "f(g(1))", "(call f (argument_list (call g (argument_list 1))))" },
        { "a.b.c(1)", "(call (member_access (member_access a b) c) (argument_list 1))" },
        { "f(1)(2)", "(call (call f (argument_list 1)) (argument_list 2))" },
    });
}

TEST(MAREAParserExpressions, GenericsVersusComparison)
{
    GTEST_COUT << "Validating the rule that decides type arguments against a comparison." << std::endl;

    MAREA_expectCases({
        { "f<sinteger32>(x)", "(call f (type_arguments (named_type sinteger32)) (argument_list x))" },
        { "a < b and c > d", "(binary:KEYWORD_AND (binary:LESS a b) (binary:GREATER c d))" },
        { "a < b > (c)", "(call a (type_arguments (named_type b)) (argument_list c))" },
        { "size_of<layout.Light>() == 32", "(binary:EQUAL_EQUAL (call size_of (type_arguments (named_type layout.Light)) (argument_list)) 32)" },
        { "array<float32>.filled(0.0)", "(call (member_access (generic_name array (type_arguments (named_type float32))) filled) (argument_list 0.0))" },
        { "a != (b)", "(binary:BANG_EQUAL a (parenthesized b))" },
    });
}

TEST(MAREAParserExpressions, LiteralValues)
{
    GTEST_COUT << "Validating literal values of every entry kind." << std::endl;

    MAREA_expectCases({
        { "Point{ x = 1.0, y = 2.0 }", "(literal_value (named_type Point) (field_entry x 1.0) (field_entry y 2.0))" },
        { "table<string, sinteger32>{ [\"a\"] = 1 }", "(literal_value (named_type table (type_arguments (named_type string) (named_type sinteger32))) (index_entry \"a\" 1))" },
        { "array<float32>{ 1.0, 2.0 }", "(literal_value (named_type array (type_arguments (named_type float32))) (value_entry 1.0) (value_entry 2.0))" },
        { "array<float32, 4>{ 1.0, 0.0 }", "(literal_value (named_type array (type_arguments (named_type float32) 4)) (value_entry 1.0) (value_entry 0.0))" },
        { "Point{ x = 1, }", "(literal_value (named_type Point) (field_entry x 1))" },
        { "Point{}", "(literal_value (named_type Point))" },
        { "ns.Point{ x = 1 }", "(literal_value (named_type ns.Point) (field_entry x 1))" },
        { "table<string, array<sinteger32>>{}", "(literal_value (named_type table (type_arguments (named_type string) (named_type array (type_arguments (named_type sinteger32))))))" },
        { "Point{ a == b }", "(literal_value (named_type Point) (value_entry (binary:EQUAL_EQUAL a b)))" },
    });
}

TEST(MAREAParserExpressions, MacroCalls)
{
    GTEST_COUT << "Validating macro calls, including the namespace macro." << std::endl;

    MAREA_expectCases({
        { "size_of!(Point)", "(macro_call size_of (argument_list Point))" },
        { "line!()", "(macro_call line (argument_list))" },
        { "namespace!()", "(macro_call namespace (argument_list))" },
        { "format!(\"{}\", a, b)", "(macro_call format (argument_list \"{}\" a b))" },
    });
}

TEST(MAREAParserExpressions, ChainedComparisonIsReported)
{
    GTEST_COUT << "Validating a chained comparison gives CHAINED_COMPARISON and still builds the tree." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(fixture.MAREA_expression("a < b < c"), "(binary:LESS (binary:LESS a b) c)") << "The tree must still be built, left to right.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_CHAINED_COMPARISON }) << "CHAINED_COMPARISON is expected and nothing more.";
}

TEST(MAREAParserExpressions, MissingOperandAndParenthesis)
{
    GTEST_COUT << "Validating a missing operand and a missing closing parenthesis." << std::endl;

    {
        MAREA_ParseFixture fixture;

        MAREA_IGNORE(fixture.MAREA_expression("1 +"));

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION }) << "EXPECTED_EXPRESSION is expected and nothing more.";
    }

    {
        MAREA_ParseFixture fixture;

        MAREA_IGNORE(fixture.MAREA_expression("f(1, 2"));

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "EXPECTED_TOKEN is expected and nothing more.";
    }

    {
        MAREA_ParseFixture fixture;

        MAREA_IGNORE(fixture.MAREA_expression("a."));

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "A dot without a member name must be reported.";
    }
}

TEST(MAREAParserExpressions, DeepNestingIsLimited)
{
    GTEST_COUT << "Validating deep nesting gives RECURSION_TOO_DEEP and never crashes." << std::endl;

    {
        MAREA_ParseFixture fixture;

        const std::string SOURCE{ std::string(300, '(') + "1" + std::string(300, ')') };

        MAREA_IGNORE(fixture.MAREA_expression(SOURCE));

        ASSERT_FALSE(fixture.MAREA_codes().empty()) << "300 nested parentheses must be reported.";

        EXPECT_EQ(fixture.MAREA_codes()[0], static_cast<MAREA_Uinteger32>(MAREA_PARSE_DIAGNOSTIC_CODE_RECURSION_TOO_DEEP)) << "The first diagnostic must be RECURSION_TOO_DEEP.";
    }

    {
        MAREA_ParseFixture fixture;

        std::string source;

        for (int index{ 0 }; index < 300; ++index) {
            source += "- ";
        }

        MAREA_IGNORE(fixture.MAREA_expression(source + "1"));

        ASSERT_FALSE(fixture.MAREA_codes().empty()) << "300 nested unary operators must be reported.";

        EXPECT_EQ(fixture.MAREA_codes()[0], static_cast<MAREA_Uinteger32>(MAREA_PARSE_DIAGNOSTIC_CODE_RECURSION_TOO_DEEP)) << "The first diagnostic must be RECURSION_TOO_DEEP.";
    }
}
