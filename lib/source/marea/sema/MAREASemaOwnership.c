// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsIdentifier(const MAREA_AstNode *node)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_IDENTIFIER)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a type that sema cannot see is treated as by value when the caller passes unknownIsValue, so it is never guessed.
static MAREA_Boolean MAREA_CALL MAREA_G_IsByValue(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Boolean unknownIsValue)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    switch (MAREA_SemaClassifyType(context, type, &declaration, &name, &length)) {
    case MAREA_SEMA_TYPE_CLASS_PRIMITIVE:
    case MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_SEMA_TYPE_CLASS_USER:
        // INFO: an enum is always by value, the one documented copy-contract exception; the retired "value" key exempts no other type.
        return (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_SEMA_TYPE_CLASS_EXTERNAL:
        return unknownIsValue;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_PushReference(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *binding)
{
    if (MAREA_G_IsIdentifier(name) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaTableAppend(context, &context->references, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = binding, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBorrowParameter(const MAREA_SemaContext *context, const MAREA_AstNode *parameter)
{
    if ((MAREA_AstNodeGetFlags(parameter) & (MAREA_AST_FLAG_MUTABLE | MAREA_AST_FLAG_REFERENCE)) != 0) {
        return MAREA_BOOLEAN_TRUE;
    }

    const MAREA_AstNode *type = MAREA_SemaChildAt(parameter, 1);

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_G_IsByValue(context, type, MAREA_BOOLEAN_FALSE) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaBindReferences(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    if (KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) {
        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *parameters = MAREA_SemaChildAt(node, index);

            if ((parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(parameters) != MAREA_AST_KIND_PARAMETER_LIST)) {
                continue;
            }

            for (MAREA_IntegerSize parameterIndex = 0; parameterIndex < MAREA_AstNodeGetChildCount(parameters); ++parameterIndex) {
                const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, parameterIndex);

                if ((parameter == MAREA_NULLPTR) || (MAREA_G_IsBorrowParameter(context, parameter) == MAREA_BOOLEAN_FALSE)) {
                    continue;
                }

                const MAREA_SemaExcept RESULT = MAREA_G_PushReference(context, MAREA_SemaChildAt(parameter, 0), parameter);

                if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return RESULT;
                }
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT) {
        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

            if (MAREA_G_IsIdentifier(child) == MAREA_BOOLEAN_FALSE) {
                break;
            }

            const MAREA_SemaExcept RESULT = MAREA_G_PushReference(context, child, node);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_PARTITION_STATEMENT) {
        return MAREA_G_PushReference(context, MAREA_SemaChildAt(node, 2), node);
    }

    if (KIND == MAREA_AST_KIND_WITH_STATEMENT) {
        return MAREA_G_PushReference(context, MAREA_SemaChildAt(node, 1), node);
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsReference(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = context->references.count; index > 0; --index) {
        const MAREA_SemaName *entry = &context->references.entries[(index - 1)];

        if ((entry->length == length) && (memcmp(entry->text, text, length) == 0)) {
            return (entry->declaration != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsConfinedName(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    if (MAREA_G_IsIdentifier(expression) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_SemaIsReference(context, MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression));
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportEscape(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_Character8 *prefix)
{
    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_REFERENCE_ESCAPES_SCOPE, MAREA_AstNodeGetPosition(name), prefix, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsExistingValue(const MAREA_AstNode *expression)
{
    if (expression == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(expression);

    return ((KIND == MAREA_AST_KIND_IDENTIFIER) || (KIND == MAREA_AST_KIND_MEMBER_ACCESS) || (KIND == MAREA_AST_KIND_INDEX_ACCESS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckCopyOrMove(MAREA_SemaContext *context, const MAREA_AstNode *at, const MAREA_AstNode *type, const MAREA_AstNode *value)
{
    if (MAREA_G_IsExistingValue(value) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *valueType = MAREA_SemaInferType(context, value);

    const MAREA_AstNode *known = (valueType != MAREA_NULLPTR) ? valueType : type;

    if ((known == MAREA_NULLPTR) || (MAREA_G_IsByValue(context, known, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE, MAREA_AstNodeGetPosition(at), "assignment of an existing value needs copy or move");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckParameter(MAREA_SemaContext *context, const MAREA_AstNode *parameter)
{
    if ((MAREA_AstNodeGetFlags(parameter) & MAREA_AST_FLAG_REFERENCE) == 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, MAREA_SemaChildAt(parameter, 1), &declaration, &name, &length);

    // INFO: every user type other than an enum, the documented by-value exception, is already borrowed now that "value" is retired.
    const MAREA_Boolean ALREADY_BORROWED = ((CLASS == MAREA_SEMA_TYPE_CLASS_GENERIC) || (CLASS == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) || ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_ENUM_DECLARATION))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (ALREADY_BORROWED == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *parameterName = MAREA_SemaChildAt(parameter, 0);

    if (MAREA_G_IsIdentifier(parameterName) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BORROW, MAREA_AstNodeGetPosition(parameter), "reference on a type that is already borrowed:", MAREA_AstNodeGetText(parameterName), MAREA_AstNodeGetTextLength(parameterName));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOwnership(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_PARAMETER:
        return MAREA_G_CheckParameter(context, node);
    case MAREA_AST_KIND_RETURN_STATEMENT:
    {
        const MAREA_AstNode *value = MAREA_SemaChildAt(node, 0);

        if (MAREA_G_IsConfinedName(context, value) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_ReportEscape(context, value, "a reference cannot be returned:");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_FIELD_ENTRY:
    {
        const MAREA_AstNode *value = MAREA_SemaChildAt(node, 1);

        if (MAREA_G_IsConfinedName(context, value) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_ReportEscape(context, value, "a reference cannot be stored in a field:");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_ASSIGNMENT:
    {
        if (MAREA_AstNodeGetOperator(node) != MAREA_TOKEN_KIND_EQUAL) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_AstNode *target = MAREA_SemaChildAt(node, 0);

        const MAREA_AstNode *value = MAREA_SemaChildAt(node, 1);

        if ((target == MAREA_NULLPTR) || (value == MAREA_NULLPTR)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if ((MAREA_AstNodeGetKind(target) == MAREA_AST_KIND_MEMBER_ACCESS) && (MAREA_G_IsConfinedName(context, value) == MAREA_BOOLEAN_TRUE)) {
            const MAREA_SemaExcept ESCAPE_RESULT = MAREA_G_ReportEscape(context, value, "a reference cannot be stored in a field:");

            if (ESCAPE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return ESCAPE_RESULT;
            }
        }

        return MAREA_G_CheckCopyOrMove(context, node, MAREA_SemaInferType(context, target), value);
    }
    case MAREA_AST_KIND_LOCAL_DECLARATION:
    {
        const MAREA_SemaExcept LITERAL_RESULT = MAREA_SemaCheckExpressionFit(context, MAREA_SemaChildAt(node, 1), MAREA_SemaChildAt(node, 2));

        if (LITERAL_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return LITERAL_RESULT;
        }

        return MAREA_G_CheckCopyOrMove(context, node, MAREA_G_IsType(MAREA_SemaChildAt(node, 1)) == MAREA_BOOLEAN_TRUE ? MAREA_SemaChildAt(node, 1) : MAREA_NULLPTR, MAREA_SemaChildAt(node, 2));
    }
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckBorrowArguments(MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arguments);

    for (MAREA_IntegerSize first = 0; first < COUNT; ++first) {
        const MAREA_AstNode *left = MAREA_SemaChildAt(arguments, first);

        if (MAREA_G_IsIdentifier(left) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        for (MAREA_IntegerSize second = (first + 1); second < COUNT; ++second) {
            const MAREA_AstNode *right = MAREA_SemaChildAt(arguments, second);

            if ((MAREA_G_IsIdentifier(right) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetTextLength(left) != MAREA_AstNodeGetTextLength(right)) || (memcmp(MAREA_AstNodeGetText(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(left)) != 0)) {
                continue;
            }

            const MAREA_AstNode *leftParameter = MAREA_SemaChildAt(parameters, first);

            const MAREA_AstNode *rightParameter = MAREA_SemaChildAt(parameters, second);

            const MAREA_Uinteger32 WRITE = (((leftParameter == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetFlags(leftParameter)) | ((rightParameter == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetFlags(rightParameter))) & MAREA_AST_FLAG_MUTABLE;

            if (WRITE == 0) {
                continue;
            }

            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT, MAREA_AstNodeGetPosition(call), "a write borrow is passed together with another use of", MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left));
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
