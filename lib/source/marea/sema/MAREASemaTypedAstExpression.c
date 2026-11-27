// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: a chain of type aliases or of generic arguments that is followed this many steps stops there, the same limit as the text of a declared type.
#define MAREA_G_EXPRESSION_PEEL_LIMIT MAREA_INTEGER_SIZE(16)

typedef struct MAREA_G_Resolver
{
    MAREA_SemaContext *context;
    MAREA_Boolean outOfMemory;
} MAREA_G_Resolver;

// INFO: a structure or an enum, with the type arguments that fill it and the frame those arguments are written in.
typedef struct MAREA_G_Nominal
{
    const MAREA_AstNode *declaration;
    const MAREA_AstNode *arguments;
    const MAREA_SemaTypeFrame *frame;
} MAREA_G_Nominal;

typedef enum MAREA_G_Peeled
{
    MAREA_G_PEELED_NONE = MAREA_UINTEGER_8(0),
    MAREA_G_PEELED_NODE = MAREA_UINTEGER_8(1),
    MAREA_G_PEELED_NOMINAL = MAREA_UINTEGER_8(2),
} MAREA_G_Peeled;

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeNone(void)
{
    return (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfNode(const MAREA_AstNode *node, const MAREA_SemaTypeFrame *frame)
{
    MAREA_SemaExprType type = MAREA_G_TypeNone();

    type.node = node;
    type.frame = frame;

    return type;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfDeclaration(const MAREA_AstNode *declaration, const MAREA_AstNode *arguments, const MAREA_SemaTypeFrame *frame)
{
    MAREA_SemaExprType type = MAREA_G_TypeNone();

    type.declaration = declaration;
    type.arguments = arguments;
    type.frame = frame;

    return type;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfText(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_SemaExprType type = MAREA_G_TypeNone();

    type.fixed = text;
    type.fixedLength = length;

    return type;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TypeIsKnown(const MAREA_SemaExprType *type)
{
    return ((type->node != MAREA_NULLPTR) || (type->declaration != MAREA_NULLPTR) || (type->fixed != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_SemaTextEquals(text, length, expected, strlen(expected));
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

// INFO: the one identifier of a QUALIFIED_NAME with a single segment, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_SingleSegment(const MAREA_AstNode *qualified)
{
    if ((MAREA_G_IsKind(qualified, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetChildCount(qualified) != 1)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *segment = MAREA_SemaChildAt(qualified, 0);

    return (MAREA_G_IsKind(segment, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) ? segment : MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsTypeDeclaration(const MAREA_AstNode *declaration)
{
    return ((MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a name of the file that stands for a declaration, never one that a local or a parameter hides.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindFileDeclaration(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    if (MAREA_SemaLocalFind(context, text, length) != MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, length);

    return ((entry != MAREA_NULLPTR) && (entry->kind == MAREA_SEMA_NAME_KIND_DECLARATION)) ? entry->declaration : MAREA_NULLPTR;
}

static MAREA_SemaTypeFrame *MAREA_CALL MAREA_G_NewFrame(MAREA_G_Resolver *resolver)
{
    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(resolver->context->arena, sizeof(MAREA_SemaTypeFrame), MAREA_ALIGNOF(MAREA_SemaTypeFrame), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        resolver->outOfMemory = MAREA_BOOLEAN_TRUE;

        return MAREA_NULLPTR;
    }

    MAREA_SemaTypeFrame *frame = memory;

    *frame = (MAREA_SemaTypeFrame){ .parameters = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .selfDeclaration = MAREA_NULLPTR, .selfArguments = MAREA_NULLPTR, .argumentFrame = MAREA_NULLPTR, .next = MAREA_NULLPTR };

    return frame;
}

// INFO: a function with generic parameters gets a frame with the type arguments written at the call, or none; a function with no generic parameters keeps the frame of the block that holds it.
static const MAREA_SemaTypeFrame *MAREA_CALL MAREA_G_FunctionFrame(MAREA_G_Resolver *resolver, const MAREA_AstNode *function, const MAREA_SemaTypeFrame *outer, const MAREA_AstNode *callTypeArguments)
{
    const MAREA_AstNode *parameters = MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS);

    if (parameters == MAREA_NULLPTR) {
        return outer;
    }

    MAREA_SemaTypeFrame *frame = MAREA_G_NewFrame(resolver);

    if (frame == MAREA_NULLPTR) {
        return outer;
    }

    frame->parameters = parameters;
    frame->arguments = callTypeArguments;
    frame->next = outer;

    return frame;
}

// INFO: a structure with generic parameters is read with the type arguments of the value that holds it.
static const MAREA_SemaTypeFrame *MAREA_CALL MAREA_G_StructureFrame(MAREA_G_Resolver *resolver, const MAREA_G_Nominal *receiver)
{
    const MAREA_AstNode *parameters = MAREA_G_FindChild(receiver->declaration, MAREA_AST_KIND_GENERIC_PARAMETERS);

    if (parameters == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    MAREA_SemaTypeFrame *frame = MAREA_G_NewFrame(resolver);

    if (frame == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    frame->parameters = parameters;
    frame->arguments = receiver->arguments;
    frame->selfDeclaration = receiver->declaration;
    frame->selfArguments = receiver->arguments;
    frame->argumentFrame = receiver->frame;

    return frame;
}

// INFO: the generic parameters of an implementation block take the type arguments of the receiver only when the block names its type as the same parameters in the same order, as every block of the examples does.
static MAREA_Boolean MAREA_CALL MAREA_G_ImplementationNamesItsParameters(const MAREA_AstNode *implementation, const MAREA_AstNode *parameters)
{
    const MAREA_AstNode *named = MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE);

    const MAREA_AstNode *arguments = (named == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(named, MAREA_AST_KIND_TYPE_ARGUMENTS);

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(parameters);

    if ((arguments == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(arguments) != COUNT)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        const MAREA_AstNode *identifier = (MAREA_G_IsKind(argument, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) ? MAREA_G_SingleSegment(MAREA_SemaChildAt(argument, 0)) : MAREA_NULLPTR;

        if ((identifier == MAREA_NULLPTR) || (MAREA_G_FindChild(argument, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR) || (MAREA_SemaGenericParameterIndex(parameters, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier)) != index)) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

static const MAREA_SemaTypeFrame *MAREA_CALL MAREA_G_ImplementationFrame(MAREA_G_Resolver *resolver, const MAREA_AstNode *implementation, const MAREA_G_Nominal *receiver)
{
    const MAREA_AstNode *parameters = MAREA_G_FindChild(implementation, MAREA_AST_KIND_GENERIC_PARAMETERS);

    MAREA_SemaTypeFrame *frame = MAREA_G_NewFrame(resolver);

    if (frame == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    frame->selfDeclaration = receiver->declaration;
    frame->selfArguments = receiver->arguments;
    frame->argumentFrame = receiver->frame;

    if (parameters != MAREA_NULLPTR) {
        frame->parameters = parameters;
        frame->arguments = (MAREA_G_ImplementationNamesItsParameters(implementation, parameters) == MAREA_BOOLEAN_TRUE) ? receiver->arguments : MAREA_NULLPTR;
    }

    return frame;
}

// INFO: the function or the constant that an implementation block of the file gives to a structure or an enum, or null. The implementation of a type is always in the file of the type.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindImplementationMember(const MAREA_SemaContext *context, const MAREA_AstNode *declaration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_AstNode **outImplementation)
{
    const MAREA_IntegerSize ITEM_COUNT = (context->root == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(context->root);

    for (MAREA_IntegerSize item = 0; item < ITEM_COUNT; ++item) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, item);

        if (MAREA_G_IsKind(implementation, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        const MAREA_AstNode *head = MAREA_G_SingleSegment(MAREA_SemaChildAt(MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE), 0));

        if ((head == MAREA_NULLPTR) || (MAREA_G_FindFileDeclaration(context, MAREA_AstNodeGetText(head), MAREA_AstNodeGetTextLength(head)) != declaration)) {
            continue;
        }

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_AstNodeGetChildCount(implementation);

        for (MAREA_IntegerSize member = 0; member < MEMBER_COUNT; ++member) {
            const MAREA_AstNode *candidate = MAREA_SemaChildAt(implementation, member);

            const MAREA_Character8 *candidateName = MAREA_NULLPTR;

            MAREA_IntegerSize candidateLength = 0;

            if (((MAREA_G_IsKind(candidate, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(candidate, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE)) && (MAREA_SemaDeclarationName(candidate, &candidateName, &candidateLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(candidateName, candidateLength, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
                *outImplementation = implementation;

                return candidate;
            }
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the type of a typed child as the child stored it, or the text a literal child was given.
static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfChild(const MAREA_SemaTypedNode *built, MAREA_IntegerSize index)
{
    if ((built->children == MAREA_NULLPTR) || (index >= built->childCount) || (built->children[index] == MAREA_NULLPTR)) {
        return MAREA_G_TypeNone();
    }

    const MAREA_SemaTypedNode *child = built->children[index];

    if (MAREA_G_TypeIsKnown(&child->exprType) == MAREA_BOOLEAN_TRUE) {
        return child->exprType;
    }

    return (child->resolvedType == MAREA_NULLPTR) ? MAREA_G_TypeNone() : MAREA_G_TypeOfText(child->resolvedType, child->resolvedTypeLength);
}

// INFO: follows a type to the first name that is not a generic parameter that a frame fills, Self or a local alias: a structure or an enum, or any other type node (a built-in, an import).
static MAREA_G_Peeled MAREA_CALL MAREA_G_Peel(const MAREA_SemaContext *context, MAREA_SemaExprType *type, MAREA_G_Nominal *outNominal)
{
    for (MAREA_IntegerSize step = 0; step < MAREA_G_EXPRESSION_PEEL_LIMIT; ++step) {
        if (type->declaration != MAREA_NULLPTR) {
            *outNominal = (MAREA_G_Nominal){ .declaration = type->declaration, .arguments = type->arguments, .frame = type->frame };

            return MAREA_G_PEELED_NOMINAL;
        }

        if ((type->fixed != MAREA_NULLPTR) || (MAREA_G_IsKind(type->node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_FALSE)) {
            return (MAREA_G_IsType(type->node) == MAREA_BOOLEAN_TRUE) ? MAREA_G_PEELED_NODE : MAREA_G_PEELED_NONE;
        }

        const MAREA_AstNode *head = MAREA_G_SingleSegment(MAREA_SemaChildAt(type->node, 0));

        if (head == MAREA_NULLPTR) {
            return MAREA_G_PEELED_NODE;
        }

        const MAREA_Character8 *name = MAREA_AstNodeGetText(head);

        const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(head);

        const MAREA_AstNode *arguments = MAREA_G_FindChild(type->node, MAREA_AST_KIND_TYPE_ARGUMENTS);

        if (arguments == MAREA_NULLPTR) {
            MAREA_Boolean replaced = MAREA_BOOLEAN_FALSE;

            for (const MAREA_SemaTypeFrame *frame = type->frame; ((frame != MAREA_NULLPTR) && (replaced == MAREA_BOOLEAN_FALSE)); frame = frame->next) {
                const MAREA_IntegerSize INDEX = MAREA_SemaGenericParameterIndex(frame->parameters, name, NAME_LENGTH);

                if ((frame->selfDeclaration != MAREA_NULLPTR) && (MAREA_G_TextIs(name, NAME_LENGTH, "Self") == MAREA_BOOLEAN_TRUE)) {
                    *outNominal = (MAREA_G_Nominal){ .declaration = frame->selfDeclaration, .arguments = frame->selfArguments, .frame = frame->argumentFrame };

                    return MAREA_G_PEELED_NOMINAL;
                }

                if ((frame->parameters == MAREA_NULLPTR) || (INDEX >= MAREA_AstNodeGetChildCount(frame->parameters))) {
                    continue;
                }

                if ((frame->arguments == MAREA_NULLPTR) || (INDEX >= MAREA_AstNodeGetChildCount(frame->arguments))) {
                    return MAREA_G_PEELED_NONE;
                }

                *type = MAREA_G_TypeOfNode(MAREA_SemaChildAt(frame->arguments, INDEX), frame->argumentFrame);

                replaced = MAREA_BOOLEAN_TRUE;
            }

            if (replaced == MAREA_BOOLEAN_TRUE) {
                continue;
            }

            // INFO: Self inside an implementation block is the type that the block names.
            if ((MAREA_G_TextIs(name, NAME_LENGTH, "Self") == MAREA_BOOLEAN_TRUE) && (context->implementation != MAREA_NULLPTR)) {
                *type = MAREA_G_TypeOfNode(MAREA_G_FindChild(context->implementation, MAREA_AST_KIND_NAMED_TYPE), MAREA_NULLPTR);

                continue;
            }
        }

        // INFO: a name written in another file is not looked up among the names of this file.
        if (MAREA_SemaTypeFrameFindForeign(type->frame) != MAREA_NULLPTR) {
            return MAREA_G_PEELED_NODE;
        }

        if (MAREA_SemaIsGenericInScope(context, name, NAME_LENGTH) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_PEELED_NODE;
        }

        const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, name, NAME_LENGTH);

        if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
            return MAREA_G_PEELED_NODE;
        }

        if (MAREA_G_IsTypeDeclaration(entry->declaration) == MAREA_BOOLEAN_TRUE) {
            *outNominal = (MAREA_G_Nominal){ .declaration = entry->declaration, .arguments = arguments, .frame = type->frame };

            return MAREA_G_PEELED_NOMINAL;
        }

        // INFO: only an alias with no generic parameters, used with no arguments, is followed; the target is written in the scope of the file.
        if ((MAREA_G_IsKind(entry->declaration, MAREA_AST_KIND_TYPE_DECLARATION) == MAREA_BOOLEAN_TRUE) && (arguments == MAREA_NULLPTR) && (MAREA_G_FindChild(entry->declaration, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_NULLPTR)) {
            const MAREA_AstNode *target = MAREA_SemaChildAt(entry->declaration, (MAREA_AstNodeGetChildCount(entry->declaration) - 1));

            if (MAREA_G_IsType(target) == MAREA_BOOLEAN_TRUE) {
                *type = MAREA_G_TypeOfNode(target, MAREA_NULLPTR);

                continue;
            }
        }

        return MAREA_G_PEELED_NODE;
    }

    return MAREA_G_PEELED_NONE;
}

// INFO: a member or an element is read through an owner-role wrapper, because the owner of a value is
// named as the value itself. A registered type is read by its role, never by its name.
static MAREA_G_Peeled MAREA_CALL MAREA_G_PeelOwned(const MAREA_SemaContext *context, MAREA_SemaExprType *type, MAREA_G_Nominal *outNominal)
{
    for (MAREA_IntegerSize step = 0; step < MAREA_G_EXPRESSION_PEEL_LIMIT; ++step) {
        const MAREA_G_Peeled PEELED = MAREA_G_Peel(context, type, outNominal);

        if ((PEELED != MAREA_G_PEELED_NODE) || (MAREA_G_IsKind(type->node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_FALSE)) {
            return PEELED;
        }

        const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

        if ((MAREA_SemaDescriptorOfType(context, type->node, &descriptor) == MAREA_BOOLEAN_FALSE) || ((descriptor->roles & MAREA_TYPE_ROLE_OWNER) == MAREA_TYPE_ROLE_NONE)) {
            return PEELED;
        }

        const MAREA_AstNode *arguments = MAREA_G_FindChild(type->node, MAREA_AST_KIND_TYPE_ARGUMENTS);

        if ((arguments == MAREA_NULLPTR) || (MAREA_G_IsType(MAREA_SemaChildAt(arguments, 0)) == MAREA_BOOLEAN_FALSE)) {
            return PEELED;
        }

        *type = MAREA_G_TypeOfNode(MAREA_SemaChildAt(arguments, 0), type->frame);
    }

    return MAREA_G_PEELED_NONE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstPeelToNamedType(const MAREA_SemaContext *context, const MAREA_SemaExprType *type, MAREA_SemaExprType *outPeeled)
{
    *outPeeled = *type;

    MAREA_G_Nominal ignored = { .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR };

    return (MAREA_G_PeelOwned(context, outPeeled, &ignored) == MAREA_G_PEELED_NODE) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a function or a constant of an implementation block, read for the receiver that holds it.
static MAREA_SemaExprType MAREA_CALL MAREA_G_ImplementationMemberType(MAREA_G_Resolver *resolver, const MAREA_G_Nominal *receiver, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, const MAREA_AstNode *callTypeArguments)
{
    const MAREA_AstNode *implementation = MAREA_NULLPTR;

    const MAREA_AstNode *member = MAREA_G_FindImplementationMember(resolver->context, receiver->declaration, name, nameLength, &implementation);

    if (member == MAREA_NULLPTR) {
        return MAREA_G_TypeNone();
    }

    const MAREA_SemaTypeFrame *frame = MAREA_G_ImplementationFrame(resolver, implementation, receiver);

    if (MAREA_G_IsKind(member, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_TypeOfNode(MAREA_SemaChildAt(member, (MAREA_AstNodeGetChildCount(member) - 2)), frame);
    }

    return MAREA_G_TypeOfNode(member, MAREA_G_FunctionFrame(resolver, member, frame, callTypeArguments));
}

// INFO: the first field with the name among the variants of an enum, or null. The type is the first type child; a default value may follow it.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindVariantField(const MAREA_AstNode *enumeration, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(enumeration);

    for (MAREA_IntegerSize variantIndex = 0; variantIndex < COUNT; ++variantIndex) {
        const MAREA_AstNode *variant = MAREA_SemaChildAt(enumeration, variantIndex);

        const MAREA_IntegerSize FIELD_COUNT = (MAREA_G_IsKind(variant, MAREA_AST_KIND_VARIANT_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_AstNodeGetChildCount(variant) : MAREA_INTEGER_SIZE(0);

        for (MAREA_IntegerSize fieldIndex = 0; fieldIndex < FIELD_COUNT; ++fieldIndex) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(variant, fieldIndex);

            const MAREA_AstNode *fieldName = (MAREA_G_IsKind(field, MAREA_AST_KIND_VARIANT_FIELD) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((fieldName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), name, nameLength) == MAREA_BOOLEAN_TRUE)) {
                return field;
            }
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a field of a structure, or a function or a constant of one of its implementation blocks.
static MAREA_SemaExprType MAREA_CALL MAREA_G_MemberType(MAREA_G_Resolver *resolver, const MAREA_G_Nominal *receiver, const MAREA_AstNode *member, const MAREA_AstNode *callTypeArguments)
{
    const MAREA_Character8 *name = MAREA_AstNodeGetText(member);

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(member);

    if (MAREA_G_IsKind(receiver->declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) {
        const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(receiver->declaration);

        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *field = MAREA_SemaChildAt(receiver->declaration, index);

            const MAREA_AstNode *fieldName = (MAREA_G_IsKind(field, MAREA_AST_KIND_FIELD_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((fieldName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), name, NAME_LENGTH) == MAREA_BOOLEAN_TRUE)) {
                const MAREA_AstNode *type = MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1));

                return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? MAREA_G_TypeOfNode(type, MAREA_G_StructureFrame(resolver, receiver)) : MAREA_G_TypeNone();
            }
        }
    }

    // INFO: a field of a variant is read on the enum value itself; the first variant that declares the name gives the type.
    if (MAREA_G_IsKind(receiver->declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *field = MAREA_G_FindVariantField(receiver->declaration, name, NAME_LENGTH);

        if (field != MAREA_NULLPTR) {
            const MAREA_AstNode *type = MAREA_SemaChildAt(field, 1);

            return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? MAREA_G_TypeOfNode(type, MAREA_NULLPTR) : MAREA_G_TypeNone();
        }
    }

    return MAREA_G_ImplementationMemberType(resolver, receiver, name, NAME_LENGTH, callTypeArguments);
}

// INFO: a variant of an enum, read on the name of the enum: the value has the type of the enum.
static MAREA_Boolean MAREA_CALL MAREA_G_EnumHasVariant(const MAREA_AstNode *enumeration, const MAREA_AstNode *member)
{
    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(enumeration);

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *variant = MAREA_SemaChildAt(enumeration, index);

        const MAREA_AstNode *variantName = (MAREA_G_IsKind(variant, MAREA_AST_KIND_VARIANT_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(variant, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

        if ((variantName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(variantName), MAREA_AstNodeGetTextLength(variantName), MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the structure or enum that a base names as a type, with no value: a bare name of the file, or a generic name such as Pair<sinteger32, float32>.
static MAREA_Boolean MAREA_CALL MAREA_G_BaseIsTypeName(const MAREA_SemaContext *context, const MAREA_AstNode *base, MAREA_G_Nominal *outNominal)
{
    const MAREA_AstNode *identifier = base;

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    if (MAREA_G_IsKind(base, MAREA_AST_KIND_GENERIC_NAME) == MAREA_BOOLEAN_TRUE) {
        identifier = MAREA_G_SingleSegment(MAREA_SemaChildAt(base, 0));

        arguments = MAREA_G_FindChild(base, MAREA_AST_KIND_TYPE_ARGUMENTS);
    }

    if (MAREA_G_IsKind(identifier, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *declaration = MAREA_G_FindFileDeclaration(context, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier));

    if (MAREA_G_IsTypeDeclaration(declaration) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outNominal = (MAREA_G_Nominal){ .declaration = declaration, .arguments = arguments, .frame = MAREA_NULLPTR };

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the type arguments written on a call, when the node is the callee of that call.
static const MAREA_AstNode *MAREA_CALL MAREA_G_CallTypeArguments(const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    return ((index == 0) && (MAREA_G_IsKind(parent, MAREA_AST_KIND_CALL_EXPRESSION) == MAREA_BOOLEAN_TRUE)) ? MAREA_G_FindChild(parent, MAREA_AST_KIND_TYPE_ARGUMENTS) : MAREA_NULLPTR;
}

// INFO: a value parameter of a function or of an implementation block, such as Count in <constant Count: sinteger32>, is read as a value of its declared type.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindValueParameterType(const MAREA_AstNode *owner, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    const MAREA_AstNode *parameters = (owner == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(owner, MAREA_AST_KIND_GENERIC_PARAMETERS);

    const MAREA_IntegerSize INDEX = MAREA_SemaGenericParameterIndex(parameters, name, nameLength);

    if ((parameters == MAREA_NULLPTR) || (INDEX >= MAREA_AstNodeGetChildCount(parameters))) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, INDEX);

    const MAREA_AstNode *type = MAREA_SemaChildAt(parameter, 1);

    return (((MAREA_AstNodeGetFlags(parameter) & MAREA_AST_FLAG_CONSTANT) != 0) && (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE)) ? type : MAREA_NULLPTR;
}

// INFO: a member (or, with no member, a bare imported name) that comes from a namespace the resolver has a tree for is typed from that tree.
static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfFound(MAREA_G_Resolver *resolver, const MAREA_AstNode *base, const MAREA_AstNode *member, const MAREA_AstNode *callTypeArguments)
{
    MAREA_SemaExprType type = MAREA_G_TypeNone();

    const MAREA_SemaTypedAstExcept RESULT = (member == MAREA_NULLPTR) ? MAREA_SemaTypedAstFoundNameType(resolver->context, base, callTypeArguments, &type) : MAREA_SemaTypedAstFoundMemberType(resolver->context, base, member, callTypeArguments, &type);

    if (RESULT == MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY) {
        resolver->outOfMemory = MAREA_BOOLEAN_TRUE;
    }

    return type;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfIdentifier(MAREA_G_Resolver *resolver, const MAREA_AstNode *identifier, const MAREA_AstNode *callTypeArguments)
{
    const MAREA_SemaContext *context = resolver->context;

    const MAREA_Character8 *name = MAREA_AstNodeGetText(identifier);

    const MAREA_IntegerSize NAME_LENGTH = MAREA_AstNodeGetTextLength(identifier);

    const MAREA_SemaName *local = MAREA_SemaLocalFind(context, name, NAME_LENGTH);

    // INFO: the name of a for, for each, partition or with statement has the type its statement gave it, when the statement could give one.
    if (local != MAREA_NULLPTR) {
        if (local->statementType != MAREA_NULLPTR) {
            return *local->statementType;
        }

        return (MAREA_G_IsType(local->declaration) == MAREA_BOOLEAN_TRUE) ? MAREA_G_TypeOfNode(local->declaration, MAREA_NULLPTR) : MAREA_G_TypeNone();
    }

    const MAREA_AstNode *valueParameter = MAREA_G_FindValueParameterType(context->function, name, NAME_LENGTH);

    if (valueParameter == MAREA_NULLPTR) {
        valueParameter = MAREA_G_FindValueParameterType(context->implementation, name, NAME_LENGTH);
    }

    if (valueParameter != MAREA_NULLPTR) {
        return MAREA_G_TypeOfNode(valueParameter, MAREA_NULLPTR);
    }

    if (context->implementation != MAREA_NULLPTR) {
        const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(context->implementation);

        for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
            const MAREA_AstNode *constant = MAREA_SemaChildAt(context->implementation, index);

            const MAREA_AstNode *constantName = (MAREA_G_IsKind(constant, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(constant, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((constantName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(constantName), MAREA_AstNodeGetTextLength(constantName), name, NAME_LENGTH) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_G_TypeOfNode(MAREA_SemaChildAt(constant, (MAREA_AstNodeGetChildCount(constant) - 2)), MAREA_NULLPTR);
            }
        }
    }

    const MAREA_AstNode *declaration = MAREA_G_FindFileDeclaration(context, name, NAME_LENGTH);

    if (MAREA_G_IsKind(declaration, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_TypeOfNode(MAREA_SemaChildAt(declaration, (MAREA_AstNodeGetChildCount(declaration) - 2)), MAREA_NULLPTR);
    }

    if (MAREA_G_IsKind(declaration, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_TypeOfNode(declaration, MAREA_G_FunctionFrame(resolver, declaration, MAREA_NULLPTR, callTypeArguments));
    }

    return MAREA_G_TypeOfFound(resolver, identifier, MAREA_NULLPTR, callTypeArguments);
}

// INFO: a receiver whose type is a bare generic parameter in scope (no frame bound it to anything concrete) has its
// member looked up through the contracts its where clause lists: the function or implementation block that holds
// the current expression. Any other shape (a frame-bound name, a compound, a name that is not generic) gives none.
static MAREA_SemaExprType MAREA_CALL MAREA_G_ContractMemberType(MAREA_G_Resolver *resolver, const MAREA_SemaExprType *baseType, const MAREA_AstNode *member, const MAREA_AstNode *callTypeArguments)
{
    const MAREA_SemaContext *context = resolver->context;

    if ((baseType->frame != MAREA_NULLPTR) || (MAREA_G_IsKind(baseType->node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_FALSE) || (MAREA_G_FindChild(baseType->node, MAREA_AST_KIND_TYPE_ARGUMENTS) != MAREA_NULLPTR)) {
        return MAREA_G_TypeNone();
    }

    const MAREA_AstNode *identifier = MAREA_G_SingleSegment(MAREA_SemaChildAt(baseType->node, 0));

    if ((identifier == MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, MAREA_AstNodeGetText(identifier), MAREA_AstNodeGetTextLength(identifier)) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_TypeNone();
    }

    const MAREA_Character8 *parameterName = MAREA_AstNodeGetText(identifier);

    const MAREA_IntegerSize PARAMETER_NAME_LENGTH = MAREA_AstNodeGetTextLength(identifier);

    const MAREA_Character8 *memberName = MAREA_AstNodeGetText(member);

    const MAREA_IntegerSize MEMBER_NAME_LENGTH = MAREA_AstNodeGetTextLength(member);

    const MAREA_AstNode *function = MAREA_SemaFindContractMember(context, context->function, parameterName, PARAMETER_NAME_LENGTH, memberName, MEMBER_NAME_LENGTH);

    if (function == MAREA_NULLPTR) {
        function = MAREA_SemaFindContractMember(context, context->implementation, parameterName, PARAMETER_NAME_LENGTH, memberName, MEMBER_NAME_LENGTH);
    }

    if (function == MAREA_NULLPTR) {
        return MAREA_G_TypeNone();
    }

    return MAREA_G_TypeOfNode(function, MAREA_G_FunctionFrame(resolver, function, MAREA_NULLPTR, callTypeArguments));
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfMemberAccess(MAREA_G_Resolver *resolver, const MAREA_SemaTypedNode *built, const MAREA_AstNode *callTypeArguments)
{
    const MAREA_AstNode *base = MAREA_SemaChildAt(built->astNode, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(built->astNode, 1);

    if (MAREA_G_IsKind(member, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_TypeNone();
    }

    MAREA_SemaExprType baseType = MAREA_G_TypeOfChild(built, 0);

    MAREA_G_Nominal receiver = { .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR };

    if (MAREA_G_TypeIsKnown(&baseType) == MAREA_BOOLEAN_TRUE) {
        if (MAREA_G_PeelOwned(resolver->context, &baseType, &receiver) == MAREA_G_PEELED_NOMINAL) {
            return MAREA_G_MemberType(resolver, &receiver, member, callTypeArguments);
        }

        // INFO: a receiver whose type is a generic parameter in scope has no structure or enum to read a member on;
        // the only members it can have are the ones its own where clause promises through a contract.
        return MAREA_G_ContractMemberType(resolver, &baseType, member, callTypeArguments);
    }

    if (MAREA_G_BaseIsTypeName(resolver->context, base, &receiver) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_TypeOfFound(resolver, base, member, callTypeArguments);
    }

    if ((MAREA_G_IsKind(receiver.declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) && (MAREA_G_EnumHasVariant(receiver.declaration, member) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_TypeOfDeclaration(receiver.declaration, receiver.arguments, MAREA_NULLPTR);
    }

    return MAREA_G_ImplementationMemberType(resolver, &receiver, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member), callTypeArguments);
}

// INFO: a registered type with the index protocol gives an element, and one with the raw pointer role too when it is asked for.
static MAREA_Boolean MAREA_CALL MAREA_G_IsElementContainer(const MAREA_SemaContext *context, const MAREA_AstNode *named, MAREA_Boolean withPointer)
{
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if (MAREA_SemaDescriptorOfType(context, named, &descriptor) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE) || ((withPointer == MAREA_BOOLEAN_TRUE) && ((descriptor->roles & MAREA_TYPE_ROLE_RAW_POINTER) != MAREA_TYPE_ROLE_NONE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaTypedAstElementOf(const MAREA_SemaContext *context, const MAREA_SemaExprType *collection, MAREA_Boolean withPointer, MAREA_SemaExprType *outElement)
{
    *outElement = MAREA_G_TypeNone();

    MAREA_SemaExprType baseType = *collection;

    MAREA_G_Nominal ignored = { .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR };

    if ((MAREA_G_PeelOwned(context, &baseType, &ignored) != MAREA_G_PEELED_NODE) || (MAREA_G_IsKind(baseType.node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsElementContainer(context, baseType.node, withPointer) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *arguments = MAREA_G_FindChild(baseType.node, MAREA_AST_KIND_TYPE_ARGUMENTS);

    const MAREA_AstNode *element = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

    if (MAREA_G_IsType(element) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outElement = MAREA_G_TypeOfNode(element, baseType.frame);

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfIndexAccess(MAREA_G_Resolver *resolver, const MAREA_SemaTypedNode *built)
{
    const MAREA_SemaExprType BASE = MAREA_G_TypeOfChild(built, 0);

    MAREA_SemaExprType element = MAREA_G_TypeNone();

    // INFO: the element of a tuple is the one that its constant index names, also through an alias.
    MAREA_SemaExprType peeled = BASE;

    MAREA_G_Nominal ignored = { .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR };

    const MAREA_AstNode *part = ((MAREA_G_PeelOwned(resolver->context, &peeled, &ignored) == MAREA_G_PEELED_NODE) && (MAREA_G_IsKind(peeled.node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE)) ? MAREA_SemaTupleElement(resolver->context, peeled.node, MAREA_SemaChildAt(built->astNode, 1)) : MAREA_NULLPTR;

    if ((part != MAREA_NULLPTR) && (MAREA_G_IsType(part) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_TypeOfNode(part, peeled.frame);
    }

    MAREA_IGNORE(MAREA_SemaTypedAstElementOf(resolver->context, &BASE, MAREA_BOOLEAN_TRUE, &element));

    if (MAREA_G_TypeIsKnown(&element) == MAREA_BOOLEAN_TRUE) {
        return element;
    }

    MAREA_SemaExprType value = MAREA_G_TypeNone();

    MAREA_IGNORE(MAREA_SemaTypedAstTableValueOf(resolver->context, &BASE, &value));

    return value;
}

// INFO: a built-in convert<T>(x) has one type argument and one argument, and the unit binds no name convert.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ConvertTarget(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_TextIs(MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee), "convert") == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    if ((MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee)) != MAREA_NULLPTR) || (MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee)) != MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *typeArguments = MAREA_SemaChildAt(call, 1);

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(call, 2);

    if ((MAREA_G_IsKind(typeArguments, MAREA_AST_KIND_TYPE_ARGUMENTS) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsKind(arguments, MAREA_AST_KIND_ARGUMENT_LIST) == MAREA_BOOLEAN_FALSE) || (MAREA_AstNodeGetChildCount(typeArguments) != 1) || (MAREA_AstNodeGetChildCount(arguments) != 1)) {
        return MAREA_NULLPTR;
    }

    const MAREA_AstNode *target = MAREA_SemaChildAt(typeArguments, 0);

    return (MAREA_G_IsType(target) == MAREA_BOOLEAN_TRUE) ? target : MAREA_NULLPTR;
}

// INFO: a call whose callee names a member of an EXTERNAL namespace the host gave a signature for is typed with its declared result; a resolver with no signature function, or a member it does not know, leaves this empty, exactly as before.
static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfExternalCall(MAREA_G_Resolver *resolver, const MAREA_AstNode *callee)
{
    const MAREA_SemaSignature *signature = MAREA_NULLPTR;

    MAREA_SemaTypedAstExcept result = MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;

    if (MAREA_G_IsKind(callee, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) {
        result = MAREA_SemaTypedAstExternalMemberSignature(resolver->context, MAREA_SemaChildAt(callee, 0), MAREA_SemaChildAt(callee, 1), &signature);
    } else if (MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        result = MAREA_SemaTypedAstExternalNameSignature(resolver->context, callee, &signature);
    }

    if (result == MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY) {
        resolver->outOfMemory = MAREA_BOOLEAN_TRUE;
    }

    return (signature == MAREA_NULLPTR) ? MAREA_G_TypeNone() : MAREA_G_TypeOfText(signature->resultTypeText, signature->resultTypeTextLength);
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfCall(MAREA_G_Resolver *resolver, const MAREA_SemaTypedNode *built)
{
    const MAREA_AstNode *call = built->astNode;

    const MAREA_AstNode *convertTarget = MAREA_G_ConvertTarget(resolver->context, call);

    if (convertTarget != MAREA_NULLPTR) {
        return MAREA_G_TypeOfNode(convertTarget, MAREA_NULLPTR);
    }

    const MAREA_SemaExprType BUILTIN = MAREA_SemaTypedAstBuiltinCallType(resolver->context, call);

    if (MAREA_G_TypeIsKnown(&BUILTIN) == MAREA_BOOLEAN_TRUE) {
        return BUILTIN;
    }

    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    const MAREA_SemaExprType EXTERNAL = MAREA_G_TypeOfExternalCall(resolver, callee);

    if (MAREA_G_TypeIsKnown(&EXTERNAL) == MAREA_BOOLEAN_TRUE) {
        return EXTERNAL;
    }

    const MAREA_SemaExprType LIBRARY = MAREA_SemaTypedAstLibraryCallType(resolver->context, built);

    if (MAREA_G_TypeIsKnown(&LIBRARY) == MAREA_BOOLEAN_TRUE) {
        return LIBRARY;
    }

    // INFO: a call on the name of a structure or an enum is a construction, so the value has that type.
    if (MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *declaration = MAREA_G_FindFileDeclaration(resolver->context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));

        if (MAREA_G_IsTypeDeclaration(declaration) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_TypeOfDeclaration(declaration, MAREA_G_FindChild(call, MAREA_AST_KIND_TYPE_ARGUMENTS), MAREA_NULLPTR);
        }
    }

    MAREA_SemaExprType calleeType = MAREA_G_TypeOfChild(built, 0);

    MAREA_G_Nominal ignored = { .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR };

    // INFO: a value whose type is a local alias of a function type is called through the function type.
    if (MAREA_G_IsKind(calleeType.node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaExprType peeled = calleeType;

        calleeType = (MAREA_G_Peel(resolver->context, &peeled, &ignored) == MAREA_G_PEELED_NODE) ? peeled : calleeType;
    }

    const MAREA_SemaExprType CALLEE = calleeType;

    // INFO: a variant of an enum that has fields is called like a function and gives the enum.
    if ((CALLEE.declaration != MAREA_NULLPTR) && (MAREA_G_IsKind(CALLEE.declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE)) {
        return CALLEE;
    }

    // INFO: a contract function is callable too, the way a method on a generic receiver reaches one.
    if ((MAREA_G_IsKind(CALLEE.node, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsKind(CALLEE.node, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsKind(CALLEE.node, MAREA_AST_KIND_CONTRACT_FUNCTION) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_TypeNone();
    }

    const MAREA_AstNode *result = MAREA_SemaReturnType(CALLEE.node);

    if (result == MAREA_NULLPTR) {
        return MAREA_SemaTypedAstVoidType();
    }

    // INFO: a generic function of this file, called by its bare name with no written type arguments, gets its type
    // arguments inferred from its arguments. An explicit call (CALLEE.frame->arguments already set), a non-generic
    // callee (CALLEE.frame null here, since MAREA_G_FunctionFrame only wraps a frame for a callee with its own
    // generic parameters) and a function reached through a FOUND namespace (callee is a member access, not a bare
    // name of this file) are untouched, matching the documented behavior of a FOUND namespace member.
    if ((MAREA_G_IsKind(CALLEE.node, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE) && (CALLEE.frame != MAREA_NULLPTR) && (CALLEE.frame->arguments == MAREA_NULLPTR) && (MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) && (MAREA_G_FindFileDeclaration(resolver->context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee)) == CALLEE.node)) {
        const MAREA_SemaExprType INFERRED = MAREA_SemaTypedAstInferCallResult(resolver->context, CALLEE.node, result, built);

        if (MAREA_G_TypeIsKnown(&INFERRED) == MAREA_BOOLEAN_TRUE) {
            return INFERRED;
        }
    }

    return MAREA_G_TypeOfNode(result, CALLEE.frame);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNumberLiteralKind(MAREA_AstKind kind)
{
    return ((kind == MAREA_AST_KIND_INTEGER_LITERAL) || (kind == MAREA_AST_KIND_FLOAT_LITERAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a number written as a literal, a sign or a parenthesis on one, or an operation on such numbers, fits any type of its family, so it never decides the type of an operation.
static MAREA_Boolean MAREA_CALL MAREA_G_IsFlexible(const MAREA_SemaTypedNode *node)
{
    if (node == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node->astNode);

    if (MAREA_G_IsNumberLiteralKind(KIND) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (KIND == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) {
        return (node->childCount == 1) ? MAREA_G_IsFlexible(node->children[0]) : MAREA_BOOLEAN_FALSE;
    }

    if (KIND == MAREA_AST_KIND_UNARY_EXPRESSION) {
        const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(node->astNode);

        return (((OPERATOR == MAREA_TOKEN_KIND_MINUS) || (OPERATOR == MAREA_TOKEN_KIND_TILDE)) && (node->childCount == 1)) ? MAREA_G_IsFlexible(node->children[0]) : MAREA_BOOLEAN_FALSE;
    }

    if (KIND == MAREA_AST_KIND_BINARY_EXPRESSION) {
        return ((node->childCount == 2) && (MAREA_G_IsFlexible(node->children[0]) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsFlexible(node->children[1]) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the two sides agree after the check, so the first side that is not a number literal and has a type gives it; a side with no type is never guessed from a literal next to it.
static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfOperands(const MAREA_SemaTypedNode *built)
{
    const MAREA_SemaExprType LEFT = MAREA_G_TypeOfChild(built, 0);

    const MAREA_SemaExprType RIGHT = MAREA_G_TypeOfChild(built, 1);

    const MAREA_Boolean LEFT_FLEXIBLE = MAREA_G_IsFlexible(built->children[0]);

    const MAREA_Boolean RIGHT_FLEXIBLE = MAREA_G_IsFlexible(built->children[1]);

    if ((LEFT_FLEXIBLE == MAREA_BOOLEAN_FALSE) && (MAREA_G_TypeIsKnown(&LEFT) == MAREA_BOOLEAN_TRUE)) {
        return LEFT;
    }

    if ((RIGHT_FLEXIBLE == MAREA_BOOLEAN_FALSE) && (MAREA_G_TypeIsKnown(&RIGHT) == MAREA_BOOLEAN_TRUE)) {
        return RIGHT;
    }

    if ((LEFT_FLEXIBLE == MAREA_BOOLEAN_FALSE) || (RIGHT_FLEXIBLE == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_TypeNone();
    }

    return (MAREA_G_TypeIsKnown(&LEFT) == MAREA_BOOLEAN_TRUE) ? LEFT : RIGHT;
}

MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstTypeOfChild(const MAREA_SemaTypedNode *built, MAREA_IntegerSize index)
{
    return MAREA_G_TypeOfChild(built, index);
}

MAREA_SemaExprType MAREA_CALL MAREA_SemaTypedAstTypeOfOperands(const MAREA_SemaTypedNode *built)
{
    return MAREA_G_TypeOfOperands(built);
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfBoolean(void)
{
    return MAREA_G_TypeOfText("boolean", MAREA_INTEGER_SIZE(7));
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfBinary(const MAREA_SemaTypedNode *built)
{
    if (built->childCount != 2) {
        return MAREA_G_TypeNone();
    }

    switch (MAREA_AstNodeGetOperator(built->astNode)) {
    case MAREA_TOKEN_KIND_KEYWORD_OR:
    case MAREA_TOKEN_KIND_KEYWORD_AND:
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
    case MAREA_TOKEN_KIND_BANG_EQUAL:
    case MAREA_TOKEN_KIND_LESS:
    case MAREA_TOKEN_KIND_LESS_EQUAL:
    case MAREA_TOKEN_KIND_GREATER:
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return MAREA_G_TypeOfBoolean();
    case MAREA_TOKEN_KIND_SHIFT_LEFT:
    case MAREA_TOKEN_KIND_SHIFT_RIGHT:
        return MAREA_G_TypeOfChild(built, 0);
    default:
        return MAREA_G_TypeOfOperands(built);
    }
}

// INFO: spawn of a call gives the spawn-handle type of the type of the call, in the arena.
static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfSpawn(MAREA_G_Resolver *resolver, const MAREA_SemaTypedNode *built)
{
    const MAREA_SemaExprType OPERAND = MAREA_G_TypeOfChild(built, 0);

    // INFO: a task of void is not a type that can be written, so spawn of a call with no result keeps no type.
    if (MAREA_SemaTypedAstTypeIsVoid(&OPERAND) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_TypeNone();
    }

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypedAstExcept
RENDERED
= MAREA_SemaTypedAstRenderExprType(resolver->context, &OPERAND, &text, &length);

if (RENDERED == MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY) {
    resolver->outOfMemory = MAREA_BOOLEAN_TRUE;
}

if ((RENDERED != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR)) {
    return MAREA_G_TypeNone();
}

const MAREA_Character8 *wrapped = MAREA_NULLPTR;

MAREA_IntegerSize wrappedLength = 0;

const MAREA_SemaExcept WRAPPED = MAREA_SemaWrapInRoleType(resolver->context, MAREA_TYPE_ROLE_SPAWN_HANDLE, text, length, &wrapped, &wrappedLength);

if (WRAPPED != MAREA_SEMA_EXCEPT_SUCCESS) {
    resolver->outOfMemory = MAREA_BOOLEAN_TRUE;

    return MAREA_G_TypeNone();
}

return (wrapped == MAREA_NULLPTR) ? MAREA_G_TypeNone() : MAREA_G_TypeOfText(wrapped, wrappedLength);
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfUnary(MAREA_G_Resolver *resolver, const MAREA_SemaTypedNode *built)
{
    if (built->childCount != 1) {
        return MAREA_G_TypeNone();
    }

    switch (MAREA_AstNodeGetOperator(built->astNode)) {
    case MAREA_TOKEN_KIND_KEYWORD_NOT:
        return MAREA_G_TypeOfBoolean();
    case MAREA_TOKEN_KIND_MINUS:
    case MAREA_TOKEN_KIND_TILDE:
    case MAREA_TOKEN_KIND_KEYWORD_COPY:
    case MAREA_TOKEN_KIND_KEYWORD_MOVE:
        return MAREA_G_TypeOfChild(built, 0);
    case MAREA_TOKEN_KIND_KEYWORD_SPAWN:
        return MAREA_G_TypeOfSpawn(resolver, built);
    default:
        return MAREA_G_TypeNone();
    }
}

// INFO: an identifier is a value only where an expression is read. A declared name, a member name, a field key and the name of a loop, a part or a with statement are not.
static MAREA_Boolean MAREA_CALL MAREA_G_IdentifierIsValue(const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    if (parent == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(parent);

    switch (MAREA_AstNodeGetKind(parent)) {
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_ARGUMENT_LIST:
    case MAREA_AST_KIND_INDEX_ENTRY:
    case MAREA_AST_KIND_VALUE_ENTRY:
    case MAREA_AST_KIND_ASSIGNMENT:
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
    case MAREA_AST_KIND_RETURN_STATEMENT:
    case MAREA_AST_KIND_IF_STATEMENT:
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
    case MAREA_AST_KIND_WHILE_STATEMENT:
    case MAREA_AST_KIND_SWITCH_STATEMENT:
    case MAREA_AST_KIND_SWITCH_ARM:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_AST_KIND_MEMBER_ACCESS:
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_WITH_STATEMENT:
        return (index == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FIELD_ENTRY:
        return (index == 1) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return (index >= 2) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
        return (index == (COUNT - 1)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return ((index == 1) || (index == 2)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        return (index == (COUNT - 2)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return (index <= 1) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

static MAREA_SemaExprType MAREA_CALL MAREA_G_TypeOfExpression(MAREA_G_Resolver *resolver, const MAREA_SemaTypedNode *built, const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    switch (MAREA_AstNodeGetKind(built->astNode)) {
    case MAREA_AST_KIND_IDENTIFIER:
        return (MAREA_G_IdentifierIsValue(parent, index) == MAREA_BOOLEAN_TRUE) ? MAREA_G_TypeOfIdentifier(resolver, built->astNode, MAREA_G_CallTypeArguments(parent, index)) : MAREA_G_TypeNone();
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        return MAREA_G_TypeOfChild(built, 0);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_TypeOfBinary(built);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        return MAREA_G_TypeOfUnary(resolver, built);
    case MAREA_AST_KIND_MEMBER_ACCESS:
        return MAREA_G_TypeOfMemberAccess(resolver, built, MAREA_G_CallTypeArguments(parent, index));
    case MAREA_AST_KIND_INDEX_ACCESS:
        return MAREA_G_TypeOfIndexAccess(resolver, built);
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_TypeOfCall(resolver, built);
    case MAREA_AST_KIND_MACRO_CALL:
        return MAREA_SemaTypedAstMacroCallType(resolver->context, built->astNode, &resolver->outOfMemory);
    case MAREA_AST_KIND_LITERAL_VALUE:
    {
        const MAREA_AstNode *type = MAREA_SemaChildAt(built->astNode, 0);

        return (MAREA_G_IsType(type) == MAREA_BOOLEAN_TRUE) ? MAREA_G_TypeOfNode(type, MAREA_NULLPTR) : MAREA_G_TypeNone();
    }
    default:
        return MAREA_G_TypeNone();
    }
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstResolveExpressionType(MAREA_SemaContext *context, MAREA_SemaTypedNode *built, const MAREA_AstNode *parent, MAREA_IntegerSize index)
{
    if ((context == MAREA_NULLPTR) || (built == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    // INFO: the ir carries this to the backends, which send only such a call to a C function.
    if (MAREA_AstNodeGetKind(built->astNode) == MAREA_AST_KIND_CALL_EXPRESSION) {
        const MAREA_SemaTypedAstExcept EXTERNAL = MAREA_SemaTypedAstIsExternalCallee(context, MAREA_SemaChildAt(built->astNode, 0), &built->isExternalCall);

        if (EXTERNAL != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return EXTERNAL;
        }
    }

    if ((MAREA_AstNodeGetKind(built->astNode) == MAREA_AST_KIND_MACRO_CALL) && (MAREA_SemaMacroIncludedFile(context, built->astNode, &built->includedFile) != MAREA_SEMA_EXCEPT_SUCCESS)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_G_Resolver resolver = { .context = context, .outOfMemory = MAREA_BOOLEAN_FALSE };

    const MAREA_SemaExprType TYPE = MAREA_G_TypeOfExpression(&resolver, built, parent, index);

    if (resolver.outOfMemory == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    if (MAREA_G_TypeIsKnown(&TYPE) == MAREA_BOOLEAN_FALSE) {
        built->isQualifier = MAREA_SemaTypedAstIsQualifier(context, built->astNode, parent, index);

        return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
    }

    built->exprType = TYPE;

    return MAREA_SemaTypedAstRenderExprType(context, &TYPE, &built->resolvedType, &built->resolvedTypeLength);
}

// INFO: an operation made from the types of its children, which a number literal below may have changed after the operation was typed.
static MAREA_Boolean MAREA_CALL MAREA_G_IsDerivedOperation(const MAREA_AstNode *node)
{
    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node);

    return ((KIND == MAREA_AST_KIND_BINARY_EXPRESSION) || (KIND == MAREA_AST_KIND_UNARY_EXPRESSION) || (KIND == MAREA_AST_KIND_PARENTHESIZED_EXPRESSION)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaTypedAstExcept MAREA_CALL MAREA_SemaTypedAstRetypeOperations(MAREA_SemaContext *context, MAREA_SemaTypedNode *built)
{
    if ((context == MAREA_NULLPTR) || (built == MAREA_NULLPTR)) {
        return MAREA_SEMA_TYPED_AST_EXCEPT_ERROR_NULL_POINTER;
    }

    for (MAREA_IntegerSize index = 0; index < built->childCount; ++index) {
        MAREA_SemaTypedNode *child = (built->children == MAREA_NULLPTR) ? MAREA_NULLPTR : built->children[index];

        if (child == MAREA_NULLPTR) {
            continue;
        }

        const MAREA_SemaTypedAstExcept RESULT = MAREA_SemaTypedAstRetypeOperations(context, child);

        if (RESULT != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return RESULT;
        }

        if (MAREA_G_IsDerivedOperation(child->astNode) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        // INFO: the type of these operations never reads the parent, so the position is not needed.
        child->exprType = (MAREA_SemaExprType){ .node = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .arguments = MAREA_NULLPTR, .frame = MAREA_NULLPTR, .fixed = MAREA_NULLPTR, .fixedLength = 0 };
        child->resolvedType = MAREA_NULLPTR;
        child->resolvedTypeLength = 0;

        const MAREA_SemaTypedAstExcept TYPED = MAREA_SemaTypedAstResolveExpressionType(context, child, MAREA_NULLPTR, 0);

        if (TYPED != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS) {
            return TYPED;
        }
    }

    return MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS;
}
