// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

typedef enum MAREA_G_LifecycleKind
{
    MAREA_G_LIFECYCLE_KIND_NONE = 0,
    MAREA_G_LIFECYCLE_KIND_CONSTRUCT = 1,
    MAREA_G_LIFECYCLE_KIND_DESTROY = 2,
} MAREA_G_LifecycleKind;

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

// INFO: the text of a string literal keeps its two quotes, so the inner bytes are compared.
static MAREA_Boolean MAREA_CALL MAREA_G_IsQuoted(const MAREA_AstNode *value, const MAREA_Character8 *inner, MAREA_IntegerSize innerLength)
{
    if ((value == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(value) != MAREA_AST_KIND_STRING_LITERAL) || (MAREA_AstNodeGetTextLength(value) != (innerLength + 2))) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_SemaTextEquals((MAREA_AstNodeGetText(value) + 1), innerLength, inner, innerLength);
}

static MAREA_G_LifecycleKind MAREA_CALL MAREA_G_KindOf(const MAREA_AstNode *function)
{
    if ((function == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(function) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
        return MAREA_G_LIFECYCLE_KIND_NONE;
    }

    const MAREA_AstNode *item = MAREA_SemaAttributeFind(MAREA_NULLPTR, function, "operator");

    const MAREA_AstNode *value = MAREA_SemaChildAt(item, 1);

    if (MAREA_G_IsQuoted(value, "construct", 9) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_LIFECYCLE_KIND_CONSTRUCT;
    }

    return (MAREA_G_IsQuoted(value, "destroy", 7) == MAREA_BOOLEAN_TRUE) ? MAREA_G_LIFECYCLE_KIND_DESTROY : MAREA_G_LIFECYCLE_KIND_NONE;
}

// INFO: the name of the implemented type when it is one plain segment, else null: a path names a type of another namespace, which this file does not declare.
static const MAREA_AstNode *MAREA_CALL MAREA_G_TypeName(const MAREA_AstNode *implementation)
{
    const MAREA_AstNode *type = MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE);

    const MAREA_AstNode *qualified = (type == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(type, MAREA_AST_KIND_QUALIFIED_NAME);

    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(qualified) != 1)) {
        return MAREA_NULLPTR;
    }

    return MAREA_SemaChildAt(qualified, 0);
}

// INFO: a type of this file must be a structure or an enum. A name this file does not declare is accepted, because the unknown type is reported by another rule.
static MAREA_Boolean MAREA_CALL MAREA_G_IsContainer(const MAREA_SemaContext *context, const MAREA_AstNode *implementation)
{
    const MAREA_AstNode *name = MAREA_G_TypeName(implementation);

    if (name == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(entry->declaration);

    return ((KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (KIND == MAREA_AST_KIND_ENUM_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasRequiredParameter(const MAREA_AstNode *parameters)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(parameters);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        if ((parameter != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(parameter) == MAREA_AST_KIND_PARAMETER) && (MAREA_SemaParameterHasDefault(parameter) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportFunction(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, const MAREA_AstNode *function, const MAREA_Character8 *prefix)
{
    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaDeclarationName(function, &name, &length) == MAREA_BOOLEAN_FALSE) {
        MAREA_SemaReport(context, code, MAREA_AstNodeGetPosition(function), prefix);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, code, MAREA_AstNodeGetPosition(function), prefix, name, length);
}

typedef struct MAREA_G_OperatorRule
{
    const MAREA_Character8 *token;
    MAREA_IntegerSize minimum;
    MAREA_IntegerSize maximum;
    MAREA_Boolean needsResult;
    MAREA_Boolean firstIsSelf;
    MAREA_Boolean returnsBoolean;
} MAREA_G_OperatorRule;

#define MAREA_G_OPERATOR_UNBOUNDED MAREA_INTEGER_SIZE_MAXIMUM

// INFO: the signature table of the operator tokens. Only the parameter count, the presence of a result, the type of the first parameter and a boolean result are fixed: every other type stays free.
static const MAREA_G_OperatorRule MAREA_G_OPERATOR_RULES[] = {
    { "+", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "-", 1, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "*", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "/", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "%", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "**", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "+%", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "-%", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "*%", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "+|", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "-|", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "*|", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "&", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "|", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "^", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "<<", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { ">>", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE },
    { "==", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE },
    { "!=", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE },
    { "<", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE },
    { "<=", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE },
    { ">", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE },
    { ">=", 2, 2, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_TRUE },
    { "~", 1, 1, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "+=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "-=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "*=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "/=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "%=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "**=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "<<=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { ">>=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "&=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "|=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "^=", 2, 2, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "[]", 2, MAREA_G_OPERATOR_UNBOUNDED, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "()", 1, MAREA_G_OPERATOR_UNBOUNDED, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
    { "convert", 1, 1, MAREA_BOOLEAN_TRUE, MAREA_BOOLEAN_FALSE, MAREA_BOOLEAN_FALSE },
};

// INFO: the one plain name of a type node, or false for a path, a function type or a missing node.
static MAREA_Boolean MAREA_CALL MAREA_G_PlainTypeName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *qualified = MAREA_G_FindChild(type, MAREA_AST_KIND_QUALIFIED_NAME);

    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(qualified) != 1)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(qualified, 0);

    if (segment == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outText = MAREA_AstNodeGetText(segment);
    *outLength = MAREA_AstNodeGetTextLength(segment);

    return MAREA_BOOLEAN_TRUE;
}

static const MAREA_G_OperatorRule *MAREA_CALL MAREA_G_FindOperatorRule(const MAREA_AstNode *value)
{
    for (MAREA_IntegerSize index = 0; index < (sizeof(MAREA_G_OPERATOR_RULES) / sizeof(MAREA_G_OPERATOR_RULES[0])); ++index) {
        const MAREA_G_OperatorRule *rule = &MAREA_G_OPERATOR_RULES[index];

        if (MAREA_G_IsQuoted(value, rule->token, strlen(rule->token)) == MAREA_BOOLEAN_TRUE) {
            return rule;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: an operator function in an implementation must fit the table above. A first parameter whose type is not one plain name, or is missing, is not judged.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckOperatorSignature(MAREA_SemaContext *context, const MAREA_AstNode *function)
{
    const MAREA_AstNode *item = MAREA_SemaAttributeFind(MAREA_NULLPTR, function, "operator");

    const MAREA_G_OperatorRule *rule = MAREA_G_FindOperatorRule(MAREA_SemaChildAt(item, 1));

    if ((rule == MAREA_NULLPTR) || (context->implementation == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *parameters = MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_IntegerSize COUNT = (parameters == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(parameters);

    const MAREA_AstNode *result = MAREA_SemaReturnType(function);

    MAREA_Boolean bad = ((COUNT < rule->minimum) || (COUNT > rule->maximum) || ((rule->needsResult == MAREA_BOOLEAN_TRUE) && (result == MAREA_NULLPTR))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((bad == MAREA_BOOLEAN_FALSE) && (rule->returnsBoolean == MAREA_BOOLEAN_TRUE) && ((MAREA_G_PlainTypeName(result, &text, &length) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaTextEquals(text, length, "boolean", 7) == MAREA_BOOLEAN_FALSE))) {
        bad = MAREA_BOOLEAN_TRUE;
    }

    if ((bad == MAREA_BOOLEAN_FALSE) && (rule->firstIsSelf == MAREA_BOOLEAN_TRUE)) {
        const MAREA_AstNode *implemented = MAREA_G_TypeName(context->implementation);

        if ((implemented != MAREA_NULLPTR) && (MAREA_G_PlainTypeName(MAREA_SemaChildAt(MAREA_SemaChildAt(parameters, 0), 1), &text, &length) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(text, length, "Self", 4) == MAREA_BOOLEAN_FALSE) && (MAREA_SemaTextEquals(text, length, MAREA_AstNodeGetText(implemented), MAREA_AstNodeGetTextLength(implemented)) == MAREA_BOOLEAN_FALSE)) {
            bad = MAREA_BOOLEAN_TRUE;
        }
    }

    if (bad == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportFunction(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPERATOR_SIGNATURE_MISMATCH, function, "operator signature does not fit its operator:");
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLifecycle(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_G_LifecycleKind KIND = MAREA_G_KindOf(node);

    if (KIND == MAREA_G_LIFECYCLE_KIND_NONE) {
        return ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_FUNCTION_DECLARATION) && (context->implementation != MAREA_NULLPTR)) ? MAREA_G_CheckOperatorSignature(context, node) : MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *implementation = context->implementation;

    const MAREA_Boolean CONCRETE = ((implementation != MAREA_NULLPTR) && (MAREA_G_FindChild(implementation, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_NULLPTR) && (MAREA_G_FindChild(node, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_NULLPTR) && (MAREA_G_IsContainer(context, implementation) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (CONCRETE == MAREA_BOOLEAN_FALSE) {
        const MAREA_SemaExcept RESULT = MAREA_G_ReportFunction(context, MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_NEEDS_CONCRETE_TYPE, node, "construct and destroy need a concrete structure or enum implementation:");

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    const MAREA_AstNode *parameters = MAREA_G_FindChild(node, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_Boolean HAS_RETURN = (MAREA_SemaReturnType(node) != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_Boolean HAS_PARAMETER = ((parameters != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(parameters) > 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_Boolean BAD = ((HAS_RETURN == MAREA_BOOLEAN_TRUE) || ((KIND == MAREA_G_LIFECYCLE_KIND_DESTROY) && (HAS_PARAMETER == MAREA_BOOLEAN_TRUE)) || ((KIND == MAREA_G_LIFECYCLE_KIND_CONSTRUCT) && (MAREA_G_HasRequiredParameter(parameters) == MAREA_BOOLEAN_TRUE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (BAD == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportFunction(context, MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_BAD_SHAPE, node, (KIND == MAREA_G_LIFECYCLE_KIND_DESTROY) ? "destroy takes no parameter and has no return type:" : "construct has no return type and every parameter has a default:");
}

// INFO: finds a lifecycle function by its name in the implementations of the file. A type name, when given, must be the implemented type.
static MAREA_Boolean MAREA_CALL MAREA_G_HasLifecycleMember(const MAREA_SemaContext *context, const MAREA_AstNode *typeName, const MAREA_AstNode *name)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(context->root);

    for (MAREA_IntegerSize current = 0; current < COUNT; ++current) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, current);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
            continue;
        }

        if (typeName != MAREA_NULLPTR) {
            const MAREA_AstNode *implemented = MAREA_G_TypeName(implementation);

            if ((implemented == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(implemented), MAREA_AstNodeGetTextLength(implemented), MAREA_AstNodeGetText(typeName), MAREA_AstNodeGetTextLength(typeName)) == MAREA_BOOLEAN_FALSE)) {
                continue;
            }
        }

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_AstNodeGetChildCount(implementation);

        for (MAREA_IntegerSize index = 0; index < MEMBER_COUNT; ++index) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, index);

            const MAREA_Character8 *text = MAREA_NULLPTR;

            MAREA_IntegerSize length = 0;

            if ((MAREA_G_KindOf(member) != MAREA_G_LIFECYCLE_KIND_NONE) && (MAREA_SemaDeclarationName(member, &text, &length) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(text, length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the implemented type of an implementation block, read back through the name table so identity is compared
// by declaration, not by text: two types of the same name in two namespaces never collide here, as the table holds
// the declaration of this file only.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ImplementedDeclaration(const MAREA_SemaContext *context, const MAREA_AstNode *implementation)
{
    const MAREA_AstNode *name = MAREA_G_TypeName(implementation);

    if (name == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION)) ? entry->declaration : MAREA_NULLPTR;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaFindOperatorFunction(const MAREA_SemaContext *context, const MAREA_AstNode *typeDeclaration, const MAREA_Character8 *token, MAREA_IntegerSize tokenLength, MAREA_IntegerSize *outCount)
{
    if (outCount != MAREA_NULLPTR) {
        *outCount = 0;
    }

    if ((context == MAREA_NULLPTR) || (typeDeclaration == MAREA_NULLPTR) || (token == MAREA_NULLPTR) || (context->root == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(context->root);

    const MAREA_AstNode *found = MAREA_NULLPTR;

    for (MAREA_IntegerSize current = 0; current < COUNT; ++current) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, current);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) || (MAREA_G_ImplementedDeclaration(context, implementation) != typeDeclaration)) {
            continue;
        }

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_AstNodeGetChildCount(implementation);

        for (MAREA_IntegerSize index = 0; index < MEMBER_COUNT; ++index) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, index);

            if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
                continue;
            }

            const MAREA_AstNode *item = MAREA_SemaAttributeFind(MAREA_NULLPTR, member, "operator");

            if (MAREA_G_IsQuoted(MAREA_SemaChildAt(item, 1), token, tokenLength) == MAREA_BOOLEAN_FALSE) {
                continue;
            }

            found = (found == MAREA_NULLPTR) ? member : found;

            if (outCount != MAREA_NULLPTR) {
                *outCount += 1;
            }
        }
    }

    return found;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaFindOperatorFunctionInTree(const MAREA_AstNode *root, const MAREA_AstNode *typeDeclaration, const MAREA_Character8 *token, MAREA_IntegerSize tokenLength, MAREA_IntegerSize *outCount)
{
    if (outCount != MAREA_NULLPTR) {
        *outCount = 0;
    }

    const MAREA_Character8 *typeName = MAREA_NULLPTR;

    MAREA_IntegerSize typeLength = 0;

    if ((root == MAREA_NULLPTR) || (token == MAREA_NULLPTR) || (MAREA_SemaDeclarationName(typeDeclaration, &typeName, &typeLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    const MAREA_AstNode *found = MAREA_NULLPTR;

    for (MAREA_IntegerSize current = 0; current < COUNT; ++current) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(root, current);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_G_TypeName(implementation);

        // INFO: the name table holds the file of the run only, so a type of another tree is matched by its name.
        if ((name == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), typeName, typeLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_AstNodeGetChildCount(implementation);

        for (MAREA_IntegerSize index = 0; index < MEMBER_COUNT; ++index) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, index);

            if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
                continue;
            }

            const MAREA_AstNode *item = MAREA_SemaAttributeFind(MAREA_NULLPTR, member, "operator");

            if (MAREA_G_IsQuoted(MAREA_SemaChildAt(item, 1), token, tokenLength) == MAREA_BOOLEAN_FALSE) {
                continue;
            }

            found = (found == MAREA_NULLPTR) ? member : found;

            if (outCount != MAREA_NULLPTR) {
                *outCount += 1;
            }
        }
    }

    return found;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaFindConstructOperator(const MAREA_SemaContext *context, const MAREA_AstNode *typeDeclaration)
{
    return MAREA_SemaFindOperatorFunction(context, typeDeclaration, "construct", strlen("construct"), MAREA_NULLPTR);
}

// INFO: the one plain name segment of an inferred type, or null for a path, a function type or an unknown type.
static const MAREA_AstNode *MAREA_CALL MAREA_G_PlainTypeSegment(const MAREA_AstNode *type)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *qualified = MAREA_G_FindChild(type, MAREA_AST_KIND_QUALIFIED_NAME);

    return ((qualified != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(qualified) == 1)) ? MAREA_SemaChildAt(qualified, 0) : MAREA_NULLPTR;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLifecycleCall(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if (callee == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *name = MAREA_NULLPTR;

    MAREA_Boolean direct = MAREA_BOOLEAN_FALSE;

    if (MAREA_AstNodeGetKind(callee) == MAREA_AST_KIND_IDENTIFIER) {
        name = callee;

        // INFO: a local hides a member, and a function of the file with the same name is the callee instead of the member.
        if (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_NULLPTR) {
            const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

            if (entry == MAREA_NULLPTR) {
                direct = MAREA_G_HasLifecycleMember(context, MAREA_NULLPTR, name);
            } else {
                direct = (MAREA_G_KindOf((entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) ? entry->declaration : MAREA_NULLPTR) != MAREA_G_LIFECYCLE_KIND_NONE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
            }
        }
    } else if (MAREA_AstNodeGetKind(callee) == MAREA_AST_KIND_MEMBER_ACCESS) {
        const MAREA_AstNode *object = MAREA_SemaChildAt(callee, 0);

        name = MAREA_SemaChildAt(callee, 1);

        if ((object != MAREA_NULLPTR) && (name != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(object) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(object), MAREA_AstNodeGetTextLength(object)) == MAREA_NULLPTR)) {
            direct = MAREA_G_HasLifecycleMember(context, object, name);
        } else if ((object != MAREA_NULLPTR) && (name != MAREA_NULLPTR)) {
            // INFO: a call on a value. Only a receiver whose inferred type is one plain name of this file is judged; any other receiver is never guessed.
            const MAREA_AstNode *typeName = MAREA_G_PlainTypeSegment(MAREA_SemaInferType(context, object));

            if (typeName != MAREA_NULLPTR) {
                direct = MAREA_G_HasLifecycleMember(context, typeName, name);
            }
        }
    }

    if (direct == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_LIFECYCLE_OPERATOR_CALLED_DIRECTLY, MAREA_AstNodeGetPosition(call), "construct and destroy are never called directly:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}
