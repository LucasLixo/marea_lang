// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAIRHELPERS_HPP)
    #define MAREA_TESTS_MAREAIRHELPERS_HPP

    #include "marea/ir/MAREAIr.h"
    #include "marea/sema/MAREASemaTypedAst.h"

    #include "tests/MAREALibraryRegistryHelpers.hpp"
    #include "tests/MAREASemaHelpers.hpp"

    #include <algorithm>
    #include <filesystem>
    #include <fstream>
    #include <map>
    #include <memory>
    #include <sstream>
    #include <string>
    #include <vector>

inline std::string MAREA_irRender(const MAREA_IrNode *node)
{
    std::string result{ "(" };

    result += MAREA_IrKindGetName(MAREA_IrNodeGetKind(node));

    if (MAREA_IrNodeGetText(node) != nullptr) {
        result += " " + std::string(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));
    }

    if (MAREA_IrNodeGetTypeName(node) != nullptr) {
        result += " :" + std::string(MAREA_IrNodeGetTypeName(node), MAREA_IrNodeGetTypeNameLength(node));
    }

    if (MAREA_IrNodeGetOperator(node) != MAREA_TOKEN_KIND_ERROR) {
        result += std::string(" op=") + MAREA_TokenKindGetName(MAREA_IrNodeGetOperator(node));
    }

    if (MAREA_IrNodeGetFlags(node) != 0) {
        result += " flags=" + std::to_string(MAREA_IrNodeGetFlags(node));
    }

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(node); ++index) {
        const MAREA_IrNode *child{ nullptr };

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, index, &child));

        result += " " + MAREA_irRender(child);
    }

    return result + ")";
}

inline const MAREA_IrNode *MAREA_irChild(const MAREA_IrNode *node, MAREA_IntegerSize index)
{
    const MAREA_IrNode *child{ nullptr };

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, index, &child));

    return child;
}

inline std::string MAREA_irText(const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetText(node) == nullptr) {
        return "";
    }

    return std::string(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));
}

inline std::string MAREA_irTypeName(const MAREA_IrNode *node)
{
    if (MAREA_IrNodeGetTypeName(node) == nullptr) {
        return "";
    }

    return std::string(MAREA_IrNodeGetTypeName(node), MAREA_IrNodeGetTypeNameLength(node));
}

inline std::string MAREA_irReadExample(const std::string &relativePath)
{
    std::ifstream stream(std::string(MAREA_TESTS_EXAMPLES_DIRECTORY) + "/" + relativePath, std::ios::binary);

    std::ostringstream buffer;

    buffer << stream.rdbuf();

    return buffer.str();
}

struct MAREA_IrExample
{
    std::string relativePath;
    std::string source;
};

struct MAREA_IrExampleSet
{
    std::vector<MAREA_IrExample> files;
    std::map<std::string, const MAREA_AstNode *> roots;
    std::vector<std::unique_ptr<MAREA_ParseFixture>> parsed;

    MAREA_IrExampleSet()
    {
        const std::filesystem::path ROOT{ MAREA_TESTS_EXAMPLES_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".marea")) {
                files.push_back({ std::filesystem::relative(entry.path(), ROOT).generic_string(), MAREA_irReadExample(std::filesystem::relative(entry.path(), ROOT).generic_string()) });
            }
        }

        std::sort(files.begin(), files.end(), [](const MAREA_IrExample &left, const MAREA_IrExample &right) { return left.relativePath < right.relativePath; });

        for (const MAREA_IrExample &file : files) {
            auto fixture{ std::make_unique<MAREA_ParseFixture>() };

            MAREA_IGNORE(MAREA_LexContextSetFileName(fixture->lexer.context, file.relativePath.c_str()));

            MAREA_IGNORE(fixture->MAREA_parse(file.source));

            std::string namespaceText{ file.relativePath.substr(0, file.relativePath.size() - std::string(".marea").size()) };

            std::replace(namespaceText.begin(), namespaceText.end(), '/', '.');

            roots[namespaceText] = fixture->root;

            parsed.push_back(std::move(fixture));
        }
    }

    MAREA_IrExampleSet(const MAREA_IrExampleSet &) = delete;
    MAREA_IrExampleSet &operator=(const MAREA_IrExampleSet &) = delete;

    MAREA_ResolveCallback MAREA_resolver() const
    {
        return [this](const std::string &text, const MAREA_AstNode **outRoot) {
            const auto FOUND{ roots.find(text) };

            if (FOUND != roots.end()) {
                *outRoot = FOUND->second;

                return MAREA_SEMA_IMPORT_RESULT_FOUND;
            }

            const bool EXTERNAL{ (text.compare(0, 12, "collections.") == 0) || (text.compare(0, 12, "concurrency.") == 0) || (text.compare(0, 7, "arenas.") == 0) || (text == "text") || ((text.compare(0, 2, "c.") == 0) || (text.compare(0, 6, "marea.") == 0) || (text.compare(0, 5, "demo.") == 0)) };

            return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        };
    }
};

struct MAREA_IrFixture
{
    MAREA_SemaFixture sema;
    MAREA_IrContext *context{ nullptr };
    const MAREA_SemaTypedNode *typedRoot{ nullptr };
    const MAREA_IrNode *module{ nullptr };

    explicit MAREA_IrFixture(MAREA_ResolveCallback resolve = nullptr)
        : sema(std::move(resolve))
    {
        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(sema.context, MAREA_buildLibraryRegistry(sema.parser.lexer.arena)));

        MAREA_IGNORE(MAREA_IrContextCreate(sema.parser.lexer.arena, sema.parser.lexer.sink, &context));
    }

    MAREA_IrFixture(const MAREA_IrFixture &) = delete;
    MAREA_IrFixture &operator=(const MAREA_IrFixture &) = delete;

    MAREA_IrExcept MAREA_lower(const std::string &source, const std::string &filePath = "main.marea")
    {
        if ((sema.MAREA_sema(source, filePath) != MAREA_SEMA_EXCEPT_SUCCESS) || (sema.MAREA_errorCount() != 0)) {
            return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
        }

        if (MAREA_SemaBuildTypedAst(sema.context, sema.parser.root, &typedRoot) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
        }

        return MAREA_IrLowerCompilationUnit(context, typedRoot, &module);
    }

    std::vector<MAREA_Uinteger32> MAREA_irCodes() const
    {
        std::vector<MAREA_Uinteger32> result;

        for (MAREA_Uinteger32 code : sema.parser.MAREA_codes()) {
            if ((code >= 4000) && (code < 5000)) {
                result.push_back(code);
            }
        }

        return result;
    }
};

#endif
