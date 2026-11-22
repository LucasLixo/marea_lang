// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_DEPTH_LIMIT MAREA_UINTEGER_32(256)

typedef enum MAREA_G_ShapeKind
{
    MAREA_G_SHAPE_KIND_UNKNOWN = MAREA_UINTEGER_8(0),
    MAREA_G_SHAPE_KIND_INTEGER_LITERAL = MAREA_UINTEGER_8(1),
    MAREA_G_SHAPE_KIND_FLOAT_LITERAL = MAREA_UINTEGER_8(2),
    MAREA_G_SHAPE_KIND_PRIMITIVE = MAREA_UINTEGER_8(3),
    MAREA_G_SHAPE_KIND_STRUCTURE = MAREA_UINTEGER_8(4),
} MAREA_G_ShapeKind;

// INFO: what sema knows of the type of an expression. A literal number fits any type of its family, a primitive carries its name.
typedef struct MAREA_G_Shape
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize length;
    MAREA_G_ShapeKind kind;
} MAREA_G_Shape;

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeMake(MAREA_G_ShapeKind kind, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    return (MAREA_G_Shape){ .name = name, .length = length, .kind = kind };
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapePrimitive(const MAREA_Character8 *name)
{
    return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, name, strlen(name));
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return MAREA_SemaTextEquals(text, length, expected, strlen(expected));
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindChild(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

    for (MAREA_IntegerSize index = 0; index < count; ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(node, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == kind)) {
            return child;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsKind(const MAREA_AstNode *node, MAREA_AstKind kind)
{
    return ((node != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(node) == kind)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsType(const MAREA_AstNode *node)
{
    return ((MAREA_G_IsKind(node, MAREA_AST_KIND_NAMED_TYPE) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(node, MAREA_AST_KIND_FUNCTION_TYPE) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_NodeTextIs(const MAREA_AstNode *left, const MAREA_AstNode *right)
{
    return MAREA_SemaTextEquals(MAREA_AstNodeGetText(left), MAREA_AstNodeGetTextLength(left), MAREA_AstNodeGetText(right), MAREA_AstNodeGetTextLength(right));
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_AppendLocal(MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if (MAREA_G_IsKind(name, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: the entry has no type, so a name that it hides is never typed from an outer local of the same name.
    return MAREA_SemaTableAppend(context, &context->locals, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = MAREA_NULLPTR, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaDeclareBinders(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, 0));
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    {
        // INFO: the children are the variables, the collection and the body, so the variables end two places before the end.
        const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

        if ((count < 3) || (MAREA_G_IsKind(MAREA_SemaChildAt(node, (count - 1)), MAREA_AST_KIND_BLOCK) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        for (MAREA_IntegerSize index = 0; index < (count - 2); ++index) {
            const MAREA_SemaExcept result = MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, index));

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, 2));
    case MAREA_AST_KIND_WITH_STATEMENT:
        return MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, 1));
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

const MAREA_SemaName *MAREA_CALL MAREA_SemaLocalFind(const MAREA_SemaContext *context, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = context->locals.count; index > 0; --index) {
        const MAREA_SemaName *local = &context->locals.entries[(index - 1)];

        if (MAREA_SemaTextEquals(local->text, local->length, text, length) == MAREA_BOOLEAN_TRUE) {
            return local;
        }
    }

    return MAREA_NULLPTR;
}

static const MAREA_SemaName *MAREA_CALL MAREA_G_FindLocal(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    return MAREA_SemaLocalFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
}

static const MAREA_AstNode *MAREA_CALL MAREA_G_FindDeclaration(const MAREA_SemaContext *context, const MAREA_AstNode *name, MAREA_AstKind kind)
{
    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (MAREA_G_IsKind(entry->declaration, kind) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    return entry->declaration;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNumberName(const MAREA_G_Shape *shape)
{
    return ((MAREA_G_TextIs(shape->name, shape->length, "string") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(shape->name, shape->length, "boolean") == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsFloatName(const MAREA_G_Shape *shape)
{
    return ((MAREA_G_TextIs(shape->name, shape->length, "float32") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(shape->name, shape->length, "float64") == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNumber(const MAREA_G_Shape *shape)
{
    if ((shape->kind == MAREA_G_SHAPE_KIND_INTEGER_LITERAL) || (shape->kind == MAREA_G_SHAPE_KIND_FLOAT_LITERAL)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((shape->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (MAREA_G_IsNumberName(shape) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_SameName(const MAREA_G_Shape *left, const MAREA_G_Shape *right)
{
    return MAREA_SemaTextEquals(left->name, left->length, right->name, right->length);
}

// INFO: the target is always a primitive. A shape that sema does not know fits, so a type is never guessed.
static MAREA_Boolean MAREA_CALL MAREA_G_Fits(const MAREA_G_Shape *target, const MAREA_G_Shape *actual)
{
    switch (actual->kind) {
    case MAREA_G_SHAPE_KIND_INTEGER_LITERAL:
        return MAREA_G_IsNumberName(target);
    case MAREA_G_SHAPE_KIND_FLOAT_LITERAL:
        return MAREA_G_IsFloatName(target);
    case MAREA_G_SHAPE_KIND_PRIMITIVE:
        return MAREA_G_SameName(target, actual);
    case MAREA_G_SHAPE_KIND_STRUCTURE:
        return MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_TRUE;
    }
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Boolean allowStructure)
{
    if (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass typeClass = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);

    if (typeClass == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, name, length);
    }

    // INFO: an enum is a plain integer and its conversion rule is not decided, so only a structure is a known non-primitive.
    if ((allowStructure == MAREA_BOOLEAN_TRUE) && (typeClass == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_STRUCTURE, name, length);
    }

    return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOf(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth);

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfName(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_SemaName *local = MAREA_G_FindLocal(context, name);

    if (local != MAREA_NULLPTR) {
        return MAREA_G_ShapeOfType(context, local->declaration, MAREA_BOOLEAN_TRUE);
    }

    const MAREA_AstNode *constant = MAREA_G_FindDeclaration(context, name, MAREA_AST_KIND_CONSTANT_DECLARATION);

    if (constant == MAREA_NULLPTR) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
    }

    return MAREA_G_ShapeOfType(context, MAREA_SemaChildAt(constant, (MAREA_AstNodeGetChildCount(constant) - 2)), MAREA_BOOLEAN_FALSE);
}

// INFO: a call has a known result only for a plain function of the file with no generic parameter, never for a name that a local hides.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfCall(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_FindLocal(context, callee) != MAREA_NULLPTR)) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
    }

    const MAREA_AstNode *function = MAREA_G_FindDeclaration(context, callee, MAREA_AST_KIND_FUNCTION_DECLARATION);

    if ((function == MAREA_NULLPTR) || (MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
    }

    return MAREA_G_ShapeOfType(context, MAREA_SemaReturnType(function), MAREA_BOOLEAN_TRUE);
}

static MAREA_G_Shape MAREA_CALL MAREA_G_Combine(const MAREA_G_Shape *left, const MAREA_G_Shape *right)
{
    const MAREA_G_Shape unknown = MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);

    if ((MAREA_G_IsNumber(left) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsNumber(right) == MAREA_BOOLEAN_FALSE)) {
        return ((left->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (right->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (MAREA_G_SameName(left, right) == MAREA_BOOLEAN_TRUE)) ? *left : unknown;
    }

    if ((left->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (right->kind == MAREA_G_SHAPE_KIND_PRIMITIVE)) {
        return (MAREA_G_SameName(left, right) == MAREA_BOOLEAN_TRUE) ? *left : unknown;
    }

    if (left->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) {
        return (MAREA_G_Fits(left, right) == MAREA_BOOLEAN_TRUE) ? *left : unknown;
    }

    if (right->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) {
        return (MAREA_G_Fits(right, left) == MAREA_BOOLEAN_TRUE) ? *right : unknown;
    }

    return ((left->kind == MAREA_G_SHAPE_KIND_FLOAT_LITERAL) || (right->kind == MAREA_G_SHAPE_KIND_FLOAT_LITERAL)) ? MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_FLOAT_LITERAL, MAREA_NULLPTR, 0) : MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_INTEGER_LITERAL, MAREA_NULLPTR, 0);
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfBinary(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth)
{
    switch (MAREA_AstNodeGetOperator(expression)) {
    case MAREA_TOKEN_KIND_KEYWORD_OR:
    case MAREA_TOKEN_KIND_KEYWORD_AND:
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
    case MAREA_TOKEN_KIND_BANG_EQUAL:
    case MAREA_TOKEN_KIND_LESS:
    case MAREA_TOKEN_KIND_LESS_EQUAL:
    case MAREA_TOKEN_KIND_GREATER:
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return MAREA_G_ShapePrimitive("boolean");
    case MAREA_TOKEN_KIND_SHIFT_LEFT:
    case MAREA_TOKEN_KIND_SHIFT_RIGHT:
    case MAREA_TOKEN_KIND_STAR_STAR:
    {
        // INFO: the right side of a shift or a power may have another type, so the result is the type of the left side.
        const MAREA_G_Shape left = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 0), (depth + 1));

        return (MAREA_G_IsNumber(&left) == MAREA_BOOLEAN_TRUE) ? left : MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
    }
    default:
    {
        const MAREA_G_Shape left = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 0), (depth + 1));

        const MAREA_G_Shape right = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 1), (depth + 1));

        return MAREA_G_Combine(&left, &right);
    }
    }
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfUnary(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth)
{
    const MAREA_TokenKind operatorKind = MAREA_AstNodeGetOperator(expression);

    if (operatorKind == MAREA_TOKEN_KIND_KEYWORD_NOT) {
        return MAREA_G_ShapePrimitive("boolean");
    }

    if ((operatorKind != MAREA_TOKEN_KIND_MINUS) && (operatorKind != MAREA_TOKEN_KIND_TILDE) && (operatorKind != MAREA_TOKEN_KIND_KEYWORD_COPY) && (operatorKind != MAREA_TOKEN_KIND_KEYWORD_MOVE)) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
    }

    const MAREA_G_Shape operand = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 0), (depth + 1));

    if ((operatorKind == MAREA_TOKEN_KIND_KEYWORD_COPY) || (operatorKind == MAREA_TOKEN_KIND_KEYWORD_MOVE) || (MAREA_G_IsNumber(&operand) == MAREA_BOOLEAN_TRUE)) {
        return operand;
    }

    return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
}

// INFO: a member or an index access is trusted only when its type is a primitive, because the field type of a generic structure is written in the scope of the structure.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfAccess(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    return MAREA_G_ShapeOfType(context, MAREA_SemaInferType(context, expression), MAREA_BOOLEAN_FALSE);
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOf(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth)
{
    const MAREA_G_Shape unknown = MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);

    if ((expression == MAREA_NULLPTR) || (depth >= MAREA_G_DEPTH_LIMIT)) {
        return unknown;
    }

    switch (MAREA_AstNodeGetKind(expression)) {
    case MAREA_AST_KIND_INTEGER_LITERAL:
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_INTEGER_LITERAL, MAREA_NULLPTR, 0);
    case MAREA_AST_KIND_FLOAT_LITERAL:
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_FLOAT_LITERAL, MAREA_NULLPTR, 0);
    case MAREA_AST_KIND_STRING_LITERAL:
        return MAREA_G_ShapePrimitive("string");
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_G_ShapePrimitive("boolean");
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        return MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 0), (depth + 1));
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_ShapeOfName(context, expression);
    case MAREA_AST_KIND_MEMBER_ACCESS:
    case MAREA_AST_KIND_INDEX_ACCESS:
        return MAREA_G_ShapeOfAccess(context, expression);
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_G_ShapeOfType(context, MAREA_SemaChildAt(expression, 0), MAREA_BOOLEAN_TRUE);
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_ShapeOfCall(context, expression);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_ShapeOfBinary(context, expression, depth);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        return MAREA_G_ShapeOfUnary(context, expression, depth);
    default:
        return unknown;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressionFit(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((type == MAREA_NULLPTR) || (expression == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Shape target = MAREA_G_ShapeOfType(context, type, MAREA_BOOLEAN_FALSE);

    if (target.kind != MAREA_G_SHAPE_KIND_PRIMITIVE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Shape actual = MAREA_G_ShapeOf(context, expression, 0);

    if (MAREA_G_Fits(&target, &actual) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", target.name, target.length);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNamespaceRoot(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_AstNode *declaration = (context->root == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(context->root, MAREA_AST_KIND_NAMESPACE_DECLARATION);

    const MAREA_AstNode *path = (declaration == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(declaration, 0);

    const MAREA_AstNode *first = (MAREA_G_IsKind(path, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_TRUE) ? MAREA_SemaChildAt(path, 0) : MAREA_NULLPTR;

    return ((first != MAREA_NULLPTR) && (MAREA_G_NodeTextIs(first, name) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a constant of an implementation block is written as Type.NAME in the examples, but the grammar does not forbid the bare name inside the block.
static MAREA_Boolean MAREA_CALL MAREA_G_IsImplementationConstant(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if ((context->selfDepth == 0) || (context->root == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(context->root); ++index) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(context->root, index);

        if (MAREA_G_IsKind(implementation, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        for (MAREA_IntegerSize member = 0; member < MAREA_AstNodeGetChildCount(implementation); ++member) {
            const MAREA_AstNode *constant = MAREA_SemaChildAt(implementation, member);

            const MAREA_AstNode *constantName = (MAREA_G_IsKind(constant, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(constant, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

            if ((constantName != MAREA_NULLPTR) && (MAREA_G_NodeTextIs(constantName, name) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: c is the reserved namespace of C, size_of, align_of and convert are the built-in calls and Self is the type of an implementation block.
static MAREA_Boolean MAREA_CALL MAREA_G_IsBuiltInName(const MAREA_AstNode *name)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(name);

    const MAREA_IntegerSize length = MAREA_AstNodeGetTextLength(name);

    return ((MAREA_G_TextIs(text, length, "c") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "convert") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "size_of") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "align_of") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "Self") == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsDeclaredName(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if ((MAREA_G_FindLocal(context, name) != MAREA_NULLPTR) || (MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) != MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_G_IsBuiltInName(name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsNamespaceRoot(context, name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsImplementationConstant(context, name) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNames(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth);

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNamesOfChildren(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_IntegerSize first, MAREA_Uinteger32 depth)
{
    for (MAREA_IntegerSize index = first; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_SemaExcept result = MAREA_G_CheckNames(context, MAREA_SemaChildAt(node, index), (depth + 1));

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: only a name that stands as a value is checked. A member name, a field key, a type and a macro call are not.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckNames(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth)
{
    if ((expression == MAREA_NULLPTR) || (depth >= MAREA_G_DEPTH_LIMIT)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    switch (MAREA_AstNodeGetKind(expression)) {
    case MAREA_AST_KIND_IDENTIFIER:
        if (MAREA_G_IsDeclaredName(context, expression) == MAREA_BOOLEAN_TRUE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME, MAREA_AstNodeGetPosition(expression), "undeclared name", MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression));
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_ARGUMENT_LIST:
    case MAREA_AST_KIND_CALL_EXPRESSION:
    case MAREA_AST_KIND_INDEX_ENTRY:
    case MAREA_AST_KIND_VALUE_ENTRY:
        return MAREA_G_CheckNamesOfChildren(context, expression, 0, depth);
    case MAREA_AST_KIND_MEMBER_ACCESS:
    {
        // INFO: the name that starts a member chain may be a namespace or a type that the file never imports, as stdio in the functions example, so only a value that is not a bare name is checked.
        const MAREA_AstNode *base = MAREA_SemaChildAt(expression, 0);

        return (MAREA_G_IsKind(base, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_CheckNames(context, base, (depth + 1));
    }
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_G_CheckNamesOfChildren(context, expression, 1, depth);
    case MAREA_AST_KIND_FIELD_ENTRY:
        return MAREA_G_CheckNamesOfChildren(context, expression, 1, depth);
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressions(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
    {
        // INFO: the initializer of a constant is typed but its names are not checked, because a constant may use a name of a later declaration.
        const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

        return MAREA_SemaCheckExpressionFit(context, MAREA_SemaChildAt(node, (count - 2)), MAREA_SemaChildAt(node, (count - 1)));
    }
    case MAREA_AST_KIND_LOCAL_DECLARATION:
        return MAREA_G_CheckNames(context, MAREA_SemaChildAt(node, 2), 0);
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
    case MAREA_AST_KIND_IF_STATEMENT:
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
    case MAREA_AST_KIND_WHILE_STATEMENT:
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_CheckNames(context, MAREA_SemaChildAt(node, 0), 0);
    case MAREA_AST_KIND_RETURN_STATEMENT:
    {
        const MAREA_SemaExcept namesResult = MAREA_G_CheckNames(context, MAREA_SemaChildAt(node, 0), 0);

        if (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return namesResult;
        }

        return MAREA_SemaCheckExpressionFit(context, MAREA_SemaReturnType(context->function), MAREA_SemaChildAt(node, 0));
    }
    case MAREA_AST_KIND_ASSIGNMENT:
    {
        const MAREA_SemaExcept targetResult = MAREA_G_CheckNames(context, MAREA_SemaChildAt(node, 0), 0);

        if (targetResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return targetResult;
        }

        const MAREA_SemaExcept valueResult = MAREA_G_CheckNames(context, MAREA_SemaChildAt(node, 1), 0);

        if ((valueResult != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_AstNodeGetOperator(node) != MAREA_TOKEN_KIND_EQUAL)) {
            return valueResult;
        }

        return MAREA_SemaCheckExpressionFit(context, MAREA_SemaInferType(context, MAREA_SemaChildAt(node, 0)), MAREA_SemaChildAt(node, 1));
    }
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}
