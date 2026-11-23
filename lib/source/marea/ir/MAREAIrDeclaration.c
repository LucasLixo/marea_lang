// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/ir/MAREAIrInternal.h"

#include <string.h>

// INFO: true when value is a quoted string literal whose inner bytes equal inner, same rule sema's own lifecycle check uses.
static MAREA_Boolean MAREA_CALL MAREA_G_TextIsQuoted(const MAREA_AstNode *value, const MAREA_Character8 *inner, MAREA_IntegerSize innerLength)
{
    if ((value == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(value) != MAREA_AST_KIND_STRING_LITERAL) || (MAREA_AstNodeGetTextLength(value) != (innerLength + MAREA_INTEGER_SIZE(2)))) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (memcmp((MAREA_AstNodeGetText(value) + MAREA_INTEGER_SIZE(1)), inner, innerLength) == 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: true when functionTyped carries #[operator = operatorName] in its first ATTRIBUTE group.
static MAREA_Boolean MAREA_CALL MAREA_G_FunctionCarriesOperator(const MAREA_SemaTypedNode *functionTyped, const MAREA_Character8 *operatorName, MAREA_IntegerSize operatorLength)
{
    const MAREA_SemaTypedNode *attributeList = MAREA_IrTypedFindChild(functionTyped, MAREA_AST_KIND_ATTRIBUTE);

    if (attributeList == MAREA_NULLPTR) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(attributeList);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(attributeList, index);

        if (MAREA_IrTypedKind(item) != MAREA_AST_KIND_ATTRIBUTE_ITEM) {
            continue;
        }

        const MAREA_SemaTypedNode *key = MAREA_IrTypedChildAt(item, MAREA_INTEGER_SIZE(0));

        if (MAREA_IrTypedKind(key) != MAREA_AST_KIND_IDENTIFIER) {
            continue;
        }

        const MAREA_AstNode *keyAst = MAREA_SemaTypedNodeGetAstNode(key);

        const MAREA_IntegerSize OPERATOR_KEY_LENGTH = (MAREA_IntegerSize)strlen("operator");

        if ((MAREA_AstNodeGetTextLength(keyAst) != OPERATOR_KEY_LENGTH) || (memcmp(MAREA_AstNodeGetText(keyAst), "operator", OPERATOR_KEY_LENGTH) != 0)) {
            continue;
        }

        const MAREA_SemaTypedNode *value = MAREA_IrTypedChildAt(item, MAREA_INTEGER_SIZE(1));

        if (MAREA_G_TextIsQuoted(MAREA_SemaTypedNodeGetAstNode(value), operatorName, operatorLength) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrFindLifecycleFunction(const MAREA_IrContext *context, const MAREA_Character8 *typeName, MAREA_IntegerSize typeNameLength, MAREA_Boolean wantConstruct)
{
    if ((context == MAREA_NULLPTR) || (context->unit == MAREA_NULLPTR) || (typeName == MAREA_NULLPTR)) {
        return MAREA_NULLPTR;
    }

    static const MAREA_Character8 CONSTRUCT_NAME[] = "construct";
    static const MAREA_Character8 DESTROY_NAME[] = "destroy";

    const MAREA_Character8 *operatorName = (wantConstruct == MAREA_BOOLEAN_TRUE) ? CONSTRUCT_NAME : DESTROY_NAME;
    const MAREA_IntegerSize operatorLength = (MAREA_IntegerSize)((wantConstruct == MAREA_BOOLEAN_TRUE) ? (sizeof(CONSTRUCT_NAME) - 1) : (sizeof(DESTROY_NAME) - 1));

    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(context->unit);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_SemaTypedNode *item = MAREA_IrTypedChildAt(context->unit, index);

        if (MAREA_IrTypedKind(item) != MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) {
            continue;
        }

        const MAREA_SemaTypedNode *typeNode = MAREA_IrTypedFindChild(item, MAREA_AST_KIND_NAMED_TYPE);

        const MAREA_Character8 *implementedName = MAREA_NULLPTR;
        MAREA_IntegerSize implementedLength = MAREA_INTEGER_SIZE(0);

        if ((typeNode == MAREA_NULLPTR) || (MAREA_IrAstTypeName(MAREA_SemaTypedNodeGetAstNode(typeNode), &implementedName, &implementedLength) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        if ((implementedLength != typeNameLength) || (memcmp(implementedName, typeName, typeNameLength) != 0)) {
            continue;
        }

        const MAREA_IntegerSize MEMBER_COUNT = MAREA_SemaTypedNodeGetChildCount(item);

        for (MAREA_IntegerSize memberIndex = MAREA_INTEGER_SIZE(0); memberIndex < MEMBER_COUNT; memberIndex = (memberIndex + MAREA_INTEGER_SIZE(1))) {
            const MAREA_SemaTypedNode *member = MAREA_IrTypedChildAt(item, memberIndex);

            if (MAREA_IrTypedKind(member) != MAREA_AST_KIND_FUNCTION_DECLARATION) {
                continue;
            }

            if (MAREA_G_FunctionCarriesOperator(member, operatorName, operatorLength) == MAREA_BOOLEAN_TRUE) {
                return member;
            }
        }
    }

    return MAREA_NULLPTR;
}

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

    if (kind == MAREA_IR_KIND_PARAMETER) {
        // INFO: a parameter no longer carries MUTABLE on its own AST node, so this reads the ownership mode sema already computed from its semantics attribute or type-class default.
        node->flags = (MAREA_SemaTypedNodeGetOwnershipMode(typed) == MAREA_SEMA_OWNERSHIP_MODE_MUTABLE_BORROW) ? (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE : MAREA_UINTEGER_32(0);
    } else {
        // INFO: only the MUTABLE flag survives; the other AST flags are erased with the type system.
        node->flags = (node->flags & (MAREA_Uinteger32)MAREA_AST_FLAG_MUTABLE);
    }

    result = MAREA_IrNodeSetTypeNameFromAst(context, node, MAREA_G_FindTypeChild(typed));

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: sema gives a resolved type only to the six declaring kinds; any other kind built here keeps none.
    result = MAREA_IrNodeSetResolvedTypeFromTyped(context, node, typed, MAREA_NULLPTR);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    *outNode = node;

    return MAREA_IR_EXCEPT_SUCCESS;
}

const MAREA_SemaTypedNode *MAREA_CALL MAREA_IrTypedFindDefault(const MAREA_SemaTypedNode *typed)
{
    const MAREA_IntegerSize COUNT = MAREA_SemaTypedNodeGetChildCount(typed);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index + MAREA_INTEGER_SIZE(1)) < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_AstKind KIND = MAREA_IrTypedKind(MAREA_IrTypedChildAt(typed, index));

        if ((KIND != MAREA_AST_KIND_NAMED_TYPE) && (KIND != MAREA_AST_KIND_FUNCTION_TYPE)) {
            continue;
        }

        const MAREA_SemaTypedNode *candidate = MAREA_IrTypedChildAt(typed, (index + MAREA_INTEGER_SIZE(1)));

        // INFO: a parameter's trailing "semantics" attribute is not a default value.
        return (MAREA_IrTypedKind(candidate) == MAREA_AST_KIND_ATTRIBUTE) ? MAREA_NULLPTR : candidate;
    }

    return MAREA_NULLPTR;
}

// INFO: the zero-default rule's own node: a leaf with no text, no type name and no children, left for a backend to fill.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendZero(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node)
{
    MAREA_IrNode *zero = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrNodeCreate(context, MAREA_IR_KIND_ZERO_VALUE, MAREA_SemaTypedNodeGetAstNode(typed), &zero);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: the zero fills the thing the typed node declares, so it has the type of that declaration.
    result = MAREA_IrNodeSetResolvedTypeFromTyped(context, zero, typed, MAREA_NULLPTR);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    return MAREA_IrNodeAppendChild(context, node, zero);
}

MAREA_IrExcept MAREA_CALL MAREA_IrAppendInitializer(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *node, MAREA_Boolean zeroWhenAbsent, MAREA_Boolean *outComplete)
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

    const MAREA_SemaTypedNode *initializer = MAREA_IrTypedFindDefault(typed);

    if (initializer == MAREA_NULLPTR) {
        if (zeroWhenAbsent == MAREA_BOOLEAN_FALSE) {
            return MAREA_IR_EXCEPT_SUCCESS;
        }

        return MAREA_G_AppendZero(context, typed, node);
    }

    MAREA_IrNode *lowered = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrLowerExpression(context, initializer, &lowered);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (lowered == MAREA_NULLPTR) {
        *outComplete = MAREA_BOOLEAN_FALSE;

        return MAREA_IR_EXCEPT_SUCCESS;
    }

    return MAREA_IrNodeAppendChild(context, node, lowered);
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

// INFO: a parameter whose default cannot be lowered drops the whole function, never a partial parameter list.
static MAREA_IrExcept MAREA_CALL MAREA_G_AppendParameter(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent, MAREA_Boolean *outComplete)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_PARAMETER, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    // INFO: a parameter with no default is a required argument, never a zero, so the zero-default rule skips it.
    result = MAREA_IrAppendInitializer(context, typed, node, MAREA_BOOLEAN_FALSE, outComplete);

    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (*outComplete == MAREA_BOOLEAN_FALSE)) {
        return result;
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

// INFO: an enum with no written tag type defaults to sinteger32, per the language spec.
static const MAREA_Character8 MAREA_G_DefaultEnumTagType[] = "sinteger32";

static MAREA_IrExcept MAREA_CALL MAREA_G_LowerEnum(MAREA_IrContext *context, const MAREA_SemaTypedNode *typed, MAREA_IrNode *parent)
{
    MAREA_IrNode *node = MAREA_NULLPTR;

    MAREA_IrExcept result = MAREA_IrCreateNamed(context, MAREA_IR_KIND_ENUM_DECLARATION, typed, MAREA_NULLPTR, MAREA_INTEGER_SIZE(0), &node);

    if (result != MAREA_IR_EXCEPT_SUCCESS) {
        return result;
    }

    if (node->typeName == MAREA_NULLPTR) {
        node->typeName = MAREA_G_DefaultEnumTagType;
        node->typeNameLength = (MAREA_IntegerSize)(sizeof(MAREA_G_DefaultEnumTagType) - MAREA_INTEGER_SIZE(1));
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

            // INFO: a variant field with no default is a required argument of the variant, never a zero, same as a parameter.
            result = MAREA_IrAppendInitializer(context, field, fieldNode, MAREA_BOOLEAN_FALSE, &complete);

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

    // INFO: the grammar makes a constant's initializer mandatory, so zeroWhenAbsent never triggers here; it stays false for clarity.
    result = MAREA_IrAppendInitializer(context, typed, node, MAREA_BOOLEAN_FALSE, &complete);

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

            MAREA_Boolean complete = MAREA_BOOLEAN_TRUE;

            result = MAREA_G_AppendParameter(context, parameter, listNode, &complete);

            if (result != MAREA_IR_EXCEPT_SUCCESS) {
                return result;
            }

            // INFO: a parameter default that cannot be lowered drops the whole function, never a partial one.
            if (complete == MAREA_BOOLEAN_FALSE) {
                return MAREA_IR_EXCEPT_SUCCESS;
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

    // INFO: an alias of a function type has no name to qualify the methods with.
    if ((result != MAREA_IR_EXCEPT_SUCCESS) || (qualifier == MAREA_NULLPTR)) {
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
