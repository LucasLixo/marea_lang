// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

// INFO: the text of the synthetic increment of a for-range loop.
static const MAREA_Character8 MAREA_G_IncrementText[] = "1";

// INFO: the name of the hidden local that holds the last value of a for-range loop; the dollar sign cannot start a source identifier.
static const MAREA_Character8 MAREA_G_BoundName[] = "$last";

// INFO: the name of the hidden local that holds the scrutinee of a switch; the dollar sign cannot start a source identifier.
static const MAREA_Character8 MAREA_G_SwitchName[] = "$switch";

// INFO: the place of the bound name among the children of a with statement (value, name, body) and of a partition statement (value, count, name, body).
static const MAREA_IntegerSize MAREA_G_WithNameIndex = MAREA_INTEGER_SIZE(1);
static const MAREA_IntegerSize MAREA_G_PartitionNameIndex = MAREA_INTEGER_SIZE(2);

// INFO: how a for-range loop goes to its next pass: the loop it comes from and the name it adds one to.
typedef struct MAREA_G_Step
{
    const MAREA_AstNode *loopAst;
    const MAREA_AstNode *nameAst;
} MAREA_G_Step;

// INFO: a child that lowers to nothing makes the whole parent lower to nothing, so no partial tree is produced.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerChildrenInto(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean *outComplete)
{
    *outComplete = MAREA_BOOLEAN_TRUE;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_IrNode *lowered = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_G_LowerStatement(context, MAREA_IrTypedChildAt(typed, index), &lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        if (lowered == MAREA_NULLPTR) {
            *outComplete = MAREA_BOOLEAN_FALSE;

            return MAREA_IR_EXCEPT_SUCCESS;
        }

        result = MAREA_IrNodeAppendChild(context, node, lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerKept(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, kind, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_G_LowerChildrenInto(context, typed, node, &complete);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_TRUE)) {
        *outNode = node;
    }

    return result;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerLocal(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_LOCAL_DECLARATION, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_IrAppendInitializer(context, typed, node, &complete);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_TRUE)) {
        *outNode = node;
    }

    return result;
}

// INFO: unreachable!() is the one built-in macro that has a lowering; every other macro is a Gap.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerMacroStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *call = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_MACRO_CALL);

    const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(call, MAREA_AST_KIND_IDENTIFIER);

    if (name == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

    const MAREA_IntegerSize LENGTH = strlen("unreachable");

    if ((MAREA_AstNodeGetTextLength(nameAst) != LENGTH) || (memcmp(MAREA_AstNodeGetText(nameAst), "unreachable", LENGTH) != 0)) {
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO, MAREA_SemaTypedNodeGetAstNode(typed), "Macro is not supported by the IR yet:", MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));
    }

    return MAREA_IrNodeCreate(context, MAREA_IR_KIND_UNREACHABLE_STATEMENT, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
}

// INFO: a synthetic node keeps only the position of the AST node it stands for, so its text and flags are cleared.
static void MAREA_CALL MAREA_G_KeepOnlyPosition(MAREA_IrNode *node)
{
    node->text = MAREA_NULLPTR;
    node->textLength = MAREA_UINTEGER_32(0);
    node->flags = MAREA_UINTEGER_32(0);
}

// INFO: builds an IDENTIFIER that reads a for-range variable; a null text keeps the text of the AST node.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateReference(MAREA_IrContext *context, const MAREA_AstNode *ast, const MAREA_Character8 *text, MAREA_IrNode **outReference)
{
    MAREA_IrNode *reference = MAREA_NULLPTR;

    const MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, ast, &reference);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (text != MAREA_NULLPTR) {
        // INFO: a hidden name stands for no source token, so it must not inherit the flags of the loop it comes from.
        MAREA_G_KeepOnlyPosition(reference);
        reference->text = text;
        reference->textLength = (MAREA_Uinteger32)strlen(text);
    }

    *outReference = reference;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: appends IF name == hidden THEN BREAK to node. The break comes first, so a later increment never runs past the last value, even when it is the maximum of the type.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendLeave(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_G_Step *step)
{
    MAREA_IrNode *guard = MAREA_NULLPTR;
    MAREA_IrNode *equality = MAREA_NULLPTR;
    MAREA_IrNode *leave = MAREA_NULLPTR;
    MAREA_IrNode *breakNode = MAREA_NULLPTR;
    MAREA_IrNode *nameReference = MAREA_NULLPTR;
    MAREA_IrNode *boundReference = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IF_STATEMENT, step->loopAst, &guard);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BINARY_EXPRESSION, step->loopAst, &equality);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, step->loopAst, &leave);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BREAK_STATEMENT, step->loopAst, &breakNode);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, step->nameAst, MAREA_NULLPTR, &nameReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, step->loopAst, MAREA_G_BoundName, &boundReference);
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    equality->operatorKind = MAREA_TOKEN_KIND_EQUAL_EQUAL;

    MAREA_IrNode *parents[] = { equality, equality, leave, guard, guard, node };
    MAREA_IrNode *children[] = { nameReference, boundReference, breakNode, equality, leave, guard };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(parents); index = (index + MAREA_INTEGER_SIZE(1))) {
        result = MAREA_IrNodeAppendChild(context, parents[index], children[index]);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: appends name += 1 to node.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendIncrement(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_G_Step *step)
{
    MAREA_IrNode *increment = MAREA_NULLPTR;
    MAREA_IrNode *incrementReference = MAREA_NULLPTR;
    MAREA_IrNode *one = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_ASSIGNMENT, step->loopAst, &increment);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, step->nameAst, MAREA_NULLPTR, &incrementReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_INTEGER_LITERAL, step->loopAst, &one);
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_G_KeepOnlyPosition(increment);
    MAREA_G_KeepOnlyPosition(one);

    increment->operatorKind = MAREA_TOKEN_KIND_PLUS_EQUAL;
    one->text = MAREA_G_IncrementText;
    one->textLength = (MAREA_Uinteger32)strlen(MAREA_G_IncrementText);

    MAREA_IrNode *parents[] = { increment, increment, node };
    MAREA_IrNode *children[] = { incrementReference, one, increment };

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(parents); index = (index + MAREA_INTEGER_SIZE(1))) {
        result = MAREA_IrNodeAppendChild(context, parents[index], children[index]);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: appends the step of a for-range loop to node: the leave at the bound, then the increment.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendStep(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_G_Step *step)
{
    const MAREA_IrExcept result = MAREA_G_AppendLeave(context, node, step);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    return MAREA_G_AppendIncrement(context, node, step);
}

// INFO: a continue skips the step at the end of the body, so each continue that targets this loop becomes BLOCK(step, continue).
// A nested while or for each stops the walk, because a continue inside it targets that loop. A nested for-range is a while by now, so it stops it too.
static MAREA_IrExcept MAREA_CALL MAREA_G_PrefixContinues(MAREA_IrContext *context, MAREA_IrNode *node, const MAREA_G_Step *step)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < node->childCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_IrNode *child = node->children[index];

        if ((child->kind == MAREA_IR_KIND_WHILE_STATEMENT) || (child->kind == MAREA_IR_KIND_FOR_EACH_STATEMENT)) {
            continue;
        }

        if (child->kind != MAREA_IR_KIND_CONTINUE_STATEMENT) {
            const MAREA_IrExcept result = MAREA_G_PrefixContinues(context, child, step);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }

            continue;
        }

        MAREA_IrNode *wrapper = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, step->loopAst, &wrapper);

        if (result == MAREA_IR_EXCEPT_SUCCESS) {
            MAREA_G_KeepOnlyPosition(wrapper);
            result = MAREA_G_AppendStep(context, wrapper, step);
        }

        if (result == MAREA_IR_EXCEPT_SUCCESS) {
            result = MAREA_IrNodeAppendChild(context, wrapper, child);
        }

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        node->children[index] = wrapper;
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: for name in first .. last do body end becomes BLOCK(LOCAL name = first, LOCAL $last = last, WHILE name <= $last: body, step).
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerForRange(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_AstNode *forAst = MAREA_SemaTypedNodeGetAstNode(typed);

    const MAREA_SemaTypedNode *nameTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    const MAREA_SemaTypedNode *firstTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(1));

    const MAREA_SemaTypedNode *lastTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(2));

    const MAREA_SemaTypedNode *bodyTyped = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(3));

    if ((nameTyped == MAREA_NULLPTR) || (firstTyped == MAREA_NULLPTR) || (lastTyped == MAREA_NULLPTR) || (bodyTyped == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(nameTyped);

    MAREA_IrNode *first = MAREA_NULLPTR;
    MAREA_IrNode *last = MAREA_NULLPTR;
    MAREA_IrNode *body = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, firstTyped, &first);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrLowerExpression(context, lastTyped, &last);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrLowerBlock(context, bodyTyped, &body);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if ((first == MAREA_NULLPTR) || (last == MAREA_NULLPTR) || (body == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_G_Step STEP = { .loopAst = forAst, .nameAst = nameAst };

    MAREA_IrNode *outer = MAREA_NULLPTR;
    MAREA_IrNode *local = MAREA_NULLPTR;
    MAREA_IrNode *bound = MAREA_NULLPTR;
    MAREA_IrNode *boundReference = MAREA_NULLPTR;
    MAREA_IrNode *loop = MAREA_NULLPTR;
    MAREA_IrNode *condition = MAREA_NULLPTR;
    MAREA_IrNode *reference = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, forAst, &outer);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_LOCAL_DECLARATION, nameAst, &local);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_LOCAL_DECLARATION, forAst, &bound);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_WHILE_STATEMENT, forAst, &loop);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BINARY_EXPRESSION, forAst, &condition);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_PrefixContinues(context, body, &STEP);
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    bound->text = MAREA_G_BoundName;
    bound->textLength = (MAREA_Uinteger32)strlen(MAREA_G_BoundName);

    // INFO: the loop local is written by the increment, so it is a write target; the hidden bound is never written.
    local->flags = (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE;

    // INFO: a range includes both ends (docs/grammar/marea.ebnf), so the bound is <=, not <.
    condition->operatorKind = MAREA_TOKEN_KIND_LESS_EQUAL;
    result = MAREA_IrNodeAppendChild(context, local, first);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, bound, last);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, nameAst, MAREA_NULLPTR, &reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, forAst, MAREA_G_BoundName, &boundReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, condition, reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, condition, boundReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_AppendStep(context, body, &STEP);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, loop, condition);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, loop, body);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, local);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, bound);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, loop);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *outNode = outer;
    }

    return result;
}

// INFO: builds the test of one case value: $switch == value, positioned at the value.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateComparison(MAREA_IrContext *context, const MAREA_AstNode *switchAst, const MAREA_SemaTypedNode *valueTyped, MAREA_IrNode **outComparison)
{
    const MAREA_AstNode *valueAst = MAREA_SemaTypedNodeGetAstNode(valueTyped);

    MAREA_IrNode *value = MAREA_NULLPTR;
    MAREA_IrNode *comparison = MAREA_NULLPTR;
    MAREA_IrNode *reference = MAREA_NULLPTR;

    *outComparison = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, valueTyped, &value);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (value == MAREA_NULLPTR)) {
        return result;
    }

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BINARY_EXPRESSION, valueAst, &comparison);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, switchAst, MAREA_G_SwitchName, &reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        MAREA_G_KeepOnlyPosition(comparison);
        comparison->operatorKind = MAREA_TOKEN_KIND_EQUAL_EQUAL;
        result = MAREA_IrNodeAppendChild(context, comparison, reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, comparison, value);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *outComparison = comparison;
    }

    return result;
}

// INFO: joins the tests of every value of a case with or, grouped to the left. All children of the arm but the last are values.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateCondition(MAREA_IrContext *context, const MAREA_SemaTypedNode *armTyped, const MAREA_AstNode *switchAst, MAREA_IrNode **outCondition)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(armTyped);

    MAREA_IrNode *condition = MAREA_NULLPTR;

    *outCondition = MAREA_NULLPTR;

    // INFO: the loop stops before the last child, which is the body; the form (index + 1) < COUNT cannot underflow on an empty arm.
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index + MAREA_INTEGER_SIZE(1)) < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *valueTyped = MAREA_IrTypedChildAt(armTyped, index);

        MAREA_IrNode *comparison = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_G_CreateComparison(context, switchAst, valueTyped, &comparison);

        if ((result != MAREA_IR_EXCEPT_SUCCESS) || (comparison == MAREA_NULLPTR)) {
            return result;
        }

        if (condition == MAREA_NULLPTR) {
            condition = comparison;

            continue;
        }

        MAREA_IrNode *joined = MAREA_NULLPTR;

        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BINARY_EXPRESSION, MAREA_SemaTypedNodeGetAstNode(valueTyped), &joined);

        if (result == MAREA_IR_EXCEPT_SUCCESS) {
            MAREA_G_KeepOnlyPosition(joined);
            joined->operatorKind = MAREA_TOKEN_KIND_KEYWORD_OR;
            result = MAREA_IrNodeAppendChild(context, joined, condition);
        }

        if (result == MAREA_IR_EXCEPT_SUCCESS) {
            result = MAREA_IrNodeAppendChild(context, joined, comparison);
        }

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        condition = joined;
    }

    *outCondition = condition;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: lowers the BLOCK of an arm, which is its last child. A null body means a gap inside it, already reported.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerArmBody(MAREA_IrContext *context, const MAREA_SemaTypedNode *armTyped, MAREA_IrNode **outBody)
{
    *outBody = MAREA_NULLPTR;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(armTyped);

    if (COUNT == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_IrLowerBlock(context, MAREA_IrTypedChildAt(armTyped, (COUNT - MAREA_INTEGER_SIZE(1))), outBody);
}

// INFO: one case becomes an IF_STATEMENT, or an ELSEIF_CLAUSE of the chain that already started; condition then body.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendCase(MAREA_IrContext *context, const MAREA_SemaTypedNode *armTyped, const MAREA_AstNode *switchAst, MAREA_IrNode **chain, MAREA_Boolean *outComplete)
{
    MAREA_IrNode *condition = MAREA_NULLPTR;
    MAREA_IrNode *body = MAREA_NULLPTR;
    MAREA_IrNode *clause = MAREA_NULLPTR;

    *outComplete = MAREA_BOOLEAN_FALSE;

    MAREA_IrExcept result = MAREA_G_CreateCondition(context, armTyped, switchAst, &condition);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (condition == MAREA_NULLPTR)) {
        return result;
    }

    result = MAREA_G_LowerArmBody(context, armTyped, &body);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (body == MAREA_NULLPTR)) {
        return result;
    }

    const MAREA_IrKind KIND = (*chain == MAREA_NULLPTR) ? MAREA_IR_KIND_IF_STATEMENT : MAREA_IR_KIND_ELSEIF_CLAUSE;

    result = MAREA_IrNodeCreate(context, KIND, MAREA_SemaTypedNodeGetAstNode(armTyped), &clause);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, clause, condition);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, clause, body);
    }

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (*chain != MAREA_NULLPTR)) {
        result = MAREA_IrNodeAppendChild(context, *chain, clause);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *chain = (*chain == MAREA_NULLPTR) ? clause : *chain;
        *outComplete = MAREA_BOOLEAN_TRUE;
    }

    return result;
}

// INFO: the default arm is the ELSE_CLAUSE of the chain, or, when there is no case, its block runs after the hidden local.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendDefault(MAREA_IrContext *context, const MAREA_AstNode *defaultAst, MAREA_IrNode *defaultBody, MAREA_IrNode *chain, MAREA_IrNode *outer)
{
    if (chain == MAREA_NULLPTR) {
        return MAREA_IrNodeAppendChild(context, outer, defaultBody);
    }

    MAREA_IrNode *clause = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_ELSE_CLAUSE, defaultAst, &clause);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, clause, defaultBody);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, chain, clause);
    }

    return result;
}

// INFO: creates the BLOCK that scopes a switch, holding the hidden local $switch set to the scrutinee, which is evaluated once.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateSwitchScope(MAREA_IrContext *context, const MAREA_AstNode *switchAst, MAREA_IrNode *scrutinee, MAREA_IrNode **outScope)
{
    MAREA_IrNode *outer = MAREA_NULLPTR;
    MAREA_IrNode *local = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, switchAst, &outer);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_LOCAL_DECLARATION, switchAst, &local);
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    local->text = MAREA_G_SwitchName;
    local->textLength = (MAREA_Uinteger32)strlen(MAREA_G_SwitchName);

    result = MAREA_IrNodeAppendChild(context, local, scrutinee);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, local);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *outScope = outer;
    }

    return result;
}

// INFO: switch value case a then A case b, c then B default then C end becomes BLOCK(LOCAL $switch = value, IF $switch == a: A, ELSEIF $switch == b or $switch == c: B, ELSE: C).
// INFO: an arm does not fall through, so the chain is exact; a break or continue in an arm targets the enclosing loop, as it does in an if.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerSwitch(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_AstNode *switchAst = MAREA_SemaTypedNodeGetAstNode(typed);

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    MAREA_IrNode *scrutinee = MAREA_NULLPTR;
    MAREA_IrNode *outer = MAREA_NULLPTR;
    MAREA_IrNode *chain = MAREA_NULLPTR;
    MAREA_IrNode *defaultBody = MAREA_NULLPTR;
    const MAREA_AstNode *defaultAst = MAREA_NULLPTR;

    if (COUNT == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0)), &scrutinee);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (scrutinee == MAREA_NULLPTR)) {
        return result;
    }

    result = MAREA_G_CreateSwitchScope(context, switchAst, scrutinee, &outer);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(1); (result == MAREA_IR_EXCEPT_SUCCESS) && (index < COUNT); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *armTyped = MAREA_IrTypedChildAt(typed, index);

        MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

        if (MAREA_IrTypedKind(armTyped) == MAREA_AST_KIND_DEFAULT_ARM) {
            defaultAst = MAREA_SemaTypedNodeGetAstNode(armTyped);
            result = MAREA_G_LowerArmBody(context, armTyped, &defaultBody);
            complete = (defaultBody != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        } else {
            result = MAREA_G_AppendCase(context, armTyped, switchAst, &chain, &complete);
        }

        if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_FALSE)) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }
    }

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (defaultBody != MAREA_NULLPTR)) {
        result = MAREA_G_AppendDefault(context, defaultAst, defaultBody, chain, outer);
    }

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (chain != MAREA_NULLPTR)) {
        result = MAREA_IrNodeAppendChild(context, outer, chain);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *outNode = outer;
    }

    return result;
}

// INFO: a with, partition or for each statement keeps its children but the value name. The name is the text of the node, as for a declaration, and MUTABLE is the one flag that stays.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerBoundScope(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_IntegerSize nameIndex, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *nameTyped = MAREA_IrTypedChildAt(typed, nameIndex);

    if (MAREA_IrTypedKind(nameTyped) != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, kind, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(nameTyped);

    result = MAREA_IrNodeSetQualifiedText(context, node, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    node->flags = (node->flags & (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE);

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (index == nameIndex) {
            continue;
        }

        MAREA_IrNode *lowered = MAREA_NULLPTR;

        result = MAREA_G_LowerStatement(context, MAREA_IrTypedChildAt(typed, index), &lowered);

        if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR)) {
            return result;
        }

        result = MAREA_IrNodeAppendChild(context, node, lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: for each key, value in collection do body end becomes FOR_EACH_STATEMENT "value" (children: IDENTIFIER key when written, collection, BLOCK body), with MUTABLE kept.
// INFO: the node walks the collection, so a backend chooses how from its type. A gap in the collection is reported once, and the body is not lowered after it.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerForEach(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    // INFO: the children are one or two names, the collection and the body, in that order; the value name is the third from the end.
    if ((COUNT < MAREA_INTEGER_SIZE(3)) || (COUNT > MAREA_INTEGER_SIZE(4))) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_G_LowerBoundScope(context, MAREA_IR_KIND_FOR_EACH_STATEMENT, typed, (COUNT - MAREA_INTEGER_SIZE(3)), outNode);
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    *outNode = MAREA_NULLPTR;

    switch (MAREA_IrTypedKind(typed)) {
    case MAREA_AST_KIND_BLOCK:
        return MAREA_IrLowerBlock(context, typed, outNode);
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return MAREA_G_LowerLocal(context, typed, outNode);
    case MAREA_AST_KIND_ASSIGNMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_ASSIGNMENT, typed, outNode);
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_EXPRESSION_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_IF_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_IF_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_ELSEIF_CLAUSE, typed, outNode);
    case MAREA_AST_KIND_ELSE_CLAUSE:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_ELSE_CLAUSE, typed, outNode);
    case MAREA_AST_KIND_WHILE_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_WHILE_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_RETURN_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_RETURN_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_BREAK_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_BREAK_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_CONTINUE_STATEMENT:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_CONTINUE_STATEMENT, typed, outNode);
    case MAREA_AST_KIND_SCOPE_BLOCK:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_SCOPE_BLOCK, typed, outNode);
    case MAREA_AST_KIND_UNSAFE_BLOCK:
        // INFO: the permission check already happened in sema, so an unsafe block is its plain BLOCK.
        return MAREA_IrLowerBlock(context, MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0)), outNode);
    case MAREA_AST_KIND_MACRO_STATEMENT:
        return MAREA_G_LowerMacroStatement(context, typed, outNode);
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return MAREA_G_LowerForRange(context, typed, outNode);
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_LowerSwitch(context, typed, outNode);
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        return MAREA_G_LowerForEach(context, typed, outNode);
    case MAREA_AST_KIND_FORK_JOIN_BLOCK:
        return MAREA_G_LowerKept(context, MAREA_IR_KIND_FORK_JOIN_BLOCK, typed, outNode);
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return MAREA_G_LowerBoundScope(context, MAREA_IR_KIND_PARTITION_STATEMENT, typed, MAREA_G_PartitionNameIndex, outNode);
    case MAREA_AST_KIND_WITH_STATEMENT:
        return MAREA_G_LowerBoundScope(context, MAREA_IR_KIND_WITH_STATEMENT, typed, MAREA_G_WithNameIndex, outNode);
    default:
        // INFO: any other kind is an expression; a Gap inside it raises its own diagnostic.
        return MAREA_IrLowerExpression(context, typed, outNode);
    }
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerBlock(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outNode == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    *outNode = MAREA_NULLPTR;

    if (MAREA_IrTypedKind(typed) != MAREA_AST_KIND_BLOCK) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_G_LowerKept(context, MAREA_IR_KIND_BLOCK, typed, outNode);
}
