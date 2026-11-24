// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the position of the collection among the children of a for each: the names come first, then the collection and the body.
#define MAREA_G_FOR_EACH_TAIL MAREA_INTEGER_SIZE(2)

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfFixedText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = text, .fixedLength = length };
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypeIsKnown(const MAREA_SemaExprType *type)
{
    return ((type->node != MAREA_NULLPTR) || (type->declaration != MAREA_NULLPTR) || (type->fixed != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the innermost open local with the name of an identifier child, which is the entry that the binder of the statement just added.
static MAREA_SemaName *MAREA_CALL MAREA_G_FindOpenLocal(MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE) {
            return local;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a name whose type is not known keeps no type, so a use of it has none, as before.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_BindType(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_SemaExprType *type)
{
    if (MAREA_G_TypeIsKnown(type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_SemaName *local = MAREA_G_FindOpenLocal(context, name);

    if (local == MAREA_NULLPTR) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, sizeof(MAREA_SemaExprType), MAREA_ALIGNOF(MAREA_SemaExprType), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaExprType *stored = memory;

    *stored = *type;

    local->statementType = stored;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

// INFO: the typed node of the declared name of a for or a for each also carries the type, so a reader of the typed tree needs no scope to know it. A type that sema cannot spell leaves the node as it is.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypeDeclaredName(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built, MAREA_IntegerSize index, const MAREA_SemaExprType *type)
{
    if ((MAREA_G_TypeIsKnown(type) == MAREA_BOOLEAN_FALSE) || (built->children == MAREA_NULLPTR) || (index >= built->childCount)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    MAREA_SemaTypedNode *name = built->children[index];

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name->astNode) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypedAstExcept TYPED = MAREA_SemaTypedAstRenderExprType(context, type, &text, &length);

    if ((TYPED != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
        return TYPED;
    }

    name->exprType = *type;
    name->resolvedType = text;
    name->resolvedTypeLength = length;

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}

// INFO: both bounds agree after the check, so the counter has the type that the first bound that is not a number literal gives; two literals give the type of the first.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypeRangeCounter(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    if ((built->children == MAREA_NULLPTR) || (built->childCount < 3)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    // INFO: the two bounds are the children 1 and 2, read as the two operands of an operation.
    MAREA_SemaTypedNode bounds = *built;

    bounds.children = &built->children[1];
    bounds.childCount = 2;

    const MAREA_SemaExprType COUNTER = MAREA_SemaTypedAstTypeOfOperands(&bounds);

    const MAREA_SemaTypedAstExcept BOUND_RESULT = MAREA_G_BindType(context, MAREA_SemaChildAt(built->astNode, 0), &COUNTER);

    return (BOUND_RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) ? BOUND_RESULT : MAREA_G_TypeDeclaredName(context, built, 0, &COUNTER);
}

// INFO: the value is the element of the collection. With two names the key comes first and is the index, uinteger64. No library type of this repository has another key.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypeForEachNames(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    const MAREA_IntegerSize NAME_COUNT = built->childCount - MAREA_G_FOR_EACH_TAIL;

    if ((NAME_COUNT < 1) || (NAME_COUNT > 2)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_SemaExprType COLLECTION = MAREA_SemaTypedAstTypeOfChild(built, NAME_COUNT);

    MAREA_SemaExprType element = { .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };

    if (MAREA_SemaTypedAstElementOf(context, &COLLECTION, MAREA_BOOLEAN_FALSE, &element) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_SemaTypedAstExcept VALUE_RESULT = MAREA_G_BindType(context, MAREA_SemaChildAt(built->astNode, (NAME_COUNT - 1)), &element);

    if (VALUE_RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
        return VALUE_RESULT;
    }

    const MAREA_SemaTypedAstExcept VALUE_NAME_RESULT = MAREA_G_TypeDeclaredName(context, built, (NAME_COUNT - 1), &element);

    if ((VALUE_NAME_RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (NAME_COUNT == 1)) {
        return VALUE_NAME_RESULT;
    }

    const MAREA_SemaExprType KEY = MAREA_G_TypeOfFixedText("uinteger64", MAREA_INTEGER_SIZE(10));

    const MAREA_SemaTypedAstExcept KEY_RESULT = MAREA_G_BindType(context, MAREA_SemaChildAt(built->astNode, 0), &KEY);

    return (KEY_RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) ? KEY_RESULT : MAREA_G_TypeDeclaredName(context, built, 0, &KEY);
}

// INFO: a part is a slice of the element type of the value, written slice<T> whatever the collection is.
static MAREA_SemaTypedAstExcept MAREA_CALL MAREA_G_TypePartitionPart(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    const MAREA_SemaExprType VALUE = MAREA_SemaTypedAstTypeOfChild(built, 0);

    MAREA_SemaExprType element = { .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };

    if (MAREA_SemaTypedAstElementOf(context, &VALUE, MAREA_BOOLEAN_FALSE, &element) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypedAstExcept
RENDERED
= MAREA_SemaTypedAstRenderExprType(context, &element, &text, &length);

if ((RENDERED != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
    return
RENDERED
;
}

MAREA_Null memory = MAREA_NULLPTR;

if (MAREA_ArenaAllocate(context->arena, (length + MAREA_INTEGER_SIZE(7)), MAREA_INTEGER_SIZE(1), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
    return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
}

MAREA_Character8 *buffer = memory;

memcpy(buffer, "slice<", MAREA_INTEGER_SIZE(6));
memcpy((buffer + MAREA_INTEGER_SIZE(6)), text, length);

buffer[(length + MAREA_INTEGER_SIZE(6))] = '>';

const MAREA_SemaExprType PART = MAREA_G_TypeOfFixedText(buffer, (length + MAREA_INTEGER_SIZE(7)));

return MAREA_G_BindType(context, MAREA_SemaChildAt(built->astNode, 2), &PART);
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstTypeScopeNames(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built, MAREA_IntegerSize index)
{
    if ((context == MAREA_NULLPTR) || (built == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: each statement is typed once, right after the last header child that decides the type of its name: the last bound, the collection, the second child of a partition, the expression of a with.
    switch (MAREA_AstNodeGetKind(built->astNode)) {
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return (index == 2) ? MAREA_G_TypeRangeCounter(context, built) : MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        return ((built->childCount >= MAREA_G_FOR_EACH_TAIL) && (index == (built->childCount - MAREA_G_FOR_EACH_TAIL))) ? MAREA_G_TypeForEachNames(context, built) : MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return (index == 1) ? MAREA_G_TypePartitionPart(context, built) : MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    case MAREA_AST_KIND_WITH_STATEMENT:
    {
        if (index != 0) {
            return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
        }

        const MAREA_SemaExprType EXPRESSION = MAREA_SemaTypedAstTypeOfChild(built, 0);

        return MAREA_G_BindType(context, MAREA_SemaChildAt(built->astNode, 1), &EXPRESSION);
    }
    default:
        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }
}
