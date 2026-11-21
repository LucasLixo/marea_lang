// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAIRHELPERS_HPP)
    #define MAREA_TESTS_MAREAIRHELPERS_HPP

    #include "marea/ir/MAREAIr.h"
    #include "marea/sema/MAREASemaTypedAst.h"

    #include "tests/MAREASemaHelpers.hpp"

    #include <fstream>
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

struct MAREA_IrFixture
{
    MAREA_SemaFixture sema;
    MAREA_IrContext *context{ nullptr };
    const MAREA_SemaTypedNode *typedRoot{ nullptr };
    const MAREA_IrNode *module{ nullptr };

    explicit MAREA_IrFixture(MAREA_ResolveCallback resolve = nullptr)
        : sema(std::move(resolve))
    {
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
