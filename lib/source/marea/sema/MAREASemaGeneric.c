// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_INSTANTIATION_LIMIT MAREA_UINTEGER_32(64)
#define MAREA_G_BINDING_LIMIT       MAREA_INTEGER_SIZE(16)
#define MAREA_G_RENDER_LIMIT        MAREA_INTEGER_SIZE(512)
#define MAREA_G_ALIAS_LIMIT         MAREA_UINTEGER_32(8)

typedef struct MAREA_G_Binding
{
    const MAREA_Character8 *name;
    const MAREA_AstNode *type;
    MAREA_IntegerSize length;
} MAREA_G_Binding;

typedef struct MAREA_G_Bindings
{
    MAREA_G_Binding items[MAREA_G_BINDING_LIMIT];
    MAREA_IntegerSize count;
} MAREA_G_Bindings;

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

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_SemaTextEquals(text, length, expected, strlen(expected));
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsPrimitiveTypeName(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    static const MAREA_Character8 *const PRIMITIVES[] = { "boolean", "sinteger8", "sinteger16", "sinteger32", "sinteger64", "uinteger8", "uinteger16", "uinteger32", "uinteger64", "float32", "float64", "character", "string" };

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(PRIMITIVES); ++index) {
        if (MAREA_G_TextIs(text, length, PRIMITIVES[index]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_SingleName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(type, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(name) != 1)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(name, 0);

    if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outText = MAREA_AstNodeGetText(segment);
    *outLength = MAREA_AstNodeGetTextLength(segment);

    return MAREA_BOOLEAN_TRUE;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaReturnType(const MAREA_AstNode *function)
{
    if (function == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(function);

    MAREA_Boolean afterParameters = MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(function, index);

        if (child == MAREA_NULLPTR) {
            continue;
        }

        if (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_PARAMETER_LIST) {
            afterParameters = MAREA_BOOLEAN_TRUE;
        } else if ((afterParameters == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsType(child) == MAREA_BOOLEAN_TRUE)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaDeclareLocal(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(node, 0);

    const MAREA_AstNode *type = MAREA_SemaChildAt(node, 1);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a local named like a built-in macro is reported and still declared, so its uses raise nothing more.
    if (MAREA_SemaIsBuiltInMacroName(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
        const MAREA_SemaExcept MACRO_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_MACRO_NAME, MAREA_AstNodeGetPosition(name), "name is a built-in macro:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

        if (MACRO_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return MACRO_RESULT;
        }
    }

    // INFO: a new local of the same name as a reference is a new owner, so the reference no longer holds.
    if ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_LOCAL_DECLARATION) && (MAREA_SemaIsReference(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
        const MAREA_SemaExcept SHADOW_RESULT = MAREA_SemaTableAppend(context, &context->references, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });

        if (SHADOW_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return SHADOW_RESULT;
        }
    }

    // INFO: the same shadow rule for the read-only table: a new local is writable even when it reuses the name of a read-only borrow.
    if ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_LOCAL_DECLARATION) && (MAREA_SemaIsReadOnly(context, name) == MAREA_BOOLEAN_TRUE)) {
        const MAREA_SemaExcept WRITABLE_RESULT = MAREA_SemaTableAppend(context, &context->readOnly, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });

        if (WRITABLE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return WRITABLE_RESULT;
        }
    }

    return MAREA_SemaTableAppend(context, &context->locals, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = type, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaInstantiationEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Boolean *outEntered)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outEntered == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outEntered = MAREA_BOOLEAN_FALSE;

    if (MAREA_G_FindChild(node, MAREA_AST_KIND_TYPE_ARGUMENTS) == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    context->instantiationDepth += 1;
    *outEntered = MAREA_BOOLEAN_TRUE;

    if (context->instantiationDepth != (MAREA_G_INSTANTIATION_LIMIT + 1)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_INSTANTIATION_TOO_DEEP, MAREA_AstNodeGetPosition(node), "generic instantiation is nested more than 64 levels");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaInstantiationLeave(MAREA_SemaContext *context, MAREA_Boolean entered)
{
    if (entered == MAREA_BOOLEAN_TRUE) {
        context->instantiationDepth -= 1;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_ConstraintListed(const MAREA_AstNode *function, const MAREA_Character8 *parameter, MAREA_IntegerSize parameterLength, const MAREA_Character8 *contract, MAREA_IntegerSize contractLength)
{
    const MAREA_AstNode *where = (function == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(function, MAREA_AST_KIND_WHERE_CLAUSE);

    if (where == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(where);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *constraint = MAREA_SemaChildAt(where, index);

        if ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_SemaChildAt(constraint, 0);

        if ((name == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), parameter, parameterLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *qualified = MAREA_SemaChildAt(constraint, contractIndex);

            const MAREA_AstNode *segment = ((qualified != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(qualified) == 1)) ? MAREA_SemaChildAt(qualified, 0) : MAREA_NULLPTR;

            if ((segment != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment), contract, contractLength) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsContractKnown(const MAREA_SemaContext *context, const MAREA_AstNode *qualified)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(qualified);

    const MAREA_AstNode *first = MAREA_SemaChildAt(qualified, 0);

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(first), MAREA_AstNodeGetTextLength(first));

    if (COUNT > 1) {
        return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_NAMESPACE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_G_TextIs(MAREA_AstNodeGetText(first), MAREA_AstNodeGetTextLength(first), "Copyable") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(MAREA_AstNodeGetText(first), MAREA_AstNodeGetTextLength(first), "Movable") == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (entry == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_CONTRACT_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsPlainName(const MAREA_AstNode *qualified)
{
    if ((qualified == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualified) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(qualified) == 0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(qualified); ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(qualified, index);

        if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckWhereClause(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *constraint = MAREA_SemaChildAt(node, index);

        if ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_SemaChildAt(constraint, 0);

        if ((name != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaIsGenericInScope(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_FALSE)) {
            const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME, MAREA_AstNodeGetPosition(name), "constraint names no generic parameter:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *qualified = MAREA_SemaChildAt(constraint, contractIndex);

            if ((MAREA_G_IsPlainName(qualified) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsContractKnown(context, qualified) == MAREA_BOOLEAN_TRUE)) {
                continue;
            }

            const MAREA_Character8 *joined = MAREA_NULLPTR;

            MAREA_IntegerSize joinedLength = 0;

            MAREA_SemaExcept result = MAREA_SemaQualifiedNameJoin(context, qualified, &joined, &joinedLength);

            if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
                result = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME, MAREA_AstNodeGetPosition(qualified), "unknown contract:", joined, joinedLength);
            }

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static void MAREA_CALL MAREA_G_Append(MAREA_Character8 *buffer, MAREA_IntegerSize *position, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if ((*position + length) >= MAREA_G_RENDER_LIMIT) {
        return;
    }

    memcpy((buffer + *position), text, length);
    *position += length;
}

// INFO: Self and the implemented type name render the same, and its type arguments are left out.
static void MAREA_CALL MAREA_G_Render(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_Character8 *selfName, MAREA_IntegerSize selfLength, MAREA_Uinteger32 depth, MAREA_Character8 *buffer, MAREA_IntegerSize *position)
{
    if (type == MAREA_NULLPTR) {
        return;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(type);

    if (KIND == MAREA_AST_KIND_INTEGER_LITERAL) {
        MAREA_G_Append(buffer, position, MAREA_AstNodeGetText(type), MAREA_AstNodeGetTextLength(type));

        return;
    }

    if (KIND == MAREA_AST_KIND_FUNCTION_TYPE) {
        MAREA_G_Append(buffer, position, "function(", MAREA_INTEGER_SIZE(9));

        const MAREA_AstNode *parameters = MAREA_G_FindChild(type, MAREA_AST_KIND_PARAMETER_LIST);

        for (MAREA_IntegerSize index = 0; ((parameters != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(parameters))); ++index) {
            const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

            MAREA_G_Render(context, MAREA_SemaChildAt(parameter, 1), selfName, selfLength, depth, buffer, position);
            MAREA_G_Append(buffer, position, ",", MAREA_INTEGER_SIZE(1));
        }

        MAREA_G_Append(buffer, position, "):", MAREA_INTEGER_SIZE(2));
        MAREA_G_Render(context, MAREA_SemaReturnType(type), selfName, selfLength, depth, buffer, position);

        return;
    }

    if (KIND != MAREA_AST_KIND_NAMED_TYPE) {
        return;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize nameLength = 0;

    if ((MAREA_G_SingleName(type, &name, &nameLength) == MAREA_BOOLEAN_TRUE) && ((MAREA_G_TextIs(name, nameLength, "Self") == MAREA_BOOLEAN_TRUE) || (MAREA_SemaTextEquals(name, nameLength, selfName, selfLength) == MAREA_BOOLEAN_TRUE))) {
        MAREA_G_Append(buffer, position, selfName, selfLength);

        return;
    }

    // INFO: an alias renders as its original, so a type written with an alias equals the type written with the original.
    if ((MAREA_G_SingleName(type, &name, &nameLength) == MAREA_BOOLEAN_TRUE) && (MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS) == MAREA_NULLPTR) && (depth < MAREA_G_ALIAS_LIMIT) && (MAREA_SemaIsGenericInScope(context, name, nameLength) == MAREA_BOOLEAN_FALSE)) {
        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, name, nameLength);

        // INFO: an import renders with its namespace, so same-named members of two namespaces are two types.
        if ((entry != MAREA_NULLPTR) && (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) && ((entry->original != MAREA_NULLPTR) || (entry->qualifier != MAREA_NULLPTR))) {
            if (entry->qualifier != MAREA_NULLPTR) {
                MAREA_G_Append(buffer, position, entry->qualifier, entry->qualifierLength);
                MAREA_G_Append(buffer, position, ".", MAREA_INTEGER_SIZE(1));
            }

            MAREA_G_Append(buffer, position, (entry->original != MAREA_NULLPTR) ? entry->original : name, (entry->original != MAREA_NULLPTR) ? entry->originalLength : nameLength);

            return;
        }

        if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) && (entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_TYPE_DECLARATION) && (MAREA_G_FindChild(entry->declaration, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_NULLPTR)) {
            const MAREA_AstNode *target = MAREA_SemaChildAt(entry->declaration, (MAREA_AstNodeGetChildCount(entry->declaration) - MAREA_INTEGER_SIZE(1)));

            if (MAREA_G_IsType(target) == MAREA_BOOLEAN_TRUE) {
                MAREA_G_Render(context, target, selfName, selfLength, (depth + MAREA_UINTEGER_32(1)), buffer, position);

                return;
            }
        }
    }

    const MAREA_AstNode *qualified = MAREA_SemaChildAt(type, 0);

    for (MAREA_IntegerSize index = 0; ((qualified != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(qualified))); ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(qualified, index);

        if (index > 0) {
            MAREA_G_Append(buffer, position, ".", MAREA_INTEGER_SIZE(1));
        }

        MAREA_G_Append(buffer, position, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
    }

    const MAREA_AstNode *arguments = MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if (arguments == MAREA_NULLPTR) {
        return;
    }

    MAREA_G_Append(buffer, position, "<", MAREA_INTEGER_SIZE(1));

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(arguments); ++index) {
        MAREA_G_Render(context, MAREA_SemaChildAt(arguments, index), selfName, selfLength, depth, buffer, position);
        MAREA_G_Append(buffer, position, ",", MAREA_INTEGER_SIZE(1));
    }

    MAREA_G_Append(buffer, position, ">", MAREA_INTEGER_SIZE(1));
}

static MAREA_Boolean MAREA_CALL MAREA_G_SameType(const MAREA_SemaContext *context, const MAREA_AstNode *left, const MAREA_AstNode *right, const MAREA_Character8 *selfName, MAREA_IntegerSize selfLength)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR)) {
        return (left == right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    MAREA_Character8 leftText[MAREA_G_RENDER_LIMIT];
    MAREA_Character8 rightText[MAREA_G_RENDER_LIMIT];

    MAREA_IntegerSize leftLength = 0;
    MAREA_IntegerSize rightLength = 0;

    MAREA_G_Render(context, left, selfName, selfLength, 0, leftText, &leftLength);
    MAREA_G_Render(context, right, selfName, selfLength, 0, rightText, &rightLength);

    return MAREA_SemaTextEquals(leftText, leftLength, rightText, rightLength);
}

static MAREA_Boolean MAREA_CALL MAREA_G_SignatureMatches(const MAREA_SemaContext *context, const MAREA_AstNode *contractFunction, const MAREA_AstNode *function, const MAREA_Character8 *selfName, MAREA_IntegerSize selfLength)
{
    const MAREA_AstNode *contractParameters = MAREA_G_FindChild(contractFunction, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_AstNode *parameters = MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST);

    if ((contractParameters == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(contractParameters) != MAREA_AstNodeGetChildCount(parameters))) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameters); ++index) {
        const MAREA_AstNode *contractParameter = MAREA_SemaChildAt(contractParameters, index);

        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        if ((contractParameter == MAREA_NULLPTR) || (parameter == MAREA_NULLPTR) || (MAREA_SemaGetParameterSemantics(context, contractParameter) != MAREA_SemaGetParameterSemantics(context, parameter))) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (MAREA_G_SameType(context, MAREA_SemaChildAt(contractParameter, 1), MAREA_SemaChildAt(parameter, 1), selfName, selfLength) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_G_SameType(context, MAREA_SemaReturnType(contractFunction), MAREA_SemaReturnType(function), selfName, selfLength);
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaResolveFunctionType(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_AstNode *current = type;

    for (MAREA_Uinteger32 depth = 0; depth < MAREA_G_ALIAS_LIMIT; ++depth) {
        if (current == MAREA_NULLPTR) {
            return MAREA_NULLPTR;
        }

        if (MAREA_AstNodeGetKind(current) == MAREA_AST_KIND_FUNCTION_TYPE) {
            return current;
        }

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        if ((MAREA_G_SingleName(current, &name, &nameLength) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaIsGenericInScope(context, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_NULLPTR;
        }

        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, name, nameLength);

        if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_TYPE_DECLARATION) || (MAREA_G_FindChild(entry->declaration, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
            return MAREA_NULLPTR;
        }

        current = MAREA_SemaChildAt(entry->declaration, (MAREA_AstNodeGetChildCount(entry->declaration) - MAREA_INTEGER_SIZE(1)));
    }

    return MAREA_NULLPTR;
}

// INFO: a type mentions a generic parameter in scope when any single name inside it is one.
static MAREA_Boolean MAREA_CALL MAREA_G_MentionsGeneric(const MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_SemaIsGenericInScope(context, MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        // INFO: a parameter name is not a type, so only the type of a PARAMETER is read.
        if ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_PARAMETER) && (index != 1)) {
            continue;
        }

        if (MAREA_G_MentionsGeneric(context, child) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaFunctionFitsFunctionType(const MAREA_SemaContext *context, const MAREA_AstNode *function, const MAREA_AstNode *functionType)
{
    if ((MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR) || (MAREA_G_MentionsGeneric(context, functionType) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return MAREA_G_SignatureMatches(context, functionType, function, MAREA_NULLPTR, 0);
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasImplementation(const MAREA_SemaContext *context, const MAREA_AstNode *contractFunction, const MAREA_Character8 *typeName, MAREA_IntegerSize typeLength)
{
    const MAREA_AstNode *contractName = MAREA_G_FindChild(contractFunction, MAREA_AST_KIND_IDENTIFIER);

    if ((contractName == MAREA_NULLPTR) || (context->root == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(context->root); ++index) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, index);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
            continue;
        }

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        if ((MAREA_G_SingleName(MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE), &name, &nameLength) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaTextEquals(name, nameLength, typeName, typeLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize memberIndex = 0; memberIndex < MAREA_AstNodeGetChildCount(implementation); ++memberIndex) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, memberIndex);

            if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
                continue;
            }

            const MAREA_AstNode *memberName = MAREA_G_FindChild(member, MAREA_AST_KIND_IDENTIFIER);

            if ((memberName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(memberName), MAREA_AstNodeGetTextLength(memberName), MAREA_AstNodeGetText(contractName), MAREA_AstNodeGetTextLength(contractName)) == MAREA_BOOLEAN_TRUE) && (MAREA_G_SignatureMatches(context, contractFunction, member, typeName, typeLength) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaTypeClass MAREA_CALL MAREA_G_Classify(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Uinteger32 depth, const MAREA_AstNode **outDeclaration, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    *outDeclaration = MAREA_NULLPTR;

    if ((type != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(type) == MAREA_AST_KIND_FUNCTION_TYPE)) {
        return MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN;
    }

    // INFO: a library registers roles and an ownership class, never a name: a registered type is sorted
    // by its descriptor before a bare single name is even looked for.
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if (MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_TRUE) {
        *outName = descriptor->name;
        *outLength = descriptor->nameLength;

        return (descriptor->ownershipClass == MAREA_TYPE_OWNERSHIP_CLASS_MOVE) ? MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN : MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN;
    }

    if (MAREA_G_SingleName(type, outName, outLength) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPE_CLASS_EXTERNAL;
    }

    if (MAREA_SemaIsGenericInScope(context, *outName, *outLength) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_TYPE_CLASS_GENERIC;
    }

    if (MAREA_SemaIsPrimitiveTypeName(*outName, *outLength) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_TYPE_CLASS_PRIMITIVE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, *outName, *outLength);

    // INFO: an import binds a declaration only for an enum of a file the resolver gave, so the enum has its real variants here.
    if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_IMPORTED_SYMBOL) && (entry->declaration != MAREA_NULLPTR)) {
        *outDeclaration = entry->declaration;

        return MAREA_SEMA_TYPE_CLASS_USER;
    }

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPE_CLASS_EXTERNAL;
    }

    if ((MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_TYPE_DECLARATION) && (depth < MAREA_G_ALIAS_LIMIT)) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(entry->declaration, (MAREA_AstNodeGetChildCount(entry->declaration) - 1));

        if (MAREA_G_IsType(target) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_Classify(context, target, (depth + 1), outDeclaration, outName, outLength);
        }
    }

    *outDeclaration = entry->declaration;

    return MAREA_SEMA_TYPE_CLASS_USER;
}

MAREA_SemaTypeClass MAREA_CALL MAREA_SemaClassifyType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode **outDeclaration, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    return MAREA_G_Classify(context, type, 0, outDeclaration, outName, outLength);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsSatisfied(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *contract)
{
    const MAREA_AstNode *contractSegment = MAREA_SemaChildAt(contract, 0);

    if ((MAREA_AstNodeGetChildCount(contract) != 1) || (contractSegment == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_Character8 *contractName = MAREA_AstNodeGetText(contractSegment);

    const MAREA_IntegerSize CONTRACT_LENGTH = MAREA_AstNodeGetTextLength(contractSegment);

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize nameLength = 0;

    const MAREA_SemaTypeClass CLASS = MAREA_G_Classify(context, type, 0, &declaration, &name, &nameLength);

    if (CLASS == MAREA_SEMA_TYPE_CLASS_EXTERNAL) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (CLASS == MAREA_SEMA_TYPE_CLASS_GENERIC) {
        return MAREA_G_ConstraintListed(context->function, name, nameLength, contractName, CONTRACT_LENGTH);
    }

    if (MAREA_G_TextIs(contractName, CONTRACT_LENGTH, "Copyable") == MAREA_BOOLEAN_TRUE) {
        if (CLASS == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
            return MAREA_BOOLEAN_TRUE;
        }

        // INFO: an array is copied element by element, so it is Copyable only when its element is. A pointer and a slice copy the handle.
        if (CLASS == MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN) {
            if (MAREA_G_TextIs(name, nameLength, "array") == MAREA_BOOLEAN_FALSE) {
                return MAREA_BOOLEAN_TRUE;
            }

            const MAREA_AstNode *arguments = MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS);

            const MAREA_AstNode *element = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

            return (element == MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_G_IsSatisfied(context, element, contract);
        }

        // INFO: an enum is always copied, the one documented copy-contract exception; the retired "value" key exempts no structure now.
        return ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_TextIs(contractName, CONTRACT_LENGTH, "Movable") == MAREA_BOOLEAN_TRUE) {
        if (CLASS == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
            return MAREA_BOOLEAN_TRUE;
        }

        if (CLASS != MAREA_SEMA_TYPE_CLASS_USER) {
            return MAREA_BOOLEAN_FALSE;
        }

        // INFO: the retired "value" key is gone, so every user type is Movable now that none can claim a copy contract instead.
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, contractName, CONTRACT_LENGTH);

    if ((entry == MAREA_NULLPTR) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_CONTRACT_DECLARATION)) {
        return MAREA_BOOLEAN_TRUE;
    }

    // INFO: a built-in type may get its functions from a library that sema cannot see, so only a user type is searched.
    if (CLASS != MAREA_SEMA_TYPE_CLASS_USER) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(entry->declaration); ++index) {
        const MAREA_AstNode *contractFunction = MAREA_SemaChildAt(entry->declaration, index);

        if ((contractFunction != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(contractFunction) == MAREA_AST_KIND_CONTRACT_FUNCTION) && (MAREA_G_HasImplementation(context, contractFunction, name, nameLength) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaInferType(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    if (expression == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(expression);

    if (KIND == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) {
        return MAREA_SemaInferType(context, MAREA_SemaChildAt(expression, 0));
    }

    if (KIND == MAREA_AST_KIND_LITERAL_VALUE) {
        const MAREA_AstNode *type = MAREA_SemaChildAt(expression, 0);

        return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? type : MAREA_NULLPTR;
    }

    if (KIND == MAREA_AST_KIND_MEMBER_ACCESS) {
        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        const MAREA_AstNode *base = MAREA_SemaInferType(context, MAREA_SemaChildAt(expression, 0));

        const MAREA_AstNode *member = MAREA_SemaChildAt(expression, 1);

        if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR) || (MAREA_SemaClassifyType(context, base, &declaration, &name, &nameLength) != MAREA_SEMA_TYPE_CLASS_USER) || (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION)) {
            return MAREA_NULLPTR;
        }

        for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(declaration); ++index) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(declaration, index);

            const MAREA_AstNode *fieldName = ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_FIELD_DECLARATION)) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((fieldName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
                const MAREA_AstNode *type = MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1));

                return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? type : MAREA_NULLPTR;
            }
        }

        return MAREA_NULLPTR;
    }

    if (KIND == MAREA_AST_KIND_INDEX_ACCESS) {
        const MAREA_AstNode *base = MAREA_SemaInferType(context, MAREA_SemaChildAt(expression, 0));

        const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        // INFO: a registered type with the index protocol (an array, a vector or a view) has an element at
        // index 0; a raw pointer is index-worthy too, exactly as the old bare "pointer" was.
        const MAREA_Boolean HAS_ELEMENT = (MAREA_SemaDescriptorOfType(context, base, &descriptor) == MAREA_BOOLEAN_TRUE) ? ((descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE) || ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) != MAREA_TYPE_ROLE_NONE)) : ((MAREA_G_SingleName(base, &name, &nameLength) == MAREA_BOOLEAN_TRUE) && ((MAREA_G_TextIs(name, nameLength, "array") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(name, nameLength, "slice") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(name, nameLength, "vector") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(name, nameLength, "pointer") == MAREA_BOOLEAN_TRUE)));

        if (HAS_ELEMENT == MAREA_BOOLEAN_FALSE) {
            return MAREA_NULLPTR;
        }

        const MAREA_AstNode *arguments = MAREA_G_FindChild(base, MAREA_AST_KIND_TYPE_ARGUMENTS);

        const MAREA_AstNode *element = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

        return (MAREA_G_IsType(element) == MAREA_BOOLEAN_TRUE) ? element : MAREA_NULLPTR;
    }

    if (KIND != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_NULLPTR;
    }

    const MAREA_SemaName *local = MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression));

    return (local == MAREA_NULLPTR) ? MAREA_NULLPTR : local->declaration;
}

#define MAREA_G_ARGUMENT_DEPTH_LIMIT MAREA_UINTEGER_32(8)

static MAREA_Boolean MAREA_CALL MAREA_G_ArgumentsDiffer(const MAREA_SemaContext *context, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_Uinteger32 depth);

// INFO: both lists must have the same length and a pair that is skipped agrees, so only a proven difference is reported.
static MAREA_Boolean MAREA_CALL MAREA_G_ArgumentListDiffers(const MAREA_SemaContext *context, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_Uinteger32 depth)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(left) != MAREA_AstNodeGetChildCount(right))) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(left); ++index) {
        if (MAREA_G_ArgumentsDiffer(context, MAREA_SemaChildAt(left, index), MAREA_SemaChildAt(right, index), depth) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: two value arguments differ by text, two primitives by name and two generic types of one head by their own arguments. A generic parameter in scope, an alias, an import, a different head and an unknown type are skipped.
static MAREA_Boolean MAREA_CALL MAREA_G_ArgumentsDiffer(const MAREA_SemaContext *context, const MAREA_AstNode *left, const MAREA_AstNode *right, MAREA_Uinteger32 depth)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR) || (depth >= MAREA_G_ARGUMENT_DEPTH_LIMIT)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_AstNodeGetKind(left) == MAREA_AST_KIND_INTEGER_LITERAL) && (MAREA_AstNodeGetKind(right) == MAREA_AST_KIND_INTEGER_LITERAL)) {
        return (MAREA_SemaTextEquals(MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(right)) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
    }

    // INFO: two registered types compare by their descriptor identity, never by a bare name: the same
    // descriptor's own type arguments differ the same way a user structure's do; a different descriptor,
    // or only one side registered, is never proven to differ.
    const MAREA_TypeDescriptor *leftDescriptor = MAREA_NULLPTR;

    const MAREA_TypeDescriptor *rightDescriptor = MAREA_NULLPTR;

    const MAREA_Boolean LEFT_REGISTERED = MAREA_SemaDescriptorOfType(context, left, &leftDescriptor);

    const MAREA_Boolean RIGHT_REGISTERED = MAREA_SemaDescriptorOfType(context, right, &rightDescriptor);

    if ((LEFT_REGISTERED == MAREA_BOOLEAN_TRUE) || (RIGHT_REGISTERED == MAREA_BOOLEAN_TRUE)) {
        if ((LEFT_REGISTERED == MAREA_BOOLEAN_FALSE) || (RIGHT_REGISTERED == MAREA_BOOLEAN_FALSE) || (leftDescriptor != rightDescriptor)) {
            return MAREA_BOOLEAN_FALSE;
        }

        return MAREA_G_ArgumentListDiffers(context, MAREA_G_FindChild(left, MAREA_AST_KIND_TYPE_ARGUMENTS), MAREA_G_FindChild(right, MAREA_AST_KIND_TYPE_ARGUMENTS), (depth + 1));
    }

    const MAREA_Character8 *leftName = MAREA_NULLPTR;
    const MAREA_Character8 *rightName = MAREA_NULLPTR;

    MAREA_IntegerSize leftLength = 0;
    MAREA_IntegerSize rightLength = 0;

    if ((MAREA_G_SingleName(left, &leftName, &leftLength) == MAREA_BOOLEAN_FALSE) || (MAREA_G_SingleName(right, &rightName, &rightLength) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaIsGenericInScope(context, leftName, leftLength) == MAREA_BOOLEAN_TRUE) || (MAREA_SemaIsGenericInScope(context, rightName, rightLength) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_SemaIsPrimitiveTypeName(leftName, leftLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaIsPrimitiveTypeName(rightName, rightLength) == MAREA_BOOLEAN_TRUE)) {
        return (MAREA_SemaTextEquals(leftName, leftLength, rightName, rightLength) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
    }

    const MAREA_AstNode *leftArguments = MAREA_G_FindChild(left, MAREA_AST_KIND_TYPE_ARGUMENTS);
    const MAREA_AstNode *rightArguments = MAREA_G_FindChild(right, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if ((leftArguments == MAREA_NULLPTR) || (rightArguments == MAREA_NULLPTR) || (MAREA_SemaTextEquals(leftName, leftLength, rightName, rightLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: an import or an alias of the same name may stand for another type, so only a name that is a declaration or a built-in head is compared.
    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, leftName, leftLength);

    if ((entry != MAREA_NULLPTR) && ((entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION))) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_G_ArgumentListDiffers(context, leftArguments, rightArguments, (depth + 1));
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypeArgumentsDiffer(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if ((context == MAREA_NULLPTR) || (type == MAREA_NULLPTR) || (expression == MAREA_NULLPTR) || (MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS) == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *value = expression;

    // INFO: a copy and a move give the type of their operand, and a parenthesized expression the type of its inside.
    while ((MAREA_AstNodeGetKind(value) == MAREA_AST_KIND_UNARY_EXPRESSION) && ((MAREA_AstNodeGetOperator(value) == MAREA_TOKEN_KIND_KEYWORD_COPY) || (MAREA_AstNodeGetOperator(value) == MAREA_TOKEN_KIND_KEYWORD_MOVE))) {
        value = MAREA_SemaChildAt(value, 0);

        if (value == MAREA_NULLPTR) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_G_ArgumentsDiffer(context, type, MAREA_SemaInferType(context, value), 0);
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaElementType(const MAREA_SemaContext *context, const MAREA_AstNode *collection)
{
    const MAREA_AstNode *type = MAREA_SemaInferType(context, collection);

    // INFO: one owner-role wrapper is peeled, then the first type argument of a registered type with the
    // index protocol (or the old bare vector, array or slice) is the element.
    for (MAREA_Uinteger32 step = 0; step < 2; ++step) {
        const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

        const MAREA_Boolean REGISTERED = MAREA_SemaDescriptorOfType(context, type, &descriptor);

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        const MAREA_Boolean BARE = (REGISTERED == MAREA_BOOLEAN_FALSE) ? MAREA_G_SingleName(type, &name, &length) : MAREA_BOOLEAN_FALSE;

        const MAREA_AstNode *arguments = ((REGISTERED == MAREA_BOOLEAN_TRUE) || (BARE == MAREA_BOOLEAN_TRUE)) ? MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS) : MAREA_NULLPTR;

        const MAREA_AstNode *first = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

        if ((first == MAREA_NULLPTR) || (MAREA_G_IsType(first) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_NULLPTR;
        }

        const MAREA_Boolean IS_OWNER = (REGISTERED == MAREA_BOOLEAN_TRUE) ? ((descriptor->roles & MAREA_TYPE_ROLE_OWNER) != MAREA_TYPE_ROLE_NONE) : ((BARE == MAREA_BOOLEAN_TRUE) && (MAREA_G_TextIs(name, length, "owned") == MAREA_BOOLEAN_TRUE));

        if ((step == 0) && (IS_OWNER == MAREA_BOOLEAN_TRUE)) {
            type = first;

            continue;
        }

        const MAREA_Boolean HAS_ELEMENT = (REGISTERED == MAREA_BOOLEAN_TRUE) ? (descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE) : ((BARE == MAREA_BOOLEAN_TRUE) && ((MAREA_G_TextIs(name, length, "vector") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(name, length, "array") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(name, length, "slice") == MAREA_BOOLEAN_TRUE)));

        return (HAS_ELEMENT == MAREA_BOOLEAN_TRUE) ? first : MAREA_NULLPTR;
    }

    return MAREA_NULLPTR;
}

static const MAREA_G_Binding *MAREA_CALL MAREA_G_FindBinding(const MAREA_G_Bindings *bindings, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < bindings->count; ++index) {
        if (MAREA_SemaTextEquals(bindings->items[index].name, bindings->items[index].length, name, length) == MAREA_BOOLEAN_TRUE) {
            return &bindings->items[index];
        }
    }

    return MAREA_NULLPTR;
}

static void MAREA_CALL MAREA_G_Bind(MAREA_G_Bindings *bindings, const MAREA_Character8 *name, MAREA_IntegerSize length, const MAREA_AstNode *type)
{
    if ((bindings->count >= MAREA_G_BINDING_LIMIT) || (type == MAREA_NULLPTR) || (MAREA_G_FindBinding(bindings, name, length) != MAREA_NULLPTR)) {
        return;
    }

    bindings->items[bindings->count] = (MAREA_G_Binding){ .name = name, .type = type, .length = length };
    bindings->count += 1;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsGenericOf(const MAREA_AstNode *genericParameters, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; ((genericParameters != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(genericParameters))); ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(genericParameters, index);

        const MAREA_AstNode *identifier = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 0);

        if ((identifier != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier), name, length) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a parameter type that is a bare generic parameter binds to the argument type, and equal wrappers unwrap one level.
static void MAREA_CALL MAREA_G_Unify(const MAREA_AstNode *genericParameters, const MAREA_AstNode *parameterType, const MAREA_AstNode *argumentType, MAREA_G_Bindings *bindings)
{
    if ((parameterType == MAREA_NULLPTR) || (argumentType == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(parameterType) != MAREA_AST_KIND_NAMED_TYPE)) {
        return;
    }

    const MAREA_AstNode *parameterArguments = MAREA_G_FindChild(parameterType, MAREA_AST_KIND_TYPE_ARGUMENTS);

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((MAREA_G_SingleName(parameterType, &name, &length) == MAREA_BOOLEAN_TRUE) && (parameterArguments == MAREA_NULLPTR) && (MAREA_G_IsGenericOf(genericParameters, name, length) == MAREA_BOOLEAN_TRUE)) {
        MAREA_G_Bind(bindings, name, length, argumentType);

        return;
    }

    const MAREA_AstNode *argumentArguments = (MAREA_AstNodeGetKind(argumentType) == MAREA_AST_KIND_NAMED_TYPE) ? MAREA_G_FindChild(argumentType, MAREA_AST_KIND_TYPE_ARGUMENTS) : MAREA_NULLPTR;

    if ((parameterArguments == MAREA_NULLPTR) || (argumentArguments == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(parameterArguments) != MAREA_AstNodeGetChildCount(argumentArguments))) {
        return;
    }

    MAREA_Character8 parameterText[MAREA_G_RENDER_LIMIT];
    MAREA_Character8 argumentText[MAREA_G_RENDER_LIMIT];

    const MAREA_AstNode *parameterName = MAREA_SemaChildAt(parameterType, 0);

    const MAREA_AstNode *argumentName = MAREA_SemaChildAt(argumentType, 0);

    MAREA_IntegerSize parameterPosition = 0;
    MAREA_IntegerSize argumentPosition = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameterName); ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(parameterName, index);

        MAREA_G_Append(parameterText, &parameterPosition, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(argumentName); ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(argumentName, index);

        MAREA_G_Append(argumentText, &argumentPosition, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
    }

    if (MAREA_SemaTextEquals(parameterText, parameterPosition, argumentText, argumentPosition) == MAREA_BOOLEAN_FALSE) {
        return;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameterArguments); ++index) {
        MAREA_G_Unify(genericParameters, MAREA_SemaChildAt(parameterArguments, index), MAREA_SemaChildAt(argumentArguments, index), bindings);
    }
}

// INFO: shared by a plain function call and a Type(args) construction call: the required count ends at the last
// parameter with no default, so a call may omit only the defaulted tail, then each argument is checked against
// the borrow rule and, when the parameter type is a primitive, against its literal fit.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckArguments(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments, const MAREA_Character8 *calleeName, MAREA_IntegerSize calleeLength)
{
    const MAREA_IntegerSize PARAMETER_COUNT = (parameters == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(parameters);

    const MAREA_IntegerSize ARGUMENT_COUNT = (arguments == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(arguments);

    MAREA_IntegerSize requiredCount = 0;

    for (MAREA_IntegerSize index = 0; index < PARAMETER_COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        if ((parameter == MAREA_NULLPTR) || (MAREA_SemaParameterHasDefault(parameter) == MAREA_BOOLEAN_FALSE)) {
            requiredCount = (index + 1);
        }
    }

    if ((ARGUMENT_COUNT < requiredCount) || (ARGUMENT_COUNT > PARAMETER_COUNT)) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH, MAREA_AstNodeGetPosition(node), "wrong number of arguments in the call of", calleeName, calleeLength);
    }

    if ((parameters != MAREA_NULLPTR) && (arguments != MAREA_NULLPTR)) {
        const MAREA_SemaExcept BORROW_RESULT = MAREA_SemaCheckBorrowArguments(context, node, parameters, arguments);

        if (BORROW_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return BORROW_RESULT;
        }

        const MAREA_SemaExcept ALIAS_RESULT = MAREA_SemaCheckAliasedArguments(context, node, parameters, arguments);

        if (ALIAS_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return ALIAS_RESULT;
        }

        const MAREA_SemaExcept WRITE_RESULT = MAREA_SemaCheckWriteArguments(context, parameters, arguments);

        if (WRITE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return WRITE_RESULT;
        }

        const MAREA_SemaExcept OWNED_RESULT = MAREA_SemaCheckOwnedArguments(context, parameters, arguments);

        if (OWNED_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return OWNED_RESULT;
        }
    }

    for (MAREA_IntegerSize index = 0; index < ARGUMENT_COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_SemaExcept LITERAL_RESULT = MAREA_SemaCheckExpressionFit(context, (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 1), MAREA_SemaChildAt(arguments, index));

        if (LITERAL_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return LITERAL_RESULT;
        }

        const MAREA_SemaExcept RANGE_RESULT = MAREA_SemaCheckLiteralRange(context, (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 1), MAREA_SemaChildAt(arguments, index));

        if (RANGE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RANGE_RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a call whose callee names a structure or an enum resolves to its construct operator: the argument list is
// checked against construct's own parameters (defaults allowed), and a type with no construct operator accepts
// only a call with no argument. This never raises a new diagnostic: a wrong count or type reuses
// ARGUMENT_COUNT_MISMATCH (3010) and TYPE_MISMATCH (3011), the same codes an ordinary call reuses.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckConstruction(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *typeDeclaration, const MAREA_Character8 *calleeName, MAREA_IntegerSize calleeLength)
{
    const MAREA_AstNode *arguments = MAREA_G_FindChild(node, MAREA_AST_KIND_ARGUMENT_LIST);

    if (arguments == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *construct = MAREA_SemaFindConstructOperator(context, typeDeclaration);

    const MAREA_AstNode *parameters = (construct == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(construct, MAREA_AST_KIND_PARAMETER_LIST);

    return MAREA_G_CheckArguments(context, node, parameters, arguments, calleeName, calleeLength);
}

// INFO: a call checked against the signature the host gave for one member of an EXTERNAL namespace: the
// argument count must match exactly (no default, the host describes none) and each argument must fit the
// parameter's primitive spelling, the same two diagnostics an ordinary call of a file function reuses.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckExternalCall(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_SemaSignature *signature, const MAREA_Character8 *calleeName, MAREA_IntegerSize calleeLength)
{
    const MAREA_AstNode *arguments = MAREA_G_FindChild(node, MAREA_AST_KIND_ARGUMENT_LIST);

    const MAREA_IntegerSize ARGUMENT_COUNT = (arguments == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(arguments);

    if (ARGUMENT_COUNT != signature->parameterCount) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH, MAREA_AstNodeGetPosition(node), "wrong number of arguments in the call of", calleeName, calleeLength);
    }

    for (MAREA_IntegerSize index = 0; index < ARGUMENT_COUNT; ++index) {
        const MAREA_SemaSignatureParameter *parameter = &signature->parameters[index];

        const MAREA_SemaExcept RESULT = MAREA_SemaCheckExpressionFitsPrimitive(context, parameter->typeText, parameter->typeTextLength, MAREA_SemaChildAt(arguments, index));

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }

        const MAREA_SemaExcept RANGE_RESULT = MAREA_SemaCheckLiteralRangePrimitive(context, parameter->typeText, parameter->typeTextLength, MAREA_SemaChildAt(arguments, index));

        if (RANGE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RANGE_RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a local or parameter whose declared type is a primitive, a structure or an enum is not a function, so a call
// through it is a type mismatch. A function type, a generic parameter, an external type and a name with no
// declared type (a loop name) are not decided here.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckCalleeIsFunction(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_SemaName *local, const MAREA_Character8 *calleeName, MAREA_IntegerSize calleeLength)
{
    if ((local == MAREA_NULLPTR) || (local->declaration == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, local->declaration, &declaration, &name, &length);

    const MAREA_Boolean IS_VALUE_TYPE = ((CLASS == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) || ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && (declaration != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION)))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    return (IS_VALUE_TYPE == MAREA_BOOLEAN_FALSE) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(node), "a value that is not a function is called:", calleeName, calleeLength);
}

// INFO: a call through a parameter or a local whose declared type is a function type is checked against the
// parameters of that function type: the same count and argument rules an ordinary call uses, so it reuses
// ARGUMENT_COUNT_MISMATCH (3010) and TYPE_MISMATCH (3011). Any other local is not a call of a function value.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFunctionValueCall(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_Character8 *calleeName, MAREA_IntegerSize calleeLength)
{
    const MAREA_SemaName *local = MAREA_SemaLocalFind(context, calleeName, calleeLength);

    const MAREA_AstNode *functionType = (local == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaResolveFunctionType(context, local->declaration);

    if (functionType == MAREA_NULLPTR) {
        return MAREA_G_CheckCalleeIsFunction(context, node, local, calleeName, calleeLength);
    }

    return MAREA_G_CheckArguments(context, node, MAREA_G_FindChild(functionType, MAREA_AST_KIND_PARAMETER_LIST), MAREA_G_FindChild(node, MAREA_AST_KIND_ARGUMENT_LIST), calleeName, calleeLength);
}

// INFO: the name of a field, a variant or a function: the first IDENTIFIER child.
static MAREA_Boolean MAREA_CALL MAREA_G_MemberNameIs(const MAREA_AstNode *member, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_AstNode *identifier = MAREA_G_FindChild(member, MAREA_AST_KIND_IDENTIFIER);

    return ((identifier != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier), name, nameLength) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: tells whether a structure of this file has the field, an enum of this file has the variant, or an implementation block of the type has the function.
static MAREA_Boolean MAREA_CALL MAREA_G_TypeHasMember(const MAREA_SemaContext *context, const MAREA_AstNode *declaration, const MAREA_Character8 *typeName, MAREA_IntegerSize typeLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_AstKind MEMBER_KIND = (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION) ? MAREA_AST_KIND_VARIANT_DECLARATION : MAREA_AST_KIND_FIELD_DECLARATION;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(declaration); ++index) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(declaration, index);

        if ((member != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(member) == MEMBER_KIND) && (MAREA_G_MemberNameIs(member, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(context->root); ++index) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, index);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
            continue;
        }

        const MAREA_Character8 *implementedName = MAREA_NULLPTR;

        MAREA_IntegerSize implementedLength = 0;

        if ((MAREA_G_SingleName(MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE), &implementedName, &implementedLength) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaTextEquals(implementedName, implementedLength, typeName, typeLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize memberIndex = 0; memberIndex < MAREA_AstNodeGetChildCount(implementation); ++memberIndex) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, memberIndex);

            if ((member != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(member) == MAREA_AST_KIND_FUNCTION_DECLARATION) && (MAREA_G_MemberNameIs(member, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a call base.name(...) where base is a type of this file (a bare type name no local hides) or a local of such a type. A receiver of another namespace, a library type, a generic parameter or an unknown type is skipped: sema cannot see its functions.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckTypeFunction(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *callee)
{
    const MAREA_AstNode *base = MAREA_SemaChildAt(callee, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(callee, 1);

    if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(base) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_IDENTIFIER) || (context->root == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *baseName = MAREA_AstNodeGetText(base);

    const MAREA_IntegerSize BASE_LENGTH = MAREA_AstNodeGetTextLength(base);

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *typeName = MAREA_NULLPTR;

    MAREA_IntegerSize typeLength = 0;

    if (MAREA_SemaLocalFind(context, baseName, BASE_LENGTH) != MAREA_NULLPTR) {
        if (MAREA_SemaClassifyType(context, MAREA_SemaInferType(context, base), &declaration, &typeName, &typeLength) != MAREA_SEMA_TYPE_CLASS_USER) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    } else {
        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, baseName, BASE_LENGTH);

        if ((MAREA_SemaIsGenericInScope(context, baseName, BASE_LENGTH) == MAREA_BOOLEAN_TRUE) || (entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        declaration = entry->declaration;

        typeName = baseName;

        typeLength = BASE_LENGTH;
    }

    if (declaration == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaName *typeEntry = MAREA_SemaNameTableFind(context, typeName, typeLength);

    // INFO: only a structure or an enum declared in this file is checked, because an imported type keeps its functions in its own file.
    if ((typeEntry == MAREA_NULLPTR) || (typeEntry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (typeEntry->declaration != declaration) || ((MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION) && (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_ENUM_DECLARATION))) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_TypeHasMember(context, declaration, typeName, typeLength, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE_FUNCTION, MAREA_AstNodeGetPosition(node), "type has no function:", MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));
}

// INFO: tells whether a contract that the where clause lists for the parameter gives the function. A contract that this file does not declare is external, so sema cannot see its functions and accepts the call. Copyable and Movable give no function.
static MAREA_Boolean MAREA_CALL MAREA_G_BoundGivesFunction(const MAREA_SemaContext *context, const MAREA_Character8 *parameter, MAREA_IntegerSize parameterLength, const MAREA_Character8 *function, MAREA_IntegerSize functionLength)
{
    const MAREA_AstNode *where = (context->function == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(context->function, MAREA_AST_KIND_WHERE_CLAUSE);

    if (where == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(where); ++index) {
        const MAREA_AstNode *constraint = MAREA_SemaChildAt(where, index);

        const MAREA_AstNode *name = ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) ? MAREA_NULLPTR : MAREA_SemaChildAt(constraint, 0);

        if ((name == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), parameter, parameterLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *qualified = MAREA_SemaChildAt(constraint, contractIndex);

            const MAREA_AstNode *segment = ((qualified != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(qualified) == 1)) ? MAREA_SemaChildAt(qualified, 0) : MAREA_NULLPTR;

            if (segment == MAREA_NULLPTR) {
                return MAREA_BOOLEAN_TRUE;
            }

            const MAREA_Character8 *contractName = MAREA_AstNodeGetText(segment);

            const MAREA_IntegerSize CONTRACT_LENGTH = MAREA_AstNodeGetTextLength(segment);

            if ((MAREA_G_TextIs(contractName, CONTRACT_LENGTH, "Copyable") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(contractName, CONTRACT_LENGTH, "Movable") == MAREA_BOOLEAN_TRUE)) {
                continue;
            }

            const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, contractName, CONTRACT_LENGTH);

            if ((entry == MAREA_NULLPTR) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_CONTRACT_DECLARATION)) {
                return MAREA_BOOLEAN_TRUE;
            }

            for (MAREA_IntegerSize functionIndex = 0; functionIndex < MAREA_AstNodeGetChildCount(entry->declaration); ++functionIndex) {
                const MAREA_AstNode *contractFunction = MAREA_SemaChildAt(entry->declaration, functionIndex);

                if ((contractFunction != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(contractFunction) == MAREA_AST_KIND_CONTRACT_FUNCTION) && (MAREA_G_MemberNameIs(contractFunction, function, functionLength) == MAREA_BOOLEAN_TRUE)) {
                    return MAREA_BOOLEAN_TRUE;
                }
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a call base.name(...) where base is a local or a parameter of a generic type needs a bound that gives the function name.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckBoundCall(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *callee)
{
    const MAREA_AstNode *base = MAREA_SemaChildAt(callee, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(callee, 1);

    if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(base) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(base), MAREA_AstNodeGetTextLength(base)) == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *typeName = MAREA_NULLPTR;

    MAREA_IntegerSize typeLength = 0;

    if (MAREA_SemaClassifyType(context, MAREA_SemaInferType(context, base), &declaration, &typeName, &typeLength) != MAREA_SEMA_TYPE_CLASS_GENERIC) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_BoundGivesFunction(context, typeName, typeLength, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_BOUND_OPERATION_NOT_GIVEN, MAREA_AstNodeGetPosition(node), "no bound gives the function:", MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCall(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *callee = MAREA_SemaChildAt(node, 0);

    const MAREA_SemaSignature *externalSignature = MAREA_NULLPTR;

    if (MAREA_AstNodeGetKind(callee) == MAREA_AST_KIND_MEMBER_ACCESS) {
        const MAREA_SemaExcept TYPE_FUNCTION_RESULT = MAREA_G_CheckTypeFunction(context, node, callee);

        if (TYPE_FUNCTION_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return TYPE_FUNCTION_RESULT;
        }

        const MAREA_SemaExcept BOUND_RESULT = MAREA_G_CheckBoundCall(context, node, callee);

        if (BOUND_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return BOUND_RESULT;
        }

        const MAREA_AstNode *base = MAREA_SemaChildAt(callee, 0);

        const MAREA_AstNode *member = MAREA_SemaChildAt(callee, 1);

        const MAREA_SemaTypedAstExcept SIGNATURE_RESULT = MAREA_SemaTypedAstExternalMemberSignature(context, base, member, &externalSignature);

        if (SIGNATURE_RESULT == MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY) {
            return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
        }

        if (externalSignature != MAREA_NULLPTR) {
            return MAREA_G_CheckExternalCall(context, node, externalSignature, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));
        }
    }

    if ((callee == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(callee) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaTypedAstExcept NAME_SIGNATURE_RESULT = MAREA_SemaTypedAstExternalNameSignature(context, callee, &externalSignature);

    if (NAME_SIGNATURE_RESULT == MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (externalSignature != MAREA_NULLPTR) {
        return MAREA_G_CheckExternalCall(context, node, externalSignature, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));
    }

    const MAREA_Character8 *calleeName = MAREA_AstNodeGetText(callee);

    const MAREA_IntegerSize CALLEE_LENGTH = MAREA_AstNodeGetTextLength(callee);

    if (MAREA_SemaLocalFind(context, calleeName, CALLEE_LENGTH) != MAREA_NULLPTR) {
        return MAREA_G_CheckFunctionValueCall(context, node, calleeName, CALLEE_LENGTH);
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, calleeName, CALLEE_LENGTH);

    // INFO: a parameter, a local or a loop name of the same name hides the declaration, so the call is never a call of it.
    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_SemaLocalFind(context, calleeName, CALLEE_LENGTH) != MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstKind DECLARATION_KIND = MAREA_AstNodeGetKind(entry->declaration);

    if ((DECLARATION_KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (DECLARATION_KIND == MAREA_AST_KIND_ENUM_DECLARATION)) {
        return MAREA_G_CheckConstruction(context, node, entry->declaration, calleeName, CALLEE_LENGTH);
    }

    if (DECLARATION_KIND != MAREA_AST_KIND_FUNCTION_DECLARATION) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *function = entry->declaration;

    const MAREA_AstNode *parameters = MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_AstNode *arguments = MAREA_G_FindChild(node, MAREA_AST_KIND_ARGUMENT_LIST);

    if ((parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize ARGUMENT_COUNT = MAREA_AstNodeGetChildCount(arguments);

    const MAREA_SemaExcept ARGUMENTS_RESULT = MAREA_G_CheckArguments(context, node, parameters, arguments, calleeName, CALLEE_LENGTH);

    if (ARGUMENTS_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return ARGUMENTS_RESULT;
    }

    const MAREA_AstNode *where = MAREA_G_FindChild(function, MAREA_AST_KIND_WHERE_CLAUSE);

    if (where == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *genericParameters = MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS);

    const MAREA_AstNode *typeArguments = MAREA_G_FindChild(node, MAREA_AST_KIND_TYPE_ARGUMENTS);

    MAREA_G_Bindings bindings = { .count = 0 };

    for (MAREA_IntegerSize index = 0; ((typeArguments != MAREA_NULLPTR) && (genericParameters != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(typeArguments)) && (index < MAREA_AstNodeGetChildCount(genericParameters))); ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(genericParameters, index);

        const MAREA_AstNode *identifier = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 0);

        if ((identifier != MAREA_NULLPTR) && (MAREA_G_IsType(MAREA_SemaChildAt(typeArguments, index)) == MAREA_BOOLEAN_TRUE)) {
            MAREA_G_Bind(&bindings, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier), MAREA_SemaChildAt(typeArguments, index));
        }
    }

    for (MAREA_IntegerSize index = 0; index < ARGUMENT_COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        MAREA_G_Unify(genericParameters, (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 1), MAREA_SemaInferType(context, MAREA_SemaChildAt(arguments, index)), &bindings);
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(where); ++index) {
        const MAREA_AstNode *constraint = MAREA_SemaChildAt(where, index);

        if ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_SemaChildAt(constraint, 0);

        const MAREA_G_Binding *binding = ((name != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_IDENTIFIER)) ? MAREA_G_FindBinding(&bindings, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) : MAREA_NULLPTR;

        if (binding == MAREA_NULLPTR) {
            continue;
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *contract = MAREA_SemaChildAt(constraint, contractIndex);

            if ((MAREA_G_IsPlainName(contract) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsSatisfied(context, binding->type, contract) == MAREA_BOOLEAN_TRUE)) {
                continue;
            }

            const MAREA_Character8 *joined = MAREA_NULLPTR;

            MAREA_IntegerSize joinedLength = 0;

            MAREA_SemaExcept result = MAREA_SemaQualifiedNameJoin(context, contract, &joined, &joinedLength);

            if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
                result = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONTRACT_NOT_SATISFIED, MAREA_AstNodeGetPosition(node), "type does not meet the contract:", joined, joinedLength);
            }

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
