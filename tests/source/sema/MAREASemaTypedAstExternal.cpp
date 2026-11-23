// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <functional>
#include <string>
#include <vector>

namespace
{
    using MAREA_SignatureCallback = std::function<MAREA_SemaImportResult(const std::string &namespaceText, const std::string &memberText, const MAREA_SemaSignature **outSignature)>;

    // Both thunks share one userData pointer, as the resolver API gives only one per resolver.
    struct MAREA_ExternalCallbacks
    {
        MAREA_ResolveCallback resolve;
        MAREA_SignatureCallback signature;
    };

    void MAREA_CALL MAREA_externalResolveThunk(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot)
    {
        auto *callbacks{ static_cast<MAREA_ExternalCallbacks *>(userData) };

        *outResult = callbacks->resolve(std::string(namespaceText, namespaceLength), outRoot);
    }

    void MAREA_CALL MAREA_externalSignatureThunk(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature)
    {
        auto *callbacks{ static_cast<MAREA_ExternalCallbacks *>(userData) };

        *outSignature = nullptr;

        *outResult = callbacks->signature(std::string(namespaceText, namespaceLength), std::string(memberText, memberLength), outSignature);
    }

    // A typed tree of one checked file, whose resolver answers EXTERNAL for a namespace and may know the signature of some of its members.
    struct MAREA_ExternalFixture
    {
        MAREA_ParseFixture parser;
        MAREA_ExternalCallbacks callbacks;
        MAREA_SemaImportResolver *resolver{ nullptr };
        MAREA_SemaContext *context{ nullptr };
        const MAREA_SemaTypedNode *root{ nullptr };

        MAREA_ExternalFixture(MAREA_ResolveCallback resolve, MAREA_SignatureCallback signature)
            : callbacks{ std::move(resolve), std::move(signature) }
        {
            MAREA_IGNORE(MAREA_SemaImportResolverCreate(parser.lexer.arena, &MAREA_externalResolveThunk, &callbacks, &resolver));
            MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(resolver, &MAREA_externalSignatureThunk));
            MAREA_IGNORE(MAREA_SemaContextCreate(parser.lexer.arena, parser.lexer.sink, resolver, &context));
        }

        MAREA_ExternalFixture(const MAREA_ExternalFixture &) = delete;
        MAREA_ExternalFixture &operator=(const MAREA_ExternalFixture &) = delete;

        MAREA_SemaExcept MAREA_check(const std::string &source)
        {
            if (MAREA_LexContextSetFileName(parser.lexer.context, "main.marea") != MAREA_LEX_EXCEPT_SUCCESS) {
                return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
            }

            if (parser.MAREA_parse(source) != MAREA_PARSE_EXCEPT_SUCCESS) {
                return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
            }

            return MAREA_SemaCheckCompilationUnit(context, parser.root);
        }

        std::vector<MAREA_Uinteger32> MAREA_codes() const
        {
            std::vector<MAREA_Uinteger32> result;

            for (MAREA_Uinteger32 code : parser.MAREA_codes()) {
                if (code >= MAREA_UINTEGER_32(3000)) {
                    result.push_back(code);
                }
            }

            return result;
        }

        // Checks with zero errors, then builds the typed shadow tree.
        bool MAREA_build(const std::string &source)
        {
            if ((MAREA_check(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (!MAREA_codes().empty())) {
                return false;
            }

            return MAREA_SemaBuildTypedAst(context, parser.root, &root) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
        }
    };

    void MAREA_collect(const MAREA_SemaTypedNode *node, MAREA_AstKind kind, std::vector<const MAREA_SemaTypedNode *> &out)
    {
        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == kind) {
            out.push_back(node);
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_collect(child, kind, out);
            }
        }
    }

    const MAREA_SemaTypedNode *MAREA_nth(const MAREA_ExternalFixture &fixture, MAREA_AstKind kind, std::size_t nth)
    {
        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collect(fixture.root, kind, nodes);

        return (nth < nodes.size()) ? nodes[nth] : nullptr;
    }

    // The resolved type text of a node, or "<none>" when the accessor reports no type.
    std::string MAREA_typeOf(const MAREA_SemaTypedNode *node)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if ((node == nullptr) || (MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
            return "<none>";
        }

        return std::string(text, length);
    }

    // The signature of c.math.pow(float64, float64): float64, the one member every test fixture below knows.
    const MAREA_SemaSignatureParameter MAREA_POW_PARAMETERS[2] = {
        { "float64", 7 },
        { "float64", 7 },
    };

    const MAREA_SemaSignature MAREA_POW_SIGNATURE{ MAREA_POW_PARAMETERS, 2, "float64", 7 };

    MAREA_SignatureCallback MAREA_powOnly()
    {
        return [](const std::string &namespaceText, const std::string &memberText, const MAREA_SemaSignature **outSignature) {
            if ((namespaceText == "c.math") && (memberText == "pow")) {
                *outSignature = &MAREA_POW_SIGNATURE;

                return MAREA_SEMA_IMPORT_RESULT_FOUND;
            }

            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        };
    }

    MAREA_ResolveCallback MAREA_externalCNamespace()
    {
        return [](const std::string &text, const MAREA_AstNode **) { return (text.rfind("c.", 0) == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; };
    }
} // namespace

TEST(MAREASemaTypedAstExternal, ImportedNameOfAKnownExternalSignatureIsTypedWithItsResult)
{
    GTEST_COUT << "Validating a call of an imported name whose EXTERNAL signature the host knows is typed with its declared result." << std::endl;

    MAREA_ExternalFixture fixture(MAREA_externalCNamespace(), MAREA_powOnly());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport c.math.pow as power;\nfunction f(): float64 return power(2.0, 10.0); end\n")) << "The source must check with zero errors and build.";

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "float64") << "The call must be typed with the declared result of the signature.";
}

TEST(MAREASemaTypedAstExternal, MemberAccessFormOfAnExternalCallIsTypedTheSameWay)
{
    GTEST_COUT << "Validating a call written as a member access on an EXTERNAL namespace name is typed the same way as the bare imported name." << std::endl;

    MAREA_ExternalFixture fixture(MAREA_externalCNamespace(), MAREA_powOnly());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport c.math;\nfunction f(): float64 return math.pow(2.0, 10.0); end\n")) << "The source must check with zero errors and build.";

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "float64") << "The member-access call must be typed with the declared result of the signature.";
}

TEST(MAREASemaTypedAstExternal, WrongArgumentCountRaisesArgumentCountMismatch)
{
    GTEST_COUT << "Validating a call of a known EXTERNAL signature with the wrong argument count raises ARGUMENT_COUNT_MISMATCH (3010)." << std::endl;

    MAREA_ExternalFixture fixture(MAREA_externalCNamespace(), MAREA_powOnly());

    EXPECT_EQ(fixture.MAREA_check("namespace main;\nimport c.math.pow as power;\nfunction f(): float64 return power(1.0); end\n"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check itself must not fail.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_UINTEGER_32(3010) }) << "Too few arguments must raise ARGUMENT_COUNT_MISMATCH and nothing else.";
}

TEST(MAREASemaTypedAstExternal, WrongArgumentTypeRaisesTypeMismatch)
{
    GTEST_COUT << "Validating a call of a known EXTERNAL signature with an argument of the wrong type raises TYPE_MISMATCH (3011)." << std::endl;

    MAREA_ExternalFixture fixture(MAREA_externalCNamespace(), MAREA_powOnly());

    EXPECT_EQ(fixture.MAREA_check("namespace main;\nimport c.math.pow as power;\nfunction f(): float64 return power(\"two\", 10.0); end\n"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check itself must not fail.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_UINTEGER_32(3011) }) << "An argument of the wrong type must raise TYPE_MISMATCH and nothing else.";
}

TEST(MAREASemaTypedAstExternal, MemberTheHostDoesNotKnowStaysOpaque)
{
    GTEST_COUT << "Validating a member the signature function answers NOT_FOUND for stays exactly as opaque as with no signature function at all." << std::endl;

    MAREA_ExternalFixture fixture(MAREA_externalCNamespace(), MAREA_powOnly());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport c.stdio;\nfunction f(text: string): sinteger32 return stdio.puts(text); end\n")) << "The source must check with zero errors and build.";

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "<none>") << "A member the signature function does not know must stay untyped, with no diagnostic.";
}
