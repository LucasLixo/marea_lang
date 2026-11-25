// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

#include "marea/internal/ir/MAREAIrInternal.h"

#if defined(__linux__)
    #include <pthread.h>
#endif

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return (text.compare(0, 12, "collections.") == 0) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::string MAREA_dumpOf(MAREA_IrFixture &fixture)
    {
        const MAREA_Character8 *text{ nullptr };

        MAREA_IntegerSize length{ 0 };

        if (MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length) != MAREA_IR_EXCEPT_SUCCESS) {
            return "<dump failed>";
        }

        return std::string(text, length);
    }
} // namespace

TEST(MAREAIrDump, SmallModuleHasAnExactText)
{
    GTEST_COUT << "Validating the dump of one structure and one function is exact, tree and layout table." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: float64; end function f(a: sinteger32): sinteger32 local b: sinteger32 = a + 1; return b; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_lines({
                                         "MODULE \"main\"",
                                         "  STRUCTURE_DECLARATION \"Point\"",
                                         "    FIELD_DECLARATION \"x\" type=float64 resolved=float64",
                                         "  FUNCTION_DECLARATION \"f\" type=sinteger32 resolved=sinteger32",
                                         "    PARAMETER_LIST",
                                         "      PARAMETER \"a\" type=sinteger32 resolved=sinteger32",
                                         "    BLOCK",
                                         "      LOCAL_DECLARATION \"b\" type=sinteger32 resolved=sinteger32",
                                         "        BINARY_EXPRESSION resolved=sinteger32 op=PLUS",
                                         "          IDENTIFIER \"a\" resolved=sinteger32",
                                         "          INTEGER_LITERAL \"1\" resolved=sinteger32",
                                         "      RETURN_STATEMENT",
                                         "        IDENTIFIER \"b\" resolved=sinteger32",
                                         "LAYOUT_TABLE",
                                         "  STRUCTURE \"Point\" verdict=measured size=8 alignment=8 members=1 offsets=0",
                                     }))
        << "The dump must match the tree line by line, then the layout table.";
}

TEST(MAREAIrDump, EmptyModuleIsOneLine)
{
    GTEST_COUT << "Validating an empty module dumps as a single MODULE line." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), "MODULE \"main\"\n") << "An empty module must be one line.";
}

TEST(MAREAIrDump, FlagsAndOperatorsAreWritten)
{
    GTEST_COUT << "Validating the dump writes the mutable flag and the operator of a desugared loop." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run() for index in 0 .. 3 do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_lines({
                                         "MODULE \"main\"",
                                         "  FUNCTION_DECLARATION \"run\"",
                                         "    PARAMETER_LIST",
                                         "    BLOCK",
                                         "      BLOCK",
                                         "        LOCAL_DECLARATION \"index\" type=sinteger32 resolved=sinteger32 flags=1",
                                         "          INTEGER_LITERAL \"0\" resolved=sinteger32",
                                         "        LOCAL_DECLARATION \"$last0\" type=sinteger32 resolved=sinteger32",
                                         "          INTEGER_LITERAL \"3\" resolved=sinteger32",
                                         "        WHILE_STATEMENT",
                                         "          BINARY_EXPRESSION resolved=boolean op=LESS_EQUAL",
                                         "            IDENTIFIER \"index\" resolved=sinteger32",
                                         "            IDENTIFIER \"$last0\" resolved=sinteger32",
                                         "          BLOCK",
                                         "            IF_STATEMENT",
                                         "              BINARY_EXPRESSION resolved=boolean op=EQUAL_EQUAL",
                                         "                IDENTIFIER \"index\" resolved=sinteger32",
                                         "                IDENTIFIER \"$last0\" resolved=sinteger32",
                                         "              BLOCK",
                                         "                BREAK_STATEMENT",
                                         "            ASSIGNMENT op=PLUS_EQUAL",
                                         "              IDENTIFIER \"index\" resolved=sinteger32",
                                         "              INTEGER_LITERAL \"1\" resolved=sinteger32",
                                     }))
        << "The desugared loop must show its flag and operators.";
}

TEST(MAREAIrDump, StringTextIsEscaped)
{
    GTEST_COUT << "Validating a string literal with a quote and a backslash is escaped in the dump." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant S: string = \"a\\\\b\";"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_NE(MAREA_dumpOf(fixture).find("STRING_LITERAL \"\\\"a\\\\\\\\b\\\"\""), std::string::npos) << "Quotes and backslashes of the text must be escaped: " << MAREA_dumpOf(fixture);
}

TEST(MAREAIrDump, CharacterTextIsEscaped)
{
    GTEST_COUT << "Validating a character literal is dumped with its single quotes and an escaped backslash." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; constant C: character = '\\n';"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_NE(MAREA_dumpOf(fixture).find("CHARACTER_LITERAL \"'\\\\n'\""), std::string::npos) << "The text must keep its quotes and escape the backslash: " << MAREA_dumpOf(fixture);
}

TEST(MAREAIrDump, QualifiedImplementationNamesAreWritten)
{
    GTEST_COUT << "Validating an implementation member is dumped under its qualified name." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: float64; end implementation Point function get(self: Point): float64 return self.x; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_NE(MAREA_dumpOf(fixture).find("FUNCTION_DECLARATION \"Point.get\" type=float64"), std::string::npos) << "The member must be named Point.get.";
}

TEST(MAREAIrDump, DumpIsStable)
{
    GTEST_COUT << "Validating two dumps of the same module are byte-identical." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f() local a: sinteger32 = 1 + 2 * 3; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_dumpOf(fixture)) << "The text form must be stable.";
}

TEST(MAREAIrDump, OutputIsNulTerminated)
{
    GTEST_COUT << "Validating the dump text ends with a NUL after its reported length." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    ASSERT_EQ(MAREA_IrModuleDump(fixture.sema.parser.lexer.arena, fixture.module, &text, &length), MAREA_IR_EXCEPT_SUCCESS) << "The dump must succeed.";

    EXPECT_EQ(text[length], '\0') << "The text must end with a NUL.";
}

TEST(MAREAIrDump, NullArgumentsAreRejected)
{
    GTEST_COUT << "Validating MAREA_IrModuleDump rejects null arguments." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    MAREA_Arena *arena{ fixture.sema.parser.lexer.arena };

    const MAREA_Character8 *text{ nullptr };

    MAREA_IntegerSize length{ 0 };

    EXPECT_EQ(MAREA_IrModuleDump(nullptr, fixture.module, &text, &length), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null arena must be rejected.";

    EXPECT_EQ(MAREA_IrModuleDump(arena, nullptr, &text, &length), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null module must be rejected.";

    EXPECT_EQ(MAREA_IrModuleDump(arena, fixture.module, nullptr, &length), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null text pointer must be rejected.";

    EXPECT_EQ(MAREA_IrModuleDump(arena, fixture.module, &text, nullptr), MAREA_IR_EXCEPT_ERROR_NULL_POINTER) << "A null length pointer must be rejected.";
}

TEST(MAREAIrDump, ForEachDumpsItsNameFlagAndChildren)
{
    GTEST_COUT << "Validating the dump of a mutable for-each loop with a key shows the value name, the flag and the three children." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.list as list; function run(#[semantics = reference_mutable] values: list<sinteger32>) for each key, value in mutable values do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_lines({
                                         "MODULE \"main\"",
                                         "  FUNCTION_DECLARATION \"run\"",
                                         "    PARAMETER_LIST",
                                         "      PARAMETER \"values\" type=collections.list resolved=list<sinteger32> flags=1",
                                         "    BLOCK",
                                         "      FOR_EACH_STATEMENT \"value\" flags=1",
                                         "        IDENTIFIER \"key\"",
                                         "        IDENTIFIER \"values\" resolved=list<sinteger32>",
                                         "        BLOCK",
                                     }))
        << "The dump must show the name as text and the MUTABLE flag on the loop.";
}

#if defined(__linux__)
namespace
{
    constexpr MAREA_IntegerSize DEEP_CHAIN_DEPTH{ 4000 };
    constexpr std::size_t SMALL_STACK_BYTES{ 128U * 1024U };

    struct MAREA_DeepDumpJob
    {
        MAREA_Arena *arena{ nullptr };
        const MAREA_IrNode *root{ nullptr };
        const MAREA_Character8 *text{ nullptr };
        MAREA_IntegerSize length{ 0 };
        MAREA_IrExcept result{ MAREA_IR_EXCEPT_ERROR_NULL_POINTER };
    };

    void *MAREA_deepDumpEntry(void *argument)
    {
        auto *job = static_cast<MAREA_DeepDumpJob *>(argument);

        job->result = MAREA_IrModuleDump(job->arena, job->root, &job->text, &job->length);

        return nullptr;
    }
} // namespace

TEST(MAREAIrDump, DeepChainDumpsOnASmallStack)
{
    GTEST_COUT << "Validating a deep left-deep chain dumps on a stack too small for one call frame per level." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    MAREA_IrNode *root{ nullptr };

    ASSERT_EQ(MAREA_IrNodeCreate(fixture.context, MAREA_IR_KIND_MODULE, fixture.sema.parser.root, &root), MAREA_IR_EXCEPT_SUCCESS) << "The root must be created.";

    MAREA_IrNode *parent{ root };

    for (MAREA_IntegerSize level{ 0 }; level < DEEP_CHAIN_DEPTH; ++level) {
        MAREA_IrNode *child{ nullptr };

        ASSERT_EQ(MAREA_IrNodeCreate(fixture.context, MAREA_IR_KIND_BLOCK, fixture.sema.parser.root, &child), MAREA_IR_EXCEPT_SUCCESS) << "A chain node must be created.";

        ASSERT_EQ(MAREA_IrNodeAppendChild(fixture.context, parent, child), MAREA_IR_EXCEPT_SUCCESS) << "A chain node must be appended.";

        parent = child;
    }

    MAREA_DeepDumpJob job;

    job.arena = fixture.sema.parser.lexer.arena;
    job.root = root;

    pthread_attr_t attributes;

    ASSERT_EQ(pthread_attr_init(&attributes), 0) << "The thread attributes must initialize.";
    ASSERT_EQ(pthread_attr_setstacksize(&attributes, SMALL_STACK_BYTES), 0) << "The small stack must be accepted.";

    pthread_t thread;

    ASSERT_EQ(pthread_create(&thread, &attributes, MAREA_deepDumpEntry, &job), 0) << "The dump thread must start.";
    ASSERT_EQ(pthread_join(thread, nullptr), 0) << "The dump thread must finish.";

    pthread_attr_destroy(&attributes);

    ASSERT_EQ(job.result, MAREA_IR_EXCEPT_SUCCESS) << "The deep dump must succeed.";

    const std::string TEXT(job.text, job.length);

    EXPECT_EQ(static_cast<MAREA_IntegerSize>(std::count(TEXT.begin(), TEXT.end(), '\n')), (DEEP_CHAIN_DEPTH + 1)) << "The dump must have one line per node.";

    const std::string LAST_LINE{ std::string(2U * DEEP_CHAIN_DEPTH, ' ') + "BLOCK\n" };

    ASSERT_GE(TEXT.size(), LAST_LINE.size()) << "The dump must hold the deepest line.";

    EXPECT_EQ(TEXT.substr(TEXT.size() - LAST_LINE.size()), LAST_LINE) << "The deepest line must be indented by two spaces per level.";
}
#endif
