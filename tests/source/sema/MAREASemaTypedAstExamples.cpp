// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/sema/MAREASemaTypedAst.h"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>

namespace
{
    struct MAREA_ExampleFile
    {
        std::string relativePath;
        std::string source;
        std::string namespaceText;
    };

    std::string MAREA_readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    std::vector<MAREA_ExampleFile> MAREA_collectExamples()
    {
        std::vector<MAREA_ExampleFile> result;

        const std::filesystem::path ROOT{ MAREA_TESTS_EXAMPLES_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (!entry.is_regular_file() || (entry.path().extension() != ".marea")) {
                continue;
            }

            std::string relative{ std::filesystem::relative(entry.path(), ROOT).generic_string() };

            std::string namespaceText{ relative.substr(0, relative.size() - std::string(".marea").size()) };

            std::replace(namespaceText.begin(), namespaceText.end(), '/', '.');

            result.push_back({ relative, MAREA_readFile(entry.path()), namespaceText });
        }

        std::sort(result.begin(), result.end(), [](const MAREA_ExampleFile &left, const MAREA_ExampleFile &right) { return left.relativePath < right.relativePath; });

        return result;
    }

    bool MAREA_startsWith(const std::string &text, const std::string &prefix)
    {
        return text.compare(0, prefix.size(), prefix) == 0;
    }
} // namespace

TEST(MAREASemaTypedAstExamples, EveryExampleBuildsATypedTreeWithoutCrashing)
{
    GTEST_COUT << "Validating every example checks clean and then builds a typed shadow tree without crashing." << std::endl;

    const std::vector<MAREA_ExampleFile> FILES{ MAREA_collectExamples() };

    std::map<std::string, const MAREA_AstNode *> roots;

    std::vector<std::unique_ptr<MAREA_ParseFixture>> parsed;

    std::vector<std::string> paths;

    for (const MAREA_ExampleFile &file : FILES) {
        paths.push_back(file.relativePath);

        auto fixture{ std::make_unique<MAREA_ParseFixture>() };

        ASSERT_EQ(MAREA_LexContextSetFileName(fixture->lexer.context, paths.back().c_str()), MAREA_LEX_EXCEPT_SUCCESS) << "The file name of " << file.relativePath << " must be set.";

        ASSERT_EQ(fixture->MAREA_parse(file.source), MAREA_PARSE_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must parse.";

        roots[file.namespaceText] = fixture->root;

        parsed.push_back(std::move(fixture));
    }

    const MAREA_ResolveCallback RESOLVE{ [&roots](const std::string &text, const MAREA_AstNode **outRoot) {
        const auto FOUND{ roots.find(text) };

        if (FOUND != roots.end()) {
            *outRoot = FOUND->second;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        }

        const bool EXTERNAL{ MAREA_startsWith(text, "collections.") || MAREA_startsWith(text, "concurrency.") || MAREA_startsWith(text, "arenas.") || (text == "text") || MAREA_startsWith(text, "c.") };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    for (const MAREA_ExampleFile &file : FILES) {
        MAREA_SemaFixture fixture(RESOLVE);

        ASSERT_EQ(fixture.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The example " << file.relativePath << " must check with zero errors.";

        const MAREA_SemaTypedNode *root{ nullptr };

        EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must build a typed tree.";

        EXPECT_NE(root, nullptr) << "The example " << file.relativePath << " must produce a typed root.";
    }
}

namespace
{
    // An identifier is a value only where an expression is read: never a declared name, a member name, a field key or a loop name.
    bool MAREA_identifierIsValue(MAREA_AstKind parent, MAREA_IntegerSize index, MAREA_IntegerSize count)
    {
        switch (parent) {
        case MAREA_AST_KIND_BINARY_EXPRESSION:
        case MAREA_AST_KIND_UNARY_EXPRESSION:
        case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        case MAREA_AST_KIND_INDEX_ACCESS:
        case MAREA_AST_KIND_ARGUMENT_LIST:
        case MAREA_AST_KIND_INDEX_ENTRY:
        case MAREA_AST_KIND_VALUE_ENTRY:
        case MAREA_AST_KIND_ASSIGNMENT:
        case MAREA_AST_KIND_EXPRESSION_STATEMENT:
        case MAREA_AST_KIND_RETURN_STATEMENT:
        case MAREA_AST_KIND_IF_STATEMENT:
        case MAREA_AST_KIND_ELSEIF_CLAUSE:
        case MAREA_AST_KIND_WHILE_STATEMENT:
        case MAREA_AST_KIND_SWITCH_STATEMENT:
        case MAREA_AST_KIND_SWITCH_ARM:
            return true;
        case MAREA_AST_KIND_MEMBER_ACCESS:
        case MAREA_AST_KIND_CALL_EXPRESSION:
        case MAREA_AST_KIND_WITH_STATEMENT:
            return index == 0;
        case MAREA_AST_KIND_FIELD_ENTRY:
            return index == 1;
        case MAREA_AST_KIND_LOCAL_DECLARATION:
            return index >= 2;
        case MAREA_AST_KIND_CONSTANT_DECLARATION:
            return index == (count - 1);
        case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
            return (index == 1) || (index == 2);
        case MAREA_AST_KIND_FOR_EACH_STATEMENT:
            return index == (count - 2);
        case MAREA_AST_KIND_PARTITION_STATEMENT:
            return (index == 0) || (index == 1);
        default:
            return false;
        }
    }

    bool MAREA_isLiteralKind(MAREA_AstKind kind)
    {
        return (kind == MAREA_AST_KIND_INTEGER_LITERAL) || (kind == MAREA_AST_KIND_FLOAT_LITERAL) || (kind == MAREA_AST_KIND_STRING_LITERAL) || (kind == MAREA_AST_KIND_CHARACTER_LITERAL) || (kind == MAREA_AST_KIND_BOOLEAN_LITERAL);
    }

    bool MAREA_isOperationKind(MAREA_AstKind kind)
    {
        return (kind == MAREA_AST_KIND_BINARY_EXPRESSION) || (kind == MAREA_AST_KIND_UNARY_EXPRESSION) || (kind == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) || (kind == MAREA_AST_KIND_MEMBER_ACCESS) || (kind == MAREA_AST_KIND_INDEX_ACCESS) || (kind == MAREA_AST_KIND_CALL_EXPRESSION) || (kind == MAREA_AST_KIND_LITERAL_VALUE) || (kind == MAREA_AST_KIND_GENERIC_NAME) || (kind == MAREA_AST_KIND_MACRO_CALL);
    }

    struct MAREA_ExpressionTally
    {
        std::size_t expressions{ 0 };
        std::size_t untypedLiterals{ 0 };
        std::size_t untypedOperations{ 0 };
        std::map<std::string, std::size_t> untypedByKind;
        std::map<std::string, std::size_t> untypedByDetail;
    };

    void MAREA_tallyExpressions(const MAREA_SemaTypedNode *node, MAREA_AstKind parentKind, MAREA_IntegerSize index, MAREA_IntegerSize siblings, MAREA_ExpressionTally &tally)
    {
        const MAREA_AstNode *ast{ MAREA_SemaTypedNodeGetAstNode(node) };

        const MAREA_AstKind KIND{ MAREA_AstNodeGetKind(ast) };

        const bool IS_VALUE_IDENTIFIER{ (KIND == MAREA_AST_KIND_IDENTIFIER) && MAREA_identifierIsValue(parentKind, index, siblings) };

        if (MAREA_isLiteralKind(KIND) || MAREA_isOperationKind(KIND) || IS_VALUE_IDENTIFIER) {
            ++tally.expressions;

            const MAREA_Character8 *text{ nullptr };

            MAREA_IntegerSize length{ 0 };

            if (MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) == MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) {
                const std::string KIND_NAME{ MAREA_AstKindGetName(KIND) };

                if (MAREA_isLiteralKind(KIND)) {
                    ++tally.untypedLiterals;
                } else {
                    ++tally.untypedOperations;
                }

                ++tally.untypedByKind[KIND_NAME];

                const MAREA_Character8 *name{ MAREA_AstNodeGetText(ast) };

                if ((name != nullptr) && (KIND == MAREA_AST_KIND_IDENTIFIER)) {
                    ++tally.untypedByDetail[KIND_NAME + " " + std::string(name, MAREA_AstNodeGetTextLength(ast))];
                }
            }
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize child{ 0 }; child < COUNT; ++child) {
            const MAREA_SemaTypedNode *typedChild{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, child, &typedChild) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_tallyExpressions(typedChild, KIND, child, COUNT, tally);
            }
        }
    }
} // namespace

TEST(MAREASemaTypedAstExamples, EveryExpressionOfEveryExampleHasATypeOrIsCounted)
{
    GTEST_COUT << "Validating the expression nodes of the 23 examples: the ones with no resolved type are counted and printed." << std::endl;

    const std::vector<MAREA_ExampleFile> FILES{ MAREA_collectExamples() };

    std::map<std::string, const MAREA_AstNode *> roots;

    std::vector<std::unique_ptr<MAREA_ParseFixture>> parsed;

    std::vector<std::string> paths;

    for (const MAREA_ExampleFile &file : FILES) {
        paths.push_back(file.relativePath);

        auto fixture{ std::make_unique<MAREA_ParseFixture>() };

        ASSERT_EQ(MAREA_LexContextSetFileName(fixture->lexer.context, paths.back().c_str()), MAREA_LEX_EXCEPT_SUCCESS) << "The file name of " << file.relativePath << " must be set.";

        ASSERT_EQ(fixture->MAREA_parse(file.source), MAREA_PARSE_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must parse.";

        roots[file.namespaceText] = fixture->root;

        parsed.push_back(std::move(fixture));
    }

    const MAREA_ResolveCallback RESOLVE{ [&roots](const std::string &text, const MAREA_AstNode **outRoot) {
        const auto FOUND{ roots.find(text) };

        if (FOUND != roots.end()) {
            *outRoot = FOUND->second;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        }

        const bool EXTERNAL{ MAREA_startsWith(text, "collections.") || MAREA_startsWith(text, "concurrency.") || MAREA_startsWith(text, "arenas.") || (text == "text") || MAREA_startsWith(text, "c.") };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    MAREA_ExpressionTally total;

    for (const MAREA_ExampleFile &file : FILES) {
        MAREA_SemaFixture fixture(RESOLVE);

        ASSERT_EQ(fixture.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The example " << file.relativePath << " must check with zero errors.";

        const MAREA_SemaTypedNode *root{ nullptr };

        ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must build a typed tree.";

        MAREA_ExpressionTally tally;

        MAREA_tallyExpressions(root, MAREA_AST_KIND_COMPILATION_UNIT, 0, 1, tally);

        total.expressions += tally.expressions;
        total.untypedLiterals += tally.untypedLiterals;
        total.untypedOperations += tally.untypedOperations;

        for (const auto &entry : tally.untypedByKind) {
            total.untypedByKind[entry.first] += entry.second;
        }

        for (const auto &entry : tally.untypedByDetail) {
            total.untypedByDetail[entry.first + " in " + file.relativePath] += entry.second;
        }
    }

    GTEST_COUT << "Examples: " << FILES.size() << ", expression nodes: " << total.expressions << ", with no resolved type: " << (total.untypedLiterals + total.untypedOperations) << " (literals: " << total.untypedLiterals << ", other expressions: " << total.untypedOperations << ")." << std::endl;

    for (const auto &entry : total.untypedByKind) {
        GTEST_COUT << "No type, by kind: " << entry.first << " = " << entry.second << std::endl;
    }

    for (const auto &entry : total.untypedByDetail) {
        GTEST_COUT << "No type, name: " << entry.first << " = " << entry.second << std::endl;
    }

    EXPECT_EQ(FILES.size(), std::size_t{ 23 }) << "The sweep must cover the 23 examples.";

    // The ceiling is the count that the documented no-type cases leave (see "Gaps" of the sema doc); a regression raises it. The literals are typed by their own rule and every one of them has a type, so none is in this count.
    EXPECT_LE(total.untypedOperations, std::size_t{ 162 }) << "Expressions other than literals with no resolved type must not grow past the documented cases.";
}

namespace
{
    // Walks a typed tree and counts the literals, checking each one has a resolved type of its own family.
    void MAREA_checkLiteralTypes(const MAREA_SemaTypedNode *node, const std::string &path, std::size_t &literalCount)
    {
        if (node == nullptr) {
            return;
        }

        const MAREA_AstKind KIND{ MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) };

        const bool IS_LITERAL{ (KIND == MAREA_AST_KIND_INTEGER_LITERAL) || (KIND == MAREA_AST_KIND_FLOAT_LITERAL) || (KIND == MAREA_AST_KIND_STRING_LITERAL) || (KIND == MAREA_AST_KIND_CHARACTER_LITERAL) || (KIND == MAREA_AST_KIND_BOOLEAN_LITERAL) };

        if (IS_LITERAL) {
            ++literalCount;

            const MAREA_Character8 *text{ nullptr };

            MAREA_IntegerSize length{ 0 };

            ASSERT_EQ(MAREA_SemaTypedNodeGetResolvedType(node, &text, &length), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "A literal of " << path << " must have a resolved type.";

            const std::string TYPE(text, length);

            const std::vector<std::string> INTEGERS{ "sinteger8", "sinteger16", "sinteger32", "sinteger64", "uinteger8", "uinteger16", "uinteger32", "uinteger64", "float32", "float64" };

            const std::vector<std::string> FLOATS{ "float32", "float64" };

            if (KIND == MAREA_AST_KIND_INTEGER_LITERAL) {
                EXPECT_NE(std::find(INTEGERS.begin(), INTEGERS.end(), TYPE), INTEGERS.end()) << "An integer literal of " << path << " got the type " << TYPE;
            } else if (KIND == MAREA_AST_KIND_FLOAT_LITERAL) {
                EXPECT_NE(std::find(FLOATS.begin(), FLOATS.end(), TYPE), FLOATS.end()) << "A float literal of " << path << " got the type " << TYPE;
            } else {
                const std::string EXPECTED{ (KIND == MAREA_AST_KIND_STRING_LITERAL) ? "string" : ((KIND == MAREA_AST_KIND_CHARACTER_LITERAL) ? "character" : "boolean") };

                EXPECT_EQ(TYPE, EXPECTED) << "A string, character or boolean literal of " << path << " keeps its own type.";
            }
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_SemaTypedNode *child{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_checkLiteralTypes(child, path, literalCount);
            }
        }
    }
} // namespace

TEST(MAREASemaTypedAstExamples, EveryLiteralOfEveryExampleHasAResolvedType)
{
    GTEST_COUT << "Validating every literal of every example has a resolved type of its own family." << std::endl;

    const std::vector<MAREA_ExampleFile> FILES{ MAREA_collectExamples() };

    std::map<std::string, const MAREA_AstNode *> roots;

    std::vector<std::unique_ptr<MAREA_ParseFixture>> parsed;

    std::vector<std::string> paths;

    for (const MAREA_ExampleFile &file : FILES) {
        paths.push_back(file.relativePath);

        auto fixture{ std::make_unique<MAREA_ParseFixture>() };

        ASSERT_EQ(MAREA_LexContextSetFileName(fixture->lexer.context, paths.back().c_str()), MAREA_LEX_EXCEPT_SUCCESS) << "The file name of " << file.relativePath << " must be set.";

        ASSERT_EQ(fixture->MAREA_parse(file.source), MAREA_PARSE_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must parse.";

        roots[file.namespaceText] = fixture->root;

        parsed.push_back(std::move(fixture));
    }

    const MAREA_ResolveCallback RESOLVE{ [&roots](const std::string &text, const MAREA_AstNode **outRoot) {
        const auto FOUND{ roots.find(text) };

        if (FOUND != roots.end()) {
            *outRoot = FOUND->second;

            return MAREA_SEMA_IMPORT_RESULT_FOUND;
        }

        const bool EXTERNAL{ MAREA_startsWith(text, "collections.") || MAREA_startsWith(text, "concurrency.") || MAREA_startsWith(text, "arenas.") || (text == "text") || MAREA_startsWith(text, "c.") };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::size_t literalCount{ 0 };

    for (const MAREA_ExampleFile &file : FILES) {
        MAREA_SemaFixture fixture(RESOLVE);

        ASSERT_EQ(fixture.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The example " << file.relativePath << " must check with zero errors.";

        const MAREA_SemaTypedNode *root{ nullptr };

        ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must build a typed tree.";

        MAREA_checkLiteralTypes(root, file.relativePath, literalCount);
    }

    EXPECT_GT(literalCount, std::size_t{ 0 }) << "The examples must hold literals.";
}
