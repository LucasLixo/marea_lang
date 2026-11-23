// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: void is not a type of the language. It is only the text of the resolved type of a call that returns nothing, and it never appears in a declaration.
#define MAREA_G_VOID_TEXT "void"

// INFO: size_of<T>() and align_of<T>() give a size in bytes, which the layout examples declare as uinteger64.
#define MAREA_G_LAYOUT_TEXT "uinteger64"

static MAREA_SemaExprType MAREA_CALL MAREA_G_FixedType(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = text, .fixedLength = length };
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_SemaTextEquals(text, length, expected, strlen(expected));
}

MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstVoidType(void)
{
    return MAREA_G_FixedType(MAREA_G_VOID_TEXT, MAREA_INTEGER_SIZE(4));
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstTypeIsVoid(const MAREA_SemaExprType *type)
{
    if ((type == MAREA_NULLPTR) || (type->fixed == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_G_TextIs(type->fixed, type->fixedLength, MAREA_G_VOID_TEXT);
}

// INFO: a call of size_of or align_of has the built-in shape: one type argument, no argument, and no name of the same text bound in the unit. The callee name itself keeps no type.
MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstBuiltinCallType(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_SemaExprType NONE = MAREA_G_FixedType(MAREA_NULLPTR, 0);

    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if (MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return NONE;
    }

    const MAREA_Character8 *name = MAREA_AstNodeGetText(callee);

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(callee);

    if ((MAREA_G_TextIs(name, NAME_LENGTH, "size_of") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(name, NAME_LENGTH, "align_of") == MAREA_BOOLEAN_FALSE)) {
        return NONE;
    }

    if ((MAREA_SemaLocalFind(context, name, NAME_LENGTH) != MAREA_NULLPTR) || (MAREA_SemaNameTableFind(context, name, NAME_LENGTH) != MAREA_NULLPTR)) {
        return NONE;
    }

    const MAREA_AstNode *typeArguments = MAREA_SemaChildAt(call, 1);

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(call, 2);

    if ((MAREA_G_IsKind(typeArguments, MAREA_AST_KIND_TYPE_ARGUMENTS) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(arguments, MAREA_AST_KIND_ARGUMENT_LIST) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetChildCount(typeArguments) != 1) || (MAREA_AstNodeGetChildCount(arguments) != 0)) {
        return NONE;
    }

    return MAREA_G_FixedType(MAREA_G_LAYOUT_TEXT, MAREA_INTEGER_SIZE(10));
}
