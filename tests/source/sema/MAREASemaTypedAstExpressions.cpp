// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>

namespace
{
    struct MAREA_ExpressionTypeResult
    {
        MAREA_SemaTypedAstExcept except{ MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER };
        std::string text;
        bool found{ false };
    };

    void MAREA_collectTypedNodesByKind(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, std::vector<const MAREA_SemaTypedNode *> &out)
    {
        if (node == nullptr) {
            return;
        }

        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            out.push_back(node);
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_collectTypedNodesByKind(child, kind, out);
            }
        }
    }

    // Checks the source, builds the typed tree and reads the resolved type of the nth node of a kind, in source order.
    MAREA_ExpressionTypeResult MAREA_expressionTypeOf(MAREA_SemaFixture &fixture, const std::string &source, MAREA_AstKind kind, std::size_t nth, const std::string &name = "")
    {
        MAREA_ExpressionTypeResult result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return result;
        }

        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collectTypedNodesByKind(root, kind, nodes);

        if (!name.empty()) {
            nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&name](const MAREA_SemaTypedNode *node) {
                            const MAREA_AstNode *ast{ MAREA_SemaTypedNodeGetAstNode(node) };

                            return std::string(MAREA_AstNodeGetText(ast), MAREA_AstNodeGetTextLength(ast)) != name;
                        }),
                        nodes.end());
        }

        if (nth >= nodes.size()) {
            return result;
        }

        result.found = true;

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        result.except = MAREA_SemaTypedNodeGetResolvedType(nodes[nth], &text, &length);

        if (result.except == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            result.text = std::string(text, length);
        }

        return result;
    }

    void MAREA_expectIdentifierType(const std::string &source, const std::string &name, std::size_t nth, const std::string &expected)
    {
        MAREA_SemaFixture fixture;

        const MAREA_ExpressionTypeResult RESULT{ MAREA_expressionTypeOf(fixture, source, MAREA_AST_KIND_IDENTIFIER, nth, name) };

        ASSERT_TRUE(RESULT.found) << "The identifier " << name << " must be in the typed tree of: " << source;

        ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The identifier " << name << " must have a resolved type in: " << source;

        EXPECT_EQ(RESULT.text, expected) << "The resolved type is wrong in: " << source;
    }

    void MAREA_expectExpressionType(const std::string &source, MAREA_AstKind kind, std::size_t nth, const std::string &expected, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_SemaFixture fixture(resolve);

        const MAREA_ExpressionTypeResult RESULT{ MAREA_expressionTypeOf(fixture, source, kind, nth) };

        ASSERT_TRUE(RESULT.found) << "The node must be in the typed tree of: " << source;

        ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The node must have a resolved type in: " << source;

        EXPECT_EQ(RESULT.text, expected) << "The resolved type is wrong in: " << source;
    }

    void MAREA_expectExpressionNoType(const std::string &source, MAREA_AstKind kind, std::size_t nth, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_SemaFixture fixture(resolve);

        const MAREA_ExpressionTypeResult RESULT{ MAREA_expressionTypeOf(fixture, source, kind, nth) };

        ASSERT_TRUE(RESULT.found) << "The node must be in the typed tree of: " << source;

        EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "The node must have no type in: " << source;
    }

    const MAREA_ResolveCallback MAREA_EXTERNAL_RESOLVE{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };
} // namespace

TEST(MAREASemaTypedAstExpressions, ArithmeticBinaryIsTheOperandType)
{
    GTEST_COUT << "Validating an arithmetic binary expression gives the type of its operands." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return a + b; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "sinteger32");

    MAREA_expectExpressionType("namespace main; function f(a: float64, b: float64): float64 return a * b - a; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "float64");

    MAREA_expectExpressionType("namespace main; function f(a: uinteger8, b: uinteger8): uinteger8 return a & b | a ^ b; end", MAREA_AST_KIND_BINARY_EXPRESSION, 1, "uinteger8");
}

TEST(MAREASemaTypedAstExpressions, BinaryWithALiteralTakesTheTypedOperand)
{
    GTEST_COUT << "Validating a binary expression with a number literal on one side gives the type of the declared side." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: sinteger64): sinteger64 return a + 1; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "sinteger64");

    MAREA_expectExpressionType("namespace main; function f(a: float32): float32 return 2.0 * a; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "float32");

    MAREA_expectExpressionType("namespace main; function f(a: uinteger16): uinteger16 return (a + 1) * 2; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "uinteger16");
}

TEST(MAREASemaTypedAstExpressions, ComparisonAndLogicalAreBoolean)
{
    GTEST_COUT << "Validating a comparison and a logical expression give boolean." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: sinteger32, b: sinteger32): boolean return a < b; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "boolean");

    MAREA_expectExpressionType("namespace main; function f(a: float32, b: float32): boolean return a == b; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "boolean");

    MAREA_expectExpressionType("namespace main; function f(a: sinteger32, b: sinteger32): boolean return (a < b) and (b != a); end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "boolean");

    MAREA_expectExpressionType("namespace main; function f(a: boolean, b: boolean): boolean return a or b; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "boolean");
}

TEST(MAREASemaTypedAstExpressions, ShiftIsTheLeftOperandType)
{
    GTEST_COUT << "Validating a shift gives the type of its left operand, whatever the count type is." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: uinteger32, count: uinteger8): uinteger32 return a << count; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "uinteger32");

    MAREA_expectExpressionType("namespace main; function f(a: sinteger64, count: uinteger8): sinteger64 return a >> count; end", MAREA_AST_KIND_BINARY_EXPRESSION, 0, "sinteger64");
}

TEST(MAREASemaTypedAstExpressions, UnaryIsTheOperandTypeAndNotIsBoolean)
{
    GTEST_COUT << "Validating minus and tilde give the operand type, and not gives boolean." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: sinteger32): sinteger32 return -a; end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "sinteger32");

    MAREA_expectExpressionType("namespace main; function f(a: uinteger16): uinteger16 return ~a; end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "uinteger16");

    MAREA_expectExpressionType("namespace main; function f(a: boolean): boolean return not a; end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "boolean");
}

TEST(MAREASemaTypedAstExpressions, ParenthesizedIsTheInnerType)
{
    GTEST_COUT << "Validating a parenthesized expression gives the type of the expression inside." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return (a + b); end", MAREA_AST_KIND_PARENTHESIZED_EXPRESSION, 0, "sinteger32");
}

TEST(MAREASemaTypedAstExpressions, CopyAndMoveAreTheOperandType)
{
    GTEST_COUT << "Validating copy and move give the full type of their operand, type arguments included." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Shape x: float32; end function f(s: Shape): Shape return copy s; end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "Shape");

    MAREA_expectExpressionType("namespace main; function f(a: array<sinteger32, 4>): array<sinteger32, 4> return copy a; end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "array<sinteger32,4>");

    MAREA_expectExpressionType("namespace main; structure Shape x: float32; end function f(s: owned<Shape>): owned<Shape> return move s; end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "owned<Shape>");
}

TEST(MAREASemaTypedAstExpressions, IdentifierIsTheTypeOfItsDeclaration)
{
    GTEST_COUT << "Validating an identifier gives the full type of its parameter, local or constant." << std::endl;

    MAREA_expectIdentifierType("namespace main; function f(a: array<sinteger32, 4>): array<sinteger32, 4> return copy a; end", "a", 1, "array<sinteger32,4>");

    MAREA_expectIdentifierType("namespace main; function f(): float32 local x: float32 = 1.0; return x; end", "x", 1, "float32");

    MAREA_expectIdentifierType("namespace main; constant LIMIT: sinteger32 = 8; function f(): sinteger32 return LIMIT; end", "LIMIT", 1, "sinteger32");

    MAREA_expectIdentifierType("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return a + b; end", "b", 1, "sinteger32");
}

TEST(MAREASemaTypedAstExpressions, FunctionNameIsAFunctionType)
{
    GTEST_COUT << "Validating the name of a function used as a callee gives its function type." << std::endl;

    MAREA_expectIdentifierType("namespace main; function g(a: sinteger32, b: float64): float64 return b; end function f(): float64 return g(1, 2.0); end", "g", 1, "function(sinteger32,float64):float64");

    MAREA_expectIdentifierType("namespace main; function g(a: string) end function f(text: string) g(text); end", "g", 1, "function(string)");

    MAREA_expectIdentifierType("namespace main; function same<Item>(a: Item): Item where Item: Copyable return copy a; end function f(x: sinteger32): sinteger32 return same<sinteger32>(x); end", "same", 1, "function(sinteger32):sinteger32");
}

TEST(MAREASemaTypedAstExpressions, IdentifierThatIsNotAValueHasNoType)
{
    GTEST_COUT << "Validating the name of a declaration and a member name are not typed as values." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Point x: float32; end function f(p: Point): float32 local x: float32 = p.x; return x; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The source must raise no error.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    std::vector<const MAREA_SemaTypedNode *> access;

    MAREA_collectTypedNodesByKind(root, MAREA_AST_KIND_MEMBER_ACCESS, access);

    ASSERT_EQ(access.size(), std::size_t{ 1 }) << "The source has one member access.";

    const MAREA_SemaTypedNode *member{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(access[0], 1, &member), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The member name must be reachable.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(member, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "The name after the dot is not a value and must have no type.";

    std::vector<const MAREA_SemaTypedNode *> locals;

    MAREA_collectTypedNodesByKind(root, MAREA_AST_KIND_LOCAL_DECLARATION, locals);

    ASSERT_EQ(locals.size(), std::size_t{ 1 }) << "The source has one local.";

    const MAREA_SemaTypedNode *localName{ nullptr };

    ASSERT_EQ(MAREA_SemaTypedNodeGetChildAt(locals[0], 0, &localName), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The local name must be reachable.";

    EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(localName, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "The name of a declaration is not a value and must have no type.";
}

TEST(MAREASemaTypedAstExpressions, MemberAccessIsTheFieldType)
{
    GTEST_COUT << "Validating a member access gives the declared type of the field." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Point x: float64; y: float32; end function f(p: Point): float32 return p.y; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "float32");

    MAREA_expectExpressionType("namespace main; structure Line points: vector<float32>; end function f(l: Line): vector<float32> return move l.points; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "vector<float32>");
}

TEST(MAREASemaTypedAstExpressions, NestedMemberAccessFollowsTheChain)
{
    GTEST_COUT << "Validating a chain of member accesses types every link." << std::endl;

    const std::string SOURCE{ "namespace main; structure Inner v: sinteger32; end structure Outer inner: Inner; end function f(o: Outer): sinteger32 return o.inner.v; end" };

    MAREA_expectExpressionType(SOURCE, MAREA_AST_KIND_MEMBER_ACCESS, 0, "sinteger32");

    MAREA_expectExpressionType(SOURCE, MAREA_AST_KIND_MEMBER_ACCESS, 1, "Inner");
}

TEST(MAREASemaTypedAstExpressions, MemberOfAGenericStructureSubstitutesTheArguments)
{
    GTEST_COUT << "Validating a field of a generic structure is written with the type arguments of the receiver." << std::endl;

    const std::string SOURCE{ "namespace main; structure Box<Item> value: Item; items: array<Item, 4>; end function f(b: Box<sinteger32>): sinteger32 return b.value; end function g(b: Box<float32>): array<float32, 4> return copy b.items; end" };

    MAREA_expectExpressionType(SOURCE, MAREA_AST_KIND_MEMBER_ACCESS, 0, "sinteger32");

    MAREA_expectExpressionType(SOURCE, MAREA_AST_KIND_MEMBER_ACCESS, 1, "array<float32,4>");
}

TEST(MAREASemaTypedAstExpressions, MemberOfAnImportedTypeHasNoType)
{
    GTEST_COUT << "Validating a member of a value whose type sema has no tree for has no type." << std::endl;

    MAREA_expectExpressionNoType("namespace main; import c.shapes { Shape }; function f(s: Shape): float32 return s.size; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstExpressions, MemberOfAGenericParameterHasNoType)
{
    GTEST_COUT << "Validating a member of a value whose type is a generic parameter has no type." << std::endl;

    MAREA_expectExpressionNoType("namespace main; contract Sized function size(self: Self): sinteger32; end function f<Item>(a: Item): sinteger32 where Item: Sized return a.size(); end", MAREA_AST_KIND_MEMBER_ACCESS, 0);
}

TEST(MAREASemaTypedAstExpressions, EnumVariantIsTheEnum)
{
    GTEST_COUT << "Validating Enum.Variant gives the enum type." << std::endl;

    MAREA_expectExpressionType("namespace main; enum Color Red; Green; end function f(): Color return Color.Red; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "Color");
}

TEST(MAREASemaTypedAstExpressions, IndexAccessIsTheElementType)
{
    GTEST_COUT << "Validating an index access on an array, a vector and a slice gives the element type." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(a: array<sinteger32, 4>): sinteger32 return a[1]; end", MAREA_AST_KIND_INDEX_ACCESS, 0, "sinteger32");

    MAREA_expectExpressionType("namespace main; function f(v: vector<float32>): float32 return v[0]; end", MAREA_AST_KIND_INDEX_ACCESS, 0, "float32");

    MAREA_expectExpressionType("namespace main; function f(s: slice<uinteger8>): uinteger8 return s[2]; end", MAREA_AST_KIND_INDEX_ACCESS, 0, "uinteger8");

    MAREA_expectExpressionType("namespace main; function f(a: array<array<float32, 2>, 3>): float32 return a[1][0]; end", MAREA_AST_KIND_INDEX_ACCESS, 0, "float32");

    MAREA_expectExpressionType("namespace main; function f(a: array<array<float32, 2>, 3>): float32 return a[1][0]; end", MAREA_AST_KIND_INDEX_ACCESS, 1, "array<float32,2>");
}

TEST(MAREASemaTypedAstExpressions, IndexOfAFieldOfAGenericStructureSubstitutes)
{
    GTEST_COUT << "Validating an element of a field of a generic structure carries the receiver type argument." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Buffer<Item> items: array<Item, 4>; end function f(b: Buffer<float32>): float32 return b.items[0]; end", MAREA_AST_KIND_INDEX_ACCESS, 0, "float32");
}

TEST(MAREASemaTypedAstExpressions, CallIsTheDeclaredReturnType)
{
    GTEST_COUT << "Validating a call to a function of the file gives its declared result type." << std::endl;

    MAREA_expectExpressionType("namespace main; function g(a: sinteger32): float64 return 1.0; end function f(x: sinteger32): float64 return g(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "float64");

    MAREA_expectExpressionType("namespace main; function g(): vector<array<sinteger32, 4>> return vector<array<sinteger32, 4>>{}; end function f(): vector<array<sinteger32, 4>> return g(); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "vector<array<sinteger32,4>>");
}

TEST(MAREASemaTypedAstExpressions, CallWithoutResultHasNoType)
{
    GTEST_COUT << "Validating a call to a function with no result type has no type." << std::endl;

    MAREA_expectExpressionNoType("namespace main; function g(a: sinteger32) end function f(x: sinteger32) g(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0);
}

TEST(MAREASemaTypedAstExpressions, GenericCallWithExplicitArgumentsSubstitutes)
{
    GTEST_COUT << "Validating a generic call with explicit type arguments substitutes them in a bare and in a compound result." << std::endl;

    MAREA_expectExpressionType("namespace main; function same<Item>(a: Item): Item where Item: Copyable return copy a; end function f(x: sinteger32): sinteger32 return same<sinteger32>(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "sinteger32");

    MAREA_expectExpressionType("namespace main; structure Pair<First, Second> first: First; second: Second; end function make<First, Second>(a: First, b: Second): Pair<Second, First> where First: Copyable and Second: Copyable return Pair<Second, First>{ first = copy b, second = copy a }; end function f(x: sinteger32, y: float32): Pair<float32, sinteger32> return make<sinteger32, float32>(x, y); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "Pair<float32,sinteger32>");
}

TEST(MAREASemaTypedAstExpressions, GenericCallWithInferredArgumentsHasNoType)
{
    GTEST_COUT << "Validating a generic call whose result needs an inferred type argument has no type." << std::endl;

    MAREA_expectExpressionNoType("namespace main; function same<Item>(a: Item): Item where Item: Copyable return copy a; end function f(x: sinteger32): sinteger32 return same(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0);
}

TEST(MAREASemaTypedAstExpressions, GenericCallWhoseResultNeedsNoArgumentKeepsItsType)
{
    GTEST_COUT << "Validating a generic call whose result type does not mention a type parameter keeps its type." << std::endl;

    MAREA_expectExpressionType("namespace main; function weigh<Item>(a: Item): uinteger64 where Item: Copyable return 1; end function f(x: sinteger32): uinteger64 return weigh(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "uinteger64");
}

TEST(MAREASemaTypedAstExpressions, ConstructionCallIsTheConstructedType)
{
    GTEST_COUT << "Validating a construction call gives the constructed type." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Point x: float32; y: float32; end implementation Point #[operator = \"construct\"] function make(x: float32 = 0.0, y: float32 = 0.0) end end function f(): Point return Point(1.0, 2.0); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "Point");
}

TEST(MAREASemaTypedAstExpressions, ConvertIsTheTargetType)
{
    GTEST_COUT << "Validating convert<T>(x) gives T." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(x: sinteger32): sinteger64 return convert<sinteger64>(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "sinteger64");

    MAREA_expectExpressionType("namespace main; structure Meters value: float32; end structure Feet value: float32; end function f(x: Feet): Meters return convert<Meters>(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "Meters");
}

TEST(MAREASemaTypedAstExpressions, StructureLiteralIsTheWrittenType)
{
    GTEST_COUT << "Validating a structure literal and an array literal give the written type." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Point x: float32; y: float32; end function f(a: float32): Point return Point{ x = a, y = a }; end", MAREA_AST_KIND_LITERAL_VALUE, 0, "Point");

    MAREA_expectExpressionType("namespace main; function f(a: float32): array<float32, 2> return array<float32, 2>{ a, a }; end", MAREA_AST_KIND_LITERAL_VALUE, 0, "array<float32,2>");

    MAREA_expectExpressionType("namespace main; structure Pair<First, Second> first: First; second: Second; end function f(a: float32): Pair<float32, float32> return Pair<float32, float32>{ first = a, second = a }; end", MAREA_AST_KIND_LITERAL_VALUE, 0, "Pair<float32,float32>");
}

TEST(MAREASemaTypedAstExpressions, MethodCallIsTheMethodResult)
{
    GTEST_COUT << "Validating a method call, a static call and a method on a generic receiver give the declared result." << std::endl;

    const std::string SOURCE{ "namespace main; structure Vector2 x: float64; y: float64; end implementation Vector2 function origin(): Vector2 return Vector2{ x = 0.0, y = 0.0 }; end function length_squared(self: Vector2): float64 return self.x * self.x + self.y * self.y; end end function f(): float64 local start: Vector2 = Vector2.origin(); return start.length_squared(); end" };

    MAREA_expectExpressionType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 0, "Vector2");

    MAREA_expectExpressionType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 1, "float64");

    MAREA_expectExpressionType("namespace main; structure Box<Item> value: Item; end implementation<Item> Box<Item> function get(self: Box<Item>): Item where Item: Copyable return copy self.value; end end function f(b: Box<float32>): float32 return b.get(); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "float32");
}

TEST(MAREASemaTypedAstExpressions, CallOfAFunctionValueIsTheFunctionResult)
{
    GTEST_COUT << "Validating a call of a parameter or a field with a function type gives its result type." << std::endl;

    MAREA_expectExpressionType("namespace main; function f(op: function(x: sinteger32): float64, a: sinteger32): float64 return op(a); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "float64");

    MAREA_expectExpressionType("namespace main; structure Handler run: function(x: sinteger32): boolean; end function f(h: Handler, a: sinteger32): boolean return h.run(a); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "boolean");
}

TEST(MAREASemaTypedAstExpressions, CallOfALibraryFunctionHasNoType)
{
    GTEST_COUT << "Validating a call whose callee comes from an import sema has no tree for has no type." << std::endl;

    MAREA_expectExpressionNoType("namespace main; import c.math.sqrt as square_root; function f(x: float64): float64 return square_root(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, MAREA_EXTERNAL_RESOLVE);

    MAREA_expectExpressionNoType("namespace main; import c.stdio; function f(text: string) stdio.puts(text); end", MAREA_AST_KIND_MEMBER_ACCESS, 0, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstExpressions, MemberThroughAnOwnerIsTheFieldType)
{
    GTEST_COUT << "Validating a member read on an owned<T> value gives the field type of T." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Shape x: float32; end function f(s: owned<Shape>): float32 return s.x; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "float32");

    MAREA_expectExpressionType("namespace main; function f(a: owned<array<float64, 3>>): float64 return a[0]; end", MAREA_AST_KIND_INDEX_ACCESS, 0, "float64");
}

TEST(MAREASemaTypedAstExpressions, EnumVariantFieldIsReadOnTheEnum)
{
    GTEST_COUT << "Validating a field of an enum variant gives the declared type of the field." << std::endl;

    MAREA_expectExpressionType("namespace main; enum Planet Earth(mass: float64 = 1.0, radius: float32 = 2.0); end function f(p: Planet): float32 return p.radius; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "float32");
}

TEST(MAREASemaTypedAstExpressions, CallThroughAFunctionAliasIsTheFunctionResult)
{
    GTEST_COUT << "Validating a call of a value whose type is a local alias of a function type gives its result." << std::endl;

    MAREA_expectExpressionType("namespace main; type Operation = function(a: sinteger32): float64; function f(op: Operation, x: sinteger32): float64 return op(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "float64");
}

TEST(MAREASemaTypedAstExpressions, ConstantOfAnImplementationIsItsType)
{
    GTEST_COUT << "Validating Type.NAME of a constant of an implementation block gives the type of the constant." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Body mass: float64; end implementation Body constant GRAVITY: float32 = 1.0; end function f(): float32 return Body.GRAVITY; end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "float32");
}

TEST(MAREASemaTypedAstExpressions, ValueParameterOfAGenericIsItsDeclaredType)
{
    GTEST_COUT << "Validating a value parameter such as Count in <constant Count: sinteger32> gives its declared type." << std::endl;

    MAREA_expectIdentifierType("namespace main; structure Buffer<Item, constant Count: sinteger32> items: array<Item, Count>; end implementation<Item, constant Count: sinteger32> Buffer<Item, Count> function capacity(self: Buffer<Item, Count>): sinteger32 return Count; end end", "Count", 5, "sinteger32");
}

TEST(MAREASemaTypedAstExpressions, SelfInAnImplementationIsTheImplementedType)
{
    GTEST_COUT << "Validating a member read on a value of type Self gives the field of the implemented type." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Vector2 x: float64; y: float32; end implementation Vector2 function height(self: Self): float32 return self.y; end end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "float32");
}

TEST(MAREASemaTypedAstExpressions, MethodNameIsAFunctionTypeWithTheReceiver)
{
    GTEST_COUT << "Validating a method named without a call gives a function type that starts with the receiver." << std::endl;

    MAREA_expectExpressionType("namespace main; structure Vector2 x: float64; end implementation Vector2 function scale(self: Vector2, factor: float64) end end function f(v: Vector2) v.scale(2.0); end", MAREA_AST_KIND_MEMBER_ACCESS, 0, "function(Vector2,float64)");
}

TEST(MAREASemaTypedAstExpressions, NameOfALoopHasNoType)
{
    GTEST_COUT << "Validating the name of a for statement has no type, because sema keeps no type for it." << std::endl;

    MAREA_SemaFixture fixture;

    const MAREA_ExpressionTypeResult RESULT{ MAREA_expressionTypeOf(fixture, "namespace main; function f(n: sinteger32): sinteger32 local total: sinteger32 = 0; for i in 0 .. n do total += i; end return total; end", MAREA_AST_KIND_IDENTIFIER, 1, "i") };

    ASSERT_TRUE(RESULT.found) << "The loop name must be read once as a value.";

    EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "The loop name has no type.";
}

TEST(MAREASemaTypedAstExpressions, NamespaceAndTypeNamesHaveNoTypeAsValues)
{
    GTEST_COUT << "Validating the name of an import and the name of a type have no type as values." << std::endl;

    MAREA_SemaFixture fixture(MAREA_EXTERNAL_RESOLVE);

    const MAREA_ExpressionTypeResult RESULT{ MAREA_expressionTypeOf(fixture, "namespace main; import c.stdio; function f(text: string) stdio.puts(text); end", MAREA_AST_KIND_IDENTIFIER, 1, "stdio") };

    ASSERT_TRUE(RESULT.found) << "The namespace name must be read once as a value.";

    EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "A namespace name has no type.";

    MAREA_SemaFixture second;

    const MAREA_ExpressionTypeResult TYPE_NAME{ MAREA_expressionTypeOf(second, "namespace main; structure Body mass: float64; end implementation Body constant GRAVITY: float32 = 1.0; end function f(): float32 return Body.GRAVITY; end", MAREA_AST_KIND_IDENTIFIER, 2, "Body") };

    ASSERT_TRUE(TYPE_NAME.found) << "The type name must be read once as a value.";

    EXPECT_EQ(TYPE_NAME.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "A type name has no type as a value.";
}

TEST(MAREASemaTypedAstExpressions, SpawnIsATaskOfTheResult)
{
    GTEST_COUT << "Validating spawn of a call gives a task of the type of the call." << std::endl;

    MAREA_expectExpressionType("namespace main; function work(a: float32): float32 return a; end function f(a: float32) fork_join local t: task<float32> = spawn work(a); end end", MAREA_AST_KIND_UNARY_EXPRESSION, 0, "task<float32>");
}

TEST(MAREASemaTypedAstExpressions, ExpressionTypeIsTheSameOnTwoRuns)
{
    GTEST_COUT << "Validating two builds of one source give the same text for an expression." << std::endl;

    const std::string SOURCE{ "namespace main; structure Box<Item> value: Item; end function f(b: Box<array<float32, 2>>): array<float32, 2> return copy b.value; end" };

    MAREA_SemaFixture first;

    MAREA_SemaFixture second;

    const MAREA_ExpressionTypeResult ONE{ MAREA_expressionTypeOf(first, SOURCE, MAREA_AST_KIND_MEMBER_ACCESS, 0) };

    const MAREA_ExpressionTypeResult TWO{ MAREA_expressionTypeOf(second, SOURCE, MAREA_AST_KIND_MEMBER_ACCESS, 0) };

    ASSERT_EQ(ONE.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The first run must resolve.";

    EXPECT_EQ(ONE.text, "array<float32,2>") << "A nested argument must be substituted whole.";

    EXPECT_EQ(ONE.text, TWO.text) << "Two runs must give the same text.";
}

TEST(MAREASemaTypedAstExpressions, BuildingTheTypedTreeAddsNoDiagnostic)
{
    GTEST_COUT << "Validating building the typed tree leaves the diagnostics of the check unchanged." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; function f(a: sinteger32, b: sinteger32): sinteger32 return a + b; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The source must check.";

    const MAREA_IntegerSize BEFORE{ MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink) };

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    EXPECT_EQ(MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink), BEFORE) << "Building the typed tree must add no diagnostic.";
}
