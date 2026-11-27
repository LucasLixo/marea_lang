// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    constexpr MAREA_Uinteger32 UNRESOLVED_IMPORT{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT };
    constexpr MAREA_Uinteger32 UNDECLARED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME };
    constexpr MAREA_Uinteger32 PRIVATE_SYMBOL{ MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_SYMBOL_ACCESS };
    constexpr MAREA_Uinteger32 UNIMPORTED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNIMPORTED_NAMESPACE };
    constexpr MAREA_Uinteger32 UNRESOLVED_REQUIRE{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE };
    constexpr MAREA_Uinteger32 AMBIGUOUS{ MAREA_SEMA_DIAGNOSTIC_CODE_AMBIGUOUS_NAMESPACE };
    constexpr MAREA_Uinteger32 RESERVED_ROOT{ MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_NAMESPACE_ROOT };

    // The registry of a host: the roots c, marea and demo exist, with any namespace below them.
    MAREA_SemaImportResult MAREA_registered(const std::string &text)
    {
        const auto BELOW{ [&text](const std::string &root) { return (text == root) || (text.rfind(root + ".", 0) == 0); } };

        return (BELOW("c") || BELOW("marea") || BELOW("demo")) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    }

    MAREA_ResolveCallback MAREA_registry()
    {
        return [](const std::string &text, const MAREA_AstNode **) { return MAREA_registered(text); };
    }

    // The public and the private function of one project namespace.
    constexpr const char *TOOLS{ "namespace util.tools; #[visibility = public] function twice(value: sinteger32): sinteger32 return value * 2; end #[visibility = private] function hidden() end" };

    struct MAREA_ProjectLibrary
    {
        MAREA_SemaFixture library;

        MAREA_ProjectLibrary()
        {
            MAREA_IGNORE(library.parser.MAREA_parse(TOOLS));
        }
    };

    std::vector<MAREA_Uinteger32> MAREA_codesOf(const std::string &source)
    {
        MAREA_SemaFixture fixture(MAREA_registry());

        MAREA_IGNORE(fixture.MAREA_sema(source));

        return fixture.MAREA_semaCodes();
    }

    std::vector<MAREA_Uinteger32> MAREA_projectCodesOf(const std::string &source)
    {
        MAREA_ProjectLibrary library;

        MAREA_SemaFixture fixture([&library](const std::string &text, const MAREA_AstNode **outRoot) {
            if (text == "util.tools") {
                *outRoot = library.library.parser.root;

                return MAREA_SEMA_IMPORT_RESULT_FOUND;
            }

            return MAREA_registered(text);
        });

        MAREA_IGNORE(fixture.MAREA_sema(source));

        return fixture.MAREA_semaCodes();
    }

    // A context with two resolvers: the project files answer an import, the registry answers a require.
    struct MAREA_TwoResolvers
    {
        MAREA_ProjectLibrary library;
        MAREA_SemaFixture fixture;
        MAREA_ResolveCallback registry{ MAREA_registry() };
        MAREA_SemaImportResolver *registryResolver{ nullptr };

        MAREA_TwoResolvers()
            : fixture([this](const std::string &text, const MAREA_AstNode **outRoot) {
                  if (text == "util.tools") {
                      *outRoot = library.library.parser.root;

                      return MAREA_SEMA_IMPORT_RESULT_FOUND;
                  }

                  return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
              })
        {
            MAREA_IGNORE(MAREA_SemaImportResolverCreate(fixture.parser.lexer.arena, &MAREA_resolveThunk, &registry, &registryResolver));
            MAREA_IGNORE(MAREA_SemaContextSetRequireResolver(fixture.context, registryResolver));
        }

        MAREA_TwoResolvers(const MAREA_TwoResolvers &) = delete;
        MAREA_TwoResolvers &operator=(const MAREA_TwoResolvers &) = delete;
    };
} // namespace

TEST(MAREASemaFullPath, TheRequireResolverIsNullUntilOneIsSet)
{
    GTEST_COUT << "Validating a context has no require resolver until one is set, and that a null context is rejected." << std::endl;

    MAREA_SemaFixture fixture;

    EXPECT_EQ(MAREA_SemaContextGetRequireResolver(fixture.context), nullptr) << "A new context must have no require resolver.";

    EXPECT_EQ(MAREA_SemaContextGetRequireResolver(nullptr), nullptr) << "A null context must give null.";

    EXPECT_EQ(MAREA_SemaContextSetRequireResolver(nullptr, fixture.resolver), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context must be rejected.";
}

TEST(MAREASemaFullPath, TheRequireResolverIsKeptAndCanBeCleared)
{
    GTEST_COUT << "Validating the require resolver is kept as set and clears with null." << std::endl;

    MAREA_TwoResolvers two;

    EXPECT_EQ(MAREA_SemaContextGetRequireResolver(two.fixture.context), two.registryResolver) << "The getter must give the resolver that was set.";

    ASSERT_EQ(MAREA_SemaContextSetRequireResolver(two.fixture.context, nullptr), MAREA_SEMA_EXCEPT_SUCCESS) << "A null registry must clear the resolver.";

    EXPECT_EQ(MAREA_SemaContextGetRequireResolver(two.fixture.context), nullptr) << "A cleared context must have no require resolver.";
}

TEST(MAREASemaFullPath, FullPathCallAfterARequireIsAccepted)
{
    GTEST_COUT << "Validating a call written with its full path is legal after a require of its namespace." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; require c.stdio; function f() c.stdio.puts(\"x\"); end").empty()) << "A full path under a require must raise nothing.";
}

TEST(MAREASemaFullPath, FullPathCallWithNoRequireIsUnimported)
{
    GTEST_COUT << "Validating a call of another namespace with no import or require raises UNIMPORTED_NAMESPACE." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; function f() c.stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A namespace used with no require must raise UNIMPORTED_NAMESPACE.";
}

TEST(MAREASemaFullPath, ARequireOfAnotherNamespaceDoesNotCover)
{
    GTEST_COUT << "Validating a require of one namespace does not cover another." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; require c.math; function f() c.stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A require of c.math must not cover c.stdio.";
}

TEST(MAREASemaFullPath, TheLastSegmentAloneNeverResolves)
{
    GTEST_COUT << "Validating the last segment of a required path binds no name." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; require c.stdio; function f() stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "The last segment alone must be an undeclared name.";
}

TEST(MAREASemaFullPath, ACutPathIsAnUndeclaredName)
{
    GTEST_COUT << "Validating a path cut after its root raises UNDECLARED_NAME." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; require marea.math; function f() math.sqrt(2.0); end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "A path with its root cut must be an undeclared name.";
}

TEST(MAREASemaFullPath, AnAliasGivesTheShortName)
{
    GTEST_COUT << "Validating the short name works through an alias, and through nothing else." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; require c.stdio as io; function f() io.puts(\"x\"); end").empty()) << "A short name through an alias must raise nothing.";

    EXPECT_EQ(MAREA_codesOf("namespace main; require c.stdio as io; function f() stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "The last segment must not resolve beside an alias.";
}

TEST(MAREASemaFullPath, AnAliasReachesTheNamespacesBelowIt)
{
    GTEST_COUT << "Validating an alias of a namespace reaches the members of the namespaces below it." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; require c as native; function f() native.stdio.puts(\"x\"); end").empty()) << "An alias must stand for its whole path.";
}

TEST(MAREASemaFullPath, AWildcardCoversEveryDepthAndBindsNoName)
{
    GTEST_COUT << "Validating a wildcard covers every full path below it and binds no name." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; require c.*; function f() c.stdio.puts(\"x\"); c.math.pow(2.0, 3.0); end").empty()) << "A wildcard must cover the paths below it at any depth.";

    EXPECT_EQ(MAREA_codesOf("namespace main; require c.*; function f() stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "A wildcard must bind no name.";

    EXPECT_EQ(MAREA_codesOf("namespace main; require marea.*; function f() c.stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A wildcard must not cover another root.";
}

TEST(MAREASemaFullPath, ANarrowWildcardCoversOnlyItsOwnBranch)
{
    GTEST_COUT << "Validating a wildcard of a namespace covers that branch and no other." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; require c.math.*; function f() c.stdio.puts(\"x\"); end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A wildcard of c.math must not cover c.stdio.";
}

TEST(MAREASemaFullPath, AWildcardOfAnUnknownNamespaceIsUnresolved)
{
    GTEST_COUT << "Validating a wildcard of a namespace nobody knows is unresolved, by its keyword." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; require nowhere.*;"), std::vector<MAREA_Uinteger32>{ UNRESOLVED_REQUIRE }) << "A require wildcard of an unknown namespace must raise UNRESOLVED_REQUIRE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; import nowhere.*;"), std::vector<MAREA_Uinteger32>{ UNRESOLVED_IMPORT }) << "An import wildcard of an unknown namespace must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaFullPath, ARequireOfAnUnregisteredNameIsUnresolved)
{
    GTEST_COUT << "Validating a require of a name nothing registered raises UNRESOLVED_REQUIRE." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; require nowhere.library;"), std::vector<MAREA_Uinteger32>{ UNRESOLVED_REQUIRE }) << "An unregistered require must raise UNRESOLVED_REQUIRE.";

    EXPECT_EQ(MAREA_codesOf("namespace main; require nowhere.library as library;"), std::vector<MAREA_Uinteger32>{ UNRESOLVED_REQUIRE }) << "An unregistered require with an alias must raise UNRESOLVED_REQUIRE.";
}

TEST(MAREASemaFullPath, ARequireOfAHostRootThatIsNotCOrMareaIsAccepted)
{
    GTEST_COUT << "Validating a require of a root the host registered, other than c and marea, is accepted." << std::endl;

    EXPECT_TRUE(MAREA_codesOf("namespace main; require demo.gpu.texture; function f() demo.gpu.texture.create(); end").empty()) << "A host root must work like c and marea.";
}

TEST(MAREASemaFullPath, ATypeOfAnotherNamespaceNeedsAnImport)
{
    GTEST_COUT << "Validating a type written with the path of another namespace needs an import or a require." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; function f(values: marea.array<sinteger32>) end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A type of another namespace with no require must raise UNIMPORTED_NAMESPACE.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; require marea.array; function f(values: marea.array<sinteger32>) end").empty()) << "A required type must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; require marea.*; function f(values: marea.array<sinteger32>) end").empty()) << "A wildcard must cover a type.";

    EXPECT_EQ(MAREA_codesOf("namespace main; require marea.vector; function f(values: marea.array<sinteger32>) end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A require of another library must not cover the type.";
}

TEST(MAREASemaFullPath, AnImportAndARequireOfTheSamePathAreAmbiguous)
{
    GTEST_COUT << "Validating one full path covered by an import and by a require raises AMBIGUOUS_NAMESPACE at the second." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import c.stdio; require c.*;"), std::vector<MAREA_Uinteger32>{ AMBIGUOUS }) << "A path under both keywords must be ambiguous.";

    EXPECT_EQ(MAREA_codesOf("namespace main; require c.stdio; import c.stdio;"), std::vector<MAREA_Uinteger32>{ AMBIGUOUS }) << "The same path under both keywords must be ambiguous, in any order.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; require c.stdio; import marea.math;").empty()) << "Two keywords on two separate paths must not be ambiguous.";

    EXPECT_TRUE(MAREA_codesOf("namespace main; require c.stdio; require c.*;").empty()) << "Two requires of one path must not be ambiguous.";
}

TEST(MAREASemaFullPath, AProjectFileIsReachedByItsFullPath)
{
    GTEST_COUT << "Validating a function of a project file is called by its full path after an import." << std::endl;

    EXPECT_TRUE(MAREA_projectCodesOf("namespace main; import util.tools; function f(): sinteger32 return util.tools.twice(2); end").empty()) << "A full path under an import must raise nothing.";

    EXPECT_TRUE(MAREA_projectCodesOf("namespace main; import util.tools.*; function f(): sinteger32 return util.tools.twice(2); end").empty()) << "A wildcard must cover the members of the namespace.";
}

TEST(MAREASemaFullPath, AProjectFileNeedsItsImport)
{
    GTEST_COUT << "Validating a project file with no import raises UNIMPORTED_NAMESPACE, and that its last segment alone never resolves." << std::endl;

    EXPECT_EQ(MAREA_projectCodesOf("namespace main; function f(): sinteger32 return util.tools.twice(2); end"), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A project namespace used with no import must raise UNIMPORTED_NAMESPACE.";

    EXPECT_EQ(MAREA_projectCodesOf("namespace main; import util.tools; function f(): sinteger32 return tools.twice(2); end"), std::vector<MAREA_Uinteger32>{ UNDECLARED }) << "The last segment alone must not resolve.";
}

TEST(MAREASemaFullPath, AProjectAliasGivesTheShortName)
{
    GTEST_COUT << "Validating a project namespace reached by an alias." << std::endl;

    EXPECT_TRUE(MAREA_projectCodesOf("namespace main; import util.tools as tools; function f(): sinteger32 return tools.twice(2); end").empty()) << "A short name through an alias must raise nothing.";
}

TEST(MAREASemaFullPath, APrivateMemberStaysPrivateThroughItsFullPath)
{
    GTEST_COUT << "Validating a private member is rejected when it is reached by its full path." << std::endl;

    EXPECT_EQ(MAREA_projectCodesOf("namespace main; import util.tools; function f() util.tools.hidden(); end"), std::vector<MAREA_Uinteger32>{ PRIVATE_SYMBOL }) << "A private member must raise PRIVATE_SYMBOL_ACCESS through its full path.";
}

TEST(MAREASemaFullPath, TheTwoResolversAnswerTheirOwnKeyword)
{
    GTEST_COUT << "Validating an import is answered by the project resolver and a require by the registry." << std::endl;

    MAREA_TwoResolvers two;

    ASSERT_EQ(two.fixture.MAREA_sema("namespace main; import util.tools; require c.stdio; function f(): sinteger32 return util.tools.twice(2); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(two.fixture.MAREA_semaCodes().empty()) << "Each keyword must reach its own resolver.";
}

TEST(MAREASemaFullPath, AnImportNeverReachesARegisteredName)
{
    GTEST_COUT << "Validating an import of a name that only the registry knows is unresolved." << std::endl;

    MAREA_TwoResolvers two;

    ASSERT_EQ(two.fixture.MAREA_sema("namespace main; import c.stdio;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(two.fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED_IMPORT }) << "An import of a registered name must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaFullPath, ARequireNeverReachesAProjectFile)
{
    GTEST_COUT << "Validating a require of a name that only the project resolver knows is unresolved." << std::endl;

    MAREA_TwoResolvers two;

    ASSERT_EQ(two.fixture.MAREA_sema("namespace main; require util.tools;"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(two.fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNRESOLVED_REQUIRE }) << "A require of a project file must raise UNRESOLVED_REQUIRE.";
}

TEST(MAREASemaFullPath, ANameInATypePositionNeedsItsCoverToo)
{
    GTEST_COUT << "Validating a registered type needs the require when the registry is separate." << std::endl;

    MAREA_TwoResolvers two;

    ASSERT_EQ(two.fixture.MAREA_sema("namespace main; function f(values: marea.array<sinteger32>) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(two.fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ UNIMPORTED }) << "A registered type with no require must raise UNIMPORTED_NAMESPACE.";
}

TEST(MAREASemaFullPath, AProjectFileCannotDeclareAReservedRoot)
{
    GTEST_COUT << "Validating a project file cannot declare a namespace under c or marea." << std::endl;

    MAREA_SemaFixture underC;

    ASSERT_EQ(underC.MAREA_sema("namespace c.tools;", "c/tools.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(underC.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ RESERVED_ROOT }) << "A namespace under c must raise RESERVED_NAMESPACE_ROOT.";

    MAREA_SemaFixture underMarea;

    ASSERT_EQ(underMarea.MAREA_sema("namespace marea.tools;", "marea/tools.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(underMarea.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ RESERVED_ROOT }) << "A namespace under marea must raise RESERVED_NAMESPACE_ROOT.";

    MAREA_SemaFixture user;

    ASSERT_EQ(user.MAREA_sema("namespace tools;", "tools.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(user.MAREA_semaCodes().empty()) << "A root of the project must raise nothing.";
}

TEST(MAREASemaFullPath, AProjectFileRaisesReservedNamespaceRootEvenWhenTheRegistryKnowsTheName)
{
    GTEST_COUT << "Validating a project file still raises RESERVED_NAMESPACE_ROOT under marea even when the host registered that name: the 2026-11-25 decision update makes the rule unconditional (sema-no-library-descriptors-roles), replacing the earlier conditional reading this test used to check." << std::endl;

    MAREA_SemaFixture fixture(MAREA_registry());

    ASSERT_EQ(fixture.MAREA_sema("namespace marea.math;", "marea/math.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ RESERVED_ROOT }) << "A project file under marea must raise RESERVED_NAMESPACE_ROOT regardless of what the registry knows.";
}

TEST(MAREASemaFullPath, AShippedLibrarySourceMayDeclareAReservedRoot)
{
    GTEST_COUT << "Validating the shipped library source input exempts the namespace declaration of the unit from the reserved root rule." << std::endl;

    MAREA_SemaFixture underMarea;

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(underMarea.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(underMarea.MAREA_sema("namespace marea.whatever;", "marea/whatever.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(underMarea.MAREA_semaCodes().empty()) << "A shipped file may declare a namespace under marea.";

    MAREA_SemaFixture underC;

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(underC.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(underC.MAREA_sema("namespace c.tools;", "c/tools.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(underC.MAREA_semaCodes().empty()) << "A shipped file may declare a namespace under c.";
}

TEST(MAREASemaFullPath, TheShippedLibrarySourceInputIsOffByDefaultAndCanBeTurnedOff)
{
    GTEST_COUT << "Validating a context with the input off rejects a reserved root as before." << std::endl;

    MAREA_SemaFixture untouched;

    ASSERT_EQ(untouched.MAREA_sema("namespace marea.whatever;", "marea/whatever.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(untouched.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ RESERVED_ROOT }) << "The default is a file of the project.";

    MAREA_SemaFixture off;

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(off.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(off.context, MAREA_BOOLEAN_FALSE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be cleared.";

    ASSERT_EQ(off.MAREA_sema("namespace marea.whatever;", "marea/whatever.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(off.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ RESERVED_ROOT }) << "An input that is cleared exempts nothing.";

    EXPECT_EQ(MAREA_SemaContextSetShippedLibrarySource(nullptr, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER) << "A null context is an error.";
}

TEST(MAREASemaFullPath, TheShippedLibrarySourceInputChangesNoOtherRule)
{
    GTEST_COUT << "Validating the shipped library source input leaves the reserved alias rule in place." << std::endl;

    MAREA_SemaFixture fixture(MAREA_registry());

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(fixture.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(fixture.MAREA_sema("namespace marea.whatever; require demo.tools as marea;", "marea/whatever.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED }) << "An alias named marea stays refused.";
}
