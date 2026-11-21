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

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChildAt(const MAREA_AstNode *node, MAREA_IntegerSize index)
{
    const MAREA_AstNode *child = MAREA_NULLPTR;

    if (MAREA_AstNodeGetChildAt(node, index, &child) != MAREA_AST_EXCEPT_SUCCESS) {
        return MAREA_NULLPTR;
    }

    return child;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_G_ChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength)
{
    if (leftLength != rightLength) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(left, right, leftLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_G_TextEquals(text, length, expected, strlen(expected));
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

    const MAREA_AstNode *name = MAREA_G_ChildAt(type, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(name) != 1)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *segment = MAREA_G_ChildAt(name, 0);

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
        const MAREA_AstNode *child = MAREA_G_ChildAt(function, index);

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

    const MAREA_AstNode *name = MAREA_G_ChildAt(node, 0);

    const MAREA_AstNode *type = MAREA_G_ChildAt(node, 1);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER) || (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a new local of the same name as a reference is a new owner, so the reference no longer holds.
    if ((MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_LOCAL_DECLARATION) && (MAREA_SemaIsReference(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
        const MAREA_SemaExcept SHADOW_RESULT = MAREA_SemaTableAppend(context, &context->references, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });

        if (SHADOW_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return SHADOW_RESULT;
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
        const MAREA_AstNode *constraint = MAREA_G_ChildAt(where, index);

        if ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_G_ChildAt(constraint, 0);

        if ((name == MAREA_NULLPTR) || (MAREA_G_TextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), parameter, parameterLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *qualified = MAREA_G_ChildAt(constraint, contractIndex);

            const MAREA_AstNode *segment = ((qualified != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(qualified) == 1)) ? MAREA_G_ChildAt(qualified, 0) : MAREA_NULLPTR;

            if ((segment != MAREA_NULLPTR) && (MAREA_G_TextEquals(MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment), contract, contractLength) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsContractKnown(const MAREA_SemaContext *context, const MAREA_AstNode *qualified)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(qualified);

    const MAREA_AstNode *first = MAREA_G_ChildAt(qualified, 0);

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
        const MAREA_AstNode *segment = MAREA_G_ChildAt(qualified, index);

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
        const MAREA_AstNode *constraint = MAREA_G_ChildAt(node, index);

        if ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_G_ChildAt(constraint, 0);

        if ((name != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaIsGenericInScope(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_FALSE)) {
            const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME, MAREA_AstNodeGetPosition(name), "constraint names no generic parameter:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *qualified = MAREA_G_ChildAt(constraint, contractIndex);

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
static void MAREA_CALL MAREA_G_Render(const MAREA_AstNode *type, const MAREA_Character8 *selfName, MAREA_IntegerSize selfLength, MAREA_Character8 *buffer, MAREA_IntegerSize *position)
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
            const MAREA_AstNode *parameter = MAREA_G_ChildAt(parameters, index);

            MAREA_G_Render(MAREA_G_ChildAt(parameter, 1), selfName, selfLength, buffer, position);
            MAREA_G_Append(buffer, position, ",", MAREA_INTEGER_SIZE(1));
        }

        MAREA_G_Append(buffer, position, "):", MAREA_INTEGER_SIZE(2));
        MAREA_G_Render(MAREA_SemaReturnType(type), selfName, selfLength, buffer, position);

        return;
    }

    if (KIND != MAREA_AST_KIND_NAMED_TYPE) {
        return;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize nameLength = 0;

    if ((MAREA_G_SingleName(type, &name, &nameLength) == MAREA_BOOLEAN_TRUE) && ((MAREA_G_TextIs(name, nameLength, "Self") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextEquals(name, nameLength, selfName, selfLength) == MAREA_BOOLEAN_TRUE))) {
        MAREA_G_Append(buffer, position, selfName, selfLength);

        return;
    }

    const MAREA_AstNode *qualified = MAREA_G_ChildAt(type, 0);

    for (MAREA_IntegerSize index = 0; ((qualified != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(qualified))); ++index) {
        const MAREA_AstNode *segment = MAREA_G_ChildAt(qualified, index);

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
        MAREA_G_Render(MAREA_G_ChildAt(arguments, index), selfName, selfLength, buffer, position);
        MAREA_G_Append(buffer, position, ",", MAREA_INTEGER_SIZE(1));
    }

    MAREA_G_Append(buffer, position, ">", MAREA_INTEGER_SIZE(1));
}

static MAREA_Boolean MAREA_CALL MAREA_G_SameType(const MAREA_AstNode *left, const MAREA_AstNode *right, const MAREA_Character8 *selfName, MAREA_IntegerSize selfLength)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR)) {
        return (left == right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    MAREA_Character8 leftText[MAREA_G_RENDER_LIMIT];
    MAREA_Character8 rightText[MAREA_G_RENDER_LIMIT];

    MAREA_IntegerSize leftLength = 0;
    MAREA_IntegerSize rightLength = 0;

    MAREA_G_Render(left, selfName, selfLength, leftText, &leftLength);
    MAREA_G_Render(right, selfName, selfLength, rightText, &rightLength);

    return MAREA_G_TextEquals(leftText, leftLength, rightText, rightLength);
}

static MAREA_Boolean MAREA_CALL MAREA_G_SignatureMatches(const MAREA_AstNode *contractFunction, const MAREA_AstNode *function, const MAREA_Character8 *selfName, MAREA_IntegerSize selfLength)
{
    const MAREA_AstNode *contractParameters = MAREA_G_FindChild(contractFunction, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_AstNode *parameters = MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST);

    if ((contractParameters == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(contractParameters) != MAREA_AstNodeGetChildCount(parameters))) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameters); ++index) {
        const MAREA_AstNode *contractParameter = MAREA_G_ChildAt(contractParameters, index);

        const MAREA_AstNode *parameter = MAREA_G_ChildAt(parameters, index);

        if ((contractParameter == MAREA_NULLPTR) || (parameter == MAREA_NULLPTR) || (MAREA_AstNodeGetFlags(contractParameter) != MAREA_AstNodeGetFlags(parameter))) {
            return MAREA_BOOLEAN_FALSE;
        }

        if (MAREA_G_SameType(MAREA_G_ChildAt(contractParameter, 1), MAREA_G_ChildAt(parameter, 1), selfName, selfLength) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_G_SameType(MAREA_SemaReturnType(contractFunction), MAREA_SemaReturnType(function), selfName, selfLength);
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasImplementation(const MAREA_SemaContext *context, const MAREA_AstNode *contractFunction, const MAREA_Character8 *typeName, MAREA_IntegerSize typeLength)
{
    const MAREA_AstNode *contractName = MAREA_G_FindChild(contractFunction, MAREA_AST_KIND_IDENTIFIER);

    if ((contractName == MAREA_NULLPTR) || (context->root == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(context->root); ++index) {
        const MAREA_AstNode *implementation = MAREA_G_ChildAt(context->root, index);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
            continue;
        }

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        if ((MAREA_G_SingleName(MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE), &name, &nameLength) == MAREA_BOOLEAN_FALSE) || (MAREA_G_TextEquals(name, nameLength, typeName, typeLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize memberIndex = 0; memberIndex < MAREA_AstNodeGetChildCount(implementation); ++memberIndex) {
            const MAREA_AstNode *member = MAREA_G_ChildAt(implementation, memberIndex);

            if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
                continue;
            }

            const MAREA_AstNode *memberName = MAREA_G_FindChild(member, MAREA_AST_KIND_IDENTIFIER);

            if ((memberName != MAREA_NULLPTR) && (MAREA_G_TextEquals(MAREA_AstNodeGetText(memberName), MAREA_AstNodeGetTextLength(memberName), MAREA_AstNodeGetText(contractName), MAREA_AstNodeGetTextLength(contractName)) == MAREA_BOOLEAN_TRUE) && (MAREA_G_SignatureMatches(contractFunction, member, typeName, typeLength) == MAREA_BOOLEAN_TRUE)) {
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

    if (MAREA_G_SingleName(type, outName, outLength) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPE_CLASS_EXTERNAL;
    }

    if (MAREA_SemaIsGenericInScope(context, *outName, *outLength) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_TYPE_CLASS_GENERIC;
    }

    if ((MAREA_G_TextIs(*outName, *outLength, "owned") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(*outName, *outLength, "task") == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN;
    }

    if ((MAREA_G_TextIs(*outName, *outLength, "vector") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(*outName, *outLength, "pointer") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(*outName, *outLength, "slice") == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN;
    }

    static const MAREA_Character8 *const PRIMITIVES[] = { "boolean", "integer8", "integer16", "integer32", "integer64", "unsigned8", "unsigned16", "unsigned32", "unsigned64", "float32", "float64", "string" };

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(PRIMITIVES); ++index) {
        if (MAREA_G_TextIs(*outName, *outLength, PRIMITIVES[index]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SEMA_TYPE_CLASS_PRIMITIVE;
        }
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, *outName, *outLength);

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPE_CLASS_EXTERNAL;
    }

    if ((MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_TYPE_DECLARATION) && (depth < MAREA_G_ALIAS_LIMIT)) {
        const MAREA_AstNode *target = MAREA_G_ChildAt(entry->declaration, (MAREA_AstNodeGetChildCount(entry->declaration) - 1));

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
    const MAREA_AstNode *contractSegment = MAREA_G_ChildAt(contract, 0);

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
        if ((CLASS == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) || (CLASS == MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN)) {
            return MAREA_BOOLEAN_TRUE;
        }

        // INFO: an enum is a plain integer, so it is always copied.
        return ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && ((MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION) || (MAREA_SemaAttributeIsOn(context->root, declaration, "value") == MAREA_BOOLEAN_TRUE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_TextIs(contractName, CONTRACT_LENGTH, "Movable") == MAREA_BOOLEAN_TRUE) {
        if (CLASS == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
            return MAREA_BOOLEAN_TRUE;
        }

        if (CLASS != MAREA_SEMA_TYPE_CLASS_USER) {
            return MAREA_BOOLEAN_FALSE;
        }

        return ((MAREA_SemaAttributeIsOn(context->root, declaration, "move_only") == MAREA_BOOLEAN_TRUE) || (MAREA_SemaAttributeIsOn(context->root, declaration, "value") == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
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
        const MAREA_AstNode *contractFunction = MAREA_G_ChildAt(entry->declaration, index);

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
        return MAREA_SemaInferType(context, MAREA_G_ChildAt(expression, 0));
    }

    if (KIND == MAREA_AST_KIND_LITERAL_VALUE) {
        const MAREA_AstNode *type = MAREA_G_ChildAt(expression, 0);

        return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? type : MAREA_NULLPTR;
    }

    if (KIND == MAREA_AST_KIND_MEMBER_ACCESS) {
        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        const MAREA_AstNode *base = MAREA_SemaInferType(context, MAREA_G_ChildAt(expression, 0));

        const MAREA_AstNode *member = MAREA_G_ChildAt(expression, 1);

        if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR) || (MAREA_SemaClassifyType(context, base, &declaration, &name, &nameLength) != MAREA_SEMA_TYPE_CLASS_USER) || (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION)) {
            return MAREA_NULLPTR;
        }

        for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(declaration); ++index) {
            const MAREA_AstNode *field = MAREA_G_ChildAt(declaration, index);

            const MAREA_AstNode *fieldName = ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_FIELD_DECLARATION)) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((fieldName != MAREA_NULLPTR) && (MAREA_G_TextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
                const MAREA_AstNode *type = MAREA_G_ChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1));

                return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? type : MAREA_NULLPTR;
            }
        }

        return MAREA_NULLPTR;
    }

    if (KIND == MAREA_AST_KIND_INDEX_ACCESS) {
        const MAREA_AstNode *base = MAREA_SemaInferType(context, MAREA_G_ChildAt(expression, 0));

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        if ((MAREA_G_SingleName(base, &name, &nameLength) == MAREA_BOOLEAN_FALSE) || ((MAREA_G_TextIs(name, nameLength, "array") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(name, nameLength, "slice") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(name, nameLength, "vector") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(name, nameLength, "pointer") == MAREA_BOOLEAN_FALSE))) {
            return MAREA_NULLPTR;
        }

        const MAREA_AstNode *arguments = MAREA_G_FindChild(base, MAREA_AST_KIND_TYPE_ARGUMENTS);

        const MAREA_AstNode *element = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ChildAt(arguments, 0);

        return (MAREA_G_IsType(element) == MAREA_BOOLEAN_TRUE) ? element : MAREA_NULLPTR;
    }

    if (KIND != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        const MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_G_TextEquals(local->text, local->length, MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression)) == MAREA_BOOLEAN_TRUE) {
            return local->declaration;
        }
    }

    return MAREA_NULLPTR;
}

static const MAREA_G_Binding *MAREA_CALL MAREA_G_FindBinding(const MAREA_G_Bindings *bindings, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < bindings->count; ++index) {
        if (MAREA_G_TextEquals(bindings->items[index].name, bindings->items[index].length, name, length) == MAREA_BOOLEAN_TRUE) {
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
        const MAREA_AstNode *parameter = MAREA_G_ChildAt(genericParameters, index);

        const MAREA_AstNode *identifier = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ChildAt(parameter, 0);

        if ((identifier != MAREA_NULLPTR) && (MAREA_G_TextEquals(MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier), name, length) == MAREA_BOOLEAN_TRUE)) {
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

    const MAREA_AstNode *parameterName = MAREA_G_ChildAt(parameterType, 0);

    const MAREA_AstNode *argumentName = MAREA_G_ChildAt(argumentType, 0);

    MAREA_IntegerSize parameterPosition = 0;
    MAREA_IntegerSize argumentPosition = 0;

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameterName); ++index) {
        const MAREA_AstNode *segment = MAREA_G_ChildAt(parameterName, index);

        MAREA_G_Append(parameterText, &parameterPosition, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(argumentName); ++index) {
        const MAREA_AstNode *segment = MAREA_G_ChildAt(argumentName, index);

        MAREA_G_Append(argumentText, &argumentPosition, MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment));
    }

    if (MAREA_G_TextEquals(parameterText, parameterPosition, argumentText, argumentPosition) == MAREA_BOOLEAN_FALSE) {
        return;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameterArguments); ++index) {
        MAREA_G_Unify(genericParameters, MAREA_G_ChildAt(parameterArguments, index), MAREA_G_ChildAt(argumentArguments, index), bindings);
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckLiteralFit(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((type == MAREA_NULLPTR) || (expression == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(expression);

    if ((KIND != MAREA_AST_KIND_STRING_LITERAL) && (KIND != MAREA_AST_KIND_BOOLEAN_LITERAL) && (KIND != MAREA_AST_KIND_INTEGER_LITERAL) && (KIND != MAREA_AST_KIND_FLOAT_LITERAL)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_G_Classify(context, type, 0, &declaration, &name, &length) != MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean IS_STRING = MAREA_G_TextIs(name, length, "string");

    const MAREA_Boolean IS_BOOLEAN = MAREA_G_TextIs(name, length, "boolean");

    const MAREA_Boolean IS_FLOAT = (MAREA_G_TextIs(name, length, "float32") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(name, length, "float64") == MAREA_BOOLEAN_TRUE);

    MAREA_Boolean fits = MAREA_BOOLEAN_TRUE;

    if (KIND == MAREA_AST_KIND_STRING_LITERAL) {
        fits = IS_STRING;
    } else if (KIND == MAREA_AST_KIND_BOOLEAN_LITERAL) {
        fits = IS_BOOLEAN;
    } else if (KIND == MAREA_AST_KIND_INTEGER_LITERAL) {
        fits = ((IS_STRING == MAREA_BOOLEAN_FALSE) && (IS_BOOLEAN == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    } else {
        fits = IS_FLOAT;
    }

    if (fits == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "literal does not fit the type", name, length);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckCall(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *callee = MAREA_G_ChildAt(node, 0);

    if ((callee == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(callee) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *calleeName = MAREA_AstNodeGetText(callee);

    const MAREA_IntegerSize CALLEE_LENGTH = MAREA_AstNodeGetTextLength(callee);

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, calleeName, CALLEE_LENGTH);

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_FUNCTION_DECLARATION)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *function = entry->declaration;

    const MAREA_AstNode *parameters = MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST);

    const MAREA_AstNode *arguments = MAREA_G_FindChild(node, MAREA_AST_KIND_ARGUMENT_LIST);

    if ((parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_AstNodeGetChildCount(parameters) != MAREA_AstNodeGetChildCount(arguments)) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH, MAREA_AstNodeGetPosition(node), "wrong number of arguments in the call of", calleeName, CALLEE_LENGTH);
    }

    const MAREA_SemaExcept BORROW_RESULT = MAREA_SemaCheckBorrowArguments(context, node, parameters, arguments);

    if (BORROW_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return BORROW_RESULT;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameters); ++index) {
        const MAREA_AstNode *parameter = MAREA_G_ChildAt(parameters, index);

        const MAREA_SemaExcept LITERAL_RESULT = MAREA_SemaCheckLiteralFit(context, (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ChildAt(parameter, 1), MAREA_G_ChildAt(arguments, index));

        if (LITERAL_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return LITERAL_RESULT;
        }
    }

    const MAREA_AstNode *where = MAREA_G_FindChild(function, MAREA_AST_KIND_WHERE_CLAUSE);

    if (where == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *genericParameters = MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS);

    const MAREA_AstNode *typeArguments = MAREA_G_FindChild(node, MAREA_AST_KIND_TYPE_ARGUMENTS);

    MAREA_G_Bindings bindings = { .count = 0 };

    for (MAREA_IntegerSize index = 0; ((typeArguments != MAREA_NULLPTR) && (genericParameters != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(typeArguments)) && (index < MAREA_AstNodeGetChildCount(genericParameters))); ++index) {
        const MAREA_AstNode *parameter = MAREA_G_ChildAt(genericParameters, index);

        const MAREA_AstNode *identifier = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ChildAt(parameter, 0);

        if ((identifier != MAREA_NULLPTR) && (MAREA_G_IsType(MAREA_G_ChildAt(typeArguments, index)) == MAREA_BOOLEAN_TRUE)) {
            MAREA_G_Bind(&bindings, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier), MAREA_G_ChildAt(typeArguments, index));
        }
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(parameters); ++index) {
        const MAREA_AstNode *parameter = MAREA_G_ChildAt(parameters, index);

        MAREA_G_Unify(genericParameters, (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_ChildAt(parameter, 1), MAREA_SemaInferType(context, MAREA_G_ChildAt(arguments, index)), &bindings);
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(where); ++index) {
        const MAREA_AstNode *constraint = MAREA_G_ChildAt(where, index);

        if ((constraint == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(constraint) != MAREA_AST_KIND_CONSTRAINT)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_G_ChildAt(constraint, 0);

        const MAREA_G_Binding *binding = ((name != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_IDENTIFIER)) ? MAREA_G_FindBinding(&bindings, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) : MAREA_NULLPTR;

        if (binding == MAREA_NULLPTR) {
            continue;
        }

        for (MAREA_IntegerSize contractIndex = 1; contractIndex < MAREA_AstNodeGetChildCount(constraint); ++contractIndex) {
            const MAREA_AstNode *contract = MAREA_G_ChildAt(constraint, contractIndex);

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
