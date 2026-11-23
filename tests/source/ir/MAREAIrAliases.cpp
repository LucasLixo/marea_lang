// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return (text.compare(0, 12, "collections.") == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    // INFO: lowers a unit and gives the exact text dump of the module, or a marker that says the unit did not lower.
    std::string MAREA_dumpOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

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

    using Codes = std::vector<MAREA_Uinteger32>;

    Codes MAREA_codesOf(const std::string &source)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        MAREA_IGNORE(fixture.MAREA_lower(source));

        return fixture.MAREA_irCodes();
    }
} // namespace

TEST(MAREAIrAliases, NamespaceAliasHeadLowersToTheLastSegment)
{
    GTEST_COUT << "Validating the head of a member chain that is a namespace alias is the last segment of the path." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list as lst; function run() lst.push(1); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"list\"")) << "The head must be the original: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "lst")) << "The alias must not appear: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"push\"")) << "The member name is not a head and stays: " << DUMP;
}

TEST(MAREAIrAliases, SymbolAliasCallLowersToTheOriginal)
{
    GTEST_COUT << "Validating a call of a member alias names the original function." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { push as add }; function run() add(1); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"push\"")) << "The callee must be the original: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "add")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, MemberAliasTypeLowersInAFieldAndAParameter)
{
    GTEST_COUT << "Validating a type that a member alias renames is the original name in a field, a parameter and a local." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { Item as Entry }; structure Box value: Entry; end function take(e: Entry) local x: Entry = e; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"value\" type=Item")) << "The field type must be the original: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"e\" type=Item")) << "The parameter type must be the original: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "LOCAL_DECLARATION \"x\" type=Item")) << "The local type must be the original: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "Entry")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, ConstantAliasLowersToTheOriginal)
{
    GTEST_COUT << "Validating a constant that a member alias renames is read under the original name." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.limits { MAX as LIMIT }; function run(): sinteger32 return LIMIT; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"MAX\"")) << "The constant must be the original: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "LIMIT")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, VariantAliasLowersToTheTwoPartPath)
{
    GTEST_COUT << "Validating a variant alias lowers to the member path as written, joined with a dot." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.shade { Color, Color.red as Crimson }; function run() local c: Color = Crimson; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"Color.red\"")) << "The variant must be the original path: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "Crimson")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, LocalTypeAliasLowersInFieldsAndLiteralsAndChains)
{
    GTEST_COUT << "Validating a local type alias, also in a chain, is the original type name everywhere a type is written." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; structure Point x: sinteger32; end type First = Point; type Second = First; structure Holder p: Second; end function run(a: Second) local q: First = First { x = 1 }; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"p\" type=Point")) << "A chain of aliases must reach the original: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"a\" type=Point")) << "The parameter type must be the original: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "LOCAL_DECLARATION \"q\" type=Point")) << "The local type must be the original: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "LITERAL_VALUE") && MAREA_has(DUMP, "type=Point")) << "The literal must carry the original: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "First")) << "The alias must not appear: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "Second")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, LocalTypeAliasOfAnImportedAliasFollowsTheChain)
{
    GTEST_COUT << "Validating a local type alias of a member alias reaches the imported original." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { Item as Entry }; type Mine = Entry; structure Box value: Mine; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"value\" type=Item")) << "The chain must end at the imported original: " << DUMP;
}

TEST(MAREAIrAliases, QualifiedTypeWithAliasHeadLowersTheHead)
{
    GTEST_COUT << "Validating a type written through a namespace alias gets the original namespace segment." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list as lst; structure Box value: lst.Item; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"value\" type=list.Item")) << "The head must be the original: " << DUMP;
}

TEST(MAREAIrAliases, ParameterShadowsAnAlias)
{
    GTEST_COUT << "Validating a parameter with the name of an alias keeps its own name." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { push as add }; function run(add: sinteger32): sinteger32 return add; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"add\"")) << "The parameter must keep its name: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "push")) << "The alias must not be rewritten there: " << DUMP;
}

TEST(MAREAIrAliases, LocalShadowsAnAliasOnlyInItsOwnFunction)
{
    GTEST_COUT << "Validating a local with the name of an alias shadows it in its function and not in another." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { push as add }; function one(): sinteger32 local add: sinteger32 = 1; return add; end function two() add(2); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FUNCTION_DECLARATION \"one\"")) << "The first function must lower: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "RETURN_STATEMENT\n        IDENTIFIER \"add\"")) << "The local must keep its name: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"push\"")) << "The other function must see the original: " << DUMP;
}

TEST(MAREAIrAliases, LocalShadowsAnAliasOnlyFromItsDeclarationOnward)
{
    GTEST_COUT << "Validating a reference before the local with the name of an alias still lowers to the original." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { push as add }; function run(): sinteger32 add(1); local add: sinteger32 = 2; return add; end") };

    EXPECT_NE(DUMP, "<lower failed>") << "The unit must lower.";
    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"push\"")) << "The reference before the local must see the original: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "RETURN_STATEMENT\n        IDENTIFIER \"add\"")) << "The reference after the local must keep its name: " << DUMP;
}

TEST(MAREAIrAliases, LoopVariableShadowsAnAlias)
{
    GTEST_COUT << "Validating a loop variable with the name of an alias keeps its own name." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { push as add }; function run(): sinteger32 for add in 0 .. 3 do end return 0; end") };

    EXPECT_NE(DUMP, "<lower failed>") << "The unit must lower.";
    EXPECT_FALSE(MAREA_has(DUMP, "push")) << "The loop variable hides the alias in its function: " << DUMP;
}

TEST(MAREAIrAliases, MemberNameAndFieldEntryAreNeverRenamed)
{
    GTEST_COUT << "Validating a member name and a field entry with the spelling of an alias are not heads, so they stay." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list { size as count }; structure Counter count: sinteger32; end function run(c: Counter): sinteger32 local d: Counter = Counter { count = 1 }; return c.count; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_ENTRY \"count\"")) << "The field entry must keep its name: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"count\"")) << "The member name must stay: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "size")) << "No head names the alias, so nothing is rewritten: " << DUMP;
}

TEST(MAREAIrAliases, StaticCallOnAnAliasedTypeLowersTheTypeAndKeepsTheMember)
{
    GTEST_COUT << "Validating a static call on a type alias names the original type and keeps the member." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.array { array as list }; function run(): sinteger32 return list<sinteger32>.filled(8, 0).length(); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "TYPE_NAME \"array\"")) << "The type must be the original: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "\"list\"")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, DumpHasNoImportNodeAndNoAliasText)
{
    GTEST_COUT << "Validating the dump of a unit with every alias form has no import node and no alias text." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.list as lst; import collections.list { push as add, Item as Entry, MAX as LIMIT }; type Mine = Entry; function run(e: Mine): sinteger32 add(LIMIT); lst.push(1); return LIMIT; end") };

    EXPECT_NE(DUMP, "<lower failed>") << "The unit must lower.";
    EXPECT_FALSE(MAREA_has(DUMP, "IMPORT")) << "An import makes no node: " << DUMP;

    for (const char *alias : { "lst", "add", "Entry", "LIMIT", "Mine" }) {
        EXPECT_FALSE(MAREA_has(DUMP, alias)) << "The alias " << alias << " must not appear: " << DUMP;
    }
}

TEST(MAREAIrAliases, TextDumpOfALoweredUnitIsExact)
{
    GTEST_COUT << "Validating the exact dump of a small unit that uses a member alias and a type alias." << std::endl;

    EXPECT_EQ(MAREA_dumpOf("namespace main; import collections.list { push as add, Item as Entry }; function run(e: Entry) add(1); end"), MAREA_lines({
                                                                                                                                              "MODULE",
                                                                                                                                              "  FUNCTION_DECLARATION \"run\"",
                                                                                                                                              "    PARAMETER_LIST",
                                                                                                                                              "      PARAMETER \"e\" type=Item",
                                                                                                                                              "    BLOCK",
                                                                                                                                              "      EXPRESSION_STATEMENT",
                                                                                                                                              "        CALL_EXPRESSION",
                                                                                                                                              "          IDENTIFIER \"push\"",
                                                                                                                                              "          ARGUMENT_LIST",
                                                                                                                                              "            INTEGER_LITERAL \"1\"",
                                                                                                                                          }))
        << "The dump must hold the original names only.";
}

TEST(MAREAIrAliases, MemberListNamesAreBoundForBuiltinLookup)
{
    GTEST_COUT << "Validating a member list binds its local names, so a built-in name is a user call only when a member binds it." << std::endl;

    const Codes GENERIC{ 4004 };

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.list { size_of }; function run(): sinteger32 return size_of<sinteger32>(); end"), GENERIC) << "A member with a built-in name binds it, so the call is a user generic.";

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.list { helper as size_of }; function run(): sinteger32 return size_of<sinteger32>(); end"), GENERIC) << "An alias with a built-in name binds it.";

    EXPECT_EQ(MAREA_codesOf("namespace main; import collections.list { size_of as measure }; function run(): sinteger32 return size_of<sinteger32>(); end"), Codes{}) << "The original of an alias is not bound, so the built-in is kept.";
}

TEST(MAREAIrAliases, GenericTypeAliasStillRaises)
{
    GTEST_COUT << "Validating a generic type alias of the unit keeps raising UNINSTANTIATED_GENERIC." << std::endl;

    const Codes CODES{ MAREA_codesOf("namespace main; import collections.array; type Many<Item> = array<Item>; function run(): sinteger32 return Many<sinteger32>.filled(8, 0).length(); end") };

    ASSERT_FALSE(CODES.empty()) << "The generic alias must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, 4004U) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }
}

TEST(MAREAIrAliases, FunctionTypeAliasLowersLikeTheWrittenFunctionType)
{
    GTEST_COUT << "Validating an alias of a function type lowers as the function type itself does, so no alias name stays." << std::endl;

    EXPECT_EQ(MAREA_dumpOf("namespace main; type Callback = function(v: sinteger32): boolean; structure Holder f: Callback; end function run(a: Callback) end"), MAREA_lines({
                                                                                                                                                                     "MODULE",
                                                                                                                                                                     "  STRUCTURE_DECLARATION \"Holder\"",
                                                                                                                                                                     "    FIELD_DECLARATION \"f\"",
                                                                                                                                                                     "  FUNCTION_DECLARATION \"run\"",
                                                                                                                                                                     "    PARAMETER_LIST",
                                                                                                                                                                     "      PARAMETER \"a\"",
                                                                                                                                                                     "    BLOCK",
                                                                                                                                                                 }))
        << "The alias name must not stay.";
}

TEST(MAREAIrAliases, FunctionTypeAliasInAChainLowersToo)
{
    GTEST_COUT << "Validating an alias of an alias of a function type lowers like the function type." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; type Callback = function(v: sinteger32): boolean; type Handler = Callback; function run(a: Handler) end") };

    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"a\"\n")) << "The parameter must carry no type name: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "Handler")) << "The alias must not appear: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "Callback")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, TypeArgumentAliasLowersToTheHeadName)
{
    GTEST_COUT << "Validating an alias of a type with concrete type arguments lowers to the head name, as the written type does." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import collections.array; type Ints = array<sinteger32>; type More = Ints; structure Box value: Ints; end function take(e: More) local x: Ints = e; end") };

    EXPECT_TRUE(MAREA_has(DUMP, "FIELD_DECLARATION \"value\" type=array")) << "The field type must be the head name: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "PARAMETER \"e\" type=array")) << "The chain must reach the head name: " << DUMP;
    EXPECT_TRUE(MAREA_has(DUMP, "LOCAL_DECLARATION \"x\" type=array")) << "The local type must be the head name: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "Ints")) << "The alias must not appear: " << DUMP;
    EXPECT_FALSE(MAREA_has(DUMP, "More")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrAliases, TypeOperandThroughALossyAliasRaises)
{
    GTEST_COUT << "Validating a type operand named by an alias that carries type arguments raises, so no argument is lost." << std::endl;

    const Codes CODES{ MAREA_codesOf("namespace main; import collections.array; type Ints = array<sinteger32>; constant SIZE: uinteger64 = size_of<Ints>();") };

    ASSERT_FALSE(CODES.empty()) << "The operand must raise.";

    for (MAREA_Uinteger32 code : CODES) {
        EXPECT_EQ(code, 4004U) << "Only UNINSTANTIATED_GENERIC may be raised.";
    }
}
