// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    // INFO: a structure with two blocks of functions: a method that takes self and defaults, and a static function.
    const std::string VECTOR{ "structure Vector2 x: float64; y: float64; end "
                              "implementation Vector2 "
                              "function scale(#[semantics = reference_mutable] self: Vector2, factor: float64 = 2.0, bias: float64 = 0.5) self.x *= factor; end "
                              "function make(x: float64, y: float64 = 4.0): Vector2 return Vector2{ x = x, y = y }; end "
                              "function plain(x: float64): float64 return x; end "
                              "end " };

    // The call of the first statement of the function named run, as a render, or a marker.
    const MAREA_IrNode *MAREA_findCall(const MAREA_IrNode *node, const std::string &calleeMember)
    {
        if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_CALL_EXPRESSION) {
            const MAREA_IrNode *callee{ MAREA_irChild(node, 0) };

            const MAREA_IrNode *member{ (MAREA_IrNodeGetKind(callee) == MAREA_IR_KIND_MEMBER_ACCESS) ? MAREA_irChild(callee, 1) : callee };

            if (MAREA_irText(member) == calleeMember) {
                return node;
            }
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(node); ++index) {
            const MAREA_IrNode *found{ MAREA_findCall(MAREA_irChild(node, index), calleeMember) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }

    // The argument list of the first call whose callee names member, as a render.
    std::string MAREA_argumentsOf(const std::string &source, const std::string &member)
    {
        MAREA_IrFixture fixture;

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *call{ MAREA_findCall(fixture.module, member) };

        if (call == nullptr) {
            return "<no call>";
        }

        return MAREA_irRender(MAREA_irChild(call, 1));
    }
} // namespace

TEST(MAREAIrCallDefaults, MethodCallThroughALocalFillsEveryOmittedDefault)
{
    GTEST_COUT << "Validating v.scale() fills both defaults, because the receiver is self." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run() local v: Vector2 = Vector2{ x = 1.0, y = 2.0 }; v.scale(); end", "scale"), "(ARGUMENT_LIST (FLOAT_LITERAL 2.0) (FLOAT_LITERAL 0.5))") << "The receiver is self, so both defaulted parameters are filled.";
}

TEST(MAREAIrCallDefaults, MethodCallFillsOnlyTheTrailingOmittedDefault)
{
    GTEST_COUT << "Validating v.scale(3.0) keeps the written argument and fills only the last default." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run() local v: Vector2 = Vector2{ x = 1.0, y = 2.0 }; v.scale(3.0); end", "scale"), "(ARGUMENT_LIST (FLOAT_LITERAL 3.0) (FLOAT_LITERAL 0.5))") << "The first default must stay replaced by the written argument.";
}

TEST(MAREAIrCallDefaults, MethodCallWithEveryArgumentIsUnchanged)
{
    GTEST_COUT << "Validating v.scale(3.0, 4.0) is unchanged." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run() local v: Vector2 = Vector2{ x = 1.0, y = 2.0 }; v.scale(3.0, 4.0); end", "scale"), "(ARGUMENT_LIST (FLOAT_LITERAL 3.0) (FLOAT_LITERAL 4.0))") << "A written argument is never replaced.";
}

TEST(MAREAIrCallDefaults, MethodCallThroughAParameterFillsItsDefaults)
{
    GTEST_COUT << "Validating a receiver that is a parameter finds the method as a local does." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run(#[semantics = reference_mutable] v: Vector2) v.scale(); end", "scale"), "(ARGUMENT_LIST (FLOAT_LITERAL 2.0) (FLOAT_LITERAL 0.5))") << "A parameter receiver must fill like a local.";
}

TEST(MAREAIrCallDefaults, MethodCallInsideAMethodThroughSelfFillsItsDefaults)
{
    GTEST_COUT << "Validating self.scale() inside another method of the same type fills its defaults." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "implementation Vector2 function twice(#[semantics = reference_mutable] self: Vector2) self.scale(); end end", "scale"), "(ARGUMENT_LIST (FLOAT_LITERAL 2.0) (FLOAT_LITERAL 0.5))") << "The receiver self has the type of the implementation.";
}

TEST(MAREAIrCallDefaults, StaticCallThroughTheTypeNameFillsItsDefault)
{
    GTEST_COUT << "Validating Vector2.make(1.0) fills the default of y, and no self is skipped." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run() local v: Vector2 = Vector2.make(1.0); end", "make"), "(ARGUMENT_LIST (FLOAT_LITERAL 1.0) (FLOAT_LITERAL 4.0))") << "A static call fills from its first parameter.";
}

TEST(MAREAIrCallDefaults, StaticCallWithAllArgumentsIsUnchanged)
{
    GTEST_COUT << "Validating Vector2.make(1.0, 2.0) is unchanged." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run() local v: Vector2 = Vector2.make(1.0, 2.0); end", "make"), "(ARGUMENT_LIST (FLOAT_LITERAL 1.0) (FLOAT_LITERAL 2.0))") << "Nothing is omitted.";
}

TEST(MAREAIrCallDefaults, MethodThatTakesTheReceiverExplicitlyThroughTheTypeNameSkipsNothing)
{
    GTEST_COUT << "Validating Vector2.scale(v) is a plain call: self is written, so the two defaults are filled." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "function run(#[semantics = reference_mutable] v: Vector2) Vector2.scale(v); end", "scale"), "(ARGUMENT_LIST (IDENTIFIER v) (FLOAT_LITERAL 2.0) (FLOAT_LITERAL 0.5))") << "A call through the type name writes self itself.";
}

TEST(MAREAIrCallDefaults, EachTypePicksItsOwnFunctionOfTheSameName)
{
    GTEST_COUT << "Validating two types with a function of the same name each get the default of their own." << std::endl;

    const std::string SOURCE{ "namespace main; structure A v: sinteger32; end structure B v: sinteger32; end "
                              "implementation A function make(n: sinteger32 = 1): A return A{ v = n }; end end "
                              "implementation B function make(n: sinteger32 = 2): B return B{ v = n }; end end "
                              "function run() local a: A = A.make(); local b: B = B.make(); end" };

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower(SOURCE), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *first{ MAREA_findCall(fixture.module, "make") };

    ASSERT_NE(first, nullptr) << "The unit must hold a call of make.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(first, 1)), "(ARGUMENT_LIST (INTEGER_LITERAL 1))") << "A.make() must get the default of A.";

    const MAREA_IrNode *run{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

    const MAREA_IrNode *secondLocal{ MAREA_irChild(MAREA_irChild(run, 1), 1) };

    const MAREA_IrNode *second{ MAREA_findCall(secondLocal, "make") };

    ASSERT_NE(second, nullptr) << "The second local must hold a call of make.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(second, 1)), "(ARGUMENT_LIST (INTEGER_LITERAL 2))") << "B.make() must get the default of B.";
}

TEST(MAREAIrCallDefaults, EnumMethodCallFillsItsDefault)
{
    GTEST_COUT << "Validating a method of an enum fills the default through a value of that enum." << std::endl;

    const std::string SOURCE{ "namespace main; enum Color Red; Green; end "
                              "implementation Color function shade(self: Color, level: sinteger32 = 3): sinteger32 return level; end end "
                              "function run(): sinteger32 local c: Color = Color.Red; return c.shade(); end" };

    EXPECT_EQ(MAREA_argumentsOf(SOURCE, "shade"), "(ARGUMENT_LIST (INTEGER_LITERAL 3))") << "An enum receiver is self too.";
}

TEST(MAREAIrCallDefaults, CallbackFieldIsNotAMethodAndFillsNothing)
{
    GTEST_COUT << "Validating a call of a field of a function type is not matched to a method and keeps its arguments." << std::endl;

    const std::string SOURCE{ "namespace main; type Step = function(n: sinteger32): sinteger32; structure Holder cb: Step; end "
                              "implementation Holder function cb_other(self: Holder, n: sinteger32 = 9): sinteger32 return n; end end "
                              "function run(h: Holder): sinteger32 return h.cb(1); end" };

    EXPECT_EQ(MAREA_argumentsOf(SOURCE, "cb"), "(ARGUMENT_LIST (INTEGER_LITERAL 1))") << "A callback field has no default to fill.";
}

TEST(MAREAIrCallDefaults, StaticFunctionCalledThroughAValueFillsNothing)
{
    GTEST_COUT << "Validating a function with no self called through a value is not guessed: it keeps the written arguments." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; " + VECTOR + "function run(v: Vector2): float64 return v.plain(1.0); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *call{ MAREA_findCall(fixture.module, "plain") };

    ASSERT_NE(call, nullptr) << "The unit must hold a call of plain.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(call, 1)), "(ARGUMENT_LIST (FLOAT_LITERAL 1.0))") << "With no self, the receiver is not a parameter, so nothing is guessed.";
}

TEST(MAREAIrCallDefaults, CallOfAnImportedModuleFunctionStaysUnfilled)
{
    GTEST_COUT << "Validating a call through an import of another unit keeps its written arguments: the ir has no link to that unit." << std::endl;

    MAREA_ParseFixture other;

    ASSERT_EQ(other.MAREA_parse("namespace other; function add(a: sinteger32, b: sinteger32 = 5): sinteger32 return a + b; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The other unit must parse.";

    const MAREA_AstNode *otherRoot{ other.root };

    MAREA_IrFixture fixture([otherRoot](const std::string &text, const MAREA_AstNode **outRoot) {
        if (text == "other") {
            *outRoot = otherRoot;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        }

        return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    });

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import other; function run(): sinteger32 return other.add(1); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *call{ MAREA_findCall(fixture.module, "add") };

    ASSERT_NE(call, nullptr) << "The unit must hold the call.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(call, 1)), "(ARGUMENT_LIST (INTEGER_LITERAL 1))") << "The default lives in another unit, so the ir leaves the call as written.";

    EXPECT_TRUE(fixture.MAREA_irCodes().empty()) << "The unfilled call raises no ir diagnostic.";
}

TEST(MAREAIrCallDefaults, StaticCallThroughATypeAliasKeepsItsWrittenArguments)
{
    GTEST_COUT << "Validating Alias.make(1.0), where Alias is a local type alias, is not followed to the type: the call keeps its written arguments." << std::endl;

    EXPECT_EQ(MAREA_argumentsOf("namespace main; " + VECTOR + "type Alias = Vector2; function run() local v: Vector2 = Alias.make(1.0); end", "make"), "(ARGUMENT_LIST (FLOAT_LITERAL 1.0))") << "The ir does not follow a type alias written as the receiver of a static call.";
}
