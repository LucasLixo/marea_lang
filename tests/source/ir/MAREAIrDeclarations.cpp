// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return (text.compare(0, 2, "c.") == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };
} // namespace

TEST(MAREAIrDeclarations, ConstantKeepsNameAndTypeName)
{
    GTEST_COUT << "Validating a constant lowers to a CONSTANT_DECLARATION with its name and type name." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant LIMIT: sinteger32 = 10;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "The module must hold the constant only.";

    const MAREA_IrNode *constant{ MAREA_irChild(fixture.module, 0) };

    EXPECT_EQ(MAREA_IrNodeGetKind(constant), MAREA_IR_KIND_CONSTANT_DECLARATION) << "The kind must be CONSTANT_DECLARATION.";

    EXPECT_EQ(MAREA_irText(constant), "LIMIT") << "The text must be the name.";

    EXPECT_EQ(MAREA_irTypeName(constant), "sinteger32") << "The type name must be the declared type.";
}

TEST(MAREAIrDeclarations, TypeNameOfAGenericTypeIsItsHeadName)
{
    GTEST_COUT << "Validating a parameter of a generic type keeps the head name of the type." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function total(values: slice<sinteger32>) end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *parameters{ MAREA_irChild(MAREA_irChild(fixture.module, 0), 0) };

    EXPECT_EQ(MAREA_irTypeName(MAREA_irChild(parameters, 0)), "slice") << "The type name must be the head name.";
}

TEST(MAREAIrDeclarations, ImportAndNamespaceProduceNoNode)
{
    GTEST_COUT << "Validating the namespace and import declarations are erased." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import c.math.sqrt; constant A: sinteger32 = 1;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the constant must remain.";

    EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(fixture.module, 0)), MAREA_IR_KIND_CONSTANT_DECLARATION) << "The remaining node must be the constant.";
}

TEST(MAREAIrDeclarations, StructureKeepsFields)
{
    GTEST_COUT << "Validating a structure lowers to a STRUCTURE_DECLARATION with one FIELD_DECLARATION per field." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: float64; y: float64; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "The module must hold the structure.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 0)), "(STRUCTURE_DECLARATION Point (FIELD_DECLARATION x :float64) (FIELD_DECLARATION y :float64))") << "The structure shape must match the source.";
}

TEST(MAREAIrDeclarations, StructureAttributesAreErased)
{
    GTEST_COUT << "Validating an attribute on a structure leaves no node." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; #[visibility = public, move_only] structure Point x: float64; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 0)), "(STRUCTURE_DECLARATION Point (FIELD_DECLARATION x :float64))") << "No ATTRIBUTE node must survive.";
}

TEST(MAREAIrDeclarations, OpaqueLowersToAStructureWithoutFields)
{
    GTEST_COUT << "Validating an opaque structure lowers to a STRUCTURE_DECLARATION with zero fields." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; opaque structure Texture;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 0)), "(STRUCTURE_DECLARATION Texture)") << "The opaque shape must have zero fields.";
}

TEST(MAREAIrDeclarations, EnumKeepsVariantsAndVariantFields)
{
    GTEST_COUT << "Validating an enum lowers to an ENUM_DECLARATION with its variants and variant fields." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Planet Mercury(mass: float64 = 1.0, radius: float64 = 2.0); Earth; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *declaration{ MAREA_irChild(fixture.module, 0) };

    EXPECT_EQ(MAREA_IrNodeGetKind(declaration), MAREA_IR_KIND_ENUM_DECLARATION) << "The kind must be ENUM_DECLARATION.";

    EXPECT_EQ(MAREA_irText(declaration), "Planet") << "The text must be the enum name.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(declaration), 2U) << "The enum must hold two variants.";

    const MAREA_IrNode *mercury{ MAREA_irChild(declaration, 0) };

    EXPECT_EQ(MAREA_IrNodeGetKind(mercury), MAREA_IR_KIND_VARIANT_DECLARATION) << "The first child must be a VARIANT_DECLARATION.";

    EXPECT_EQ(MAREA_irText(mercury), "Mercury") << "The variant name must be kept.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(mercury), 2U) << "Mercury must hold two variant fields.";

    const MAREA_IrNode *mass{ MAREA_irChild(mercury, 0) };

    EXPECT_EQ(MAREA_IrNodeGetKind(mass), MAREA_IR_KIND_VARIANT_FIELD) << "The child must be a VARIANT_FIELD.";

    EXPECT_EQ(MAREA_irText(mass), "mass") << "The field name must be kept.";

    EXPECT_EQ(MAREA_irTypeName(mass), "float64") << "The field type name must be kept.";

    EXPECT_EQ(MAREA_IrNodeGetChildCount(MAREA_irChild(declaration, 1)), 0U) << "A variant without fields must have zero children.";
}

TEST(MAREAIrDeclarations, FunctionKeepsParametersAndReturnTypeName)
{
    GTEST_COUT << "Validating a function lowers to a FUNCTION_DECLARATION with a PARAMETER_LIST and an empty BLOCK." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function add(a: sinteger32, b: sinteger32): sinteger32 return a + b; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *function{ MAREA_irChild(fixture.module, 0) };

    EXPECT_EQ(MAREA_IrNodeGetKind(function), MAREA_IR_KIND_FUNCTION_DECLARATION) << "The kind must be FUNCTION_DECLARATION.";

    EXPECT_EQ(MAREA_irText(function), "add") << "The text must be the function name.";

    EXPECT_EQ(MAREA_irTypeName(function), "sinteger32") << "The type name must be the return type name.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(function), 2U) << "The function must hold a parameter list and a body.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(function, 0)), "(PARAMETER_LIST (PARAMETER a :sinteger32) (PARAMETER b :sinteger32))") << "The parameters must keep name and type name.";

    EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(function, 1)), MAREA_IR_KIND_BLOCK) << "The second child must be the BLOCK.";
}

TEST(MAREAIrDeclarations, FunctionWithoutReturnTypeHasNoTypeName)
{
    GTEST_COUT << "Validating a function without a return type carries no type name and an empty parameter list." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function nothing() end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *function{ MAREA_irChild(fixture.module, 0) };

    EXPECT_EQ(MAREA_IrNodeGetTypeName(function), nullptr) << "No return type means no type name.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(function, 0)), "(PARAMETER_LIST)") << "The parameter list must be present and empty.";
}

TEST(MAREAIrDeclarations, MutableParameterKeepsTheMutableFlag)
{
    GTEST_COUT << "Validating a mutable parameter keeps only the MUTABLE flag." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Counter n: sinteger32; end function bump(c: mutable Counter) end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *parameters{ MAREA_irChild(MAREA_irChild(fixture.module, 1), 0) };

    const MAREA_IrNode *parameter{ MAREA_irChild(parameters, 0) };

    EXPECT_EQ(MAREA_IrNodeGetFlags(parameter), static_cast<MAREA_Uinteger32>(MAREA_AST_FLAG_MUTABLE)) << "The MUTABLE flag must be carried over.";

    EXPECT_EQ(MAREA_irTypeName(parameter), "Counter") << "The parameter type name must be kept.";
}

TEST(MAREAIrDeclarations, ContractProducesNoNode)
{
    GTEST_COUT << "Validating a contract declaration is erased." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; contract Comparable function less(self: Self, other: Self): boolean; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 0U) << "A contract must leave no node.";
}

TEST(MAREAIrDeclarations, ImplementationQualifiesItsMembers)
{
    GTEST_COUT << "Validating an implementation lowers its members as module-level nodes named Type.name." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: float64; end implementation Point constant ORIGIN: float64 = 0.0; function get(self: Point): float64 return self.x; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 3U) << "The structure and two members must be module-level.";

    const MAREA_IrNode *constant{ MAREA_irChild(fixture.module, 1) };

    EXPECT_EQ(MAREA_IrNodeGetKind(constant), MAREA_IR_KIND_CONSTANT_DECLARATION) << "The first member must be the constant.";

    EXPECT_EQ(MAREA_irText(constant), "Point.ORIGIN") << "The constant must be qualified.";

    const MAREA_IrNode *function{ MAREA_irChild(fixture.module, 2) };

    EXPECT_EQ(MAREA_IrNodeGetKind(function), MAREA_IR_KIND_FUNCTION_DECLARATION) << "The second member must be the function.";

    EXPECT_EQ(MAREA_irText(function), "Point.get") << "The function must be qualified.";
}

TEST(MAREAIrDeclarations, ModuleKeepsSourceOrder)
{
    GTEST_COUT << "Validating the module children follow the source order." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant A: sinteger32 = 1; function f() end enum E X; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 3U) << "The module must hold three declarations.";

    EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(fixture.module, 0)), MAREA_IR_KIND_CONSTANT_DECLARATION) << "The first must be the constant.";

    EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(fixture.module, 1)), MAREA_IR_KIND_FUNCTION_DECLARATION) << "The second must be the function.";

    EXPECT_EQ(MAREA_IrNodeGetKind(MAREA_irChild(fixture.module, 2)), MAREA_IR_KIND_ENUM_DECLARATION) << "The third must be the enum.";
}

TEST(MAREAIrDeclarations, StructuresExampleKeepsItsDeclarationShells)
{
    GTEST_COUT << "Validating examples/types/structures.marea lowers to the declaration shells of its source." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("types/structures.marea"), "types/structures.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    ASSERT_EQ(MAREA_IrNodeGetKind(fixture.module), MAREA_IR_KIND_MODULE) << "The root must be a MODULE.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 5U) << "The example declares five items.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 0)), "(STRUCTURE_DECLARATION Point (FIELD_DECLARATION x :float64) (FIELD_DECLARATION y :float64))") << "Point must match the source.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 1)), "(ENUM_DECLARATION ShapeKind (VARIANT_DECLARATION Circle) (VARIANT_DECLARATION Rectangle))") << "ShapeKind must match the source.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(fixture.module, 2)), "(STRUCTURE_DECLARATION Shape (FIELD_DECLARATION kind :ShapeKind) (FIELD_DECLARATION center :Point) (FIELD_DECLARATION width :float64) (FIELD_DECLARATION height :float64))") << "Shape must match the source.";

    EXPECT_EQ(MAREA_irText(MAREA_irChild(fixture.module, 3)), "area") << "The fourth item must be area.";

    EXPECT_EQ(MAREA_irText(MAREA_irChild(fixture.module, 4)), "make_square") << "The fifth item must be make_square.";

    EXPECT_EQ(MAREA_irTypeName(MAREA_irChild(fixture.module, 4)), "Shape") << "make_square must return Shape.";
}

TEST(MAREAIrDeclarations, MethodsExampleKeepsItsDeclarationShells)
{
    GTEST_COUT << "Validating examples/types/methods.marea lowers its implementation members as Vector2.name." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("types/methods.marea"), "types/methods.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    std::vector<std::string> names;

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
        names.push_back(MAREA_irText(MAREA_irChild(fixture.module, index)));
    }

    ASSERT_GE(names.size(), 7U) << "The example holds a structure, six methods and a function.";

    EXPECT_EQ(names[0], "Vector2") << "The structure must come first.";

    EXPECT_EQ(names[1], "Vector2.origin") << "The first method must be qualified.";

    EXPECT_EQ(names[2], "Vector2.length_squared") << "The second method must be qualified.";

    EXPECT_EQ(names[4], "Vector2.scale") << "The fourth method must be qualified.";

    const MAREA_IrNode *scale{ MAREA_irChild(fixture.module, 4) };

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(scale, 0)), "(PARAMETER_LIST (PARAMETER self :Vector2 flags=1) (PARAMETER factor :float64))") << "A mutable self must keep its flag.";
}
