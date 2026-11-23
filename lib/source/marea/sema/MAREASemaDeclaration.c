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
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}
