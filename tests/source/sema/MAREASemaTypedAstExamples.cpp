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

    // The closed list of the classes of expression node that may stay with no resolved type (see "Gaps" of the sema doc). Every other untyped expression node fails the sweep.
    enum MAREA_UntypedClass
    {
        MAREA_UNTYPED_CLASS_MACRO_SUBTREE,
        MAREA_UNTYPED_CLASS_QUALIFYING_TYPE_NAME,
        MAREA_UNTYPED_CLASS_BUILTIN_CALLEE_NAME,
        MAREA_UNTYPED_CLASS_UNTYPED_FUNCTION_NAME,
        MAREA_UNTYPED_CLASS_UNTYPED_FUNCTION_MEMBER,
        MAREA_UNTYPED_CLASS_UNTYPED_CALL,
        MAREA_UNTYPED_CLASS_UNTYPED_LOOP_NAME,
        MAREA_UNTYPED_CLASS_DERIVED_OPERATION,
        MAREA_UNTYPED_CLASS_SPAWN_OF_VOID_CALL,
        MAREA_UNTYPED_CLASS_COUNT
    };

    struct MAREA_UntypedClassInfo
    {
        const char *name;
        const char *reason;
    };

    const MAREA_UntypedClassInfo MAREA_UNTYPED_CLASS_INFO[MAREA_UNTYPED_CLASS_COUNT] = {
        { "MACRO_SUBTREE", "a MACRO_CALL and every node below it: the ir raises 4005 for a macro, and the arguments of a macro are never read for a type" },
        { "QUALIFYING_TYPE_NAME", "a GENERIC_NAME on the left of a member access (c.array<T>.filled): a written type that qualifies a static member, not a value" },
        { "BUILTIN_CALLEE_NAME", "an IDENTIFIER named size_of, align_of or convert that is the callee of a call: the call has the type, the name has none" },
        { "UNTYPED_FUNCTION_NAME", "an IDENTIFIER that is the callee of a call and has no function type: power and square_root (imported by name from an EXTERNAL namespace with no signature) and maximum (a generic function)" },
        { "UNTYPED_FUNCTION_MEMBER", "a MEMBER_ACCESS that is the callee of a call: a function or a method of an EXTERNAL namespace or of a library type, or of a generic-parameter receiver, that has no function type (the call has its own type or none)" },
        { "UNTYPED_CALL", "a CALL_EXPRESSION whose callee has no type: nothing gives the result (an EXTERNAL function with no signature, or a library member the signature function does not know)" },
        { "UNTYPED_LOOP_NAME", "an IDENTIFIER pixels or index: the name of a with or for statement whose deciding expression has no type, read as a value" },
        { "DERIVED_OPERATION", "a BINARY_EXPRESSION or INDEX_ACCESS that has an operand with no type, as the root cause is already counted on its own" },
        { "SPAWN_OF_VOID_CALL", "a UNARY_EXPRESSION whose operand is a call typed void: spawn of such a call keeps no type" },
    };

    struct MAREA_UntypedSite
    {
        MAREA_AstKind kind;
        MAREA_AstKind parentKind;
        MAREA_IntegerSize index;
        bool insideMacro;
        std::string name;
    };

    struct MAREA_ExpressionTally
    {
        std::size_t expressions{ 0 };
        std::size_t qualifiers{ 0 };
        std::size_t classCount[MAREA_UNTYPED_CLASS_COUNT]{};
        std::vector<std::string> unclassified;
    };

    bool MAREA_nodeHasType(const MAREA_SemaTypedNode *node)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        return MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    std::string MAREA_nodeTypeText(const MAREA_SemaTypedNode *node)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_SemaTypedNodeGetResolvedType(node, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return std::string();
        }

        return std::string(text, length);
    }

    const MAREA_SemaTypedNode *MAREA_childAt(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index)
    {
        const MAREA_SemaTypedNode *child{ nullptr };

        if (MAREA_SemaTypedNodeGetChildAt(node, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return nullptr;
        }

        return child;
    }

    bool MAREA_isOneOf(const std::string &name, const std::vector<std::string> &names)
    {
        return std::find(names.begin(), names.end(), name) != names.end();
    }

    // Gives the class of an expression node that has no resolved type, or MAREA_UNTYPED_CLASS_COUNT when it is in none.
    MAREA_UntypedClass MAREA_classifyUntyped(const MAREA_SemaTypedNode *node, const MAREA_UntypedSite &site)
    {
        const bool IS_CALLEE{ (site.parentKind == MAREA_AST_KIND_CALL_EXPRESSION) && (site.index == 0) };

        if ((site.kind == MAREA_AST_KIND_MACRO_CALL) || site.insideMacro) {
            return MAREA_UNTYPED_CLASS_MACRO_SUBTREE;
        }

        switch (site.kind) {
        case MAREA_AST_KIND_GENERIC_NAME:
            if ((site.parentKind == MAREA_AST_KIND_MEMBER_ACCESS) && (site.index == 0)) {
                return MAREA_UNTYPED_CLASS_QUALIFYING_TYPE_NAME;
            }

            break;
        case MAREA_AST_KIND_IDENTIFIER:
            if (IS_CALLEE && MAREA_isOneOf(site.name, { "size_of", "align_of", "convert" })) {
                return MAREA_UNTYPED_CLASS_BUILTIN_CALLEE_NAME;
            }

            if (IS_CALLEE && MAREA_isOneOf(site.name, { "power", "square_root", "maximum" })) {
                return MAREA_UNTYPED_CLASS_UNTYPED_FUNCTION_NAME;
            }

            if (!IS_CALLEE && MAREA_isOneOf(site.name, { "pixels", "index" })) {
                return MAREA_UNTYPED_CLASS_UNTYPED_LOOP_NAME;
            }

            break;
        case MAREA_AST_KIND_MEMBER_ACCESS:
            if (IS_CALLEE) {
                return MAREA_UNTYPED_CLASS_UNTYPED_FUNCTION_MEMBER;
            }

            break;
        case MAREA_AST_KIND_CALL_EXPRESSION:
        {
            const MAREA_SemaTypedNode *callee{ MAREA_childAt(node, 0) };

            if ((callee != nullptr) && !MAREA_nodeHasType(callee)) {
                return MAREA_UNTYPED_CLASS_UNTYPED_CALL;
            }

            break;
        }
        case MAREA_AST_KIND_BINARY_EXPRESSION:
        case MAREA_AST_KIND_INDEX_ACCESS:
        {
            const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

            for (MAREA_IntegerSize child{ 0 }; child < COUNT; ++child) {
                const MAREA_SemaTypedNode *operand{ MAREA_childAt(node, child) };

                if ((operand != nullptr) && !MAREA_nodeHasType(operand)) {
                    return MAREA_UNTYPED_CLASS_DERIVED_OPERATION;
                }
            }

            break;
        }
        case MAREA_AST_KIND_UNARY_EXPRESSION:
        {
            const MAREA_SemaTypedNode *operand{ MAREA_childAt(node, 0) };

            if ((operand != nullptr) && (MAREA_nodeTypeText(operand) == "void")) {
                return MAREA_UNTYPED_CLASS_SPAWN_OF_VOID_CALL;
            }

            break;
        }
        default:
            break;
        }

        return MAREA_UNTYPED_CLASS_COUNT;
    }

    void MAREA_tallyExpressions(const MAREA_SemaTypedNode *node, MAREA_AstKind parentKind, MAREA_IntegerSize index, MAREA_IntegerSize siblings, bool insideMacro, const std::string &file, MAREA_ExpressionTally &tally)
    {
        const MAREA_AstNode *ast{ MAREA_SemaTypedNodeGetAstNode(node) };

        const MAREA_AstKind KIND{ MAREA_AstNodeGetKind(ast) };

        const bool IS_VALUE_IDENTIFIER{ (KIND == MAREA_AST_KIND_IDENTIFIER) && MAREA_identifierIsValue(parentKind, index, siblings) };

        // A name that only qualifies a member is not a value: it has no type and is counted on its own.
        if (MAREA_SemaTypedNodeIsQualifier(node)) {
            ++tally.qualifiers;

            const MAREA_Character8 *qualifierText{ nullptr };

            MAREA_IntegerSize qualifierLength{ 0 };

            EXPECT_EQ(MAREA_SemaTypedNodeGetResolvedType(node, &qualifierText, &qualifierLength), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NO_TYPE) << "A qualifier has no resolved type.";
        } else if (MAREA_isLiteralKind(KIND) || MAREA_isOperationKind(KIND) || IS_VALUE_IDENTIFIER) {
            ++tally.expressions;

            if (!MAREA_nodeHasType(node)) {
                const MAREA_Character8 *nameText{ MAREA_AstNodeGetText(ast) };

                const MAREA_SourcePosition POSITION{ MAREA_AstNodeGetPosition(ast) };

                const MAREA_UntypedSite SITE{ KIND, parentKind, index, insideMacro, (nameText != nullptr) ? std::string(nameText, MAREA_AstNodeGetTextLength(ast)) : std::string() };

                const MAREA_UntypedClass CLASS{ MAREA_classifyUntyped(node, SITE) };

                if (CLASS == MAREA_UNTYPED_CLASS_COUNT) {
                    tally.unclassified.push_back(file + ":" + std::to_string(POSITION.line) + ":" + std::to_string(POSITION.column) + " " + MAREA_AstKindGetName(KIND) + " " + (SITE.name.empty() ? std::string("-") : SITE.name) + " has no resolved type and is in no allowed class");
                } else {
                    ++tally.classCount[CLASS];
                }
            }
        }

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        for (MAREA_IntegerSize child{ 0 }; child < COUNT; ++child) {
            const MAREA_SemaTypedNode *typedChild{ nullptr };

            if (MAREA_SemaTypedNodeGetChildAt(node, child, &typedChild) == MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
                MAREA_tallyExpressions(typedChild, KIND, child, COUNT, insideMacro || (KIND == MAREA_AST_KIND_MACRO_CALL), file, tally);
            }
        }
    }

    // The fixture signatures of the library members the examples use, keyed by the canonical "c.<library>" namespace.
    const MAREA_SemaSignatureParameter MAREA_FIXTURE_FILLED_PARAMETERS[1] = { { "float32", 7 } };
    const MAREA_SemaSignature MAREA_FIXTURE_SELF_SIGNATURE{ MAREA_FIXTURE_FILLED_PARAMETERS, 1, "Self", 4 };
    const MAREA_SemaSignature MAREA_FIXTURE_ITEM_SIGNATURE{ nullptr, 0, "Item", 4 };
    const MAREA_SemaSignature MAREA_FIXTURE_VOID_SIGNATURE{ MAREA_FIXTURE_FILLED_PARAMETERS, 1, "void", 4 };

    void MAREA_CALL MAREA_fixtureSignatureThunk(MAREA_Null, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature)
    {
        const std::string LIBRARY(namespaceText, namespaceLength);

        const std::string MEMBER(memberText, memberLength);

        *outSignature = nullptr;

        *outResult = MAREA_SEMA_IMPORT_RESULT_FOUND;

        if ((LIBRARY == "c.array") && (MEMBER == "filled")) {
            *outSignature = &MAREA_FIXTURE_SELF_SIGNATURE;
        } else if ((LIBRARY == "c.array") && (MEMBER == "append")) {
            *outSignature = &MAREA_FIXTURE_VOID_SIGNATURE;
        } else if ((LIBRARY == "c.array") && (MEMBER == "sum")) {
            *outSignature = &MAREA_FIXTURE_ITEM_SIGNATURE;
        } else if ((LIBRARY == "c.task") && (MEMBER == "result")) {
            *outSignature = &MAREA_FIXTURE_ITEM_SIGNATURE;
        } else {
            *outResult = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
        }
    }
} // namespace

TEST(MAREASemaTypedAstExamples, EveryUntypedExpressionOfEveryExampleIsInAnAllowedClass)
{
    GTEST_COUT << "Validating every expression node of the 23 examples has a resolved type or is in the closed list of the classes allowed to have none." << std::endl;

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

        ASSERT_EQ(MAREA_SemaImportResolverSetSignatureFunction(fixture.resolver, &MAREA_fixtureSignatureThunk), MAREA_SEMA_EXCEPT_SUCCESS);

        ASSERT_EQ(fixture.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The example " << file.relativePath << " must check with zero errors.";

        const MAREA_SemaTypedNode *root{ nullptr };

        ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must build a typed tree.";

        MAREA_tallyExpressions(root, MAREA_AST_KIND_COMPILATION_UNIT, 0, 1, false, file.relativePath, total);
    }

    std::size_t untyped{ 0 };

    for (std::size_t index{ 0 }; index < std::size_t{ MAREA_UNTYPED_CLASS_COUNT }; ++index) {
        untyped += total.classCount[index];

        GTEST_COUT << "Allowed class " << MAREA_UNTYPED_CLASS_INFO[index].name << " = " << total.classCount[index] << ": " << MAREA_UNTYPED_CLASS_INFO[index].reason << std::endl;
    }

    GTEST_COUT << "Examples: " << FILES.size() << ", expression nodes: " << total.expressions << ", qualifier names (not values): " << total.qualifiers << ", with no resolved type: " << (untyped + total.unclassified.size()) << " (in an allowed class: " << untyped << ", in none: " << total.unclassified.size() << ")." << std::endl;

    for (const std::string &line : total.unclassified) {
        ADD_FAILURE() << line;
    }

    EXPECT_EQ(FILES.size(), std::size_t{ 23 }) << "The sweep must cover the 23 examples.";

    // A class that no example reaches any more is stale: the list stays exact, so a class that a fix emptied is removed from the list and from "Gaps" of the sema doc.
    for (std::size_t index{ 0 }; index < std::size_t{ MAREA_UNTYPED_CLASS_COUNT }; ++index) {
        EXPECT_GT(total.classCount[index], std::size_t{ 0 }) << "The allowed class " << MAREA_UNTYPED_CLASS_INFO[index].name << " matches no node of the examples.";
    }

    // The names that only qualify a member are not counted above, so a qualifier that stopped being one would raise the count above or fall here.
    EXPECT_GE(total.qualifiers, std::size_t{ 36 }) << "The examples hold at least 36 names that only qualify a member.";
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

namespace
{
    struct MAREA_LoopTally
    {
        std::size_t loops{ 0 };
        std::size_t namesTyped{ 0 };
        std::vector<std::string> namesUntyped;
        std::vector<std::string> collectionsUntyped;
    };

    // Counts the declared names of every for range and for each, and the collection of a for each, that carry a resolved type.
    void MAREA_tallyLoops(const MAREA_SemaTypedNode *node, const std::string &path, MAREA_LoopTally &tally)
    {
        if (node == nullptr) {
            return;
        }

        const MAREA_AstKind KIND{ MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(node)) };

        const MAREA_IntegerSize COUNT{ MAREA_SemaTypedNodeGetChildCount(node) };

        if ((KIND == MAREA_AST_KIND_FOR_RANGE_STATEMENT) || (KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT)) {
            ++tally.loops;

            // A for range has one name, a for each has one or two names, then the collection and the body.
            const MAREA_IntegerSize NAMES{ (KIND == MAREA_AST_KIND_FOR_RANGE_STATEMENT) ? MAREA_INTEGER_SIZE(1) : (COUNT - MAREA_INTEGER_SIZE(2)) };

            for (MAREA_IntegerSize index{ 0 }; index < NAMES; ++index) {
                if (MAREA_nodeHasType(MAREA_childAt(node, index))) {
                    ++tally.namesTyped;
                } else {
                    tally.namesUntyped.push_back(path + " name " + std::to_string(index));
                }
            }

            if ((KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT) && (MAREA_nodeHasType(MAREA_childAt(node, NAMES)) == false)) {
                tally.collectionsUntyped.push_back(path);
            }
        }

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            MAREA_tallyLoops(MAREA_childAt(node, index), path, tally);
        }
    }
} // namespace

TEST(MAREASemaTypedAstExamples, EveryLoopNameOfEveryExampleHasATypeUnlessItsHeaderHasNone)
{
    GTEST_COUT << "Validating the declared names and the collection of every for range and for each of every example carry a resolved type." << std::endl;

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

    MAREA_LoopTally tally;

    for (const MAREA_ExampleFile &file : FILES) {
        MAREA_SemaFixture fixture(RESOLVE);

        ASSERT_EQ(fixture.MAREA_sema(file.source, file.relativePath), MAREA_SEMA_EXCEPT_SUCCESS) << "The check of " << file.relativePath << " must succeed.";

        ASSERT_EQ(fixture.MAREA_errorCount(), MAREA_INTEGER_SIZE(0)) << "The example " << file.relativePath << " must check with zero errors.";

        const MAREA_SemaTypedNode *root{ nullptr };

        ASSERT_EQ(MAREA_SemaBuildTypedAst(fixture.context, fixture.parser.root, &root), MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) << "The example " << file.relativePath << " must build a typed tree.";

        MAREA_tallyLoops(root, file.relativePath, tally);
    }

    GTEST_COUT << "Loops: " << tally.loops << ", names with a type: " << tally.namesTyped << ", names with none: " << tally.namesUntyped.size() << ", collections with none: " << tally.collectionsUntyped.size() << "." << std::endl;

    for (const std::string &line : tally.namesUntyped) {
        GTEST_COUT << "No type: " << line << std::endl;
    }

    EXPECT_GE(tally.loops, std::size_t{ 8 }) << "The examples hold at least 8 loops of these two kinds.";

    EXPECT_TRUE(tally.collectionsUntyped.empty()) << "Every collection of a for each of the examples has a resolved type.";

    // The one range counter with no type is the one of gpu_handles, whose last bound is a call of an EXTERNAL function with no signature.
    ASSERT_EQ(tally.namesUntyped.size(), std::size_t{ 1 }) << "Only the counter of the range with an untyped bound has no type.";

    EXPECT_NE(tally.namesUntyped[0].find("gpu_handles"), std::string::npos) << "The one name with no type is in gpu_handles.";
}
