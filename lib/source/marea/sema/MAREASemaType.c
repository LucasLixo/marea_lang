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

MAREA_Boolean MAREA_CALL MAREA_SemaDescriptorOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_TypeDescriptor **outDescriptor)
{
    *outDescriptor = MAREA_NULLPTR;

    if ((context == MAREA_NULLPTR) || (type == MAREA_NULLPTR) || (context->typeRegistry == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE) && (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_GENERIC_NAME)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *qualifiedName = MAREA_SemaChildAt(type, 0);

    if ((qualifiedName == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(qualifiedName) != MAREA_AST_KIND_QUALIFIED_NAME)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize SEGMENT_COUNT = MAREA_AstNodeGetChildCount(qualifiedName);

    // INFO: a registered type always has a namespace, so at least two segments: the namespace, then the name.
    if (SEGMENT_COUNT < 2) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    // INFO: the join only reads the written segments and allocates from the context's own arena; it never
    // changes the context's own state, so the cast is safe for a query function that takes it const.
    if (MAREA_SemaQualifiedNameJoin((MAREA_SemaContext *)context, qualifiedName, &joined, &joinedLength) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(qualifiedName, (SEGMENT_COUNT - MAREA_INTEGER_SIZE(1)));

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(last);

    const MAREA_IntegerSize NAMESPACE_LENGTH = (joinedLength - NAME_LENGTH - MAREA_INTEGER_SIZE(1));

    *outDescriptor = MAREA_TypeRegistryFind(context->typeRegistry, joined, NAMESPACE_LENGTH, MAREA_AstNodeGetText(last), NAME_LENGTH);

    return (*outDescriptor != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaWrapInRoleType(MAREA_SemaContext *context, MAREA_TypeRole role, const MAREA_Character8 *argumentText, MAREA_IntegerSize argumentLength, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    *outText = MAREA_NULLPTR;
    *outLength = 0;

    const MAREA_TypeDescriptor *descriptor = MAREA_TypeRegistryFindByRole(context->typeRegistry, role);

    if (descriptor == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: namespace, one dot, name, "<", argument, ">".
    const MAREA_IntegerSize TOTAL = (descriptor->namespaceLength + descriptor->nameLength + argumentLength + MAREA_INTEGER_SIZE(3));

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, TOTAL, MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *buffer = memory;

    MAREA_IntegerSize cursor = 0;

    memcpy(buffer, descriptor->namespaceText, descriptor->namespaceLength);
    cursor += descriptor->namespaceLength;

    buffer[cursor++] = '.';

    memcpy((buffer + cursor), descriptor->name, descriptor->nameLength);
    cursor += descriptor->nameLength;

    buffer[cursor++] = '<';

    memcpy((buffer + cursor), argumentText, argumentLength);
    cursor += argumentLength;

    buffer[cursor++] = '>';

    *outText = buffer;
    *outLength = TOTAL;

    return MAREA_SEMA_EXCEPT_SUCCESS;
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
    outMark->readOnlyCount = context->readOnly.count;
    outMark->captureCount = context->captures.count;
    outMark->captureLocalBase = context->captureLocalBase;
    outMark->forkBody = context->forkBody;
    outMark->captureScope = (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_FORK_JOIN_BLOCK) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    outMark->localStart = context->localStart;
    outMark->localStartBinders = context->localStartBinders;
    outMark->functionLocalStart = context->functionLocalStart;
    outMark->function = context->function;
    outMark->implementation = context->implementation;
    outMark->selfDepth = context->selfDepth;
    outMark->forkDepth = context->forkDepth;
    outMark->partitionDepth = context->partitionDepth;
    outMark->loopDepth = context->loopDepth;
    outMark->unsafeDepth = context->unsafeDepth;

    const MAREA_SemaExcept BIND_RESULT = MAREA_SemaBindReferences(context, node);

    if (BIND_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return BIND_RESULT;
    }

    const MAREA_SemaExcept READ_ONLY_RESULT = MAREA_SemaBindReadOnly(context, node);

    if (READ_ONLY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return READ_ONLY_RESULT;
    }

    const MAREA_SemaExcept BINDER_RESULT = MAREA_SemaDeclareBinders(context, node);

    if (BINDER_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return BINDER_RESULT;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    // INFO: the parameters of a function share the scope of the top-level statements of its body, so the body block starts its scope where the function did.
    if (KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) {
        context->functionLocalStart = outMark->localCount;
    }

    if (KIND == MAREA_AST_KIND_BLOCK) {
        const MAREA_IntegerSize FUNCTION_COUNT = (context->function == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(context->function);

        const MAREA_Boolean IS_BODY = ((FUNCTION_COUNT > 0) && (MAREA_SemaChildAt(context->function, (FUNCTION_COUNT - 1)) == node)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        // INFO: the names of a for, for each, partition or with statement are names of its body block too.
        const MAREA_IntegerSize BINDERS = MAREA_SemaBinderCount(context->parent);

        context->localStart = (IS_BODY == MAREA_BOOLEAN_TRUE) ? context->functionLocalStart : ((context->locals.count >= BINDERS) ? (context->locals.count - BINDERS) : context->locals.count);
        context->localStartBinders = (IS_BODY == MAREA_BOOLEAN_TRUE) ? 0 : BINDERS;
    }

    if ((KIND == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) || (KIND == MAREA_AST_KIND_CONTRACT_DECLARATION)) {
        context->selfDepth += 1;
    }

    if (KIND == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) {
        context->implementation = node;
    }

    if (KIND == MAREA_AST_KIND_FORK_JOIN_BLOCK) {
        context->forkDepth += 1;

        // INFO: the locals declared before the block are shared by every turn of a loop in it, and its own block is the code that runs next to the workers.
        context->captureLocalBase = context->locals.count;
        context->forkBody = MAREA_SemaChildAt(node, 0);
    }

    if (KIND == MAREA_AST_KIND_UNSAFE_BLOCK) {
        context->unsafeDepth += 1;
    }

    // INFO: the body of a partition is a barrier. A loop outside it is not a target of a jump inside it.
    if (KIND == MAREA_AST_KIND_PARTITION_STATEMENT) {
        context->partitionDepth += 1;
        context->loopDepth = 0;
    }

    if ((KIND == MAREA_AST_KIND_WHILE_STATEMENT) || (KIND == MAREA_AST_KIND_DO_WHILE_STATEMENT) || (KIND == MAREA_AST_KIND_FOR_RANGE_STATEMENT) || (KIND == MAREA_AST_KIND_FOR_EACH_STATEMENT)) {
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
    context->readOnly.count = mark.readOnlyCount;
    context->captureLocalBase = mark.captureLocalBase;
    context->forkBody = mark.forkBody;

    // INFO: what a worker took stays until the whole fork_join block ends, so only the scope of the block drops it.
    if (mark.captureScope == MAREA_BOOLEAN_TRUE) {
        context->captures.count = mark.captureCount;
    }

    context->localStart = mark.localStart;
    context->localStartBinders = mark.localStartBinders;
    context->functionLocalStart = mark.functionLocalStart;
    context->function = mark.function;
    context->implementation = mark.implementation;
    context->selfDepth = mark.selfDepth;
    context->forkDepth = mark.forkDepth;
    context->partitionDepth = mark.partitionDepth;
    context->loopDepth = mark.loopDepth;
    context->unsafeDepth = mark.unsafeDepth;
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsBinderHeader(const MAREA_AstNode *node, MAREA_IntegerSize index)
{
    MAREA_IntegerSize first = 0;

    MAREA_IntegerSize end = 0;

    if (MAREA_SemaBinderRange(node, &first, &end) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: the body is the last child, and every other child that is no declared name is part of the header.
    return (((index < first) || (index >= end)) && ((index + 1) < MAREA_AstNodeGetChildCount(node))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaBinderWindow MAREA_CALL MAREA_SemaHideBinders(MAREA_SemaContext *context, MAREA_SemaScopeMark mark)
{
    const MAREA_SemaBinderWindow WINDOW = { .localCount = context->locals.count, .referenceCount = context->references.count, .readOnlyCount = context->readOnly.count };

    context->locals.count = mark.localCount;
    context->references.count = mark.referenceCount;
    context->readOnly.count = mark.readOnlyCount;

    return WINDOW;
}

void MAREA_CALL MAREA_SemaShowBinders(MAREA_SemaContext *context, MAREA_SemaBinderWindow window)
{
    context->locals.count = window.localCount;
    context->references.count = window.referenceCount;
    context->readOnly.count = window.readOnlyCount;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckArity(MAREA_SemaContext *context, const MAREA_AstNode *node, const MAREA_AstNode *declaration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_AstNode *arguments = MAREA_SemaChildAt(node, 1);

    const MAREA_IntegerSize ACTUAL = ((arguments != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(arguments) == MAREA_AST_KIND_TYPE_ARGUMENTS)) ? MAREA_AstNodeGetChildCount(arguments) : MAREA_INTEGER_SIZE(0);

    const MAREA_AstNode *parameters = MAREA_NULLPTR;

    MAREA_IntegerSize expected = 0;

    if (declaration == MAREA_NULLPTR) {
        expected = MAREA_INTEGER_SIZE(0);
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

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindTypeArguments(const MAREA_AstNode *node)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_TYPE_ARGUMENTS)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: whether sema knows the type has a measured layout at this shallow level: a primitive always
// does, a user structure or enum always does (its own layout record decides the detail), and a generic
// parameter is trusted until it is filled with a concrete argument (checked again then, at the call or
// the instantiation that fills it, not here). A registered type has the shape its descriptor gives (a
// handle for a task, an owner or a pointer, an inline run for an array), and its own type arguments are
// checked when its own node is visited. A function type is rejected.
static MAREA_Boolean MAREA_CALL MAREA_G_HasKnownLayout(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if (MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    // INFO: a type sema cannot see (an external one, or an unknown name that its own check reports) is not
    // guessed here; only a function type, the one shape that is known to have no layout, is rejected.
    return (MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
}

// INFO: an element type with no measured layout cannot be held by value (decision 28 of the
// 2026-11-24 plan): a registered type whose descriptor holds its first type argument inline
// (hasIndexProtocol) needs that argument to have a known layout.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckElementLayout(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if ((MAREA_SemaDescriptorOfType(context, node, &descriptor) == MAREA_BOOLEAN_FALSE) || (descriptor->hasIndexProtocol == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *arguments = MAREA_G_FindTypeArguments(node);

    const MAREA_AstNode *element = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

    if ((element == MAREA_NULLPTR) || (MAREA_G_HasKnownLayout(context, element) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_ELEMENT_WITHOUT_MEASURED_LAYOUT, MAREA_AstNodeGetPosition(element), "an element type with no measured layout cannot be held by value");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a raw pointer exists only inside an unsafe block, in every place a type is written (issue
// #190, decision 30 of the 2026-11-24 plan): a parameter, a result, a field, an alias, and nested
// inside another written type's own type arguments. MAREA_SemaCheckNamedType runs once for every
// written NAMED_TYPE, so this one hook covers all of them; a local's own check (MAREASemaRawPointer.c)
// stays the one place still reporting with the local's own name in the message.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckRawPointerPlacement(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if (context->unsafeDepth > 0) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if ((MAREA_SemaDescriptorOfType(context, node, &descriptor) == MAREA_BOOLEAN_FALSE) || ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) == MAREA_TYPE_ROLE_NONE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE, MAREA_AstNodeGetPosition(node), "a raw pointer exists only inside an unsafe block");

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckNamedType(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_SemaExcept ELEMENT_LAYOUT_RESULT = MAREA_G_CheckElementLayout(context, node);

    if (ELEMENT_LAYOUT_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return ELEMENT_LAYOUT_RESULT;
    }

    const MAREA_SemaExcept RAW_POINTER_RESULT = MAREA_G_CheckRawPointerPlacement(context, node);

    if (RAW_POINTER_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return RAW_POINTER_RESULT;
    }

    // INFO: a registered type takes no more type arguments than its descriptor declares; a library may also
    // be written with fewer, so a missing argument is the library's own business.
    const MAREA_TypeDescriptor *registered = MAREA_NULLPTR;

    const MAREA_AstNode *writtenArguments = MAREA_G_FindTypeArguments(node);

    if ((writtenArguments != MAREA_NULLPTR) && (MAREA_SemaDescriptorOfType(context, node, &registered) == MAREA_BOOLEAN_TRUE) && (MAREA_AstNodeGetChildCount(writtenArguments) > registered->typeParameterCount)) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, MAREA_AstNodeGetPosition(node), "type arguments do not match the generic parameters of", registered->name, registered->nameLength);
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

    // INFO: a type of another namespace is written with its full path, and a cover must reach that path.
    if ((SEGMENT_COUNT > 1) && (entry == MAREA_NULLPTR)) {
        MAREA_SemaPathStatus status = MAREA_SEMA_PATH_STATUS_NOT_A_PATH;

        const MAREA_SemaExcept CLASSIFY_RESULT = MAREA_SemaClassifyPath(context, joined, joinedLength, MAREA_BOOLEAN_FALSE, &status, MAREA_NULLPTR, MAREA_NULLPTR);

        if (CLASSIFY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return CLASSIFY_RESULT;
        }

        if (status == MAREA_SEMA_PATH_STATUS_COVERED) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if (status == MAREA_SEMA_PATH_STATUS_UNIMPORTED) {
            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNIMPORTED_NAMESPACE, MAREA_AstNodeGetPosition(node), "a namespace that no import or require covers:", joined, joinedLength);
        }
    }

    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    const MAREA_AstNode *foundRoot = MAREA_NULLPTR;

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION)) {
        result = MAREA_SemaResolveNamespace(context, joined, joinedLength, &foundRoot);
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
