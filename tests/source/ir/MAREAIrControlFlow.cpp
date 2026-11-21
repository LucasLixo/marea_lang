// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIrHelpers.hpp"

namespace
{
    const MAREA_ResolveCallback RESOLVE_EXTERNAL{ [](const std::string &text, const MAREA_AstNode **) {
        return ((text.compare(0, 12, "collections.") == 0) || (text.compare(0, 2, "c.") == 0)) ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };

    std::string MAREA_lowerBody(const std::string &prefix, const std::string &signature, const std::string &body)
    {
        MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

        if (fixture.MAREA_lower("namespace main; " + prefix + " function run(" + signature + ") " + body + " end") != MAREA_IR_EXCEPT_SUCCESS) {
            return "<lower failed>";
        }

        const MAREA_IrNode *function{ MAREA_irChild(fixture.module, MAREA_IrNodeGetChildCount(fixture.module) - 1) };

        if ((function == nullptr) || (MAREA_IrNodeGetChildCount(function) < 2)) {
            return "<no body>";
        }

        return MAREA_irRender(MAREA_irChild(function, MAREA_IrNodeGetChildCount(function) - 1));
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

    EXPECT_EQ(MAREA_lowerBody("", "", "local total: integer32 = 1 + 2;"), "(BLOCK (LOCAL_DECLARATION total :integer32 (BINARY_EXPRESSION op=PLUS (INTEGER_LITERAL 1) (INTEGER_LITERAL 2))))") << "The local must keep its shape.";
}

TEST(MAREAIrControlFlow, LocalDeclarationWithoutInitializerHasNoChild)
{
    GTEST_COUT << "Validating a local declaration without an initializer has no child." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "local total: integer32;"), "(BLOCK (LOCAL_DECLARATION total :integer32))") << "No initializer means no child.";
}

TEST(MAREAIrControlFlow, AssignmentKeepsOperatorAndOperands)
{
    GTEST_COUT << "Validating an assignment keeps its operator, target and value." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "value: integer32", "local total: integer32 = 0; total += value;"), "(BLOCK (LOCAL_DECLARATION total :integer32 (INTEGER_LITERAL 0)) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER total) (IDENTIFIER value)))") << "A compound assignment must keep its operator.";
}

TEST(MAREAIrControlFlow, ExpressionStatementKeepsItsExpression)
{
    GTEST_COUT << "Validating an expression statement holds its lowered call." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function ping() end", "", "ping();"), "(BLOCK (EXPRESSION_STATEMENT (CALL_EXPRESSION (IDENTIFIER ping) (ARGUMENT_LIST))))") << "The call must be the only child.";
}

TEST(MAREAIrControlFlow, IfKeepsConditionBlockElseifAndElse)
{
    GTEST_COUT << "Validating an if statement keeps its condition, block, elseif clause and else clause." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32", "if a < 1 then return; elseif a < 2 then return; else return; end"), "(BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER a) (INTEGER_LITERAL 1)) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=LESS (IDENTIFIER a) (INTEGER_LITERAL 2)) (BLOCK (RETURN_STATEMENT))) (ELSE_CLAUSE (BLOCK (RETURN_STATEMENT)))))") << "The if shape must match the source.";
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

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function one(): integer32 return 1; end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(MAREA_irRender(MAREA_irChild(MAREA_irChild(fixture.module, 0), 1)), "(BLOCK (RETURN_STATEMENT (INTEGER_LITERAL 1)))") << "The return must hold its value.";
}

TEST(MAREAIrControlFlow, ScopeBlockKeepsItsKindAndBlock)
{
    GTEST_COUT << "Validating a do-end scope lowers to a SCOPE_BLOCK that holds a BLOCK." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "do local inner: integer32 = 1; end"), "(BLOCK (SCOPE_BLOCK (BLOCK (LOCAL_DECLARATION inner :integer32 (INTEGER_LITERAL 1)))))") << "The scope must keep its block.";
}

TEST(MAREAIrControlFlow, UnsafeBlockLowersToAPlainBlock)
{
    GTEST_COUT << "Validating an unsafe block lowers to a plain BLOCK." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "unsafe local raw: integer32 = 1; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION raw :integer32 (INTEGER_LITERAL 1))))") << "The unsafe wrapper must leave no node.";
}

TEST(MAREAIrControlFlow, UnreachableMacroBecomesAnUnreachableStatement)
{
    GTEST_COUT << "Validating unreachable!() lowers to an UNREACHABLE_STATEMENT with no children." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "unreachable!();"), "(BLOCK (UNREACHABLE_STATEMENT))") << "The macro must lower to its own kind.";
}

TEST(MAREAIrControlFlow, ForRangeDesugarsToABlockWithAWhileAndAnIncrement)
{
    GTEST_COUT << "Validating a for-range loop desugars to a hidden local, a while with an inclusive bound and an increment." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "first: integer32, last: integer32", "local total: integer32 = 0; for index in first .. last do total += index; end"), "(BLOCK (LOCAL_DECLARATION total :integer32 (INTEGER_LITERAL 0)) (BLOCK (LOCAL_DECLARATION index flags=1 (IDENTIFIER first)) (LOCAL_DECLARATION $last (IDENTIFIER last)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER total) (IDENTIFIER index)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "The loop must desugar with an inclusive upper bound.";
}

TEST(MAREAIrControlFlow, ForRangeWithLiteralBoundsKeepsBoth)
{
    GTEST_COUT << "Validating a for-range loop with literal bounds, as in the simd example, keeps both bounds." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for index in 0 .. 3 do end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "An empty body must still carry the increment.";
}

TEST(MAREAIrControlFlow, ForRangeContinueRunsTheIncrementFirst)
{
    GTEST_COUT << "Validating a continue in a for-range body is preceded by the increment, so the loop still ends." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: boolean", "for index in 0 .. 3 do if a then continue; end end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (IF_STATEMENT (IDENTIFIER a) (BLOCK (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)))) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "Every continue that targets the for-range loop must run the increment first.";
}

TEST(MAREAIrControlFlow, ForRangeContinueOfAnInnerWhileIsLeftAlone)
{
    GTEST_COUT << "Validating a continue that targets an inner while loop gets no increment of the outer for-range loop." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: boolean", "for index in 0 .. 3 do while a do continue; end end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (WHILE_STATEMENT (IDENTIFIER a) (BLOCK (CONTINUE_STATEMENT))) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "A continue of an inner loop must stay as it is.";
}

TEST(MAREAIrControlFlow, ForRangeContinueOfNestedForRangeLoopsUsesEachOwnIncrement)
{
    GTEST_COUT << "Validating each continue in nested for-range loops runs the increment of its own loop only." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for outer in 0 .. 1 do for inner in 0 .. 2 do continue; end continue; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION outer flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 1)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER outer) (IDENTIFIER $last)) (BLOCK (BLOCK (LOCAL_DECLARATION inner flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 2)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER inner) (IDENTIFIER $last)) (BLOCK (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER inner) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER inner) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER inner) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER inner) (INTEGER_LITERAL 1))))) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER outer) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER outer) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER outer) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER outer) (INTEGER_LITERAL 1))))))") << "Each loop must increment its own variable before its own continue.";
}

TEST(MAREAIrControlFlow, ForRangeEvaluatesTheLastBoundOnce)
{
    GTEST_COUT << "Validating a for-range loop evaluates a call in its last bound once, into a hidden local before the loop." << std::endl;

    const std::string rendered{ MAREA_lowerBody("function bound(): integer32 return 3; end", "", "for index in 0 .. bound() do end") };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $last (CALL_EXPRESSION"), std::string::npos) << "The bound call must sit in the hidden local: " << rendered;

    EXPECT_EQ(rendered.find("(IDENTIFIER bound)", rendered.find("(WHILE_STATEMENT")), std::string::npos) << "The loop must not call the bound again: " << rendered;
}

TEST(MAREAIrControlFlow, ForRangeLeavesBeforeTheIncrementAtTheLastValue)
{
    GTEST_COUT << "Validating the loop breaks when the variable equals the hidden bound, so the increment never passes the type maximum." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "", "for index in 254 .. 255 do end"), "(BLOCK (BLOCK (LOCAL_DECLARATION index flags=1 (INTEGER_LITERAL 254)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 255)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1))))))") << "The break must come before the increment.";
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

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32", "switch a case 1 then return; default then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (RETURN_STATEMENT)) (ELSE_CLAUSE (BLOCK (RETURN_STATEMENT))))))") << "The switch must keep the shape of its desugaring.";
}

TEST(MAREAIrControlFlow, SwitchCasesAfterTheFirstBecomeElseifClauses)
{
    GTEST_COUT << "Validating each case after the first becomes an elseif clause and a switch without default has no else." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32", "switch a case 1 then return; case 2 then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 2)) (BLOCK (RETURN_STATEMENT))))))") << "A second case must be an elseif clause, and no default means no else.";
}

TEST(MAREAIrControlFlow, SwitchCaseWithManyValuesJoinsThemWithOr)
{
    GTEST_COUT << "Validating a case that lists three values joins the comparisons with or, grouped to the left." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32", "switch a case 1, 2, 3 then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=KEYWORD_OR (BINARY_EXPRESSION op=KEYWORD_OR (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 2))) (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 3))) (BLOCK (RETURN_STATEMENT)))))") << "Many values must become one condition with or.";
}

TEST(MAREAIrControlFlow, SwitchWithOnlyADefaultRunsItsBlockAfterTheHiddenLocal)
{
    GTEST_COUT << "Validating a switch with only a default keeps the scrutinee local and then the default block." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32", "switch a default then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (BLOCK (RETURN_STATEMENT))))") << "A default alone has no condition to test.";
}

TEST(MAREAIrControlFlow, SwitchWithoutArmsKeepsOnlyTheScrutinee)
{
    GTEST_COUT << "Validating a switch with no arm still evaluates its scrutinee once, into the hidden local." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("function ping(): integer32 return 1; end", "", "switch ping() end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (CALL_EXPRESSION :integer32 (IDENTIFIER ping) (ARGUMENT_LIST)))))") << "An empty switch must keep the call.";
}

TEST(MAREAIrControlFlow, SwitchEvaluatesTheScrutineeOnce)
{
    GTEST_COUT << "Validating a call as the scrutinee is evaluated once, into the hidden local, and never again per case." << std::endl;

    const std::string rendered{ MAREA_lowerBody("function pick(): integer32 return 1; end", "", "switch pick() case 1 then return; case 2 then return; end") };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $switch (CALL_EXPRESSION"), std::string::npos) << "The call must sit in the hidden local: " << rendered;

    EXPECT_EQ(rendered.find("(IDENTIFIER pick)", rendered.find("(IF_STATEMENT")), std::string::npos) << "The cases must not call the scrutinee again: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchOnAnEnumComparesWithTheVariantAccess)
{
    GTEST_COUT << "Validating a switch on an enum compares the hidden local with the lowered variant access." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("enum Color red; green; end", "c: Color", "switch c case Color.red then return; case Color.green then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER c)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (MEMBER_ACCESS (IDENTIFIER Color) (IDENTIFIER red))) (BLOCK (RETURN_STATEMENT)) (ELSEIF_CLAUSE (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (MEMBER_ACCESS (IDENTIFIER Color) (IDENTIFIER green))) (BLOCK (RETURN_STATEMENT))))))") << "A variant case must compare with the variant access.";
}

TEST(MAREAIrControlFlow, SwitchNestedInAnArmKeepsItsOwnHiddenLocal)
{
    GTEST_COUT << "Validating a switch nested in an arm gets its own scope and hidden local, so the two never mix." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32, b: integer32", "switch a case 1 then switch b case 2 then return; end default then return; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER b)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 2)) (BLOCK (RETURN_STATEMENT))))) (ELSE_CLAUSE (BLOCK (RETURN_STATEMENT))))))") << "The inner switch must sit in its own scope.";
}

TEST(MAREAIrControlFlow, SwitchArmBreakAndContinueTargetTheEnclosingLoop)
{
    GTEST_COUT << "Validating a break in a switch arm stays a break and a continue in an arm of a for-range body still runs the step first." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("", "a: integer32", "while true do switch a case 1 then break; default then continue; end end"), "(BLOCK (WHILE_STATEMENT (BOOLEAN_LITERAL true) (BLOCK (BLOCK (LOCAL_DECLARATION $switch (IDENTIFIER a)) (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER $switch) (INTEGER_LITERAL 1)) (BLOCK (BREAK_STATEMENT)) (ELSE_CLAUSE (BLOCK (CONTINUE_STATEMENT))))))))") << "An arm must not swallow a break or a continue.";

    const std::string rendered{ MAREA_lowerBody("", "a: integer32", "for index in 0 .. 3 do switch a case 1 then continue; end end") };

    EXPECT_NE(rendered.find("(BLOCK (IF_STATEMENT (BINARY_EXPRESSION op=EQUAL_EQUAL (IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))"), std::string::npos) << "The continue inside the arm must carry the step of the for-range loop: " << rendered;
}

TEST(MAREAIrControlFlow, SwitchArmWithAnUnsupportedStatementDropsTheFunction)
{
    GTEST_COUT << "Validating a gap inside a switch arm still drops the whole function and raises its diagnostic once." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(fixture.MAREA_lower("namespace main; function keep() end function lose(a: integer32) switch a case 1 then print!(\"hi\"); default then return; end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the lowerable function must remain.";

    EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The macro in the arm must raise its own code once.";
}

TEST(MAREAIrControlFlow, ForEachBecomesAHiddenCollectionALengthAndAnIndexedWhile)
{
    GTEST_COUT << "Validating a for-each loop desugars to a hidden collection, a hidden length, a hidden index and a while over the index." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.array;", "values: array<integer32>", "local total: integer32 = 0; for each value in values do total += value; end"), "(BLOCK (LOCAL_DECLARATION total :integer32 (INTEGER_LITERAL 0)) (BLOCK (LOCAL_DECLARATION $each (IDENTIFIER values)) (LOCAL_DECLARATION $count (CALL_EXPRESSION (IDENTIFIER $length) (ARGUMENT_LIST (IDENTIFIER $each)))) (LOCAL_DECLARATION $index flags=1 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER $index) (IDENTIFIER $count)) (BLOCK (LOCAL_DECLARATION value (INDEX_ACCESS (IDENTIFIER $each) (IDENTIFIER $index))) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER total) (IDENTIFIER value))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1))))))") << "The loop must keep the shape of its desugaring.";
}

TEST(MAREAIrControlFlow, ForEachWithAnEmptyBodyStillAdvancesTheIndex)
{
    GTEST_COUT << "Validating a for-each loop with an empty body keeps an empty block and the increment of the index." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.array;", "values: array<integer32>", "for each value in values do end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $each (IDENTIFIER values)) (LOCAL_DECLARATION $count (CALL_EXPRESSION (IDENTIFIER $length) (ARGUMENT_LIST (IDENTIFIER $each)))) (LOCAL_DECLARATION $index flags=1 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER $index) (IDENTIFIER $count)) (BLOCK (LOCAL_DECLARATION value (INDEX_ACCESS (IDENTIFIER $each) (IDENTIFIER $index))) (BLOCK) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1))))))") << "An empty body must still advance the index, or the loop would never end.";
}

TEST(MAREAIrControlFlow, ForEachWithKeyAndValueBindsTheKeyToTheIndex)
{
    GTEST_COUT << "Validating the key of a for-each loop with two names is a local set to the hidden index, before the value." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.array;", "values: array<integer32>", "for each key, value in values do end") };

    EXPECT_NE(rendered.find("(BLOCK (LOCAL_DECLARATION key (IDENTIFIER $index)) (LOCAL_DECLARATION value (INDEX_ACCESS (IDENTIFIER $each) (IDENTIFIER $index))) (BLOCK) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1)))"), std::string::npos) << "The key must come first and read the index: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachMutableMarksTheHiddenCollectionAndTheElementOnly)
{
    GTEST_COUT << "Validating a for-each loop over a mutable collection sets MUTABLE on the hidden collection and the element, and on no other node it makes." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.array;", "values: mutable array<integer32>", "for each key, value in mutable values do value += 1; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $each flags=1 (IDENTIFIER values)) (LOCAL_DECLARATION $count (CALL_EXPRESSION (IDENTIFIER $length) (ARGUMENT_LIST (IDENTIFIER $each)))) (LOCAL_DECLARATION $index flags=1 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER $index) (IDENTIFIER $count)) (BLOCK (LOCAL_DECLARATION key (IDENTIFIER $index)) (LOCAL_DECLARATION value flags=1 (INDEX_ACCESS (IDENTIFIER $each) (IDENTIFIER $index))) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER value) (INTEGER_LITERAL 1))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1))))))") << "Only the collection and the element may carry MUTABLE; the key and every synthetic node must not.";
}

TEST(MAREAIrControlFlow, ForEachEvaluatesTheCollectionOnce)
{
    GTEST_COUT << "Validating a for-each loop evaluates a call in its collection once, into the hidden collection before the loop." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.array; function source(): array<integer32> return source(); end", "", "for each value in source() do end") };

    EXPECT_NE(rendered.find("(LOCAL_DECLARATION $each (CALL_EXPRESSION"), std::string::npos) << "The call must sit in the hidden collection: " << rendered;

    EXPECT_EQ(rendered.find("(IDENTIFIER source)", rendered.find("(WHILE_STATEMENT")), std::string::npos) << "The loop must not call the collection again: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachContinueRunsTheStepFirst)
{
    GTEST_COUT << "Validating a continue in a for-each body is preceded by the increment of the index, and a break stays as it is." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.array;", "values: array<integer32>, a: boolean", "for each value in values do if a then continue; end if a then break; end end") };

    EXPECT_NE(rendered.find("(IF_STATEMENT (IDENTIFIER a) (BLOCK (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))))"), std::string::npos) << "The continue must run the step first: " << rendered;

    EXPECT_NE(rendered.find("(IF_STATEMENT (IDENTIFIER a) (BLOCK (BREAK_STATEMENT)))"), std::string::npos) << "The break must stay as it is: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachContinueOfAnInnerWhileIsLeftAlone)
{
    GTEST_COUT << "Validating a continue that targets a while loop inside a for-each body gets no increment of the for-each loop." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.array;", "values: array<integer32>, a: boolean", "for each value in values do while a do continue; end end") };

    EXPECT_NE(rendered.find("(BLOCK (WHILE_STATEMENT (IDENTIFIER a) (BLOCK (CONTINUE_STATEMENT)))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1))"), std::string::npos) << "A continue of an inner loop must stay as it is: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachNestedInAForEachKeepsItsOwnHiddenLocals)
{
    GTEST_COUT << "Validating a for-each loop nested in another gets its own scope and hidden locals, and each continue runs its own step." << std::endl;

    EXPECT_EQ(MAREA_lowerBody("import collections.array;", "rows: array<array<integer32>>", "for each row in rows do for each cell in row do continue; end continue; end"), "(BLOCK (BLOCK (LOCAL_DECLARATION $each (IDENTIFIER rows)) (LOCAL_DECLARATION $count (CALL_EXPRESSION (IDENTIFIER $length) (ARGUMENT_LIST (IDENTIFIER $each)))) (LOCAL_DECLARATION $index flags=1 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER $index) (IDENTIFIER $count)) (BLOCK (LOCAL_DECLARATION row (INDEX_ACCESS (IDENTIFIER $each) (IDENTIFIER $index))) (BLOCK (BLOCK (LOCAL_DECLARATION $each (IDENTIFIER row)) (LOCAL_DECLARATION $count (CALL_EXPRESSION (IDENTIFIER $length) (ARGUMENT_LIST (IDENTIFIER $each)))) (LOCAL_DECLARATION $index flags=1 (INTEGER_LITERAL 0)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS (IDENTIFIER $index) (IDENTIFIER $count)) (BLOCK (LOCAL_DECLARATION cell (INDEX_ACCESS (IDENTIFIER $each) (IDENTIFIER $index))) (BLOCK (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1))))) (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1))))))") << "Each loop must read and step its own hidden index.";
}

TEST(MAREAIrControlFlow, ForEachContinueInsideAForRangeBodyUsesTheStepOfTheForEachOnly)
{
    GTEST_COUT << "Validating a continue inside a for-each nested in a for-range loop gets the for-each step, and not the for-range step." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.array;", "values: array<integer32>", "for index in 0 .. 3 do for each value in values do continue; end end") };

    EXPECT_NE(rendered.find("(BLOCK (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)))"), std::string::npos) << "The continue must run the for-each step: " << rendered;

    EXPECT_EQ(rendered.find("(CONTINUE_STATEMENT)", rendered.find("(CONTINUE_STATEMENT)") + 1), std::string::npos) << "The loop must hold a single continue: " << rendered;

    EXPECT_EQ(rendered.find("(IDENTIFIER index) (IDENTIFIER $last)) (BLOCK (BREAK_STATEMENT))) (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT)"), std::string::npos) << "The continue must not carry the for-range step: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachSyntheticNodesTakeTheirPositionFromTheLoop)
{
    GTEST_COUT << "Validating the hidden locals sit at the position of the for statement and the element local at its name." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; function run(values: array<integer32>) for each value in values do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *scope{ MAREA_irChild(MAREA_irChild(MAREA_irChild(fixture.module, 0), 1), 0) };

    ASSERT_EQ(MAREA_IrNodeGetKind(scope), MAREA_IR_KIND_BLOCK) << "The loop must sit in its own scope.";

    const MAREA_SourcePosition LOOP{ MAREA_IrNodeGetPosition(scope) };

    for (MAREA_IntegerSize index{ 0 }; index < 3; ++index) {
        EXPECT_EQ(MAREA_IrNodeGetPosition(MAREA_irChild(scope, index)).column, LOOP.column) << "The hidden local " << index << " must sit at the for statement.";
    }

    const MAREA_IrNode *element{ MAREA_irChild(MAREA_irChild(MAREA_irChild(scope, 3), 1), 0) };

    EXPECT_EQ(MAREA_irText(element), "value") << "The element local must carry the loop variable name.";

    EXPECT_GT(MAREA_IrNodeGetPosition(element).column, LOOP.column) << "The element local must sit at its name, after the for keyword.";
}

TEST(MAREAIrControlFlow, ForEachWithAnUnsupportedStatementDropsTheFunction)
{
    GTEST_COUT << "Validating a gap inside a for-each body still drops the whole function and raises its diagnostic once." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; function keep() end function lose(values: array<integer32>) for each value in values do print!(\"hi\"); end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 1U) << "Only the lowerable function must remain.";

    EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO }) << "The macro in the body must raise its own code once.";
}

TEST(MAREAIrControlFlow, ForEachWithAGapInTheCollectionDropsTheFunction)
{
    GTEST_COUT << "Validating a gap inside the collection expression drops the function and raises its own diagnostic once." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function lose(values: array<integer32>) for each value in identity(values) do end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    ASSERT_EQ(MAREA_IrNodeGetChildCount(fixture.module), 0U) << "Neither function may remain.";

    EXPECT_EQ(fixture.MAREA_irCodes(), (std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC })) << "The generic function and its call must each raise 4004 once, and for each must raise nothing.";
}

TEST(MAREAIrControlFlow, ForEachMutableContinueWrapperCarriesNoFlag)
{
    GTEST_COUT << "Validating the block that holds the step before a continue in a mutable for-each loop carries no MUTABLE flag." << std::endl;

    const std::string rendered{ MAREA_lowerBody("import collections.array;", "values: mutable array<integer32>, a: boolean", "for each value in mutable values do if a then continue; end end") };

    EXPECT_NE(rendered.find("(IF_STATEMENT (IDENTIFIER a) (BLOCK (BLOCK (ASSIGNMENT op=PLUS_EQUAL (IDENTIFIER $index) (INTEGER_LITERAL 1)) (CONTINUE_STATEMENT))))"), std::string::npos) << "The wrapper of the continue must carry no flag: " << rendered;
}

TEST(MAREAIrControlFlow, ForEachWithAGapInTheCollectionDoesNotLowerItsBody)
{
    GTEST_COUT << "Validating a for-each loop whose collection holds a gap reports that gap only, and not the gap of its body." << std::endl;

    MAREA_IrFixture fixture(RESOLVE_EXTERNAL);

    ASSERT_EQ(fixture.MAREA_lower("namespace main; import collections.array; function identity<Item>(value: Item): Item where Item: Copyable return copy value; end function lose(values: array<integer32>) for each value in identity(values) do print!(\"hi\"); end end"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    EXPECT_EQ(fixture.MAREA_irCodes(), (std::vector<MAREA_Uinteger32>{ MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC })) << "Only the gaps of the collection may be reported.";
}

TEST(MAREAIrControlFlow, ForEachExamplesLowerTheirLoops)
{
    GTEST_COUT << "Validating sum_array of basics/control_flow.marea and the two worker functions of parallel/scaling.marea are lowered with their for-each loops." << std::endl;

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

            EXPECT_NE(MAREA_irRender(MAREA_irChild(fixture.module, index)).find("(LOCAL_DECLARATION $each "), std::string::npos) << "The function " << name << " must hold the desugared for each.";
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

        EXPECT_NE(rendered.find("(LOCAL_DECLARATION index flags=1 (INTEGER_LITERAL 0)) (LOCAL_DECLARATION $last (INTEGER_LITERAL 3)) (WHILE_STATEMENT (BINARY_EXPRESSION op=LESS_EQUAL (IDENTIFIER index) (IDENTIFIER $last))"), std::string::npos) << "The loop must be desugared: " << rendered;
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

            EXPECT_NE(MAREA_irRender(MAREA_irChild(fixture.module, index)).find("(LOCAL_DECLARATION $last (IDENTIFIER last))"), std::string::npos) << "The range must include its last value.";
        }
    }

    EXPECT_TRUE(sumRange) << "The function sum_range must be lowered.";
}
