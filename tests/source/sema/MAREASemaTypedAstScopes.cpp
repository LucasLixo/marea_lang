// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREALibraryRegistryHelpers.hpp"
#include "tests/MAREASemaHelpers.hpp"

namespace
{
    struct MAREA_ScopeTypeResult
    {
        bool found{ false };
        bool typed{ false };
        std::string text;
    };

    void MAREA_collectIdentifiers(const MAREA_SemaTypedNode *node, const std::string &name, std::vector<const MAREA_SemaTypedNode *> &out)
    {
        if (node == nullptr) {
            return;
        }

        const MAREA_AstNode *ast{ MAREA_SemaTypedNodeGetAstNode(node) };

        if ((MAREA_AstNodeGetKind(ast) == MAREA_AST_KIND_IDENTIFIER) && (std::string(MAREA_AstNodeGetText(ast), MAREA_AstNodeGetTextLength(ast)) == name)) {
            out.push_back(node);
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_collectIdentifiers(child, name, out);
            }
        }
    }

    // Checks the source clean, builds the typed tree and reads the type of the nth identifier with the name, in source order. A declared name counts as an identifier.
    MAREA_ScopeTypeResult MAREA_scopeTypeOf(const std::string &source, const std::string &name, std::size_t nth, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_SemaFixture fixture(resolve);

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, MAREA_buildLibraryRegistry(fixture.parser.lexer.arena)));

        MAREA_ScopeTypeResult result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            ADD_FAILURE() << "The source must check clean: " << source;

            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            ADD_FAILURE() << "The typed tree must build: " << source;

            return result;
        }

        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collectIdentifiers(root, name, nodes);

        if (nth >= nodes.size()) {
            return result;
        }

        result.found = true;

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        result.typed = (MAREA_SemaTypedNodeGetResolvedType(nodes[nth], &text, &length) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

        if (result.typed) {
            result.text = std::string(text, length);
        }

        return result;
    }

    void MAREA_expectScopeType(const std::string &source, const std::string &name, std::size_t nth, const std::string &expected, MAREA_ResolveCallback resolve = nullptr)
    {
        const MAREA_ScopeTypeResult RESULT{ MAREA_scopeTypeOf(source, name, nth, resolve) };

        ASSERT_TRUE(RESULT.found) << "The identifier " << name << " number " << nth << " must be in the typed tree of: " << source;

        ASSERT_TRUE(RESULT.typed) << "The identifier " << name << " number " << nth << " must have a resolved type in: " << source;

        EXPECT_EQ(RESULT.text, expected) << "The resolved type of " << name << " is wrong in: " << source;
    }

    void MAREA_expectScopeNoType(const std::string &source, const std::string &name, std::size_t nth, MAREA_ResolveCallback resolve = nullptr)
    {
        const MAREA_ScopeTypeResult RESULT{ MAREA_scopeTypeOf(source, name, nth, resolve) };

        ASSERT_TRUE(RESULT.found) << "The identifier " << name << " number " << nth << " must be in the typed tree of: " << source;

        EXPECT_FALSE(RESULT.typed) << "The identifier " << name << " number " << nth << " must have no type in: " << source;
    }

    const MAREA_SemaTypedNode *MAREA_findStatement(const MAREA_SemaTypedNode *node, MAREA_AstKind kind)
    {
        if (node == nullptr) {
            return nullptr;
        }

        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            return node;
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                continue;
            }

            const MAREA_SemaTypedNode *found{ MAREA_findStatement(child, kind) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }

    // Checks the source clean, builds the typed tree and reads the type of one direct child of the first statement of a kind.
    MAREA_ScopeTypeResult MAREA_headerChildTypeOf(const std::string &source, MAREA_AstKind kind, MAREA_IntegerSize childIndex, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_SemaFixture fixture(resolve);

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, MAREA_buildLibraryRegistry(fixture.parser.lexer.arena)));

        MAREA_ScopeTypeResult result;

        if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
            ADD_FAILURE() << "The source must check clean: " << source;

            return result;
        }

        const MAREA_SemaTypedNode *root{ nullptr };

        if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            ADD_FAILURE() << "The typed tree must build: " << source;

            return result;
        }

        const MAREA_SemaTypedNode *statement{ MAREA_findStatement(root, kind) };

        const MAREA_SemaTypedNode *child{ nullptr };

        if ((statement == nullptr) || (MAREA_SemaTypedNodeGetChildAt(statement, childIndex, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
            return result;
        }

        result.found = true;

        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        result.typed = (MAREA_SemaTypedNodeGetResolvedType(child, &text, &length) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS);

        if (result.typed) {
            result.text = std::string(text, length);
        }

        return result;
    }

    void MAREA_expectHeaderType(const std::string &source, MAREA_AstKind kind, MAREA_IntegerSize childIndex, const std::string &expected, MAREA_ResolveCallback resolve = nullptr)
    {
        const MAREA_ScopeTypeResult RESULT{ MAREA_headerChildTypeOf(source, kind, childIndex, resolve) };

        ASSERT_TRUE(RESULT.found) << "The child " << childIndex << " must be in the typed tree of: " << source;

        ASSERT_TRUE(RESULT.typed) << "The child " << childIndex << " must have a resolved type in: " << source;

        EXPECT_EQ(RESULT.text, expected) << "The resolved type of the child " << childIndex << " is wrong in: " << source;
    }

    void MAREA_expectHeaderNoType(const std::string &source, MAREA_AstKind kind, MAREA_IntegerSize childIndex, MAREA_ResolveCallback resolve = nullptr)
    {
        const MAREA_ScopeTypeResult RESULT{ MAREA_headerChildTypeOf(source, kind, childIndex, resolve) };

        ASSERT_TRUE(RESULT.found) << "The child " << childIndex << " must be in the typed tree of: " << source;

        EXPECT_FALSE(RESULT.typed) << "The child " << childIndex << " must have no type in: " << source;
    }

    const MAREA_ResolveCallback MAREA_EXTERNAL_RESOLVE{ [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; } };
} // namespace

TEST(MAREASemaTypedAstScopes, RangeCounterHasTheTypeOfTheTypedBound)
{
    GTEST_COUT << "Validating the counter of a for range has the type of the bounds, and a number literal bound never decides." << std::endl;

    MAREA_expectScopeType("namespace main; function f(n: uinteger64): uinteger64 local t: uinteger64 = 0; for i in 0 .. n do t += i; end return t; end", "i", 1, "uinteger64");

    MAREA_expectScopeType("namespace main; function f(a: sinteger8, b: sinteger8): sinteger8 local t: sinteger8 = 0; for i in a .. b do t += i; end return t; end", "i", 1, "sinteger8");

    MAREA_expectScopeType("namespace main; function f(first: float64): float64 local t: float64 = 0.0; for i in first .. 10.0 do t += i; end return t; end", "i", 1, "float64");
}

TEST(MAREASemaTypedAstScopes, RangeCounterOfTwoLiteralsIsTheDefault)
{
    GTEST_COUT << "Validating the counter of a for range with two literal bounds has the default integer type." << std::endl;

    MAREA_expectScopeType("namespace main; function f(): sinteger32 local t: sinteger32 = 0; for i in 0 .. 3 do t += i; end return t; end", "i", 1, "sinteger32");
}

TEST(MAREASemaTypedAstScopes, RangeCounterOfAnUntypedBoundHasNoType)
{
    GTEST_COUT << "Validating the counter of a for range is not guessed when the bound that decides has no type." << std::endl;

    MAREA_expectScopeNoType("namespace main; import c.limits as limits; function f(): sinteger32 local t: sinteger32 = 0; for i in 0 .. limits.top() do t += i; end return t; end", "i", 1, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, DeclaredNamesOfALoopHaveTheTypeOfTheirLoop)
{
    GTEST_COUT << "Validating the declared name of a for range and of a for each carries the type its loop gives it." << std::endl;

    MAREA_expectScopeType("namespace main; function f(n: sinteger32): sinteger32 local t: sinteger32 = 0; for i in 0 .. n do t += i; end return t; end", "i", 0, "sinteger32");

    MAREA_expectScopeType("namespace main; function f(v: array<float32, 4>): float32 local t: float32 = 0.0; for each k, x in v do t += x; end return t; end", "k", 0, "uinteger64");

    MAREA_expectScopeType("namespace main; function f(v: array<float32, 4>): float32 local t: float32 = 0.0; for each k, x in v do t += x; end return t; end", "x", 0, "float32");
}

TEST(MAREASemaTypedAstScopes, ForRangeCounterChildHasTheTypeOfTheBounds)
{
    GTEST_COUT << "Validating the counter child of a for range has the type of its bounds, read from the statement itself." << std::endl;

    MAREA_expectHeaderType("namespace main; function f(n: uinteger64): uinteger64 local t: uinteger64 = 0; for i in 0 .. n do t += i; end return t; end", MAREA_AST_KIND_FOR_RANGE_STATEMENT, 0, "uinteger64");

    MAREA_expectHeaderType("namespace main; function f(a: sinteger8, b: sinteger8): sinteger8 local t: sinteger8 = 0; for i in a .. b do t += i; end return t; end", MAREA_AST_KIND_FOR_RANGE_STATEMENT, 0, "sinteger8");

    MAREA_expectHeaderType("namespace main; function f(): sinteger32 local t: sinteger32 = 0; for i in 0 .. 3 do t += i; end return t; end", MAREA_AST_KIND_FOR_RANGE_STATEMENT, 0, "sinteger32");
}

TEST(MAREASemaTypedAstScopes, ForRangeBoundsKeepTheirType)
{
    GTEST_COUT << "Validating the two bounds of a for range have a resolved type that agrees with the counter." << std::endl;

    const std::string SOURCE{ "namespace main; function f(n: uinteger8): uinteger8 local t: uinteger8 = 0; for i in 0 .. n do t += i; end return t; end" };

    MAREA_expectHeaderType(SOURCE, MAREA_AST_KIND_FOR_RANGE_STATEMENT, 1, "uinteger8");

    MAREA_expectHeaderType(SOURCE, MAREA_AST_KIND_FOR_RANGE_STATEMENT, 2, "uinteger8");
}

TEST(MAREASemaTypedAstScopes, ForRangeCounterOfAnUntypedBoundHasNoTypeOnTheStatement)
{
    GTEST_COUT << "Validating the counter child of a for range is not guessed when the bound that decides has no type." << std::endl;

    MAREA_expectHeaderNoType("namespace main; import c.limits as limits; function f(): sinteger32 local t: sinteger32 = 0; for i in 0 .. limits.top() do t += i; end return t; end", MAREA_AST_KIND_FOR_RANGE_STATEMENT, 0, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, ForEachCollectionChildHasItsResolvedType)
{
    GTEST_COUT << "Validating the collection child of a for each carries the full resolved type of the collection." << std::endl;

    MAREA_expectHeaderType("namespace main; require demo.array; function f(v: demo.array<float32, 4>): float32 local t: float32 = 0.0; for each x in v do t += x; end return t; end", MAREA_AST_KIND_FOR_EACH_STATEMENT, 1, "demo.array<float32,4>", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectHeaderType("namespace main; require demo.vector; function f(v: demo.vector<uinteger8>): uinteger8 local t: uinteger8 = 0; for each k, x in v do t += x; end return t; end", MAREA_AST_KIND_FOR_EACH_STATEMENT, 2, "demo.vector<uinteger8>", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectHeaderType("namespace main; require demo.slice; function f(v: demo.slice<float32>): float32 local t: float32 = 0.0; for each x in mutable v do t += x; end return t; end", MAREA_AST_KIND_FOR_EACH_STATEMENT, 1, "demo.slice<float32>", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectHeaderType("namespace main; require c.array; import collections.array as array; function f(v: c.array<sinteger32>): sinteger32 local t: sinteger32 = 0; for each x in v do t += x; end return t; end", MAREA_AST_KIND_FOR_EACH_STATEMENT, 1, "c.array<sinteger32>", MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, ForEachNameChildrenHaveTheirType)
{
    GTEST_COUT << "Validating the value child of a for each has the element type and the key child has uinteger64, read from the statement itself." << std::endl;

    const std::string ONE{ "namespace main; function f(v: array<float32, 4>): float32 local t: float32 = 0.0; for each x in v do t += x; end return t; end" };

    MAREA_expectHeaderType(ONE, MAREA_AST_KIND_FOR_EACH_STATEMENT, 0, "float32");

    const std::string TWO{ "namespace main; function f(v: vector<sinteger16>): sinteger16 local t: sinteger16 = 0; local n: uinteger64 = 0; for each k, x in v do n += k; t += x; end return t; end" };

    MAREA_expectHeaderType(TWO, MAREA_AST_KIND_FOR_EACH_STATEMENT, 0, "uinteger64");

    MAREA_expectHeaderType(TWO, MAREA_AST_KIND_FOR_EACH_STATEMENT, 1, "sinteger16");
}

TEST(MAREASemaTypedAstScopes, ForEachNameChildrenOfAnUntypedCollectionHaveNoType)
{
    GTEST_COUT << "Validating a for each over a collection with no type leaves both loop names without a type, and never guesses the key." << std::endl;

    const std::string SOURCE{ "namespace main; import c.things as things; function consume(x: sinteger32, k: uinteger64) end function f() for each k, x in things.all() do consume(x, k); end end" };

    MAREA_expectHeaderNoType(SOURCE, MAREA_AST_KIND_FOR_EACH_STATEMENT, 0, MAREA_EXTERNAL_RESOLVE);

    MAREA_expectHeaderNoType(SOURCE, MAREA_AST_KIND_FOR_EACH_STATEMENT, 1, MAREA_EXTERNAL_RESOLVE);

    MAREA_expectHeaderNoType(SOURCE, MAREA_AST_KIND_FOR_EACH_STATEMENT, 2, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, NestedLoopsKeepTheirOwnNameTypes)
{
    GTEST_COUT << "Validating an inner loop that reuses an outer name types its own declared name, not the outer one." << std::endl;

    const std::string SOURCE{ "namespace main; function f(n: uinteger64, v: array<float32, 4>): float32 local t: float32 = 0.0; for i in 0 .. n do for each i in v do t += i; end end return t; end" };

    // The first declared i is the counter of the range, the second is the value of the for each.
    MAREA_expectScopeType(SOURCE, "i", 0, "uinteger64");

    MAREA_expectScopeType(SOURCE, "i", 1, "float32");
}

TEST(MAREASemaTypedAstScopes, ForEachValueIsTheElementType)
{
    GTEST_COUT << "Validating the value of a for each is the element type of an array, a vector and a slice." << std::endl;

    MAREA_expectScopeType("namespace main; require demo.array; function f(v: demo.array<float32, 4>): float32 local t: float32 = 0.0; for each x in v do t += x; end return t; end", "x", 1, "float32", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectScopeType("namespace main; require demo.vector; function f(v: demo.vector<uinteger8>): uinteger8 local t: uinteger8 = 0; for each x in v do t += x; end return t; end", "x", 1, "uinteger8", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectScopeType("namespace main; require demo.slice; function f(v: demo.slice<float32>): float32 local t: float32 = 0.0; for each x in mutable v do t += x; end return t; end", "x", 1, "float32", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectScopeType("namespace main; require c.array; import collections.array as array; function f(v: c.array<sinteger32>): sinteger32 local t: sinteger32 = 0; for each x in v do t += x; end return t; end", "x", 1, "sinteger32", MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, ForEachElementOfAGenericTypeIsKeptWhole)
{
    GTEST_COUT << "Validating the element of a collection of a generic type is spelled whole." << std::endl;

    MAREA_expectScopeType("namespace main; require c.array; import collections.array as array; import concurrency.task as task; function consume(x: task<float32>) end function f(v: c.array<task<float32>>) for each x in v do consume(x); end end", "x", 2, "task<float32>", MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, ForEachKeyIsTheIndexType)
{
    GTEST_COUT << "Validating the key of a for each with two names is uinteger64 and the value is the element." << std::endl;

    const std::string SOURCE{ "namespace main; function f(v: array<float32, 4>): float32 local t: float32 = 0.0; local n: uinteger64 = 0; for each k, x in v do n += k; t += x; end return t; end" };

    MAREA_expectScopeType(SOURCE, "k", 1, "uinteger64");

    MAREA_expectScopeType(SOURCE, "x", 1, "float32");
}

TEST(MAREASemaTypedAstScopes, ForEachOverAnUntypedCollectionHasNoType)
{
    GTEST_COUT << "Validating the value of a for each is not guessed when the collection has no type." << std::endl;

    MAREA_expectScopeNoType("namespace main; import c.things as things; function consume(x: sinteger32) end function f() for each x in things.all() do consume(x); end end", "x", 2, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, PartitionPartIsASliceOfTheValue)
{
    GTEST_COUT << "Validating a partition part is a slice of the element type of the value, whatever the collection." << std::endl;

    MAREA_expectScopeType("namespace main; require c.array; import collections.array as array; function consume(p: slice<float32>) end function f(#[semantics = reference_mutable] v: c.array<float32>) fork_join partition mutable v into 4 as part do consume(part); end end end", "part", 1, "slice<float32>", MAREA_EXTERNAL_RESOLVE);

    MAREA_expectScopeType("namespace main; function consume(p: slice<uinteger8>) end function f(v: array<uinteger8, 8>) fork_join partition v into 2 as part do consume(part); end end end", "part", 1, "slice<uinteger8>");

    MAREA_expectScopeType("namespace main; function consume(p: slice<sinteger32>) end function f(v: slice<sinteger32>) fork_join partition v into 2 as part do consume(part); end end end", "part", 1, "slice<sinteger32>");
}

TEST(MAREASemaTypedAstScopes, WithNameHasTheTypeOfItsExpression)
{
    GTEST_COUT << "Validating a with name has the type of its expression, and none when the expression has none." << std::endl;

    MAREA_expectScopeType("namespace main; function use(x: slice<float32>) end function f(v: slice<float32>) with v as view do use(view); end end", "view", 1, "slice<float32>");

    MAREA_expectScopeType("namespace main; function f(n: sinteger64): sinteger64 local t: sinteger64 = 0; with n as m do t += m; end return t; end", "m", 1, "sinteger64");

    MAREA_expectScopeNoType("namespace main; import c.gpu as gpu; function use(x: sinteger32) end function f() with gpu.map() as view do use(view); end end", "view", 1, MAREA_EXTERNAL_RESOLVE);
}

TEST(MAREASemaTypedAstScopes, InnerNameShadowsAnOuterOne)
{
    GTEST_COUT << "Validating a nested statement gives its own type to a name that hides an outer one, and the outer one is back at the end." << std::endl;

    const std::string SOURCE{ "namespace main; function f(n: uinteger64, v: array<float32, 4>): float32 local t: float32 = 0.0; local u: uinteger64 = 0; for i in 0 .. n do for each i in v do t += i; end u += i; end return t; end" };

    // The first use of the name is read by the inner loop, the second by the outer one.
    MAREA_expectScopeType(SOURCE, "i", 2, "float32");

    MAREA_expectScopeType(SOURCE, "i", 3, "uinteger64");
}

TEST(MAREASemaTypedAstScopes, NestedStatementsEachKeepTheirOwnName)
{
    GTEST_COUT << "Validating nested statements of different kinds type each of their names." << std::endl;

    const std::string SOURCE{ "namespace main; function consume(x: float32, k: uinteger8) end function f(n: uinteger8, v: slice<float32>) for i in 0 .. n do for each x in v do consume(x, i); end end end" };

    MAREA_expectScopeType(SOURCE, "x", 2, "float32");

    MAREA_expectScopeType(SOURCE, "i", 1, "uinteger8");
}

TEST(MAREASemaTypedAstScopes, ASiblingStatementDoesNotLeakItsName)
{
    GTEST_COUT << "Validating the name of a statement ends with it: a later local of the same name has its own type." << std::endl;

    const std::string SOURCE{ "namespace main; function f(n: uinteger64): sinteger8 local t: uinteger64 = 0; for i in 0 .. n do t += i; end local i: sinteger8 = 1; return i; end" };

    MAREA_expectScopeType(SOURCE, "i", 1, "uinteger64");

    MAREA_expectScopeType(SOURCE, "i", 3, "sinteger8");
}

TEST(MAREASemaTypedAstScopes, TwoLoopsInARowDoNotShareATypeBeforeTheirBounds)
{
    GTEST_COUT << "Validating a second loop of the same name is typed by its own bounds, not by the loop before it." << std::endl;

    const std::string SOURCE{ "namespace main; function f(a: uinteger64, b: sinteger8): sinteger8 local t: uinteger64 = 0; local u: sinteger8 = 0; for i in 0 .. a do t += i; end for i in 0 .. b do u += i; end return u; end" };

    MAREA_expectScopeType(SOURCE, "i", 1, "uinteger64");

    MAREA_expectScopeType(SOURCE, "i", 3, "sinteger8");
}

TEST(MAREASemaTypedAstScopes, ExpressionsMadeFromAScopeNameAreTyped)
{
    GTEST_COUT << "Validating an index access and an operation made from a loop name take its type." << std::endl;

    const std::string SOURCE{ "namespace main; function f(v: array<float32, 4>): float32 local t: float32 = 0.0; for i in 0 .. 3 do t += v[i] * v[i]; end return t; end" };

    MAREA_expectScopeType(SOURCE, "i", 1, "sinteger32");
}
