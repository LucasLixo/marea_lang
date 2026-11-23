// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const std::string POINT{ "structure Point x: sinteger32; y: sinteger32; end " };

    std::string MAREA_lowerInitializer(const std::string &prefix, const std::string &type, const std::string &expression)
    {
        MAREA_IrFixture fixture([](const std::string &text, const MAREA_AstNode **) {
            return (text.compare(0, 12, "collections.") == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        });

        if (fixture.MAREA_lower("namespace main; import collections.table; " + prefix + " constant VALUE: " + type + " = " + expression + ";") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *constant{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if (MAREA_IrNodeGetChildCount(constant) != 1) {
            return "<no initializer>";
        }

        return MAREA_irRender(MAREA_irChild(constant, 0));
    }
} // namespace

TEST(MAREAIrExpressions, LiteralsKeepTheirKindAndText)
{
    GTEST_COUT << "Validating integer, float, string and boolean literals keep their kind and text." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("", "sinteger32", "42"), "(INTEGER_LITERAL 42)") << "An integer literal must keep its text.";

    EXPECT_EQ(MAREA_lowerInitializer("", "float64", "1.5"), "(FLOAT_LITERAL 1.5)") << "A float literal must keep its text.";

    EXPECT_EQ(MAREA_lowerInitializer("", "boolean", "true"), "(BOOLEAN_LITERAL true)") << "A boolean literal must keep its text.";

    EXPECT_EQ(MAREA_lowerInitializer("", "string", "\"hi\""), "(STRING_LITERAL \"hi\")") << "A string literal must keep its text as written.";
}

TEST(MAREAIrExpressions, IdentifierKeepsItsName)
{
    GTEST_COUT << "Validating an identifier keeps its name and has no children." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("constant BASE: sinteger32 = 1;", "sinteger32", "BASE"), "(IDENTIFIER BASE)") << "An identifier must keep its name.";
}

TEST(MAREAIrExpressions, BinaryKeepsOperatorAndOperands)
{
    GTEST_COUT << "Validating a binary expression keeps its operator and two lowered operands." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("", "sinteger32", "1 + 2 * 3"), "(BINARY_EXPRESSION op=PLUS (INTEGER_LITERAL 1) (BINARY_EXPRESSION op=STAR (INTEGER_LITERAL 2) (INTEGER_LITERAL 3)))") << "Precedence must be kept in the tree.";
}

TEST(MAREAIrExpressions, UnaryKeepsOperatorAndOperand)
{
    GTEST_COUT << "Validating a unary expression keeps its operator and one operand." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("", "sinteger32", "-5"), "(UNARY_EXPRESSION op=MINUS (INTEGER_LITERAL 5))") << "A negation must keep its operator.";

    EXPECT_EQ(MAREA_lowerInitializer("", "boolean", "not true"), "(UNARY_EXPRESSION op=KEYWORD_NOT (BOOLEAN_LITERAL true))") << "A not must keep its operator.";
}

TEST(MAREAIrExpressions, ParenthesizedExpressionIsElided)
{
    GTEST_COUT << "Validating a parenthesized expression leaves no node of its own." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("", "sinteger32", "(1 + 2) * 3"), "(BINARY_EXPRESSION op=STAR (BINARY_EXPRESSION op=PLUS (INTEGER_LITERAL 1) (INTEGER_LITERAL 2)) (INTEGER_LITERAL 3))") << "The parentheses must change the tree shape only.";

    EXPECT_EQ(MAREA_lowerInitializer("", "sinteger32", "((7))"), "(INTEGER_LITERAL 7)") << "Nested parentheses must all be elided.";
}

TEST(MAREAIrExpressions, StructLiteralKeepsTypeNameAndFieldEntries)
{
    GTEST_COUT << "Validating a struct literal keeps its type name and one FIELD_ENTRY per field." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer(POINT, "Point", "Point{ x = 1, y = 2 }"), "(LITERAL_VALUE :Point (FIELD_ENTRY x (INTEGER_LITERAL 1)) (FIELD_ENTRY y (INTEGER_LITERAL 2)))") << "A struct literal must keep the shape of the source.";
}

TEST(MAREAIrExpressions, NestedStructLiteralLowersItsValues)
{
    GTEST_COUT << "Validating a field value that is itself a struct literal is lowered." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer(POINT + "structure Line start: Point; end ", "Line", "Line{ start = Point{ x = 1, y = 2 } }"), "(LITERAL_VALUE :Line (FIELD_ENTRY start (LITERAL_VALUE :Point (FIELD_ENTRY x (INTEGER_LITERAL 1)) (FIELD_ENTRY y (INTEGER_LITERAL 2)))))") << "The nested literal must be lowered.";
}

TEST(MAREAIrExpressions, MemberAccessKeepsTargetAndMemberName)
{
    GTEST_COUT << "Validating a member access keeps the lowered target and an IDENTIFIER for the member." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer(POINT + "constant ORIGIN: Point = Point{ x = 0, y = 0 };", "sinteger32", "ORIGIN.x"), "(MEMBER_ACCESS (IDENTIFIER ORIGIN) (IDENTIFIER x))") << "The member name must be an IDENTIFIER child.";
}

TEST(MAREAIrExpressions, CallKeepsCalleeArgumentsAndResultTypeName)
{
    GTEST_COUT << "Validating a call keeps its callee, an ARGUMENT_LIST and the name of a non-generic result type." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("function twice(a: sinteger32): sinteger32 return a + a; end", "sinteger32", "twice(4)"), "(CALL_EXPRESSION :sinteger32 (IDENTIFIER twice) (ARGUMENT_LIST (INTEGER_LITERAL 4)))") << "A call must keep its callee and arguments.";
}

TEST(MAREAIrExpressions, CallWithoutArgumentsKeepsAnEmptyArgumentList)
{
    GTEST_COUT << "Validating a call without arguments keeps an empty ARGUMENT_LIST." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("function one(): sinteger32 return 1; end", "sinteger32", "one()"), "(CALL_EXPRESSION :sinteger32 (IDENTIFIER one) (ARGUMENT_LIST))") << "The argument list must be present and empty.";
}

TEST(MAREAIrExpressions, IndexAccessKeepsTargetAndIndex)
{
    GTEST_COUT << "Validating an index access keeps the lowered target and the lowered index." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("constant TABLE: vector<sinteger32, 3> = vector<sinteger32, 3>{ 1, 2, 3 };", "sinteger32", "TABLE[1]"), "(INDEX_ACCESS (IDENTIFIER TABLE) (INTEGER_LITERAL 1))") << "An index access must keep both operands.";
}

TEST(MAREAIrExpressions, ArrayLiteralKeepsValueEntries)
{
    GTEST_COUT << "Validating a literal of values keeps one VALUE_ENTRY per value." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("", "vector<sinteger32, 2>", "vector<sinteger32, 2>{ 5, 6 }"), "(LITERAL_VALUE :vector (VALUE_ENTRY (INTEGER_LITERAL 5)) (VALUE_ENTRY (INTEGER_LITERAL 6)))") << "Each value must be a VALUE_ENTRY.";
}

TEST(MAREAIrExpressions, IndexedLiteralKeepsIndexEntries)
{
    GTEST_COUT << "Validating a literal of keyed values keeps one INDEX_ENTRY per key and value." << std::endl;

    EXPECT_EQ(MAREA_lowerInitializer("", "table<string, sinteger32>", "table<string, sinteger32>{ [\"a\"] = 1 }"), "(LITERAL_VALUE :table (INDEX_ENTRY (STRING_LITERAL \"a\") (INTEGER_LITERAL 1)))") << "A keyed entry must keep its key and value.";
}

TEST(MAREAIrExpressions, EnumVariantFieldKeepsItsInitializer)
{
    GTEST_COUT << "Validating a variant field default value is lowered as a child of the VARIANT_FIELD." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Planet Mercury(mass: float64 = 1.5); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(MAREA_irChild(fixture.module, 0), 0)), "(VARIANT_DECLARATION Mercury (VARIANT_FIELD mass :float64 (FLOAT_LITERAL 1.5)))") << "The default value must be the only child of the field.";
}

TEST(MAREAIrExpressions, ConstantKeepsItsInitializer)
{
    GTEST_COUT << "Validating a constant carries its lowered initializer as its only child." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant LIMIT: sinteger32 = 10;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 0)), "(CONSTANT_DECLARATION LIMIT :sinteger32 (INTEGER_LITERAL 10))") << "The initializer must be the only child.";
}

namespace
{
    const MAREA_IrNode *MAREA_findFunction(const MAREA_IrNode *module, const std::string &name)
    {
        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(module); ++index) {
            const MAREA_IrNode *declaration{ MAREA_irChild(module, index) };

            if ((MAREA_IrNodeGetKind(declaration) == MAREA_IR_KIND_FUNCTION_DECLARATION) && (MAREA_irText(declaration) == name)) {
                return declaration;
            }
        }

        return nullptr;
    }
} // namespace

TEST(MAREAIrExpressions, StructuresExampleKeepsItsStructLiteralShape)
{
    GTEST_COUT << "Validating the struct literals of examples/types/structures.marea keep their FIELD_ENTRY shape." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("types/structures.marea"), "types/structures.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    const MAREA_IrNode *function{ MAREA_findFunction(fixture.module, "make_square") };

    ASSERT_NE(function, nullptr) << "The function make_square must be lowered.";

    EXPECT_EQ(MAREA_irRender(function), "(FUNCTION_DECLARATION make_square :Shape (PARAMETER_LIST (PARAMETER side :float64)) (BLOCK (RETURN_STATEMENT (LITERAL_VALUE :Shape (FIELD_ENTRY kind (MEMBER_ACCESS (IDENTIFIER ShapeKind) (IDENTIFIER Rectangle))) (FIELD_ENTRY center (LITERAL_VALUE :Point (FIELD_ENTRY x (FLOAT_LITERAL 0.0)) (FIELD_ENTRY y (FLOAT_LITERAL 0.0)))) (FIELD_ENTRY width (IDENTIFIER side)) (FIELD_ENTRY height (IDENTIFIER side))))))") << "The nested struct literal must keep its shape.";
}

TEST(MAREAIrExpressions, OperatorsExampleKeepsItsOperatorShapes)
{
    GTEST_COUT << "Validating examples/basics/operators.marea keeps its operators and the precedence of -left ** 2." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("basics/operators.marea"), "basics/operators.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    const MAREA_IrNode *arithmetic{ MAREA_findFunction(fixture.module, "arithmetic") };

    ASSERT_NE(arithmetic, nullptr) << "The function arithmetic must be lowered.";

    const std::string rendered{ MAREA_irRender(arithmetic) };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION negative_power :sinteger32 (UNARY_EXPRESSION op=MINUS (BINARY_EXPRESSION op=STAR_STAR (IDENTIFIER left) (INTEGER_LITERAL 2))))"), std::string::npos) << "-left ** 2 must keep the negation outside the power: " << rendered;

    const MAREA_IrNode *comparison{ MAREA_findFunction(fixture.module, "comparison") };

    ASSERT_NE(comparison, nullptr) << "The function comparison must be lowered.";

    EXPECT_NE(MAREA_irRender(comparison).find("(LOCAL_DECLARATION grouped :boolean (BINARY_EXPRESSION op=KEYWORD_AND (BINARY_EXPRESSION op=KEYWORD_OR"), std::string::npos) << "The parentheses of a grouped condition must be elided but keep their shape.";
}
