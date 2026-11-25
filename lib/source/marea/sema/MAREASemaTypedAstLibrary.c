// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: one library type this rule reads: its canonical namespace, the full written path of the type, which is
// what the signature function of the resolver is asked with, and the generic parameter names sema recognizes
// inside a signature's result text: a result spelled with one of them stands for the receiver's own type
// argument at that place, read in the receiver's frame. A registered type takes its facts from its descriptor,
// never from its name: the names are positional, Item then Count.
typedef struct MAREA_G_LibraryShape
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize nameLength;
    const MAREA_Character8 *namespaceText;
    MAREA_IntegerSize namespaceLength;
    const MAREA_Character8 *const *parameters;
    MAREA_IntegerSize parameterCount;
} MAREA_G_LibraryShape;

static const MAREA_Character8 *const MAREA_G_POSITIONAL_PARAMETERS[2] = { "Item", "Count" };

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

// INFO: a NAMED_TYPE or a GENERIC_NAME names a registered type when the registry knows its full written path.
// Gives the shape and the TYPE_ARGUMENTS node of the receiver, or false when the registry does not know it.
static MAREA_Boolean MAREA_CALL MAREA_G_LibraryHeadOf(MAREA_SemaContext *context, const MAREA_AstNode *typeExpression, MAREA_G_LibraryShape *outShape, const MAREA_AstNode **outArguments)
{
    *outArguments = MAREA_NULLPTR;

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if (MAREA_SemaDescriptorOfType(context, typeExpression, &descriptor) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_Character8 *joined = MAREA_NULLPTR;

    MAREA_IntegerSize joinedLength = 0;

    if (MAREA_SemaQualifiedNameJoin(context, MAREA_SemaChildAt(typeExpression, 0), &joined, &joinedLength) != MAREA_SEMA_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    outShape->name = descriptor->name;
    outShape->nameLength = descriptor->nameLength;
    outShape->namespaceText = joined;
    outShape->namespaceLength = joinedLength;
    outShape->parameters = MAREA_G_POSITIONAL_PARAMETERS;
    outShape->parameterCount = (descriptor->typeParameterCount < MAREA_INTEGER_SIZE(2)) ? descriptor->typeParameterCount : MAREA_INTEGER_SIZE(2);

    *outArguments = MAREA_G_FindChild(typeExpression, MAREA_AST_KIND_TYPE_ARGUMENTS);

    return MAREA_BOOLEAN_TRUE;
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

    MAREA_G_LibraryShape shape;

    if (MAREA_G_LibraryHeadOf(context, base, &shape, &arguments) == MAREA_BOOLEAN_FALSE) {
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

        if (MAREA_G_LibraryHeadOf(context, peeled.node, &shape, &arguments) == MAREA_BOOLEAN_FALSE) {
            return MAREA_G_TypeNone();
        }

        receiver = peeled.node;

        frame = peeled.frame;
    }

    const MAREA_SemaSignature *signature = MAREA_G_QuerySignature(context, &shape, member);

    if (signature == MAREA_NULLPTR) {
        return MAREA_G_TypeNone();
    }

    return MAREA_G_SubstituteResult(&shape, receiver, arguments, frame, signature);
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstTableValueOf(const MAREA_SemaContext *context, const MAREA_SemaExprType *collection, MAREA_SemaExprType *outValue)
{
    *outValue = MAREA_G_TypeNone();

    MAREA_SemaExprType peeled = MAREA_G_TypeNone();

    if (MAREA_SemaTypedAstPeelToNamedType(context, collection, &peeled) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    // INFO: a keyed table is a registered type with two type parameters and no index protocol: the second
    // argument is the value.
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if ((MAREA_SemaDescriptorOfType(context, peeled.node, &descriptor) == MAREA_BOOLEAN_FALSE) || (descriptor->typeParameterCount != MAREA_INTEGER_SIZE(2)) || (descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *arguments = MAREA_G_FindChild(peeled.node, MAREA_AST_KIND_TYPE_ARGUMENTS);

    if ((arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(arguments) < 2)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *value = MAREA_SemaChildAt(arguments, 1);

    if (MAREA_G_IsType(value) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outValue = MAREA_G_TypeOfNode(value, peeled.frame);

    return MAREA_BOOLEAN_TRUE;
}
