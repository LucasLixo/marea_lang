// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    using Types = std::vector<std::string>;

    // Appends the resolved type of every typed node of one kind, in tree order. A node with no type gives "<none>".
    void MAREA_collectResolvedTypes(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, Types &out)
    {
        if (node == nullptr) {
            return;
        }

        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            const MAREA_Character8 *text{ nullptr };

            MAREA_IntegerSize length{ 0 };

            if (MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                out.push_back(std::string(text, length));
            } else {
                out.push_back("<none>");
            }
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_collectResolvedTypes(child, kind, out);
            }
        }
    }

    // Checks the source with zero errors, builds its typed tree and lists the resolved type of every literal of one kind.
    Types MAREA_literalTypes(const std::string &source, MAREA_AstKind kind)
    {
        MAREA_SemaFixture fixture;

        Types result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            std::string codes{ "<unchecked" };

            for (const MAREA_Uinteger32 CODE : fixture.parser.MAREA_codes()) {
                codes += " " + std::to_string(CODE);
            }

            result.push_back(codes + ">");

            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            result.push_back("<unbuilt>");

            return result;
        }

        MAREA_collectResolvedTypes(root, kind, result);

        return result;
    }

    Types MAREA_integers(const std::string &source)
    {
        return MAREA_literalTypes(source, MAREA_AST_KIND_INTEGER_LITERAL);
    }

    Types MAREA_floats(const std::string &source)
    {
        return MAREA_literalTypes(source, MAREA_AST_KIND_FLOAT_LITERAL);
    }
} // namespace

TEST(MAREASemaTypedAstLiteral, LocalDeclaredTypeFixesTheLiteral)
{
    GTEST_COUT << "Validating a literal that initializes a local takes the declared type of the local." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f() local x: uinteger8 = 5; end"), Types{ "uinteger8" }) << "An integer literal must take the type of its local.";

    EXPECT_EQ(MAREA_floats("namespace main; function f() local x: float32 = 1.5; end"), Types{ "float32" }) << "A float literal must take the type of its local.";

    EXPECT_EQ(MAREA_integers("namespace main; function f() local x: float32 = 1; end"), Types{ "float32" }) << "Sema accepts an integer literal for a float type, so the literal takes that float type.";
}

TEST(MAREASemaTypedAstLiteral, LocalTypeAliasIsFollowed)
{
    GTEST_COUT << "Validating a local type alias gives the primitive it names." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; type Small = uinteger8; function f() local x: Small = 5; end"), Types{ "uinteger8" }) << "The literal must take the primitive behind the alias.";
}

TEST(MAREASemaTypedAstLiteral, ConstantDeclaredTypeFixesTheLiteral)
{
    GTEST_COUT << "Validating a literal that initializes a constant takes the declared type of the constant." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; constant LIMIT: sinteger64 = 7;"), Types{ "sinteger64" }) << "The constant type must fix the literal.";

    EXPECT_EQ(MAREA_floats("namespace main; constant EPSILON: float32 = 0.5;"), Types{ "float32" }) << "The constant type must fix a float literal.";
}

TEST(MAREASemaTypedAstLiteral, VariantFieldDefaultTakesTheFieldType)
{
    GTEST_COUT << "Validating the default value of a variant field takes the type of the variant field." << std::endl;

    EXPECT_EQ(MAREA_floats("namespace main; enum Shape Dot; Circle(radius: float32 = 1.0); end"), Types{ "float32" }) << "The variant field type must fix the default.";

    EXPECT_EQ(MAREA_integers("namespace main; enum Shape Dot; Box(side: uinteger16 = 4); end"), Types{ "uinteger16" }) << "The variant field type must fix an integer default.";
}

TEST(MAREASemaTypedAstLiteral, ParameterTypeFixesAnArgument)
{
    GTEST_COUT << "Validating an argument takes the type of the parameter it is passed to." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function g(a: sinteger64) end function f() g(5); end"), Types{ "sinteger64" }) << "The parameter type must fix the argument.";

    EXPECT_EQ(MAREA_floats("namespace main; function g(a: float32, b: float64) end function f() g(1.0, 2.0); end"), (Types{ "float32", "float64" })) << "Each argument must take the type of its own parameter.";
}

TEST(MAREASemaTypedAstLiteral, DefaultArgumentTakesTheParameterType)
{
    GTEST_COUT << "Validating a default argument takes the type of its parameter." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function g(a: uinteger16 = 9) end"), Types{ "uinteger16" }) << "The parameter type must fix its default value.";
}

TEST(MAREASemaTypedAstLiteral, ConstructionArgumentTakesTheConstructParameterType)
{
    GTEST_COUT << "Validating an argument of a type construction call takes the type of the construct parameter." << std::endl;

    const std::string SOURCE{ "namespace main; structure Point x: float32; y: float64; end implementation Point #[operator = \"construct\"] function make(x: float32 = 0.0, y: float64 = 0.0) end end function g() Point(1, 2); end" };

    EXPECT_EQ(MAREA_integers(SOURCE), (Types{ "float32", "float64" })) << "The construct parameters must fix the arguments.";

    EXPECT_EQ(MAREA_floats(SOURCE), (Types{ "float32", "float64" })) << "The defaults of the construct parameters must take their own types.";
}

TEST(MAREASemaTypedAstLiteral, ReturnTypeFixesTheReturnedLiteral)
{
    GTEST_COUT << "Validating a returned literal takes the result type of the function." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f(): uinteger8 return 3; end"), Types{ "uinteger8" }) << "The result type must fix the returned literal.";
}

TEST(MAREASemaTypedAstLiteral, OtherOperandOfABinaryExpressionFixesTheLiteral)
{
    GTEST_COUT << "Validating a literal operand takes the type of the other operand when that one is known." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: float32): float32 return a + 1; end"), Types{ "float32" }) << "A literal on the right must take the type of the left operand.";

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: uinteger8): uinteger8 return 2 * a; end"), Types{ "uinteger8" }) << "A literal on the left must take the type of the right operand.";

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: uinteger16) if a < 2 then end end end"), Types{ "uinteger16" }) << "A compare must fix the literal from the other operand.";
}

TEST(MAREASemaTypedAstLiteral, OtherOperandReachesANestedSum)
{
    GTEST_COUT << "Validating the type of the other operand reaches the literals of a nested sum." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: sinteger8): sinteger8 return a + (1 + 2); end"), (Types{ "sinteger8", "sinteger8" })) << "Both literals of a nested sum must take the type of the other operand.";
}

TEST(MAREASemaTypedAstLiteral, ContextPassesThroughParenthesesAndArithmetic)
{
    GTEST_COUT << "Validating the type of a context reaches the literals of a parenthesized and unary expression." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f() local x: sinteger8 = (1 + 2) * 3; end"), (Types{ "sinteger8", "sinteger8", "sinteger8" })) << "Every literal of an arithmetic expression must take the type of the local.";

    EXPECT_EQ(MAREA_integers("namespace main; function f() local x: sinteger16 = -(4); end"), Types{ "sinteger16" }) << "A unary minus and a parenthesis must pass the type down.";
}

TEST(MAREASemaTypedAstLiteral, ShiftCountDoesNotTakeTheShiftedType)
{
    GTEST_COUT << "Validating the count of a shift takes the default, not the type of the shifted value." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f() local x: uinteger8 = 1 << 2; end"), (Types{ "uinteger8", "sinteger32" })) << "The left side takes the local type and the count keeps the default.";
}

TEST(MAREASemaTypedAstLiteral, ElementTypeFixesAnArrayEntry)
{
    GTEST_COUT << "Validating an entry of an array literal takes the element type." << std::endl;

    EXPECT_EQ(MAREA_floats("namespace main; function f() local a: array<float32, 2> = array<float32, 2>{ 1.0, 2.0 }; end"), (Types{ "float32", "float32" })) << "Every plain entry must take the element type.";

    EXPECT_EQ(MAREA_integers("namespace main; function f() local a: array<uinteger8, 2> = array<uinteger8, 2>{ [0] = 1, [1] = 2 }; end"), (Types{ "sinteger32", "sinteger32", "sinteger32", "uinteger8", "sinteger32", "uinteger8" })) << "An index entry value must take the element type; the size arguments and the keys keep the default.";
}

TEST(MAREASemaTypedAstLiteral, FieldTypeFixesAStructureLiteralEntry)
{
    GTEST_COUT << "Validating a field entry of a structure literal takes the declared type of the field." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; structure P x: float32; y: uinteger8; end function f() local p: P = P{ x = 1, y = 2 }; end"), (Types{ "float32", "uinteger8" })) << "Each field entry must take the type of its field.";
}

TEST(MAREASemaTypedAstLiteral, ConvertTargetFixesTheOperandLiteral)
{
    GTEST_COUT << "Validating the literal operand of convert takes the target type of the conversion." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f() local x: sinteger8 = convert<sinteger8>(5); end"), Types{ "sinteger8" }) << "The target type of convert must fix its literal operand.";
}

TEST(MAREASemaTypedAstLiteral, AssignmentTargetFixesTheLiteral)
{
    GTEST_COUT << "Validating an assigned literal takes the type of the target, for = and for a compound operator." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: uinteger8) a = 5; a += 1; end"), (Types{ "uinteger8", "uinteger8" })) << "The target type must fix the literal of a plain and of a compound assignment.";
}

TEST(MAREASemaTypedAstLiteral, SwitchSubjectFixesTheCaseLiteral)
{
    GTEST_COUT << "Validating a case literal takes the type of the switched value." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: uinteger8) switch a case 1, 2 then end default then end end end"), (Types{ "uinteger8", "uinteger8" })) << "Each case literal must take the type of the subject.";
}

TEST(MAREASemaTypedAstLiteral, RangeBoundTakesTheTypeOfTheOtherBound)
{
    GTEST_COUT << "Validating a literal bound of a range takes the type of the other bound." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f(n: uinteger16) for i in 0 .. n do end end"), Types{ "uinteger16" }) << "The known bound must fix the literal bound.";
}

TEST(MAREASemaTypedAstLiteral, LiteralWithNoContextTakesTheDefault)
{
    GTEST_COUT << "Validating a literal with no fixing context takes sinteger32 or float64." << std::endl;

    EXPECT_EQ(MAREA_integers("namespace main; function f() if 1 < 2 then end end end"), (Types{ "sinteger32", "sinteger32" })) << "Two integer literals compared must take sinteger32.";

    EXPECT_EQ(MAREA_floats("namespace main; function f() if 1.5 < 2.5 then end end end"), (Types{ "float64", "float64" })) << "Two float literals compared must take float64.";

    EXPECT_EQ(MAREA_integers("namespace main; function g<Item>(a: Item) end function f() g(5); end"), Types{ "sinteger32" }) << "A parameter of a generic type fixes no primitive, so the default is used.";

    EXPECT_EQ(MAREA_integers("namespace main; function f(a: array<sinteger32, 4>): sinteger32 return a[0]; end"), (Types{ "sinteger32", "sinteger32" })) << "A size argument and an index are not values of a context, so each keeps the default.";
}

TEST(MAREASemaTypedAstLiteral, StringCharacterAndBooleanKeepTheirOwnType)
{
    GTEST_COUT << "Validating a string, character and boolean literal always give their own primitive type." << std::endl;

    const std::string SOURCE{ "namespace main; function f() local s: string = \"text\"; local c: character = 'a'; local b: boolean = true; if \"x\" == \"y\" then end end end" };

    EXPECT_EQ(MAREA_literalTypes(SOURCE, MAREA_AST_KIND_STRING_LITERAL), (Types{ "string", "string", "string" })) << "A string literal must be a string.";

    EXPECT_EQ(MAREA_literalTypes(SOURCE, MAREA_AST_KIND_CHARACTER_LITERAL), Types{ "character" }) << "A character literal must be a character.";

    EXPECT_EQ(MAREA_literalTypes(SOURCE, MAREA_AST_KIND_BOOLEAN_LITERAL), Types{ "boolean" }) << "A boolean literal must be a boolean.";
}

TEST(MAREASemaTypedAstLiteral, LiteralTypeIsTheSameOnTwoRuns)
{
    GTEST_COUT << "Validating two builds of one source give the same literal types." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: float32): float32 local x: uinteger8 = 5; return a + 1; end" };

    EXPECT_EQ(MAREA_integers(SOURCE), MAREA_integers(SOURCE)) << "Two runs must give the same types.";
}
