// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the core types the language itself defines. Every other type is a library or a user type.
static const MAREA_Character8 *const MAREA_G_BuiltinTypes[] = {
    "boolean",
    "sinteger8",
    "sinteger16",
    "sinteger32",
    "sinteger64",
    "uinteger8",
    "uinteger16",
    "uinteger32",
    "uinteger64",
    "float32",
    "float64",
    "character",
    "string",
    "array",
    "vector",
    "pointer",
    "owned",
    "slice",
    "task",
};

static MAREA_Boolean MAREA_CALL MAREA_G_TextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right)
{
    const MAREA_IntegerSize RIGHT_LENGTH = strlen(right);

    if (leftLength != RIGHT_LENGTH) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp(left, right, leftLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBuiltin(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_BuiltinTypes); ++index) {
        if (MAREA_G_TextEquals(text, length, MAREA_G_BuiltinTypes[index]) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsTypeDeclaration(const MAREA_AstNode *declaration)
{
    if (declaration == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    switch (MAREA_AstNodeGetKind(declaration)) {
    case MAREA_AST_KIND_STRUCTURE_DECLARATION:
    case MAREA_AST_KIND_ENUM_DECLARATION:
    case MAREA_AST_KIND_OPAQUE_DECLARATION:
    case MAREA_AST_KIND_TYPE_DECLARATION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsGenericInScope(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < context->generics.count; ++index) {
        const MAREA_SemaName *entry = &context->generics.entries[index];

        if ((entry->length == length) && (memcmp(entry->text, text, length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_PushGeneric(MAREA_SemaContext *context, const MAREA_AstNode *identifier)
{
    return MAREA_SemaTableAppend(context, &context->generics, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(identifier), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(identifier), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaScopeEnter(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_SemaScopeMark *outMark)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outMark == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    outMark->genericCount = context->generics.count;
    outMark->localCount = context->locals.count;
    outMark->referenceCount = context->references.count;
    outMark->function = context->function;
    outMark->implementation = context->implementation;
    outMark->selfDepth = context->selfDepth;
    outMark->forkDepth = context->forkDepth;
    outMark->partitionDepth = context->partitionDepth;
    outMark->loopDepth = context->loopDepth;

    const MAREA_SemaExcept BIND_RESULT = MAREA_SemaBindReferences(context, node);

    if (BIND_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return BIND_RESULT;
    }

    const MAREA_SemaExcept BINDER_RESULT = MAREA_SemaDeclareBinders(context, node);

    if (BINDER_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return BINDER_RESULT;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    if ((KIND == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) || (KIND == MAREA_AST_KIND_CONTRACT_DECLARATION)) {
        context->selfDepth += 1;
    }

    if (KIND == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) {
        context->implementation = node;
    }

    if (KIND == MAREA_AST_KIND_FORK_JOIN_BLOCK) {
        context->forkDepth += 1;
    }

    // INFO: the body of a partition is a barrier. A loop outside it is not a target of a jump inside it.
    if (KIND == MAREA_AST_KIND_PARTITION_STATEMENT) {
        context->partitionDepth += 1;
        context->loopDepth = 0;
    }

    if ((KIND == MAREA_AST_KIND_WHILE_STATEMENT) || (KIND == MAREA_AST_KIND_FOR_RANGE_STATEMENT) || (KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT)) {
        context->loopDepth += 1;
    }

    if ((KIND != MAREA_AST_KIND_STRUCTURE_DECLARATION) && (KIND != MAREA_AST_KIND_TYPE_DECLARATION) && (KIND != MAREA_AST_KIND_FUNCTION_DECLARATION) && (KIND != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) {
        context->function = node;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *parameters = MAREA_SemaChildAt(node, index);

        if ((KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) && (parameters != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(parameters) == MAREA_AST_KIND_PARAMETER_LIST)) {
            for (MAREA_IntegerSize parameterIndex = 0; parameterIndex < MAREA_AstNodeGetChildCount(parameters); ++parameterIndex) {
                const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, parameterIndex);

                const MAREA_SemaExcept DECLARE_RESULT = MAREA_SemaDeclareLocal(context, parameter);

                if (DECLARE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return DECLARE_RESULT;
                }
            }

            continue;
        }

        if ((parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(parameters) != MAREA_AST_KIND_GENERIC_PARAMETERS)) {
            continue;
        }

        for (MAREA_IntegerSize parameterIndex = 0; parameterIndex < MAREA_AstNodeGetChildCount(parameters); ++parameterIndex) {
            const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, parameterIndex);

            const MAREA_AstNode *name = (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 0);

            if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
                continue;
            }

            const MAREA_SemaExcept RESULT = MAREA_G_PushGeneric(context, name);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_SemaScopeLeave(MAREA_SemaContext *context, MAREA_SemaScopeMark mark)
{
    context->generics.count = mark.genericCount;
    context->locals.count = mark.localCount;
    context->references.count = mark.referenceCount;
    context->function = mark.function;
    context->implementation = mark.implementation;
    context->selfDepth = mark.selfDepth;
    context->forkDepth = mark.forkDepth;
    context->partitionDepth = mark.partitionDepth;
    context->loopDepth = mark.loopDepth;
}

static MAREA_IntegerSize MAREA_CALL MAREA_G_BuiltinArity(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (MAREA_G_TextEquals(text, length, "array") == MAREA_BOOLEAN_TRUE) {
        return MAREA_INTEGER_SIZE(2);
    }

    const MAREA_Boolean TAKES_ONE = (MAREA_G_TextEquals(text, length, "pointer") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextEquals(text, length, "owned") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextEquals(text, length, "slice") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextEquals(text, length, "vector") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextEquals(text, length, "task") == MAREA_BOOLEAN_TRUE);

    return (TAKES_ONE == MAREA_BOOLEAN_TRUE) ? MAREA_INTEGER_SIZE(1) : MAREA_INTEGER_SIZE(0);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckArity(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *declaration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_AstNode *arguments = MAREA_SemaChildAt(node, 1);

    const MAREA_IntegerSize ACTUAL = ((arguments != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(arguments) == MAREA_AST_KIND_TYPE_ARGUMENTS)) ? MAREA_AstNodeGetChildCount(arguments) : MAREA_INTEGER_SIZE(0);

    const MAREA_AstNode *parameters = MAREA_NULLPTR;

    MAREA_IntegerSize expected = 0;

    if (declaration == MAREA_NULLPTR) {
        expected = MAREA_G_BuiltinArity(name, nameLength);
    } else {
        const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(declaration);

        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *child = MAREA_SemaChildAt(declaration, index);

            if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_GENERIC_PARAMETERS)) {
                parameters = child;
            }
        }

        expected = (parameters == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(parameters);
    }

    MAREA_Boolean mismatch = (ACTUAL != expected) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    // INFO: a number may not fill a type parameter, and a function type may not fill a value parameter.
    for (MAREA_IntegerSize index = 0; ((mismatch == MAREA_BOOLEAN_FALSE) && (parameters != MAREA_NULLPTR) && (index < ACTUAL)); ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((parameter == MAREA_NULLPTR) || (argument == MAREA_NULLPTR)) {
            continue;
        }

        const MAREA_Boolean IS_VALUE_PARAMETER = ((MAREA_AstNodeGetFlags(parameter) & MAREA_AST_FLAG_CONSTANT) != 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        if ((IS_VALUE_PARAMETER == MAREA_BOOLEAN_FALSE) && (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_INTEGER_LITERAL)) {
            mismatch = MAREA_BOOLEAN_TRUE;
        }

        if ((IS_VALUE_PARAMETER == MAREA_BOOLEAN_TRUE) && (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_FUNCTION_TYPE)) {
            mismatch = MAREA_BOOLEAN_TRUE;
        }
    }

    if (mismatch == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, MAREA_AstNodeGetPosition(node), "type arguments do not match the generic parameters of", name, nameLength);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamedType(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *qualifiedName = MAREA_SemaChildAt(node, 0);

    if ((qualifiedName == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualifiedName) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize SEGMENT_COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    if (SEGMENT_COUNT == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < SEGMENT_COUNT; ++index) {
        const MAREA_AstNode *segment = MAREA_SemaChildAt(qualifiedName, index);

        if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    }

    const MAREA_AstNode *first = MAREA_SemaChildAt(qualifiedName, 0);

    const MAREA_Character8 *firstText = MAREA_AstNodeGetText(first);

    const MAREA_IntegerSize FIRST_LENGTH = MAREA_AstNodeGetTextLength(first);

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, firstText, FIRST_LENGTH);

    if (SEGMENT_COUNT == 1) {
        if (MAREA_SemaIsGenericInScope(context, firstText, FIRST_LENGTH) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if (MAREA_G_IsBuiltin(firstText, FIRST_LENGTH) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_CheckArity(context, node, MAREA_NULLPTR, firstText, FIRST_LENGTH);
        }

        if ((context->selfDepth > 0) && (MAREA_G_TextEquals(firstText, FIRST_LENGTH, "Self") == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) && (MAREA_G_IsTypeDeclaration(entry->declaration) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_G_CheckArity(context, node, entry->declaration, firstText, FIRST_LENGTH);
        }

        if ((entry != MAREA_NULLPTR) && (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    } else if ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_NAMESPACE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a name that no local rule knows is a library type, so the resolver has the last word.
    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaQualifiedNameJoin(context, qualifiedName, &joined, &joinedLength);

    if (JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return JOIN_RESULT;
    }

    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    const MAREA_AstNode *foundRoot = MAREA_NULLPTR;

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION)) {
        context->resolver->resolve(context->resolver->userData, joined, joinedLength, &result, &foundRoot);
    }

    if (result != MAREA_SEMA_IMPORT_RESULT_NOT_FOUND) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE, MAREA_AstNodeGetPosition(node), "unknown type:", joined, joinedLength);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOpaqueField(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *type = MAREA_SemaChildAt(node, index);

        if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
            continue;
        }

        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        if ((MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_USER) && (declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_OPAQUE_DECLARATION)) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_FIELD_BY_VALUE, MAREA_AstNodeGetPosition(type), "an opaque type cannot be held by value in a field");
        }

        break;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
