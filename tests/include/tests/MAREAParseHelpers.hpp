// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAPARSEHELPERS_HPP)
    #define MAREA_TESTS_MAREAPARSEHELPERS_HPP

    #include "tests/MAREALexHelpers.hpp"

    #include <cctype>
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

inline std::string MAREA_render(const MAREA_AstNode *node)
{
    const MAREA_AstKind KIND{ MAREA_AstNodeGetKind(node) };

    const MAREA_IntegerSize COUNT{ MAREA_AstNodeGetChildCount(node) };

    std::vector<std::string> children;

    for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
        const MAREA_AstNode *child{ nullptr };

        MAREA_IGNORE(MAREA_AstNodeGetChildAt(node, index, &child));

        children.push_back(MAREA_render(child));
    }

    if (MAREA_AstNodeGetText(node) != nullptr) {
        return std::string(MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
    }

    if (KIND == MAREA_AST_KIND_QUALIFIED_NAME) {
        std::string joined;

        for (const std::string &part : children) {
            joined += (joined.empty() ? "" : ".") + part;
        }

        return joined;
    }

    std::string name{ MAREA_AstKindGetName(KIND) };

    for (char &letter : name) {
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    }

    const std::string SUFFIX{ "_expression" };

    if ((name.size() > SUFFIX.size()) && (name.compare(name.size() - SUFFIX.size(), SUFFIX.size(), SUFFIX) == 0)) {
        name.resize(name.size() - SUFFIX.size());
    }

    if (MAREA_AstNodeGetOperator(node) != MAREA_TOKEN_KIND_ERROR) {
        name += std::string(":") + MAREA_TokenKindGetName(MAREA_AstNodeGetOperator(node));
    }

    if (MAREA_AstNodeGetFlags(node) != 0) {
        name += "#" + std::to_string(MAREA_AstNodeGetFlags(node));
    }

    std::string result{ "(" + name };

    for (const std::string &child : children) {
        result += " " + child;
    }

    return result + ")";
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

    std::string MAREA_expression(const std::string &text)
    {
        if (MAREA_parse("namespace main; constant x: integer32 = " + text + ";") != MAREA_PARSE_EXCEPT_SUCCESS) {
            return "<parse failed>";
        }

        const MAREA_AstNode *declaration{ nullptr };

        const MAREA_AstNode *value{ nullptr };

        if ((MAREA_AstNodeGetChildAt(root, 1, &declaration) != MAREA_AST_EXCEPT_SUCCESS) || (MAREA_AstNodeGetChildAt(declaration, 2, &value) != MAREA_AST_EXCEPT_SUCCESS)) {
            return "<no expression>";
        }

        return MAREA_render(value);
    }

    std::string MAREA_statements(const std::string &text)
    {
        if (MAREA_parse("namespace main; function f() " + text + " end") != MAREA_PARSE_EXCEPT_SUCCESS) {
            return "<parse failed>";
        }

        const MAREA_AstNode *function{ nullptr };

        const MAREA_AstNode *block{ nullptr };

        if ((MAREA_AstNodeGetChildAt(root, 1, &function) != MAREA_AST_EXCEPT_SUCCESS) || (MAREA_AstNodeGetChildAt(function, 2, &block) != MAREA_AST_EXCEPT_SUCCESS)) {
            return "<no block>";
        }

        std::string result;

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_AstNodeGetChildCount(block); ++index) {
            const MAREA_AstNode *statement{ nullptr };

            MAREA_IGNORE(MAREA_AstNodeGetChildAt(block, index, &statement));

            result += (result.empty() ? "" : " ") + MAREA_render(statement);
        }

        return result;
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
