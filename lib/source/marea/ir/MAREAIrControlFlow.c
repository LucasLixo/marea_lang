// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStatement(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

// INFO: the text of the synthetic increment of a for-range loop.
static const MAREA_Character8 MAREA_G_IncrementText[] = "1";

// INFO: the start of the name of the hidden local that holds the last value of a for-range loop; the dollar sign cannot start a source identifier. The number of the loop in its function follows it.
static const MAREA_Character8 MAREA_G_BoundPrefix[] = "$last";

// INFO: the most decimal digits of the largest unsigned integer of 64 bits.
#define MAREA_G_BOUND_DIGITS MAREA_INTEGER_SIZE(20)

// INFO: the start of the name of the hidden local that holds the scrutinee of a switch; the dollar sign cannot start a source identifier. The number of the switch in its function follows it.
static const MAREA_Character8 MAREA_G_SwitchPrefix[] = "$switch";

// INFO: the start of the name of the hidden local that holds the value of a return until the destroy calls ran; the dollar sign cannot start a source identifier. The number of the hoisted return in its function follows it.
static const MAREA_Character8 MAREA_G_ReturnPrefix[] = "$return";

// INFO: the place of the bound name among the children of a with statement (value, name, body) and of a partition statement (value, count, name, body).
static const MAREA_IntegerSize MAREA_G_WithNameIndex = MAREA_INTEGER_SIZE(1);
static const MAREA_IntegerSize MAREA_G_PartitionNameIndex = MAREA_INTEGER_SIZE(2);

// INFO: the type of every test the ir invents: a comparison and an or give a boolean.
static const MAREA_Character8 MAREA_G_TestType[] = "boolean";

// INFO: how a for-range loop goes to its next pass: the loop it comes from, the name it adds one to, the name of its hidden bound, and the two hidden or loop locals that give the types of what it reads.
typedef struct MAREA_G_Step
{
    const MAREA_AstNode *loopAst;
    const MAREA_AstNode *nameAst;
    const MAREA_Character8 *boundName;
    const MAREA_IrNode *variable;
    const MAREA_IrNode *bound;
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

    // INFO: sema rejects a local with no written initializer, so the ir adds no value for it.
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

// INFO: builds an IDENTIFIER that reads a local; a null text keeps the text of the AST node. The read has the resolved type of the local it reads.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateReference(MAREA_IrContext *context, const MAREA_AstNode *ast, const MAREA_Character8 *text, const MAREA_IrNode *local, MAREA_IrNode **outReference)
{
    MAREA_IrNode *reference = MAREA_NULLPTR;

    const MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, ast, &reference);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNodeShareResolvedType(reference, local);

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
        result = MAREA_G_CreateReference(context, step->nameAst, MAREA_NULLPTR, step->variable, &nameReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, step->loopAst, step->boundName, step->bound, &boundReference);
    }

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    equality->operatorKind = MAREA_TOKEN_KIND_EQUAL_EQUAL;
    MAREA_IrNodeSetResolvedTypeStatic(equality, MAREA_G_TestType);

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
        result = MAREA_G_CreateReference(context, step->nameAst, MAREA_NULLPTR, step->variable, &incrementReference);
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

    // INFO: the step adds one to the loop variable, so the literal has the type of that variable.
    MAREA_IrNodeShareResolvedType(one, step->variable);

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
// INFO: a nested while or for each stops the walk, because a continue inside it targets that loop. A nested for-range is a while by now, so it stops it too.
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

// INFO: gives a hidden local the name prefix and its number, and adds one to the counter, so two hidden locals of one kind in one function never share a name. The name ends with a NUL byte, as every reference to it needs.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateNumberedName(MAREA_IrContext *context, const MAREA_Character8 *prefix, MAREA_IntegerSize *counter, const MAREA_Character8 **outName)
{
    MAREA_Character8 digits[MAREA_G_BOUND_DIGITS];

    MAREA_IntegerSize digitCount = MAREA_INTEGER_SIZE(0);
    MAREA_IntegerSize rest = *counter;

    *counter = (*counter + MAREA_INTEGER_SIZE(1));

    do {
        digits[digitCount] = (MAREA_Character8)('0' + (rest % MAREA_INTEGER_SIZE(10)));
        digitCount = (digitCount + MAREA_INTEGER_SIZE(1));
        rest = (rest / MAREA_INTEGER_SIZE(10));
    } while (rest != MAREA_INTEGER_SIZE(0));

    const MAREA_IntegerSize PREFIX_LENGTH = strlen(prefix);

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (PREFIX_LENGTH + digitCount + MAREA_INTEGER_SIZE(1)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *name = memory;

    memcpy(name, prefix, PREFIX_LENGTH);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < digitCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        name[(PREFIX_LENGTH + index)] = digits[(digitCount - MAREA_INTEGER_SIZE(1) - index)];
    }

    name[(PREFIX_LENGTH + digitCount)] = '\0';

    *outName = name;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: gives the next for-range loop of the function the name of its hidden bound, "$last" and its number.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateBoundName(MAREA_IrContext *context, const MAREA_Character8 **outName)
{
    return MAREA_G_CreateNumberedName(context, MAREA_G_BoundPrefix, &context->loopCount, outName);
}

// INFO: for name in first .. last do body end becomes BLOCK(LOCAL name = first, LOCAL $lastN = last, WHILE name <= $lastN: body, step), where N is the number of the loop in its function, the outer loop first.
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

    // INFO: the number is taken before the body is lowered, so a loop inside the body gets a larger one.
    const MAREA_Character8 *boundName = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_CreateBoundName(context, &boundName);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrLowerExpression(context, firstTyped, &first);

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

    MAREA_G_Step step = { .loopAst = forAst, .nameAst = nameAst, .boundName = boundName, .variable = MAREA_NULLPTR, .bound = MAREA_NULLPTR };

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

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: the loop variable has the type sema gave its name, or else the type of the first value; the hidden bound has the type of the last value.
    MAREA_Boolean variableTyped = MAREA_BOOLEAN_FALSE;

    result = MAREA_IrNodeSetResolvedTypeFromTyped(context, local, nameTyped, &variableTyped);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (variableTyped == MAREA_BOOLEAN_FALSE) {
        MAREA_IrNodeShareResolvedType(local, first);
    }

    MAREA_IrNodeShareResolvedType(bound, last);

    // INFO: a local of the source has a type name, so the hidden loop locals have one too, the head of their resolved type.
    MAREA_IrNodeSetTypeNameFromResolved(local);
    MAREA_IrNodeSetTypeNameFromResolved(bound);

    step.variable = local;
    step.bound = bound;

    result = MAREA_G_PrefixContinues(context, body, &step);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    bound->text = boundName;
    bound->textLength = (MAREA_Uinteger32)strlen(boundName);

    // INFO: the loop local is written by the increment, so it is a write target; the hidden bound is never written.
    local->flags = (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE;

    // INFO: a range includes both ends (docs/grammar/marea.ebnf), so the bound is <=, not <.
    condition->operatorKind = MAREA_TOKEN_KIND_LESS_EQUAL;
    MAREA_IrNodeSetResolvedTypeStatic(condition, MAREA_G_TestType);
    result = MAREA_IrNodeAppendChild(context, local, first);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, bound, last);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, nameAst, MAREA_NULLPTR, local, &reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_CreateReference(context, forAst, boundName, bound, &boundReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, condition, reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, condition, boundReference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_G_AppendStep(context, body, &step);
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
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateComparison(MAREA_IrContext *context, const MAREA_AstNode *switchAst, const MAREA_IrNode *switchLocal, const MAREA_SemaTypedNode *valueTyped, MAREA_IrNode **outComparison)
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
        result = MAREA_G_CreateReference(context, switchAst, switchLocal->text, switchLocal, &reference);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        MAREA_G_KeepOnlyPosition(comparison);
        comparison->operatorKind = MAREA_TOKEN_KIND_EQUAL_EQUAL;
        MAREA_IrNodeSetResolvedTypeStatic(comparison, MAREA_G_TestType);
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
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateCondition(MAREA_IrContext *context, const MAREA_SemaTypedNode *armTyped, const MAREA_AstNode *switchAst, const MAREA_IrNode *switchLocal, MAREA_IrNode **outCondition)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(armTyped);

    MAREA_IrNode *condition = MAREA_NULLPTR;

    *outCondition = MAREA_NULLPTR;

    // INFO: the loop stops before the last child, which is the body; the form (index + 1) < COUNT cannot underflow on an empty arm.
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index + MAREA_INTEGER_SIZE(1)) < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *valueTyped = MAREA_IrTypedChildAt(armTyped, index);

        MAREA_IrNode *comparison = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_G_CreateComparison(context, switchAst, switchLocal, valueTyped, &comparison);

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
            MAREA_IrNodeSetResolvedTypeStatic(joined, MAREA_G_TestType);
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
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendCase(MAREA_IrContext *context, const MAREA_SemaTypedNode *armTyped, const MAREA_AstNode *switchAst, const MAREA_IrNode *switchLocal, MAREA_IrNode **chain, MAREA_Boolean *outComplete)
{
    MAREA_IrNode *condition = MAREA_NULLPTR;
    MAREA_IrNode *body = MAREA_NULLPTR;
    MAREA_IrNode *clause = MAREA_NULLPTR;

    *outComplete = MAREA_BOOLEAN_FALSE;

    MAREA_IrExcept result = MAREA_G_CreateCondition(context, armTyped, switchAst, switchLocal, &condition);

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

// INFO: sema reports NON_EXHAUSTIVE_SWITCH for an enum switch with no default that misses a variant, so one that passed covers them all.
static MAREA_Boolean MAREA_CALL MAREA_G_IsEnumScrutinee(const MAREA_SemaTypedNode *scrutinee)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    if ((MAREA_SemaTypedNodeGetTypeClass(scrutinee) != MAREA_SEMA_TYPE_CLASS_USER) || (MAREA_SemaTypedNodeGetTypeDeclaration(scrutinee, &declaration) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (declaration == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the else of a chain that sema proved exhaustive holds an UNREACHABLE_STATEMENT, the node that unreachable!() lowers to.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendUnreachableElse(MAREA_IrContext *context, const MAREA_AstNode *switchAst, MAREA_IrNode *chain)
{
    MAREA_IrNode *clause = MAREA_NULLPTR;
    MAREA_IrNode *body = MAREA_NULLPTR;
    MAREA_IrNode *marker = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_ELSE_CLAUSE, switchAst, &clause);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, switchAst, &body);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_UNREACHABLE_STATEMENT, switchAst, &marker);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, body, marker);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, clause, body);
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

    const MAREA_Character8 *name = MAREA_NULLPTR;

    result = MAREA_G_CreateNumberedName(context, MAREA_G_SwitchPrefix, &context->switchCount, &name);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    local->text = name;
    local->textLength = (MAREA_Uinteger32)strlen(name);

    // INFO: the hidden local holds the scrutinee, so it has the type of the scrutinee.
    MAREA_IrNodeShareResolvedType(local, scrutinee);

    result = MAREA_IrNodeAppendChild(context, local, scrutinee);

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        result = MAREA_IrNodeAppendChild(context, outer, local);
    }

    if (result == MAREA_IR_EXCEPT_SUCCESS) {
        *outScope = outer;
    }

    return result;
}

// INFO: switch value case a then A case b, c then B default then C end becomes BLOCK(LOCAL $switch = value, IF $switch == a: A, ELSEIF $switch == b or $switch == c: B, ELSE: C); an enum switch with no default ends with ELSE: unreachable.
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
            result = MAREA_G_AppendCase(context, armTyped, switchAst, outer->children[0], &chain, &complete);
        }

        if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_FALSE)) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }
    }

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (defaultBody != MAREA_NULLPTR)) {
        result = MAREA_G_AppendDefault(context, defaultAst, defaultBody, chain, outer);
    } else if ((result == MAREA_IR_EXCEPT_SUCCESS) && (chain != MAREA_NULLPTR) && (MAREA_G_IsEnumScrutinee(MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0))) == MAREA_BOOLEAN_TRUE)) {
        result = MAREA_G_AppendUnreachableElse(context, switchAst, chain);
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

    const MAREA_IntegerSize NAME_INDEX = (COUNT - MAREA_INTEGER_SIZE(3));

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_LowerBoundScope(context, MAREA_IR_KIND_FOR_EACH_STATEMENT, typed, NAME_INDEX, &node);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (node == MAREA_NULLPTR)) {
        return result;
    }

    // INFO: the node declares the value, so it has the type sema gave that name; the key, the first child of a loop with two names, was lowered as a read and has its type already.
    result = MAREA_IrNodeSetResolvedTypeFromTyped(context, node, MAREA_IrTypedChildAt(typed, NAME_INDEX), MAREA_NULLPTR);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNodeSetTypeNameFromResolved(node);

    if (NAME_INDEX > MAREA_INTEGER_SIZE(0)) {
        MAREA_IrNodeSetTypeNameFromResolved(node->children[0]);
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
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

// INFO: one destructible local directly declared in a block, paired with the destroy operator its type names.
typedef struct MAREA_G_DestroyPlanEntry
{
    const MAREA_IrNode *local;
    const MAREA_SemaTypedNode *function;
} MAREA_G_DestroyPlanEntry;

// INFO: true when a UNARY_EXPRESSION with the operator move, anywhere in node's subtree, names the local text:
// INFO: syntactic and order-insensitive (the language has no lifetime analysis), so a local moved anywhere in its
// INFO: own block, on any branch, is never destroyed at any exit of that block (see docs/ir/marea-ir.md Gaps).
static MAREA_Boolean MAREA_CALL MAREA_G_ContainsMoveOf(const MAREA_IrNode *node, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((node->kind == MAREA_IR_KIND_UNARY_EXPRESSION) && (node->operatorKind == MAREA_TOKEN_KIND_KEYWORD_MOVE) && (node->childCount > MAREA_INTEGER_SIZE(0))) {
        const MAREA_IrNode *operand = node->children[0];

        if ((operand->kind == MAREA_IR_KIND_IDENTIFIER) && (operand->textLength == (MAREA_Uinteger32)length) && (memcmp(operand->text, text, length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < node->childCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_ContainsMoveOf(node->children[index], text, length) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: builds one DESTROY_CALL: the text is the qualified destroy function ("Type.name"), the one child an
// INFO: IDENTIFIER naming the local, for a backend to bind as the receiver.
static MAREA_IrExcept MAREA_CALL MAREA_G_CreateDestroyCall(MAREA_IrContext *context, const MAREA_AstNode *positionAst, const MAREA_IrNode *local, const MAREA_SemaTypedNode *destroyFunction, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(destroyFunction, MAREA_AST_KIND_IDENTIFIER);

    if (name == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_DESTROY_CALL, positionAst, &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_G_KeepOnlyPosition(node);

    result = MAREA_IrNodeSetQualifiedText(context, node, local->typeName, local->typeNameLength, MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNode *reference = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, positionAst, &reference);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_G_KeepOnlyPosition(reference);
    reference->text = local->text;
    reference->textLength = local->textLength;

    result = MAREA_IrNodeAppendChild(context, node, reference);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: builds one fresh DESTROY_CALL per plan entry, in the plan's own order (already reverse declaration order).
static MAREA_IrExcept MAREA_CALL MAREA_G_MaterializeDestroys(MAREA_IrContext *context, const MAREA_AstNode *positionAst, const MAREA_G_DestroyPlanEntry *plan, MAREA_IntegerSize planCount, MAREA_IrNode ***outNodes)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (planCount * sizeof(MAREA_IrNode *)), MAREA_ALIGNOF(MAREA_IrNode *), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_IrNode **nodes = memory;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < planCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        nodes[index] = MAREA_NULLPTR;

        const MAREA_IrExcept result = MAREA_G_CreateDestroyCall(context, positionAst, plan[index].local, plan[index].function, &nodes[index]);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    *outNodes = nodes;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: true when the value of a return is already the hidden local $return, so an outer wrap must not hoist it again.
static MAREA_Boolean MAREA_CALL MAREA_G_IsReturnReference(const MAREA_IrNode *value)
{
    const MAREA_IntegerSize LENGTH = strlen(MAREA_G_ReturnPrefix);

    return ((value->kind == MAREA_IR_KIND_IDENTIFIER) && (value->textLength > (MAREA_Uinteger32)LENGTH) && (memcmp(value->text, MAREA_G_ReturnPrefix, LENGTH) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: moves the value of a RETURN_STATEMENT into a new LOCAL_DECLARATION $return and leaves IDENTIFIER $return as the value, so
// INFO: the value is read where the declaration stands. A return with no value, or with the value already $return, gives no declaration.
static MAREA_IrExcept MAREA_CALL MAREA_G_HoistReturnValue(MAREA_IrContext *context, const MAREA_AstNode *positionAst, MAREA_IrNode *returnNode, MAREA_IrNode **outDeclaration)
{
    *outDeclaration = MAREA_NULLPTR;

    if ((returnNode->kind != MAREA_IR_KIND_RETURN_STATEMENT) || (returnNode->childCount == MAREA_INTEGER_SIZE(0))) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *value = returnNode->children[0];

    if (MAREA_G_IsReturnReference(value) == MAREA_BOOLEAN_TRUE) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *declaration = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_LOCAL_DECLARATION, positionAst, &declaration);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_G_KeepOnlyPosition(declaration);

    const MAREA_Character8 *name = MAREA_NULLPTR;

    result = MAREA_G_CreateNumberedName(context, MAREA_G_ReturnPrefix, &context->returnCount, &name);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    declaration->text = name;
    declaration->textLength = (MAREA_Uinteger32)strlen(name);

    // INFO: the hidden local holds the returned value, so it has the type of that value.
    MAREA_IrNodeShareResolvedType(declaration, value);

    MAREA_IrNode *reference = MAREA_NULLPTR;

    result = MAREA_G_CreateReference(context, positionAst, name, declaration, &reference);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, declaration, value);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    returnNode->children[0] = reference;
    *outDeclaration = declaration;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: replaces original, a matched exit, with a fresh BLOCK(destroy..., original): a new, independent set of
// INFO: DESTROY_CALL nodes, never the same node instance reused at two exits. A return with a value becomes
// INFO: BLOCK(LOCAL_DECLARATION $return (value), destroy..., RETURN_STATEMENT(IDENTIFIER $return)), so the value is read before any destroy.
static MAREA_IrExcept MAREA_CALL MAREA_G_WrapWithDestroys(MAREA_IrContext *context, const MAREA_AstNode *positionAst, const MAREA_G_DestroyPlanEntry *plan, MAREA_IntegerSize planCount, MAREA_IrNode *original, MAREA_IrNode **outWrapped)
{
    MAREA_IrNode **destroys = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_MaterializeDestroys(context, positionAst, plan, planCount, &destroys);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNode *wrapper = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BLOCK, positionAst, &wrapper);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_G_KeepOnlyPosition(wrapper);

    MAREA_IrNode *hoisted = MAREA_NULLPTR;

    result = MAREA_G_HoistReturnValue(context, positionAst, original, &hoisted);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (hoisted != MAREA_NULLPTR) {
        result = MAREA_IrNodeAppendChild(context, wrapper, hoisted);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < planCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        result = MAREA_IrNodeAppendChild(context, wrapper, destroys[index]);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    result = MAREA_IrNodeAppendChild(context, wrapper, original);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outWrapped = wrapper;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: recurses through node's whole subtree, wrapping every matched exit with a fresh copy of plan's destroys.
// INFO: matchReturn finds RETURN_STATEMENT and descends into a nested loop too (a return unwinds the whole
// INFO: function); matchReturn false finds BREAK_STATEMENT and CONTINUE_STATEMENT but stops at a nested
// INFO: WHILE_STATEMENT or FOR_EACH_STATEMENT, whose own break or continue targets that loop instead (the same
// INFO: stopping rule MAREA_G_PrefixContinues already uses for the for-range step). Recursing generically into
// INFO: every child, rather than matching a bare kind only at the top, is what makes repeated calls at nested
// INFO: block levels compose correctly: an inner block's own wrap becomes this pass's new subtree to walk, so
// INFO: an outer block's destroys end up nested inside it, and so run after the inner ones, innermost first.
static MAREA_IrExcept MAREA_CALL MAREA_G_PrefixExits(MAREA_IrContext *context, const MAREA_AstNode *positionAst, MAREA_IrNode *node, const MAREA_G_DestroyPlanEntry *plan, MAREA_IntegerSize planCount, MAREA_Boolean matchReturn)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < node->childCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_IrNode *child = node->children[index];

        if ((matchReturn == MAREA_BOOLEAN_FALSE) && ((child->kind == MAREA_IR_KIND_WHILE_STATEMENT) || (child->kind == MAREA_IR_KIND_FOR_EACH_STATEMENT))) {
            continue;
        }

        const MAREA_Boolean isMatch = (matchReturn == MAREA_BOOLEAN_TRUE) ? (child->kind == MAREA_IR_KIND_RETURN_STATEMENT) : ((child->kind == MAREA_IR_KIND_BREAK_STATEMENT) || (child->kind == MAREA_IR_KIND_CONTINUE_STATEMENT));

        if (isMatch == MAREA_BOOLEAN_FALSE) {
            const MAREA_IrExcept result = MAREA_G_PrefixExits(context, positionAst, child, plan, planCount, matchReturn);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }

            continue;
        }

        MAREA_IrNode *wrapped = MAREA_NULLPTR;

        const MAREA_IrExcept result = MAREA_G_WrapWithDestroys(context, positionAst, plan, planCount, child, &wrapped);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        node->children[index] = wrapped;
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: true when block's own last statement unconditionally transfers control: a RETURN, BREAK or CONTINUE, or
// INFO: (recursively, through a wrap MAREA_G_WrapWithDestroys built) a BLOCK whose own last statement does.
static MAREA_Boolean MAREA_CALL MAREA_G_BlockEndsInJump(const MAREA_IrNode *block)
{
    if (block->childCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IrNode *last = block->children[(block->childCount - MAREA_INTEGER_SIZE(1))];

    if ((last->kind == MAREA_IR_KIND_RETURN_STATEMENT) || (last->kind == MAREA_IR_KIND_BREAK_STATEMENT) || (last->kind == MAREA_IR_KIND_CONTINUE_STATEMENT)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return (last->kind == MAREA_IR_KIND_BLOCK) ? MAREA_G_BlockEndsInJump(last) : MAREA_BOOLEAN_FALSE;
}

// INFO: a local is destructible when its written type names a destroy operator in some implementation of it.
static const MAREA_SemaTypedNode *MAREA_CALL MAREA_G_FindDestroyOperator(MAREA_IrContext *context, const MAREA_IrNode *local)
{
    if (local->typeName == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return MAREA_IrFindLifecycleFunction(context, local->typeName, local->typeNameLength, MAREA_BOOLEAN_FALSE);
}

// INFO: inserts the destroy calls for every destructible local block declares directly, at every exit of block:
// INFO: a RETURN, BREAK or CONTINUE reachable in its subtree, and the fallthrough end when block does not
// INFO: already end in a jump. Reverse declaration order; a local moved anywhere in block is skipped everywhere
// INFO: (see MAREA_G_ContainsMoveOf). A block with no destructible local, by far the common case, costs one
// INFO: pass over its own direct children and nothing else.
static MAREA_IrExcept MAREA_CALL MAREA_G_InsertDestroys(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *block)
{
    if (block->childCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_Null forwardMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (block->childCount * sizeof(MAREA_G_DestroyPlanEntry)), MAREA_ALIGNOF(MAREA_G_DestroyPlanEntry), &forwardMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_G_DestroyPlanEntry *forward = forwardMemory;
    MAREA_IntegerSize forwardCount = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < block->childCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = block->children[index];

        if (child->kind != MAREA_IR_KIND_LOCAL_DECLARATION) {
            continue;
        }

        const MAREA_SemaTypedNode *destroyFunction = MAREA_G_FindDestroyOperator(context, child);

        if (destroyFunction == MAREA_NULLPTR) {
            continue;
        }

        forward[forwardCount].local = child;
        forward[forwardCount].function = destroyFunction;
        forwardCount = (forwardCount + MAREA_INTEGER_SIZE(1));
    }

    if (forwardCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_Null planMemory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (forwardCount * sizeof(MAREA_G_DestroyPlanEntry)), MAREA_ALIGNOF(MAREA_G_DestroyPlanEntry), &planMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_IR_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_G_DestroyPlanEntry *plan = planMemory;
    MAREA_IntegerSize planCount = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = forwardCount; index > MAREA_INTEGER_SIZE(0); index = (index - MAREA_INTEGER_SIZE(1))) {
        const MAREA_G_DestroyPlanEntry *entry = &forward[(index - MAREA_INTEGER_SIZE(1))];

        if (MAREA_G_ContainsMoveOf(block, entry->local->text, entry->local->textLength) == MAREA_BOOLEAN_TRUE) {
            continue;
        }

        plan[planCount] = *entry;
        planCount = (planCount + MAREA_INTEGER_SIZE(1));
    }

    if (planCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *positionAst = MAREA_SemaTypedNodeGetAstNode(typed);

    MAREA_IrExcept result = MAREA_G_PrefixExits(context, positionAst, block, plan, planCount, MAREA_BOOLEAN_TRUE);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_G_PrefixExits(context, positionAst, block, plan, planCount, MAREA_BOOLEAN_FALSE);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (MAREA_G_BlockEndsInJump(block) == MAREA_BOOLEAN_TRUE) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode **destroys = MAREA_NULLPTR;

    result = MAREA_G_MaterializeDestroys(context, positionAst, plan, planCount, &destroys);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < planCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        result = MAREA_IrNodeAppendChild(context, block, destroys[index]);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
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

    MAREA_IrNode *block = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_LowerKept(context, MAREA_IR_KIND_BLOCK, typed, &block);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (block == MAREA_NULLPTR)) {
        *outNode = block;

        return result;
    }

    result = MAREA_G_InsertDestroys(context, typed, block);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = block;

    return MAREA_IR_EXCEPT_SUCCESS;
}
