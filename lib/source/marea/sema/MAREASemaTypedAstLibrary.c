// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the libraries this rule knows, each with the canonical "c.<name>" namespace the signature function of the
// resolver is asked with, and the generic parameter names sema recognizes inside a signature's result text: a
// result spelled with one of them stands for the receiver's own type argument at that place, read in the receiver's
// frame. array has two (the element, then the fixed size), table has two (the key, then the value), and vector,
// slice, pointer, owned and task have one (the element).
typedef struct MAREA_G_LibraryShape
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize nameLength;
    const MAREA_Character8 *namespaceText;
    MAREA_IntegerSize namespaceLength;
    const MAREA_Character8 *const *parameters;
    MAREA_IntegerSize parameterCount;
} MAREA_G_LibraryShape;

static const MAREA_Character8 *const MAREA_G_ARRAY_PARAMETERS[2] = { "Item", "Count" };
static const MAREA_Character8 *const MAREA_G_ONE_PARAMETER[1] = { "Item" };
static const MAREA_Character8 *const MAREA_G_TABLE_PARAMETERS[2] = { "Key", "Value" };

static const MAREA_G_LibraryShape MAREA_G_LIBRARY_SHAPES[7] = {
    { "array", MAREA_INTEGER_SIZE(5), "c.array", MAREA_INTEGER_SIZE(7), MAREA_G_ARRAY_PARAMETERS, MAREA_INTEGER_SIZE(2) },
    { "vector", MAREA_INTEGER_SIZE(6), "c.vector", MAREA_INTEGER_SIZE(8), MAREA_G_ONE_PARAMETER, MAREA_INTEGER_SIZE(1) },
    { "slice", MAREA_INTEGER_SIZE(5), "c.slice", MAREA_INTEGER_SIZE(7), MAREA_G_ONE_PARAMETER, MAREA_INTEGER_SIZE(1) },
    { "pointer", MAREA_INTEGER_SIZE(7), "c.pointer", MAREA_INTEGER_SIZE(9), MAREA_G_ONE_PARAMETER, MAREA_INTEGER_SIZE(1) },
    { "owned", MAREA_INTEGER_SIZE(5), "c.owned", MAREA_INTEGER_SIZE(7), MAREA_G_ONE_PARAMETER, MAREA_INTEGER_SIZE(1) },
    { "task", MAREA_INTEGER_SIZE(4), "c.task", MAREA_INTEGER_SIZE(6), MAREA_G_ONE_PARAMETER, MAREA_INTEGER_SIZE(1) },
    { "table", MAREA_INTEGER_SIZE(5), "c.table", MAREA_INTEGER_SIZE(7), MAREA_G_TABLE_PARAMETERS, MAREA_INTEGER_SIZE(2) },
};

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_SemaTextEquals(text, length, expected, strlen(expected));
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if (MAREA_G_IsKind(child, kind) == MAREA_BOOLEAN_TRUE) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static const MAREA_G_LibraryShape *MAREA_CALL MAREA_G_FindShape(const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = (sizeof(MAREA_G_LIBRARY_SHAPES) / sizeof(MAREA_G_LIBRARY_SHAPES[0]));

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_G_LibraryShape *shape = &MAREA_G_LIBRARY_SHAPES[index];

        if (MAREA_G_TextIs(name, nameLength, shape->name) == MAREA_BOOLEAN_TRUE) {
            return shape;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a NAMED_TYPE or a GENERIC_NAME names a library, bare or under the reserved namespace c, when its qualified
// name has one segment (the library name) or two with the first "c". Gives the shape and the TYPE_ARGUMENTS node of
// the receiver, or null when the receiver gives none.
static const MAREA_G_LibraryShape *MAREA_CALL MAREA_G_LibraryHeadOf(const MAREA_AstNode *typeExpression, const MAREA_AstNode **outArguments)
{
    *outArguments = MAREA_NULLPTR;

    if ((MAREA_G_IsKind(typeExpression, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsKind(typeExpression, MAREA_AST_KIND_GENERIC_NAME) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *qualified = MAREA_SemaChildAt(typeExpression, 0);

    const MAREA_IntegerSize SEGMENTS = (MAREA_G_IsKind(qualified, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_TRUE) ? MAREA_AstNodeGetChildCount(qualified) : MAREA_INTEGER_SIZE(0);

    if ((SEGMENTS < 1) || (SEGMENTS > 2)) {
        return MAREA_NULLPTR;
    }

    // INFO: a library is named bare, or under one of the two reserved first segments.
    if ((SEGMENTS == 2) && (MAREA_G_TextIs(MAREA_AstNodeGetText(MAREA_SemaChildAt(qualified, 0)), MAREA_AstNodeGetTextLength(MAREA_SemaChildAt(qualified, 0)), "c") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(MAREA_AstNodeGetText(MAREA_SemaChildAt(qualified, 0)), MAREA_AstNodeGetTextLength(MAREA_SemaChildAt(qualified, 0)), "marea") == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *last = MAREA_SemaChildAt(qualified, (SEGMENTS - 1));

    const MAREA_G_LibraryShape *shape = MAREA_G_FindShape(MAREA_AstNodeGetText(last), MAREA_AstNodeGetTextLength(last));

    if (shape == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    *outArguments = MAREA_G_FindChild(typeExpression, MAREA_AST_KIND_TYPE_ARGUMENTS);

    return shape;
}

// INFO: asks the resolver's optional signature function for one member of the library's own canonical namespace; a
// resolver with none, or an answer other than FOUND, gives null, exactly as an EXTERNAL call reads it.
static const MAREA_SemaSignature *MAREA_CALL MAREA_G_QuerySignature(const MAREA_SemaContext *context, const MAREA_G_LibraryShape *shape, const MAREA_AstNode *member)
{
    const MAREA_SemaImportResolver *owner = MAREA_SemaResolverOf(context, shape->namespaceText, shape->namespaceLength);

    if (owner == MAREA_NULLPTR) {
        owner = context->resolver;
    }

    if ((owner == MAREA_NULLPTR) || (owner->signature == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    MAREA_SemaImportResult result = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    const MAREA_SemaSignature *signature = MAREA_NULLPTR;

    owner->signature(owner->userData, shape->namespaceText, shape->namespaceLength, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member), &result, &signature);

    return (result == MAREA_SEMA_IMPORT_RESULT_FOUND) ? signature : MAREA_NULLPTR;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfNode(const MAREA_AstNode *node, const MAREA_SemaTypeFrame *frame)
{
    return (MAREA_SemaExprType){ .node = node, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = frame, .fixed = MAREA_NULLPTR, .fixedLength = 0 };
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    return (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = text, .fixedLength = length };
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeNone(void)
{
    return MAREA_G_TypeOfText(MAREA_NULLPTR, 0);
}

// INFO: "Self" gives the receiver's own type; one of the library's own generic parameter names gives the receiver's
// type argument at that place, read in the receiver's frame; any other spelling is the literal result text.
static MAREA_SemaExprType MAREA_CALL MAREA_G_SubstituteResult(const MAREA_G_LibraryShape *shape, const MAREA_AstNode *receiver, const MAREA_AstNode *arguments, const MAREA_SemaTypeFrame *frame, const MAREA_SemaSignature *signature)
{
    if (MAREA_G_TextIs(signature->resultTypeText, signature->resultTypeTextLength, "Self") == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_TypeOfNode(receiver, frame);
    }

    for (MAREA_IntegerSize index = 0; index < shape->parameterCount; ++index) {
        if (MAREA_G_TextIs(signature->resultTypeText, signature->resultTypeTextLength, shape->parameters[index]) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        if ((arguments == MAREA_NULLPTR) || (index >= MAREA_AstNodeGetChildCount(arguments))) {
            return MAREA_G_TypeNone();
        }

        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        return (MAREA_G_IsType(argument) == MAREA_BOOLEAN_TRUE) ? MAREA_G_TypeOfNode(argument, frame) : MAREA_G_TypeNone();
    }

    return MAREA_G_TypeOfText(signature->resultTypeText, signature->resultTypeTextLength);
}

MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstLibraryCallType(MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    const MAREA_AstNode *call = built->astNode;

    const MAREA_AstNode *callee = (call == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(call, 0);

    if (MAREA_G_IsKind(callee, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_TypeNone();
    }

    const MAREA_AstNode *base = MAREA_SemaChildAt(callee, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(callee, 1);

    if (MAREA_G_IsKind(member, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_TypeNone();
    }

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    const MAREA_AstNode *receiver = base;

    const MAREA_SemaTypeFrame *frame = MAREA_NULLPTR;

    const MAREA_G_LibraryShape *shape = MAREA_G_LibraryHeadOf(base, &arguments);

    if (shape == MAREA_NULLPTR) {
        // INFO: not a static call on the type itself; read the already-typed base value of the member access and
        // peel it down to a plain NAMED_TYPE, the way an array's element is read before MAREA_SemaTypedAstElementOf
        // looks at the type arguments.
        MAREA_SemaTypedNode *memberNode = ((built->children == MAREA_NULLPTR) || (built->childCount == 0)) ? MAREA_NULLPTR : built->children[0];

        if (memberNode == MAREA_NULLPTR) {
            return MAREA_G_TypeNone();
        }

        MAREA_SemaExprType baseType = MAREA_SemaTypedAstTypeOfChild(memberNode, 0);

        MAREA_SemaExprType peeled = MAREA_G_TypeNone();

        if (MAREA_SemaTypedAstPeelToNamedType(context, &baseType, &peeled) == MAREA_BOOLEAN_FALSE) {
            return MAREA_G_TypeNone();
        }

        shape = MAREA_G_LibraryHeadOf(peeled.node, &arguments);

        if (shape == MAREA_NULLPTR) {
            return MAREA_G_TypeNone();
        }

        receiver = peeled.node;

        frame = peeled.frame;
    }

    const MAREA_SemaSignature *signature = MAREA_G_QuerySignature(context, shape, member);

    if (signature == MAREA_NULLPTR) {
        return MAREA_G_TypeNone();
    }

    return MAREA_G_SubstituteResult(shape, receiver, arguments, frame, signature);
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstTableValueOf(const MAREA_SemaContext *context, const MAREA_SemaExprType *collection, MAREA_SemaExprType *outValue)
{
    *outValue = MAREA_G_TypeNone();

    MAREA_SemaExprType peeled = MAREA_G_TypeNone();

    if (MAREA_SemaTypedAstPeelToNamedType(context, collection, &peeled) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    const MAREA_G_LibraryShape *shape = MAREA_G_LibraryHeadOf(peeled.node, &arguments);

    if ((shape == MAREA_NULLPTR) || (MAREA_G_TextIs(shape->name, shape->nameLength, "table") == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(arguments) < 2)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *value = MAREA_SemaChildAt(arguments, 1);

    if (MAREA_G_IsType(value) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outValue = MAREA_G_TypeOfNode(value, peeled.frame);

    return MAREA_BOOLEAN_TRUE;
}
