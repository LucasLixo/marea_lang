// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

// INFO: the type system is erased, so a type or a type-argument child never becomes an IR node.
static MAREA_Boolean MAREA_CALL MAREA_G_IsErasedChild(MAREA_AstKind kind)
{
    return ((kind == MAREA_AST_KIND_NAMED_TYPE) || (kind == MAREA_AST_KIND_FUNCTION_TYPE) || (kind == MAREA_AST_KIND_TYPE_ARGUMENTS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: an identifier read as a value lowers to the original name when it is an alias that no local of the function hides.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerIdentifier(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    const MAREA_AstNode *ast = MAREA_SemaTypedNodeGetAstNode(typed);

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, ast, &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_Character8 *original = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (MAREA_IrAliasValue(context, MAREA_AstNodeGetText(ast), MAREA_AstNodeGetTextLength(ast), MAREA_AstNodeGetPosition(ast).offset, &original, &length) == MAREA_BOOLEAN_TRUE) {
        result = MAREA_IrNodeSetQualifiedText(context, node, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), original, length);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: a child that lowers to nothing makes the whole parent lower to nothing, so no partial tree is produced.
// INFO: with headOnly, only the first child may be an alias; a later IDENTIFIER is a member name and keeps its text.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerChildren(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean headOnly, MAREA_Boolean *outComplete)
{
    *outComplete = MAREA_BOOLEAN_TRUE;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *child = MAREA_IrTypedChildAt(typed, index);

        if (MAREA_G_IsErasedChild(MAREA_IrTypedKind(child)) == MAREA_BOOLEAN_TRUE) {
            continue;
        }

        MAREA_IrNode *lowered = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_IR_EXCEPT_SUCCESS;

        if ((headOnly == MAREA_BOOLEAN_TRUE) && (index != MAREA_INTEGER_SIZE(0)) && (MAREA_IrTypedKind(child) == MAREA_AST_KIND_IDENTIFIER)) {
            result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, MAREA_SemaTypedNodeGetAstNode(child), &lowered);
        } else {
            result = MAREA_IrLowerExpression(context, child, &lowered);
        }

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

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerWithChildrenOf(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_Boolean headOnly, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, kind, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_G_LowerChildren(context, typed, node, headOnly, &complete);

    if ((result == MAREA_IR_EXCEPT_SUCCESS) && (complete == MAREA_BOOLEAN_TRUE)) {
        *outNode = node;
    }

    return result;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerWithChildren(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    return MAREA_G_LowerWithChildrenOf(context, kind, typed, MAREA_BOOLEAN_FALSE, outNode);
}

static MAREA_IrExcept MAREA_CALL MAREA_G_RaiseGeneric(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *message)
{
    return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_SemaTypedNodeGetAstNode(typed), message, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
}

// INFO: true when two AST nodes carry the same text, used to match a callee identifier against a declared name.
static MAREA_Boolean MAREA_CALL MAREA_G_SameText(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if (MAREA_AstNodeGetTextLength(left) != MAREA_AstNodeGetTextLength(right)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(MAREA_AstNodeGetText(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(left)) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a call fills a default only for a plain function of the same unit; any other callee shape fills nothing, the same limit the built-in lookup already has.
// INFO: a parameter or a local of the enclosing function with the callee's name hides the module function, so its own call is never filled either.
static const MAREA_SemaTypedNode *MAREA_CALL MAREA_G_FindCalleeFunction(const MAREA_IrContext *context, const MAREA_SemaTypedNode *callee)
{
    if (MAREA_IrTypedKind(callee) != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *calleeAst = MAREA_SemaTypedNodeGetAstNode(callee);

    if (MAREA_IrFunctionDeclares(context->function, MAREA_AstNodeGetText(calleeAst), MAREA_AstNodeGetTextLength(calleeAst), MAREA_AstNodeGetPosition(calleeAst).offset) == MAREA_BOOLEAN_TRUE) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(context->unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(context->unit, index);

        if (MAREA_IrTypedKind(item) != MAREA_AST_KIND_FUNCTION_DECLARATION) {
            continue;
        }

        const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER);

        if ((name != MAREA_NULLPTR) && (MAREA_G_SameText(calleeAst, MAREA_SemaTypedNodeGetAstNode(name)) == MAREA_BOOLEAN_TRUE)) {
            return item;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: lowers one expression and appends it to node; a result that lowers to nothing marks the caller incomplete.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerAndAppend(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean *outComplete)
{
    MAREA_IrNode *lowered = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, typed, &lowered);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (lowered == MAREA_NULLPTR) {
        *outComplete = MAREA_BOOLEAN_FALSE;

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_IrNodeAppendChild(context, node, lowered);
}

// INFO: an argument list lowers every argument the call wrote, then fills every parameter the call omitted with
// INFO: its own default, read fresh from function's typed parameter list: the ir materializes every default.
// INFO: typed may be null (a Type{...} literal writes no argument); position then gives the node its source position.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerArgumentsAgainst(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_AstNode *position, const MAREA_SemaTypedNode *function, MAREA_IrNode **outNode, MAREA_Boolean *outComplete)
{
    *outComplete = MAREA_BOOLEAN_TRUE;

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_ARGUMENT_LIST, (typed != MAREA_NULLPTR) ? MAREA_SemaTypedNodeGetAstNode(typed) : position, &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_IntegerSize PROVIDED = (typed != MAREA_NULLPTR) ? MAREA_SemaTypedNodeGetChildCount(typed) : MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < PROVIDED; index = (index + MAREA_INTEGER_SIZE(1))) {
        result = MAREA_G_LowerAndAppend(context, MAREA_IrTypedChildAt(typed, index), node, outComplete);

        if ((result != MAREA_IR_EXCEPT_SUCCESS) || (*outComplete == MAREA_BOOLEAN_FALSE)) {
            return result;
        }
    }

    const MAREA_SemaTypedNode *parameters = (function != MAREA_NULLPTR) ? MAREA_IrTypedFindChild(function, MAREA_AST_KIND_PARAMETER_LIST) : MAREA_NULLPTR;

    if (parameters != MAREA_NULLPTR) {
        const MAREA_IntegerSize TOTAL = MAREA_SemaTypedNodeGetChildCount(parameters);

        MAREA_IntegerSize supplied = MAREA_INTEGER_SIZE(0);

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < TOTAL; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_SemaTypedNode *parameter = MAREA_IrTypedChildAt(parameters, index);

            if (MAREA_IrTypedKind(parameter) != MAREA_AST_KIND_PARAMETER) {
                continue;
            }

            if (supplied < PROVIDED) {
                supplied = (supplied + MAREA_INTEGER_SIZE(1));

                continue;
            }

            const MAREA_SemaTypedNode *defaultExpression = MAREA_IrTypedFindDefault(parameter);

            if (defaultExpression == MAREA_NULLPTR) {
                continue;
            }

            result = MAREA_G_LowerAndAppend(context, defaultExpression, node, outComplete);

            if ((result != MAREA_IR_EXCEPT_SUCCESS) || (*outComplete == MAREA_BOOLEAN_FALSE)) {
                return result;
            }
        }
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: the structure or enum of this unit named like callee, not hidden by a local or a parameter of the enclosing function: the same shadow rule MAREA_G_FindCalleeFunction uses for an ordinary call.
static const MAREA_SemaTypedNode *MAREA_CALL MAREA_G_FindTypeOfUnit(const MAREA_IrContext *context, const MAREA_SemaTypedNode *callee)
{
    if (MAREA_IrTypedKind(callee) != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *calleeAst = MAREA_SemaTypedNodeGetAstNode(callee);

    if (MAREA_IrFunctionDeclares(context->function, MAREA_AstNodeGetText(calleeAst), MAREA_AstNodeGetTextLength(calleeAst), MAREA_AstNodeGetPosition(calleeAst).offset) == MAREA_BOOLEAN_TRUE) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(context->unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(context->unit, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(item);

        if ((KIND != MAREA_AST_KIND_STRUCTURE_DECLARATION) && (KIND != MAREA_AST_KIND_ENUM_DECLARATION)) {
            continue;
        }

        const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER);

        if ((name != MAREA_NULLPTR) && (MAREA_G_SameText(calleeAst, MAREA_SemaTypedNodeGetAstNode(name)) == MAREA_BOOLEAN_TRUE)) {
            return item;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: Type(args): the callee names a structure or an enum of the unit, so the call is construction, not an ordinary call. The type name is the constructed type's own written name (a call result carries its declaration, not a name text, so the name comes from typeDeclaration's own IDENTIFIER); the arguments are filled against construct's own parameter list, defaults allowed, same as an ordinary call fills them.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerConstructCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_SemaTypedNode *typeDeclaration, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_CONSTRUCT_CALL, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_SemaTypedNode *typeName = MAREA_IrTypedFindChild(typeDeclaration, MAREA_AST_KIND_IDENTIFIER);

    if (typeName != MAREA_NULLPTR) {
        const MAREA_AstNode *typeNameAst = MAREA_SemaTypedNodeGetAstNode(typeName);

        node->typeName = MAREA_AstNodeGetText(typeNameAst);
        node->typeNameLength = MAREA_AstNodeGetTextLength(typeNameAst);
    }

    const MAREA_SemaTypedNode *constructOperator = MAREA_IrFindLifecycleFunction(context, node->typeName, node->typeNameLength, MAREA_BOOLEAN_TRUE);

    const MAREA_SemaTypedNode *arguments = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_ARGUMENT_LIST);

    if (arguments != MAREA_NULLPTR) {
        MAREA_IrNode *loweredArguments = MAREA_NULLPTR;
        MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

        result = MAREA_G_LowerArgumentsAgainst(context, arguments, MAREA_SemaTypedNodeGetAstNode(typed), constructOperator, &loweredArguments, &complete);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        if (complete == MAREA_BOOLEAN_FALSE) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }

        result = MAREA_IrNodeAppendChild(context, node, loweredArguments);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerTyped(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_Boolean asCallee, MAREA_IrNode **outNode);

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *callee = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    if (callee == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    // INFO: a built-in call and a static call on a written type carry their type as an operand, so they lower here.
    MAREA_Boolean handled = MAREA_BOOLEAN_FALSE;

    const MAREA_IrExcept operand = MAREA_IrLowerTypeOperandCall(context, typed, &handled, outNode);

    if ((operand != MAREA_IR_EXCEPT_SUCCESS) || (handled == MAREA_BOOLEAN_TRUE)) {
        return operand;
    }

    // INFO: Type(args) is construction, not an ordinary call (docs/sema/marea-sema.md, "Type construction call").
    const MAREA_SemaTypedNode *typeDeclaration = MAREA_G_FindTypeOfUnit(context, callee);

    if (typeDeclaration != MAREA_NULLPTR) {
        return MAREA_G_LowerConstructCall(context, typed, typeDeclaration, outNode);
    }

    // INFO: a generic callee, or explicit type arguments, ask for an instantiation that no pass performs.
    if ((MAREA_IrTypedKind(callee) == MAREA_AST_KIND_GENERIC_NAME) || (MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR)) {
        return MAREA_G_RaiseGeneric(context, typed, "A call that needs a generic instantiation is not supported by the IR yet.");
    }

    // INFO: an unbound generic result type names no concrete type, so the call cannot carry one.
    if (MAREA_SemaTypedNodeGetTypeClass(typed) == MAREA_SEMA_TYPE_CLASS_GENERIC) {
        return MAREA_G_RaiseGeneric(context, typed, "A call with an unbound generic result type is not supported by the IR yet.");
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_CALL_EXPRESSION, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNode *loweredCallee = MAREA_NULLPTR;

    result = MAREA_G_LowerTyped(context, callee, MAREA_BOOLEAN_TRUE, &loweredCallee);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (loweredCallee == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    result = MAREA_IrNodeAppendChild(context, node, loweredCallee);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_SemaTypedNode *arguments = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_ARGUMENT_LIST);

    if (arguments != MAREA_NULLPTR) {
        MAREA_IrNode *loweredArguments = MAREA_NULLPTR;
        MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

        result = MAREA_G_LowerArgumentsAgainst(context, arguments, MAREA_SemaTypedNodeGetAstNode(typed), MAREA_G_FindCalleeFunction(context, callee), &loweredArguments, &complete);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        if (complete == MAREA_BOOLEAN_FALSE) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }

        result = MAREA_IrNodeAppendChild(context, node, loweredArguments);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    result = MAREA_IrNodeSetTypeNameFromTyped(context, node, typed);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: the structure of this unit named text, found only when it is not generic, because the ir does not instantiate one: a structure of another namespace is the same gap, and an opaque declaration has no field to find.
static const MAREA_SemaTypedNode *MAREA_CALL MAREA_G_FindStructureOfUnit(const MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (text == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(context->unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(context->unit, index);

        if ((MAREA_IrTypedKind(item) != MAREA_AST_KIND_STRUCTURE_DECLARATION) || (MAREA_IrTypedFindChild(item, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
            continue;
        }

        const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER);

        const MAREA_AstNode *nameAst = (name != MAREA_NULLPTR) ? MAREA_SemaTypedNodeGetAstNode(name) : MAREA_NULLPTR;

        if ((nameAst != MAREA_NULLPTR) && (MAREA_AstNodeGetTextLength(nameAst) == length) && (memcmp(MAREA_AstNodeGetText(nameAst), text, length) == 0)) {
            return item;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: Type{...} sets the fields, then runs the type's construct with its defaults, so a literal of a structure of this unit that has a construct operator lowers to a CONSTRUCT_CALL: the defaults are its ARGUMENT_LIST and the literal is its second child, the initial value. Any other literal stays a LITERAL_VALUE.
static MAREA_IrExcept MAREA_CALL MAREA_G_WrapLiteralInConstruct(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *literal, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *constructOperator = MAREA_NULLPTR;

    if (MAREA_G_FindStructureOfUnit(context, literal->typeName, literal->typeNameLength) != MAREA_NULLPTR) {
        constructOperator = MAREA_IrFindLifecycleFunction(context, literal->typeName, literal->typeNameLength, MAREA_BOOLEAN_TRUE);
    }

    if (constructOperator == MAREA_NULLPTR) {
        *outNode = literal;

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *literalAst = MAREA_SemaTypedNodeGetAstNode(typed);

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_CONSTRUCT_CALL, literalAst, &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    node->typeName = literal->typeName;
    node->typeNameLength = literal->typeNameLength;

    MAREA_IrNode *defaults = MAREA_NULLPTR;
    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_G_LowerArgumentsAgainst(context, MAREA_NULLPTR, literalAst, constructOperator, &defaults, &complete);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (complete == MAREA_BOOLEAN_FALSE) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    result = MAREA_IrNodeAppendChild(context, node, defaults);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, node, literal);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerLiteralValue(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_LITERAL_VALUE, typed, &node);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (node == MAREA_NULLPTR)) {
        return result;
    }

    const MAREA_SemaTypedNode *type = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_NAMED_TYPE);

    result = MAREA_IrNodeSetTypeNameFromAst(context, node, MAREA_SemaTypedNodeGetAstNode(type));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: sema rejects a literal that leaves a field out (MISSING_FIELD), so the ir adds no value for one.
    return MAREA_G_WrapLiteralInConstruct(context, typed, node, outNode);
}

// INFO: a field entry is named after its IDENTIFIER and holds the lowered value as its only child.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerFieldEntry(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *name = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    const MAREA_SemaTypedNode *value = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(1));

    if ((name == MAREA_NULLPTR) || (value == MAREA_NULLPTR)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_FIELD_ENTRY, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

    result = MAREA_IrNodeSetQualifiedText(context, node, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNode *lowered = MAREA_NULLPTR;

    result = MAREA_IrLowerExpression(context, value, &lowered);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR)) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, node, lowered);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerExpressionKind(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    switch (MAREA_IrTypedKind(typed)) {
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_LowerIdentifier(context, typed, outNode);
    case MAREA_AST_KIND_INTEGER_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_INTEGER_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_FLOAT_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_FLOAT_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_STRING_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_STRING_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_CHARACTER_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_CHARACTER_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_IrNodeCreate(context, MAREA_IR_KIND_BOOLEAN_LITERAL, MAREA_SemaTypedNodeGetAstNode(typed), outNode);
    case MAREA_AST_KIND_QUALIFIED_NAME:
        // INFO: only the first segment can be a namespace alias.
        return MAREA_G_LowerWithChildrenOf(context, MAREA_IR_KIND_QUALIFIED_NAME, typed, MAREA_BOOLEAN_TRUE, outNode);
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        // INFO: the parentheses only shaped the tree; the inner expression lowers directly.
        return MAREA_IrLowerExpression(context, MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0)), outNode);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_BINARY_EXPRESSION, typed, outNode);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        // INFO: spawn, copy and move keep their operator on a UNARY_EXPRESSION; spawn needs no kind of its own.
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_UNARY_EXPRESSION, typed, outNode);
    case MAREA_AST_KIND_MEMBER_ACCESS:
        // INFO: the member name after the dot is never an alias.
        return MAREA_G_LowerWithChildrenOf(context, MAREA_IR_KIND_MEMBER_ACCESS, typed, MAREA_BOOLEAN_TRUE, outNode);
    case MAREA_AST_KIND_INDEX_ACCESS:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_INDEX_ACCESS, typed, outNode);
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_LowerCall(context, typed, outNode);
    case MAREA_AST_KIND_ARGUMENT_LIST:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_ARGUMENT_LIST, typed, outNode);
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_G_LowerLiteralValue(context, typed, outNode);
    case MAREA_AST_KIND_FIELD_ENTRY:
        return MAREA_G_LowerFieldEntry(context, typed, outNode);
    case MAREA_AST_KIND_INDEX_ENTRY:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_INDEX_ENTRY, typed, outNode);
    case MAREA_AST_KIND_VALUE_ENTRY:
        return MAREA_G_LowerWithChildren(context, MAREA_IR_KIND_VALUE_ENTRY, typed, outNode);
    case MAREA_AST_KIND_GENERIC_NAME:
        return MAREA_G_RaiseGeneric(context, typed, "A generic name is not supported by the IR yet.");
    case MAREA_AST_KIND_MACRO_CALL:
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO, MAREA_SemaTypedNodeGetAstNode(typed), "A macro call inside an expression is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
    default:
        return MAREA_IR_EXCEPT_SUCCESS;
    }
}

// INFO: the kinds whose lowered node must carry the resolved type that sema gave the expression. A parenthesized expression lowers to its inner expression, which carries its own, and an argument list, a field entry or an entry of a literal is not an expression of its own.
static MAREA_Boolean MAREA_CALL MAREA_G_IsTypedExpression(MAREA_AstKind kind)
{
    switch (kind) {
    case MAREA_AST_KIND_IDENTIFIER:
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    case MAREA_AST_KIND_MEMBER_ACCESS:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: true when sema gave the typed node no resolved type and the node is a value that should have had one: a name that only qualifies a member and a written type are not.
static MAREA_Boolean MAREA_CALL MAREA_G_IsUntypedValue(const MAREA_SemaTypedNode *typed)
{
    if ((typed == MAREA_NULLPTR) || (MAREA_G_IsErasedChild(MAREA_IrTypedKind(typed)) == MAREA_BOOLEAN_TRUE) || (MAREA_SemaTypedNodeIsQualifier(typed) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    return ((MAREA_SemaTypedNodeGetResolvedType(typed, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the operands whose type decides the type of an operation: the receiver of a member access, the callee of a call and every child of an operator, an index or a parenthesis. A member name, an argument and a literal entry are not operands.
static MAREA_Boolean MAREA_CALL MAREA_G_HasUntypedOperand(const MAREA_SemaTypedNode *typed)
{
    const MAREA_AstKind KIND = MAREA_IrTypedKind(typed);

    if ((KIND == MAREA_AST_KIND_MEMBER_ACCESS) || (KIND == MAREA_AST_KIND_CALL_EXPRESSION)) {
        return MAREA_G_IsUntypedValue(MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0)));
    }

    if ((KIND != MAREA_AST_KIND_BINARY_EXPRESSION) && (KIND != MAREA_AST_KIND_UNARY_EXPRESSION) && (KIND != MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) && (KIND != MAREA_AST_KIND_INDEX_ACCESS)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_IsUntypedValue(MAREA_IrTypedChildAt(typed, index)) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a task of a call that returns nothing keeps no type in sema, so a spawn of a call typed void is untyped on purpose.
static MAREA_Boolean MAREA_CALL MAREA_G_IsSpawnOfVoid(const MAREA_SemaTypedNode *typed)
{
    if (MAREA_IrTypedKind(typed) != MAREA_AST_KIND_UNARY_EXPRESSION) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    const MAREA_SemaTypedNode *operand = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    if ((operand == MAREA_NULLPTR) || (MAREA_SemaTypedNodeGetResolvedType(operand, &text, &length) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((length == MAREA_INTEGER_SIZE(4)) && (memcmp(text, "void", 4) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: an untyped expression that sema leaves untyped on purpose (the closed list of "Gaps" in the sema doc) is kept without a type and raises nothing: a name (a loop name, a function or a built-in name, any callee), a call whose callee has no type, an operation with an untyped operand, and a spawn of a void call. Every other untyped expression is a hole in sema and raises UNTYPED_EXPRESSION.
static MAREA_Boolean MAREA_CALL MAREA_G_UntypedOnPurpose(const MAREA_SemaTypedNode *typed, MAREA_Boolean asCallee)
{
    if ((asCallee == MAREA_BOOLEAN_TRUE) || (MAREA_IrTypedKind(typed) == MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_G_HasUntypedOperand(typed) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsSpawnOfVoid(typed) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerTyped(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_Boolean asCallee, MAREA_IrNode **outNode)
{
    *outNode = MAREA_NULLPTR;

    MAREA_IrNode *lowered = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_LowerExpressionKind(context, typed, &lowered);

    // INFO: a node that lowered to nothing already raised its own diagnostic, or its parent is left out because of a child that did; either way no second one is raised for the same expression.
    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR) || (MAREA_G_IsTypedExpression(MAREA_IrTypedKind(typed)) == MAREA_BOOLEAN_FALSE)) {
        *outNode = lowered;

        return result;
    }

    MAREA_Boolean typedResult = MAREA_BOOLEAN_FALSE;

    result = MAREA_IrNodeSetResolvedTypeFromTyped(context, lowered, typed, &typedResult);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: a name that only qualifies a member is not a value, so it has no type and is not an expression that is missing one.
    if ((typedResult == MAREA_BOOLEAN_FALSE) && (MAREA_SemaTypedNodeIsQualifier(typed) == MAREA_BOOLEAN_FALSE) && (MAREA_G_UntypedOnPurpose(typed, asCallee) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNTYPED_EXPRESSION, MAREA_SemaTypedNodeGetAstNode(typed), "An expression with no resolved type is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
    }

    *outNode = lowered;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerExpression(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
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

    return MAREA_G_LowerTyped(context, typed, MAREA_BOOLEAN_FALSE, outNode);
}
