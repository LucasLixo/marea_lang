// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: the types of a literal that no context fixes. The repository names no default, so these are the documented assumption of the plan.
static const MAREA_Character8 MAREA_G_DEFAULT_INTEGER[] = "sinteger32";
static const MAREA_Character8 MAREA_G_DEFAULT_FLOAT[] = "float64";
static const MAREA_Character8 MAREA_G_TYPE_STRING[] = "string";
static const MAREA_Character8 MAREA_G_TYPE_CHARACTER[] = "character";
static const MAREA_Character8 MAREA_G_TYPE_BOOLEAN[] = "boolean";

typedef enum MAREA_G_BinaryClass
{
    MAREA_G_BINARY_CLASS_LOGICAL = MAREA_UINTEGER_8(0),
    MAREA_G_BINARY_CLASS_COMPARE = MAREA_UINTEGER_8(1),
    MAREA_G_BINARY_CLASS_SHIFT = MAREA_UINTEGER_8(2),
    MAREA_G_BINARY_CLASS_ARITHMETIC = MAREA_UINTEGER_8(3),
} MAREA_G_BinaryClass;

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
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

static MAREA_SemaTypedNode *MAREA_CALL MAREA_G_ChildAt(const MAREA_SemaTypedNode *node, MAREA_IntegerSize index)
{
    return ((node != MAREA_NULLPTR) && (index < node->childCount)) ? node->children[index] : MAREA_NULLPTR;
}

static MAREA_SemaTypedNode *MAREA_CALL MAREA_G_FindTypedChild(const MAREA_SemaTypedNode *node, MAREA_AstKind kind)
{
    for (MAREA_IntegerSize index = 0; index < node->childCount; ++index) {
        const MAREA_SemaTypedNode *child = node->children[index];

        if ((child != MAREA_NULLPTR) && (MAREA_G_IsKind(child->astNode, kind) == MAREA_BOOLEAN_TRUE)) {
            return node->children[index];
        }
    }

    return MAREA_NULLPTR;
}

static void MAREA_CALL MAREA_G_SetType(MAREA_SemaTypedNode *node, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    node->resolvedType = name;
    node->resolvedTypeLength = length;
}

static MAREA_Boolean MAREA_CALL MAREA_G_HasContextType(const MAREA_SemaTypedNode *node)
{
    return ((node->resolvedType != MAREA_G_DEFAULT_INTEGER) && (node->resolvedType != MAREA_G_DEFAULT_FLOAT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_G_BinaryClass MAREA_CALL MAREA_G_ClassifyBinary(MAREA_TokenKind operatorKind)
{
    switch (operatorKind) {
    case MAREA_TOKEN_KIND_KEYWORD_OR:
    case MAREA_TOKEN_KIND_KEYWORD_AND:
        return MAREA_G_BINARY_CLASS_LOGICAL;
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
    case MAREA_TOKEN_KIND_BANG_EQUAL:
    case MAREA_TOKEN_KIND_LESS:
    case MAREA_TOKEN_KIND_LESS_EQUAL:
    case MAREA_TOKEN_KIND_GREATER:
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return MAREA_G_BINARY_CLASS_COMPARE;
    case MAREA_TOKEN_KIND_SHIFT_LEFT:
    case MAREA_TOKEN_KIND_SHIFT_RIGHT:
        return MAREA_G_BINARY_CLASS_SHIFT;
    default:
        return MAREA_G_BINARY_CLASS_ARITHMETIC;
    }
}

// INFO: the primitive type is passed down through the nodes that keep the type of their operands: a parenthesis, a unary minus or tilde, and an arithmetic or bitwise operator (the left side only for a shift). A literal that already has a type from a nearer context keeps it.
static void MAREA_CALL MAREA_G_Apply(MAREA_SemaTypedNode *node, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    if ((node == MAREA_NULLPTR) || (name == MAREA_NULLPTR)) {
        return;
    }

    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(node->astNode);

    switch (KIND) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
    case MAREA_AST_KIND_FLOAT_LITERAL:
        if ((MAREA_G_HasContextType(node) == MAREA_BOOLEAN_FALSE) && (MAREA_SemaNumberLiteralFitsPrimitive(KIND, name, length) == MAREA_BOOLEAN_TRUE)) {
            MAREA_G_SetType(node, name, length);
        }

        return;
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        MAREA_G_Apply(MAREA_G_ChildAt(node, 0), name, length);

        return;
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    {
        const MAREA_TokenKind OPERATOR_KIND = MAREA_AstNodeGetOperator(node->astNode);

        if ((OPERATOR_KIND == MAREA_TOKEN_KIND_MINUS) || (OPERATOR_KIND == MAREA_TOKEN_KIND_TILDE)) {
            MAREA_G_Apply(MAREA_G_ChildAt(node, 0), name, length);
        }

        return;
    }
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    {
        const MAREA_G_BinaryClass CLASS = MAREA_G_ClassifyBinary(MAREA_AstNodeGetOperator(node->astNode));

        if ((CLASS == MAREA_G_BINARY_CLASS_ARITHMETIC) || (CLASS == MAREA_G_BINARY_CLASS_SHIFT)) {
            MAREA_G_Apply(MAREA_G_ChildAt(node, 0), name, length);
        }

        if (CLASS == MAREA_G_BINARY_CLASS_ARITHMETIC) {
            MAREA_G_Apply(MAREA_G_ChildAt(node, 1), name, length);
        }

        return;
    }
    default:
        return;
    }
}

// INFO: the context is a written type: it fixes the literal only when it is a primitive that the literal fits.
static void MAREA_CALL MAREA_G_ApplyType(const MAREA_SemaContext *context, MAREA_SemaTypedNode *node, const MAREA_AstNode *type)
{
    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((node != MAREA_NULLPTR) && (MAREA_SemaPrimitiveOfType(context, type, &name, &length) == MAREA_BOOLEAN_TRUE)) {
        MAREA_G_Apply(node, name, length);
    }
}

// INFO: the context is the type of another expression, the other operand of a pair.
static void MAREA_CALL MAREA_G_ApplyExpression(const MAREA_SemaContext *context, MAREA_SemaTypedNode *node, const MAREA_AstNode *expression)
{
    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((node != MAREA_NULLPTR) && (MAREA_SemaPrimitiveOfExpression(context, expression, &name, &length) == MAREA_BOOLEAN_TRUE)) {
        MAREA_G_Apply(node, name, length);
    }
}

// INFO: two operands that must have one type: each one that has no known type takes the type of the other.
static void MAREA_CALL MAREA_G_ApplyPair(const MAREA_SemaContext *context, MAREA_SemaTypedNode *left, MAREA_SemaTypedNode *right)
{
    if ((left == MAREA_NULLPTR) || (right == MAREA_NULLPTR)) {
        return;
    }

    MAREA_G_ApplyExpression(context, left, right->astNode);
    MAREA_G_ApplyExpression(context, right, left->astNode);
}

static void MAREA_CALL MAREA_G_ApplyArguments(const MAREA_SemaContext *context, MAREA_SemaTypedNode *arguments, const MAREA_AstNode *parameters)
{
    const MAREA_IntegerSize PARAMETER_COUNT = (parameters == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_AstNodeGetChildCount(parameters);

    for (MAREA_IntegerSize index = 0; ((index < arguments->childCount) && (index < PARAMETER_COUNT)); ++index) {
        const MAREA_AstNode *parameter = MAREA_SemaChildAt(parameters, index);

        MAREA_G_ApplyType(context, arguments->children[index], (parameter == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(parameter, 1));
    }
}

static void MAREA_CALL MAREA_G_ApplyCall(const MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    MAREA_SemaTypedNode *arguments = MAREA_G_FindTypedChild(built, MAREA_AST_KIND_ARGUMENT_LIST);

    if (arguments == MAREA_NULLPTR) {
        return;
    }

    const MAREA_AstNode *convertTarget = MAREA_SemaConvertTarget(context, built->astNode);

    if (convertTarget != MAREA_NULLPTR) {
        MAREA_G_ApplyType(context, MAREA_G_ChildAt(arguments, 0), convertTarget);

        return;
    }

    const MAREA_AstNode *callee = MAREA_SemaChildAt(built->astNode, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee)) != MAREA_NULLPTR)) {
        return;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR)) {
        return;
    }

    const MAREA_AstKind DECLARATION_KIND = MAREA_AstNodeGetKind(entry->declaration);

    // INFO: a call that names a structure or an enum is a construction: its arguments meet the parameters of the construct operator.
    if ((DECLARATION_KIND == MAREA_AST_KIND_STRUCTURE_DECLARATION) || (DECLARATION_KIND == MAREA_AST_KIND_ENUM_DECLARATION)) {
        const MAREA_AstNode *construct = MAREA_SemaFindConstructOperator(context, entry->declaration);

        MAREA_G_ApplyArguments(context, arguments, (construct == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(construct, MAREA_AST_KIND_PARAMETER_LIST));

        return;
    }

    if (DECLARATION_KIND == MAREA_AST_KIND_FUNCTION_DECLARATION) {
        MAREA_G_ApplyArguments(context, arguments, MAREA_G_FindChild(entry->declaration, MAREA_AST_KIND_PARAMETER_LIST));
    }
}

// INFO: the type of the field that a structure declares under a name, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FieldType(const MAREA_AstNode *structure, const MAREA_AstNode *name)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(structure); ++index) {
        const MAREA_AstNode *field = MAREA_SemaChildAt(structure, index);

        const MAREA_AstNode *fieldName = (MAREA_G_IsKind(field, MAREA_AST_KIND_FIELD_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

        if ((fieldName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1));
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsElementContainer(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    return ((MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_TRUE) && (descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: an entry of a literal value takes the declared type of its field in a structure, or the element type in an array, a vector or a slice. Any other library type is not guessed.
static void MAREA_CALL MAREA_G_ApplyLiteralValue(const MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    const MAREA_AstNode *type = MAREA_SemaChildAt(built->astNode, 0);

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE) {
        return;
    }

    const MAREA_SemaTypeClass CLASS = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);

    const MAREA_Boolean IS_STRUCTURE = ((CLASS == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_AstNode *arguments = MAREA_G_FindChild(type, MAREA_AST_KIND_TYPE_ARGUMENTS);

    const MAREA_AstNode *firstArgument = (arguments == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(arguments, 0);

    const MAREA_AstNode *element = ((IS_STRUCTURE == MAREA_BOOLEAN_FALSE) && ((CLASS == MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN) || (CLASS == MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN)) && (MAREA_G_IsElementContainer(context, type) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsType(firstArgument) == MAREA_BOOLEAN_TRUE)) ? firstArgument : MAREA_NULLPTR;

    for (MAREA_IntegerSize index = 1; index < built->childCount; ++index) {
        const MAREA_SemaTypedNode *entry = built->children[index];

        if (entry == MAREA_NULLPTR) {
            continue;
        }

        switch (MAREA_AstNodeGetKind(entry->astNode)) {
        case MAREA_AST_KIND_FIELD_ENTRY:
            if (IS_STRUCTURE == MAREA_BOOLEAN_TRUE) {
                MAREA_G_ApplyType(context, MAREA_G_ChildAt(entry, 1), MAREA_G_FieldType(declaration, MAREA_SemaChildAt(entry->astNode, 0)));
            }

            break;
        case MAREA_AST_KIND_VALUE_ENTRY:
            MAREA_G_ApplyType(context, MAREA_G_ChildAt(entry, 0), element);

            break;
        case MAREA_AST_KIND_INDEX_ENTRY:
            MAREA_G_ApplyType(context, MAREA_G_ChildAt(entry, 1), element);

            break;
        default:
            break;
        }
    }
}

static void MAREA_CALL MAREA_G_ApplySwitch(const MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    const MAREA_SemaTypedNode *subject = MAREA_G_ChildAt(built, 0);

    if (subject == MAREA_NULLPTR) {
        return;
    }

    for (MAREA_IntegerSize index = 1; index < built->childCount; ++index) {
        const MAREA_SemaTypedNode *arm = built->children[index];

        if ((arm == MAREA_NULLPTR) || (MAREA_G_IsKind(arm->astNode, MAREA_AST_KIND_SWITCH_ARM) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        // INFO: the values of an arm come before its block.
        for (MAREA_IntegerSize value = 0; ((value < arm->childCount) && (arm->children[value] != MAREA_NULLPTR) && (MAREA_G_IsKind(arm->children[value]->astNode, MAREA_AST_KIND_BLOCK) == MAREA_BOOLEAN_FALSE)); ++value) {
            MAREA_G_ApplyExpression(context, arm->children[value], subject->astNode);
        }
    }
}

static void MAREA_CALL MAREA_G_ApplyAssignment(const MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    const MAREA_TokenKind OPERATOR_KIND = MAREA_AstNodeGetOperator(built->astNode);

    // INFO: the count of a compound shift may be any integer, so it takes no type from the target.
    if ((OPERATOR_KIND == MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL) || (OPERATOR_KIND == MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL)) {
        return;
    }

    MAREA_G_ApplyExpression(context, MAREA_G_ChildAt(built, 1), MAREA_SemaChildAt(built->astNode, 0));
}

void MAREA_CALL MAREA_SemaTypedAstResolveLiteralDefault(MAREA_SemaTypedNode *built)
{
    if (built == MAREA_NULLPTR) {
        return;
    }

    switch (MAREA_AstNodeGetKind(built->astNode)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
        MAREA_G_SetType(built, MAREA_G_DEFAULT_INTEGER, (sizeof(MAREA_G_DEFAULT_INTEGER) - MAREA_INTEGER_SIZE(1)));

        return;
    case MAREA_AST_KIND_FLOAT_LITERAL:
        MAREA_G_SetType(built, MAREA_G_DEFAULT_FLOAT, (sizeof(MAREA_G_DEFAULT_FLOAT) - MAREA_INTEGER_SIZE(1)));

        return;
    case MAREA_AST_KIND_STRING_LITERAL:
        MAREA_G_SetType(built, MAREA_G_TYPE_STRING, (sizeof(MAREA_G_TYPE_STRING) - MAREA_INTEGER_SIZE(1)));

        return;
    case MAREA_AST_KIND_CHARACTER_LITERAL:
        MAREA_G_SetType(built, MAREA_G_TYPE_CHARACTER, (sizeof(MAREA_G_TYPE_CHARACTER) - MAREA_INTEGER_SIZE(1)));

        return;
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        MAREA_G_SetType(built, MAREA_G_TYPE_BOOLEAN, (sizeof(MAREA_G_TYPE_BOOLEAN) - MAREA_INTEGER_SIZE(1)));

        return;
    default:
        return;
    }
}

void MAREA_CALL MAREA_SemaTypedAstResolveLiteralContext(const MAREA_SemaContext *context, const MAREA_SemaTypedNode *built)
{
    if ((context == MAREA_NULLPTR) || (built == MAREA_NULLPTR) || (built->childCount == 0)) {
        return;
    }

    const MAREA_AstNode *node = built->astNode;

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
        MAREA_G_ApplyType(context, MAREA_G_ChildAt(built, (built->childCount - 1)), (built->childCount < 2) ? MAREA_NULLPTR : MAREA_SemaChildAt(node, (built->childCount - 2)));

        return;
    case MAREA_AST_KIND_LOCAL_DECLARATION:
    case MAREA_AST_KIND_VARIANT_FIELD:
        MAREA_G_ApplyType(context, MAREA_G_ChildAt(built, 2), MAREA_SemaChildAt(node, 1));

        return;
    case MAREA_AST_KIND_PARAMETER:
        if (MAREA_SemaParameterHasDefault(node) == MAREA_BOOLEAN_TRUE) {
            MAREA_G_ApplyType(context, MAREA_G_ChildAt(built, 2), MAREA_SemaChildAt(node, 1));
        }

        return;
    case MAREA_AST_KIND_RETURN_STATEMENT:
        MAREA_G_ApplyType(context, MAREA_G_ChildAt(built, 0), (context->function == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaReturnType(context->function));

        return;
    case MAREA_AST_KIND_ASSIGNMENT:
        MAREA_G_ApplyAssignment(context, built);

        return;
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    {
        const MAREA_G_BinaryClass CLASS = MAREA_G_ClassifyBinary(MAREA_AstNodeGetOperator(node));

        if ((CLASS == MAREA_G_BINARY_CLASS_COMPARE) || (CLASS == MAREA_G_BINARY_CLASS_ARITHMETIC)) {
            MAREA_G_ApplyPair(context, MAREA_G_ChildAt(built, 0), MAREA_G_ChildAt(built, 1));
        }

        return;
    }
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        MAREA_G_ApplyPair(context, MAREA_G_ChildAt(built, 1), MAREA_G_ChildAt(built, 2));

        return;
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        MAREA_G_ApplySwitch(context, built);

        return;
    case MAREA_AST_KIND_CALL_EXPRESSION:
        MAREA_G_ApplyCall(context, built);

        return;
    case MAREA_AST_KIND_LITERAL_VALUE:
        MAREA_G_ApplyLiteralValue(context, built);

        return;
    default:
        return;
    }
}
