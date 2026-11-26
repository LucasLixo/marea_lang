// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    constexpr MAREA_Uinteger32 UNRESOLVED{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT };
    constexpr MAREA_Uinteger32 PRIVATE_SYMBOL{ MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_SYMBOL_ACCESS };
    constexpr MAREA_Uinteger32 PRIVATE_TYPE{ MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_TYPE_ACCESS };
    constexpr MAREA_Uinteger32 PRIVATE_FIELD{ MAREA_SEMA_DIAGNOSTIC_CODE_PRIVATE_FIELD_ACCESS };

    // INFO: the library of every test: a public, a private and an attribute-less function, a public structure with a public, a private and an attribute-less field, a private structure and a public constant.
    constexpr const char *LIBRARY{
        "namespace a.b;"
        "#[visibility = public] function shown() end "
        "#[visibility = private] function hidden() end "
        "function plain() end "
        "#[visibility = public] structure Box open: sinteger32; #[visibility = private] value: sinteger32; other: sinteger32; end "
        "#[visibility = private] structure Secret end "
        "#[visibility = public] constant LIMIT: sinteger32 = 1;"
        "#[visibility = public] structure Outer inner: Box; end "
        "implementation Box #[visibility = private] function conceal(self: Box) end function reveal(self: Box) end function create(): Self return Box{ open = 1, value = 2, other = 3 }; end end "
        "function make(): Box return Box{ open = 1, value = 2, other = 3 }; end "
        "type Renamed = Box;"
    };

    struct MAREA_VisibilityLibrary
    {
        MAREA_SemaFixture library;

        MAREA_VisibilityLibrary()
        {
            MAREA_IGNORE(library.parser.MAREA_parse(LIBRARY));
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

    std::vector<MAREA_Uinteger32> MAREA_codesOf(const std::string &source, MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_FOUND)
    {
        MAREA_VisibilityLibrary library;

        MAREA_SemaFixture fixture(MAREA_answer("a.b", result, (result == MAREA_SEMA_IMPORT_RESULT_FOUND) ? library.library.parser.root : nullptr));

        MAREA_IGNORE(fixture.MAREA_sema(source));

        return fixture.MAREA_semaCodes();
    }

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

    const std::vector<MAREA_Uinteger32> NONE{};
} // namespace

TEST(MAREASemaVisibility, PrivateFunctionOfAnotherNamespaceIsRejected)
{
    GTEST_COUT << "Validating a private function reached as namespace.name raises PRIVATE_SYMBOL_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() b.hidden(); end"), std::vector<MAREA_Uinteger32>{ PRIVATE_SYMBOL }) << "A private function must raise PRIVATE_SYMBOL_ACCESS.";
}

TEST(MAREASemaVisibility, PrivateSymbolMessageNamesTheSymbol)
{
    GTEST_COUT << "Validating the message names the symbol." << std::endl;

    MAREA_VisibilityLibrary library;

    MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b as b; function f() b.hidden(); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(MAREA_firstMessage(fixture, PRIVATE_SYMBOL), "symbol is private: 'hidden'") << "The message must name the symbol.";
}

TEST(MAREASemaVisibility, PublicAndAttributeLessMembersRaiseNothing)
{
    GTEST_COUT << "Validating a public symbol, an attribute-less symbol and a public constant raise nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() b.shown(); b.plain(); local x: sinteger32 = b.LIMIT; end"), NONE) << "A public member must raise nothing.";
}

TEST(MAREASemaVisibility, ImportOfAttributeLessSymbolResolves)
{
    GTEST_COUT << "Validating an import of an attribute-less symbol is public by default." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b.plain as plain;"), NONE) << "An attribute-less symbol must be importable.";
}

TEST(MAREASemaVisibility, ImportOfPrivateSymbolIsStillRejected)
{
    GTEST_COUT << "Validating an import of an explicitly private symbol is unresolved." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b.hidden as hidden;"), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A private symbol must raise UNRESOLVED_IMPORT.";
}

TEST(MAREASemaVisibility, PrivateTypeInParameterIsRejected)
{
    GTEST_COUT << "Validating a private type written as a parameter type raises PRIVATE_TYPE_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(s: b.Secret) end"), std::vector<MAREA_Uinteger32>{ PRIVATE_TYPE }) << "A private type must raise PRIVATE_TYPE_ACCESS.";
}

TEST(MAREASemaVisibility, PrivateTypeInLocalAndLiteralIsRejectedTwice)
{
    GTEST_COUT << "Validating a private type written as a local type and as a literal type raises PRIVATE_TYPE_ACCESS at each." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() local s: b.Secret = b.Secret{}; end"), (std::vector<MAREA_Uinteger32>{ PRIVATE_TYPE, PRIVATE_TYPE })) << "Each written private type must raise PRIVATE_TYPE_ACCESS.";
}

TEST(MAREASemaVisibility, PrivateTypeMessageNamesTheType)
{
    GTEST_COUT << "Validating the message names the type." << std::endl;

    MAREA_VisibilityLibrary library;

    MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b as b; function f(s: b.Secret) end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(MAREA_firstMessage(fixture, PRIVATE_TYPE), "type is private: 'Secret'") << "The message must name the type.";
}

TEST(MAREASemaVisibility, PublicTypeRaisesNothing)
{
    GTEST_COUT << "Validating a public structure written as a type raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(x: b.Box) end"), NONE) << "A public type must raise nothing.";
}

TEST(MAREASemaVisibility, AliasAndPlainFormAreBothChecked)
{
    GTEST_COUT << "Validating a namespace alias is checked as the plain form is." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as x; function f() x.hidden(); end"), std::vector<MAREA_Uinteger32>{ PRIVATE_SYMBOL }) << "An alias must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as x; function f(s: x.Secret) end"), std::vector<MAREA_Uinteger32>{ PRIVATE_TYPE }) << "An alias must be checked for a type.";
}

TEST(MAREASemaVisibility, ExternalNamespaceIsNeverChecked)
{
    GTEST_COUT << "Validating an EXTERNAL namespace raises nothing for any member." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(s: b.Secret) b.hidden(); end", MAREA_SEMA_IMPORT_RESULT_EXTERNAL), NONE) << "An EXTERNAL namespace must not be checked.";
}

TEST(MAREASemaVisibility, NotFoundNamespaceRaisesOnlyTheImport)
{
    GTEST_COUT << "Validating a NOT_FOUND namespace raises only UNRESOLVED_IMPORT." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() b.hidden(); end", MAREA_SEMA_IMPORT_RESULT_NOT_FOUND), std::vector<MAREA_Uinteger32>{ UNRESOLVED }) << "A NOT_FOUND namespace must raise only UNRESOLVED_IMPORT.";
}

TEST(MAREASemaVisibility, PrivateSymbolInItsOwnFileRaisesNothing)
{
    GTEST_COUT << "Validating a private symbol used inside its own namespace raises nothing." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; #[visibility = private] function hidden() end function f() hidden(); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "A private symbol of the own file must raise nothing.";
}

TEST(MAREASemaVisibility, MemberTheNamespaceDoesNotDeclareRaisesNothing)
{
    GTEST_COUT << "Validating a member that the tree does not declare is not this rule." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() b.missing(); end"), NONE) << "A member the tree does not declare must raise nothing.";
}

TEST(MAREASemaVisibility, LocalHidesTheNamespaceName)
{
    GTEST_COUT << "Validating a local that hides the namespace name is not a namespace access." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(b: sinteger32) b.hidden(); end"), NONE) << "A local must hide the namespace name.";
}

TEST(MAREASemaVisibility, PrivateFieldReadOfAnotherNamespaceIsRejected)
{
    GTEST_COUT << "Validating a read of a private field of a structure of another namespace raises PRIVATE_FIELD_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(box: b.Box): sinteger32 return box.value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A private field must raise PRIVATE_FIELD_ACCESS.";
}

TEST(MAREASemaVisibility, PrivateFieldMessageNamesTheField)
{
    GTEST_COUT << "Validating the message names the field." << std::endl;

    MAREA_VisibilityLibrary library;

    MAREA_SemaFixture fixture(MAREA_answer("a.b", MAREA_SEMA_IMPORT_RESULT_FOUND, library.library.parser.root));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import a.b as b; function f(box: b.Box): sinteger32 return box.value; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(MAREA_firstMessage(fixture, PRIVATE_FIELD), "field is private: 'value'") << "The message must name the field.";
}

TEST(MAREASemaVisibility, PublicAndAttributeLessFieldsRaiseNothing)
{
    GTEST_COUT << "Validating a public field and an attribute-less field raise nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(box: b.Box): sinteger32 return box.open + box.other; end"), NONE) << "A public field must raise nothing.";
}

TEST(MAREASemaVisibility, PrivateFieldWriteIsRejected)
{
    GTEST_COUT << "Validating a write of a private field raises PRIVATE_FIELD_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(#[semantics = reference_mutable] box: b.Box) box.value = 1; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A write of a private field must raise PRIVATE_FIELD_ACCESS.";
}

TEST(MAREASemaVisibility, PrivateFieldInLiteralIsRejected)
{
    GTEST_COUT << "Validating a private field set in a struct literal raises PRIVATE_FIELD_ACCESS, a public one raises nothing." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() local box: b.Box = b.Box{ value = 1 }; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A private field in a literal must raise PRIVATE_FIELD_ACCESS.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() local box: b.Box = b.Box{ open = 1 }; end"), NONE) << "A public field in a literal must raise nothing.";
}

TEST(MAREASemaVisibility, PrivateFieldOfTheSameFileRaisesNothing)
{
    GTEST_COUT << "Validating a private field of a structure of the same file is never checked." << std::endl;

    MAREA_SemaFixture fixture;

    ASSERT_EQ(fixture.MAREA_sema("namespace main; structure Box #[visibility = private] value: sinteger32; end function f(box: Box): sinteger32 return box.value; end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "A field of the own file must raise nothing.";
}

TEST(MAREASemaVisibility, PrivateStructureRaisesTheTypeCodeOnly)
{
    GTEST_COUT << "Validating a private structure raises PRIVATE_TYPE_ACCESS and no PRIVATE_FIELD_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(s: b.Secret) end"), std::vector<MAREA_Uinteger32>{ PRIVATE_TYPE }) << "A private structure must raise only PRIVATE_TYPE_ACCESS.";
}

TEST(MAREASemaVisibility, PrivateFieldOfACallResultIsRejected)
{
    GTEST_COUT << "Validating a private field read on the result of a call of another namespace raises PRIVATE_FIELD_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() local x: sinteger32 = b.make().value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A call result must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function g(): b.Box return b.make(); end function f() local x: sinteger32 = g().value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A result of a function of this file must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() local x: sinteger32 = b.Box.create().value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A Self result of a static function must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b.make as build; function f() local x: sinteger32 = build().value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A result of an imported function must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() local x: sinteger32 = b.make().open; end"), NONE) << "A public field of a call result must raise nothing.";
}

TEST(MAREASemaVisibility, PrivateFieldOfAChainAndOfAnElementIsRejected)
{
    GTEST_COUT << "Validating a private field at the end of a chain or on an element raises PRIVATE_FIELD_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(o: b.Outer) local x: sinteger32 = o.inner.value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A chain through a structure of another namespace must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; structure Holder box: b.Box; end function f(h: Holder) local x: sinteger32 = h.box.value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A chain through a structure of this file must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; require demo.array; function f(v: demo.array<b.Box, 2>) local x: sinteger32 = v[0].value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "An element must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; require demo.array; function f(o: b.Outer, v: demo.array<b.Box, 2>) local x: sinteger32 = o.inner.open + v[0].other; end"), NONE) << "A public field must raise nothing.";
}

TEST(MAREASemaVisibility, PrivateMethodIsRejected)
{
    GTEST_COUT << "Validating a private function of an implementation block of another namespace raises PRIVATE_SYMBOL_ACCESS." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f() b.make().conceal(); end"), std::vector<MAREA_Uinteger32>{ PRIVATE_SYMBOL }) << "A private method of a call result must be rejected.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(x: b.Box) x.conceal(); end"), std::vector<MAREA_Uinteger32>{ PRIVATE_SYMBOL }) << "A private method of a local must be rejected.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(x: b.Box) x.reveal(); b.make().reveal(); end"), NONE) << "A public method must raise nothing.";
}

TEST(MAREASemaVisibility, TypeReachedByAFullPathOrAnImportedSymbolIsChecked)
{
    GTEST_COUT << "Validating a type or a field reached by a full path or by an imported symbol is checked." << std::endl;

    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b; function f(s: a.b.Secret) end"), std::vector<MAREA_Uinteger32>{ PRIVATE_TYPE }) << "A full path to a private type must be rejected.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b; function f(x: a.b.Box) local y: sinteger32 = x.value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A field of a full path type must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b.Box as Crate; function f(x: Crate) local y: sinteger32 = x.value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A field of an imported type must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b as b; function f(x: b.Renamed) local y: sinteger32 = x.value; end"), std::vector<MAREA_Uinteger32>{ PRIVATE_FIELD }) << "A field of an aliased type of another namespace must be checked.";
    EXPECT_EQ(MAREA_codesOf("namespace main; import a.b; function f(x: a.b.Box) local y: sinteger32 = x.open; end"), NONE) << "A public field of a full path type must raise nothing.";
}
