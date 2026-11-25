// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_C{ [](const std::string &text, const MAREA_AstNode **) {
        return ((text.compare(0, 2, "c.") == 0) || (text.compare(0, 6, "marea.") == 0) || (text.compare(0, 5, "demo.") == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::string MAREA_dumpOf(const std::string &source)
    {
        MAREA_IrFixture fixture{ RESOLVE_C };

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<dump failed>";
        }

        return std::string(text, length);
    }

    bool MAREA_has(const std::string &text, const std::string &part)
    {
        return text.find(part) != std::string::npos;
    }

    std::string MAREA_resolvedOf(const MAREA_IrNode *node)
    {
        if (MAREA_IrNodeGetResolvedType(node) == nullptr) {
            return "";
        }

        return std::string(MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node));
    }

    bool MAREA_isTypedDeclaration(MAREA_IrKind kind)
    {
        return (kind == MAREA_IR_KIND_PARAMETER) || (kind == MAREA_IR_KIND_LOCAL_DECLARATION) || (kind == MAREA_IR_KIND_FIELD_DECLARATION) || (kind == MAREA_IR_KIND_VARIANT_FIELD) || (kind == MAREA_IR_KIND_CONSTANT_DECLARATION) || (kind == MAREA_IR_KIND_FUNCTION_DECLARATION);
    }

    // INFO: a declaration the ir invents has a leading dollar sign. Its type is covered by the loop tests.
    bool MAREA_isHidden(const MAREA_IrNode *node)
    {
        return MAREA_irText(node).compare(0, 1, "$") == 0;
    }

    // Collects every declaration whose type name is not the head of its resolved type, and every one that has a type name and no resolved type.
    void MAREA_collectMismatch(const MAREA_IrNode *node, const std::string &file, std::string &mismatch, std::size_t &checked)
    {
        if (MAREA_isTypedDeclaration(MAREA_IrNodeGetKind(node)) && (MAREA_isHidden(node) == false) && (MAREA_IrNodeGetTypeName(node) != nullptr)) {
            ++checked;

            const std::string NAME{ MAREA_irTypeName(node) };

            const std::string RESOLVED{ MAREA_resolvedOf(node) };

            const bool HEAD_MATCHES{ (RESOLVED.compare(0, NAME.size(), NAME) == 0) && ((RESOLVED.size() == NAME.size()) || (RESOLVED[NAME.size()] == '<')) };

            if (HEAD_MATCHES == false) {
                mismatch += file + ": " + MAREA_IrKindGetName(MAREA_IrNodeGetKind(node)) + " " + MAREA_irText(node) + " type=" + NAME + " resolved=" + RESOLVED + "\n";
            }
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(node); ++index) {
            MAREA_collectMismatch(MAREA_irChild(node, index), file, mismatch, checked);
        }
    }

    // Collects every declaration that sema gives no type: a function with no result type is the one expected case.
    void MAREA_collectUntyped(const MAREA_IrNode *node, const std::string &file, std::string &untyped)
    {
        const MAREA_IrKind KIND{ MAREA_IrNodeGetKind(node) };

        if (MAREA_isTypedDeclaration(KIND) && (KIND != MAREA_IR_KIND_FUNCTION_DECLARATION) && (MAREA_isHidden(node) == false) && (MAREA_IrNodeGetResolvedType(node) == nullptr)) {
            untyped += file + ": " + MAREA_IrKindGetName(KIND) + " " + MAREA_irText(node) + "\n";
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(node); ++index) {
            MAREA_collectUntyped(MAREA_irChild(node, index), file, untyped);
        }
    }
} // namespace

TEST(MAREAIrDeclarationTypes, EveryDeclarationKindKeepsTheTypeArgumentsInTheDump)
{
    GTEST_COUT << "Validating a constant, a field, a variant field, a parameter, a local and a result type keep array<sinteger32,4> in full, next to the head name." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import c.array as array; "
                                         "constant TABLE: array<sinteger32, 4> = array<sinteger32, 4>{ 1, 2, 3, 4 }; "
                                         "structure Holder items: array<sinteger32, 4>; end "
                                         "enum Shape Quad(corners: array<sinteger32, 4>); end "
                                         "function take(values: array<sinteger32, 4>): array<sinteger32, 4> local copy_of: array<sinteger32, 4> = values; return copy_of; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "CONSTANT_DECLARATION \"TABLE\" type=array resolved=array<sinteger32,4>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"items\" type=array resolved=array<sinteger32,4>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "VARIANT_FIELD \"corners\" type=array resolved=array<sinteger32,4>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"values\" type=array resolved=array<sinteger32,4>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "LOCAL_DECLARATION \"copy_of\" type=array resolved=array<sinteger32,4>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "FUNCTION_DECLARATION \"take\" type=array resolved=array<sinteger32,4>")) << DUMP;
}

TEST(MAREAIrDeclarationTypes, NestedTypeArgumentsStayWhole)
{
    GTEST_COUT << "Validating a type argument that holds a type argument is kept whole in the resolved type, and the type name stays the head." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import c.array as array; import c.vector as vector; function f(grid: array<array<sinteger32, 2>, 3>, rows: vector<array<float32, 2>>) end") };

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"grid\" type=array resolved=array<array<sinteger32,2>,3>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"rows\" type=vector resolved=vector<array<float32,2>>")) << DUMP;
}

TEST(MAREAIrDeclarationTypes, AliasOfAGenericTypeKeepsTheFullTypeItStandsFor)
{
    GTEST_COUT << "Validating a local alias of array<sinteger32, 4> gives the full type the alias stands for, not the alias name." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import c.array as array; type Grid = array<sinteger32, 4>; function f(g: Grid) local h: Grid = g; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"g\" type=array resolved=array<sinteger32,4>")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "LOCAL_DECLARATION \"h\" type=array resolved=array<sinteger32,4>")) << DUMP;
}

TEST(MAREAIrDeclarationTypes, FunctionTypeOnEveryDeclarationKindIsNotLost)
{
    GTEST_COUT << "Validating a function type, written or reached by an alias, keeps its full resolved type on a field, a parameter, a local, a variant field and a result, and has no type name." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; type Step = function(n: sinteger32): sinteger32; "
                                         "structure Holder inline_cb: function(n: sinteger32): sinteger32; alias_cb: Step; end "
                                         "enum Shape Pick(step: Step); end "
                                         "function pick(cb: function(n: sinteger32): sinteger32, s: Step): function(n: sinteger32): sinteger32 local l: Step = s; return l; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"inline_cb\" resolved=function(sinteger32):sinteger32\n")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"alias_cb\" resolved=function(sinteger32):sinteger32\n")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "VARIANT_FIELD \"step\" resolved=function(sinteger32):sinteger32\n")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"cb\" resolved=function(sinteger32):sinteger32\n")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"s\" resolved=function(sinteger32):sinteger32\n")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "LOCAL_DECLARATION \"l\" resolved=function(sinteger32):sinteger32\n")) << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "FUNCTION_DECLARATION \"pick\" resolved=function(sinteger32):sinteger32\n")) << DUMP;
}

TEST(MAREAIrDeclarationTypes, FunctionWithNoResultTypeHasNoTypeAndNoResolvedType)
{
    GTEST_COUT << "Validating a function with no result type is never given a guessed type." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; function nothing() end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FUNCTION_DECLARATION \"nothing\"\n")) << DUMP;
}

TEST(MAREAIrDeclarationTypes, ExamplesKeepTheFullTypeOfEveryWrittenType)
{
    GTEST_COUT << "Validating, in all the examples, that every declaration with a type name has a resolved type that starts with it, and every written type is typed." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    std::string mismatch;

    std::string untyped;

    std::size_t checked{ 0 };

    for (const MAREA_IrExample &file : EXAMPLES.files) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.MAREA_lower(file.source, file.relativePath), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must lower.";

        MAREA_collectMismatch(fixture.module, file.relativePath, mismatch, checked);

        MAREA_collectUntyped(fixture.module, file.relativePath, untyped);
    }

    GTEST_COUT << "Declarations with a type name checked: " << checked << std::endl;

    EXPECT_GT(checked, std::size_t{ 100 }) << "The sweep must reach many declarations.";

    EXPECT_TRUE(mismatch.empty()) << "A type name must be the head of the resolved type:\n"
                                  << mismatch;

    EXPECT_TRUE(untyped.empty()) << "A written type must have a resolved type:\n"
                                 << untyped;
}
