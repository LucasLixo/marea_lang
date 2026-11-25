// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using MAREA_SignatureCallback = std::function<MAREA_SemaImportResult(const std::string &namespaceText, const std::string &memberText, const MAREA_SemaSignature **outSignature)>;

    struct MAREA_LibraryCallbacks
    {
        MAREA_ResolveCallback resolve;
        MAREA_SignatureCallback signature;
    };

    void MAREA_CALL MAREA_libraryResolveThunk(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot)
    {
        auto *callbacks{ static_cast<MAREA_LibraryCallbacks *>(userData) };

        *outResult = callbacks->resolve(std::string(namespaceText, namespaceLength), outRoot);
    }

    void MAREA_CALL MAREA_librarySignatureThunk(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature)
    {
        auto *callbacks{ static_cast<MAREA_LibraryCallbacks *>(userData) };

        *outSignature = nullptr;

        *outResult = callbacks->signature(std::string(namespaceText, namespaceLength), std::string(memberText, memberLength), outSignature);
    }

    // A typed tree of one checked file whose test resolver answers the signatures of library members, or none at all.
    struct MAREA_LibraryFixture
    {
        MAREA_ParseFixture parser;
        MAREA_LibraryCallbacks callbacks;
        MAREA_SemaImportResolver *resolver{ nullptr };
        MAREA_SemaContext *context{ nullptr };
        const MAREA_SemaTypedNode *root{ nullptr };

        MAREA_LibraryFixture(MAREA_ResolveCallback resolve, MAREA_SignatureCallback signature)
            : callbacks{ std::move(resolve), std::move(signature) }
        {
            MAREA_IGNORE(MAREA_SemaImportResolverCreate(parser.lexer.arena, &MAREA_libraryResolveThunk, &callbacks, &resolver));

            if (callbacks.signature) {
                MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(resolver, &MAREA_librarySignatureThunk));
            }

            MAREA_IGNORE(MAREA_SemaContextCreate(parser.lexer.arena, parser.lexer.sink, resolver, &context));
        }

        MAREA_LibraryFixture(const MAREA_LibraryFixture &) = delete;
        MAREA_LibraryFixture &operator=(const MAREA_LibraryFixture &) = delete;

        bool MAREA_build(const std::string &source)
        {
            if ((MAREA_LexContextSetFileName(parser.lexer.context, "main.marea") != MAREA_LEX_EXCEPT_SUCCESS) || (parser.MAREA_parse(source) != MAREA_PARSE_EXCEPT_SUCCESS)) {
                return false;
            }

            if (MAREA_SemaCheckCompilationUnit(context, parser.root) != MAREA_SEMA_EXCEPT_SUCCESS) {
                return false;
            }

            for (MAREA_Uinteger32 code : parser.MAREA_codes()) {
                if (code >= MAREA_UINTEGER_32(3000)) {
                    return false;
                }
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

    const MAREA_SemaTypedNode *MAREA_nth(const MAREA_LibraryFixture &fixture, MAREA_AstKind kind, std::size_t nth)
    {
        std::vector<const MAREA_SemaTypedNode *> nodes;

        MAREA_collect(fixture.root, kind, nodes);

        return (nth < nodes.size()) ? nodes[nth] : nullptr;
    }

    std::string MAREA_typeOf(const MAREA_SemaTypedNode *node)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if ((node == nullptr) || (MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
            return "<none>";
        }

        return std::string(text, length);
    }

    const MAREA_SemaSignatureParameter MAREA_FILLED_PARAMETERS[1] = { { "float32", 7 } };
    const MAREA_SemaSignatureParameter
MAREA_APPEND_PARAMETERS
[1] = { { "Item", 4 } };

const MAREA_SemaSignature MAREA_FILLED_SIGNATURE{ MAREA_FILLED_PARAMETERS, 1, "Self", 4 };
const MAREA_SemaSignature
MAREA_APPEND_SIGNATURE
{ MAREA_APPEND_PARAMETERS, 1, "void", 4 };
const MAREA_SemaSignature MAREA_RESULT_SIGNATURE{ nullptr, 0, "Item", 4 };
const MAREA_SemaSignature MAREA_SUM_SIGNATURE{ nullptr, 0, "Item", 4 };
const MAREA_SemaSignature MAREA_COUNT_SIGNATURE{ nullptr, 0, "Count", 5 };

// The fixture signatures, keyed by the canonical "c.<library>" namespace and the member name.
MAREA_SignatureCallback MAREA_librarySignatures()
{
    return [](const std::string &namespaceText, const std::string &memberText, const MAREA_SemaSignature **outSignature) {
        static const std::map<std::pair<std::string, std::string>, const MAREA_SemaSignature *> SIGNATURES{
            { { "c.array", "filled" }, &MAREA_FILLED_SIGNATURE },
            { { "c.array", "append" }, &MAREA_APPEND_SIGNATURE },
            { { "c.array", "sum" }, &MAREA_SUM_SIGNATURE },
            { { "c.array", "count" }, &MAREA_COUNT_SIGNATURE },
            { { "c.task", "result" }, &MAREA_RESULT_SIGNATURE },
        };

        const auto FOUND{ SIGNATURES.find({ namespaceText, memberText }) };

        if (FOUND == SIGNATURES.end()) {
            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        }

        *outSignature = FOUND->second;

        return MAREA_SEMA_IMPORT_RESULT_FOUND;
    };
}

MAREA_ResolveCallback MAREA_externalLibraries()
{
    return [](const std::string &text, const MAREA_AstNode **) { return ((text.rfind("c.", 0) == 0) || (text.rfind("collections.", 0) == 0) || (text.rfind("concurrency.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; };
}
} // namespace

TEST(MAREASemaTypedAstLibraries, StaticCallOnAFixedArrayTypeGivesTheReceiverType)
{
    GTEST_COUT << "Validating array<float32, 16>.filled(0.0) is typed with the receiver type through a fixture signature." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport collections.array as array;\nfunction f(): sinteger32\n    local fixed: array<float32, 16> = array<float32, 16>.filled(0.0);\n    return 0;\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "array<float32,16>") << "A result spelled Self must give the receiver type with its type arguments.";
}

TEST(MAREASemaTypedAstLibraries, StaticCallUnderTheReservedNamespaceGivesTheReceiverType)
{
    GTEST_COUT << "Validating c.array<sinteger32>.filled(8, 0) is typed through the canonical c.array key." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main; require c.array;\nimport collections.array as array;\nfunction f(): sinteger32\n    local scratch: c.array<sinteger32> = c.array<sinteger32>.filled(8, 0);\n    return 0;\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "c.array<sinteger32>");
}

TEST(MAREASemaTypedAstLibraries, InstanceCallOfATypeParameterNameGivesTheTypeArgument)
{
    GTEST_COUT << "Validating a result spelled with the library's own generic parameter gives the receiver's type argument." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport collections.array as array;\nfunction f(values: array<float32, 4>): float32\n    return values.sum();\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "float32") << "Item of array<float32, 4> is float32.";
}

TEST(MAREASemaTypedAstLibraries, SizeArgumentIsNotATypeSoItStaysUntyped)
{
    GTEST_COUT << "Validating a result spelled Count, whose argument is a size and not a type, stays untyped." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport collections.array as array;\nfunction f(values: array<float32, 4>): sinteger32\n    return values.count();\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "<none>") << "A size argument is not a type, so the call stays untyped.";
}

TEST(MAREASemaTypedAstLibraries, TaskResultGivesTheTaskTypeArgument)
{
    GTEST_COUT << "Validating item.result() of a task<float32> is typed float32." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport concurrency.task as task;\nfunction f(item: task<float32>): float32\n    return item.result();\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "float32");
}

TEST(MAREASemaTypedAstLibraries, AppendOnAReceiverUnderTheReservedNamespaceIsTypedByItsLiteralResult)
{
    GTEST_COUT << "Validating tasks.append(x) on a c.array receiver is typed with the literal result of its signature." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main; require c.array;\nimport collections.array as array;\nfunction f(): sinteger32\n    local values: c.array<float32> = c.array<float32>{};\n    values.append(1.0);\n    return 0;\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "void");
}

TEST(MAREASemaTypedAstLibraries, TableIndexGivesTheValueType)
{
    GTEST_COUT << "Validating scores[\"ana\"] on a table<string, sinteger32> is typed with the value type." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport collections.table as table;\nfunction f(scores: table<string, sinteger32>): sinteger32\n    return scores[\"ana\"];\nend\n"));

    const MAREA_SemaTypedNode *access{ MAREA_nth(fixture, MAREA_AST_KIND_INDEX_ACCESS, 0) };

    ASSERT_NE(access, nullptr);

    EXPECT_EQ(MAREA_typeOf(access), "sinteger32");
}

TEST(MAREASemaTypedAstLibraries, ArrayIndexStillGivesTheElementType)
{
    GTEST_COUT << "Validating the table rule leaves the element read of a c.array unchanged." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main; require c.array;\nimport collections.array as array;\nfunction f(values: c.array<float32>): float32\n    return values[0];\nend\n"));

    const MAREA_SemaTypedNode *access{ MAREA_nth(fixture, MAREA_AST_KIND_INDEX_ACCESS, 0) };

    ASSERT_NE(access, nullptr);

    EXPECT_EQ(MAREA_typeOf(access), "float32");
}

TEST(MAREASemaTypedAstLibraries, MemberWithNoSignatureStaysOpaque)
{
    GTEST_COUT << "Validating a library member the signature function does not know stays untyped, with no diagnostic." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), MAREA_librarySignatures());

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport collections.array as array;\nfunction f(values: array<float32, 4>): float32\n    return values.unknown_member();\nend\n"));

    const MAREA_SemaTypedNode *call{ MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0) };

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(MAREA_typeOf(call), "<none>");
}

TEST(MAREASemaTypedAstLibraries, NoSignatureFunctionChangesNothing)
{
    GTEST_COUT << "Validating a resolver with no signature function leaves every library call untyped, as before." << std::endl;

    MAREA_LibraryFixture fixture(MAREA_externalLibraries(), nullptr);

    ASSERT_TRUE(fixture.MAREA_build("namespace main;\nimport collections.array as array;\nfunction f(values: array<float32, 4>): array<float32, 4>\n    local fixed: array<float32, 4> = array<float32, 4>.filled(0.0);\n    local total: float32 = values.sum();\n    return fixed;\nend\n"));

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 0)), "<none>") << "The static call must stay untyped.";

    EXPECT_EQ(MAREA_typeOf(MAREA_nth(fixture, MAREA_AST_KIND_CALL_EXPRESSION, 1)), "<none>") << "The instance call must stay untyped.";
}
