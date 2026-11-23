// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>

namespace
{
    constexpr MAREA_Uinteger32 PATH_MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_NAMESPACE_PATH_MISMATCH };
    constexpr MAREA_Uinteger32 UNRESOLVED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT };
    constexpr MAREA_Uinteger32 ALREADY_DECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED };

    struct MAREA_LibraryResolver
    {
        MAREA_SemaFixture library;

        MAREA_LibraryResolver()
        {
            MAREA_IGNORE(library.parser.MAREA_parse("namespace a.b; #[visibility = public] function c() end function hidden() end #[visibility = public] enum Color red; green; end #[visibility = public] constant MAX: sinteger32 = 1; #[visibility = public] structure Item end"));
        }
    };

    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };

    std::string MAREA_firstMessage(const MAREA_SemaFixture &fixture, MAREA_Uinteger32 code)
    {
        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, index, &diagnostic));

            if (MAREA_DiagnosticGetCode(diagnostic) == code) {
                return MAREA_DiagnosticGetMessage(diagnostic);
            }
        }

        return {};
    }

    MAREA_ResolveCallback MAREA_externalAll()
    {
        return [](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; };
    }

    MAREA_ResolveCallback MAREA_answer(const std::string &found, MAREA_SemaImportResult result, const MAREA_AstNode *root = nullptr)
    {
        return [found, result, root](const std::string &text, const MAREA_AstNode **outRoot) {
            if (text == found) {
                *outRoot = root;

                return result;
            }

            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        };
    }
} // namespace

TEST(MAREASemaNamespaceAndNames, NamespaceMatchesPath)
{
    GTEST_COUT << "Validating a namespace that follows the file path raises nothing." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace a.b;", "a/b.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "A matching namespace must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, NamespaceWithExtraSegmentIsRejected)
{
    GTEST_COUT << "Validating a namespace with an extra segment is a path mismatch." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace a.b.c;", "a/b.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ PATH_MISMATCH }) << "An extra segment must raise NAMESPACE_PATH_MISMATCH.";
}

TEST(MAREASemaNamespaceAndNames, NamespaceWithMissingSegmentIsRejected)
{
    GTEST_COUT << "Validating a namespace with a missing segment is a path mismatch." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace a;", "a/b.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ PATH_MISMATCH }) << "A missing segment must raise NAMESPACE_PATH_MISMATCH.";
}

TEST(MAREASemaNamespaceAndNames, NamespaceWithWrongOrderIsRejected)
{
    GTEST_COUT << "Validating a namespace with segments in the wrong order is a path mismatch." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace b.a;", "a/b.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ PATH_MISMATCH }) << "Reordered segments must raise NAMESPACE_PATH_MISMATCH.";
}

TEST(MAREASemaNamespaceAndNames, ImportFoundAsNamespaceRaisesNothing)
{
    GTEST_COUT << "Validating an import that the resolver finds as a namespace raises nothing." << std::endl;

    MAREA_SemaFixture fixture(MAREA_answer("a.b.c", MAREA_SEMA_IMPORT_RESULT_EXTERNAL));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.c;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "A resolved import must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, ImportOfPublicSymbolRaisesNothing)
{
    GTEST_COUT << "Validating an import of a public top-level symbol of a found namespace raises nothing." << std::endl;

    MAREA_LibraryResolver library;

    MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.c;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "A public symbol import must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, ImportOfPrivateSymbolIsRejected)
{
    GTEST_COUT << "Validating an import of a symbol that is not public is unresolved." << std::endl;

    MAREA_LibraryResolver library;

    MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.hidden;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A private symbol must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaNamespaceAndNames, ImportHonoursFileVisibilityAndExplicitPrivate)
{
    GTEST_COUT << "Validating a file visibility makes symbols importable and a symbol visibility = private overrides it." << std::endl;

    MAREA_SemaFixture library;

    MAREA_IGNORE(library.parser.MAREA_parse("namespace a.b; #![visibility = public] function open() end #[visibility = private] function closed() end"));

    MAREA_SemaFixture openFixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.parser.root));

    ASSERT_EQ(openFixture.MAREA_sema("namespace main; import a.b.open;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(openFixture.MAREA_semaCodes().empty()) << "A symbol of a public file must be importable.";

    MAREA_SemaFixture closedFixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.parser.root));

    ASSERT_EQ(closedFixture.MAREA_sema("namespace main; import a.b.closed;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(closedFixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A private symbol of a public file must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaNamespaceAndNames, ImportOfMissingSymbolIsRejected)
{
    GTEST_COUT << "Validating an import of a symbol the namespace does not hold is unresolved." << std::endl;

    MAREA_LibraryResolver library;

    MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.missing;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A missing symbol must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaNamespaceAndNames, ImportNotFoundAnywhereIsRejected)
{
    GTEST_COUT << "Validating an import that both resolver answers miss is unresolved." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.c;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A missing import must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaNamespaceAndNames, SingleSegmentImportNotFoundIsRejected)
{
    GTEST_COUT << "Validating a one-segment import that is not found is unresolved." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import text;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A missing one-segment import must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaNamespaceAndNames, AliasedImportRaisesNothing)
{
    GTEST_COUT << "Validating an aliased import takes the alias as its local name." << std::endl;

    MAREA_SemaFixture fixture(MAREA_answer("a.b.c", MAREA_SEMA_IMPORT_RESULT_EXTERNAL));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.c as x; structure x2 end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "An aliased import must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, DeclarationClashingWithImportIsRejected)
{
    GTEST_COUT << "Validating a declaration named like an import is a clash." << std::endl;

    MAREA_SemaFixture fixture(MAREA_answer("a.b.c", MAREA_SEMA_IMPORT_RESULT_EXTERNAL));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.c; structure c end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ ALREADY_DECLARED }) << "A clash must raise NAME_ALREADY_DECLARED.";
}

TEST(MAREASemaNamespaceAndNames, AliasResolvesTheClash)
{
    GTEST_COUT << "Validating an alias removes the clash between a declaration and an import." << std::endl;

    MAREA_SemaFixture fixture(MAREA_answer("a.b.c", MAREA_SEMA_IMPORT_RESULT_EXTERNAL));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b.c as other; structure c end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "An alias must resolve the clash.";
}

TEST(MAREASemaNamespaceAndNames, TwoImportsWithTheSameLocalNameClash)
{
    GTEST_COUT << "Validating two imports with the same local name are a clash." << std::endl;

    MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.item; import b.item;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ ALREADY_DECLARED }) << "Two imports of one local name must clash.";
}

TEST(MAREASemaNamespaceAndNames, SameNameDeclaredTwiceIsRejected)
{
    GTEST_COUT << "Validating a top-level name declared twice is a clash." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Item end function Item() end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ ALREADY_DECLARED }) << "A repeated name must raise NAME_ALREADY_DECLARED.";
}

TEST(MAREASemaNamespaceAndNames, DistinctNamesRaiseNothing)
{
    GTEST_COUT << "Validating distinct top-level names raise nothing." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure First end structure Second end function third() end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "Distinct names must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, ErrorNodeInPlaceOfImportOrDeclarationIsSkipped)
{
    GTEST_COUT << "Validating an ERROR_NODE in place of an import or a declaration raises no sema diagnostic." << std::endl;

    MAREA_SemaFixture brokenImport;

    ASSERT_EQ(brokenImport.MAREA_sema("namespace main; import ;"), MAREA_SEMA_EXCEPT_SUCCESS) << "A broken import must still check.";

    EXPECT_TRUE(brokenImport.MAREA_semaCodes().empty()) << "A broken import must raise no sema diagnostic.";

    MAREA_SemaFixture brokenDeclaration;

    ASSERT_EQ(brokenDeclaration.MAREA_sema("namespace main; structure"), MAREA_SEMA_EXCEPT_SUCCESS) << "A broken declaration must still check.";

    EXPECT_TRUE(brokenDeclaration.MAREA_semaCodes().empty()) << "A broken declaration must raise no sema diagnostic.";
}

TEST(MAREASemaNamespaceAndNames, MemberListBindsEveryLocalName)
{
    GTEST_COUT << "Validating a member list binds the alias, or the last segment, of every member." << std::endl;

    MAREA_SemaFixture fixture(MAREA_externalAll());

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { f as g, T as U, Color.red as Crimson, MAX }; structure other end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "A member list must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, MemberAliasIsTheLocalName)
{
    GTEST_COUT << "Validating a member alias is the local name and the member name is free." << std::endl;

    MAREA_SemaFixture alias(MAREA_externalAll());

    ASSERT_EQ(alias.MAREA_sema("namespace main; import a.b { f as g }; structure f end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(alias.MAREA_semaCodes().empty()) << "The member name must stay free when it has an alias.";

    MAREA_SemaFixture plain(MAREA_externalAll());

    ASSERT_EQ(plain.MAREA_sema("namespace main; import a.b { f }; structure f end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(plain.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ ALREADY_DECLARED }) << "A member without alias binds its last segment.";
}

TEST(MAREASemaNamespaceAndNames, MemberAliasClashesAreRejected)
{
    GTEST_COUT << "Validating a member alias that repeats a local name is a clash." << std::endl;

    const std::vector<MAREA_Uinteger32> EXPECTED{ ALREADY_DECLARED };

    MAREA_SemaFixture withDeclaration(MAREA_externalAll());

    ASSERT_EQ(withDeclaration.MAREA_sema("namespace main; import a.b { f as g }; function g() end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(withDeclaration.MAREA_semaCodes(), EXPECTED) << "An alias and a declaration must clash.";

    MAREA_SemaFixture betweenAliases(MAREA_externalAll());

    ASSERT_EQ(betweenAliases.MAREA_sema("namespace main; import a.b { f as g, h as g };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(betweenAliases.MAREA_semaCodes(), EXPECTED) << "Two aliases of one name must clash.";

    MAREA_SemaFixture withPlainImport(MAREA_externalAll());

    ASSERT_EQ(withPlainImport.MAREA_sema("namespace main; import a.g; import a.b { f as g };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(withPlainImport.MAREA_semaCodes(), EXPECTED) << "An alias and a plain import must clash.";
}

TEST(MAREASemaNamespaceAndNames, AliasNamedCIsReserved)
{
    GTEST_COUT << "Validating an alias named c is rejected with the reserved-name message." << std::endl;

    const std::vector<MAREA_Uinteger32> EXPECTED{ ALREADY_DECLARED };

    const char *const SOURCES[]{ "namespace main; import a.b as c;", "namespace main; import a.b { f as c };", "namespace main; type c = sinteger32;" };

    for (const char *source : SOURCES) {
        MAREA_SemaFixture fixture(MAREA_externalAll());

        ASSERT_EQ(fixture.MAREA_sema(source), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

        EXPECT_EQ(fixture.MAREA_semaCodes(), EXPECTED) << "A reserved name must raise NAME_ALREADY_DECLARED: " << source;

        EXPECT_EQ(MAREA_firstMessage(fixture, ALREADY_DECLARED), "name is reserved: 'c'") << "The message must say the name is reserved: " << source;
    }
}

TEST(MAREASemaNamespaceAndNames, PlainDeclarationNamedCKeepsItsBehavior)
{
    GTEST_COUT << "Validating the reserved-name rule does not change a plain declaration or a plain import named c." << std::endl;

    MAREA_SemaFixture declaration;

    ASSERT_EQ(declaration.MAREA_sema("namespace main; structure c end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(declaration.MAREA_semaCodes().empty()) << "A plain declaration named c stays accepted.";

    MAREA_SemaFixture import(MAREA_externalAll());

    ASSERT_EQ(import.MAREA_sema("namespace main; import c.stdio;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(import.MAREA_semaCodes().empty()) << "An import of the C namespace stays accepted.";
}

TEST(MAREASemaNamespaceAndNames, MemberOfAMissingNamespaceIsRejected)
{
    GTEST_COUT << "Validating a member list of a namespace the resolver does not find is unresolved." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { f as g, h };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), (std::vector<MAREA_Uinteger32>{ UNRESOLVED, UNRESOLVED })) << "Each member must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaNamespaceAndNames, MemberMustBePublicInAFoundNamespace)
{
    GTEST_COUT << "Validating a member of a found namespace must be a public top-level symbol." << std::endl;

    MAREA_LibraryResolver library;

    const std::vector<MAREA_Uinteger32> EXPECTED{ UNRESOLVED };

    const char *const BAD[]{ "namespace main; import a.b { hidden };", "namespace main; import a.b { missing as other };", "namespace main; import a.b { hidden as other };" };

    for (const char *source : BAD) {
        MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

        ASSERT_EQ(fixture.MAREA_sema(source), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

        EXPECT_EQ(fixture.MAREA_semaCodes(), EXPECTED) << "A private or missing member must raise UNRESOLVED_IMPORT: " << source;

        EXPECT_EQ(MAREA_firstMessage(fixture, UNRESOLVED).rfind("unresolved import:", 0), 0U) << "The message must start with the unresolved import text.";
    }

    MAREA_SemaFixture good(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(good.MAREA_sema("namespace main; import a.b { c as run, MAX as LIMIT, Item as Thing };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(good.MAREA_semaCodes().empty()) << "Public members must raise nothing.";
}

TEST(MAREASemaNamespaceAndNames, EnumVariantMemberNeedsAnEnumWithThatVariant)
{
    GTEST_COUT << "Validating a two-part member names a variant of a public enum." << std::endl;

    MAREA_LibraryResolver library;

    MAREA_SemaFixture good(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(good.MAREA_sema("namespace main; import a.b { Color.red as Crimson, Color.green };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(good.MAREA_semaCodes().empty()) << "A variant of a public enum must raise nothing.";

    const char *const BAD[]{ "namespace main; import a.b { Color.blue };", "namespace main; import a.b { Item.red };", "namespace main; import a.b { c.red };", "namespace main; import a.b { Missing.red };", "namespace main; import a.b { Color.red.deep };" };

    for (const char *source : BAD) {
        MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

        ASSERT_EQ(fixture.MAREA_sema(source), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

        EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A bad variant path must raise UNRESOLVED_IMPORT: " << source;
    }

    MAREA_SemaFixture external(MAREA_externalAll());

    ASSERT_EQ(external.MAREA_sema("namespace main; import a.b { Color.red.deep };"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(external.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A path of three segments is never a member, even for an external namespace.";
}

TEST(MAREASemaNamespaceAndNames, ConstantAliasInAnExpressionIsDeclared)
{
    GTEST_COUT << "Validating a constant and a variant brought in under an alias are declared names in an expression." << std::endl;

    MAREA_SemaFixture fixture(MAREA_externalAll());

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b { MAX as LIMIT, Color.red as Crimson }; function f(): sinteger32 return LIMIT; end function g(): sinteger32 local n: sinteger32 = Crimson; return 0; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_semaCodes() };

    EXPECT_EQ(std::count(CODES.begin(), CODES.end(), UNDECLARED), 0) << "An alias must not raise UNDECLARED_NAME.";
}

TEST(MAREASemaNamespaceAndNames, BrokenMemberListIsTolerated)
{
    GTEST_COUT << "Validating a member list with a broken alias or no member raises no extra sema code." << std::endl;

    MAREA_SemaFixture brokenAlias(MAREA_externalAll());

    ASSERT_EQ(brokenAlias.MAREA_sema("namespace main; import a.b { f as };"), MAREA_SEMA_EXCEPT_SUCCESS) << "A broken alias must still check.";

    EXPECT_TRUE(brokenAlias.MAREA_semaCodes().empty()) << "A broken alias must raise no sema code.";

    MAREA_SemaFixture empty(MAREA_externalAll());

    ASSERT_EQ(empty.MAREA_sema("namespace main; import a.b { };"), MAREA_SEMA_EXCEPT_SUCCESS) << "An empty list must still check.";

    EXPECT_TRUE(empty.MAREA_semaCodes().empty()) << "An empty list must raise no sema code.";
}
