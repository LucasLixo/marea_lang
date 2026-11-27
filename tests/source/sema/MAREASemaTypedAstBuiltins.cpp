// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    struct MAREA_BuiltinTypeResult
    {
        MAREA_SemaTypedAstExcept except{ MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER };
        std::string text;
        bool found{ false };
    };

    void MAREA_collectNodesByKind(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, std::vector<const MAREA_SemaTypedNode *> &out)
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
                MAREA_collectNodesByKind(child, kind, out);
            }
        }
    }

    // Checks the source with zero errors, builds the typed tree and reads the resolved type of the nth node of a kind, in source order.
    MAREA_BuiltinTypeResult MAREA_typeOfNth(const std::string &source, MAREA_AstKind kind, std::size_t nth, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_SemaFixture fixture(resolve);

        MAREA_BuiltinTypeResult result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return result;
        }

        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collectNodesByKind(root, kind, nodes);

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

    void MAREA_expectType(const std::string &source, MAREA_AstKind kind, std::size_t nth, const std::string &expected, MAREA_ResolveCallback resolve = nullptr)
    {
        const MAREA_BuiltinTypeResult RESULT{ MAREA_typeOfNth(source, kind, nth, resolve) };

        ASSERT_TRUE(RESULT.found) << "The node must be in the typed tree of: " << source;

        ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The node must have a resolved type in: " << source;

        EXPECT_EQ(RESULT.text, expected) << "The resolved type is wrong in: " << source;
    }

    void MAREA_expectNoType(const std::string &source, MAREA_AstKind kind, std::size_t nth, MAREA_ResolveCallback resolve = nullptr)
    {
        const MAREA_BuiltinTypeResult RESULT{ MAREA_typeOfNth(source, kind, nth, resolve) };

        ASSERT_TRUE(RESULT.found) << "The node must be in the typed tree of: " << source;

        EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "The node must have no type in: " << source;
    }

    const MAREA_ResolveCallback MAREA_EXTERNAL_RESOLVE{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };
} // namespace

TEST(MAREASemaTypedAstBuiltins, CallWithoutResultIsVoid)
{
    GTEST_COUT << "Validating a call of a function with no result type has the type void." << std::endl;

    MAREA_expectType("namespace main; function g(a: sinteger32) end function f(x: sinteger32) g(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "void");
}

TEST(MAREASemaTypedAstBuiltins, CallOfAMethodWithoutResultIsVoid)
{
    GTEST_COUT << "Validating a call of a method or a function value with no result type has the type void." << std::endl;

    MAREA_expectType("namespace main; structure Counter value: sinteger32; end implementation Counter function reset(self: Counter) end end function f(c: Counter) c.reset(); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "void");

    MAREA_expectType("namespace main; function f(op: function(x: sinteger32), a: sinteger32) op(a); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "void");
}

TEST(MAREASemaTypedAstBuiltins, CallWithResultKeepsItsResult)
{
    GTEST_COUT << "Validating a call of a function with a result type still has that result." << std::endl;

    MAREA_expectType("namespace main; function g(a: sinteger32): float64 return 1.0; end function f(x: sinteger32): float64 return g(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "float64");
}

TEST(MAREASemaTypedAstBuiltins, CallOfAnImportStaysWithoutAType)
{
    GTEST_COUT << "Validating a call that sema cannot resolve (an import) is not given the type void." << std::endl;

    MAREA_expectNoType("namespace main; import c.stdio as stdio; function f(text: string) stdio.puts(text); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstBuiltins, SpawnOfAVoidCallStaysWithoutAType)
{
    GTEST_COUT << "Validating spawn of a call with no result has no type, not task<void>." << std::endl;

    MAREA_expectNoType("namespace main; function work(a: float32) end function f(a: float32) fork_join spawn work(a); end end", MAREA_AST_KIND_UNARY_EXPRESSION, 0);
}

TEST(MAREASemaTypedAstBuiltins, SizeOfAndAlignOfCallsAreUinteger64)
{
    GTEST_COUT << "Validating size_of<T>() and align_of<T>() have the type uinteger64." << std::endl;

    const std::string SOURCE{ "namespace main; structure Light x: float32; end constant LIGHT_SIZE: uinteger64 = size_of<Light>(); constant LIGHT_ALIGN: uinteger64 = align_of<Light>(); function f(): uinteger64 return size_of<sinteger32>() + align_of<sinteger32>(); end" };

    MAREA_expectType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 0, "uinteger64");

    MAREA_expectType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 1, "uinteger64");

    MAREA_expectType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 2, "uinteger64");

    MAREA_expectType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 3, "uinteger64");

    MAREA_expectType(SOURCE, MAREA_AST_KIND_BINARY_EXPRESSION, 0, "uinteger64");
}

TEST(MAREASemaTypedAstBuiltins, SizeOfOfAGenericParameterIsUinteger64)
{
    GTEST_COUT << "Validating size_of<Item>() with a generic parameter as the type has the type uinteger64." << std::endl;

    MAREA_expectType("namespace main; function weigh<Item>(a: Item): uinteger64 where Item: Copyable return size_of<Item>(); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "uinteger64");
}

TEST(MAREASemaTypedAstBuiltins, BuiltinNamesKeepNoTypeOfTheirOwn)
{
    GTEST_COUT << "Validating the name size_of or align_of as a callee has no type, and the call has the result." << std::endl;

    const std::string SOURCE{ "namespace main; function f(): uinteger64 return size_of<sinteger32>(); end" };

    MAREA_expectNoType(SOURCE, MAREA_AST_KIND_IDENTIFIER, 1);

    MAREA_expectType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 0, "uinteger64");
}

TEST(MAREASemaTypedAstBuiltins, ConvertStillGivesItsTarget)
{
    GTEST_COUT << "Validating convert<T>(x) is unchanged: it has the type T." << std::endl;

    MAREA_expectType("namespace main; function f(x: sinteger32): sinteger64 return convert<sinteger64>(x); end", MAREA_AST_KIND_CALL_EXPRESSION, 0, "sinteger64");
}
