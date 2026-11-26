// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREACONTAINERHELPERS_HPP)
    #define MAREA_TESTS_MAREACONTAINERHELPERS_HPP

    #include "marea/sema/MAREASemaLayout.h"
    #include "marea/sema/MAREASemaTypedAst.h"

    #include "tests/MAREASemaHelpers.hpp"

    #include <cstdint>
    #include <string>
    #include <vector>

// INFO: the answer of the layout of one structure named in a test: the verdict, and when it is measured the size, the
// alignment and the offset of each field.
struct MAREA_HolderLayout
{
    bool found{ false };
    bool measured{ false };
    MAREA_SemaMeasureReason reason{ MAREA_SEMA_MEASURE_REASON_NONE };
    std::uint64_t size{ 0 };
    std::uint64_t alignment{ 0 };
    std::vector<std::uint64_t> offsets;
};

// INFO: the libraries of the language answer as external namespaces, the same as the caller of a real compile.
inline MAREA_ResolveCallback MAREA_containerResolver()
{
    return [](const std::string &text, const MAREA_AstNode **) { return ((text.rfind("marea.", 0) == 0) || (text.rfind("c.", 0) == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; };
}

// INFO: the layout of the last structure of the source, which a test names Holder. A source with an error has none.
inline MAREA_HolderLayout MAREA_holderLayout(const std::string &source)
{
    MAREA_HolderLayout result;

    MAREA_SemaFixture fixture(MAREA_containerResolver());

    if ((fixture.MAREA_sema(source) != MAREA_SEMA_EXCEPT_SUCCESS) || (fixture.MAREA_errorCount() != MAREA_INTEGER_SIZE(0))) {
        return result;
    }

    const MAREA_SemaTypedNode *root{ nullptr };

    if (MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_SemaTypedNode *holder{ nullptr };

    std::vector<const MAREA_SemaTypedNode *> pending{ root };

    while (!pending.empty()) {
        const MAREA_SemaTypedNode *node{ pending.back() };

        pending.pop_back();

        if (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) == MAREA_AST_KIND_STRUCTURE_DECLARATION) {
            holder = node;

            break;
        }

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypedNodeGetChildCount(node); ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                pending.push_back(child);
            }
        }
    }

    const MAREA_SemaTypeLayout *layout{ nullptr };

    if ((holder == nullptr) || (MAREA_SemaTypedNodeGetLayout(holder, &layout) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS)) {
        return result;
    }

    result.found = true;
    result.reason = MAREA_SemaTypeLayoutGetReason(layout);
    result.measured = (MAREA_SemaTypeLayoutGetVerdict(layout) == MAREA_SEMA_MEASURE_VERDICT_MEASURED);

    if (!result.measured) {
        return result;
    }

    MAREA_Uinteger64 value{ 0 };

    MAREA_IGNORE(MAREA_SemaTypeLayoutGetSize(layout, &value));

    result.size = value;

    MAREA_IGNORE(MAREA_SemaTypeLayoutGetAlignment(layout, &value));

    result.alignment = value;

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_SemaTypeLayoutGetMemberCount(layout); ++index) {
        MAREA_IGNORE(MAREA_SemaTypeLayoutGetMemberOffset(layout, index, &value));

        result.offsets.push_back(value);
    }

    return result;
}

// INFO: the codes of a require of one library with a require resolver that knows a namespace only when the registry of
// the shipped libraries holds its type.
inline std::vector<MAREA_Uinteger32> MAREA_requireCodes(const std::string &namespaceText, const std::string &typeName, const MAREA_TypeRegistry *registry)
{
    MAREA_SemaFixture fixture;

    MAREA_ResolveCallback answer{ [&](const std::string &text, const MAREA_AstNode **) {
        const bool HELD{ (text == namespaceText) && (MAREA_TypeRegistryFind(registry, namespaceText.c_str(), namespaceText.size(), typeName.c_str(), typeName.size()) != nullptr) };

        return HELD ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    MAREA_SemaImportResolver *resolver{ nullptr };

    MAREA_IGNORE(MAREA_SemaImportResolverCreate(fixture.parser.lexer.arena, &MAREA_resolveThunk, &answer, &resolver));
    MAREA_IGNORE(MAREA_SemaContextSetRequireResolver(fixture.context, resolver));
    MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, registry));
    MAREA_IGNORE(fixture.MAREA_sema("namespace main; require " + namespaceText + ";"));

    return fixture.MAREA_semaCodes();
}

#endif
