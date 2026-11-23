// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

// INFO: a type argument pattern is unified at most this many levels deep (array<Item,N> is one level, the limit guards a pathological type text).
#define MAREA_G_GENERIC_UNIFY_DEPTH_LIMIT MAREA_INTEGER_SIZE(8)

// INFO: a compound type text splits into at most this many top-level comma-separated parts (array<Item,N> has two).
#define MAREA_G_GENERIC_SPLIT_LIMIT MAREA_INTEGER_SIZE(8)

// INFO: a call unifies at most this many distinct generic parameter names; a generic parameter list longer than this is not a realistic program.
#define MAREA_G_GENERIC_BINDING_LIMIT MAREA_INTEGER_SIZE(8)

typedef struct MAREA_G_GenericBinding
{
    const MAREA_Character8 *name;
    const MAREA_Character8 *text;
    MAREA_IntegerSize nameLength;
    MAREA_IntegerSize textLength;
} MAREA_G_GenericBinding;

typedef struct MAREA_G_GenericBindings
{
    MAREA_G_GenericBinding items[MAREA_G_GENERIC_BINDING_LIMIT];
    MAREA_IntegerSize count;
    MAREA_Boolean conflict;
} MAREA_G_GenericBindings;

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the written type of a PARAMETER node: the first type child, after the name.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ParameterType(const MAREA_AstNode *parameter)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(parameter);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(parameter, index);

        if (MAREA_G_IsType(child) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_SingleSegment(const MAREA_AstNode *qualified)
{
    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualified) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(qualified) != 1)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(qualified, 0);

    return ((segment != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(segment) == MAREA_AST_KIND_IDENTIFIER)) ? segment : MAREA_NULLPTR;
}

// INFO: a written type that is one bare name, with no type arguments: Item, never array<Item> or c.array.
static MAREA_Boolean MAREA_CALL MAREA_G_BareName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE) || (MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *identifier = MAREA_G_SingleSegment(MAREA_SemaChildAt(type, 0));

    if (identifier == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outText = MAREA_AstNodeGetText(identifier);
    *outLength = MAREA_AstNodeGetTextLength(identifier);

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_ExprTypeOfNode(const MAREA_AstNode *node)
{
    return (MAREA_SemaExprType){ .node = node, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };
}

// INFO: the first binding of a name wins; a second binding that disagrees marks the whole inference a conflict, so the call stays untyped rather than guess.
static void MAREA_CALL MAREA_G_BindGeneric(MAREA_G_GenericBindings *bindings, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_Character8 *text, MAREA_IntegerSize textLength)
{
    for (MAREA_IntegerSize index = 0; index < bindings->count; ++index) {
        if (MAREA_SemaTextEquals(bindings->items[index].name, bindings->items[index].nameLength, name, nameLength) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if (MAREA_SemaTextEquals(bindings->items[index].text, bindings->items[index].textLength, text, textLength) == MAREA_BOOLEAN_FALSE) {
            bindings->conflict = MAREA_BOOLEAN_TRUE;
        }

        return;
    }

    if (bindings->count >= MAREA_G_GENERIC_BINDING_LIMIT) {
        return;
    }

    bindings->items[bindings->count] = (MAREA_G_GenericBinding){ .name = name, .text = text, .nameLength = nameLength, .textLength = textLength };
    bindings->count += 1;
}

// INFO: splits "head<args>" into the head and the inner argument text; a bare name with no angle brackets gives false.
static MAREA_Boolean MAREA_CALL MAREA_G_SplitGenericText(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_IntegerSize *outHeadLength, MAREA_IntegerSize *outArgumentsStart, MAREA_IntegerSize *outArgumentsLength)
{
    MAREA_IntegerSize open = length;

    for (MAREA_IntegerSize index = 0; index < length; ++index) {
        if (text[index] == '<') {
            open = index;

            break;
        }
    }

    if ((open >= length) || (length == 0) || (text[(length - 1)] != '>')) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outHeadLength = open;
    *outArgumentsStart = (open + 1);
    *outArgumentsLength = (length - open - 2);

    return MAREA_BOOLEAN_TRUE;
}

// INFO: splits a comma list at bracket depth zero ('<' '>' and '(' ')' both nest), up to a bounded number of parts.
static MAREA_IntegerSize MAREA_CALL MAREA_G_SplitArguments(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_IntegerSize *outStarts, MAREA_IntegerSize *outLengths, MAREA_IntegerSize maximumParts)
{
    MAREA_IntegerSize count = 0;
    MAREA_IntegerSize depth = 0;
    MAREA_IntegerSize partStart = 0;

    for (MAREA_IntegerSize index = 0; index <= length; ++index) {
        const MAREA_Character8 character = (index < length) ? text[index] : ',';

        if ((character == '<') || (character == '(')) {
            depth += 1;

            continue;
        }

        if ((character == '>') || (character == ')')) {
            depth -= 1;

            continue;
        }

        if ((character != ',') || (depth != 0)) {
            continue;
        }

        if (count < maximumParts) {
            outStarts[count] = partStart;
            outLengths[count] = (index - partStart);
            count += 1;
        }

        partStart = (index + 1);
    }

    return count;
}

// INFO: a pattern that is exactly one of the callee's generic parameters binds whole to the concrete text; a compound
// pattern (array<Item,N>) unifies head to head and argument to argument, each one recursively. A shape mismatch
// (a different head, a different argument count, no angle brackets on one side) binds nothing: it is not a conflict,
// the parameter simply contributes no information, the way a non-generic parameter type does.
static void MAREA_CALL MAREA_G_UnifyGenericText(const MAREA_AstNode *genericParameters, const MAREA_Character8 *pattern, MAREA_IntegerSize patternLength, const MAREA_Character8 *concrete, MAREA_IntegerSize concreteLength, MAREA_IntegerSize depth, MAREA_G_GenericBindings *bindings)
{
    if ((bindings->conflict == MAREA_BOOLEAN_TRUE) || (depth >= MAREA_G_GENERIC_UNIFY_DEPTH_LIMIT)) {
        return;
    }

    if (MAREA_SemaGenericParameterIndex(genericParameters, pattern, patternLength) < MAREA_AstNodeGetChildCount(genericParameters)) {
        MAREA_G_BindGeneric(bindings, pattern, patternLength, concrete, concreteLength);

        return;
    }

    MAREA_IntegerSize patternHeadLength = 0;
    MAREA_IntegerSize patternArgumentsStart = 0;
    MAREA_IntegerSize patternArgumentsLength = 0;

    MAREA_IntegerSize concreteHeadLength = 0;
    MAREA_IntegerSize concreteArgumentsStart = 0;
    MAREA_IntegerSize concreteArgumentsLength = 0;

    if ((MAREA_G_SplitGenericText(pattern, patternLength, &patternHeadLength, &patternArgumentsStart, &patternArgumentsLength) == MAREA_BOOLEAN_FALSE) || (MAREA_G_SplitGenericText(concrete, concreteLength, &concreteHeadLength, &concreteArgumentsStart, &concreteArgumentsLength) == MAREA_BOOLEAN_FALSE)) {
        return;
    }

    if (MAREA_SemaTextEquals(pattern, patternHeadLength, concrete, concreteHeadLength) == MAREA_BOOLEAN_FALSE) {
        return;
    }

    MAREA_IntegerSize patternStarts[MAREA_G_GENERIC_SPLIT_LIMIT];
    MAREA_IntegerSize patternLengths[MAREA_G_GENERIC_SPLIT_LIMIT];
    MAREA_IntegerSize concreteStarts[MAREA_G_GENERIC_SPLIT_LIMIT];
    MAREA_IntegerSize concreteLengths[MAREA_G_GENERIC_SPLIT_LIMIT];

    const MAREA_IntegerSize PATTERN_COUNT = MAREA_G_SplitArguments((pattern + patternArgumentsStart), patternArgumentsLength, patternStarts, patternLengths, MAREA_G_GENERIC_SPLIT_LIMIT);

    const MAREA_IntegerSize CONCRETE_COUNT = MAREA_G_SplitArguments((concrete + concreteArgumentsStart), concreteArgumentsLength, concreteStarts, concreteLengths, MAREA_G_GENERIC_SPLIT_LIMIT);

    if (PATTERN_COUNT != CONCRETE_COUNT) {
        return;
    }

    for (MAREA_IntegerSize index = 0; index < PATTERN_COUNT; ++index) {
        MAREA_G_UnifyGenericText(genericParameters, (pattern + patternArgumentsStart + patternStarts[index]), patternLengths[index], (concrete + concreteArgumentsStart + concreteStarts[index]), concreteLengths[index], (depth + MAREA_INTEGER_SIZE(1)), bindings);
    }
}

// INFO: the typed ARGUMENT_LIST child of a call's typed node, found by kind, the same way the untyped AST is searched.
static const MAREA_SemaTypedNode *MAREA_CALL MAREA_G_TypedArgumentList(const MAREA_SemaTypedNode *call)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(call);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_SemaTypedNode *child = MAREA_NULLPTR;

        if (MAREA_SemaTypedNodeGetChildAt(call, index, &child) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            continue;
        }

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(MAREA_SemaTypedNodeGetAstNode(child)) == MAREA_AST_KIND_ARGUMENT_LIST)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstInferCallResult(MAREA_SemaContext *context, const MAREA_AstNode *function, const MAREA_AstNode *resultType, const MAREA_SemaTypedNode *call)
{
    const MAREA_SemaExprType NONE = MAREA_G_ExprTypeOfNode(MAREA_NULLPTR);

    const MAREA_AstNode *genericParameters = MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS);

    const MAREA_AstNode *parameterList = MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST);

    if ((context == MAREA_NULLPTR) || (genericParameters == MAREA_NULLPTR) || (parameterList == MAREA_NULLPTR) || (resultType == MAREA_NULLPTR) || (call == MAREA_NULLPTR)) {
        return NONE;
    }

    const MAREA_SemaTypedNode *argumentList = MAREA_G_TypedArgumentList(call);

    if (argumentList == MAREA_NULLPTR) {
        return NONE;
    }

    const MAREA_IntegerSize PARAMETER_COUNT = MAREA_AstNodeGetChildCount(parameterList);

    const MAREA_IntegerSize ARGUMENT_COUNT = MAREA_SemaTypedNodeGetChildCount(argumentList);

    const MAREA_IntegerSize COUNT = (PARAMETER_COUNT < ARGUMENT_COUNT) ? PARAMETER_COUNT : ARGUMENT_COUNT;

    MAREA_G_GenericBindings bindings = { .count = 0, .conflict = MAREA_BOOLEAN_FALSE };

    for (MAREA_IntegerSize index = 0; ((index < COUNT) && (bindings.conflict == MAREA_BOOLEAN_FALSE)); ++index) {
        const MAREA_AstNode *parameterType = MAREA_G_ParameterType(MAREA_SemaChildAt(parameterList, index));

        if (parameterType == MAREA_NULLPTR) {
            continue;
        }

        const MAREA_SemaExprType PATTERN = MAREA_G_ExprTypeOfNode(parameterType);

        const MAREA_Character8 *patternText = MAREA_NULLPTR;

        MAREA_IntegerSize patternLength = 0;

        if ((MAREA_SemaTypedAstRenderExprType(context, &PATTERN, &patternText, &patternLength) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (patternText == MAREA_NULLPTR)) {
            continue;
        }

        const MAREA_SemaTypedNode *argument = MAREA_NULLPTR;

        if (MAREA_SemaTypedNodeGetChildAt(argumentList, index, &argument) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            continue;
        }

        const MAREA_Character8 *argumentText = MAREA_NULLPTR;

        MAREA_IntegerSize argumentLength = 0;

        if (MAREA_SemaTypedNodeGetResolvedType(argument, &argumentText, &argumentLength) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            continue;
        }

        MAREA_G_UnifyGenericText(genericParameters, patternText, patternLength, argumentText, argumentLength, 0, &bindings);
    }

    if ((bindings.conflict == MAREA_BOOLEAN_TRUE) || (bindings.count == 0)) {
        return NONE;
    }

    const MAREA_Character8 *resultName = MAREA_NULLPTR;

    MAREA_IntegerSize resultNameLength = 0;

    if (MAREA_G_BareName(resultType, &resultName, &resultNameLength) == MAREA_BOOLEAN_FALSE) {
        return NONE;
    }

    for (MAREA_IntegerSize index = 0; index < bindings.count; ++index) {
        if (MAREA_SemaTextEquals(bindings.items[index].name, bindings.items[index].nameLength, resultName, resultNameLength) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        MAREA_SemaExprType result = NONE;

        result.fixed = bindings.items[index].text;
        result.fixedLength = bindings.items[index].textLength;

        return result;
    }

    return NONE;
}

// INFO: the function a contract of parameter's where clause declares with the given name, searched through every contract that constrains it, the first one wins.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindContractFunctionByName(const MAREA_AstNode *contract, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(contract);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *function = MAREA_SemaChildAt(contract, index);

        if (MAREA_AstNodeGetKind(function) != MAREA_AST_KIND_CONTRACT_FUNCTION) {
            continue;
        }

        const MAREA_AstNode *functionName = MAREA_SemaChildAt(function, 0);

        if ((functionName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(functionName), MAREA_AstNodeGetTextLength(functionName), name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return function;
        }
    }

    return MAREA_NULLPTR;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaFindContractMember(const MAREA_SemaContext *context, const MAREA_AstNode *scope, const MAREA_Character8 *parameterName, MAREA_IntegerSize parameterNameLength, const MAREA_Character8 *memberName, MAREA_IntegerSize memberNameLength)
{
    const MAREA_AstNode *where = (scope == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(scope, MAREA_AST_KIND_WHERE_CLAUSE);

    if ((context == MAREA_NULLPTR) || (where == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(where);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *constraint = MAREA_SemaChildAt(where, index);

        if (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_SemaChildAt(constraint, 0);

        if ((name == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), parameterName, parameterNameLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_IntegerSize CONTRACT_COUNT = MAREA_AstNodeGetChildCount(constraint);

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < CONTRACT_COUNT; ++contractIndex) {
            const MAREA_AstNode *qualified = MAREA_SemaChildAt(constraint, contractIndex);

            const MAREA_AstNode *segment = ((qualified != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(qualified) == 1)) ? MAREA_SemaChildAt(qualified, 0) : MAREA_NULLPTR;

            if (segment == MAREA_NULLPTR) {
                continue;
            }

            const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));

            if ((entry == MAREA_NULLPTR) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_CONTRACT_DECLARATION)) {
                continue;
            }

            const MAREA_AstNode *function = MAREA_G_FindContractFunctionByName(entry->declaration, memberName, memberNameLength);

            if (function != MAREA_NULLPTR) {
                return function;
            }
        }
    }

    return MAREA_NULLPTR;
}
