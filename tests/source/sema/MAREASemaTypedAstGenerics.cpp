// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    struct MAREA_GenericTypeResult
    {
        MAREA_SemaTypedAstExcept except{ MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER };
        std::string text;
        bool found{ false };
    };

    void MAREA_collectGenericNodesByKind(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, std::vector<const MAREA_SemaTypedNode *> &out)
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
                MAREA_collectGenericNodesByKind(child, kind, out);
            }
        }
    }

    // Checks the source with zero errors, builds the typed tree and reads the resolved type of the nth node of a kind.
    MAREA_GenericTypeResult MAREA_genericTypeOfNth(const std::string &source, MAREA_AstKind kind, std::size_t nth)
    {
        MAREA_SemaFixture fixture;

        MAREA_GenericTypeResult result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return result;
        }

        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collectGenericNodesByKind(root, kind, nodes);

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

    void MAREA_expectGenericType(const std::string &source, MAREA_AstKind kind, std::size_t nth, const std::string &expected)
    {
        const MAREA_GenericTypeResult RESULT{ MAREA_genericTypeOfNth(source, kind, nth) };

        ASSERT_TRUE(RESULT.found) << "The node must be in the typed tree of: " << source;

        ASSERT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The node must have a resolved type in: " << source;

        EXPECT_EQ(RESULT.text, expected) << "The resolved type is wrong in: " << source;
    }

    void MAREA_expectGenericNoType(const std::string &source, MAREA_AstKind kind, std::size_t nth)
    {
        const MAREA_GenericTypeResult RESULT{ MAREA_genericTypeOfNth(source, kind, nth) };

        ASSERT_TRUE(RESULT.found) << "The node must be in the typed tree of: " << source;

        EXPECT_EQ(RESULT.except, MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "The node must have no type in: " << source;
    }

    const std::string COMPARABLE{ "namespace main; contract Comparable function less(self: Self, other: Self): boolean; end " };

    const std::string PRINTABLE{ "contract Printable function describe(self: Self): string; end " };

    const std::string MAXIMUM{ "function maximum<Item>(left: Item, right: Item): Item where Item: Comparable, Copyable "
                               "if left.less(right) then return copy right; end return copy left; end " };
} // namespace

TEST(MAREASemaTypedAstGenerics, BareGenericResultIsInferredFromTheArgument)
{
    GTEST_COUT << "Validating maximum(a, b) with sinteger32 arguments is sinteger32." << std::endl;

    const std::string SOURCE{ COMPARABLE + MAXIMUM + "function run(a: sinteger32, b: sinteger32): sinteger32 return maximum(a, b); end" };

    // INFO: the first CALL_EXPRESSION in source order is left.less(right), inside maximum's own body; the second is
    // maximum(a, b) itself, in run.
    MAREA_expectGenericType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 1, "sinteger32");
}

TEST(MAREASemaTypedAstGenerics, ExplicitTypeArgumentsAreStillUnaffected)
{
    GTEST_COUT << "Validating an explicit type argument keeps substituting the way it already did, unchanged by inference." << std::endl;

    const std::string SOURCE{ COMPARABLE + MAXIMUM + "function run(a: sinteger32, b: sinteger32): sinteger32 return maximum<sinteger32>(a, b); end" };

    MAREA_expectGenericType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 1, "sinteger32");
}

TEST(MAREASemaTypedAstGenerics, CompoundParameterBindsThroughItsArguments)
{
    GTEST_COUT << "Validating a compound parameter such as demo.array<Item,4> binds Item from the matching argument." << std::endl;

    const std::string SOURCE{ "namespace main; require demo.array; require demo.vector; require demo.pointer; require demo.owned; require demo.slice; require demo.task; function first<Item>(values: demo.array<Item,4>): Item where Item: Copyable return copy values[0]; end "
                              "function run(data: demo.array<sinteger32,4>): sinteger32 return first(data); end" };

    MAREA_expectGenericType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 0, "sinteger32");
}

TEST(MAREASemaTypedAstGenerics, ConflictingBindingLeavesNoType)
{
    GTEST_COUT << "Validating two parameters of the same generic name bound to two different argument types leave the call untyped." << std::endl;

    const std::string SOURCE{ "namespace main; function same<Item>(a: Item, b: Item): Item where Item: Copyable return copy a; end "
                              "enum Left a; end enum Right b; end function run(x: Left, y: Right): sinteger32 return convert<sinteger32>(same(x, y)); end" };

    // INFO: the outer call in source order is convert<sinteger32>(...) itself (index 0); the inner call, same(x, y)
    // (index 1), binds Item to Left from x and to Right from y: a conflict, so it keeps no type of its own.
    MAREA_expectGenericNoType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 1);
}

TEST(MAREASemaTypedAstGenerics, ContractMethodCallThroughAGenericReceiverHasTheContractResult)
{
    GTEST_COUT << "Validating a method call on a receiver whose type is a generic parameter (left.less(right)) has the contract's result." << std::endl;

    const std::string SOURCE{ COMPARABLE + MAXIMUM + "function run(a: sinteger32, b: sinteger32): sinteger32 return maximum(a, b); end" };

    // INFO: the first CALL_EXPRESSION in source order inside maximum's own body is left.less(right).
    MAREA_expectGenericType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 0, "boolean");
}

TEST(MAREASemaTypedAstGenerics, ContractMethodCallReachesAnyContractTheWhereClauseLists)
{
    GTEST_COUT << "Validating a contract method is found across every contract a where clause lists, not only the first." << std::endl;

    const std::string SOURCE{ COMPARABLE + PRINTABLE + "function larger_description<Item>(left: Item, right: Item): string where Item: Comparable, Printable, Copyable "
                                                       "local winner: Item = copy left; return winner.describe(); end "
                                                       "function run(a: sinteger32, b: sinteger32): string return larger_description(a, b); end" };

    MAREA_expectGenericType(SOURCE, MAREA_AST_KIND_CALL_EXPRESSION, 0, "string");
}
