// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

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
    GTEST_COUT << "Validating the dump of one structure and one function is exact." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; structure Point x: float64; end function f(a: integer32): integer32 local b: integer32 = a + 1; return b; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_lines({
                                         "MODULE",
                                         "  STRUCTURE_DECLARATION \"Point\"",
                                         "    FIELD_DECLARATION \"x\" type=float64",
                                         "  FUNCTION_DECLARATION \"f\" type=integer32",
                                         "    PARAMETER_LIST",
                                         "      PARAMETER \"a\" type=integer32",
                                         "    BLOCK",
                                         "      LOCAL_DECLARATION \"b\" type=integer32",
                                         "        BINARY_EXPRESSION op=PLUS",
                                         "          IDENTIFIER \"a\"",
                                         "          INTEGER_LITERAL \"1\"",
                                         "      RETURN_STATEMENT",
                                         "        IDENTIFIER \"b\"",
                                     }))
        << "The dump must match the tree line by line.";
}

TEST(MAREAIrDump, EmptyModuleIsOneLine)
{
    GTEST_COUT << "Validating an empty module dumps as a single MODULE line." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main;"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), "MODULE\n") << "An empty module must be one line.";
}

TEST(MAREAIrDump, FlagsAndOperatorsAreWritten)
{
    GTEST_COUT << "Validating the dump writes the mutable flag and the operator of a desugared loop." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run() for index in 0 .. 3 do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_lines({
                                         "MODULE",
                                         "  FUNCTION_DECLARATION \"run\"",
                                         "    PARAMETER_LIST",
                                         "    BLOCK",
                                         "      BLOCK",
                                         "        LOCAL_DECLARATION \"index\" flags=1",
                                         "          INTEGER_LITERAL \"0\"",
                                         "        LOCAL_DECLARATION \"$last\"",
                                         "          INTEGER_LITERAL \"3\"",
                                         "        WHILE_STATEMENT",
                                         "          BINARY_EXPRESSION op=LESS_EQUAL",
                                         "            IDENTIFIER \"index\"",
                                         "            IDENTIFIER \"$last\"",
                                         "          BLOCK",
                                         "            IF_STATEMENT",
                                         "              BINARY_EXPRESSION op=EQUAL_EQUAL",
                                         "                IDENTIFIER \"index\"",
                                         "                IDENTIFIER \"$last\"",
                                         "              BLOCK",
                                         "                BREAK_STATEMENT",
                                         "            ASSIGNMENT op=PLUS_EQUAL",
                                         "              IDENTIFIER \"index\"",
                                         "              INTEGER_LITERAL \"1\"",
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

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function f() local a: integer32 = 1 + 2 * 3; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

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

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; function run(values: mutable array<integer32>) for each key, value in mutable values do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_dumpOf(fixture), MAREA_lines({
                                         "MODULE",
                                         "  FUNCTION_DECLARATION \"run\"",
                                         "    PARAMETER_LIST",
                                         "      PARAMETER \"values\" type=array flags=1",
                                         "    BLOCK",
                                         "      FOR_EACH_STATEMENT \"value\" flags=1",
                                         "        IDENTIFIER \"key\"",
                                         "        IDENTIFIER \"values\"",
                                         "        BLOCK",
                                     }))
        << "The dump must show the name as text and the MUTABLE flag on the loop.";
}
