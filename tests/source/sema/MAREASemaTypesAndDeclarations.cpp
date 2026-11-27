// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 UNKNOWN_TYPE{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE };
    constexpr MAREA_Uinteger32 OPAQUE_BY_VALUE{ MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_FIELD_BY_VALUE };
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

    EXPECT_TRUE(MAREA_codesOf("structure S a: boolean; b: sinteger8; c: sinteger16; d: sinteger32; e: sinteger64; f: uinteger8; g: uinteger16; h: uinteger32; i: uinteger64; j: float32; k: float64; l: string; m: character; end").empty()) << "A primitive type must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, SignedAndUnsignedIntegerNamesResolve)
{
    GTEST_COUT << "Validating every sinteger and uinteger width resolves, also inside a generic." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: sinteger8; b: sinteger16; c: sinteger32; d: sinteger64; e: uinteger8; f: uinteger16; g: uinteger32; h: uinteger64; i: demo.slice<sinteger32>; j: demo.slice<uinteger8>; end").empty()) << "A sinteger or uinteger name must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, RetiredIntegerNamesAreUnknownTypes)
{
    GTEST_COUT << "Validating the retired integerN and unsignedN names raise UNKNOWN_TYPE." << std::endl;

    for (const char *name : { "integer8", "integer16", "integer32", "integer64", "unsigned8", "unsigned16", "unsigned32", "unsigned64" }) {
        EXPECT_EQ(MAREA_codesOf(std::string("structure S a: ") + name + "; end"), Codes{ UNKNOWN_TYPE }) << "The retired name " << name << " must raise UNKNOWN_TYPE.";
        EXPECT_EQ(MAREA_codesOf(std::string("structure S a: demo.slice<") + name + ">; end"), Codes{ UNKNOWN_TYPE }) << "The retired name " << name << " must raise UNKNOWN_TYPE inside a generic.";
    }
}

TEST(MAREASemaTypesAndDeclarations, CoreGenericTypesResolve)
{
    GTEST_COUT << "Validating array, vector, pointer, owned, slice and task resolve." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure S a: demo.array<float32, 4>; f: demo.vector<float32>; b: demo.slice<sinteger32>; c: demo.owned<string>; d: demo.slice<uinteger8>; e: demo.task<float32>; end").empty()) << "A core generic type must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, CoreGenericTypeTakesAGenericParameter)
{
    GTEST_COUT << "Validating a core generic type accepts a generic parameter of the declaration." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Box<Item> inner: demo.slice<Item>; end").empty()) << "A generic parameter must be in scope.";
}

TEST(MAREASemaTypesAndDeclarations, ExternalTypesResolve)
{
    GTEST_COUT << "Validating a type the resolver reports as EXTERNAL resolves." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("import collections.list; require c.math.Vector3; structure S a: list<sinteger32>; b: table<string, sinteger32>; c: handle<sinteger32>; d: collections.list<sinteger32>; e: c.math.Vector3; end").empty()) << "An external type must raise nothing.";
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

    EXPECT_EQ(MAREA_codesOf("structure S a: demo.slice<Missing>; end"), Codes{ UNKNOWN_TYPE }) << "A missing type argument must raise UNKNOWN_TYPE.";
}

TEST(MAREASemaTypesAndDeclarations, LocalTypesResolve)
{
    GTEST_COUT << "Validating a structure, an enum, an opaque type and a type alias of the file resolve." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("structure Point x: float32; end enum Color red; green; end opaque structure Handle; type Alias = Point; structure Holder a: Point; b: Color; c: demo.slice<Handle>; d: Alias; end").empty()) << "A local type must raise nothing.";
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

    EXPECT_TRUE(MAREA_codesOf("import arenas.Arena as Arena; structure S a: Arena; end", resolve).empty()) << "An imported symbol must resolve as a type.";
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

    EXPECT_TRUE(MAREA_codesOf("import lib.Thing as Thing; import lib.Thing as Alias; " + BASE + WHOLE + RUN, external).empty()) << "A whole-path import alias must equal the original.";

    const std::string MEMBER{ "implementation Score function join(self: Score, other: Alias): boolean return true; end end " };

    EXPECT_TRUE(MAREA_codesOf("import lib.Thing as Thing; import lib { Thing as Alias }; " + BASE + MEMBER + RUN, external).empty()) << "A member alias must equal the original.";

    const std::string WRONG{ "implementation Score function join(self: Score, other: Alias): boolean return true; end end " };

    EXPECT_EQ(MAREA_codesOf("import lib.Thing as Thing; import lib { Other as Alias }; " + BASE + WRONG + RUN, external), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED }) << "An alias of another type must still miss the contract.";
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

TEST(MAREASemaTypesAndDeclarations, EnumTagTypeMustBeAnIntegerPrimitive)
{
    GTEST_COUT << "Validating an enum tag is an integer primitive and defaults to sinteger32." << std::endl;

    for (const char *name : { "sinteger8", "sinteger16", "sinteger32", "sinteger64", "uinteger8", "uinteger16", "uinteger32", "uinteger64" }) {
        EXPECT_TRUE(MAREA_codesOf(std::string("enum A : ") + name + " Red; Green; end").empty()) << "The tag " << name << " must raise nothing.";
    }

    EXPECT_TRUE(MAREA_codesOf("enum A Red; Green; end").empty()) << "An enum with no tag must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, EnumTagTypeOfAnotherKindRaisesInvalidEnumTagType)
{
    GTEST_COUT << "Validating a float, string, boolean, structure or generic enum tag raises INVALID_ENUM_TAG_TYPE." << std::endl;

    constexpr MAREA_Uinteger32 INVALID_TAG{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ENUM_TAG_TYPE };

    for (const char *name : { "float32", "float64", "string", "boolean", "SomeStruct", "demo.slice<sinteger32>" }) {
        EXPECT_EQ(MAREA_codesOf(std::string("structure SomeStruct a: sinteger32; end enum A : ") + name + " Red; end"), Codes{ INVALID_TAG }) << "The tag " << name << " must raise INVALID_ENUM_TAG_TYPE.";
    }
}

TEST(MAREASemaTypesAndDeclarations, OpaqueTypeHeldByValueInAFieldRaisesOpaqueFieldByValue)
{
    GTEST_COUT << "Validating a bare opaque type as a structure field raises OPAQUE_FIELD_BY_VALUE." << std::endl;

    EXPECT_EQ(MAREA_codesOf("opaque structure T; structure H t: T; end"), Codes{ OPAQUE_BY_VALUE }) << "An opaque type held by value must raise OPAQUE_FIELD_BY_VALUE.";

    EXPECT_EQ(MAREA_codesOf("opaque structure T; structure H a: sinteger32; t: T; u: T; end"), (Codes{ OPAQUE_BY_VALUE, OPAQUE_BY_VALUE })) << "Each opaque field must raise once.";

    EXPECT_TRUE(MAREA_codesOf("opaque structure T; structure H t: demo.owned<T>; end").empty()) << "An owner of an opaque type in a field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("opaque structure T; structure H t: demo.slice<T>; end").empty()) << "A pointer to an opaque type in a field must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("structure S a: sinteger32; end structure H s: S; end").empty()) << "An ordinary structure held by value must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("require c.math.Vector3; structure H t: c.math.Vector3; end").empty()) << "A type of another file is not checked.";
}

TEST(MAREASemaTypesAndDeclarations, OpaqueTypeOfAnotherFileIsClassified)
{
    GTEST_COUT << "Validating an opaque structure of a FOUND namespace raises 3057 and 3058 as one of this file does." << std::endl;

    MAREA_SemaFixture library;

    ASSERT_EQ(library.parser.MAREA_parse("namespace a.b; opaque structure Texture; structure Box x: sinteger32; end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The library must parse.";

    const MAREA_ResolveCallback FOUND{ [&library](const std::string &text, const MAREA_AstNode **outRoot) {
        if (text != "a.b") {
            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        }

        *outRoot = library.parser.root;

        return MAREA_SEMA_IMPORT_RESULT_FOUND;
    } };

    constexpr MAREA_Uinteger32 MISUSE{ MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE };

    EXPECT_EQ(MAREA_codesOf("import a.b as b; structure H t: b.Texture; end", FOUND), Codes{ OPAQUE_BY_VALUE }) << "A field of an opaque type reached by an alias must raise 3058.";
    EXPECT_EQ(MAREA_codesOf("import a.b; structure H t: a.b.Texture; end", FOUND), Codes{ OPAQUE_BY_VALUE }) << "A field of an opaque type reached by a full path must raise 3058.";
    EXPECT_EQ(MAREA_codesOf("import a.b.Texture as Image; structure H t: Image; end", FOUND), Codes{ OPAQUE_BY_VALUE }) << "A field of an imported opaque type must raise 3058.";
    EXPECT_EQ(MAREA_codesOf("import a.b as b; function f(t: b.Texture) local x: sinteger32 = t.width; end", FOUND), Codes{ MISUSE }) << "A field read on an imported opaque value must raise 3057.";
    EXPECT_EQ(MAREA_codesOf("import a.b as b; function f(t: b.Texture, u: b.Texture) local x: b.Texture = t + u; end", FOUND), Codes{ MISUSE }) << "An operator on imported opaque values must raise 3057.";
    EXPECT_EQ(MAREA_codesOf("import a.b as b; function f() local t: b.Texture = b.Texture{ }; end", FOUND), Codes{ MISUSE }) << "A literal of an imported opaque type must raise 3057.";
    EXPECT_TRUE(MAREA_codesOf("import a.b as b; structure H box: b.Box; end function f(v: b.Box) local x: sinteger32 = v.x; end", FOUND).empty()) << "A structure of another file must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, UnknownVariantOfAnEnumOfAnotherFileIsRejected)
{
    GTEST_COUT << "Validating UNKNOWN_ENUM_VARIANT for an enum of a FOUND namespace and an imported enum." << std::endl;

    MAREA_SemaFixture library;

    ASSERT_EQ(library.parser.MAREA_parse("namespace a.b; enum Color Red; Green; end implementation Color function paint(self: Color) end end"), MAREA_PARSE_EXCEPT_SUCCESS) << "The library must parse.";

    const MAREA_ResolveCallback FOUND{ [&library](const std::string &text, const MAREA_AstNode **outRoot) {
        if (text != "a.b") {
            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        }

        *outRoot = library.parser.root;

        return MAREA_SEMA_IMPORT_RESULT_FOUND;
    } };

    constexpr MAREA_Uinteger32 VARIANT{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ENUM_VARIANT };

    EXPECT_EQ(MAREA_codesOf("import a.b as b; function f() local c: b.Color = b.Color.Purple; end", FOUND), Codes{ VARIANT }) << "An enum reached by an alias must be checked.";
    EXPECT_EQ(MAREA_codesOf("import a.b; function f() local c: a.b.Color = a.b.Color.Purple; end", FOUND), Codes{ VARIANT }) << "An enum reached by a full path must be checked.";
    EXPECT_EQ(MAREA_codesOf("import a.b.Color as Paint; function f() local c: Paint = Paint.Purple; end", FOUND), Codes{ VARIANT }) << "An imported enum must be checked.";
    EXPECT_TRUE(MAREA_codesOf("import a.b as b; function f() local c: b.Color = b.Color.Red; b.Color.paint(c); end", FOUND).empty()) << "A variant and an implemented function must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf("import a.b; function f() local c: a.b.Color = a.b.Color.Green; end", FOUND).empty()) << "A variant written with a full path must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf("import a.b.Color as Paint; function f() local c: Paint = Paint.Red; end", FOUND).empty()) << "A variant of an imported enum must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, StructureHoldingItselfByValueIsRecursiveType)
{
    GTEST_COUT << "Validating a structure or enum that holds itself by value raises RECURSIVE_TYPE." << std::endl;

    constexpr MAREA_Uinteger32 RECURSIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_RECURSIVE_TYPE };

    EXPECT_EQ(MAREA_codesOf("structure Node next: Node; end"), Codes{ RECURSIVE }) << "A direct self field must raise once.";
    EXPECT_EQ(MAREA_codesOf("structure Left other: Right; end structure Right other: Left; end"), (Codes{ RECURSIVE, RECURSIVE })) << "Two structures that hold each other must raise once each.";
    EXPECT_EQ(MAREA_codesOf("structure A items: demo.array<A, 2>; end"), Codes{ RECURSIVE }) << "An array holds its element inline.";
    EXPECT_EQ(MAREA_codesOf("type Same = Node; structure Node next: Same; end"), Codes{ RECURSIVE }) << "An alias must be followed.";
    EXPECT_EQ(MAREA_codesOf("enum Tree Leaf; Branch(inner: Tree); end"), Codes{ RECURSIVE }) << "A variant field that holds the enum by value must raise.";
}

TEST(MAREASemaTypesAndDeclarations, RecursionThroughAGenericStructureIsReported)
{
    GTEST_COUT << "Validating a field of an instantiated generic structure or enum is followed with its type arguments." << std::endl;

    constexpr MAREA_Uinteger32 RECURSIVE{ MAREA_SEMA_DIAGNOSTIC_CODE_RECURSIVE_TYPE };

    const std::string BOX{ "structure Box<T> value: T; end " };

    EXPECT_EQ(MAREA_codesOf(BOX + "structure Node next: Box<Node>; end"), Codes{ RECURSIVE }) << "A wrapper holds its argument by value.";
    EXPECT_EQ(MAREA_codesOf(BOX + "structure Node next: Box<Box<Node>>; end"), Codes{ RECURSIVE }) << "A wrapper of a wrapper too.";
    EXPECT_EQ(MAREA_codesOf("structure Pair<A, B> first: A; end structure Node next: Pair<Node, sinteger32>; end"), Codes{ RECURSIVE }) << "A held parameter.";
    EXPECT_EQ(MAREA_codesOf("structure Pair<A, B> first: A; end structure Node next: Pair<sinteger32, Node>; end"), Codes{}) << "A parameter that no field holds.";
    EXPECT_EQ(MAREA_codesOf(BOX + "enum Tree Leaf; Branch(inner: Box<Tree>); end"), Codes{ RECURSIVE }) << "A variant field that holds the enum through a wrapper.";
    EXPECT_EQ(MAREA_codesOf("structure Holder<T> items: demo.vector<T>; end structure Node next: Holder<Node>; end"), Codes{}) << "A vector is indirect.";
    EXPECT_EQ(MAREA_codesOf("structure Node next: Wrap<sinteger32>; end structure Wrap<T> back: Node; end"), (Codes{ RECURSIVE, RECURSIVE })) << "A generic structure that holds the target.";
    EXPECT_EQ(MAREA_codesOf("structure Grow<T> left: Grow<Grow<T>>; right: Grow<Grow<T>>; end"), Codes{ RECURSIVE }) << "An expanding recursion ends.";
    EXPECT_EQ(MAREA_codesOf(BOX + "structure Node next: sinteger32; end structure Other next: Box<Node>; end"), Codes{}) << "No cycle.";
}

TEST(MAREASemaTypesAndDeclarations, IndirectSelfReferenceIsAccepted)
{
    GTEST_COUT << "Validating an indirect self reference and a plain nesting raise nothing." << std::endl;

    for (const char *wrapper : { "demo.owned", "demo.vector", "demo.slice" }) {
        EXPECT_TRUE(MAREA_codesOf(std::string("structure Node next: ") + wrapper + "<Node>; end").empty()) << "The wrapper " << wrapper << " is indirect.";
    }

    EXPECT_TRUE(MAREA_codesOf("type A = sinteger32; type B = A;").empty()) << "A chain of aliases that ends must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf("structure Outer inner: Inner; end structure Inner x: sinteger32; end").empty()) << "A later declaration is fine.";
}

TEST(MAREASemaTypesAndDeclarations, CyclicAliasIsRejected)
{
    GTEST_COUT << "Validating a cyclic type alias raises CYCLIC_TYPE_ALIAS once per alias on the cycle." << std::endl;

    constexpr MAREA_Uinteger32 CYCLIC{ MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_TYPE_ALIAS };

    EXPECT_EQ(MAREA_codesOf("type First = Second; type Second = First;"), (Codes{ CYCLIC, CYCLIC })) << "Two aliases that lead to each other must raise twice.";
    EXPECT_EQ(MAREA_codesOf("type Loop = Loop;"), Codes{ CYCLIC }) << "An alias of itself must raise once.";
}

TEST(MAREASemaTypesAndDeclarations, OverlappingImplementationIsRejected)
{
    GTEST_COUT << "Validating two implementation blocks that unify and share a member raise OVERLAPPING_IMPLEMENTATION." << std::endl;

    constexpr MAREA_Uinteger32 OVERLAP{ MAREA_SEMA_DIAGNOSTIC_CODE_OVERLAPPING_IMPLEMENTATION };

    const std::string BOX{ "structure Box<Item> item: Item; end " };

    EXPECT_EQ(MAREA_codesOf(BOX + "implementation<Item> Box<Item> function same(self: Box<Item>): sinteger32 return 1; end end implementation Box<sinteger32> function same(self: Box<sinteger32>): sinteger32 return 2; end end"), Codes{ OVERLAP }) << "A generic block and a concrete block with one name must raise once.";
    EXPECT_TRUE(MAREA_codesOf(BOX + "implementation<Item> Box<Item> function same(self: Box<Item>): sinteger32 return 1; end end implementation Box<sinteger32> function other(self: Box<sinteger32>): sinteger32 return 2; end end").empty()) << "Different member names must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf(BOX + "implementation Box<sinteger32> function same(self: Box<sinteger32>): sinteger32 return 1; end end implementation Box<float64> function same(self: Box<float64>): sinteger32 return 2; end end").empty()) << "Two blocks that do not unify must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, EnumWithNoVariantIsRejected)
{
    GTEST_COUT << "Validating an enum with no variant raises EMPTY_ENUM once, and an enum with a variant does not." << std::endl;

    constexpr MAREA_Uinteger32 EMPTY_ENUM{ MAREA_SEMA_DIAGNOSTIC_CODE_EMPTY_ENUM };

    EXPECT_EQ(MAREA_codesOf("enum Empty end"), Codes{ EMPTY_ENUM }) << "An enum with no variant must raise EMPTY_ENUM.";
    EXPECT_EQ(MAREA_codesOf("enum Small : uinteger8 end"), Codes{ EMPTY_ENUM }) << "A tagged enum with no variant must raise EMPTY_ENUM.";
    EXPECT_TRUE(MAREA_codesOf("enum Small : uinteger8 A; end").empty()) << "A tagged enum with a variant must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf("enum Color Red; Green; end").empty()) << "An enum with variants must raise nothing.";
}

TEST(MAREASemaTypesAndDeclarations, NameThatIsNotAnEnumVariantIsRejected)
{
    GTEST_COUT << "Validating Enum.Name raises UNKNOWN_ENUM_VARIANT unless Name is a variant, a function or a constant of the enum." << std::endl;

    constexpr MAREA_Uinteger32 UNKNOWN_VARIANT{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ENUM_VARIANT };

    const std::string COLOR{ "enum Color Red; Green; end " };

    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(): Color return Color.Purple; end"), Codes{ UNKNOWN_VARIANT }) << "A name that is not a variant must raise UNKNOWN_ENUM_VARIANT.";
    EXPECT_EQ(MAREA_codesOf(COLOR + "function f(): Color local c: Color = Color.Purple; return c; end"), Codes{ UNKNOWN_VARIANT }) << "A local initializer must be checked.";
    EXPECT_TRUE(MAREA_codesOf(COLOR + "function f(): Color return Color.Red; end").empty()) << "A variant must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf("enum Small : uinteger8 A; end function f(): Small return Small.A; end").empty()) << "A variant of a tagged enum must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf("enum Shape Circle(radius: float32); Square; end function f(): Shape return Shape.Square; end").empty()) << "A variant with a payload must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf(COLOR + "implementation Color constant DEFAULT_VALUE: sinteger32 = 1; end function f(): sinteger32 return Color.DEFAULT_VALUE; end").empty()) << "A constant of an implementation block must raise nothing.";
    EXPECT_TRUE(MAREA_codesOf(COLOR + "function f(Color: sinteger32): sinteger32 return Color.Purple; end").empty()) << "A parameter that hides the enum name must raise nothing.";
}
