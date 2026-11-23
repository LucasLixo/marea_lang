// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 UNKNOWN_TYPE{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE };
    constexpr MAREA_Uinteger32 DUPLICATE_MEMBER{ MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_MEMBER };

    MAREA_ResolveCallback MAREA_externalOnly()
    {
        return [](const std::string &text, const MAREA_AstNode **) {
            const bool EXTERNAL{ (text == "list") || (text == "table") || (text == "handle") || (text == "collections.list") || (text == "c.math.Vector3") };

            return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        };
    }

    Codes MAREA_codesOf(const std::string &body, MAREA_ResolveCallback resolve = nullptr)
    {
        MAREA_SemaFixture fixture(resolve ? std::move(resolve) : MAREA_externalOnly());

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaTypesAndDeclarations, PrimitiveTypesResolve)
{
    GTEST_COUT << "Validating every primitive type name resolves." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: boolean; b: sinteger8; c: sinteger16; d: sinteger32; e: sinteger64; f: uinteger8; g: uinteger16; h: uinteger32; i: uinteger64; j: float32; k: float64; l: string; end").empty()) << "A primitive type must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, SignedAndUnsignedIntegerNamesResolve)
{
    GTEST_COUT << "Validating every sinteger and uinteger width resolves, also inside a generic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: sinteger8; b: sinteger16; c: sinteger32; d: sinteger64; e: uinteger8; f: uinteger16; g: uinteger32; h: uinteger64; i: pointer<sinteger32>; j: slice<uinteger8>; end").empty()) << "A sinteger or uinteger name must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, RetiredIntegerNamesAreUnknownTypes)
{
    GTEST_COUT << "Validating the retired integerN and unsignedN names raise UNKNOWN_TYPE." << std::endl;

    for (const char *name : { "integer8", "integer16", "integer32", "integer64", "unsigned8", "unsigned16", "unsigned32", "unsigned64" }) {
        EXPECT_EQ(MAREA_codesOf(std::string("structure S a: ") + name + "; end"), Codes{ UNKNOWN_TYPE }) << "The retired name " << name << " must raise UNKNOWN_TYPE.";
        EXPECT_EQ(MAREA_codesOf(std::string("structure S a: pointer<") + name + ">; end"), Codes{ UNKNOWN_TYPE }) << "The retired name " << name << " must raise UNKNOWN_TYPE inside a generic.";
    }
}

TEST(MAREASemaTypesAndDeclarations, CoreGenericTypesResolve)
{
    GTEST_COUT << "Validating array, vector, pointer, owned, slice and task resolve." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: array<float32, 4>; f: vector<float32>; b: pointer<sinteger32>; c: owned<string>; d: slice<uinteger8>; e: task<float32>; end").empty()) << "A core generic type must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, CoreGenericTypeTakesAGenericParameter)
{
    GTEST_COUT << "Validating a core generic type accepts a generic parameter of the declaration." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Box<Item> inner: pointer<Item>; end").empty()) << "A generic parameter must be in scope.";
}

TEST(MAREASemaTypesAndDeclarations, ExternalTypesResolve)
{
    GTEST_COUT << "Validating a type the resolver reports as EXTERNAL resolves." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: list<sinteger32>; b: table<string, sinteger32>; c: handle<sinteger32>; d: collections.list<sinteger32>; e: c.math.Vector3; end").empty()) << "An external type must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, UnknownTypeIsRejected)
{
    GTEST_COUT << "Validating a type the resolver cannot find is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure S a: Missing; end"), Codes{ UNKNOWN_TYPE }) << "A missing type must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, UnknownTypeIsPositionedOnTheType)
{
    GTEST_COUT << "Validating the UNKNOWN_TYPE diagnostic sits on the type, not on the declaration." << std::endl;

    MAREA_SemaFixture fixture(MAREA_externalOnly());

    const std::string SOURCE{ "namespace main; structure S a: Missing; end" };

    ASSERT_EQ(fixture.MAREA_sema(SOURCE), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    ASSERT_EQ(fixture.MAREA_semaCodes().size(), 1U) << "One diagnostic must be raised.";

    const MAREA_Diagnostic *diagnostic{ nullptr };

    ASSERT_EQ(MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, 0, &diagnostic), MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) << "The diagnostic must be readable.";

    EXPECT_EQ(MAREA_DiagnosticGetPosition(diagnostic).offset, SOURCE.find("Missing")) << "The position must be the type.";
}

TEST(MAREASemaTypesAndDeclarations, UnknownTypeInParameterAndReturnIsRejected)
{
    GTEST_COUT << "Validating a missing parameter type and a missing return type are rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f(a: Missing) end"), Codes{ UNKNOWN_TYPE }) << "A missing parameter type must raise UNKNOWN_TYPE.";

    EXPECT_EQ(MAREA_codesOf("function f(): Missing return 1; end"), Codes{ UNKNOWN_TYPE }) << "A missing return type must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, UnknownTypeInsideTypeArgumentIsRejected)
{
    GTEST_COUT << "Validating a missing type inside a type argument is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure S a: pointer<Missing>; end"), Codes{ UNKNOWN_TYPE }) << "A missing type argument must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, LocalTypesResolve)
{
    GTEST_COUT << "Validating a structure, an enum, an opaque type and a type alias of the file resolve." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end enum Color red; green; end opaque structure Handle; type Alias = Point; structure Holder a: Point; b: Color; c: pointer<Handle>; d: Alias; end").empty()) << "A local type must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, FunctionNameIsNotAType)
{
    GTEST_COUT << "Validating a function name used as a type is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function f() end structure S a: f; end"), Codes{ UNKNOWN_TYPE }) << "A function name must raise UNKNOWN_TYPE as a type.";
}

TEST(MAREASemaTypesAndDeclarations, ImportedTypeResolves)
{
    GTEST_COUT << "Validating a type brought in by an import resolves." << std::endl;

    auto resolve = [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; };

    EXPECT_TRUE(MAREA_codesOf("import arenas.Arena; structure S a: Arena; end", resolve).empty()) << "An imported symbol must resolve as a type.";
}

TEST(MAREASemaTypesAndDeclarations, EnumWithTypedDefaultedFields)
{
    GTEST_COUT << "Validating an enum with typed and defaulted variant fields raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("enum Shape circle(radius: float32 = 1.0); square(side: float32, tag: sinteger32 = 0); empty; end").empty()) << "A well-typed enum must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("enum Shape circle(radius: Missing); end"), Codes{ UNKNOWN_TYPE }) << "A missing variant field type must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, OpaqueStructureHasNoFields)
{
    GTEST_COUT << "Validating an opaque structure raises nothing." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("opaque structure Texture;").empty()) << "An opaque structure must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, FunctionTypeResolves)
{
    GTEST_COUT << "Validating a function type with known types resolves and one with a missing type does not." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S callback: function(a: sinteger32): float32; end").empty()) << "A function type must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("structure S callback: function(a: Missing): float32; end"), Codes{ UNKNOWN_TYPE }) << "A missing parameter type of a function type must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, SelfResolvesInsideAnImplementation)
{
    GTEST_COUT << "Validating Self resolves in an implementation and a contract and nowhere else." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure P x: float32; end implementation P function same(other: Self): Self return copy other; end end").empty()) << "Self must resolve in an implementation.";

    EXPECT_TRUE(MAREA_codesOf("contract Same function same(other: Self): Self; end").empty()) << "Self must resolve in a contract.";

    EXPECT_EQ(MAREA_codesOf("function f(other: Self) end"), Codes{ UNKNOWN_TYPE }) << "Self outside a type must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, GenericParameterScopeEnds)
{
    GTEST_COUT << "Validating a generic parameter is visible only inside its declaration." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("function first<Item>(a: Item): Item return copy a; end").empty()) << "A function generic parameter must be in scope.";

    EXPECT_EQ(MAREA_codesOf("function first<Item>(a: Item): Item return copy a; end function other(a: Item) end"), Codes{ UNKNOWN_TYPE }) << "The generic parameter must not leak out.";

    EXPECT_TRUE(MAREA_codesOf("structure Box<Item> inner: Item; end implementation<Item> Box<Item> function get(self: Box<Item>): Item return self.inner; end end").empty()) << "An implementation generic parameter must be in scope.";
}

TEST(MAREASemaTypesAndDeclarations, FunctionRepeatedAcrossImplementationsIsRejected)
{
    GTEST_COUT << "Validating the same function name in two implementation blocks of one type is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure P x: float32; end implementation P function area() end end implementation P function area() end end"), Codes{ DUPLICATE_MEMBER }) << "A repeated function must raise DUPLICATE_MEMBER.";
}

TEST(MAREASemaTypesAndDeclarations, FunctionRepeatedInOneImplementationIsRejected)
{
    GTEST_COUT << "Validating the same function name twice in one implementation block is rejected." << std::endl;

    EXPECT_EQ(MAREA_codesOf("structure P x: float32; end implementation P function area() end function area() end end"), Codes{ DUPLICATE_MEMBER }) << "A repeated function in one block must raise DUPLICATE_MEMBER.";
}

TEST(MAREASemaTypesAndDeclarations, SameFunctionNameOnDifferentTypesIsAccepted)
{
    GTEST_COUT << "Validating the same function name on two different types is accepted." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure P x: float32; end structure Q y: float32; end implementation P function area() end end implementation Q function area() end end").empty()) << "Different types must not collide.";

    EXPECT_TRUE(MAREA_codesOf("structure B<Item> v: Item; end implementation B<sinteger32> function show() end end implementation B<float32> function show() end end").empty()) << "Different instances must not collide.";
}

TEST(MAREASemaTypesAndDeclarations, ErrorNodeInPlaceOfFieldOrTypeIsSkipped)
{
    GTEST_COUT << "Validating a broken field or type raises no sema diagnostic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: ; end").empty()) << "A broken field type must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("structure S : sinteger32; end").empty()) << "A broken field name must raise no sema diagnostic.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: ) end").empty()) << "A broken parameter type must raise no sema diagnostic.";
}

TEST(MAREASemaTypesAndDeclarations, LocalTypeAliasDeclaredBeforeOrAfterItsTarget)
{
    GTEST_COUT << "Validating a local type alias resolves whether its target comes before or after it." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Other x: float32; end type Name = Other; structure H a: Name; end").empty()) << "An alias after its target must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("type Name = Other; structure Other x: float32; end structure H a: Name; end").empty()) << "An alias before its target must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("type Name = Missing; structure H a: Name; end"), Codes{ UNKNOWN_TYPE }) << "An alias of a missing type must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, ChainOfTypeAliasesResolves)
{
    GTEST_COUT << "Validating a chain of two type aliases resolves to the type at its end." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end type Mid = Point; type Top = Mid; structure H a: Top; end").empty()) << "A chain of aliases must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, AliasOfAPrimitiveIsThePrimitive)
{
    GTEST_COUT << "Validating an alias of a primitive meets a contract that only a primitive meets." << std::endl;

    const std::string COPY{ "function copy_of<Item>(a: Item): Item where Item: Copyable return copy a; end " };

    EXPECT_TRUE(MAREA_codesOf(COPY + "type Id = sinteger32; function run(a: Id): Id return copy_of<Id>(a); end").empty()) << "An alias of a primitive must be Copyable.";
}

TEST(MAREASemaTypesAndDeclarations, AliasTypeMatchesTheOriginalInAContractSignature)
{
    GTEST_COUT << "Validating a type written with a local alias or an import alias equals the original in a contract signature." << std::endl;

    auto external = [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; };

    const std::string BASE{ "contract Joiner function join(self: Self, other: Thing): boolean; end structure Score points: sinteger32; end function join_all<Item>(a: Item): Item where Item: Joiner return copy a; end " };

    const std::string RUN{ "function run(a: Score): Score return join_all(a); end" };

    const std::string LOCAL{ "type Thing = Other; structure Other end type Back = Thing; implementation Score function join(self: Score, other: Back): boolean return true; end end " };

    EXPECT_TRUE(MAREA_codesOf(BASE + LOCAL + RUN, external).empty()) << "A local alias chain must equal the original.";

    const std::string WHOLE{ "implementation Score function join(self: Score, other: Alias): boolean return true; end end " };

    EXPECT_TRUE(MAREA_codesOf("import lib.Thing; import lib.Thing as Alias; " + BASE + WHOLE + RUN, external).empty()) << "A whole-path import alias must equal the original.";

    const std::string MEMBER{ "implementation Score function join(self: Score, other: Alias): boolean return true; end end " };

    EXPECT_TRUE(MAREA_codesOf("import lib.Thing; import lib { Thing as Alias }; " + BASE + MEMBER + RUN, external).empty()) << "A member alias must equal the original.";

    const std::string WRONG{ "implementation Score function join(self: Score, other: Alias): boolean return true; end end " };

    EXPECT_EQ(MAREA_codesOf("import lib.Thing; import lib { Other as Alias }; " + BASE + WRONG + RUN, external), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED }) << "An alias of another type must still miss the contract.";
}

TEST(MAREASemaTypesAndDeclarations, AliasesOfDifferentNamespacesDoNotMatchInAContractSignature)
{
    GTEST_COUT << "Validating two aliases of same-named members of two namespaces are two types in a contract signature." << std::endl;

    auto external = [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; };

    const std::string BASE{ "contract Joiner function join(self: Self, other: X): boolean; end structure Score points: sinteger32; end function join_all<Item>(a: Item): Item where Item: Joiner return copy a; end " };

    const std::string RUN{ "function run(a: Score): Score return join_all(a); end" };

    const std::string IMPLEMENTATION{ "implementation Score function join(self: Score, other: Y): boolean return true; end end " };

    EXPECT_EQ(MAREA_codesOf("import a { T as X }; import b { T as Y }; " + BASE + IMPLEMENTATION + RUN, external), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED }) << "Members of two namespaces must stay two types.";

    EXPECT_TRUE(MAREA_codesOf("import a { T as X }; import a { T as Y }; " + BASE + IMPLEMENTATION + RUN, external).empty()) << "Two aliases of one member must stay one type.";

    EXPECT_EQ(MAREA_codesOf("import a.T as X; import b.T as Y; " + BASE + IMPLEMENTATION + RUN, external), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED }) << "Whole-path aliases of two namespaces must stay two types.";
}
