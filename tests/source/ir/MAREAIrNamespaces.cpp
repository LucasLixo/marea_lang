// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return ((text.rfind("a.", 0) == 0) || (text.rfind("b.", 0) == 0) || (text.rfind("collections.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    // INFO: lowers a unit and gives the exact text dump of the module, or a marker.
    std::string MAREA_dumpOf(const std::string &source, const std::string &path = "main.marea")
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower(source, path) != MAREA_IR_EXCEPT_SUCCESS) {
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
} // namespace

TEST(MAREAIrNamespaces, ModuleCarriesItsFullNamespace)
{
    GTEST_COUT << "Validating the MODULE node holds the full namespace of the unit, so the name of each of its declarations is that namespace and the name." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace a.one; function greet() end", "a/one.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irText(fixture.module), "a.one") << "The module must carry the dotted namespace.";

    EXPECT_EQ(MAREA_irText(MAREA_irChild(fixture.module, 0)), "greet") << "The declaration keeps its own name.";
}

TEST(MAREAIrNamespaces, TwoNamespacesThatExportOneNameLowerToTwoNames)
{
    GTEST_COUT << "Validating two namespaces that each export greet are two different modules, so the two names never collide." << std::endl;

    MAREA_IrFixture first;

    MAREA_IrFixture second;

    ASSERT_EQ(first.MAREA_lower("namespace a.one; function greet() end", "a/one.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The first unit must lower.";

    ASSERT_EQ(second.MAREA_lower("namespace b.two; function greet() end", "b/two.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The second unit must lower.";

    EXPECT_NE(MAREA_irText(first.module) + "." + MAREA_irText(MAREA_irChild(first.module, 0)), MAREA_irText(second.module) + "." + MAREA_irText(MAREA_irChild(second.module, 0))) << "The full names must differ.";
}

TEST(MAREAIrNamespaces, DumpShowsTheNamespaceOnTheModuleLine)
{
    GTEST_COUT << "Validating the text dump writes the namespace after MODULE." << std::endl;

    EXPECT_EQ(MAREA_dumpOf("namespace a.one;", "a/one.marea"), "MODULE \"a.one\"\n") << "The namespace must be on the first line.";
}

TEST(MAREAIrNamespaces, TwoImportedNamespacesWithOneMemberNameKeepTwoNames)
{
    GTEST_COUT << "Validating a call of greet through two namespace aliases lowers to the full path of each namespace, never to the last segment." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import a.one as first; import b.two as second; function run() first.greet(); second.greet(); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"a.one\"")) << "The first alias must stand for the full path: " << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"b.two\"")) << "The second alias must stand for the full path: " << DUMP;
}

TEST(MAREAIrNamespaces, RequireAliasStandsForItsFullPathToo)
{
    GTEST_COUT << "Validating an alias of a require is lowered as the alias of an import is." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; require collections.list as items; function run() items.push(1); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"collections.list\"")) << "The alias of a require must be replaced by the full path: " << DUMP;

    EXPECT_FALSE(MAREA_has(DUMP, "items")) << "The alias must not appear: " << DUMP;
}

TEST(MAREAIrNamespaces, MemberAliasStandsForTheNamespaceThenTheMember)
{
    GTEST_COUT << "Validating a member alias stands for the namespace of the import, then the member path." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import a.one { greet as hello }; function run() hello(); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"a.one.greet\"")) << "The member must carry its namespace: " << DUMP;
}

TEST(MAREAIrNamespaces, FullPathWrittenInTheSourceIsKeptWhole)
{
    GTEST_COUT << "Validating a name written with its full path lowers as it is written." << std::endl;

    const std::string DUMP{ MAREA_dumpOf("namespace main; import a.one; function run() a.one.greet(); end") };

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"a\"")) << "The path must stay a chain of members: " << DUMP;

    EXPECT_TRUE(MAREA_has(DUMP, "IDENTIFIER \"greet\"")) << DUMP;
}
