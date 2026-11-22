// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

#include "marea/internal/ir/MAREAIrInternal.h"

namespace
{
    const MAREA_Uinteger32 GENERIC{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC };

    using Codes = std::vector<MAREA_Uinteger32>;

    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        const bool EXTERNAL{ (text.compare(0, 12, "collections.") == 0) || (text.compare(0, 2, "c.") == 0) };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    const std::string PRELUDE{ "namespace main; import collections.array; structure Light x: float32; end " };

    // INFO: renders the initializer of the constant VALUE, or a marker that says why there is none.
    std::string MAREA_lowerConstant(const std::string &type, const std::string &expression)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(PRELUDE + "constant VALUE: " + type + " = " + expression + ";") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *constant{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((constant == nullptr) || (MAREA_IrNodeGetChildCount(constant) != 1)) {
            return "<no initializer>";
        }

        return MAREA_irRender(MAREA_irChild(constant, 0));
    }

    // INFO: renders the initializer of the first local of a function body.
    std::string MAREA_lowerLocal(const std::string &parameters, const std::string &type, const std::string &expression)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(PRELUDE + "function run(" + parameters + ") local result: " + type + " = " + expression + "; end") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((function == nullptr) || (MAREA_IrNodeGetChildCount(function) != 2)) {
            return "<no function>";
        }

        const MAREA_IrNode *local{ MAREA_irChild(MAREA_irChild(function, 1), 0) };

        if ((local == nullptr) || (MAREA_IrNodeGetChildCount(local) != 1)) {
            return "<no initializer>";
        }

        return MAREA_irRender(MAREA_irChild(local, 0));
    }

    Codes MAREA_codesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_irCodes();
    }

    std::string MAREA_namesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        std::string names;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
            names += (names.empty() ? "" : " ") + MAREA_irText(MAREA_irChild(fixture.module, index));
        }

        return names;
    }
} // namespace

TEST(MAREAIrTypeArguments, SizeOfLowersToABuiltinCall)
{
    GTEST_COUT << "Validating size_of<T>() lowers to a CALL_EXPRESSION whose callee is a BUILTIN_NAME that holds the type." << std::endl;

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "size_of<Light>()"), "(CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME Light)) (ARGUMENT_LIST))") << "The type must be the only child of the name, and the call must keep its argument list last.";
}

TEST(MAREAIrTypeArguments, AlignOfLowersToABuiltinCall)
{
    GTEST_COUT << "Validating align_of<T>() lowers like size_of<T>()." << std::endl;

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "align_of<Light>()"), "(CALL_EXPRESSION (BUILTIN_NAME align_of (TYPE_NAME Light)) (ARGUMENT_LIST))") << "align_of must be a built-in name too.";
}

TEST(MAREAIrTypeArguments, ConvertKeepsItsTypeAndItsValue)
{
    GTEST_COUT << "Validating convert<T>(x) keeps the target type in the callee and the value in the argument list." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("count: integer32", "integer64", "convert<integer64>(count)"), "(CALL_EXPRESSION :integer64 (BUILTIN_NAME convert (TYPE_NAME integer64)) (ARGUMENT_LIST (IDENTIFIER count)))") << "The target type must not be mixed with the value.";
}

TEST(MAREAIrTypeArguments, BuiltinCallsNestAndJoinExpressions)
{
    GTEST_COUT << "Validating a built-in call lowers inside another call and inside a binary expression." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("count: integer32", "float64", "convert<float64>(convert<integer64>(count))"), "(CALL_EXPRESSION :float64 (BUILTIN_NAME convert (TYPE_NAME float64)) (ARGUMENT_LIST (CALL_EXPRESSION :integer64 (BUILTIN_NAME convert (TYPE_NAME integer64)) (ARGUMENT_LIST (IDENTIFIER count)))))") << "A built-in call must lower as an argument.";

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "size_of<Light>() * 2"), "(BINARY_EXPRESSION op=STAR (CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME Light)) (ARGUMENT_LIST)) (INTEGER_LITERAL 2))") << "A built-in call must lower as an operand.";
}

TEST(MAREAIrTypeArguments, TypeKeepsItsOwnTypeArguments)
{
    GTEST_COUT << "Validating a type operand keeps its type arguments: a nested type and a value argument." << std::endl;

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "size_of<array<integer32>>()"), "(CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME array (TYPE_NAME integer32))) (ARGUMENT_LIST))") << "A nested type must be a child of its head.";

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "size_of<vector<float32, 4>>()"), "(CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME vector (TYPE_NAME float32) (INTEGER_LITERAL 4))) (ARGUMENT_LIST))") << "A value argument must be an INTEGER_LITERAL child, in order.";

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "size_of<array<array<float32>>>()"), "(CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME array (TYPE_NAME array (TYPE_NAME float32)))) (ARGUMENT_LIST))") << "A type must nest to any depth.";
}

TEST(MAREAIrTypeArguments, TypeKeepsADottedNameWhole)
{
    GTEST_COUT << "Validating a dotted type name stays one text, as a type name does elsewhere." << std::endl;

    EXPECT_EQ(MAREA_lowerConstant("unsigned64", "size_of<collections.array<integer32>>()"), "(CALL_EXPRESSION (BUILTIN_NAME size_of (TYPE_NAME collections.array (TYPE_NAME integer32))) (ARGUMENT_LIST))") << "The dotted name must be kept whole in the text.";
}

TEST(MAREAIrTypeArguments, StaticCallLowersToATypeQualifiedMember)
{
    GTEST_COUT << "Validating array<T>.filled(...) lowers to a call of a member of a TYPE_NAME." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("", "array<integer32>", "array<integer32>.filled(8, 0)"), "(CALL_EXPRESSION (MEMBER_ACCESS (TYPE_NAME array (TYPE_NAME integer32)) (IDENTIFIER filled)) (ARGUMENT_LIST (INTEGER_LITERAL 8) (INTEGER_LITERAL 0)))") << "The call must keep its callee and arguments layout, with the type as the receiver.";
}

TEST(MAREAIrTypeArguments, StaticCallKeepsAValueArgument)
{
    GTEST_COUT << "Validating a static call on a type with a value argument keeps it, in order." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("", "array<float32, 16>", "array<float32, 16>.filled(0.0)"), "(CALL_EXPRESSION (MEMBER_ACCESS (TYPE_NAME array (TYPE_NAME float32) (INTEGER_LITERAL 16)) (IDENTIFIER filled)) (ARGUMENT_LIST (FLOAT_LITERAL 0.0)))") << "The value argument must follow the type argument.";
}

TEST(MAREAIrTypeArguments, StaticCallResultCanBeUsedAgain)
{
    GTEST_COUT << "Validating a static call lowers as the receiver of a later member call and as an argument." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("", "integer32", "array<integer32>.filled(8, 0).length()"), "(CALL_EXPRESSION (MEMBER_ACCESS (CALL_EXPRESSION (MEMBER_ACCESS (TYPE_NAME array (TYPE_NAME integer32)) (IDENTIFIER filled)) (ARGUMENT_LIST (INTEGER_LITERAL 8) (INTEGER_LITERAL 0))) (IDENTIFIER length)) (ARGUMENT_LIST))") << "A static call must be a plain expression for its parent.";
}

TEST(MAREAIrTypeArguments, TypeAndBuiltinNodesCarryPositions)
{
    GTEST_COUT << "Validating the new nodes take the position of what they were written from." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main;\nstructure Light x: float32; end\nconstant SIZE: unsigned64 = size_of<Light>();"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *call{ MAREA_irChild(MAREA_irChild(fixture.module, 1), 0) };

    const MAREA_IrNode *name{ MAREA_irChild(call, 0) };

    const MAREA_IrNode *type{ MAREA_irChild(name, 0) };

    ASSERT_EQ(MAREA_IrNodeGetKind(name), MAREA_IR_KIND_BUILTIN_NAME) << "The callee must be a BUILTIN_NAME.";

    ASSERT_EQ(MAREA_IrNodeGetKind(type), MAREA_IR_KIND_TYPE_NAME) << "The argument must be a TYPE_NAME.";

    EXPECT_EQ(MAREA_IrNodeGetPosition(name).line, 3U) << "The name must stand on the line of the call.";

    EXPECT_EQ(MAREA_IrNodeGetPosition(type).line, 3U) << "The type must stand on the line of the call.";

    EXPECT_GT(MAREA_IrNodeGetPosition(type).column, MAREA_IrNodeGetPosition(name).column) << "The type is written after the name.";

    EXPECT_EQ(MAREA_IrNodeGetTypeName(name), nullptr) << "A built-in name has no type name.";

    EXPECT_EQ(MAREA_IrNodeGetTypeName(type), nullptr) << "A type name node carries its name as text, not as a type name.";

    EXPECT_EQ(MAREA_IrNodeGetFlags(type), 0U) << "A type name has no flags.";
}

TEST(MAREAIrTypeArguments, BuiltinNameWithoutTypeArgumentsStaysAPlainCall)
{
    GTEST_COUT << "Validating a call of a built-in name with no type arguments is a plain call." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("", "integer32", "size_of(1)"), "(CALL_EXPRESSION (IDENTIFIER size_of) (ARGUMENT_LIST (INTEGER_LITERAL 1)))") << "Without type arguments there is no instantiation, so the call stays as it was.";
}

TEST(MAREAIrTypeArguments, NonGenericStaticCallIsUnchanged)
{
    GTEST_COUT << "Validating a static call on a type with no type arguments is a plain member call." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("", "Light", "Light.origin()"), "(CALL_EXPRESSION (MEMBER_ACCESS (IDENTIFIER Light) (IDENTIFIER origin)) (ARGUMENT_LIST))") << "A type with no type arguments is an IDENTIFIER, as before.";
}

TEST(MAREAIrTypeArguments, FunctionOfTheUnitWithABuiltinNameStaysAGeneric)
{
    GTEST_COUT << "Validating a generic function that the unit declares with a built-in name is a user generic, so its call keeps raising 4004." << std::endl;

    const std::string SOURCE{ "namespace main; function convert<Item>(value: Item): Item where Item: Copyable return copy value; end function run(): integer32 return convert<integer32>(5); end function keep() end" };

    EXPECT_EQ(MAREA_codesOf(SOURCE), (Codes{ GENERIC, GENERIC })) << "The declaration and the call must each raise 4004 once.";

    EXPECT_EQ(MAREA_namesOf(SOURCE), "keep") << "The generic function and its caller must be dropped.";
}

TEST(MAREAIrTypeArguments, ImportedNameWithABuiltinNameStaysAGeneric)
{
    GTEST_COUT << "Validating a name that the unit imports with a built-in name is a user call, so it keeps raising 4004." << std::endl;

    const std::string SOURCE{ "namespace main; import collections.convert; function run(): integer32 return convert<integer32>(5); end function keep() end" };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{ GENERIC }) << "The call must raise 4004 once.";

    EXPECT_EQ(MAREA_namesOf(SOURCE), "keep") << "The caller must be dropped.";
}

TEST(MAREAIrTypeArguments, ImportAliasWithABuiltinNameStaysAGeneric)
{
    GTEST_COUT << "Validating an import alias with a built-in name is a user call, and an alias that hides the name frees it." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.helper as size_of; function run(): integer32 return size_of<integer32>(); end"), Codes{ GENERIC }) << "The alias is the name that the unit binds, so the call is a user call.";

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.size_of as measure; function run(): integer32 return size_of<integer32>(); end"), Codes{}) << "The last segment is not bound when an alias is given, so the built-in is kept.";
}

TEST(MAREAIrTypeArguments, QualifiedCalleeWithTypeArgumentsStaysAGeneric)
{
    GTEST_COUT << "Validating a call of a function reached through a namespace with explicit type arguments is not a built-in." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.array; function run(): integer32 return array.convert<integer32>(5); end"), Codes{ GENERIC }) << "Only the bare names are built-ins.";
}

TEST(MAREAIrTypeArguments, StaticCallOnAGenericStructureOfTheUnitStaysAGeneric)
{
    GTEST_COUT << "Validating a static call on a generic structure of the same unit is a user generic, so it keeps raising 4004." << std::endl;

    const std::string SOURCE{ "namespace main; structure Box<Item> value: Item; end implementation<Item> Box<Item> function make(): integer32 return 1; end end function run(): integer32 return Box<integer32>.make(); end function keep() end" };

    const Codes CODES{ MAREA_codesOf(SOURCE) };

    ASSERT_FALSE(CODES.empty()) << "The generic structure and its call must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, GENERIC) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }

    EXPECT_EQ(MAREA_namesOf(SOURCE), "keep") << "The caller must be dropped with the generic declarations.";
}

TEST(MAREAIrTypeArguments, StaticCallOnAGenericAliasOfTheUnitStaysAGeneric)
{
    GTEST_COUT << "Validating a static call on a generic type alias of the same unit is a user generic too." << std::endl;

    const std::string SOURCE{ "namespace main; import collections.array; type Many<Item> = array<Item>; function run(): integer32 return Many<integer32>.filled(8, 0).length(); end" };

    const Codes CODES{ MAREA_codesOf(SOURCE) };

    ASSERT_FALSE(CODES.empty()) << "The generic alias and its call must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, GENERIC) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }
}

TEST(MAREAIrTypeArguments, StaticCallWithItsOwnTypeArgumentsStaysAGeneric)
{
    GTEST_COUT << "Validating a static call that also names type arguments for the function is not lowered." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.array; function run(): integer32 return array<integer32>.cast<float32>(1); end"), Codes{ GENERIC }) << "A type argument of the function itself is an instantiation that no pass performs.";
}

TEST(MAREAIrTypeArguments, GenericNameOutsideACallStaysAGeneric)
{
    GTEST_COUT << "Validating a type name used as a value but not as the receiver of a call keeps raising 4004." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.array; constant LIMIT: integer32 = array<integer32>.LIMIT;"), Codes{ GENERIC }) << "Only the receiver of a call is lowered.";
}

TEST(MAREAIrTypeArguments, FunctionTypeArgumentStaysAGeneric)
{
    GTEST_COUT << "Validating a function type as a type argument is not lowered and raises 4004, for a built-in and for a static call." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; constant SIZE: unsigned64 = size_of<function(value: integer32): integer32>();"), Codes{ GENERIC }) << "A function type cannot be a TYPE_NAME, so the built-in call must raise.";

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.array; function run(): integer32 return array<function(value: integer32): integer32>.filled(8, 0).length(); end"), Codes{ GENERIC }) << "A function type cannot be a TYPE_NAME, so the static call must raise.";

    EXPECT_EQ(MAREA_codesOf("namespace main; constant SIZE: unsigned64 = size_of<function(value: integer32): integer32>(); constant OTHER: integer32 = 1;"), Codes{ GENERIC }) << "The gap must be reported once and the next declaration must lower.";
}

TEST(MAREAIrTypeArguments, NestedFunctionTypeArgumentStaysAGenericOnce)
{
    GTEST_COUT << "Validating a function type nested in a type argument is reported once and drops the declaration whole." << std::endl;

    const std::string SOURCE{ "namespace main; import collections.array; constant SIZE: unsigned64 = size_of<array<function(value: integer32): integer32>>(); function keep() end" };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{ GENERIC }) << "The nested gap must be reported once.";

    EXPECT_EQ(MAREA_namesOf(SOURCE), "keep") << "The constant must be dropped and the next declaration kept.";
}

TEST(MAREAIrTypeArguments, GapInAStaticCallArgumentDropsTheDeclaration)
{
    GTEST_COUT << "Validating a gap inside the arguments of a static call drops the declaration whole and is reported once." << std::endl;

    const std::string SOURCE{ "namespace main; import collections.array; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function run(): integer32 return array<integer32>.filled(identity(5), 0).length(); end function keep() end" };

    EXPECT_EQ(MAREA_codesOf(SOURCE), (Codes{ GENERIC, GENERIC })) << "The generic function and the generic call inside the static call must each raise once.";

    EXPECT_EQ(MAREA_namesOf(SOURCE), "keep") << "The function that holds the gap must be dropped.";
}

TEST(MAREAIrTypeArguments, StaticCallKeepsADottedTypeName)
{
    GTEST_COUT << "Validating a static call on a dotted type name keeps the name whole in the TYPE_NAME." << std::endl;

    EXPECT_EQ(MAREA_lowerLocal("", "array<integer32>", "collections.array<integer32>.filled(8, 0)"), "(CALL_EXPRESSION (MEMBER_ACCESS (TYPE_NAME collections.array (TYPE_NAME integer32)) (IDENTIFIER filled)) (ARGUMENT_LIST (INTEGER_LITERAL 8) (INTEGER_LITERAL 0)))") << "The receiver must keep the dotted name as one text.";
}

TEST(MAREAIrTypeArguments, GapInABuiltinArgumentDropsTheDeclaration)
{
    GTEST_COUT << "Validating a gap inside the arguments of a built-in call drops the declaration whole and is reported once." << std::endl;

    const std::string SOURCE{ "namespace main; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function run(): float64 return convert<float64>(identity(5)); end function keep() end" };

    EXPECT_EQ(MAREA_codesOf(SOURCE), (Codes{ GENERIC, GENERIC })) << "The generic function and the generic call inside the built-in call must each raise once.";

    EXPECT_EQ(MAREA_namesOf(SOURCE), "keep") << "The function that holds the gap must be dropped.";
}

TEST(MAREAIrTypeArguments, UnitIsForgottenWhenTheLoweringEnds)
{
    GTEST_COUT << "Validating the context keeps no pointer to a unit after its lowering ends, so a later expression is judged alone." << std::endl;

    MAREA_IrFixture first(RESOLVE_EXTERNAL);

    ASSERT_EQ(first.MAREA_lower("namespace main; function convert(value: integer32): integer32 return value; end"), MAREA_IR_EXCEPT_SUCCESS) << "The first unit must lower.";

    MAREA_IrFixture second(RESOLVE_EXTERNAL);

    ASSERT_EQ(second.sema.MAREA_sema("namespace main; constant VALUE: integer32 = convert<integer32>(5);", "main.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The second unit must pass sema.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(second.sema.context, second.sema.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must build.";

    const MAREA_SemaTypedNode *call{ MAREA_IrTypedFindChild(MAREA_IrTypedChildAt(root, MAREA_SemaTypedNodeGetChildCount(root) - 1), MAREA_AST_KIND_CALL_EXPRESSION) };

    ASSERT_NE(call, nullptr) << "The constant must hold the call.";

    MAREA_IrNode *lowered{ nullptr };

    ASSERT_EQ(MAREA_IrLowerExpression(first.context, call, &lowered), MAREA_IR_EXCEPT_SUCCESS) << "The call must lower.";

    ASSERT_NE(lowered, nullptr) << "The call must be a built-in call, because no unit is being lowered.";
}

TEST(MAREAIrTypeArguments, BuiltinCallsRaiseNothingAndKeepTheirDeclaration)
{
    GTEST_COUT << "Validating a unit with built-in and static calls raises no ir diagnostic and keeps every declaration." << std::endl;

    const std::string SOURCE{ PRELUDE + "constant SIZE: unsigned64 = size_of<Light>(); function run(count: integer32): integer64 local scratch: array<integer32> = array<integer32>.filled(8, 0); return convert<integer64>(count); end" };

    EXPECT_EQ(MAREA_codesOf(SOURCE), Codes{}) << "No ir diagnostic may be raised.";

    EXPECT_EQ(MAREA_namesOf(SOURCE), "Light SIZE run") << "Every declaration must remain in source order.";
}

TEST(MAREAIrTypeArguments, DumpShowsTheNewKinds)
{
    GTEST_COUT << "Validating the text dump shows a built-in call and a static call with the new kinds, indented." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; function run(count: integer32) local wide: integer64 = convert<integer64>(count); local items: array<integer32> = array<integer32>.filled(8, 0); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length), MAREA_IR_EXCEPT_SUCCESS) << "The dump must succeed.";

    const std::string DUMP{ text, length };

    EXPECT_NE(DUMP.find("      LOCAL_DECLARATION \"wide\" type=integer64\n        CALL_EXPRESSION type=integer64\n          BUILTIN_NAME \"convert\"\n            TYPE_NAME \"integer64\"\n          ARGUMENT_LIST\n            IDENTIFIER \"count\"\n"), std::string::npos) << "The built-in call must dump with its name, its type and its argument.\n"
                                                                                                                                                                                                                                                              << DUMP;

    EXPECT_NE(DUMP.find("        CALL_EXPRESSION\n          MEMBER_ACCESS\n            TYPE_NAME \"array\"\n              TYPE_NAME \"integer32\"\n            IDENTIFIER \"filled\"\n          ARGUMENT_LIST\n"), std::string::npos) << "The static call must dump with its type as the receiver.\n"
                                                                                                                                                                                                                                      << DUMP;
}
