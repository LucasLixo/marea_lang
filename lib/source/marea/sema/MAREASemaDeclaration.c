// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_TypeEquals(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_AstNodeGetKind(left) != MAREA_AstNodeGetKind(right)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(left);

    if (LENGTH != MAREA_AstNodeGetTextLength(right)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((LENGTH > 0) && (memcmp(MAREA_AstNodeGetText(left), MAREA_AstNodeGetText(right), LENGTH) != 0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(left);

    if (COUNT != MAREA_AstNodeGetChildCount(right)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        if (MAREA_G_TypeEquals(MAREA_SemaChildAt(left, index), MAREA_SemaChildAt(right, index)) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ImplementedType(const MAREA_AstNode *implementation)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(implementation);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(implementation, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_NAMED_TYPE)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_MemberName(const MAREA_AstNode *member, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (member == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(member);

    if ((KIND != MAREA_AST_KIND_FUNCTION_DECLARATION) && (KIND != MAREA_AST_KIND_CONSTANT_DECLARATION)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_SemaDeclarationName(member, outText, outLength);
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasMember(const MAREA_AstNode *implementation, MAREA_IntegerSize limit, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    for (MAREA_IntegerSize index = 0; index < limit; ++index) {
        const MAREA_Character8 *text = MAREA_NULLPTR;

        MAREA_IntegerSize length = 0;

        if ((MAREA_G_MemberName(MAREA_SemaChildAt(implementation, index), &text, &length) == MAREA_BOOLEAN_TRUE) && (length == nameLength) && (memcmp(text, name, length) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the integer family a tag may name. An enum with no tag is sinteger32.
static const MAREA_Character8 *const MAREA_G_IntegerTagTypes[] = {
    "sinteger8",
    "sinteger16",
    "sinteger32",
    "sinteger64",
    "uinteger8",
    "uinteger16",
    "uinteger32",
    "uinteger64",
};

static MAREA_Boolean MAREA_CALL MAREA_G_IsIntegerTag(const MAREA_AstNode *tag)
{
    // INFO: a tag with type arguments has two children, so only a bare one-segment name can match.
    if ((MAREA_AstNodeGetKind(tag) != MAREA_AST_KIND_NAMED_TYPE) || (MAREA_AstNodeGetChildCount(tag) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *path = MAREA_SemaChildAt(tag, 0);

    if ((path == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(path) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(path) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(path, 0);

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_IntegerTagTypes); ++index) {
        if ((MAREA_AstNodeGetTextLength(name) == strlen(MAREA_G_IntegerTagTypes[index])) && (memcmp(MAREA_AstNodeGetText(name), MAREA_G_IntegerTagTypes[index], MAREA_AstNodeGetTextLength(name)) == 0)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckEnumTag(MAREA_SemaContext *context, const MAREA_AstNode *enumeration)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(enumeration);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(enumeration, index);

        if ((child == MAREA_NULLPTR) || ((MAREA_AstNodeGetKind(child) != MAREA_AST_KIND_NAMED_TYPE) && (MAREA_AstNodeGetKind(child) != MAREA_AST_KIND_FUNCTION_TYPE))) {
            continue;
        }

        if (MAREA_G_IsIntegerTag(child) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_AstNode *name = MAREA_SemaChildAt(enumeration, (index > 0) ? (index - 1) : 0);

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ENUM_TAG_TYPE, MAREA_AstNodeGetPosition(child), "an enum tag must be an integer primitive:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the first identifier child is the name of a field, a variant, a contract function, a generic parameter or a parameter.
static const MAREA_AstNode *MAREA_CALL MAREA_G_NameOf(const MAREA_AstNode *member)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(member);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(member, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_IDENTIFIER)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_SameName(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    return ((left != MAREA_NULLPTR) && (right != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(right)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: true when an earlier child of the list, of the same kind, has the name of the child at the given index.
static MAREA_Boolean MAREA_CALL MAREA_G_RepeatsEarlier(const MAREA_AstNode *list, MAREA_IntegerSize index, MAREA_AstKind kind)
{
    const MAREA_AstNode *name = MAREA_G_NameOf(MAREA_SemaChildAt(list, index));

    for (MAREA_IntegerSize earlier = 0; ((name != MAREA_NULLPTR) && (earlier < index)); ++earlier) {
        const MAREA_AstNode *other = MAREA_SemaChildAt(list, earlier);

        if ((other != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(other) == kind) && (MAREA_G_SameName(name, MAREA_G_NameOf(other)) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckMembers(MAREA_SemaContext *context, const MAREA_AstNode *declaration)
{
    const MAREA_AstKind DECLARATION_KIND = MAREA_AstNodeGetKind(declaration);

    MAREA_AstKind memberKind = MAREA_AST_KIND_CONTRACT_FUNCTION;

    if (DECLARATION_KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) {
        memberKind = MAREA_AST_KIND_FIELD_DECLARATION;
    } else if (DECLARATION_KIND == MAREA_AST_KIND_ENUM_DECLARATION) {
        memberKind = MAREA_AST_KIND_VARIANT_DECLARATION;
    } else if (DECLARATION_KIND != MAREA_AST_KIND_CONTRACT_DECLARATION) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(declaration);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(declaration, index);

        if ((member == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(member) != memberKind) || (MAREA_G_RepeatsEarlier(declaration, index, memberKind) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_AstNode *name = MAREA_G_NameOf(member);

        const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_MEMBER, MAREA_AstNodeGetPosition(name), "member is already declared:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindKind(const MAREA_AstNode *node, MAREA_AstKind kind)
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

// INFO: true for a NAMED_TYPE that is one plain segment; a qualified name belongs to another file and is skipped.
static MAREA_Boolean MAREA_CALL MAREA_G_PlainTypeName(const MAREA_AstNode *type, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if ((type == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(type) != MAREA_AST_KIND_NAMED_TYPE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *path = MAREA_SemaChildAt(type, 0);

    if ((path == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(path) != MAREA_AST_KIND_QUALIFIED_NAME) || (MAREA_AstNodeGetChildCount(path) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(path, 0);

    if ((segment == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(segment) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outText = MAREA_AstNodeGetText(segment);
    *outLength = MAREA_AstNodeGetTextLength(segment);

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the declaration of the file that a plain type name stands for, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_LocalDeclaration(const MAREA_SemaContext *context, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, name, nameLength);

    return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION)) ? entry->declaration : MAREA_NULLPTR;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckAlias(MAREA_SemaContext *context, const MAREA_AstNode *alias, MAREA_IntegerSize limit)
{
    if (MAREA_G_FindKind(alias, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *current = alias;

    for (MAREA_IntegerSize step = 0; step <= limit; ++step) {
        const MAREA_AstNode *target = MAREA_SemaChildAt(current, (MAREA_AstNodeGetChildCount(current) - MAREA_INTEGER_SIZE(1)));

        const MAREA_Character8 *name = MAREA_NULLPTR;

        MAREA_IntegerSize nameLength = 0;

        if ((MAREA_G_PlainTypeName(target, &name, &nameLength) == MAREA_BOOLEAN_FALSE) || (MAREA_G_FindKind(target, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_AstNode *next = MAREA_G_LocalDeclaration(context, name, nameLength);

        if ((next == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(next) != MAREA_AST_KIND_TYPE_DECLARATION) || (MAREA_G_FindKind(next, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if (next == alias) {
            const MAREA_AstNode *identifier = MAREA_G_NameOf(alias);

            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_TYPE_ALIAS, MAREA_AstNodeGetPosition(alias), "type alias leads back to itself:", MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier));
        }

        current = next;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

typedef struct MAREA_G_ValueWalk
{
    const MAREA_SemaContext *context;
    const MAREA_AstNode *target;
    const MAREA_AstNode **visited;
    MAREA_IntegerSize visitedCount;
    MAREA_IntegerSize limit;
} MAREA_G_ValueWalk;

static MAREA_Boolean MAREA_CALL MAREA_G_HoldsTarget(MAREA_G_ValueWalk *walk, const MAREA_AstNode *type, const MAREA_AstNode *generics, MAREA_IntegerSize depth);

// INFO: true when a field type of the declaration holds the target by value.
static MAREA_Boolean MAREA_CALL MAREA_G_MemberHoldsTarget(MAREA_G_ValueWalk *walk, const MAREA_AstNode *member, const MAREA_AstNode *generics)
{
    if (MAREA_AstNodeGetKind(member) == MAREA_AST_KIND_FIELD_DECLARATION) {
        return MAREA_G_HoldsTarget(walk, MAREA_SemaChildAt(member, (MAREA_AstNodeGetChildCount(member) - MAREA_INTEGER_SIZE(1))), generics, 0);
    }

    if (MAREA_AstNodeGetKind(member) != MAREA_AST_KIND_VARIANT_DECLARATION) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(member); ++index) {
        const MAREA_AstNode *field = MAREA_SemaChildAt(member, index);

        if ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_VARIANT_FIELD) && (MAREA_G_HoldsTarget(walk, MAREA_SemaChildAt(field, 1), generics, 0) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: owned, vector, pointer, slice, task and a function type are indirect, so only a structure, an enum, an alias to one and the element of an array are followed.
static MAREA_Boolean MAREA_CALL MAREA_G_HoldsTarget(MAREA_G_ValueWalk *walk, const MAREA_AstNode *type, const MAREA_AstNode *generics, MAREA_IntegerSize depth)
{
    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize nameLength = 0;

    if ((MAREA_G_PlainTypeName(type, &name, &nameLength) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaGenericParameterIndex(generics, name, nameLength) < ((generics == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(generics)))) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaTextEquals(name, nameLength, "array", MAREA_INTEGER_SIZE(5)) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *arguments = MAREA_G_FindKind(type, MAREA_AST_KIND_TYPE_ARGUMENTS);

        return (arguments == MAREA_NULLPTR) ? MAREA_BOOLEAN_FALSE : MAREA_G_HoldsTarget(walk, MAREA_SemaChildAt(arguments, 0), generics, depth);
    }

    const MAREA_AstNode *declaration = MAREA_G_LocalDeclaration(walk->context, name, nameLength);

    if (declaration == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(declaration);

    if (KIND == MAREA_AST_KIND_TYPE_DECLARATION) {
        if ((depth >= walk->limit) || (MAREA_G_FindKind(declaration, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
            return MAREA_BOOLEAN_FALSE;
        }

        return MAREA_G_HoldsTarget(walk, MAREA_SemaChildAt(declaration, (MAREA_AstNodeGetChildCount(declaration) - MAREA_INTEGER_SIZE(1))), MAREA_NULLPTR, (depth + 1));
    }

    if ((KIND != MAREA_AST_KIND_STRUCTURE_DECLARATION) && (KIND != MAREA_AST_KIND_ENUM_DECLARATION)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (declaration == walk->target) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < walk->visitedCount; ++index) {
        if (walk->visited[index] == declaration) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    if (walk->visitedCount >= walk->limit) {
        return MAREA_BOOLEAN_FALSE;
    }

    walk->visited[walk->visitedCount] = declaration;
    ++walk->visitedCount;

    const MAREA_AstNode *inner = MAREA_G_FindKind(declaration, MAREA_AST_KIND_GENERIC_PARAMETERS);

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(declaration); ++index) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(declaration, index);

        if ((member != MAREA_NULLPTR) && (MAREA_G_MemberHoldsTarget(walk, member, inner) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckRecursion(MAREA_SemaContext *context, const MAREA_AstNode *declaration, const MAREA_AstNode **visited, MAREA_IntegerSize limit)
{
    MAREA_G_ValueWalk walk = { context, declaration, visited, 0, limit };

    const MAREA_AstNode *generics = MAREA_G_FindKind(declaration, MAREA_AST_KIND_GENERIC_PARAMETERS);

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(declaration); ++index) {
        const MAREA_AstNode *member = MAREA_SemaChildAt(declaration, index);

        if ((member == MAREA_NULLPTR) || (MAREA_G_MemberHoldsTarget(&walk, member, generics) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        const MAREA_AstNode *identifier = MAREA_G_NameOf(declaration);

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_RECURSIVE_TYPE, MAREA_AstNodeGetPosition(member), "type holds itself by value:", MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier));
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: the alias pass runs first, so a cyclic alias reports once and then ends every walk through it by the depth limit.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckTypeCycles(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, index);

        if ((declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_TYPE_DECLARATION)) {
            const MAREA_SemaExcept RESULT = MAREA_G_CheckAlias(context, declaration, COUNT);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    }

    void *memory = MAREA_NULLPTR;

    if ((COUNT == 0) || (MAREA_ArenaAllocate(context->arena, (COUNT * sizeof(const MAREA_AstNode *)), MAREA_ALIGNOF(const MAREA_AstNode *), &memory) != MAREA_ARENA_EXCEPT_SUCCESS)) {
        return (COUNT == 0) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, index);

        if ((declaration == MAREA_NULLPTR) || ((MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION) && (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_ENUM_DECLARATION))) {
            continue;
        }

        const MAREA_SemaExcept RESULT = MAREA_G_CheckRecursion(context, declaration, (const MAREA_AstNode **)memory, COUNT);

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_ChildOfKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: an argument that is a bare name of a generic parameter of its own block matches any type.
static MAREA_Boolean MAREA_CALL MAREA_G_IsOwnParameter(const MAREA_AstNode *implementation, const MAREA_AstNode *argument)
{
    const MAREA_AstNode *qualified = ((argument != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_NAMED_TYPE) && (MAREA_AstNodeGetChildCount(argument) == 1)) ? MAREA_SemaChildAt(argument, 0) : MAREA_NULLPTR;

    const MAREA_AstNode *segment = ((qualified != MAREA_NULLPTR) && (MAREA_AstNodeGetChildCount(qualified) == 1)) ? MAREA_SemaChildAt(qualified, 0) : MAREA_NULLPTR;

    const MAREA_AstNode *parameters = MAREA_G_ChildOfKind(implementation, MAREA_AST_KIND_GENERIC_PARAMETERS);

    for (MAREA_IntegerSize index = 0; ((segment != MAREA_NULLPTR) && (parameters != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(parameters))); ++index) {
        const MAREA_AstNode *name = MAREA_G_NameOf(MAREA_SemaChildAt(parameters, index));

        if ((name != MAREA_NULLPTR) && (MAREA_G_SameName(name, segment) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: two blocks overlap when their types have the same head and argument count and every pair of arguments is equal text or has a bare generic parameter on a side. A type written the same twice is not an overlap: it is a duplicate member.
static MAREA_Boolean MAREA_CALL MAREA_G_Overlaps(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    const MAREA_AstNode *leftType = MAREA_G_ImplementedType(left);

    const MAREA_AstNode *rightType = MAREA_G_ImplementedType(right);

    if ((leftType == MAREA_NULLPTR) || (rightType == MAREA_NULLPTR) || (MAREA_G_TypeEquals(leftType, rightType) == MAREA_BOOLEAN_TRUE) || (MAREA_G_TypeEquals(MAREA_SemaChildAt(leftType, 0), MAREA_SemaChildAt(rightType, 0)) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *leftArguments = MAREA_G_ChildOfKind(leftType, MAREA_AST_KIND_TYPE_ARGUMENTS);

    const MAREA_AstNode *rightArguments = MAREA_G_ChildOfKind(rightType, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if ((leftArguments == MAREA_NULLPTR) || (rightArguments == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(leftArguments) != MAREA_AstNodeGetChildCount(rightArguments))) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(leftArguments); ++index) {
        const MAREA_AstNode *leftArgument = MAREA_SemaChildAt(leftArguments, index);

        const MAREA_AstNode *rightArgument = MAREA_SemaChildAt(rightArguments, index);

        if ((MAREA_G_TypeEquals(leftArgument, rightArgument) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsOwnParameter(left, leftArgument) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsOwnParameter(right, rightArgument) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_OverlapsEarlier(const MAREA_AstNode *root, MAREA_IntegerSize current, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_AstNode *implementation = MAREA_SemaChildAt(root, current);

    for (MAREA_IntegerSize earlier = 0; earlier < current; ++earlier) {
        const MAREA_AstNode *other = MAREA_SemaChildAt(root, earlier);

        if ((other != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(other) == MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) && (MAREA_G_Overlaps(implementation, other) == MAREA_BOOLEAN_TRUE) && (MAREA_G_HasMember(other, MAREA_AstNodeGetChildCount(other), name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: an enum with no VARIANT_DECLARATION child has no value, so it is reported once at the enum.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckEnumNotEmpty(MAREA_SemaContext *context, const MAREA_AstNode *enumeration)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(enumeration);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(enumeration, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_VARIANT_DECLARATION)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }
    }

    const MAREA_AstNode *name = MAREA_G_NameOf(enumeration);

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_EMPTY_ENUM, MAREA_AstNodeGetPosition(enumeration), "enum has no variant:", (name != MAREA_NULLPTR) ? MAREA_AstNodeGetText(name) : "", (name != MAREA_NULLPTR) ? MAREA_AstNodeGetTextLength(name) : MAREA_INTEGER_SIZE(0));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckDeclarations(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if ((context == MAREA_NULLPTR) || (root == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

    for (MAREA_IntegerSize current = 0; current < COUNT; ++current) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, current);

        if ((declaration != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION)) {
            const MAREA_SemaExcept TAG_RESULT = MAREA_G_CheckEnumTag(context, declaration);

            if (TAG_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return TAG_RESULT;
            }

            const MAREA_SemaExcept EMPTY_RESULT = MAREA_G_CheckEnumNotEmpty(context, declaration);

            if (EMPTY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return EMPTY_RESULT;
            }
        }
    }

    for (MAREA_IntegerSize current = 0; current < COUNT; ++current) {
        const MAREA_AstNode *declaration = MAREA_SemaChildAt(root, current);

        if (declaration != MAREA_NULLPTR) {
            const MAREA_SemaExcept MEMBER_RESULT = MAREA_G_CheckMembers(context, declaration);

            if (MEMBER_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return MEMBER_RESULT;
            }
        }
    }

    for (MAREA_IntegerSize current = 0; current < COUNT; ++current) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(root, current);

        if ((implementation == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(implementation) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
            continue;
        }

        const MAREA_AstNode *type = MAREA_G_ImplementedType(implementation);

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_AstNodeGetChildCount(implementation);

        for (MAREA_IntegerSize memberIndex = 0; memberIndex < MEMBER_COUNT; ++memberIndex) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, memberIndex);

            const MAREA_Character8 *name = MAREA_NULLPTR;

            MAREA_IntegerSize nameLength = 0;

            if (MAREA_G_MemberName(member, &name, &nameLength) == MAREA_BOOLEAN_FALSE) {
                continue;
            }

            MAREA_Boolean duplicate = MAREA_G_HasMember(implementation, memberIndex, name, nameLength);

            for (MAREA_IntegerSize earlier = 0; ((earlier < current) && (duplicate == MAREA_BOOLEAN_FALSE)); ++earlier) {
                const MAREA_AstNode *other = MAREA_SemaChildAt(root, earlier);

                if ((other == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(other) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION)) {
                    continue;
                }

                if (MAREA_G_TypeEquals(type, MAREA_G_ImplementedType(other)) == MAREA_BOOLEAN_TRUE) {
                    duplicate = MAREA_G_HasMember(other, MAREA_AstNodeGetChildCount(other), name, nameLength);
                }
            }

            if (duplicate == MAREA_BOOLEAN_TRUE) {
                const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_MEMBER, MAREA_AstNodeGetPosition(member), "member is already declared:", name, nameLength);

                if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return RESULT;
                }
            }

            if ((duplicate == MAREA_BOOLEAN_FALSE) && (MAREA_G_OverlapsEarlier(root, current, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
                const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_OVERLAPPING_IMPLEMENTATION, MAREA_AstNodeGetPosition(member), "implementation blocks overlap on the member:", name, nameLength);

                if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return RESULT;
                }
            }
        }
    }

    return MAREA_G_CheckTypeCycles(context, root);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckDuplicateNames(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    if ((KIND == MAREA_AST_KIND_PARAMETER_LIST) || (KIND == MAREA_AST_KIND_GENERIC_PARAMETERS)) {
        const MAREA_AstKind ITEM_KIND = (KIND == MAREA_AST_KIND_PARAMETER_LIST) ? MAREA_AST_KIND_PARAMETER : MAREA_AST_KIND_GENERIC_PARAMETER;

        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *name = MAREA_G_NameOf(MAREA_SemaChildAt(node, index));

            if (name == MAREA_NULLPTR) {
                continue;
            }

            const MAREA_Boolean REPEATED = MAREA_G_RepeatsEarlier(node, index, ITEM_KIND);

            const MAREA_Boolean TOP_LEVEL = ((KIND == MAREA_AST_KIND_GENERIC_PARAMETERS) && (MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

            if ((REPEATED == MAREA_BOOLEAN_TRUE) || (TOP_LEVEL == MAREA_BOOLEAN_TRUE)) {
                const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED, MAREA_AstNodeGetPosition(name), "name is already declared:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

                if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return RESULT;
                }
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (KIND == MAREA_AST_KIND_LOCAL_DECLARATION) {
        const MAREA_AstNode *name = MAREA_G_NameOf(node);

        // INFO: the table holds the names of the current function, so only the parameters and the entries of this block are compared.
        for (MAREA_IntegerSize index = context->localStart; ((name != MAREA_NULLPTR) && (index < context->locals.count)); ++index) {
            const MAREA_SemaName *entry = &context->locals.entries[index];

            if (MAREA_SemaTextEquals(entry->text, entry->length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
                return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_NAME_ALREADY_DECLARED, MAREA_AstNodeGetPosition(name), "name is already declared:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
