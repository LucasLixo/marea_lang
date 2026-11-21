// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

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
            MAREA_IGNORE(library.parser.MAREA_parse("namespace a.b; #[public] function c() end function hidden() end"));
        }
    };

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
