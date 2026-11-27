// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathLibrary.h"
#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREACatalogueHelpers.hpp"
#include "tests/MAREASemaHelpers.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace
{
    struct MAREA_MathTypeExpectation
    {
        const char *name;
        MAREA_Uinteger64 size;
        MAREA_Uinteger64 laneSize;
        std::size_t lanes;
    };

    const std::vector<MAREA_MathTypeExpectation> MATH_TYPES{
        { "Vector2Float32", 16, 4, 2 },
        { "Vector3Float32", 16, 4, 3 },
        { "Vector4Float32", 16, 4, 4 },
        { "Vector2Sinteger32", 16, 4, 2 },
        { "Vector3Sinteger32", 16, 4, 3 },
        { "Vector4Sinteger32", 16, 4, 4 },
        { "Vector2Boolean", 4, 1, 2 },
        { "Vector3Boolean", 4, 1, 3 },
        { "Vector4Boolean", 4, 1, 4 },
    };

    std::string MAREA_mathSource()
    {
        return MAREA_readFile(MAREA_mathSourcePath());
    }

    const MAREA_SemaTypedNode *MAREA_findStructure(const MAREA_SemaTypedNode *node, const std::string &name)
    {
        const MAREA_AstNode *syntax{ MAREA_SemaTypedNodeGetAstNode(node) };

        if ((MAREA_AstNodeGetKind(syntax) == MAREA_AST_KIND_STRUCTURE_DECLARATION) && (MAREA_render(syntax).find(" " + name + " ") != std::string::npos)) {
            return node;
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypedNodeGetChildCount(node); ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                continue;
            }

            const MAREA_SemaTypedNode *found{ MAREA_findStructure(child, name) };

            if (found != nullptr) {
                return found;
            }
        }

        return nullptr;
    }

    std::string MAREA_implementationOf(const std::string &source, const std::string &name)
    {
        const std::string HEAD{ "\nimplementation " + name + "\n" };

        const std::size_t BEGIN{ source.find(HEAD) };

        if (BEGIN == std::string::npos) {
            return "";
        }

        const std::size_t
END
{ source.find("\nend\n", BEGIN + HEAD.size()) };

return source.substr(BEGIN, (END == std::string::npos) ? std::string::npos : END - BEGIN);
}
// INFO: the signature of one registered operation that these tests call, made once from its name. A name that
// marea.math does not register has none, so a call that is typed names a real function.
struct MAREA_MathSignature
{
    std::string type;
    std::string result;
    std::vector<MAREA_SemaSignatureParameter> parameters;
    MAREA_SemaSignature signature{};
};

const MAREA_SemaSignature *MAREA_mathSignature(const std::string &member)
{
    static MAREA_ShippedRegistry registry;
    static const bool REGISTERED{ MAREA_registerMathLibrary(registry.registry) };
    static std::map<std::string, std::unique_ptr<MAREA_MathSignature>> signatures;

    if (!REGISTERED || (MAREA_TypeRegistryFindFunction(registry.registry, "marea.math", MAREA_INTEGER_SIZE(10), member.c_str(), member.size()) == nullptr)) {
        return nullptr;
    }

    std::unique_ptr<MAREA_MathSignature> &row{ signatures[member] };

    if (row == nullptr) {
        row = std::make_unique<MAREA_MathSignature>();

        const std::size_t SIZE{ member.find("_vector") + 7 };
        const std::string LANE{ member.substr(SIZE + 2) };

        row->type = "marea.math.Vector" + member.substr(SIZE, 1) + (LANE == "float32" ? "Float32" : "Sinteger32");
        row->result = (member.rfind("dot_", 0) == 0) ? LANE : row->type;
        row->parameters.assign(2, { row->type.c_str(), row->type.size() });
        row->signature = { row->parameters.data(), row->parameters.size(), row->result.c_str(), row->result.size() };
    }

    return &row->signature;
}

void MAREA_CALL MAREA_mathSignatureThunk(MAREA_Null, const MAREA_Character8 *, MAREA_IntegerSize, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature)
{
    *outSignature = MAREA_mathSignature(std::string(memberText, memberLength));

    *outResult = (*outSignature == nullptr) ? MAREA_SEMA_IMPORT_RESULT_NOT_FOUND : MAREA_SEMA_IMPORT_RESULT_FOUND;
}

// INFO: the resolved type of each call that is external, in the order of the tree.
void MAREA_collectExternalCalls(const MAREA_SemaTypedNode *node, std::vector<std::string> &types)
{
    if (MAREA_SemaTypedNodeIsExternalCall(node) == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *text{ nullptr };
        MAREA_IntegerSize length{ 0 };

        const bool TYPED{ MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS };

        types.emplace_back((TYPED && (text != nullptr)) ? std::string(text, length) : std::string());
    }

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypedNodeGetChildCount(node); ++index) {
        const MAREA_SemaTypedNode *child{ nullptr };

        if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            MAREA_collectExternalCalls(child, types);
        }
    }
}

std::vector<std::string> MAREA_externalCallsOf(MAREA_SemaFixture &fixture)
{
    std::vector<std::string> types;
    const MAREA_SemaTypedNode *root{ nullptr };

    if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        MAREA_collectExternalCalls(root, types);
    }

    return types;
}

// INFO: a program that requires marea.math, whose tree the resolver gives, with the signatures of its C functions.
struct MAREA_MathProgramFixture : MAREA_SemaFixture
{
    MAREA_MathProgramFixture()
        : MAREA_SemaFixture([](const std::string &text, const MAREA_AstNode **outRoot) {
              if (text != "marea.math") {
                  return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
              }

              *outRoot = MAREA_shippedMath();

              return MAREA_SEMA_IMPORT_RESULT_FOUND;
          })
    {
        MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(resolver, &MAREA_mathSignatureThunk));
    }
};
} // namespace

TEST(MAREASemaMathLibrary, ShippedSourcePassesSemaAndDeclaresTheNineVectorTypes)
{
    GTEST_COUT << "Validating the shipped source of marea.math passes sema and measures each vector type." << std::endl;

    const std::string SOURCE{ MAREA_mathSource() };

    ASSERT_FALSE(SOURCE.empty()) << "The shipped source must exist at " << MAREA_mathSourcePath();

    MAREA_SemaFixture fixture;

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(fixture.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(fixture.MAREA_sema(SOURCE, "marea/math.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The shipped source must give no diagnostic.";

    const MAREA_SemaTypedNode *root{ nullptr };

    ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The typed tree must be built.";

    for (const MAREA_MathTypeExpectation &expectation : MATH_TYPES) {
        const MAREA_SemaTypedNode *node{ MAREA_findStructure(root, expectation.name) };

        ASSERT_NE(node, nullptr) << "The shipped source must declare " << expectation.name;

        const MAREA_SemaTypeLayout *layout{ nullptr };

        ASSERT_EQ(MAREA_SemaTypedNodeGetLayout(node, &layout), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must have a layout.";

        MAREA_Uinteger64 size{ 0 };

        MAREA_Uinteger64 alignment{ 0 };

        ASSERT_EQ(MAREA_SemaTypeLayoutGetSize(layout, &size), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must be measured.";

        ASSERT_EQ(MAREA_SemaTypeLayoutGetAlignment(layout, &alignment), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must be measured.";

        EXPECT_EQ(size, expectation.size) << "The size of " << expectation.name;

        EXPECT_EQ(alignment, expectation.size) << "The alignment of " << expectation.name;

        for (std::size_t lane{ 0 }; lane < expectation.lanes; ++lane) {
            MAREA_Uinteger64 offset{ 0 };

            ASSERT_EQ(MAREA_SemaTypeLayoutGetMemberOffset(layout, lane, &offset), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << expectation.name << " must have lane " << lane;

            EXPECT_EQ(offset, lane * expectation.laneSize) << "The place of lane " << lane << " of " << expectation.name;
        }
    }
}

TEST(MAREASemaMathLibrary, ShippedSourceDeclaresTheOperatorsOfEachVectorType)
{
    GTEST_COUT << "Validating each vector type of the shipped source has its operator functions." << std::endl;

    const std::string SOURCE{ MAREA_mathSource() };

    ASSERT_FALSE(SOURCE.empty()) << "The shipped source must exist at " << MAREA_mathSourcePath();

    for (const MAREA_MathTypeExpectation &expectation : MATH_TYPES) {
        const std::string BODY{ MAREA_implementationOf(SOURCE, expectation.name) };

        ASSERT_FALSE(BODY.empty()) << expectation.name << " must have an implementation block.";

        std::vector<std::string> tokens{ "==", "!=", "[]" };

        if (std::string(expectation.name).find("Boolean") == std::string::npos) {
            tokens.insert(tokens.end(), { "+", "-", "*", "/" });
        }

        for (const std::string &token : tokens) {
            EXPECT_NE(BODY.find("#[operator = \"" + token + "\"]"), std::string::npos) << expectation.name << " must declare the operator " << token;
        }
    }
}

TEST(MAREASemaMathLibrary, OperatorBodiesCallTheRegisteredFunctionsOfTheirOwnNamespace)
{
    GTEST_COUT << "Validating each arithmetic operator of marea.math is a typed external call into its own namespace." << std::endl;

    MAREA_SemaFixture fixture;

    MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(fixture.resolver, &MAREA_mathSignatureThunk));

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(fixture.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(fixture.MAREA_sema(MAREA_mathSource(), "marea/math.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The shipped source must give no diagnostic.";

    std::vector<std::string> expected;

    for (const MAREA_MathTypeExpectation &expectation : MATH_TYPES) {
        if (std::string(expectation.name).find("Boolean") == std::string::npos) {
            expected.insert(expected.end(), 4, "marea.math." + std::string(expectation.name));
        }
    }

    EXPECT_EQ(MAREA_externalCallsOf(fixture), expected) << "Four operators of six types call a registered function.";
}

TEST(MAREASemaMathLibrary, AFileChecksACallIntoItsOwnNamespaceAgainstTheSignature)
{
    GTEST_COUT << "Validating a wrong argument count into the namespace of the file is raised." << std::endl;

    MAREA_SemaFixture fixture;

    MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(fixture.resolver, &MAREA_mathSignatureThunk));

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(fixture.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(fixture.MAREA_sema("namespace marea.math; function check(value: float32) marea.math.dot_vector3_float32(value); end", "marea/math.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(fixture.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH }) << "The signature decides the count.";
}

TEST(MAREASemaMathLibrary, AProgramCallIntoMathIsTypedByTheSignature)
{
    GTEST_COUT << "Validating a call of a registered operation from a program is external and typed." << std::endl;

    MAREA_MathProgramFixture typed;

    ASSERT_EQ(typed.MAREA_sema("namespace main; require marea.math as math; function check(value: math.Vector3Float32): float32 return math.dot_vector3_float32(value, value) + marea.math.dot_vector3_float32(value, value); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    ASSERT_EQ(typed.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The program must give no diagnostic.";

    EXPECT_EQ(MAREA_externalCallsOf(typed), (std::vector<std::string>{ "float32", "float32" })) << "An alias and a full path call a registered function.";

    MAREA_MathProgramFixture wrong;

    ASSERT_EQ(wrong.MAREA_sema("namespace main; require marea.math as math; function check(value: math.Vector3Float32) math.dot_vector3_float32(\"text\", value); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_EQ(wrong.MAREA_semaCodes(), std::vector<MAREA_Uinteger32>{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH }) << "An argument is compared with the signature.";
}

TEST(MAREASemaMathLibrary, AMemberOfAProjectNamespaceIsNeverAskedAsARegisteredFunction)
{
    GTEST_COUT << "Validating only a namespace under c or marea has registered functions beside its source." << std::endl;

    static MAREA_ParseFixture tools;

    ASSERT_EQ(tools.MAREA_parse("namespace project.tools;"), MAREA_PARSE_EXCEPT_SUCCESS) << "The tree must parse.";

    MAREA_SemaFixture fixture([](const std::string &text, const MAREA_AstNode **outRoot) {
        if (text != "project.tools") {
            return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        }

        *outRoot = tools.root;

        return MAREA_SEMA_IMPORT_RESULT_FOUND;
    });

    MAREA_IGNORE(MAREA_SemaImportResolverSetSignatureFunction(fixture.resolver, &MAREA_mathSignatureThunk));

    ASSERT_EQ(fixture.MAREA_sema("namespace main; import project.tools as tools; function check() tools.dot_vector3_float32(); end"), MAREA_SEMA_EXCEPT_SUCCESS) << "The check must succeed.";

    EXPECT_TRUE(fixture.MAREA_semaCodes().empty()) << "No signature is asked for a project namespace.";

    EXPECT_TRUE(MAREA_externalCallsOf(fixture).empty()) << "The call is not external.";
}
