// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParseHelpers.hpp"

#include <utility>

namespace
{

    using MAREA_Case = std::pair<const char *, const char *>;

    void MAREA_expectCases(const std::vector<MAREA_Case> &cases)
    {
        for (const MAREA_Case &entry : cases) {
            MAREA_ParseFixture fixture;

            EXPECT_EQ(fixture.MAREA_statements(entry.first), entry.second) << "The statements '" << entry.first << "' must have this shape.";

            EXPECT_TRUE(fixture.MAREA_codes().empty()) << "The statements '" << entry.first << "' must give no diagnostic.";
        }
    }

} // namespace

TEST(MAREAParserStatements, LocalDeclarations)
{
    GTEST_COUT << "Validating local declarations with and without a value." << std::endl;

    MAREA_expectCases({
        { "local x: sinteger32;", "(local_declaration x (named_type sinteger32))" },
        { "local x: sinteger32 = 1;", "(local_declaration x (named_type sinteger32) 1)" },
    });
}

TEST(MAREAParserStatements, EveryAssignmentOperator)
{
    GTEST_COUT << "Validating every assignment operator." << std::endl;

    const std::vector<std::pair<std::string, std::string>> OPERATORS{
        { "=", "EQUAL" },
        { "+=", "PLUS_EQUAL" },
        { "-=", "MINUS_EQUAL" },
        { "*=", "STAR_EQUAL" },
        { "/=", "SLASH_EQUAL" },
        { "%=", "PERCENT_EQUAL" },
        { "**=", "STAR_STAR_EQUAL" },
        { "<<=", "SHIFT_LEFT_EQUAL" },
        { ">>=", "SHIFT_RIGHT_EQUAL" },
        { "&=", "AMPERSAND_EQUAL" },
        { "|=", "PIPE_EQUAL" },
        { "^=", "CARET_EQUAL" },
    };

    for (const std::pair<std::string, std::string> &entry : OPERATORS) {
        MAREA_ParseFixture fixture;

        EXPECT_EQ(fixture.MAREA_statements("a " + entry.first + " b;"), "(assignment:" + entry.second + " a b)") << "The operator " << entry.first << " must give an assignment with its token kind.";

        EXPECT_TRUE(fixture.MAREA_codes().empty()) << "The operator " << entry.first << " must give no diagnostic.";
    }

    MAREA_expectCases({
        { "p.x = 1;", "(assignment:EQUAL (member_access p x) 1)" },
        { "v[0] += 2;", "(assignment:PLUS_EQUAL (index_access v 0) 2)" },
    });
}

TEST(MAREAParserStatements, ExpressionAndMacroStatements)
{
    GTEST_COUT << "Validating an expression statement and a macro statement." << std::endl;

    MAREA_expectCases({
        { "f(1);", "(expression_statement (call f (argument_list 1)))" },
        { "print!(\"x\");", "(macro_statement (macro_call print (argument_list \"x\")))" },
    });
}

TEST(MAREAParserStatements, IfStatements)
{
    GTEST_COUT << "Validating if with elseif, else and nesting." << std::endl;

    MAREA_expectCases({
        { "if a then b(); end", "(if_statement a (block (expression_statement (call b (argument_list)))))" },
        { "if a then x = 1; elseif b then x = 2; else x = 3; end", "(if_statement a (block (assignment:EQUAL x 1)) (elseif_clause b (block (assignment:EQUAL x 2))) (else_clause (block (assignment:EQUAL x 3))))" },
        { "if a then if b then return; end end", "(if_statement a (block (if_statement b (block (return_statement)))))" },
    });
}

TEST(MAREAParserStatements, Loops)
{
    GTEST_COUT << "Validating while, for range and for each." << std::endl;

    MAREA_expectCases({
        { "while a do break; end", "(while_statement a (block (break_statement)))" },
        { "for i in 0 .. 10 do continue; end", "(for_range_statement i 0 10 (block (continue_statement)))" },
        { "for i in 0..10 do end", "(for_range_statement i 0 10 (block))" },
        { "for each x in v do end", "(for_each_statement x v (block))" },
        { "for each k, w in m do end", "(for_each_statement k w m (block))" },
        { "for each x in mutable v do end", "(for_each_statement#1 x v (block))" },
    });
}

TEST(MAREAParserStatements, DoWhile)
{
    GTEST_COUT << "Validating do ... while cond end builds a DO_WHILE_STATEMENT with the body block, then the condition." << std::endl;

    MAREA_expectCases({
        { "do x = 1; while a end", "(do_while_statement (block (assignment:EQUAL x 1)) a)" },
        { "do while a end", "(do_while_statement (block) a)" },
        { "do x = 1; y = 2; while a and b end", "(do_while_statement (block (assignment:EQUAL x 1) (assignment:EQUAL y 2)) (binary:KEYWORD_AND a b))" },
        { "do total += v[i]; i += 1; while i < v.size() end", "(do_while_statement (block (assignment:PLUS_EQUAL total (index_access v i)) (assignment:PLUS_EQUAL i 1)) (binary:LESS i (call (member_access v size) (argument_list))))" },
    });
}

TEST(MAREAParserStatements, DoWhileKeepsTheScopeBlock)
{
    GTEST_COUT << "Validating a plain do ... end is still a scope block, also with an ordinary while inside it." << std::endl;

    MAREA_expectCases({
        { "do x = 1; end", "(scope_block (block (assignment:EQUAL x 1)))" },
        { "do end", "(scope_block (block))" },
        { "do while a do break; end end", "(scope_block (block (while_statement a (block (break_statement)))))" },
        { "do x = 1; while a do end y = 2; end", "(scope_block (block (assignment:EQUAL x 1) (while_statement a (block)) (assignment:EQUAL y 2)))" },
    });
}

TEST(MAREAParserStatements, DoWhileNestsWithOrdinaryWhile)
{
    GTEST_COUT << "Validating an ordinary while inside a do-while body is told apart by the word after its condition." << std::endl;

    MAREA_expectCases({
        { "do while a do end while b end", "(do_while_statement (block (while_statement a (block))) b)" },
        { "do x = 1; while a do y = 2; end z = 3; while b end", "(do_while_statement (block (assignment:EQUAL x 1) (while_statement a (block (assignment:EQUAL y 2))) (assignment:EQUAL z 3)) b)" },
        { "do while a do while b do end end while c end", "(do_while_statement (block (while_statement a (block (while_statement b (block))))) c)" },
        { "while a do do x = 1; while b end end", "(while_statement a (block (do_while_statement (block (assignment:EQUAL x 1)) b)))" },
        { "do do x = 1; while a end while b end", "(do_while_statement (block (do_while_statement (block (assignment:EQUAL x 1)) a)) b)" },
        { "do if a then x = 1; end while b end", "(do_while_statement (block (if_statement a (block (assignment:EQUAL x 1)))) b)" },
    });
}

TEST(MAREAParserStatements, DoWhileWithJumps)
{
    GTEST_COUT << "Validating break, continue and return inside a do-while body." << std::endl;

    MAREA_expectCases({
        { "do if a then continue; end if b then break; end while c end", "(do_while_statement (block (if_statement a (block (continue_statement))) (if_statement b (block (break_statement)))) c)" },
        { "do return; while a end", "(do_while_statement (block (return_statement)) a)" },
    });
}

TEST(MAREAParserStatements, DoWhileWithoutEndIsReported)
{
    GTEST_COUT << "Validating a do-while with no closing end at the end of the file gives one UNTERMINATED_BLOCK." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; function f() do x = 1; while a"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK }) << "The missing end is one UNTERMINATED_BLOCK.";

    EXPECT_NE(fixture.MAREA_dump().find("DO_WHILE_STATEMENT"), std::string::npos) << "The statement must stay in the tree as a do-while.";
}

TEST(MAREAParserStatements, DoWhileWithoutConditionIsReported)
{
    GTEST_COUT << "Validating a do-while with no condition gives EXPECTED_EXPRESSION and the tree keeps the statement." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(fixture.MAREA_statements("do x = 1; while end"), "(do_while_statement (block (assignment:EQUAL x 1)) (error_node))") << "An error node must stand for the condition.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_EXPRESSION }) << "A missing condition is EXPECTED_EXPRESSION.";
}

TEST(MAREAParserStatements, DoWithoutWhileOrEndKeepsParsing)
{
    GTEST_COUT << "Validating a do whose body ends at a declaration word reports the missing end and keeps the next function." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; function f() do x = 1; function g() end"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_FALSE(fixture.MAREA_codes().empty()) << "The unclosed do must give a diagnostic.";

    EXPECT_NE(fixture.MAREA_dump().find("IDENTIFIER \"g\""), std::string::npos) << "The next function must still parse.";
}

TEST(MAREAParserStatements, DoWhileNestedDeeplyIsLimited)
{
    GTEST_COUT << "Validating a very deep do-while nesting gives RECURSION_TOO_DEEP and never crashes." << std::endl;

    std::string source{ "namespace main; function f() " };

    for (int level{ 0 }; level < 5000; ++level) {
        source += "do ";
    }

    for (int level{ 0 }; level < 5000; ++level) {
        source += "while a end ";
    }

    source += "end";

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse(source), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    const std::vector<MAREA_Uinteger32> CODES{ fixture.MAREA_codes() };

    ASSERT_FALSE(CODES.empty()) << "The depth limit must be reported.";

    EXPECT_EQ(CODES[0], static_cast<MAREA_Uinteger32>(MAREA_PARSE_DIAGNOSTIC_CODE_RECURSION_TOO_DEEP)) << "The first diagnostic must be RECURSION_TOO_DEEP.";
}

TEST(MAREAParserStatements, SwitchStatements)
{
    GTEST_COUT << "Validating switch with several values, a default, without a default and empty." << std::endl;

    MAREA_expectCases({
        { "switch a case 1, 2 then x = 1; default then x = 2; end", "(switch_statement a (switch_arm 1 2 (block (assignment:EQUAL x 1))) (default_arm (block (assignment:EQUAL x 2))))" },
        { "switch a case 1 then end", "(switch_statement a (switch_arm 1 (block)))" },
        { "switch a end", "(switch_statement a)" },
    });
}

TEST(MAREAParserStatements, WithAndPartition)
{
    GTEST_COUT << "Validating with and partition, plain and mutable." << std::endl;

    MAREA_expectCases({
        { "with t.map() as p do end", "(with_statement (call (member_access t map) (argument_list)) p (block))" },
        { "partition v into 4 as p do end", "(partition_statement v 4 p (block))" },
        { "partition mutable v into 4 as p do end", "(partition_statement#1 v 4 p (block))" },
    });
}

TEST(MAREAParserStatements, JumpsAndScopes)
{
    GTEST_COUT << "Validating return, break, continue and the three scope blocks." << std::endl;

    MAREA_expectCases({
        { "return;", "(return_statement)" },
        { "return 1 + 2;", "(return_statement (binary:PLUS 1 2))" },
        { "break;", "(break_statement)" },
        { "continue;", "(continue_statement)" },
        { "do x = 1; end", "(scope_block (block (assignment:EQUAL x 1)))" },
        { "unsafe end", "(unsafe_block (block))" },
        { "fork_join spawn f(); end", "(fork_join_block (block (expression_statement (unary:KEYWORD_SPAWN (call f (argument_list))))))" },
    });
}

TEST(MAREAParserStatements, BlockKeepsOrder)
{
    GTEST_COUT << "Validating a block of ten statements keeps their order." << std::endl;

    MAREA_ParseFixture fixture;

    std::string source;

    std::string expected;

    for (int index{ 0 }; index < 10; ++index) {
        const std::string NAME{ "f" + std::to_string(index) };

        source += NAME + "(); ";

        expected += (expected.empty() ? "" : " ") + std::string("(expression_statement (call ") + NAME + " (argument_list)))";
    }

    EXPECT_EQ(fixture.MAREA_statements(source), expected) << "Every statement must appear once, in source order.";
}

TEST(MAREAParserStatements, FunctionBodiesInsideImplementation)
{
    GTEST_COUT << "Validating a function with a body inside an implementation block." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; implementation Point function norm(self: Point): float32 return self.x; end end"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";

    EXPECT_NE(fixture.MAREA_dump().find("RETURN_STATEMENT"), std::string::npos) << "The body must hold the return statement.";
}

TEST(MAREAParserStatements, TopLevelMacroStatement)
{
    GTEST_COUT << "Validating a macro statement at the top level, with an attribute." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; #[a] static_assert!(size_of<layout.Light>() == 32, \"x\");"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_TRUE(fixture.MAREA_codes().empty()) << "No diagnostic is expected.";

    EXPECT_EQ(fixture.MAREA_itemsDump().compare(0, 18, "  MACRO_STATEMENT\n"), 0) << "The item must be a MACRO_STATEMENT.";

    EXPECT_NE(fixture.MAREA_itemsDump().find("    ATTRIBUTE\n"), std::string::npos) << "The attribute must lead the statement.";
}

TEST(MAREAParserStatements, MissingSemicolonKeepsParsing)
{
    GTEST_COUT << "Validating a missing semicolon does not hide the next statement." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(fixture.MAREA_statements("x = 1 y = 2;"), "(assignment:EQUAL x 1) (assignment:EQUAL y 2)") << "Both statements must be parsed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "One EXPECTED_TOKEN is expected.";
}

TEST(MAREAParserStatements, MissingThenAndDo)
{
    GTEST_COUT << "Validating a missing then and a missing do." << std::endl;

    {
        MAREA_ParseFixture fixture;

        EXPECT_EQ(fixture.MAREA_statements("if a x = 1; end"), "(if_statement a (block (assignment:EQUAL x 1)))") << "The body must still be parsed.";

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "One EXPECTED_TOKEN is expected.";
    }

    {
        MAREA_ParseFixture fixture;

        EXPECT_EQ(fixture.MAREA_statements("while a x = 1; end"), "(while_statement a (block (assignment:EQUAL x 1)))") << "The body must still be parsed.";

        EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN }) << "One EXPECTED_TOKEN is expected.";
    }
}

TEST(MAREAParserStatements, MissingEndAtEndOfFile)
{
    GTEST_COUT << "Validating a missing end of a function gives UNTERMINATED_BLOCK." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(fixture.MAREA_statements("while a do"), "(while_statement a (block))") << "The end of the wrapper closes the while.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_UNTERMINATED_BLOCK }) << "The function lacks its own end.";
}

TEST(MAREAParserStatements, CaseOutsideSwitchIsReported)
{
    GTEST_COUT << "Validating a case outside a switch gives EXPECTED_TOKEN for the missing end." << std::endl;

    MAREA_ParseFixture fixture;

    MAREA_IGNORE(fixture.MAREA_statements("case 1 then end"));

    ASSERT_FALSE(fixture.MAREA_codes().empty()) << "The case must be reported.";

    EXPECT_EQ(fixture.MAREA_codes()[0], static_cast<MAREA_Uinteger32>(MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN)) << "The first diagnostic must be EXPECTED_TOKEN.";
}

TEST(MAREAParserStatements, DefaultArmNotLastIsReported)
{
    GTEST_COUT << "Validating a switch arm after default gives DEFAULT_ARM_NOT_LAST." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(fixture.MAREA_statements("switch a default then x = 1; case 2 then y = 2; end"), "(switch_statement a (default_arm (block (assignment:EQUAL x 1))) (switch_arm 2 (block (assignment:EQUAL y 2))))") << "Both arms must still be parsed, in source order.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_DEFAULT_ARM_NOT_LAST }) << "One DEFAULT_ARM_NOT_LAST is expected.";
}

TEST(MAREAParserStatements, TwoDefaultArmsAreBothReportedNotLast)
{
    GTEST_COUT << "Validating a second default arm also gives DEFAULT_ARM_NOT_LAST." << std::endl;

    MAREA_ParseFixture fixture;

    EXPECT_EQ(fixture.MAREA_statements("switch a default then x = 1; default then y = 2; end"), "(switch_statement a (default_arm (block (assignment:EQUAL x 1))) (default_arm (block (assignment:EQUAL y 2))))") << "Both default arms must still be parsed.";

    EXPECT_EQ(fixture.MAREA_codes(), std::vector<MAREA_Uinteger32>{ MAREA_PARSE_DIAGNOSTIC_CODE_DEFAULT_ARM_NOT_LAST }) << "One DEFAULT_ARM_NOT_LAST is expected.";
}

TEST(MAREAParserStatements, SingleEndFormIsAccepted)
{
    GTEST_COUT << "Validating one closing end for a whole if or switch gives no diagnostic." << std::endl;

    const std::vector<const char *> sources = {
        "if a then x = 1; else x = 2; end",
        "if a then x = 1; elseif b then x = 2; end",
        "switch a case 1 then x = 1; default then x = 2; end",
        "if a then x = 1; end",
    };

    for (const char *source : sources) {
        MAREA_ParseFixture fixture;

        MAREA_IGNORE(fixture.MAREA_statements(source));

        EXPECT_TRUE(fixture.MAREA_codes().empty()) << "One closing end must be accepted: " << source;
    }
}

TEST(MAREAParserStatements, PerBranchEndFormIsReported)
{
    GTEST_COUT << "Validating an end after every branch or arm of an if or switch gives a diagnostic." << std::endl;

    const std::vector<const char *> sources = {
        "if a then x = 1; end else x = 2; end end",
        "if a then x = 1; end elseif b then x = 2; end end",
        "switch a case 1 then x = 1; end default then x = 2; end end",
    };

    for (const char *source : sources) {
        MAREA_ParseFixture fixture;

        MAREA_IGNORE(fixture.MAREA_statements(source));

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "An end after a branch or arm must be reported: " << source;
    }
}

TEST(MAREAParserStatements, ZeroEndFormIsReported)
{
    GTEST_COUT << "Validating an if or switch with no end of its own gives a diagnostic." << std::endl;

    const std::vector<const char *> sources = {
        "if a then x = 1; elseif b then x = 2;",
        "switch a case 1 then x = 1; default then x = 2;",
    };

    for (const char *source : sources) {
        MAREA_ParseFixture fixture;

        MAREA_IGNORE(fixture.MAREA_statements(source));

        EXPECT_FALSE(fixture.MAREA_codes().empty()) << "A statement with no end must be reported: " << source;
    }
}

TEST(MAREAParserStatements, UnfinishedIfDoesNotHideNextFunction)
{
    GTEST_COUT << "Validating an unfinished if does not hide the next function." << std::endl;

    MAREA_ParseFixture fixture;

    ASSERT_EQ(fixture.MAREA_parse("namespace main; function a() if x then local y: sinteger32 = 1; function b() end"), MAREA_PARSE_EXCEPT_SUCCESS) << "Parsing must succeed.";

    EXPECT_FALSE(fixture.MAREA_codes().empty()) << "The unfinished if must be reported.";

    EXPECT_NE(fixture.MAREA_dump().find("IDENTIFIER \"b\""), std::string::npos) << "The second function must still be parsed.";
}
