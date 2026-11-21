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

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckDeclarations(MAREA_SemaContext *context, const MAREA_AstNode *root)
{
    if ((context == MAREA_NULLPTR) || (root == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(root);

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
