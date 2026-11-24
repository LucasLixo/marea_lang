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
    MAREA_G_SHAPE_KIND_ENUM = MAREA_UINTEGER_8(5),
    MAREA_G_SHAPE_KIND_LIBRARY = MAREA_UINTEGER_8(6),
    MAREA_G_SHAPE_KIND_GENERIC = MAREA_UINTEGER_8(7),
    MAREA_G_SHAPE_KIND_BUILTIN = MAREA_UINTEGER_8(8),
    MAREA_G_SHAPE_KIND_NONE = MAREA_UINTEGER_8(9),
} MAREA_G_ShapeKind;

// INFO: what sema knows of the type of an expression. A literal number fits any type of its family, a primitive carries its name.
typedef struct MAREA_G_Shape
{
    const MAREA_AstNode *declaration;
    const MAREA_Character8 *name;
    MAREA_IntegerSize length;
    MAREA_G_ShapeKind kind;
} MAREA_G_Shape;

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeMake(MAREA_G_ShapeKind kind, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    return (MAREA_G_Shape){ .declaration = MAREA_NULLPTR, .name = name, .length = length, .kind = kind };
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeUnknown(void)
{
    return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_UNKNOWN, MAREA_NULLPTR, 0);
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
    return ((MAREA_G_TextIs(shape->name, shape->length, "string") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(shape->name, shape->length, "boolean") == MAREA_BOOLEAN_FALSE) && (MAREA_G_TextIs(shape->name, shape->length, "character") == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
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

// INFO: the target is always a primitive. An enum, a library type, a generic parameter and an owned, task or vector value are known but never compared: their conversion rule is not decided, so each fits and a type is never guessed. A shape that sema does not know fits too.
// INFO: an unknown shape and the four known but undecided shapes (enum, library type, generic parameter, owned, task and vector value) have no conversion rule in sema.
static MAREA_Boolean MAREA_CALL MAREA_G_IsUndecided(const MAREA_G_Shape *shape)
{
    switch (shape->kind) {
    case MAREA_G_SHAPE_KIND_UNKNOWN:
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_LIBRARY:
    case MAREA_G_SHAPE_KIND_GENERIC:
    case MAREA_G_SHAPE_KIND_BUILTIN:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

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
    // INFO: a macro with no value gives nothing, so it fits no type.
    case MAREA_G_SHAPE_KIND_NONE:
        return MAREA_BOOLEAN_FALSE;
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_LIBRARY:
    case MAREA_G_SHAPE_KIND_GENERIC:
    case MAREA_G_SHAPE_KIND_BUILTIN:
    default:
        return MAREA_BOOLEAN_TRUE;
    }
}

// INFO: pointer, slice and a function type are not classified yet and stay unknown. An enum is a plain integer and its conversion rule is not decided.
static MAREA_G_ShapeKind MAREA_CALL MAREA_G_ShapeKindOfClass(MAREA_SemaTypeClass typeClass, const MAREA_AstNode *declaration, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    switch (typeClass) {
    case MAREA_SEMA_TYPE_CLASS_USER:
        if (MAREA_G_IsKind(declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE) {
            return MAREA_G_SHAPE_KIND_STRUCTURE;
        }

        return (MAREA_G_IsKind(declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_SHAPE_KIND_ENUM : MAREA_G_SHAPE_KIND_UNKNOWN;
    case MAREA_SEMA_TYPE_CLASS_EXTERNAL:
        return MAREA_G_SHAPE_KIND_LIBRARY;
    case MAREA_SEMA_TYPE_CLASS_GENERIC:
        return MAREA_G_SHAPE_KIND_GENERIC;
    case MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN:
        return MAREA_G_SHAPE_KIND_BUILTIN;
    case MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN:
        return (MAREA_G_TextIs(name, length, "array") == MAREA_BOOLEAN_TRUE) ? MAREA_G_SHAPE_KIND_BUILTIN : MAREA_G_SHAPE_KIND_UNKNOWN;
    default:
        return MAREA_G_SHAPE_KIND_UNKNOWN;
    }
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Boolean allowNonPrimitive)
{
    if (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_ShapeUnknown();
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass typeClass = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);

    if (typeClass == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, name, length);
    }

    // INFO: a non-primitive is classified only where the type is written in the scope of the expression, so a field or a constant type stays unknown.
    if (allowNonPrimitive == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_ShapeUnknown();
    }

    const MAREA_G_ShapeKind kind = MAREA_G_ShapeKindOfClass(typeClass, declaration, name, length);

    if (kind == MAREA_G_SHAPE_KIND_UNKNOWN) {
        return MAREA_G_ShapeUnknown();
    }

    MAREA_G_Shape shape = MAREA_G_ShapeMake(kind, name, length);

    shape.declaration = declaration;

    return shape;
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOf(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth);

// INFO: a constant of an implementation block is written as Type.NAME in the examples, but the grammar does not forbid the bare name inside the block. Only the block being visited is searched.
static const MAREA_AstNode *MAREA_CALL MAREA_G_FindImplementationConstant(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if (context->implementation == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize member = 0; member < MAREA_AstNodeGetChildCount(context->implementation); ++member) {
        const MAREA_AstNode *constant = MAREA_SemaChildAt(context->implementation, member);

        const MAREA_AstNode *constantName = (MAREA_G_IsKind(constant, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(constant, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

        if ((constantName != MAREA_NULLPTR) && (MAREA_G_NodeTextIs(constantName, name) == MAREA_BOOLEAN_TRUE)) {
            return constant;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfName(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_SemaName *local = MAREA_G_FindLocal(context, name);

    if (local != MAREA_NULLPTR) {
        return MAREA_G_ShapeOfType(context, local->declaration, MAREA_BOOLEAN_TRUE);
    }

    // INFO: inside an implementation block, a constant of the block shadows a top-level constant of the same name.
    const MAREA_AstNode *inner = MAREA_G_FindImplementationConstant(context, name);

    const MAREA_AstNode *constant = (inner != MAREA_NULLPTR) ? inner : MAREA_G_FindDeclaration(context, name, MAREA_AST_KIND_CONSTANT_DECLARATION);

    if (constant == MAREA_NULLPTR) {
        return MAREA_G_ShapeUnknown();
    }

    return MAREA_G_ShapeOfType(context, MAREA_SemaChildAt(constant, (MAREA_AstNodeGetChildCount(constant) - 2)), MAREA_BOOLEAN_FALSE);
}

// INFO: a built-in conversion is convert<T>(x) with one type argument and one argument, where the unit binds no name convert of its own. A local, a function or an import with that name makes it a plain call, as the IR reads it. Returns the type argument, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_ConvertTarget(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_TextIs(MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee), "convert") == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    if ((MAREA_G_FindLocal(context, callee) != MAREA_NULLPTR) || (MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee)) != MAREA_NULLPTR)) {
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

// INFO: a call has a known result only for a plain function of the file with no generic parameter, never for a name that a local hides. The result of convert<T>(x) is T.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfCall(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *convertTarget = MAREA_G_ConvertTarget(context, call);

    if (convertTarget != MAREA_NULLPTR) {
        return MAREA_G_ShapeOfType(context, convertTarget, MAREA_BOOLEAN_TRUE);
    }

    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_FindLocal(context, callee) != MAREA_NULLPTR)) {
        return MAREA_G_ShapeUnknown();
    }

    const MAREA_AstNode *function = MAREA_G_FindDeclaration(context, callee, MAREA_AST_KIND_FUNCTION_DECLARATION);

    if ((function == MAREA_NULLPTR) || (MAREA_G_FindChild(function, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR)) {
        return MAREA_G_ShapeUnknown();
    }

    return MAREA_G_ShapeOfType(context, MAREA_SemaReturnType(function), MAREA_BOOLEAN_TRUE);
}

// INFO: only a literal or a primitive is compared. Any other shape is accepted, because an operator may be declared for a structure or a library type and sema does not decide the other types.
static MAREA_Boolean MAREA_CALL MAREA_G_IsComparable(const MAREA_G_Shape *shape)
{
    return ((shape->kind == MAREA_G_SHAPE_KIND_INTEGER_LITERAL) || (shape->kind == MAREA_G_SHAPE_KIND_FLOAT_LITERAL) || (shape->kind == MAREA_G_SHAPE_KIND_PRIMITIVE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsInteger(const MAREA_G_Shape *shape)
{
    if (shape->kind == MAREA_G_SHAPE_KIND_INTEGER_LITERAL) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((shape->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (MAREA_G_IsNumberName(shape) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsFloatName(shape) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the count of a shift may be any integer of any width, so a shift needs two integers and not two equal types. An unknown shape is accepted.
static MAREA_Boolean MAREA_CALL MAREA_G_ShiftAgree(const MAREA_G_Shape *left, const MAREA_G_Shape *right)
{
    return (((MAREA_G_IsComparable(left) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsInteger(left) == MAREA_BOOLEAN_TRUE)) && ((MAREA_G_IsComparable(right) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsInteger(right) == MAREA_BOOLEAN_TRUE))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_G_Shape MAREA_CALL MAREA_G_Combine(const MAREA_G_Shape *left, const MAREA_G_Shape *right)
{
    const MAREA_G_Shape unknown = MAREA_G_ShapeUnknown();

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
    {
        // INFO: the result is the type of the left side. A shift with a side that is not an integer has no type.
        const MAREA_G_Shape left = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 0), (depth + 1));

        const MAREA_G_Shape right = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 1), (depth + 1));

        return ((MAREA_G_IsInteger(&left) == MAREA_BOOLEAN_TRUE) && (MAREA_G_ShiftAgree(&left, &right) == MAREA_BOOLEAN_TRUE)) ? left : MAREA_G_ShapeUnknown();
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
        return MAREA_G_ShapeUnknown();
    }

    const MAREA_G_Shape operand = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(expression, 0), (depth + 1));

    if ((operatorKind == MAREA_TOKEN_KIND_KEYWORD_COPY) || (operatorKind == MAREA_TOKEN_KIND_KEYWORD_MOVE) || (MAREA_G_IsNumber(&operand) == MAREA_BOOLEAN_TRUE)) {
        return operand;
    }

    return MAREA_G_ShapeUnknown();
}

// INFO: a member or an index access is trusted only when its type is a primitive, because the field type of a generic structure is written in the scope of the structure.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfAccess(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    return MAREA_G_ShapeOfType(context, MAREA_SemaInferType(context, expression), MAREA_BOOLEAN_FALSE);
}

// INFO: a built-in macro gives no value, a string, or a type that sema does not know. A name that no macro has is unknown, and MAREA_SemaCheckMacro reports it.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfMacro(const MAREA_AstNode *call)
{
    const MAREA_AstNode *name = MAREA_SemaChildAt(call, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_G_ShapeUnknown();
    }

    switch (MAREA_SemaMacroValueOf(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name))) {
    case MAREA_SEMA_MACRO_VALUE_NONE:
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_NONE, MAREA_NULLPTR, 0);
    case MAREA_SEMA_MACRO_VALUE_STRING:
        return MAREA_G_ShapePrimitive("string");
    default:
        return MAREA_G_ShapeUnknown();
    }
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOf(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth)
{
    const MAREA_G_Shape unknown = MAREA_G_ShapeUnknown();

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
    case MAREA_AST_KIND_CHARACTER_LITERAL:
        return MAREA_G_ShapePrimitive("character");
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
    case MAREA_AST_KIND_MACRO_CALL:
        return MAREA_G_ShapeOfMacro(expression);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_ShapeOfBinary(context, expression, depth);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        return MAREA_G_ShapeOfUnary(context, expression, depth);
    default:
        return unknown;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsLiteralKind(MAREA_AstKind kind)
{
    return ((kind == MAREA_AST_KIND_INTEGER_LITERAL) || (kind == MAREA_AST_KIND_FLOAT_LITERAL) || (kind == MAREA_AST_KIND_STRING_LITERAL) || (kind == MAREA_AST_KIND_CHARACTER_LITERAL) || (kind == MAREA_AST_KIND_BOOLEAN_LITERAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a value for a function type target. A literal is a mismatch (TYPE_MISMATCH) and a bare name of a function of the
// file, not hidden by a local, must have the signature of the function type (FUNCTION_TYPE_MISMATCH). Any other value is accepted.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFunctionTypeFit(MAREA_SemaContext *context, const MAREA_AstNode *functionType, const MAREA_AstNode *expression)
{
    const MAREA_AstKind KIND = MAREA_AstNodeGetKind(expression);

    if (MAREA_G_IsLiteralKind(KIND) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", "function", strlen("function"));
    }

    if (KIND != MAREA_AST_KIND_IDENTIFIER) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_AstNodeGetText(expression);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(expression);

    const MAREA_AstNode *function = MAREA_G_FindDeclaration(context, expression, MAREA_AST_KIND_FUNCTION_DECLARATION);

    if ((function == MAREA_NULLPTR) || (MAREA_SemaLocalFind(context, name, LENGTH) != MAREA_NULLPTR) || (MAREA_SemaFunctionFitsFunctionType(context, function, functionType) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_FUNCTION_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "function signature does not fit the function type:", name, LENGTH);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressionFit(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((type == MAREA_NULLPTR) || (expression == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *functionType = MAREA_SemaResolveFunctionType(context, type);

    if (functionType != MAREA_NULLPTR) {
        return MAREA_G_CheckFunctionTypeFit(context, functionType, expression);
    }

    const MAREA_G_Shape target = MAREA_G_ShapeOfType(context, type, MAREA_BOOLEAN_FALSE);

    if (target.kind != MAREA_G_SHAPE_KIND_PRIMITIVE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Shape actual = MAREA_G_ShapeOf(context, expression, 0);

    if (MAREA_G_Fits(&target, &actual) == MAREA_BOOLEAN_TRUE) {
        // INFO: a division of the lowest signed value by -1 is checked against the declared type that holds it.
        return MAREA_SemaCheckSignedDivision(context, type, expression);
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", target.name, target.length);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressionFitsPrimitive(MAREA_SemaContext *context, const MAREA_Character8 *primitiveName, MAREA_IntegerSize primitiveLength, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((primitiveName == MAREA_NULLPTR) || (expression == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Shape target = MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, primitiveName, primitiveLength);

    const MAREA_G_Shape actual = MAREA_G_ShapeOf(context, expression, 0);

    if (MAREA_G_Fits(&target, &actual) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", target.name, target.length);
}

static MAREA_Boolean MAREA_CALL MAREA_G_PrimitiveOfShape(const MAREA_G_Shape *shape, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    *outName = MAREA_NULLPTR;
    *outLength = 0;

    if (shape->kind != MAREA_G_SHAPE_KIND_PRIMITIVE) {
        return MAREA_BOOLEAN_FALSE;
    }

    *outName = shape->name;
    *outLength = shape->length;

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaPrimitiveOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    const MAREA_G_Shape shape = MAREA_G_ShapeOfType(context, type, MAREA_BOOLEAN_FALSE);

    return MAREA_G_PrimitiveOfShape(&shape, outName, outLength);
}

MAREA_Boolean MAREA_CALL MAREA_SemaPrimitiveOfExpression(const MAREA_SemaContext *context, const MAREA_AstNode *expression, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    const MAREA_G_Shape shape = MAREA_G_ShapeOf(context, expression, 0);

    return MAREA_G_PrimitiveOfShape(&shape, outName, outLength);
}

MAREA_Boolean MAREA_CALL MAREA_SemaNumberLiteralFitsPrimitive(MAREA_AstKind kind, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    if ((kind != MAREA_AST_KIND_INTEGER_LITERAL) && (kind != MAREA_AST_KIND_FLOAT_LITERAL)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_G_Shape target = MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, name, length);

    const MAREA_G_Shape actual = MAREA_G_ShapeMake((kind == MAREA_AST_KIND_INTEGER_LITERAL) ? MAREA_G_SHAPE_KIND_INTEGER_LITERAL : MAREA_G_SHAPE_KIND_FLOAT_LITERAL, MAREA_NULLPTR, 0);

    return MAREA_G_Fits(&target, &actual);
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaConvertTarget(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    return MAREA_G_ConvertTarget(context, call);
}

// INFO: two primitives agree by exact name and a literal agrees with a primitive as it fits a declared type. Two literals always agree.
static MAREA_Boolean MAREA_CALL MAREA_G_OperandsAgree(const MAREA_G_Shape *left, const MAREA_G_Shape *right)
{
    if ((MAREA_G_IsComparable(left) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsComparable(right) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (left->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) {
        return MAREA_G_Fits(left, right);
    }

    return (right->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) ? MAREA_G_Fits(right, left) : MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsShiftOperator(MAREA_TokenKind operatorKind)
{
    return ((operatorKind == MAREA_TOKEN_KIND_SHIFT_LEFT) || (operatorKind == MAREA_TOKEN_KIND_SHIFT_RIGHT) || (operatorKind == MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL) || (operatorKind == MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a shift and a compound shift follow the shift rule, every other operator and compound operator needs the same type on both sides. The prefix names what is compared.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckPair(MAREA_SemaContext *context, MAREA_SourcePosition position, MAREA_TokenKind operatorKind, const MAREA_G_Shape *left, const MAREA_G_Shape *right, const MAREA_Character8 *prefix)
{
    if (MAREA_G_IsShiftOperator(operatorKind) == MAREA_BOOLEAN_TRUE) {
        if (MAREA_G_ShiftAgree(left, right) == MAREA_BOOLEAN_FALSE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, position, "a shift needs an integer left operand and an integer count");
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (MAREA_G_OperandsAgree(left, right) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: the name that is reported is the primitive side, because a literal has no name.
    const MAREA_G_Shape *known = (left->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) ? left : right;

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, position, prefix, known->name, known->length);
}

// INFO: an opaque structure has no known layout. Only a bare name, or one in parentheses, whose written type is an opaque declaration is classified.
static MAREA_Boolean MAREA_CALL MAREA_G_IsOpaqueType(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    if (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    return ((MAREA_SemaClassifyType(context, type, &declaration, &name, &length) == MAREA_SEMA_TYPE_CLASS_USER) && (MAREA_G_IsKind(declaration, MAREA_AST_KIND_OPAQUE_DECLARATION) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsOpaqueName(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    const MAREA_AstNode *inner = expression;

    for (MAREA_Uinteger32 depth = 0; (depth < MAREA_G_DEPTH_LIMIT) && (MAREA_G_IsKind(inner, MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) == MAREA_BOOLEAN_TRUE); ++depth) {
        inner = MAREA_SemaChildAt(inner, 0);
    }

    if (MAREA_G_IsKind(inner, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaName *local = MAREA_G_FindLocal(context, inner);

    return (local == MAREA_NULLPTR) ? MAREA_BOOLEAN_FALSE : MAREA_G_IsOpaqueType(context, local->declaration);
}

// INFO: the operand rule runs only when the two sides agree in type, so one node never carries TYPE_MISMATCH and an operand code together.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckOperands(MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    const MAREA_AstNode *leftNode = MAREA_SemaChildAt(expression, 0);

    const MAREA_AstNode *rightNode = MAREA_SemaChildAt(expression, 1);

    if ((MAREA_G_IsOpaqueName(context, leftNode) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsOpaqueName(context, rightNode) == MAREA_BOOLEAN_TRUE)) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE, MAREA_AstNodeGetPosition(expression), "an operator cannot take an opaque operand");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Shape left = MAREA_G_ShapeOf(context, leftNode, 0);

    const MAREA_G_Shape right = MAREA_G_ShapeOf(context, rightNode, 0);

    const MAREA_TokenKind operatorKind = MAREA_AstNodeGetOperator(expression);

    const MAREA_SemaExcept result = MAREA_G_CheckPair(context, MAREA_AstNodeGetPosition(expression), operatorKind, &left, &right, "operands of a binary operator differ in type, one is");

    if ((result != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_G_OperandsAgree(&left, &right) == MAREA_BOOLEAN_FALSE)) {
        return result;
    }

    const MAREA_SemaExcept OPERAND = MAREA_SemaCheckOperatorOperands(context, operatorKind, leftNode, rightNode, MAREA_AstNodeGetPosition(expression));

    if (OPERAND != MAREA_SEMA_EXCEPT_SUCCESS) {
        return OPERAND;
    }

    const MAREA_SemaExcept CONSTANT = MAREA_SemaCheckConstantOperands(context, operatorKind, leftNode, rightNode, MAREA_AstNodeGetPosition(expression));

    // INFO: with no declared type here, the signed type of a division comes from an operand.
    return (CONSTANT != MAREA_SEMA_EXCEPT_SUCCESS) ? CONSTANT : MAREA_SemaCheckSignedDivision(context, MAREA_NULLPTR, expression);
}

// INFO: a compound assignment is the same as a = a op b, so its two sides are compared like the operands of the operator.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckCompound(MAREA_SemaContext *context, const MAREA_AstNode *assignment)
{
    const MAREA_AstNode *targetNode = MAREA_SemaChildAt(assignment, 0);

    const MAREA_AstNode *valueNode = MAREA_SemaChildAt(assignment, 1);

    const MAREA_G_Shape target = MAREA_G_ShapeOf(context, targetNode, 0);

    const MAREA_G_Shape value = MAREA_G_ShapeOf(context, valueNode, 0);

    const MAREA_TokenKind operatorKind = MAREA_AstNodeGetOperator(assignment);

    const MAREA_SemaExcept result = MAREA_G_CheckPair(context, MAREA_AstNodeGetPosition(assignment), operatorKind, &target, &value, "sides of a compound assignment differ in type, one is");

    if ((result != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_G_OperandsAgree(&target, &value) == MAREA_BOOLEAN_FALSE)) {
        return result;
    }

    const MAREA_SemaExcept OPERAND = MAREA_SemaCheckOperatorOperands(context, operatorKind, targetNode, valueNode, MAREA_AstNodeGetPosition(assignment));

    return (OPERAND != MAREA_SEMA_EXCEPT_SUCCESS) ? OPERAND : MAREA_SemaCheckConstantOperands(context, operatorKind, targetNode, valueNode, MAREA_AstNodeGetPosition(assignment));
}

// INFO: a conversion is always written. A number goes to a number of any width and sign, a structure to a structure whose layout sema measures as the same (a layout that sema cannot measure is rejected), and any type to itself. A shape that sema does not know fits.
static MAREA_Boolean MAREA_CALL MAREA_G_CanConvert(const MAREA_SemaContext *context, const MAREA_G_Shape *target, const MAREA_G_Shape *operand)
{
    if ((MAREA_G_IsUndecided(target) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsUndecided(operand) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    if (target->kind == MAREA_G_SHAPE_KIND_STRUCTURE) {
        return ((operand->kind == MAREA_G_SHAPE_KIND_STRUCTURE) && (MAREA_SemaCompareStructureLayout(context, operand->declaration, target->declaration) == MAREA_SEMA_LAYOUT_VERDICT_COMPATIBLE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    }

    if (operand->kind == MAREA_G_SHAPE_KIND_STRUCTURE) {
        return MAREA_BOOLEAN_FALSE;
    }

    if ((operand->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (MAREA_G_SameName(target, operand) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_G_IsNumber(target) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsNumber(operand) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckConvert(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *targetType = MAREA_G_ConvertTarget(context, call);

    if (targetType == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Shape target = MAREA_G_ShapeOfType(context, targetType, MAREA_BOOLEAN_TRUE);

    const MAREA_G_Shape operand = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(MAREA_SemaChildAt(call, 2), 0), 0);

    if (MAREA_G_CanConvert(context, &target, &operand) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(call), "convert does not take the operand to the type", target.name, target.length);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNamespaceRoot(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_AstNode *declaration = (context->root == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_G_FindChild(context->root, MAREA_AST_KIND_NAMESPACE_DECLARATION);

    const MAREA_AstNode *path = (declaration == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(declaration, 0);

    const MAREA_AstNode *first = (MAREA_G_IsKind(path, MAREA_AST_KIND_QUALIFIED_NAME) == MAREA_BOOLEAN_TRUE) ? MAREA_SemaChildAt(path, 0) : MAREA_NULLPTR;

    return ((first != MAREA_NULLPTR) && (MAREA_G_NodeTextIs(first, name) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
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

    return ((MAREA_G_IsBuiltInName(name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsNamespaceRoot(context, name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_FindImplementationConstant(context, name) != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a name is declared before it is used, so a bare name that stands for a function of this file that starts after the use is not yet declared. A local of the same name hides the function, and a constant initializer has no function, so it keeps its freedom.
static MAREA_Boolean MAREA_CALL MAREA_G_IsFunctionUsedBeforeDeclaration(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if ((context->function == MAREA_NULLPTR) || (MAREA_G_FindLocal(context, name) != MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

    if ((entry == MAREA_NULLPTR) || (MAREA_G_IsKind(entry->declaration, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_SourcePosition declaration = MAREA_AstNodeGetPosition(entry->declaration);

    const MAREA_SourcePosition use = MAREA_AstNodeGetPosition(name);

    return ((declaration.file == use.file) && (declaration.offset > use.offset)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckValue(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth, MAREA_Boolean checkNames);

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckValueOfChildren(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_IntegerSize first, MAREA_Uinteger32 depth, MAREA_Boolean checkNames)
{
    for (MAREA_IntegerSize index = first; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_SemaExcept result = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, index), (depth + 1), checkNames);

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: only a name that stands as a value is checked, and only when checkNames is true. A member name, a field key, a type and a macro call are not. Every binary expression has its two operands compared, and every built-in conversion its operand, after the expressions inside them.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckValue(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth, MAREA_Boolean checkNames)
{
    if ((expression == MAREA_NULLPTR) || (depth >= MAREA_G_DEPTH_LIMIT)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    switch (MAREA_AstNodeGetKind(expression)) {
    case MAREA_AST_KIND_IDENTIFIER:
        if (checkNames == MAREA_BOOLEAN_FALSE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        if (MAREA_G_IsDeclaredName(context, expression) == MAREA_BOOLEAN_TRUE) {
            return (MAREA_G_IsFunctionUsedBeforeDeclaration(context, expression) == MAREA_BOOLEAN_TRUE) ? MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME, MAREA_AstNodeGetPosition(expression), "function is used before its declaration", MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression)) : MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME, MAREA_AstNodeGetPosition(expression), "undeclared name", MAREA_AstNodeGetText(expression), MAREA_AstNodeGetTextLength(expression));
    case MAREA_AST_KIND_BINARY_EXPRESSION:
    {
        const MAREA_SemaExcept result = MAREA_G_CheckValueOfChildren(context, expression, 0, depth, checkNames);

        return (result != MAREA_SEMA_EXCEPT_SUCCESS) ? result : MAREA_G_CheckOperands(context, expression);
    }
    case MAREA_AST_KIND_CALL_EXPRESSION:
    {
        const MAREA_SemaExcept result = MAREA_G_CheckValueOfChildren(context, expression, 0, depth, checkNames);

        return (result != MAREA_SEMA_EXCEPT_SUCCESS) ? result : MAREA_G_CheckConvert(context, expression);
    }
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    {
        const MAREA_SemaExcept result = MAREA_G_CheckValueOfChildren(context, expression, 0, depth, checkNames);

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_SemaExcept OPERAND = MAREA_SemaCheckOperatorOperands(context, MAREA_AstNodeGetOperator(expression), MAREA_SemaChildAt(expression, 0), MAREA_NULLPTR, MAREA_AstNodeGetPosition(expression));

        // INFO: the rule for a negated unsigned value runs after the operand is checked.
        return (OPERAND != MAREA_SEMA_EXCEPT_SUCCESS) ? OPERAND : MAREA_SemaCheckNegation(context, expression);
    }
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
    case MAREA_AST_KIND_INDEX_ACCESS:
    case MAREA_AST_KIND_ARGUMENT_LIST:
    case MAREA_AST_KIND_INDEX_ENTRY:
    case MAREA_AST_KIND_VALUE_ENTRY:
        return MAREA_G_CheckValueOfChildren(context, expression, 0, depth, checkNames);
    case MAREA_AST_KIND_MEMBER_ACCESS:
    {
        // INFO: the name that starts a member chain is checked like any other name, so a namespace that no import binds, or that only its original name reaches through an alias, is undeclared.
        const MAREA_AstNode *base = MAREA_SemaChildAt(expression, 0);

        if (MAREA_G_IsOpaqueName(context, base) == MAREA_BOOLEAN_TRUE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE, MAREA_AstNodeGetPosition(expression), "an opaque type has no fields");

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_G_CheckValue(context, base, (depth + 1), checkNames);
    }
    case MAREA_AST_KIND_LITERAL_VALUE:
        if (MAREA_G_IsOpaqueType(context, MAREA_SemaChildAt(expression, 0)) == MAREA_BOOLEAN_TRUE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE, MAREA_AstNodeGetPosition(expression), "an opaque type cannot be built with a literal");

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        return MAREA_G_CheckValueOfChildren(context, expression, 1, depth, checkNames);
    case MAREA_AST_KIND_FIELD_ENTRY:
        return MAREA_G_CheckValueOfChildren(context, expression, 1, depth, checkNames);
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}

// INFO: a node that the parser could not build leaves tokens behind that it may read as an expression, so a declaration with an error node below it has no name check.
static MAREA_Boolean MAREA_CALL MAREA_G_HasErrorNode(const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    if ((node == MAREA_NULLPTR) || (depth >= MAREA_G_DEPTH_LIMIT)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_AstNodeGetKind(node) == MAREA_AST_KIND_ERROR_NODE) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        if (MAREA_G_HasErrorNode(MAREA_SemaChildAt(node, index), (depth + 1)) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressions(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_CONSTANT_DECLARATION:
    {
        // INFO: every declaration of the file is in the name table before the first check, so a constant may use a name of a later declaration. A constant with an error node below it is typed but its names are not checked.
        const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

        const MAREA_SemaExcept operandsResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, (count - 1)), 0, (MAREA_G_HasErrorNode(node, 0) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE);

        if (operandsResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return operandsResult;
        }

        const MAREA_SemaExcept fitResult = MAREA_SemaCheckExpressionFit(context, MAREA_SemaChildAt(node, (count - 2)), MAREA_SemaChildAt(node, (count - 1)));

        return (fitResult != MAREA_SEMA_EXCEPT_SUCCESS) ? fitResult : MAREA_SemaCheckLiteralRange(context, MAREA_SemaChildAt(node, (count - 2)), MAREA_SemaChildAt(node, (count - 1)));
    }
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_SemaCheckLiteralFields(context, node);
    case MAREA_AST_KIND_LOCAL_DECLARATION:
    {
        const MAREA_SemaExcept pointerResult = MAREA_SemaCheckRawPointerLocal(context, node);

        if (pointerResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return pointerResult;
        }

        const MAREA_SemaExcept initializerResult = MAREA_SemaCheckLocalInitializer(context, node);

        if (initializerResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return initializerResult;
        }

        return MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 2), 0, MAREA_BOOLEAN_TRUE);
    }
    case MAREA_AST_KIND_PARAMETER:
    {
        // INFO: the default value is the optional third child, and it must fit the type of its parameter.
        if (MAREA_SemaParameterHasDefault(node) == MAREA_BOOLEAN_FALSE) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_SemaExcept valueResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 2), 0, MAREA_BOOLEAN_TRUE);

        if (valueResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return valueResult;
        }

        const MAREA_SemaExcept fitResult = MAREA_SemaCheckExpressionFit(context, MAREA_SemaChildAt(node, 1), MAREA_SemaChildAt(node, 2));

        return (fitResult != MAREA_SEMA_EXCEPT_SUCCESS) ? fitResult : MAREA_SemaCheckLiteralRange(context, MAREA_SemaChildAt(node, 1), MAREA_SemaChildAt(node, 2));
    }
    case MAREA_AST_KIND_PARAMETER_LIST:
    {
        // INFO: a defaulted parameter is reported once when any required parameter follows it.
        MAREA_Boolean requiredFollows = MAREA_BOOLEAN_FALSE;

        for (MAREA_IntegerSize index = MAREA_AstNodeGetChildCount(node); index > 0; --index) {
            const MAREA_AstNode *parameter = MAREA_SemaChildAt(node, (index - 1));

            if ((parameter == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(parameter) != MAREA_AST_KIND_PARAMETER)) {
                continue;
            }

            if (MAREA_SemaParameterHasDefault(parameter) == MAREA_BOOLEAN_FALSE) {
                requiredFollows = MAREA_BOOLEAN_TRUE;
            } else if (requiredFollows == MAREA_BOOLEAN_TRUE) {
                const MAREA_AstNode *name = MAREA_SemaChildAt(parameter, 0);

                const MAREA_SemaExcept reportResult = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_DEFAULT_PARAMETER_BEFORE_REQUIRED, MAREA_AstNodeGetPosition(parameter), "a default value comes before a required parameter:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));

                if (reportResult != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return reportResult;
                }
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
    {
        const MAREA_SemaExcept firstResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 1), 0, MAREA_BOOLEAN_TRUE);

        return (firstResult != MAREA_SEMA_EXCEPT_SUCCESS) ? firstResult : MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 2), 0, MAREA_BOOLEAN_TRUE);
    }
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    {
        // INFO: the children are the variables, the collection and the body, so the collection is the child before the last.
        const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

        return (count < 3) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, (count - 2)), 0, MAREA_BOOLEAN_TRUE);
    }
    case MAREA_AST_KIND_WITH_STATEMENT:
        return MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);
    case MAREA_AST_KIND_PARTITION_STATEMENT:
    {
        const MAREA_SemaExcept valueResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);

        return (valueResult != MAREA_SEMA_EXCEPT_SUCCESS) ? valueResult : MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 1), 0, MAREA_BOOLEAN_TRUE);
    }
    case MAREA_AST_KIND_IF_STATEMENT:
    case MAREA_AST_KIND_ELSEIF_CLAUSE:
    case MAREA_AST_KIND_WHILE_STATEMENT:
    {
        const MAREA_SemaExcept namesResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);

        return (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) ? namesResult : MAREA_SemaCheckCondition(context, MAREA_SemaChildAt(node, 0));
    }
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
    case MAREA_AST_KIND_SWITCH_STATEMENT:
        return MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);
    case MAREA_AST_KIND_RETURN_STATEMENT:
    {
        const MAREA_SemaExcept namesResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);

        if (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return namesResult;
        }

        const MAREA_SemaExcept fitResult = MAREA_SemaCheckExpressionFit(context, MAREA_SemaReturnType(context->function), MAREA_SemaChildAt(node, 0));

        return (fitResult != MAREA_SEMA_EXCEPT_SUCCESS) ? fitResult : MAREA_SemaCheckLiteralRange(context, MAREA_SemaReturnType(context->function), MAREA_SemaChildAt(node, 0));
    }
    case MAREA_AST_KIND_ASSIGNMENT:
    {
        const MAREA_SemaExcept targetResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);

        if (targetResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return targetResult;
        }

        const MAREA_SemaExcept valueResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 1), 0, MAREA_BOOLEAN_TRUE);

        if (valueResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return valueResult;
        }

        const MAREA_SemaExcept constantResult = MAREA_SemaCheckConstantAssignment(context, node);

        if (constantResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return constantResult;
        }

        if (MAREA_AstNodeGetOperator(node) != MAREA_TOKEN_KIND_EQUAL) {
            return MAREA_G_CheckCompound(context, node);
        }

        const MAREA_SemaExcept fitResult = MAREA_SemaCheckExpressionFit(context, MAREA_SemaInferType(context, MAREA_SemaChildAt(node, 0)), MAREA_SemaChildAt(node, 1));

        return (fitResult != MAREA_SEMA_EXCEPT_SUCCESS) ? fitResult : MAREA_SemaCheckLiteralRange(context, MAREA_SemaInferType(context, MAREA_SemaChildAt(node, 0)), MAREA_SemaChildAt(node, 1));
    }
    default:
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
}
