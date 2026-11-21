// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAPARSEHELPERS_HPP)
    #define MAREA_TESTS_MAREAPARSEHELPERS_HPP

    #include "tests/MAREALexHelpers.hpp"

    #include <initializer_list>
    #include <sstream>
    #include <string>
    #include <vector>

inline std::string MAREA_lines(std::initializer_list<const char *> lines)
{
    std::string result;

    for (const char *line : lines) {
        result += line;
        result += "\n";
    }

    return result;
}

struct MAREA_ParseFixture
{
    MAREA_LexFixture lexer;
    MAREA_ParseContext *context{ nullptr };
    const MAREA_AstNode *root{ nullptr };

    MAREA_ParseFixture()
    {
        MAREA_IGNORE(MAREA_ParseContextCreate(lexer.arena, lexer.sink, &context));
    }

    MAREA_ParseFixture(const MAREA_ParseFixture &) = delete;
    MAREA_ParseFixture &operator=(const MAREA_ParseFixture &) = delete;

    MAREA_ParseExcept MAREA_parse(const std::string &source)
    {
        if (lexer.MAREA_lex(source) != MAREA_LEX_EXCEPT_SUCCESS) {
            return MAREA_PARSE_EXCEPT_ERROR_NULL_POINTER;
        }

        return MAREA_ParseCompilationUnit(context, lexer.list, &root);
    }

    std::string MAREA_dump() const
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        MAREA_IGNORE(MAREA_AstDump(lexer.arena, root, &text, &length));

        return std::string(text, length);
    }

    std::string MAREA_itemsDump() const
    {
        const std::string PREFIX{ MAREA_lines({ "COMPILATION_UNIT", "  NAMESPACE_DECLARATION", "    QUALIFIED_NAME", "      IDENTIFIER \"main\"" }) };

        const std::string FULL{ MAREA_dump() };

        if (FULL.compare(0, PREFIX.size(), PREFIX) != 0) {
            return FULL;
        }

        return FULL.substr(PREFIX.size());
    }

    std::vector<MAREA_Uinteger32> MAREA_codes() const
    {
        return lexer.MAREA_diagnosticCodes();
    }

    std::vector<MAREA_Uinteger32> MAREA_offsets() const
    {
        std::vector<MAREA_Uinteger32> result;

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(lexer.sink) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(lexer.sink, index, &diagnostic));

            result.push_back(MAREA_DiagnosticGetPosition(diagnostic).offset);
        }

        return result;
    }
};

#endif
