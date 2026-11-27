// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

// INFO: the built-in functions that take explicit type arguments; the IR knows no other.
static const MAREA_Character8 *const MAREA_G_BuiltinNames[] = {
    "size_of",
    "align_of",
    "convert",
};

static MAREA_Boolean MAREA_CALL MAREA_G_TextEquals(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *name)
{
    if (length != strlen(name)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(text, name, length) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBuiltinName(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < MAREA_ARRAY_SIZE(MAREA_G_BuiltinNames); index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_TextEquals(text, length, MAREA_G_BuiltinNames[index]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: true when an IDENTIFIER typed node is written as text.
static MAREA_Boolean MAREA_CALL MAREA_G_IdentifierIs(const MAREA_SemaTypedNode *identifier, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_AstNode *ast = MAREA_SemaTypedNodeGetAstNode(identifier);

    if (MAREA_AstNodeGetTextLength(ast) != length) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(MAREA_AstNodeGetText(ast), text, length) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: an import binds its alias when it has one, otherwise the last segment of its path. A member list binds one name
// INFO: per member: its alias, or the last segment of its member path.
static MAREA_Boolean MAREA_CALL MAREA_G_ImportBindsName(const MAREA_SemaTypedNode *item, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_SemaTypedNode *path = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_QUALIFIED_NAME);

    if ((MAREA_AstNodeGetFlags(MAREA_SemaTypedNodeGetAstNode(item)) & (MAREA_Uinteger32)MAREA_AST_FLAG_HAS_MEMBERS) == 0) {
        const MAREA_SemaTypedNode *alias = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER);

        const MAREA_SemaTypedNode *bound = (alias != MAREA_NULLPTR) ? alias : MAREA_IrTypedChildAt(path, (MAREA_SemaTypedNodeGetChildCount(path) - MAREA_INTEGER_SIZE(1)));

        return MAREA_G_IdentifierIs(bound, text, length);
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(item);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *member = MAREA_IrTypedChildAt(item, index);

        if (MAREA_IrTypedKind(member) != MAREA_AST_KIND_IMPORT_MEMBER) {
            continue;
        }

        const MAREA_SemaTypedNode *alias = MAREA_IrTypedFindChild(member, MAREA_AST_KIND_IDENTIFIER);

        const MAREA_SemaTypedNode *memberPath = MAREA_IrTypedFindChild(member, MAREA_AST_KIND_QUALIFIED_NAME);

        const MAREA_SemaTypedNode *bound = (alias != MAREA_NULLPTR) ? alias : MAREA_IrTypedChildAt(memberPath, (MAREA_SemaTypedNodeGetChildCount(memberPath) - MAREA_INTEGER_SIZE(1)));

        if (MAREA_G_IdentifierIs(bound, text, length) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a function of the unit, or an import, with the name of a built-in is a call to a user function.
static MAREA_Boolean MAREA_CALL MAREA_G_UnitBindsName(const MAREA_IrContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(context->unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(context->unit, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(item);

        if (KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) {
            if (MAREA_G_IdentifierIs(MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER), text, length) == MAREA_BOOLEAN_TRUE) {
                return MAREA_BOOLEAN_TRUE;
            }
        } else if (KIND == MAREA_AST_KIND_IMPORT_DECLARATION) {
            if (MAREA_G_ImportBindsName(item, text, length) == MAREA_BOOLEAN_TRUE) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a generic structure or type alias of the unit is a user generic, which the IR cannot instantiate.
static MAREA_Boolean MAREA_CALL MAREA_G_UnitDeclaresGenericType(const MAREA_IrContext *context, const MAREA_SemaTypedNode *head)
{
    // INFO: a dotted head names a type of another namespace.
    if (MAREA_SemaTypedNodeGetChildCount(head) != MAREA_INTEGER_SIZE(1)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(head, MAREA_INTEGER_SIZE(0)));

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(context->unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(context->unit, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(item);

        if (((KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (KIND == MAREA_AST_KIND_TYPE_DECLARATION)) && (MAREA_IrTypedFindChild(item, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR) && (MAREA_G_IdentifierIs(MAREA_IrTypedFindChild(item, MAREA_AST_KIND_IDENTIFIER), MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerTypeName(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerFunctionType(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode);

// INFO: the text of the TYPE_NAME that stands for a function type; function is a keyword, so no written type can have this name.
static const MAREA_Character8 MAREA_G_FunctionTypeName[] = "function";

// INFO: a written type is a NAMED_TYPE or a FUNCTION_TYPE. Any other kind makes no node and raises nothing.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerTypeOperand(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    *outNode = MAREA_NULLPTR;

    if (MAREA_IrTypedKind(typed) == MAREA_AST_KIND_FUNCTION_TYPE) {
        return MAREA_G_LowerFunctionType(context, typed, outNode);
    }

    if (MAREA_IrTypedKind(typed) == MAREA_AST_KIND_NAMED_TYPE) {
        return MAREA_G_LowerTypeName(context, typed, outNode);
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: the first direct child that is a written type, or null.
static const MAREA_SemaTypedNode *MAREA_CALL MAREA_G_FindWrittenType(const MAREA_SemaTypedNode *typed)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *child = MAREA_IrTypedChildAt(typed, index);

        if ((MAREA_IrTypedKind(child) == MAREA_AST_KIND_NAMED_TYPE) || (MAREA_IrTypedKind(child) == MAREA_AST_KIND_FUNCTION_TYPE)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: function(a: A, b: B): R becomes TYPE_NAME function (PARAMETER_LIST (TYPE_NAME A) (TYPE_NAME B)) (TYPE_NAME R). The list is there even when it is empty, so a result is always the second child. A gap in one part leaves the whole type out.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerFunctionType(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    *outNode = MAREA_NULLPTR;

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_TYPE_NAME, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    node->text = MAREA_G_FunctionTypeName;
    node->textLength = (MAREA_Uinteger32)strlen(MAREA_G_FunctionTypeName);

    const MAREA_SemaTypedNode *parameters = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_PARAMETER_LIST);

    MAREA_IrNode *list = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_PARAMETER_LIST, MAREA_SemaTypedNodeGetAstNode((parameters != MAREA_NULLPTR) ? parameters : typed), &list);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_IntegerSize COUNT = (parameters != MAREA_NULLPTR) ? MAREA_SemaTypedNodeGetChildCount(parameters) : MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *parameter = MAREA_IrTypedChildAt(parameters, index);

        const MAREA_SemaTypedNode *type = (MAREA_IrTypedKind(parameter) == MAREA_AST_KIND_PARAMETER) ? MAREA_G_FindWrittenType(parameter) : MAREA_NULLPTR;

        if (type == MAREA_NULLPTR) {
            continue;
        }

        MAREA_IrNode *lowered = MAREA_NULLPTR;

        result = MAREA_G_LowerTypeOperand(context, type, &lowered);

        if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR)) {
            return result;
        }

        result = MAREA_IrNodeAppendChild(context, list, lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    result = MAREA_IrNodeAppendChild(context, node, list);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_SemaTypedNode *resultType = MAREA_G_FindWrittenType(typed);

    if (resultType != MAREA_NULLPTR) {
        MAREA_IrNode *lowered = MAREA_NULLPTR;

        result = MAREA_G_LowerTypeOperand(context, resultType, &lowered);

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

// INFO: a type argument is a written type (a named type or a function type) or a number.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerTypeArguments(MAREA_IrContext *context, const MAREA_SemaTypedNode *arguments, MAREA_IrNode *parent, MAREA_Boolean *outComplete)
{
    *outComplete = MAREA_BOOLEAN_FALSE;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(arguments);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *argument = MAREA_IrTypedChildAt(arguments, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(argument);

        MAREA_IrNode *lowered = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_IR_EXCEPT_SUCCESS;

        if ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) {
            result = MAREA_G_LowerTypeOperand(context, argument, &lowered);
        } else if (KIND == MAREA_AST_KIND_INTEGER_LITERAL) {
            result = MAREA_IrLowerExpression(context, argument, &lowered);
        } else {
            return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_SemaTypedNodeGetAstNode(argument), "A type argument that is not a written type or a number is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
        }

        if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR)) {
            return result;
        }

        result = MAREA_IrNodeAppendChild(context, parent, lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    *outComplete = MAREA_BOOLEAN_TRUE;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: a NAMED_TYPE and a GENERIC_NAME share one layout: a QUALIFIED_NAME, then optional TYPE_ARGUMENTS.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerTypeName(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode **outNode)
{
    *outNode = MAREA_NULLPTR;

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_TYPE_NAME, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (MAREA_IrAstQualifiedNameSpan(MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0))), &text, &length) == MAREA_BOOLEAN_FALSE) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    // INFO: an alias that carries type arguments or names a function type cannot stand for the whole operand.
    if (MAREA_IrAliasTypeIsLossy(context, text, length) == MAREA_BOOLEAN_TRUE) {
        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_SemaTypedNodeGetAstNode(typed), "A type operand named by a type alias with type arguments is not supported by the IR yet.", MAREA_NULLPTR, MAREA_INTEGER_SIZE(0));
    }

    // INFO: the type operand names the original type, never an alias of it.
    result = MAREA_IrAliasType(context, text, length, &text, &length);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    node->text = text;
    node->textLength = (MAREA_Uinteger32)length;

    const MAREA_SemaTypedNode *arguments = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if (arguments != MAREA_NULLPTR) {
        MAREA_Boolean complete = MAREA_BOOLEAN_FALSE;

        result = MAREA_G_LowerTypeArguments(context, arguments, node, &complete);

        if ((result != MAREA_IR_EXCEPT_SUCCESS) || (complete == MAREA_BOOLEAN_FALSE)) {
            return result;
        }
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

// INFO: the call keeps the layout of every call: the callee first, then the argument list.
static MAREA_IrExcept MAREA_CALL MAREA_G_FinishCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *callee, MAREA_IrNode **outNode)
{
    const MAREA_SemaTypedNode *arguments = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_ARGUMENT_LIST);

    if (arguments == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    MAREA_IrNode *lowered = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, arguments, &lowered);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (lowered == MAREA_NULLPTR)) {
        return result;
    }

    MAREA_IrNode *call = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_CALL_EXPRESSION, MAREA_SemaTypedNodeGetAstNode(typed), &call);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, call, callee);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, call, lowered);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeSetTypeNameFromTyped(context, call, typed);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = call;

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerBuiltinCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_SemaTypedNode *callee, const MAREA_SemaTypedNode *typeArguments, MAREA_IrNode **outNode)
{
    MAREA_IrNode *name = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_BUILTIN_NAME, MAREA_SemaTypedNodeGetAstNode(callee), &name);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_FALSE;

    result = MAREA_G_LowerTypeArguments(context, typeArguments, name, &complete);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (complete == MAREA_BOOLEAN_FALSE)) {
        return result;
    }

    return MAREA_G_FinishCall(context, typed, name, outNode);
}

// INFO: the callee is a MEMBER_ACCESS whose receiver is the written type, so the call keeps the shape of a member call.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStaticCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_SemaTypedNode *callee, MAREA_IrNode **outNode)
{
    MAREA_IrNode *type = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_G_LowerTypeName(context, MAREA_IrTypedChildAt(callee, MAREA_INTEGER_SIZE(0)), &type);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (type == MAREA_NULLPTR)) {
        return result;
    }

    // INFO: the member after the dot names a function of the type, so it is never an alias.
    MAREA_IrNode *member = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_IDENTIFIER, MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(callee, MAREA_INTEGER_SIZE(1))), &member);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_IrNode *access = MAREA_NULLPTR;

    result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_MEMBER_ACCESS, MAREA_SemaTypedNodeGetAstNode(callee), &access);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, access, type);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    result = MAREA_IrNodeAppendChild(context, access, member);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    return MAREA_G_FinishCall(context, typed, access, outNode);
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerTypeOperandCall(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_Boolean *outHandled, MAREA_IrNode **outNode)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outHandled == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outNode == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    *outHandled = MAREA_BOOLEAN_FALSE;
    *outNode = MAREA_NULLPTR;

    const MAREA_SemaTypedNode *callee = MAREA_IrTypedChildAt(typed, MAREA_INTEGER_SIZE(0));

    const MAREA_SemaTypedNode *typeArguments = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if (callee == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    if ((MAREA_IrTypedKind(callee) == MAREA_AST_KIND_IDENTIFIER) && (typeArguments != MAREA_NULLPTR)) {
        const MAREA_AstNode *name = MAREA_SemaTypedNodeGetAstNode(callee);

        if ((MAREA_G_IsBuiltinName(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) && (MAREA_G_UnitBindsName(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_FALSE)) {
            *outHandled = MAREA_BOOLEAN_TRUE;

            return MAREA_G_LowerBuiltinCall(context, typed, callee, typeArguments, outNode);
        }
    }

    if ((MAREA_IrTypedKind(callee) == MAREA_AST_KIND_MEMBER_ACCESS) && (typeArguments == MAREA_NULLPTR)) {
        const MAREA_SemaTypedNode *receiver = MAREA_IrTypedChildAt(callee, MAREA_INTEGER_SIZE(0));

        if ((MAREA_IrTypedKind(receiver) == MAREA_AST_KIND_GENERIC_NAME) && (MAREA_G_UnitDeclaresGenericType(context, MAREA_IrTypedChildAt(receiver, MAREA_INTEGER_SIZE(0))) == MAREA_BOOLEAN_FALSE)) {
            *outHandled = MAREA_BOOLEAN_TRUE;

            return MAREA_G_LowerStaticCall(context, typed, callee, outNode);
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}
