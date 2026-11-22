// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

// INFO: the first direct child that is a type; a type always comes before an optional expression.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindTypeChild(const MAREA_SemaTypedNode *typed)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstNode *ast = MAREA_SemaTypedNodeGetAstNode(MAREA_IrTypedChildAt(typed, index));

        const MAREA_AstKind KIND = MAREA_AstNodeGetKind(ast);

        if ((KIND == MAREA_AST_KIND_NAMED_TYPE) || (KIND == MAREA_AST_KIND_FUNCTION_TYPE)) {
            return ast;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasChildOfKind(const MAREA_SemaTypedNode *typed, MAREA_AstKind kind)
{
    return (MAREA_IrTypedFindChild(typed, kind) != MAREA_NULLPTR) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: raises UNINSTANTIATED_GENERIC for a declaration, naming it when it has an IDENTIFIER.
static MAREA_IrExcept MAREA_CALL MAREA_G_RaiseGenericDeclaration(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *message)
{
    const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_IDENTIFIER);

    const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

    return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNINSTANTIATED_GENERIC, MAREA_SemaTypedNodeGetAstNode(typed), message, MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));
}

MAREA_IrExcept MAREA_CALL MAREA_IrCreateNamed(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode **outNode)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outNode == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, kind, MAREA_SemaTypedNodeGetAstNode(typed), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_IDENTIFIER);

    if (name != MAREA_NULLPTR) {
        const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

        result = MAREA_IrNodeSetQualifiedText(context, node, qualifier, qualifierLength, MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    // INFO: only the MUTABLE flag survives; the other AST flags are erased with the type system.
    node->flags = (node->flags & (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE);

    result = MAREA_IrNodeSetTypeNameFromAst(context, node, MAREA_G_FindTypeChild(typed));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrAppendInitializer(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean *outComplete)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outComplete == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    *outComplete = MAREA_BOOLEAN_TRUE;

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index + MAREA_INTEGER_SIZE(1)) < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstKind KIND = MAREA_IrTypedKind(MAREA_IrTypedChildAt(typed, index));

        if ((KIND != MAREA_AST_KIND_NAMED_TYPE) && (KIND != MAREA_AST_KIND_FUNCTION_TYPE)) {
            continue;
        }

        MAREA_IrNode *lowered = MAREA_NULLPTR;

        MAREA_IrExcept result = MAREA_IrLowerExpression(context, MAREA_IrTypedChildAt(typed, (index + MAREA_INTEGER_SIZE(1))), &lowered);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        if (lowered == MAREA_NULLPTR) {
            *outComplete = MAREA_BOOLEAN_FALSE;

            return MAREA_IR_EXCEPT_SUCCESS;
        }

        return MAREA_IrNodeAppendChild(context, node, lowered);
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

static MAREA_IrExcept MAREA_CALL MAREA_G_AppendNamed(MAREA_IrContext *context, MAREA_IrKind kind, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    const MAREA_IrExcept created = MAREA_IrCreateNamed(context, kind, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (created != MAREA_IR_EXCEPT_SUCCESS) {
        return created;
    }

    return MAREA_IrNodeAppendChild(context, parent, node);
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerStructure(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent)
{
    if (MAREA_G_HasChildOfKind(typed, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_RaiseGenericDeclaration(context, typed, "A generic structure is not instantiated by the IR yet:");
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_STRUCTURE_DECLARATION, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *field = MAREA_IrTypedChildAt(typed, index);

        if (MAREA_IrTypedKind(field) != MAREA_AST_KIND_FIELD_DECLARATION) {
            continue;
        }

        result = MAREA_G_AppendNamed(context, MAREA_IR_KIND_FIELD_DECLARATION, field, node);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IrNodeAppendChild(context, parent, node);
}

// INFO: an opaque structure is one uniform type-declaration shape: a structure with zero fields.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerOpaque(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent)
{
    return MAREA_G_AppendNamed(context, MAREA_IR_KIND_STRUCTURE_DECLARATION, typed, parent);
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerEnum(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_ENUM_DECLARATION, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_IntegerSize VARIANT_COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < VARIANT_COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *variant = MAREA_IrTypedChildAt(typed, index);

        if (MAREA_IrTypedKind(variant) != MAREA_AST_KIND_VARIANT_DECLARATION) {
            continue;
        }

        MAREA_IrNode *variantNode = MAREA_NULLPTR;

        result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_VARIANT_DECLARATION, variant, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &variantNode);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_IntegerSize FIELD_COUNT = MAREA_SemaTypedNodeGetChildCount(variant);

        for (MAREA_IntegerSize fieldIndex = MAREA_INTEGER_SIZE(0); fieldIndex < FIELD_COUNT; fieldIndex = (fieldIndex + MAREA_INTEGER_SIZE(1))) {
            const MAREA_SemaTypedNode *field = MAREA_IrTypedChildAt(variant, fieldIndex);

            if (MAREA_IrTypedKind(field) != MAREA_AST_KIND_VARIANT_FIELD) {
                continue;
            }

            MAREA_IrNode *fieldNode = MAREA_NULLPTR;

            result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_VARIANT_FIELD, field, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &fieldNode);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }

            MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

            result = MAREA_IrAppendInitializer(context, field, fieldNode, &complete);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }

            // INFO: a default value that cannot be lowered drops the whole enum, never a partial one.
            if (complete == MAREA_BOOLEAN_FALSE) {
                return MAREA_IR_EXCEPT_SUCCESS;
            }

            result = MAREA_IrNodeAppendChild(context, variantNode, fieldNode);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }
        }

        result = MAREA_IrNodeAppendChild(context, node, variantNode);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IrNodeAppendChild(context, parent, node);
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerConstant(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode *parent)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_CONSTANT_DECLARATION, typed, qualifier, qualifierLength, &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

    result = MAREA_IrAppendInitializer(context, typed, node, &complete);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (complete == MAREA_BOOLEAN_FALSE)) {
        return result;
    }

    return MAREA_IrNodeAppendChild(context, parent, node);
}

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerFunctionBody(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode *parent)
{
    if (MAREA_G_HasChildOfKind(typed, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_RaiseGenericDeclaration(context, typed, "A generic function is not instantiated by the IR yet:");
    }

    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_FUNCTION_DECLARATION, typed, qualifier, qualifierLength, &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_SemaTypedNode *parameters = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_PARAMETER_LIST);

    if (parameters != MAREA_NULLPTR) {
        MAREA_IrNode *listNode = MAREA_NULLPTR;

        result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_PARAMETER_LIST, MAREA_SemaTypedNodeGetAstNode(parameters), &listNode);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(parameters);

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_SemaTypedNode *parameter = MAREA_IrTypedChildAt(parameters, index);

            if (MAREA_IrTypedKind(parameter) != MAREA_AST_KIND_PARAMETER) {
                continue;
            }

            result = MAREA_G_AppendNamed(context, MAREA_IR_KIND_PARAMETER, parameter, listNode);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }
        }

        result = MAREA_IrNodeAppendChild(context, node, listNode);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    const MAREA_SemaTypedNode *body = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_BLOCK);

    if (body != MAREA_NULLPTR) {
        MAREA_IrNode *blockNode = MAREA_NULLPTR;

        result = MAREA_IrLowerBlock(context, body, &blockNode);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }

        // INFO: a body that cannot be lowered drops the whole function, never a partial one.
        if (blockNode == MAREA_NULLPTR) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }

        result = MAREA_IrNodeAppendChild(context, node, blockNode);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IrNodeAppendChild(context, parent, node);
}

// INFO: the function stays in the context while it is lowered, so that a name it declares hides an alias.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerFunction(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode *parent)
{
    context->function = typed;

    const MAREA_IrExcept result = MAREA_G_LowerFunctionBody(context, typed, qualifier, qualifierLength, parent);

    context->function = MAREA_NULLPTR;

    return result;
}

// INFO: an implementation block makes no node; its members are lowered as module-level nodes named Type.name.
static MAREA_IrExcept MAREA_CALL MAREA_G_LowerImplementation(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent)
{
    if (MAREA_G_HasChildOfKind(typed, MAREA_AST_KIND_GENERIC_PARAMETERS) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_RaiseGenericDeclaration(context, typed, "A generic implementation is not instantiated by the IR yet.");
    }

    const MAREA_SemaTypedNode *typeNode = MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_NAMED_TYPE);

    const MAREA_Character8 *qualifier = MAREA_NULLPTR;
    MAREA_IntegerSize qualifierLength = MAREA_INTEGER_SIZE(0);

    if ((typeNode == MAREA_NULLPTR) || (MAREA_IrAstTypeName(MAREA_SemaTypedNodeGetAstNode(typeNode), &qualifier, &qualifierLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_IR_EXCEPT_SUCCESS;
    }

    // INFO: the methods are named after the original type, never after an alias of it.
    MAREA_IrExcept result = MAREA_IrAliasType(context, qualifier, qualifierLength, &qualifier, &qualifierLength);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *member = MAREA_IrTypedChildAt(typed, index);

        const MAREA_AstKind KIND = MAREA_IrTypedKind(member);

        if ((KIND != MAREA_AST_KIND_FUNCTION_DECLARATION) && (KIND != MAREA_AST_KIND_CONSTANT_DECLARATION)) {
            continue;
        }

        result = MAREA_IrLowerDeclaration(context, member, qualifier, qualifierLength, parent);

        if (result != MAREA_IR_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_IR_EXCEPT_SUCCESS;
}

MAREA_IrExcept MAREA_CALL MAREA_IrLowerDeclaration(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, const MAREA_Character8 *qualifier, MAREA_IntegerSize qualifierLength, MAREA_IrNode *parent)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (typed == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    if (parent == MAREA_NULLPTR) {
        return MAREA_IR_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_IrTypedKind(typed)) {
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
        return MAREA_G_LowerConstant(context, typed, qualifier, qualifierLength, parent);
    case MAREA_AST_KIND_STRUCTURE_DECLARATION:
        return MAREA_G_LowerStructure(context, typed, parent);
    case MAREA_AST_KIND_OPAQUE_DECLARATION:
        return MAREA_G_LowerOpaque(context, typed, parent);
    case MAREA_AST_KIND_ENUM_DECLARATION:
        return MAREA_G_LowerEnum(context, typed, parent);
    case MAREA_AST_KIND_FUNCTION_DECLARATION:
        return MAREA_G_LowerFunction(context, typed, qualifier, qualifierLength, parent);
    case MAREA_AST_KIND_IMPLEMENTATION_DECLARATION:
        return MAREA_G_LowerImplementation(context, typed, parent);
    case MAREA_AST_KIND_MACRO_STATEMENT:
    {
        const MAREA_SemaTypedNode *name = MAREA_IrTypedFindChild(MAREA_IrTypedFindChild(typed, MAREA_AST_KIND_MACRO_CALL), MAREA_AST_KIND_IDENTIFIER);

        const MAREA_AstNode *nameAst = MAREA_SemaTypedNodeGetAstNode(name);

        return MAREA_IrRaise(context, MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO, MAREA_SemaTypedNodeGetAstNode(typed), "A macro at module level is not supported by the IR yet:", MAREA_AstNodeGetText(nameAst), MAREA_AstNodeGetTextLength(nameAst));
    }
    default:
        // INFO: namespace, import, file attribute, type alias, contract and error nodes make no IR node.
        return MAREA_IR_EXCEPT_SUCCESS;
    }
}
