// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <map>
#include <memory>

namespace
{
    // A typed tree of one checked file, with the trees of the namespaces that the resolver answers FOUND for.
    struct MAREA_QualifierFixture
    {
        std::map<std::string, std::unique_ptr<MAREA_ParseFixture>> found;
        std::unique_ptr<MAREA_SemaFixture> sema;
        const MAREA_SemaTypedNode *root{ nullptr };

        // Parses the namespace trees, checks the main source with zero errors and builds the typed tree.
        bool MAREA_build(const std::string &source, const std::map<std::string, std::string> &namespaces = {})
        {
            for (const auto &entry : namespaces) {
                auto fixture{ std::make_unique<MAREA_ParseFixture>() };

                if (fixture->MAREA_parse(entry.second) != MAREA_PARSE_EXCEPT_SUCCESS) {
                    return false;
                }

                found[entry.first] = std::move(fixture);
            }

            sema = std::make_unique<MAREA_SemaFixture>([this](const std::string &text, const MAREA_AstNode **outRoot) {
                const auto FOUND{ found.find(text) };

                if (FOUND != found.end()) {
                    *outRoot = FOUND->second->root;

                    return MAREA_SEMA_IMPORT_RESULT_FOUND;
                }

                return (text.rfind("c.", 0) == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
            });

            if ((sema->MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (sema->MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
                return false;
            }

            return MAREA_SemaBuildTypedAst(sema->context, sema->parser.root, &root) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
        }
    };

    void MAREA_collect(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, std::vector<const MAREA_SemaTypedNode *> &out)
    {
        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            out.push_back(node);
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_collect(child, kind, out);
            }
        }
    }

    const MAREA_SemaTypedNode *MAREA_nth(const MAREA_QualifierFixture &fixture, MAREA_AstKind kind, std::size_t nth)
    {
        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collect(fixture.root, kind, nodes);

        return (nth < nodes.size()) ? nodes[nth] : nullptr;
    }

    const MAREA_SemaTypedNode *MAREA_childOf(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index)
    {
        const MAREA_SemaTypedNode *child{ nullptr };

        return (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) ? child : nullptr;
    }

    // The resolved type text of a node, or "<none>" when the accessor reports no type.
    std::string MAREA_typeOf(const MAREA_SemaTypedNode *node)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return "<none>";
        }

        return std::string(text, length);
    }

    const char *const MAREA_PLANET =
        "namespace main;\n"
        "enum Planet Mercury(mass: float64 = 1.0); Earth(mass: float64 = 2.0); end\n"
        "implementation Planet\n"
        "  constant GRAVITY: float64 = 6.0;\n"
        "  function weight(self: Planet): float64 return Planet.GRAVITY; end\n"
        "end\n"
        "function f(): Planet return Planet.Earth; end\n";

    const char *const MAREA_SHAPES =
        "namespace lib.shapes;\n"
        "structure Box width: float64; end\n"
        "#[visibility = public] constant LIMIT: sinteger32 = 10;\n"
        "#[visibility = public] function area(width: float64, height: float64): float64 return width * height; end\n"
        "#[visibility = public] function make(side: float64): Box return Box{ width = side }; end\n"
        "#[visibility = public] function identity<Item>(value: Item): Item return value; end\n"
        "#[visibility = private] function hidden(): sinteger32 return 1; end\n";
} // namespace

TEST(MAREASemaTypedAstQualifiers, NullNodeIsNotAQualifier)
{
    GTEST_COUT << "Validating that a null node is not a qualifier." << std::endl;

    EXPECT_FALSE(MAREA_SemaTypedNodeIsQualifier(nullptr)) << "A null node must not be a qualifier.";
}

TEST(MAREASemaTypedAstQualifiers, EnumNameOfAVariantIsAQualifier)
{
    GTEST_COUT << "Validating that the name of an enum of the file, read before a variant, is a qualifier with no type." << std::endl;

    MAREA_QualifierFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build(MAREA_PLANET)) << "The source must check and build.";

    // INFO: member accesses in source order: Planet.GRAVITY, then Planet.Earth.
    const MAREA_SemaTypedNode *variant{ MAREA_nth(fixture, MAREA_AST_KIND_MEMBER_ACCESS, 1) };

    ASSERT_NE(variant, nullptr);

    const MAREA_SemaTypedNode *name{ MAREA_childOf(variant, 0) };

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(name)) << "Planet before a variant must be a qualifier.";

    EXPECT_EQ(MAREA_typeOf(name), "<none>") << "A qualifier has no resolved type.";

    EXPECT_EQ(MAREA_typeOf(variant), "Planet") << "A variant has the type of its enum.";

    EXPECT_FALSE(MAREA_SemaTypedNodeIsQualifier(variant)) << "A member access is never a qualifier.";
}

TEST(MAREASemaTypedAstQualifiers, EnumNameOfAnEnumConstantIsAQualifier)
{
    GTEST_COUT << "Validating that Planet.GRAVITY is a float64 and Planet is a qualifier." << std::endl;

    MAREA_QualifierFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build(MAREA_PLANET)) << "The source must check and build.";

    const MAREA_SemaTypedNode *constant{ MAREA_nth(fixture, MAREA_AST_KIND_MEMBER_ACCESS, 0) };

    ASSERT_NE(constant, nullptr);

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(constant, 0))) << "Planet before a constant must be a qualifier.";

    EXPECT_EQ(MAREA_typeOf(constant), "float64") << "A constant of an implementation block has its declared type.";
}

TEST(MAREASemaTypedAstQualifiers, StructureNameOfAStaticCallIsAQualifier)
{
    GTEST_COUT << "Validating that the name of a structure before a static call is a qualifier and the call has the result type." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "structure Vector2 x: float64; end\n"
                              "implementation Vector2 function origin(): Vector2 return Vector2{ x = 0.0 }; end end\n"
                              "function f(): Vector2 return Vector2.origin(); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE)) << "The source must check and build.";

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    const MAREA_SemaTypedNode *member{ MAREA_childOf(call, 0) };

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(member, 0))) << "Vector2 before a static function must be a qualifier.";

    EXPECT_EQ(MAREA_typeOf(call), "Vector2") << "A static call has the result type of the function.";
}

TEST(MAREASemaTypedAstQualifiers, AValueBeforeAMemberIsNotAQualifier)
{
    GTEST_COUT << "Validating that a parameter, a local and a loop name before a member are not qualifiers." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "require c.array;\n"
                              "structure Point x: float64; end\n"
                              "function f(point: Point, items: c.array<Point>): float64\n"
                              "  local origin: Point = Point{ x = 1.0 };\n"
                              "  local total: float64 = 0.0;\n"
                              "  for each item in items do\n"
                              "    total += item.x;\n"
                              "  end\n"
                              "  return point.x + origin.x + total;\n"
                              "end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE)) << "The source must check and build.";

    std::vector<const MAREA_SemaTypedNode *> members;

    MAREA_collect(fixture.root, MAREA_AST_KIND_MEMBER_ACCESS, members);

    ASSERT_EQ(members.size(), std::size_t{ 3 });

    for (const MAREA_SemaTypedNode *member : members) {
        EXPECT_FALSE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(member, 0))) << "A value is not a qualifier.";
    }
}

TEST(MAREASemaTypedAstQualifiers, ExternalNamespaceIsAQualifierAndItsMembersKeepNoType)
{
    GTEST_COUT << "Validating that an EXTERNAL namespace name is a qualifier and the call on it stays untyped." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import c.stdio as stdio;\n"
                              "function f(text: string): sinteger32 return stdio.puts(text); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE)) << "The source must check and build.";

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    const MAREA_SemaTypedNode *member{ MAREA_childOf(call, 0) };

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(member, 0))) << "stdio must be a qualifier.";

    EXPECT_EQ(MAREA_typeOf(member), "<none>") << "A member of an EXTERNAL namespace stays untyped.";

    EXPECT_EQ(MAREA_typeOf(call), "<none>") << "A call on an EXTERNAL namespace stays untyped.";
}

TEST(MAREASemaTypedAstQualifiers, NameWithNoDeclarationBeforeAMemberIsAQualifier)
{
    GTEST_COUT << "Validating that a name that an import binds before a member is a qualifier." << std::endl;

    MAREA_QualifierFixture fixture;

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport c.stdio as stdio;\nfunction f(text: string) stdio.puts(text); end\n")) << "The source must check and build.";

    const MAREA_SemaTypedNode *member{ MAREA_nth(fixture, MAREA_AST_KIND_MEMBER_ACCESS, 0) };

    ASSERT_NE(member, nullptr);

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(member, 0))) << "stdio must be a qualifier.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceFunctionHasItsDeclaredType)
{
    GTEST_COUT << "Validating that a function reached through a FOUND namespace has its declared function type and result." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "function f(): float64 return shapes.area(1.0, 2.0); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    const MAREA_SemaTypedNode *member{ MAREA_childOf(call, 0) };

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(member, 0))) << "shapes must be a qualifier.";

    EXPECT_EQ(MAREA_typeOf(member), "function(float64,float64):float64") << "The member is the function type of the declaration.";

    EXPECT_EQ(MAREA_typeOf(call), "float64") << "The call has the declared result.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceConstantHasItsDeclaredType)
{
    GTEST_COUT << "Validating that a constant reached through a FOUND namespace has its declared type." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "function f(): sinteger32 return shapes.LIMIT; end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_MEMBER_ACCESS, 0)), "sinteger32") << "The constant has its declared type.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceThroughAnAliasAndThroughAMemberImport)
{
    GTEST_COUT << "Validating a FOUND namespace read through an import alias, and one symbol imported by name." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as sh;\n"
                              "import lib.shapes.area as compute;\n"
                              "function f(): float64 return sh.area(1.0, 2.0) + compute(1.0, 2.0); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    const MAREA_SemaTypedNode *aliased{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    const MAREA_SemaTypedNode *imported{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 1) };

    ASSERT_NE(aliased, nullptr);
    ASSERT_NE(imported, nullptr);

    EXPECT_TRUE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(MAREA_childOf(aliased, 0), 0))) << "The alias sh must be a qualifier.";

    EXPECT_EQ(MAREA_typeOf(aliased), "float64") << "A call through an alias has the declared result.";

    EXPECT_FALSE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(imported, 0))) << "A function imported by name is a value, not a qualifier.";

    EXPECT_EQ(MAREA_typeOf(imported), "float64") << "A call of an imported function has the declared result.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceMemberThatIsPrivateIsRejectedBeforeTheTypedTree)
{
    GTEST_COUT << "Validating that a private function of a FOUND namespace is rejected, so the typed tree is never built for it." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "function f(): sinteger32 return shapes.hidden(); end\n" };

    // INFO: the checker raises PRIVATE_SYMBOL_ACCESS, and the typed tree is built only for a file with no error.
    EXPECT_FALSE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "A private member must not check clean, so no typed tree is built.";

    EXPECT_EQ(fixture.sema->MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_SYMBOL_ACCESS }) << "The only error must be PRIVATE_SYMBOL_ACCESS.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceResultWrittenInThatFileIsQualified)
{
    GTEST_COUT << "Validating that a result type written in a FOUND file is spelled with the namespace, even when this file has the same name." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "structure Box height: float64; end\n"
                              "function f() local box: shapes.Box = shapes.make(1.0); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0)), "shapes.Box") << "The type of the other file is not the Box of this file.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceGenericResultTakesTheTypeArgumentOfTheCall)
{
    GTEST_COUT << "Validating that a generic function of a FOUND namespace takes the explicit type argument of the call." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "function f(): sinteger32 return shapes.identity<sinteger32>(5); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0)), "sinteger32") << "The type argument of the call fills the parameter.";
}

TEST(MAREASemaTypedAstQualifiers, ALocalNameHidesAFoundNamespace)
{
    GTEST_COUT << "Validating that a local with the name of a namespace is a value and is not a qualifier." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "structure Point x: float64; end\n"
                              "function f(shapes: Point): float64 return shapes.x; end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    const MAREA_SemaTypedNode *member{ MAREA_nth(fixture, MAREA_AST_KIND_MEMBER_ACCESS, 0) };

    EXPECT_FALSE(MAREA_SemaTypedNodeIsQualifier(MAREA_childOf(member, 0))) << "A parameter hides the namespace.";

    EXPECT_EQ(MAREA_typeOf(member), "float64") << "The field of the parameter is read, not the namespace.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceGenericResultWithNoTypeArgumentHasNoType)
{
    GTEST_COUT << "Validating that a generic result with no explicit type argument has no type, never a guess." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "function f(): sinteger32 return shapes.identity(5); end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0)), "<none>") << "A type argument that is only inferred is not guessed.";
}

TEST(MAREASemaTypedAstQualifiers, FoundNamespaceAliasOfATypeIsFollowed)
{
    GTEST_COUT << "Validating that a local type alias of the other file is replaced by the type it names." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "function f(): float64 return shapes.distance(); end\n" };

    const std::string SHAPES{ std::string(MAREA_SHAPES) + "type Meters = float64;\n#[visibility = public] function distance(): Meters return 1.0; end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", SHAPES } })) << "The source must check and build.";

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0)), "float64") << "The alias of the other file is followed to its target.";
}

TEST(MAREASemaTypedAstQualifiers, MemberOfAResultWrittenInAnotherFileIsNotReadInThisFile)
{
    GTEST_COUT << "Validating that a field of a type written in another file is not read from the same name in this file." << std::endl;

    MAREA_QualifierFixture fixture;

    const std::string SOURCE{ "namespace main;\n"
                              "import lib.shapes as shapes;\n"
                              "structure Box width: sinteger8; end\n"
                              "function f(): sinteger8 return shapes.make(1.0).width; end\n" };

    ASSERT_TRUE(fixture.MAREA_build(SOURCE, { { "lib.shapes", MAREA_SHAPES } })) << "The source must check and build.";

    // INFO: the outer member access comes first in the walk: shapes.make(1.0).width, then shapes.make.
    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_MEMBER_ACCESS, 0)), "<none>") << "The Box of this file must not give the type of a field of the Box of the other file.";
}
