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

// INFO: a library storage type is written exactly c.array, c.vector, c.table, c.owned or c.task; c.slice and c.pointer are a view and an address, so they stay by value.
static MAREA_Boolean MAREA_CALL MAREA_G_IsLibraryStorageType(const MAREA_AstNode *type)
{
    static const MAREA_Character8 *const STORAGE[] = { "array", "vector", "table", "owned", "task" };

    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(type, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(name) != 2)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *first = MAREA_SemaChildAt(name, 0);

    const MAREA_AstNode *second = MAREA_SemaChildAt(name, 1);

    if ((MAREA_G_IsIdentifier(first) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsIdentifier(second) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_AstNodeGetTextLength(first) != 1) || (MAREA_AstNodeGetText(first)[0] != 'c')) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(second);

    const MAREA_IntegerSize length = MAREA_AstNodeGetTextLength(second);

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(STORAGE); ++index) {
        if ((strlen(STORAGE[index]) == length) && (memcmp(STORAGE[index], text, length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
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
        return ((unknownIsValue == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsLibraryStorageType(type) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_FALSE : unknownIsValue;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypeIsByValue(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Boolean unknownIsValue)
{
    return MAREA_G_IsByValue(context, type, unknownIsValue);
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_PushReference(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *binding)
{
    if (MAREA_G_IsIdentifier(name) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaTableAppend(context, &context->references, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = binding, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

MAREA_SemaParameterSemantics MAREA_CALL MAREA_SemaGetParameterSemantics(const MAREA_SemaContext *context, const MAREA_AstNode *parameter)
{
    if (MAREA_SemaAttributeHasMember(MAREA_NULLPTR, parameter, "semantics", "value_read_only") == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_PARAMETER_SEMANTICS_VALUE_READ_ONLY;
    }

    if (MAREA_SemaAttributeHasMember(MAREA_NULLPTR, parameter, "semantics", "reference_read_only") == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_READ_ONLY;
    }

    if (MAREA_SemaAttributeHasMember(MAREA_NULLPTR, parameter, "semantics", "reference_mutable") == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE;
    }

    const MAREA_AstNode *type = MAREA_SemaChildAt(parameter, 1);

    // INFO: the default by type class: a primitive or the one documented by-value exception (an enum) is VALUE_READ_ONLY, a move-in type (owned, task, vector) keeps REFERENCE_MUTABLE, anything else is a read-only borrow.
    if (MAREA_G_IsByValue(context, type, MAREA_BOOLEAN_FALSE) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_PARAMETER_SEMANTICS_VALUE_READ_ONLY;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
        return MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE;
    }

    return MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_READ_ONLY;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsBorrowParameter(const MAREA_SemaContext *context, const MAREA_AstNode *parameter)
{
    const MAREA_AstNode *type = MAREA_SemaChildAt(parameter, 1);

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_SemaGetParameterSemantics(context, parameter) == MAREA_SEMA_PARAMETER_SEMANTICS_VALUE_READ_ONLY) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
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

static const MAREA_AstNode *MAREA_CALL MAREA_G_RootOf(const MAREA_AstNode *expression)
{
    const MAREA_AstNode *root = expression;

    while ((root != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_MEMBER_ACCESS) || (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_INDEX_ACCESS))) {
        root = MAREA_SemaChildAt(root, 0);
    }

    return (MAREA_G_IsIdentifier(root) == MAREA_BOOLEAN_TRUE) ? root : MAREA_NULLPTR;
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsReadOnly(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    const MAREA_AstNode *root = MAREA_G_RootOf(expression);

    if (root == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(root);

    const MAREA_IntegerSize length = MAREA_AstNodeGetTextLength(root);

    for (MAREA_IntegerSize index = context->readOnly.count; index > 0; --index) {
        const MAREA_SemaName *entry = &context->readOnly.entries[(index - 1)];

        if ((entry->length == length) && (memcmp(entry->text, text, length) == 0)) {
            return (entry->declaration != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the declaration is the binder when the name is read-only and null when it is writable, the same shape as the references table.
static MAREA_SemaExcept MAREA_CALL MAREA_G_PushReadOnly(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *binding, MAREA_Boolean readOnly)
{
    if (MAREA_G_IsIdentifier(name) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaTableAppend(context, &context->readOnly, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = (readOnly == MAREA_BOOLEAN_TRUE) ? binding : MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsExistingValue(const MAREA_AstNode *expression);

MAREA_SemaExcept MAREA_CALL MAREA_SemaBindReadOnly(MAREA_SemaContext *context, const MAREA_AstNode *node)
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

                if (parameter == MAREA_NULLPTR) {
                    continue;
                }

                const MAREA_SemaExcept RESULT = MAREA_G_PushReadOnly(context, MAREA_SemaChildAt(parameter, 0), parameter, (MAREA_SemaGetParameterSemantics(context, parameter) == MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_READ_ONLY) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);

                if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return RESULT;
                }
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if ((KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT) && (COUNT >= 2)) {
        // INFO: the children are the loop names, the source and the body, so the source is the second child from the end.
        const MAREA_IntegerSize SOURCE_INDEX = (COUNT - 2);

        const MAREA_Boolean WRITABLE = (((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) != 0) && (MAREA_SemaIsReadOnly(context, MAREA_SemaChildAt(node, SOURCE_INDEX)) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        for (MAREA_IntegerSize index = 0; index < SOURCE_INDEX; ++index) {
            const MAREA_SemaExcept RESULT = MAREA_G_PushReadOnly(context, MAREA_SemaChildAt(node, index), node, (WRITABLE == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_PARTITION_STATEMENT) {
        const MAREA_Boolean WRITABLE = (((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) != 0) && (MAREA_SemaIsReadOnly(context, MAREA_SemaChildAt(node, 0)) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        return MAREA_G_PushReadOnly(context, MAREA_SemaChildAt(node, 2), node, (WRITABLE == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE);
    }

    if (KIND == MAREA_AST_KIND_WITH_STATEMENT) {
        // INFO: a with over an existing value is a borrowed view; a with over a call holds a value that the call returned, so it may be written.
        return MAREA_G_PushReadOnly(context, MAREA_SemaChildAt(node, 1), node, MAREA_G_IsExistingValue(MAREA_SemaChildAt(node, 0)));
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
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

// INFO: a field or an element chain rooted in a confined name is a view into the borrow, so it is an escape unless the field is a value; an unknown type is accepted.
static MAREA_Boolean MAREA_CALL MAREA_G_IsEscapingField(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    if ((expression == MAREA_NULLPTR) || ((MAREA_AstNodeGetKind(expression) != MAREA_AST_KIND_MEMBER_ACCESS) && (MAREA_AstNodeGetKind(expression) != MAREA_AST_KIND_INDEX_ACCESS))) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *root = MAREA_G_RootOf(expression);

    if ((root == MAREA_NULLPTR) || (MAREA_G_IsConfinedName(context, root) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *type = MAREA_SemaInferType(context, expression);

    if (type == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    // INFO: a generic type is not known to be a view until it is bound, so it is accepted like an unknown type.
    if (MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_GENERIC) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_G_IsByValue(context, type, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_FALSE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_DeclaresLocal(const MAREA_AstNode *node, const MAREA_AstNode *name)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_LOCAL_DECLARATION) {
        const MAREA_AstNode *declared = MAREA_SemaChildAt(node, 0);

        if ((MAREA_G_IsIdentifier(declared) == MAREA_BOOLEAN_TRUE) && (MAREA_AstNodeGetTextLength(declared) == MAREA_AstNodeGetTextLength(name)) && (memcmp(MAREA_AstNodeGetText(declared), MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        if (MAREA_G_DeclaresLocal(MAREA_SemaChildAt(node, index), name) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: a partition part lives for the body only, so a name that the partition body does not declare outlives it.
static MAREA_Boolean MAREA_CALL MAREA_G_IsPartStoredOutside(const MAREA_SemaContext *context, const MAREA_AstNode *target, const MAREA_AstNode *value)
{
    if ((MAREA_G_IsIdentifier(target) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsIdentifier(value) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = context->references.count; index > 0; --index) {
        const MAREA_SemaName *entry = &context->references.entries[(index - 1)];

        if ((entry->length != MAREA_AstNodeGetTextLength(value)) || (memcmp(entry->text, MAREA_AstNodeGetText(value), entry->length) != 0)) {
            continue;
        }

        if ((entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_PARTITION_STATEMENT)) {
            return MAREA_BOOLEAN_FALSE;
        }

        return (MAREA_G_DeclaresLocal(entry->declaration, target) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: append is the one library call that stores its argument and that sema knows by name; any other library call that stores is not seen.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckAppendArguments(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((callee == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(callee) != MAREA_AST_KIND_MEMBER_ACCESS)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *method = MAREA_SemaChildAt(callee, 1);

    if ((MAREA_G_IsIdentifier(method) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetTextLength(method) != 6) || (memcmp(MAREA_AstNodeGetText(method), "append", 6) != 0)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(call, 1);

    for (MAREA_IntegerSize index = 0; ((arguments != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(arguments))); ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if (MAREA_G_IsConfinedName(context, argument) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_G_ReportEscape(context, argument, "a reference cannot be stored in a vector:");

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
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
    // INFO: value_read_only is legal only for a primitive; every non-primitive is never a copy contract.
    if (MAREA_SemaAttributeHasMember(MAREA_NULLPTR, parameter, "semantics", "value_read_only") == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaClassifyType(context, MAREA_SemaChildAt(parameter, 1), &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *parameterName = MAREA_SemaChildAt(parameter, 0);

    if (MAREA_G_IsIdentifier(parameterName) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_BORROW, MAREA_AstNodeGetPosition(parameter), "value_read_only is legal only for a primitive:", MAREA_AstNodeGetText(parameterName), MAREA_AstNodeGetTextLength(parameterName));
}

// INFO: a mutable loop or partition binds a read-only name exactly when its source is read-only, so the bound name is the test and the report goes to the source.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckMutableSource(MAREA_SemaContext *context, const MAREA_AstNode *bound, const MAREA_AstNode *source)
{
    if ((MAREA_G_IsIdentifier(bound) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaIsReadOnly(context, bound) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *root = MAREA_G_RootOf(source);

    if (root == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW, MAREA_AstNodeGetPosition(source), "a mutable view of a read-only borrow:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
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

        if (MAREA_G_IsEscapingField(context, value) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_ReportEscape(context, MAREA_G_RootOf(value), "a field of a reference cannot be returned:");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_CheckAppendArguments(context, node);
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
        const MAREA_AstNode *written = MAREA_SemaChildAt(node, 0);

        if (MAREA_SemaIsReadOnly(context, written) == MAREA_BOOLEAN_TRUE) {
            const MAREA_AstNode *root = MAREA_G_RootOf(written);

            const MAREA_SemaExcept WRITE_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW, MAREA_AstNodeGetPosition(written), "a write through a read-only borrow:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));

            if (WRITE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return WRITE_RESULT;
            }
        }

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

        if (MAREA_G_IsPartStoredOutside(context, target, value) == MAREA_BOOLEAN_TRUE) {
            const MAREA_SemaExcept PART_RESULT = MAREA_G_ReportEscape(context, value, "a partition part cannot outlive its body:");

            if (PART_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return PART_RESULT;
            }
        }

        return MAREA_G_CheckCopyOrMove(context, node, MAREA_SemaInferType(context, target), value);
    }
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    {
        const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

        if ((COUNT < 2) || ((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) == 0)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        // INFO: the binder of this node was just pushed, and it is read-only only when the source is, so the first loop name tells.
        return MAREA_G_CheckMutableSource(context, MAREA_SemaChildAt(node, 0), MAREA_SemaChildAt(node, (COUNT - 2)));
    }
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        if ((MAREA_AstNodeGetFlags(node) & MAREA_AST_FLAG_MUTABLE) == 0) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_G_CheckMutableSource(context, MAREA_SemaChildAt(node, 2), MAREA_SemaChildAt(node, 0));
    case MAREA_AST_KIND_LOCAL_DECLARATION:
    {
        const MAREA_SemaExcept LITERAL_RESULT = MAREA_SemaCheckExpressionFit(context, MAREA_SemaChildAt(node, 1), MAREA_SemaChildAt(node, 2));

        if (LITERAL_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return LITERAL_RESULT;
        }

        const MAREA_SemaExcept RANGE_RESULT = MAREA_SemaCheckLiteralRange(context, MAREA_SemaChildAt(node, 1), MAREA_SemaChildAt(node, 2));

        if (RANGE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RANGE_RESULT;
        }

        return MAREA_G_CheckCopyOrMove(context, node, MAREA_G_IsType(MAREA_SemaChildAt(node, 1)) == MAREA_BOOLEAN_TRUE ? MAREA_SemaChildAt(node, 1) : MAREA_NULLPTR, MAREA_SemaChildAt(node, 2));
    }
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsFileConstantIdentifier(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) != MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION) && (entry->declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry->declaration) == MAREA_AST_KIND_CONSTANT_DECLARATION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChainRoot(const MAREA_AstNode *expression)
{
    const MAREA_AstNode *root = expression;

    while ((root != MAREA_NULLPTR) && ((MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_MEMBER_ACCESS) || (MAREA_AstNodeGetKind(root) == MAREA_AST_KIND_INDEX_ACCESS))) {
        root = MAREA_SemaChildAt(root, 0);
    }

    return root;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckBorrowArguments(MAREA_SemaContext *context, const MAREA_AstNode *call, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arguments);

    for (MAREA_IntegerSize first = 0; first < COUNT; ++first) {
        const MAREA_AstNode *left = MAREA_SemaChildAt(arguments, first);

        const MAREA_AstNode *firstParameter = MAREA_SemaChildAt(parameters, first);

        const MAREA_Boolean FIRST_WRITE = ((firstParameter != MAREA_NULLPTR) && (MAREA_SemaGetParameterSemantics(context, firstParameter) == MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        const MAREA_AstNode *root = MAREA_G_ChainRoot(left);

        if ((FIRST_WRITE == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsIdentifier(root) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsFileConstantIdentifier(context, root) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_CANNOT_BORROW_CONSTANT_AS_MUTABLE, MAREA_AstNodeGetPosition(left), "a constant cannot be bound to a mutable borrow:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));
        }

        if (MAREA_G_IsIdentifier(left) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        for (MAREA_IntegerSize second = (first + 1); second < COUNT; ++second) {
            const MAREA_AstNode *right = MAREA_SemaChildAt(arguments, second);

            if ((MAREA_G_IsIdentifier(right) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetTextLength(left) != MAREA_AstNodeGetTextLength(right)) || (memcmp(MAREA_AstNodeGetText(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(left)) != 0)) {
                continue;
            }

            const MAREA_AstNode *rightParameter = MAREA_SemaChildAt(parameters, second);

            const MAREA_Boolean RIGHT_WRITE = ((rightParameter != MAREA_NULLPTR) && (MAREA_SemaGetParameterSemantics(context, rightParameter) == MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

            if ((FIRST_WRITE == MAREA_BOOLEAN_FALSE) && (RIGHT_WRITE == MAREA_BOOLEAN_FALSE)) {
                continue;
            }

            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT, MAREA_AstNodeGetPosition(call), "a write borrow is passed together with another use of", MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left));
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsTemporary(const MAREA_AstNode *argument)
{
    switch (MAREA_AstNodeGetKind(argument)) {
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_LITERAL_VALUE:
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
    case MAREA_AST_KIND_STRING_LITERAL:
    case MAREA_AST_KIND_CHARACTER_LITERAL:
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckWriteArguments(MAREA_SemaContext *context, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments)
{
    if ((context == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arguments);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((parameter == MAREA_NULLPTR) || (argument == MAREA_NULLPTR) || (MAREA_SemaGetParameterSemantics(context, parameter) != MAREA_SEMA_PARAMETER_SEMANTICS_REFERENCE_MUTABLE)) {
            continue;
        }

        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        // INFO: an owned, task or vector parameter takes its argument by move, so it is not a write borrow of a place.
        if (MAREA_SemaClassifyType(context, MAREA_SemaChildAt(parameter, 1), &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) {
            continue;
        }

        if (MAREA_G_IsTemporary(argument) == MAREA_BOOLEAN_TRUE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW, MAREA_AstNodeGetPosition(argument), "a write borrow of a temporary, which has no storage to write back");

            continue;
        }

        const MAREA_AstNode *root = MAREA_G_RootOf(argument);

        if ((root == MAREA_NULLPTR) || (MAREA_SemaIsReadOnly(context, argument) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_SemaExcept READ_ONLY_RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_WRITE_THROUGH_READ_ONLY_BORROW, MAREA_AstNodeGetPosition(argument), "a read-only borrow is passed to a write borrow:", MAREA_AstNodeGetText(root), MAREA_AstNodeGetTextLength(root));

        if (READ_ONLY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return READ_ONLY_RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckOwnedArguments(MAREA_SemaContext *context, const MAREA_AstNode *parameters, const MAREA_AstNode *arguments)
{
    if ((context == MAREA_NULLPTR) || (parameters == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(arguments);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((parameter == MAREA_NULLPTR) || (argument == MAREA_NULLPTR)) {
            continue;
        }

        const MAREA_AstNode *declaration = MAREA_NULLPTR;

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        // INFO: only owned is checked; a vector or task parameter can be an explicit borrow, so it is left alone.
        if ((MAREA_SemaClassifyType(context, MAREA_SemaChildAt(parameter, 1), &declaration, &name, &length) != MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN) || (length != 5) || (memcmp(name, "owned", 5) != 0)) {
            continue;
        }

        // INFO: a null type keeps the rule conservative: an argument of an unknown type is accepted.
        const MAREA_SemaExcept RESULT = MAREA_G_CheckCopyOrMove(context, argument, MAREA_NULLPTR, argument);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
