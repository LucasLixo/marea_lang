// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return ((text.compare(0, 12, "collections.") == 0) || ((text.compare(0, 2, "c.") == 0) || (text.compare(0, 6, "marea.") == 0) || (text.compare(0, 5, "demo.") == 0))) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::string MAREA_lowerBody(const std::string &prefix, const std::string &signature, const std::string &body, const std::string &result = "")
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower("namespace main; " + prefix + " function run(" + signature + ")" + result + " " + body + " end") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((function == nullptr) || (MAREA_IrNodeGetChildCount(function) < 2)) {
            return "<no body>";
        }

        return MAREA_irRender(MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1));
    }

    std::size_t MAREA_countOf(const std::string &text, const std::string &part)
    {
        std::size_t count{ 0 };

        for (std::size_t at{ text.find(part) }; at != std::string::npos; at = text.find(part, at + 1)) {
            ++count;
        }

        return count;
    }
} // namespace

TEST(MAREAIrControlFlow, EmptyBodyIsAnEmptyBlock)
{
    GTEST_COUT << "Validating an empty function body lowers to an empty BLOCK." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", ""), "(BLOCK)") << "An empty body must be an empty BLOCK.";
}

TEST(MAREAIrControlFlow, LocalDeclarationKeepsNameTypeNameAndInitializer)
{
    GTEST_COUT << "Validating a local declaration keeps its name, type name and lowered initializer." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "local total: sinteger32 = 1 + 2;"), "(BLOCK (LOCAL_DECLARATION total :sinteger32 (BINARY_EXPRESSION op=PLUS (INTEGER_LITERAL 1) (INTEGER_LITERAL 2))))") << "The local must keep its shape.";
}

TEST(MAREAIrControlFlow, AssignmentKeepsOperatorAndOperands)
{
    GTEST_COUT << "Validating an assignment keeps its operator, target and value." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "value: sinteger32", "local total: sinteger32 = 0; total += value;"), "(BLOCK (LOCAL_DECLARATION total :sinteger32 (INTEGER_LITERAL 0)) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER total) (IDENTIFIER value)))") << "A compound assignment must keep its operator.";
}

TEST(MAREAIrControlFlow, ExpressionStatementKeepsItsExpression)
{
    GTEST_COUT << "Validating an expression statement holds its lowered call." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function ping() end", "", "ping();"), "(BLOCK (EXPRESSION_STATEMENT (CALL_EXPRESSION (IDENTIFIER ping) (ARGUMENT_LIST))))") << "The call must be the only child.";
}

TEST(MAREAIrControlFlow, IfKeepsConditionBlockElseifAndElse)
{
    GTEST_COUT << "Validating an if statement keeps its condition, block, elseif clause and else clause." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "if a < 1 then return; elseif a < 2 then return; else return; end"), "(BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER a) (INTEGER_LITERAL 1)) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=LESS (IDENTIFIER a) (INTEGER_LITERAL 2)) (BLOCK (RETURN_STATEMENT))) (ELSE_CLAUSE (BLOCK (RETURN_STATEMENT)))))") << "The if shape must match the source.";
}

TEST(MAREAIrControlFlow, WhileKeepsConditionAndBlock)
{
    GTEST_COUT << "Validating a while statement keeps its condition and block, with break and continue." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: boolean", "while a do if a then break; end continue; end"), "(BLOCK (WHILE_STATEMENT (IDENTIFIER a) (BLOCK (IF_STATEMENT (IDENTIFIER a) (BLOCK (BREAK_STATEMENT))) (CONTINUE_STATEMENT))))") << "The while shape must match the source.";
}

TEST(MAREAIrControlFlow, ReturnKeepsItsValue)
{
    GTEST_COUT << "Validating a return keeps its lowered value when it has one." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function one(): sinteger32 return 1; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(MAREA_irChild(fixture.module, 0), 1)), "(BLOCK (RETURN_STATEMENT (INTEGER_LITERAL 1)))") << "The return must hold its value.";
}

TEST(MAREAIrControlFlow, ScopeBlockKeepsItsKindAndBlock)
{
    GTEST_COUT << "Validating a do-end scope lowers to a SCOPE_BLOCK that holds a BLOCK." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "do local inner: sinteger32 = 1; end"), "(BLOCK (SCOPE_BLOCK (BLOCK (LOCAL_DECLARATION inner :sinteger32 (INTEGER_LITERAL 1)))))") << "The scope must keep its block.";
}

TEST(MAREAIrControlFlow, UnsafeBlockLowersToAPlainBlock)
{
    GTEST_COUT << "Validating an unsafe block lowers to a plain BLOCK." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "unsafe local raw: sinteger32 = 1; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION raw :sinteger32 (INTEGER_LITERAL 1))))") << "The unsafe wrapper must leave no node.";
}

TEST(MAREAIrControlFlow, UnreachableMacroBecomesAnUnreachableStatement)
{
    GTEST_COUT << "Validating unreachable!() lowers to an UNREACHABLE_STATEMENT with no children." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "unreachable!();"), "(BLOCK (UNREACHABLE_STATEMENT))") << "The macro must lower to its own kind.";
}

TEST(MAREAIrControlFlow, ForRangeDesugarsToABlockWithAWhileAndAnIncrement)
{
    GTEST_COUT << "Validating a for-range loop desugars to a hidden local, a while with an inclusive bound and an increment." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "first: sinteger32, last: sinteger32", "local total: sinteger32 = 0; for index in first .. last do total += index; end"), "(BLOCK (LOCAL_DECLARATION total :sinteger32 (INTEGER_LITERAL 0)) (BLOCK (LOCAL_DECLARATION index :sinteger32 flags=1 (IDENTIFIER first)) (LOCAL_DECLARATION $last0 :sinteger32 (IDENTIFIER last)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER total) (IDENTIFIER index)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "The loop must desugar with an inclusive upper bound.";
}

TEST(MAREAIrControlFlow, ForRangeWithLiteralBoundsKeepsBoth)
{
    GTEST_COUT << "Validating a for-range loop with literal bounds, as in the simd example, keeps both bounds." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for index in 0 .. 3 do end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "An empty body must still carry the increment.";
}

TEST(MAREAIrControlFlow, ForRangeContinueRunsTheIncrementFirst)
{
    GTEST_COUT << "Validating a continue in a for-range body is preceded by the increment, so the loop still ends." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: boolean", "for index in 0 .. 3 do if a then continue; end end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (IF_STATEMENT (IDENTIFIER a) (BLOCK (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)))) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "Every continue that targets the for-range loop must run the increment first.";
}

TEST(MAREAIrControlFlow, ForRangeContinueOfAnInnerWhileIsLeftAlone)
{
    GTEST_COUT << "Validating a continue that targets an inner while loop gets no increment of the outer for-range loop." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: boolean", "for index in 0 .. 3 do while a do continue; end end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (WHILE_STATEMENT (IDENTIFIER a) (BLOCK (CONTINUE_STATEMENT))) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "A continue of an inner loop must stay as it is.";
}

TEST(MAREAIrControlFlow, ForRangeContinueOfNestedForRangeLoopsUsesEachOwnIncrement)
{
    GTEST_COUT << "Validating each continue in nested for-range loops runs the increment of its own loop only." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for outer in 0 .. 1 do for inner in 0 .. 2 do continue; end continue; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION outer :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 1)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER outer) (IDENTIFIER $last0)) (BLOCK (BLOCK (LOCAL_DECLARATION inner :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last1 :sinteger32 (INTEGER_LITERAL 2)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER inner) (IDENTIFIER $last1)) (BLOCK (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER inner) (IDENTIFIER $last1)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER inner) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER inner) (IDENTIFIER $last1)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER inner) (INTEGER_LITERAL 1))))) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER outer) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER outer) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER outer) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER outer) (INTEGER_LITERAL 1))))))") << "Each loop must increment its own variable before its own continue.";
}

TEST(MAREAIrControlFlow, ForRangeNestedLoopsKeepADistinctHiddenBoundEach)
{
    GTEST_COUT << "Validating two nested for-range loops name their hidden bounds apart, and each test reads the bound of its own loop." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for outer in 0 .. 1 do for inner in 0 .. 2 do end end"), "(BLOCK (BLOCK (LOCAL_DECLARATION outer :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 1)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER outer) (IDENTIFIER $last0)) (BLOCK (BLOCK (LOCAL_DECLARATION inner :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last1 :sinteger32 (INTEGER_LITERAL 2)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER inner) (IDENTIFIER $last1)) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER inner) (IDENTIFIER $last1)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER inner) (INTEGER_LITERAL 1))))) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER outer) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER outer) (INTEGER_LITERAL 1))))))") << "The outer loop must take the first number, so a reader sees the loops in source order.";
}

TEST(MAREAIrControlFlow, ForRangeLoopsOneAfterTheOtherKeepADistinctHiddenBoundEach)
{
    GTEST_COUT << "Validating two for-range loops in a row never share a hidden bound name inside one function." << std::endl;

    const std::string rendered{ MAREA_lowerBody("", "", "for first in 0 .. 1 do end for second in 0 .. 2 do end for third in 0 .. 3 do end") };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last0 "), std::string::npos) << rendered;

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last1 "), std::string::npos) << rendered;

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last2 "), std::string::npos) << rendered;

    EXPECT_EQ(rendered.find("$last "), std::string::npos) << "The bare name must not remain: " << rendered;

    EXPECT_EQ(rendered.find("$last)"), std::string::npos) << "The bare name must not remain: " << rendered;
}

TEST(MAREAIrControlFlow, ForRangeHiddenBoundNumberKeepsEveryDigitOfALargeLoopCount)
{
    GTEST_COUT << "Validating the hidden bound name of the tenth and the hundred and first loop of a function keeps all its digits, in order, and ends the name." << std::endl;

    std::string body;

    for (int index{ 0 }; index < 101; ++index) {
        body += "for v" + std::to_string(index) + " in 0 .. 1 do end ";
    }

    const std::string rendered{ MAREA_lowerBody("", "", body) };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last9 "), std::string::npos) << "The tenth loop must be $last9.";

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last10 "), std::string::npos) << "The eleventh loop must be $last10, not $last01 or $last1.";

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last99 "), std::string::npos) << "The hundredth loop must be $last99.";

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last100 "), std::string::npos) << "The hundred and first loop must be $last100.";

    EXPECT_EQ(rendered.find("$last01"), std::string::npos) << "The digits must not be written backwards.";

    EXPECT_EQ(MAREA_countOf(rendered, "(LOCAL_DECLARATION $last"), std::size_t{ 101 }) << "Every loop must have a hidden bound of its own.";
}

TEST(MAREAIrControlFlow, ForRangeHiddenBoundNumberStartsAgainInEachFunction)
{
    GTEST_COUT << "Validating the number of a hidden bound counts the loops of one function, so a function lowers the same wherever it stands in the unit." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function one() for a in 0 .. 1 do for b in 0 .. 1 do end end end function two() for c in 0 .. 1 do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string FIRST{ MAREA_irRender(MAREA_irChild(MAREA_irChild(fixture.module, 0), 1)) };

    const std::string SECOND{ MAREA_irRender(MAREA_irChild(MAREA_irChild(fixture.module, 1), 1)) };

    EXPECT_NE(FIRST.find("$last1"), std::string::npos) << "The first function has two loops: " << FIRST;

    EXPECT_EQ(SECOND.find("$last1"), std::string::npos) << "The second function must start again at zero: " << SECOND;

    EXPECT_NE(SECOND.find("(LOCAL_DECLARATION $last0 "), std::string::npos) << SECOND;
}

TEST(MAREAIrControlFlow, ForRangeContinueStepOfANestedLoopReadsItsOwnHiddenBound)
{
    GTEST_COUT << "Validating the step that a continue gets reads the hidden bound of the loop the continue targets." << std::endl;

    const std::string rendered{ MAREA_lowerBody("", "", "for outer in 0 .. 1 do for inner in 0 .. 2 do continue; end continue; end") };

    EXPECT_NE(rendered.find("(BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER inner) (IDENTIFIER $last1)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER inner) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))"), std::string::npos) << rendered;

    EXPECT_NE(rendered.find("(BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER outer) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER outer) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))"), std::string::npos) << rendered;
}

TEST(MAREAIrControlFlow, ForRangeEvaluatesTheLastBoundOnce)
{
    GTEST_COUT << "Validating a for-range loop evaluates a call in its last bound once, into a hidden local before the loop." << std::endl;

    const std::string rendered{ MAREA_lowerBody("function bound(): sinteger32 return 3; end", "", "for index in 0 .. bound() do end") };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last0 :sinteger32 (CALL_EXPRESSION"), std::string::npos) << "The bound call must sit in the hidden local: " << rendered;

    EXPECT_EQ(rendered.find("(IDENTIFIER bound)", rendered.find("(WHILE_STATEMENT")), std::string::npos) << "The loop must not call the bound again: " << rendered;
}

TEST(MAREAIrControlFlow, ForRangeLeavesBeforeTheIncrementAtTheLastValue)
{
    GTEST_COUT << "Validating the loop breaks when the variable equals the hidden bound, so the increment never passes the type maximum." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for index in 254 .. 255 do end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index :sinteger32 flags=1 (INTEGER_LITERAL 254)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 255)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "The break must come before the increment.";
}

TEST(MAREAIrControlFlow, ForRangeIncrementTextIsOne)
{
    GTEST_COUT << "Validating the synthetic increment of a for-range loop is the integer literal 1." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function run() for index in 0 .. 3 do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *body{ MAREA_irChild(MAREA_irChild(fixture.module, 0), 1) };

    const MAREA_IrNode *loop{ MAREA_irChild(MAREA_irChild(body, 0), 2) };

    const MAREA_IrNode *increment{ MAREA_irChild(MAREA_irChild(loop, 1), 1) };

    EXPECT_EQ(MAREA_IrNodeGetKind(increment), MAREA_IR_KIND_ASSIGNMENT) << "The last statement of the loop body must be the increment.";

    const MAREA_IrNode *one{ MAREA_irChild(increment, 1) };

    EXPECT_EQ(MAREA_IrNodeGetKind(one), MAREA_IR_KIND_INTEGER_LITERAL) << "The increment value must be an integer literal.";

    EXPECT_EQ(MAREA_irText(one), "1") << "The increment value must be 1.";
}

TEST(MAREAIrControlFlow, SwitchBecomesAHiddenLocalAndAnIfChain)
{
    GTEST_COUT << "Validating a switch with a case and a default lowers to a scope with a hidden local and an if with an else." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "switch a case 1 then return; default then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (RETURN_STATEMENT)) (ELSE_CLAUSE (BLOCK (RETURN_STATEMENT))))))") << "The switch must keep the shape of its desugaring.";
}

TEST(MAREAIrControlFlow, SwitchCasesAfterTheFirstBecomeElseifClauses)
{
    GTEST_COUT << "Validating each case after the first becomes an elseif clause and a switch without default has no else." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "switch a case 1 then return; case 2 then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 2)) (BLOCK (RETURN_STATEMENT))))))") << "A second case must be an elseif clause, and no default means no else.";
}

TEST(MAREAIrControlFlow, SwitchCaseWithManyValuesJoinsThemWithOr)
{
    GTEST_COUT << "Validating a case that lists three values joins the comparisons with or, grouped to the left." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "switch a case 1, 2, 3 then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=KEYWORD_OR (BINARY_EXPRESSION op=KEYWORD_OR (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 2))) (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 3))) (BLOCK (RETURN_STATEMENT)))))") << "Many values must become one condition with or.";
}

TEST(MAREAIrControlFlow, SwitchWithOnlyADefaultRunsItsBlockAfterTheHiddenLocal)
{
    GTEST_COUT << "Validating a switch with only a default keeps the scrutinee local and then the default block." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "switch a default then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (BLOCK (RETURN_STATEMENT))))") << "A default alone has no condition to test.";
}

TEST(MAREAIrControlFlow, SwitchWithoutArmsKeepsOnlyTheScrutinee)
{
    GTEST_COUT << "Validating a switch with no arm still evaluates its scrutinee once, into the hidden local." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function ping(): sinteger32 return 1; end", "", "switch ping() end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (CALL_EXPRESSION :sinteger32 (IDENTIFIER ping) (ARGUMENT_LIST)))))") << "An empty switch must keep the call.";
}

TEST(MAREAIrControlFlow, SwitchEvaluatesTheScrutineeOnce)
{
    GTEST_COUT << "Validating a call as the scrutinee is evaluated once, into the hidden local, and never again per case." << std::endl;

    const std::string rendered{ MAREA_lowerBody("function pick(): sinteger32 return 1; end", "", "switch pick() case 1 then return; case 2 then return; end") };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $switch (CALL_EXPRESSION"), std::string::npos) << "The call must sit in the hidden local: " << rendered;

    EXPECT_EQ(rendered.find("(IDENTIFIER pick)", rendered.find("(IF_STATEMENT")), std::string::npos) << "The cases must not call the scrutinee again: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchOnAnEnumComparesWithTheVariantAccess)
{
    GTEST_COUT << "Validating a switch on an enum compares the hidden local with the lowered variant access." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("enum Color red; green; end", "c: Color", "switch c case Color.red then return; case Color.green then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER c)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (MEMBER_ACCESS (IDENTIFIER Color) (IDENTIFIER red))) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (MEMBER_ACCESS (IDENTIFIER Color) (IDENTIFIER green))) (BLOCK (RETURN_STATEMENT))) (ELSE_CLAUSE (BLOCK (UNREACHABLE_STATEMENT))))))") << "A variant case must compare with the variant access.";
}

TEST(MAREAIrControlFlow, SwitchOverAnEnumWithoutDefaultEndsWithAnUnreachableElse)
{
    GTEST_COUT << "Validating an enum switch that covers every variant and has no default ends its chain with an else holding UNREACHABLE_STATEMENT." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("enum Color red; green; end", "c: Color", "switch c case Color.red then return; case Color.green then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER c)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (MEMBER_ACCESS (IDENTIFIER Color) (IDENTIFIER red))) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (MEMBER_ACCESS (IDENTIFIER Color) (IDENTIFIER green))) (BLOCK (RETURN_STATEMENT))) (ELSE_CLAUSE (BLOCK (UNREACHABLE_STATEMENT))))))") << "The proven-exhaustive chain must end with an unreachable else.";
}

TEST(MAREAIrControlFlow, SwitchOverAnEnumWithADefaultKeepsItsDefaultElse)
{
    GTEST_COUT << "Validating an enum switch with a default keeps the default as its else and adds no unreachable marker." << std::endl;

    const std::string rendered{ MAREA_lowerBody("enum Color red; green; end", "c: Color", "switch c case Color.red then return; default then return; end") };

    EXPECT_EQ(rendered.find("UNREACHABLE_STATEMENT"), std::string::npos) << "A default arm must stay the only else: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchOverANonEnumWithoutDefaultKeepsNoElse)
{
    GTEST_COUT << "Validating an integer switch with no default has no else, because it can really fall through." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "switch a case 1 then return; end").find("UNREACHABLE_STATEMENT"), std::string::npos) << "An integer switch is not exhaustive, so it must not be marked.";
}

TEST(MAREAIrControlFlow, SwitchOverAnEnumThroughATypeAliasEndsWithAnUnreachableElse)
{
    GTEST_COUT << "Validating an enum switch whose scrutinee is typed by an alias of the enum is still proven exhaustive and ends with the unreachable else." << std::endl;

    const std::string rendered{ MAREA_lowerBody("enum Color red; green; end type Shade = Color;", "c: Shade", "switch c case Color.red then return; case Color.green then return; end") };

    EXPECT_NE(rendered.find("(ELSE_CLAUSE (BLOCK (UNREACHABLE_STATEMENT)))"), std::string::npos) << rendered;
}

TEST(MAREAIrControlFlow, SwitchOverAnEnumWithManyValuesPerCaseEndsWithOneUnreachableElse)
{
    GTEST_COUT << "Validating an enum switch whose cases hold several variants each gets exactly one unreachable else, at the end of the chain." << std::endl;

    const std::string rendered{ MAREA_lowerBody("enum Color red; green; blue; end", "c: Color", "switch c case Color.red, Color.green then return; case Color.blue then return; end") };

    EXPECT_EQ(MAREA_countOf(rendered, "UNREACHABLE_STATEMENT"), std::size_t{ 1 }) << rendered;

    const std::string CLOSING{ "(ELSE_CLAUSE (BLOCK (UNREACHABLE_STATEMENT))))))" };

    EXPECT_EQ(rendered.rfind(CLOSING), rendered.size() - CLOSING.size()) << "The else must close the chain: " << rendered;
}

TEST(MAREAIrControlFlow, EnumSwitchThatReturnsOnEveryPathLeavesNoPathWithoutAnEnd)
{
    GTEST_COUT << "Validating a function that returns in every arm of an enum switch has an end on every path of the lowered chain: each arm returns and the else is unreachable." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; enum Color red; green; end function f(c: Color): sinteger32 switch c case Color.red then return 1; case Color.green then return 2; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const std::string rendered{ MAREA_irRender(MAREA_irChild(MAREA_irChild(fixture.module, 1), 1)) };

    EXPECT_EQ(MAREA_countOf(rendered, "(RETURN_STATEMENT"), std::size_t{ 2 }) << rendered;

    EXPECT_EQ(MAREA_countOf(rendered, "(ELSE_CLAUSE (BLOCK (UNREACHABLE_STATEMENT)))"), std::size_t{ 1 }) << "The one path with no return must end in the marker: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchOverAnEnumWithOnlyADefaultHasNoUnreachableMarker)
{
    GTEST_COUT << "Validating an enum switch with only a default keeps no chain and so no unreachable marker." << std::endl;

    const std::string rendered{ MAREA_lowerBody("enum Color red; green; end", "c: Color", "switch c default then return; end") };

    EXPECT_EQ(rendered, "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER c)) (BLOCK (RETURN_STATEMENT))))") << "The default block must follow the hidden local, with no marker.";
}

TEST(MAREAIrControlFlow, SwitchOverAnExternalTypeWithoutDefaultKeepsNoElse)
{
    GTEST_COUT << "Validating a switch whose scrutinee has a type sema does not know has no unreachable else, as the other no-default switches that can fall through." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.size as size;", "a: size", "switch a case 1 then return; end") };

    EXPECT_NE(rendered.find("(IF_STATEMENT"), std::string::npos) << "The switch must lower: " << rendered;

    EXPECT_EQ(rendered.find("ELSE_CLAUSE"), std::string::npos) << "Nothing proves an external type exhaustive: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchWithTwoDefaultsOrARepeatedCaseNeverReachesLowering)
{
    GTEST_COUT << "Validating that a switch with two defaults, or a repeated case, gets a sema error and no typed tree, so the lowering cannot see it." << std::endl;

    for (const char *body : { "switch a case 1 then return; default then return; default then unreachable!(); end", "switch a case 1 then return; case 1 then unreachable!(); end" }) {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        ASSERT_EQ(fixture.sema.MAREA_sema(std::string("namespace main; function run(a: sinteger32) ") + body + " end", "main.marea"), MAREA_SEMA_EXCEPT_SUCCESS) << "Sema must run: " << body;

        const std::vector<MAREA_Uinteger32> codes{ fixture.sema.parser.MAREA_codes() };

        EXPECT_NE(std::count(codes.begin(), codes.end(), MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_SWITCH_ARM), 0) << "Sema must raise DUPLICATE_SWITCH_ARM: " << body;

        EXPECT_EQ(MAREA_SemaBuildTypedAst(fixture.sema.context, fixture.sema.parser.root, &fixture.typedRoot), MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NOT_CHECKED) << "The typed tree, the only input of the IR, must refuse a tree with a sema error: " << body;
    }
}

TEST(MAREAIrControlFlow, SwitchNestedInAnArmKeepsItsOwnHiddenLocal)
{
    GTEST_COUT << "Validating a switch nested in an arm gets its own scope and hidden local, so the two never mix." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32, b: sinteger32", "switch a case 1 then switch b case 2 then return; end default then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER b)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 2)) (BLOCK (RETURN_STATEMENT))))) (ELSE_CLAUSE (BLOCK (RETURN_STATEMENT))))))") << "The inner switch must sit in its own scope.";
}

TEST(MAREAIrControlFlow, SwitchArmBreakAndContinueTargetTheEnclosingLoop)
{
    GTEST_COUT << "Validating a break in a switch arm stays a break and a continue in an arm of a for-range body still runs the step first." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "while true do switch a case 1 then break; default then continue; end end"), "(BLOCK (WHILE_STATEMENT (BOOLEAN_LITERAL true) (BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (BREAK_STATEMENT)) (ELSE_CLAUSE (BLOCK (CONTINUE_STATEMENT))))))))") << "An arm must not swallow a break or a continue.";

    const std::string rendered{ MAREA_lowerBody("", "a: sinteger32", "for index in 0 .. 3 do switch a case 1 then continue; end end") };

    EXPECT_NE(rendered.find("(BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))"), std::string::npos) << "The continue inside the arm must carry the step of the for-range loop: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchArmWithAnUnsupportedStatementDropsTheFunction)
{
    GTEST_COUT << "Validating a gap inside a switch arm still drops the whole function and raises its diagnostic once." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function keep() end function lose(a: sinteger32) switch a case 1 then print!(\"hi\"); default then return; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the lowerable function must remain.";

    EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The macro in the arm must raise its own code once.";
}

TEST(MAREAIrControlFlow, SwitchDefaultArmBeforeTheCasesNoLongerReachesLowering)
{
    GTEST_COUT << "Validating a default arm written before the cases is now a parse diagnostic, so lowering never sees it." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: sinteger32", "switch a default then return; case 1 then break; case 2 then continue; end"), "<lower failed>") << "The parser now rejects a default arm that is not last, so the body never reaches the lowerer.";
}

TEST(MAREAIrControlFlow, SwitchWithAGapInACaseValueOrTheDefaultDropsTheFunction)
{
    GTEST_COUT << "Validating a gap in a case value, and a gap in the default arm, each drop the function and raise one diagnostic." << std::endl;

    for (const char *body : { "switch a case print!(\"hi\") then return; end", "switch a case 1 then return; default then print!(\"hi\"); end" }) {
        MAREA_IrFixture fixture;

        ASSERT_EQ(fixture.MAREA_lower(std::string("namespace main; function keep() end function lose(a: sinteger32) ") + body + " end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

        EXPECT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the lowerable function must remain: " << body;

        EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The gap must raise its own code once: " << body;
    }
}

TEST(MAREAIrControlFlow, ForEachBecomesItsOwnKindWithTheNameAsText)
{
    GTEST_COUT << "Validating a for-each loop lowers to one FOR_EACH_STATEMENT with the value name as text, the collection and the body." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>", "local total: sinteger32 = 0; for each value in values do total += value; end"), "(BLOCK (LOCAL_DECLARATION total :sinteger32 (INTEGER_LITERAL 0)) (FOR_EACH_STATEMENT value (IDENTIFIER values) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER total) (IDENTIFIER value)))))") << "The loop must keep its own kind, with no desugaring.";
}

TEST(MAREAIrControlFlow, ForEachWithAnEmptyBodyKeepsAnEmptyBlock)
{
    GTEST_COUT << "Validating a for-each loop with an empty body keeps an empty block as its last child." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>", "for each value in values do end"), "(BLOCK (FOR_EACH_STATEMENT value (IDENTIFIER values) (BLOCK)))") << "An empty body must stay an empty block.";
}

TEST(MAREAIrControlFlow, ForEachWithKeyAndValueKeepsTheKeyAsTheFirstChild)
{
    GTEST_COUT << "Validating the key of a for-each loop with two names is an IDENTIFIER child before the collection, and the text is the value name." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>", "for each key, value in values do end"), "(BLOCK (FOR_EACH_STATEMENT value (IDENTIFIER key) (IDENTIFIER values) (BLOCK)))") << "The key must be the first child and the value the text.";
}

TEST(MAREAIrControlFlow, ForEachMutableKeepsTheFlagOnTheLoopOnly)
{
    GTEST_COUT << "Validating a for-each loop over a mutable collection sets MUTABLE on the loop node and on no other node." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "#[semantics = reference_mutable] values: list<sinteger32>", "for each key, value in mutable values do value += 1; end"), "(BLOCK (FOR_EACH_STATEMENT value flags=1 (IDENTIFIER key) (IDENTIFIER values) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER value) (INTEGER_LITERAL 1)))))") << "Only the loop node may carry MUTABLE.";
}

TEST(MAREAIrControlFlow, ForEachLeavesNoHiddenNameInTheOutput)
{
    GTEST_COUT << "Validating a for-each loop in both forms leaves no hidden name, with a dollar sign, in the output." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>", "for each value in values do end for each key, value in values do end").find('$'), std::string::npos) << "No hidden local or call may remain.";
}

TEST(MAREAIrControlFlow, ForEachKeepsTheCollectionAsOneChildEvaluatedByTheNode)
{
    GTEST_COUT << "Validating a call in the collection of a for-each loop appears once, as the child of the loop." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.list as list; function source(): list<sinteger32> return source(); end", "", "for each value in source() do end") };

    EXPECT_NE(rendered.find("(FOR_EACH_STATEMENT value (CALL_EXPRESSION :list (IDENTIFIER source) (ARGUMENT_LIST)) (BLOCK))"), std::string::npos) << "The call must be the collection child: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachContinueAndBreakStayAsTheyAre)
{
    GTEST_COUT << "Validating a continue and a break in a for-each body stay plain statements, with no step in front of them." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>, a: boolean", "for each value in values do if a then continue; end if a then break; end end") };

    EXPECT_EQ(rendered, "(BLOCK (FOR_EACH_STATEMENT value (IDENTIFIER values) (BLOCK (IF_STATEMENT (IDENTIFIER a) (BLOCK (CONTINUE_STATEMENT))) (IF_STATEMENT (IDENTIFIER a) (BLOCK (BREAK_STATEMENT))))))") << "The loop kind is the target of both.";
}

TEST(MAREAIrControlFlow, ForEachContinueOfAnInnerWhileIsLeftAlone)
{
    GTEST_COUT << "Validating a continue that targets a while loop inside a for-each body stays as it is." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>, a: boolean", "for each value in values do while a do continue; end end"), "(BLOCK (FOR_EACH_STATEMENT value (IDENTIFIER values) (BLOCK (WHILE_STATEMENT (IDENTIFIER a) (BLOCK (CONTINUE_STATEMENT))))))") << "A continue of an inner loop must stay as it is.";
}

TEST(MAREAIrControlFlow, ForEachNestedInAForEachIsAPlainNesting)
{
    GTEST_COUT << "Validating a for-each loop nested in another is a nested node, and each continue stays as it is." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "rows: list<list<sinteger32>>", "for each row in rows do for each cell in row do continue; end continue; end"), "(BLOCK (FOR_EACH_STATEMENT row (IDENTIFIER rows) (BLOCK (FOR_EACH_STATEMENT cell (IDENTIFIER row) (BLOCK (CONTINUE_STATEMENT))) (CONTINUE_STATEMENT))))") << "The inner loop must be a child of the body of the outer one.";
}

TEST(MAREAIrControlFlow, ForEachInsideAForRangeKeepsItsContinueAndTheRangeContinueGetsItsStep)
{
    GTEST_COUT << "Validating a continue of a for-range loop gets the step of that loop, and a continue inside a for-each nested in it does not." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.list as list;", "values: list<sinteger32>, a: boolean", "for index in 0 .. 3 do if a then continue; end for each value in values do continue; end end") };

    EXPECT_NE(rendered.find("(FOR_EACH_STATEMENT value (IDENTIFIER values) (BLOCK (CONTINUE_STATEMENT)))"), std::string::npos) << "The continue of the for each must stay bare: " << rendered;

    const std::string STEP{ "(IDENTIFIER index) (IDENTIFIER $last0)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))" };

    const auto FIRST{ rendered.find(STEP) };

    ASSERT_NE(FIRST, std::string::npos) << "The continue of the for-range loop must get its step: " << rendered;

    const auto SECOND{ rendered.find(STEP, (FIRST + 1)) };

    ASSERT_NE(SECOND, std::string::npos) << "The end of the for-range body must hold its step: " << rendered;

    EXPECT_EQ(rendered.find(STEP, (SECOND + 1)), std::string::npos) << "The for each must get no for-range step: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachPositionsComeFromTheSourceNodes)
{
    GTEST_COUT << "Validating the loop node sits at the for statement and the key at its own name." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.list as list; function run(values: list<sinteger32>) for each key, value in values do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *loop{ MAREA_irChild(MAREA_irChild(MAREA_irChild(fixture.module, 0), 1), 0) };

    ASSERT_EQ(MAREA_IrNodeGetKind(loop), MAREA_IR_KIND_FOR_EACH_STATEMENT) << "The loop must be its own kind.";

    EXPECT_EQ(MAREA_irText(loop), "value") << "The text must be the value name.";

    const MAREA_IrNode *key{ MAREA_irChild(loop, 0) };

    EXPECT_EQ(MAREA_irText(key), "key") << "The first child must be the key.";

    EXPECT_GT(MAREA_IrNodeGetPosition(key).column, MAREA_IrNodeGetPosition(loop).column) << "The key must sit at its name, after the for keyword.";
}

TEST(MAREAIrControlFlow, ForEachWithAnUnsupportedStatementDropsTheFunction)
{
    GTEST_COUT << "Validating a gap inside a for-each body still drops the whole function and raises its diagnostic once." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.list as list; function keep() end function lose(values: list<sinteger32>) for each value in values do print!(\"hi\"); end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the lowerable function must remain.";

    EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The macro in the body must raise its own code once.";
}

TEST(MAREAIrControlFlow, ForEachWithAGapInTheCollectionDropsTheFunction)
{
    GTEST_COUT << "Validating a gap inside the collection expression drops the function and raises its own diagnostic once." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.list as list; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function lose(values: list<sinteger32>) for each value in identity(values) do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 0U) << "Neither function may remain.";

    EXPECT_EQ(fixture.MAREA_irCodes(), (std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC })) << "The generic function and its call must each raise 4004 once, and for each must raise nothing.";
}

TEST(MAREAIrControlFlow, ForEachMutableContinueIsNotWrapped)
{
    GTEST_COUT << "Validating a continue in a mutable for-each loop gets no wrapper block, so no flag is copied to a synthetic node." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.list as list;", "#[semantics = reference_mutable] values: list<sinteger32>, a: boolean", "for each value in mutable values do if a then continue; end end"), "(BLOCK (FOR_EACH_STATEMENT value flags=1 (IDENTIFIER values) (BLOCK (IF_STATEMENT (IDENTIFIER a) (BLOCK (CONTINUE_STATEMENT))))))") << "The continue must stay bare.";
}

TEST(MAREAIrControlFlow, ForEachWithAGapInTheCollectionDoesNotLowerItsBody)
{
    GTEST_COUT << "Validating a for-each loop whose collection holds a gap reports that gap only, and not the gap of its body." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.list as list; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function lose(values: list<sinteger32>) for each value in identity(values) do print!(\"hi\"); end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(fixture.MAREA_irCodes(), (std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC })) << "Only the gaps of the collection may be reported.";
}

TEST(MAREAIrControlFlow, ForEachExamplesLowerTheirLoops)
{
    GTEST_COUT << "Validating sum_array of basics/control_flow.marea and the two worker functions of parallel/scaling.marea are lowered with their for-each loops as FOR_EACH_STATEMENT." << std::endl;

    const MAREA_IrExampleSet EXAMPLES;

    const std::vector<std::pair<std::string, std::string>> LOOPS{
        { "basics/control_flow.marea", "sum_array" },
        { "parallel/scaling.marea", "scale_slice" },
        { "parallel/scaling.marea", "sum_slice" },
    };

    for (const auto &[file, name] : LOOPS) {
        MAREA_IrFixture fixture(EXAMPLES.MAREA_resolver());

        ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample(file), file), MAREA_IR_EXCEPT_SUCCESS) << "The example " << file << " must lower.";

        bool found{ false };

        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
            if (MAREA_irText(MAREA_irChild(fixture.module, index)) != name) {
                continue;
            }

            found = true;

            EXPECT_NE(MAREA_irRender(MAREA_irChild(fixture.module, index)).find("(FOR_EACH_STATEMENT "), std::string::npos) << "The function " << name << " must hold its for each loop.";
        }

        EXPECT_TRUE(found) << "The function " << name << " of " << file << " must be lowered.";
    }
}

TEST(MAREAIrControlFlow, BodyWithAnUnsupportedStatementDropsTheFunction)
{
    GTEST_COUT << "Validating a function whose body holds a construct that cannot be lowered yet is left out as a whole." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function keep() end function lose() print!(\"hi\"); end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the lowerable function must remain.";

    EXPECT_EQ(MAREA_irText(MAREA_irChild(fixture.module, 0)), "keep") << "The remaining function must be the one without the gap.";
}

TEST(MAREAIrControlFlow, BlocksExampleKeepsItsScopes)
{
    GTEST_COUT << "Validating examples/basics/blocks.marea lowers its do-end scopes as SCOPE_BLOCK nodes where it can." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("basics/blocks.marea"), "basics/blocks.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    EXPECT_EQ(MAREA_IrNodeGetKind(fixture.module), MAREA_IR_KIND_MODULE) << "The root must be a MODULE.";
}

TEST(MAREAIrControlFlow, FunctionsExampleLowersEveryFunctionWithABody)
{
    GTEST_COUT << "Validating examples/basics/functions.marea lowers its functions with a BLOCK body." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("basics/functions.marea"), "basics/functions.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    ASSERT_GT(MAREA_IrNodeGetChildCount(fixture.module), 0U) << "The module must hold functions.";

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
        const MAREA_IrNode *declaration{ MAREA_irChild(fixture.module, index) };

        if (MAREA_IrNodeGetKind(declaration) != MAREA_IR_KIND_FUNCTION_DECLARATION) {
            continue;
        }

        const MAREA_IrNode *body{ MAREA_irChild(declaration, MAREA_IrNodeGetChildCount(declaration) - 1) };

        EXPECT_EQ(MAREA_IrNodeGetKind(body), MAREA_IR_KIND_BLOCK) << "The function " << MAREA_irText(declaration) << " must end with its BLOCK body.";
    }
}

TEST(MAREAIrControlFlow, SimdExampleDesugarsItsForRangeLoop)
{
    GTEST_COUT << "Validating examples/attributes/simd.marea lowers its for index in 0 .. 3 loop to the desugared shape." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("attributes/simd.marea"), "attributes/simd.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    bool found{ false };

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
        const MAREA_IrNode *declaration{ MAREA_irChild(fixture.module, index) };

        if (MAREA_irText(declaration) != "dot_product_reference") {
            continue;
        }

        found = true;

        const std::string rendered{ MAREA_irRender(declaration) };

        EXPECT_NE(rendered.find("(LOCAL_DECLARATION index :sinteger32 flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last0 :sinteger32 (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last0))"), std::string::npos) << "The loop must be desugared: " << rendered;
    }

    EXPECT_TRUE(found) << "The function dot_product_reference must be lowered.";
}

TEST(MAREAIrControlFlow, ControlFlowExampleKeepsItsIfAndWhileShapes)
{
    GTEST_COUT << "Validating examples/basics/control_flow.marea keeps the declarations without a gap construct." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower(MAREA_irReadExample("basics/control_flow.marea"), "basics/control_flow.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The example must lower.";

    bool sumRange{ false };

    for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(fixture.module); ++index) {
        if (MAREA_irText(MAREA_irChild(fixture.module, index)) == "sum_range") {
            sumRange = true;

            EXPECT_NE(MAREA_irRender(MAREA_irChild(fixture.module, index)).find("(LOCAL_DECLARATION $last0 :sinteger32 (IDENTIFIER last))"), std::string::npos) << "The range must include its last value.";
        }
    }

    EXPECT_TRUE(sumRange) << "The function sum_range must be lowered.";
}

namespace
{
    const std::string MAREA_BOX{ "structure Box x: sinteger32; end implementation Box #[operator = \"destroy\"] function drop() end end " };
} // namespace

TEST(MAREAIrControlFlow, DestructibleLocalIsDestroyedAtTheEndOfItsBlock)
{
    GTEST_COUT << "Validating a local of a destructible type is destroyed, in reverse order, at the end of its own block." << std::endl;

    EXPECT_EQ(MAREA_lowerBody(MAREA_BOX, "", "local a: Box = Box(); local b: Box = Box();"), "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (DESTROY_CALL Box.drop (IDENTIFIER b)) (DESTROY_CALL Box.drop (IDENTIFIER a)))") << "Both locals must be destroyed, last declared first.";
}

TEST(MAREAIrControlFlow, LocalInitializedFromATypeBraceLiteralIsDestroyedLikeOneFromTypeCall)
{
    GTEST_COUT << "Validating a local initialized from Type{...} of a type with construct is destroyed like one initialized from Type(args)." << std::endl;

    const std::string CONSTRUCTED_BOX{ "structure Box x: sinteger32; end implementation Box #[operator = \"construct\"] function make(x: sinteger32 = 0) end #[operator = \"destroy\"] function drop() end end " };

    EXPECT_EQ(MAREA_lowerBody(CONSTRUCTED_BOX, "", "local a: Box = Box{ x = 1 };"), "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST (INTEGER_LITERAL 0)) (LITERAL_VALUE :Box (FIELD_ENTRY x (INTEGER_LITERAL 1))))) (DESTROY_CALL Box.drop (IDENTIFIER a)))") << "The literal local must get its DESTROY_CALL at the end of the block.";
}

TEST(MAREAIrControlFlow, NonDestructibleLocalGetsNoDestroyCall)
{
    GTEST_COUT << "Validating a local of a type with no destroy operator gets no DESTROY_CALL." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "local total: sinteger32 = 1;"), "(BLOCK (LOCAL_DECLARATION total :sinteger32 (INTEGER_LITERAL 1)))") << "A plain primitive local must never be destroyed.";
}

TEST(MAREAIrControlFlow, EarlyReturnDestroysEverySurroundingDestructibleLocal)
{
    GTEST_COUT << "Validating a return inside a nested if destroys every enclosing destructible local first." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(MAREA_BOX, "flag: boolean", "local a: Box = Box(); if flag then local b: Box = Box(); return; end") };

EXPECT_EQ(RENDERED, "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (IF_STATEMENT (IDENTIFIER flag) (BLOCK (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (DESTROY_CALL Box.drop (IDENTIFIER b)) (BLOCK (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT))))) (DESTROY_CALL Box.drop (IDENTIFIER a)))") << "The return must destroy b, then a, in that order, before transferring control: " <<
RENDERED
;
}

TEST(MAREAIrControlFlow, BreakDestroysOnlyScopesInsideItsOwnLoop)
{
    GTEST_COUT << "Validating a break destroys the destructible locals nested inside its own loop, never an outer one." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(MAREA_BOX, "flag: boolean", "local a: Box = Box(); while flag do local b: Box = Box(); break; end") };

EXPECT_EQ(RENDERED, "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (WHILE_STATEMENT (IDENTIFIER flag) (BLOCK (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (DESTROY_CALL Box.drop (IDENTIFIER b)) (BREAK_STATEMENT)))) (DESTROY_CALL Box.drop (IDENTIFIER a)))") << "The break must destroy only b, never a, which is destroyed later at the function's own end: " <<
RENDERED
;
}

TEST(MAREAIrControlFlow, ContinueDestroysTheLoopBodyScopeBeforeTheStep)
{
    GTEST_COUT << "Validating a continue destroys the loop body's own destructible local before the for-range step." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(MAREA_BOX, "", "for index in 1 .. 3 do local b: Box = Box(); continue; end") };

EXPECT_NE(RENDERED.find("(BLOCK (DESTROY_CALL Box.drop (IDENTIFIER b)) (BLOCK (IF_STATEMENT"), std::string::npos) << "The destroy of b must come before the loop's own step: " <<
RENDERED
;
}

TEST(MAREAIrControlFlow, MovedLocalIsNotDestroyed)
{
    GTEST_COUT << "Validating a local moved out with move x is not destroyed at the end of its block." << std::endl;

    EXPECT_EQ(MAREA_lowerBody(MAREA_BOX + "function sink(value: Box) end ", "", "local a: Box = Box(); sink(move a);"), "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (EXPRESSION_STATEMENT (CALL_EXPRESSION (IDENTIFIER sink) (ARGUMENT_LIST (UNARY_EXPRESSION op=KEYWORD_MOVE (IDENTIFIER a))))))") << "A moved-from local must not get a DESTROY_CALL.";
}

TEST(MAREAIrControlFlow, LoopBodyLocalIsDestroyedAtTheEndOfEveryIteration)
{
    GTEST_COUT << "Validating a destructible local of a loop body is destroyed at the end of the body, so once per iteration." << std::endl;

    EXPECT_EQ(MAREA_lowerBody(MAREA_BOX, "flag: boolean", "while flag do local b: Box = Box(); end"), "(BLOCK (WHILE_STATEMENT (IDENTIFIER flag) (BLOCK (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (DESTROY_CALL Box.drop (IDENTIFIER b)))))") << "The destroy must sit in the loop body, never after the loop.";
}

TEST(MAREAIrControlFlow, ReturnWithAValueIsHoistedBeforeTheDestroys)
{
    GTEST_COUT << "Validating a return that carries a value reads the value into $return before the destroys of the locals it leaves." << std::endl;

    EXPECT_EQ(MAREA_lowerBody(MAREA_BOX, "", "local a: Box = Box(); return 1;", ": sinteger32"), "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (LOCAL_DECLARATION $return (INTEGER_LITERAL 1)) (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT (IDENTIFIER $return))))") << "The value must be evaluated first, then the destroy, then the return of the hidden local.";
}

TEST(MAREAIrControlFlow, ReturnValueReadingADestroyedLocalIsEvaluatedFirst)
{
    GTEST_COUT << "Validating a return whose value names a destructible local evaluates that value before the destroy of the local." << std::endl;

    EXPECT_EQ(MAREA_lowerBody(MAREA_BOX, "", "local a: Box = Box(); return a.x;", ": sinteger32"), "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (LOCAL_DECLARATION $return (MEMBER_ACCESS (IDENTIFIER a) (IDENTIFIER x))) (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT (IDENTIFIER $return))))") << "The read of a.x must come before the destroy of a.";
}

TEST(MAREAIrControlFlow, NestedReturnWithAValueIsHoistedOnceAndDestroysInnerScopeFirst)
{
    GTEST_COUT << "Validating a value return through two destructible scopes holds one $return, the inner destroy first." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(MAREA_BOX, "flag: boolean", "local a: Box = Box(); if flag then local b: Box = Box(); return a.x; end unreachable!();", ": sinteger32") };

EXPECT_EQ(RENDERED, "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (IF_STATEMENT (IDENTIFIER flag) (BLOCK (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (LOCAL_DECLARATION $return (MEMBER_ACCESS (IDENTIFIER a) (IDENTIFIER x))) (DESTROY_CALL Box.drop (IDENTIFIER b)) (BLOCK (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT (IDENTIFIER $return)))))) (UNREACHABLE_STATEMENT) (DESTROY_CALL Box.drop (IDENTIFIER a)))") << "The value must be hoisted once, then b destroyed, then a, then the return: " <<
RENDERED
;

const std::string DECLARATION{ "LOCAL_DECLARATION $return" };

EXPECT_EQ(RENDERED.find(DECLARATION, (RENDERED.find(DECLARATION) + 1)), std::string::npos) << "No block may hold a second $return declaration: " <<
RENDERED
;
}

TEST(MAREAIrControlFlow, ReturnWithNoValueIsNotHoisted)
{
    GTEST_COUT << "Validating a return with no value is wrapped with the destroys only, with no $return." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(MAREA_BOX, "", "local a: Box = Box(); return;") };

EXPECT_EQ(RENDERED, "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT)))") << "A return with no value needs no hidden local: " <<
RENDERED
;
}

TEST(MAREAIrControlFlow, ReturnWithAValueInALoopBodyIsHoistedInsideTheLoop)
{
    GTEST_COUT << "Validating a value return in a loop body hoists inside the loop and destroys the body local then the outer one." << std::endl;

    const std::string
RENDERED
{ MAREA_lowerBody(MAREA_BOX, "flag: boolean", "local a: Box = Box(); while flag do local b: Box = Box(); return a.x; end unreachable!();", ": sinteger32") };

EXPECT_EQ(RENDERED, "(BLOCK (LOCAL_DECLARATION a :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (WHILE_STATEMENT (IDENTIFIER flag) (BLOCK (LOCAL_DECLARATION b :Box (CONSTRUCT_CALL :Box (ARGUMENT_LIST))) (BLOCK (LOCAL_DECLARATION $return (MEMBER_ACCESS (IDENTIFIER a) (IDENTIFIER x))) (DESTROY_CALL Box.drop (IDENTIFIER b)) (BLOCK (DESTROY_CALL Box.drop (IDENTIFIER a)) (RETURN_STATEMENT (IDENTIFIER $return)))))) (UNREACHABLE_STATEMENT) (DESTROY_CALL Box.drop (IDENTIFIER a)))") << "The return inside the loop must hoist inside the loop body: " <<
RENDERED
;
}
