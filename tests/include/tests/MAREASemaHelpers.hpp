// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREASEMAHELPERS_HPP)
    #define MAREA_TESTS_MAREASEMAHELPERS_HPP

    #include "tests/MAREAParseHelpers.hpp"

    #include <functional>
    #include <string>
    #include <vector>

using MAREA_ResolveCallback = std::function<MAREA_SemaImportResult(const std::string &, const MAREA_AstNode **)>;

inline void MAREA_CALL MAREA_resolveThunk(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot)
{
    auto *callback{ static_cast<MAREA_ResolveCallback *>(userData) };

    *outResult = (*callback)(std::string(namespaceText, namespaceLength), outRoot);
}

struct MAREA_SemaFixture
{
    MAREA_ParseFixture parser;
    MAREA_ResolveCallback callback;
    MAREA_SemaImportResolver *resolver{ nullptr };
    MAREA_SemaContext *context{ nullptr };

    explicit MAREA_SemaFixture(MAREA_ResolveCallback resolve = nullptr)
        : callback(resolve ? std::move(resolve) : MAREA_ResolveCallback([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_NOT_FOUND; }))
    {
        MAREA_IGNORE(MAREA_SemaImportResolverCreate(parser.lexer.arena, &MAREA_resolveThunk, &callback, &resolver));
        MAREA_IGNORE(MAREA_SemaContextCreate(parser.lexer.arena, parser.lexer.sink, resolver, &context));
    }

    MAREA_SemaFixture(const MAREA_SemaFixture &) = delete;
    MAREA_SemaFixture &operator=(const MAREA_SemaFixture &) = delete;

    MAREA_SemaExcept MAREA_sema(const std::string &source, const std::string &filePath = "main.marea")
    {
        ownedPath = filePath;

        if (MAREA_LexContextSetFileName(parser.lexer.context, ownedPath.c_str()) != MAREA_LEX_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
        }

        if (parser.MAREA_parse(source) != MAREA_PARSE_EXCEPT_SUCCESS) {
            return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
        }

        return MAREA_SemaCheckCompilationUnit(context, parser.root);
    }

    std::vector<MAREA_Uinteger32> MAREA_semaCodes() const
    {
        std::vector<MAREA_Uinteger32> result;

        for (MAREA_Uinteger32 code : parser.MAREA_codes()) {
            if (code >= 3000) {
                result.push_back(code);
            }
        }

        return result;
    }

    MAREA_IntegerSize MAREA_errorCount() const
    {
        return MAREA_DiagnosticSinkGetErrorCount(parser.lexer.sink);
    }

  private:
    std::string ownedPath;
};

#endif
