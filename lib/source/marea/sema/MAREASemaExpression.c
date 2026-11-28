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
    MAREA_G_SHAPE_KIND_REGISTERED = MAREA_UINTEGER_8(10),
    MAREA_G_SHAPE_KIND_VIEW = MAREA_UINTEGER_8(11),
    MAREA_G_SHAPE_KIND_FUNCTION = MAREA_UINTEGER_8(12),
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

static MAREA_SemaExcept MAREA_CALL MAREA_G_AppendLocal(MAREA_SemaContext *context, const MAREA_AstNode *name, const MAREA_AstNode *type)
{
    if (MAREA_G_IsKind(name, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: the entry has the given type or none, so a name that it hides is never typed from an outer local of the same name.
    return MAREA_SemaTableAppend(context, &context->locals, (MAREA_SemaName){ .text = MAREA_AstNodeGetText(name), .declaration = type, .length = MAREA_AstNodeGetTextLength(name), .kind = MAREA_SEMA_NAME_KIND_DECLARATION });
}

MAREA_Boolean MAREA_CALL MAREA_SemaBinderRange(const MAREA_AstNode *node, MAREA_IntegerSize *outFirst, MAREA_IntegerSize *outEnd)
{
    if ((node == MAREA_NULLPTR) || (outFirst == MAREA_NULLPTR) || (outEnd == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    // INFO: the places of the names that a statement declares. A for each has its variables before the collection and the body.
    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        *outFirst = 0;
        *outEnd = 1;
        break;
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
        *outFirst = 0;
        *outEnd = (COUNT > 2) ? (COUNT - 2) : 0;
        break;
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        *outFirst = 2;
        *outEnd = 3;
        break;
    case MAREA_AST_KIND_WITH_STATEMENT:
        *outFirst = 1;
        *outEnd = 2;
        break;
    default:
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_IntegerSize MAREA_CALL MAREA_SemaBinderCount(const MAREA_AstNode *node)
{
    MAREA_IntegerSize first = 0;

    MAREA_IntegerSize end = 0;

    if (MAREA_SemaBinderRange(node, &first, &end) == MAREA_BOOLEAN_FALSE) {
        return 0;
    }

    MAREA_IntegerSize binders = 0;

    for (MAREA_IntegerSize index = first; index < end; ++index) {
        if (MAREA_G_IsKind(MAREA_SemaChildAt(node, index), MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
            ++binders;
        }
    }

    return binders;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaDeclareBinders(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_FOR_RANGE_STATEMENT:
        return MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, 0), MAREA_NULLPTR);
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    {
        // INFO: the children are the variables, the collection and the body, so the variables end two places before the end.
        const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

        if ((count < 3) || (MAREA_G_IsKind(MAREA_SemaChildAt(node, (count - 1)), MAREA_AST_KIND_BLOCK) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        // INFO: the last variable holds the element, so it has the element type of the collection. With two variables the first is the index and stays untyped.
        const MAREA_AstNode *elementType = MAREA_SemaElementType(context, MAREA_SemaChildAt(node, (count - 2)));

        for (MAREA_IntegerSize index = 0; index < (count - 2); ++index) {
            const MAREA_SemaExcept result = MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, index), (index == (count - 3)) ? elementType : MAREA_NULLPTR);

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }
        }

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }
    case MAREA_AST_KIND_PARTITION_STATEMENT:
        return MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, 2), MAREA_NULLPTR);
    case MAREA_AST_KIND_WITH_STATEMENT:
        return MAREA_G_AppendLocal(context, MAREA_SemaChildAt(node, 1), MAREA_NULLPTR);
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

// INFO: the target is a primitive, a view or a raw pointer. An enum, a library type, a generic parameter and an owned, task or vector value have no conversion rule, so each fits and a type is never guessed.
// INFO: the shapes that have no conversion rule for convert<T>(x): an unknown shape, the known but undecided shapes, and a view or a raw pointer.
static MAREA_Boolean MAREA_CALL MAREA_G_IsUndecided(const MAREA_G_Shape *shape)
{
    switch (shape->kind) {
    case MAREA_G_SHAPE_KIND_UNKNOWN:
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_LIBRARY:
    case MAREA_G_SHAPE_KIND_GENERIC:
    case MAREA_G_SHAPE_KIND_BUILTIN:
    case MAREA_G_SHAPE_KIND_VIEW:
    case MAREA_G_SHAPE_KIND_FUNCTION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: a view and a raw pointer take only a value of their own kind. The type arguments and the head are compared before this.
static MAREA_Boolean MAREA_CALL MAREA_G_FitsView(const MAREA_G_Shape *actual)
{
    switch (actual->kind) {
    case MAREA_G_SHAPE_KIND_INTEGER_LITERAL:
    case MAREA_G_SHAPE_KIND_FLOAT_LITERAL:
    case MAREA_G_SHAPE_KIND_PRIMITIVE:
    case MAREA_G_SHAPE_KIND_STRUCTURE:
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_NONE:
    case MAREA_G_SHAPE_KIND_FUNCTION:
        return MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_TRUE;
    }
}

static MAREA_Boolean MAREA_CALL MAREA_G_Fits(const MAREA_G_Shape *target, const MAREA_G_Shape *actual)
{
    if (target->kind == MAREA_G_SHAPE_KIND_VIEW) {
        return MAREA_G_FitsView(actual);
    }

    switch (actual->kind) {
    case MAREA_G_SHAPE_KIND_INTEGER_LITERAL:
        return MAREA_G_IsNumberName(target);
    case MAREA_G_SHAPE_KIND_FLOAT_LITERAL:
        return MAREA_G_IsFloatName(target);
    case MAREA_G_SHAPE_KIND_PRIMITIVE:
        return MAREA_G_SameName(target, actual);
    case MAREA_G_SHAPE_KIND_STRUCTURE:
    // INFO: a macro with no value gives nothing, so it fits no type. The value of a macro that a library registers is a library type, so it fits no primitive.
    case MAREA_G_SHAPE_KIND_NONE:
    case MAREA_G_SHAPE_KIND_REGISTERED:
    case MAREA_G_SHAPE_KIND_VIEW:
    case MAREA_G_SHAPE_KIND_FUNCTION:
        return MAREA_BOOLEAN_FALSE;
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_LIBRARY:
    case MAREA_G_SHAPE_KIND_GENERIC:
    case MAREA_G_SHAPE_KIND_BUILTIN:
    default:
        return MAREA_BOOLEAN_TRUE;
    }
}

// INFO: a registered type with the view role or the raw pointer role is a VIEW shape; its element type is the first type argument, which MAREA_SemaInferType reads. A function type is classified before this. An enum is a plain integer and its conversion rule is not decided.
static MAREA_G_ShapeKind MAREA_CALL MAREA_G_ShapeKindOfClass(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_SemaTypeClass typeClass, const MAREA_AstNode *declaration)
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
    case MAREA_SEMA_TYPE_CLASS_TUPLE:
    case MAREA_SEMA_TYPE_CLASS_MOVE_BUILTIN:
        return MAREA_G_SHAPE_KIND_BUILTIN;
    case MAREA_SEMA_TYPE_CLASS_COPY_BUILTIN:
    {
        // INFO: a copied inline run (an index-protocol type with no handle role) is a built-in shape.
        const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

        if (MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_FALSE) {
            return MAREA_G_SHAPE_KIND_UNKNOWN;
        }

        if ((descriptor->roles & (MAREA_TYPE_ROLE_VIEW | MAREA_TYPE_ROLE_RAW_POINTER)) != MAREA_TYPE_ROLE_NONE) {
            return MAREA_G_SHAPE_KIND_VIEW;
        }

        return ((descriptor->hasIndexProtocol == MAREA_BOOLEAN_TRUE) && (descriptor->roles == MAREA_TYPE_ROLE_NONE)) ? MAREA_G_SHAPE_KIND_BUILTIN : MAREA_G_SHAPE_KIND_UNKNOWN;
    }
    default:
        return MAREA_G_SHAPE_KIND_UNKNOWN;
    }
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    if (MAREA_G_IsType(type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_ShapeUnknown();
    }

    if (MAREA_SemaResolveFunctionType(context, type) != MAREA_NULLPTR) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_FUNCTION, "function", strlen("function"));
    }

    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_SemaTypeClass typeClass = MAREA_SemaClassifyType(context, type, &declaration, &name, &length);

    if (typeClass == MAREA_SEMA_TYPE_CLASS_PRIMITIVE) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, name, length);
    }

    const MAREA_G_ShapeKind kind = MAREA_G_ShapeKindOfClass(context, type, typeClass, declaration);

    if (kind == MAREA_G_SHAPE_KIND_UNKNOWN) {
        return MAREA_G_ShapeUnknown();
    }

    MAREA_G_Shape shape = MAREA_G_ShapeMake(kind, name, length);

    shape.declaration = declaration;

    return shape;
}

// INFO: a bare generic name has the shape of its bound type, and Self the shape of the implemented type.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfWritten(const MAREA_SemaContext *context, MAREA_SemaWrittenType *written)
{
    MAREA_SemaResolveWrittenType(context, written);

    return MAREA_G_ShapeOfType(context, written->type);
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

// INFO: inside an implementation block, a constant of the block shadows a top-level constant of the same name.
const MAREA_AstNode *MAREA_CALL MAREA_SemaFindConstant(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    const MAREA_AstNode *inner = MAREA_G_FindImplementationConstant(context, name);

    return (inner != MAREA_NULLPTR) ? inner : MAREA_G_FindDeclaration(context, name, MAREA_AST_KIND_CONSTANT_DECLARATION);
}

static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfDeclared(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    MAREA_SemaWrittenType written;

    return (MAREA_SemaWrittenTypeOf(context, expression, &written) == MAREA_BOOLEAN_TRUE) ? MAREA_G_ShapeOfWritten(context, &written) : MAREA_G_ShapeUnknown();
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

// INFO: a call on the name of a structure or an enum is a construction, so it has that type. Every other call has the written result of its callee, and convert<T>(x) is T.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfCall(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) && (MAREA_G_FindLocal(context, callee) == MAREA_NULLPTR)) {
        const MAREA_AstNode *structure = MAREA_G_FindDeclaration(context, callee, MAREA_AST_KIND_STRUCTURE_DECLARATION);

        const MAREA_AstNode *declaration = (structure != MAREA_NULLPTR) ? structure : MAREA_G_FindDeclaration(context, callee, MAREA_AST_KIND_ENUM_DECLARATION);

        if (declaration != MAREA_NULLPTR) {
            MAREA_G_Shape shape = MAREA_G_ShapeMake((structure != MAREA_NULLPTR) ? MAREA_G_SHAPE_KIND_STRUCTURE : MAREA_G_SHAPE_KIND_ENUM, MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));

            shape.declaration = declaration;

            return shape;
        }
    }

    // INFO: the count of an inline run is a uinteger64 and its empty test a boolean.
    MAREA_SemaInlineRunCall runCall;

    if (MAREA_SemaInlineRunCallOf(context, call, &runCall) == MAREA_BOOLEAN_TRUE) {
        if ((runCall.member == MAREA_SEMA_INLINE_RUN_MEMBER_SIZE) || (runCall.member == MAREA_SEMA_INLINE_RUN_MEMBER_MAX_SIZE) || (runCall.member == MAREA_SEMA_INLINE_RUN_MEMBER_CAPACITY)) {
            return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, "uinteger64", strlen("uinteger64"));
        }

        if (runCall.member == MAREA_SEMA_INLINE_RUN_MEMBER_EMPTY) {
            return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, "boolean", strlen("boolean"));
        }
    }

    return MAREA_G_ShapeOfDeclared(context, call);
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

// INFO: a build_value call has the type of its second argument and a registered macro has the result type of its descriptor. A built-in macro gives no value, a string, a uinteger32, or a type that sema does not know. A name that no macro has is unknown, and MAREA_SemaCheckMacro reports it.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfMacro(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_SemaMacroType TYPE = MAREA_SemaMacroTypeOf(context, call);

    if (TYPE.kind == MAREA_SEMA_MACRO_TYPE_KIND_PRIMITIVE) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_PRIMITIVE, TYPE.name, TYPE.length);
    }

    if (TYPE.kind == MAREA_SEMA_MACRO_TYPE_KIND_ENUM) {
        MAREA_G_Shape shape = MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_ENUM, TYPE.name, TYPE.length);

        shape.declaration = TYPE.declaration;

        return shape;
    }

    if (TYPE.kind == MAREA_SEMA_MACRO_TYPE_KIND_REGISTERED) {
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_REGISTERED, TYPE.name, TYPE.length);
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(call, 0);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_G_ShapeUnknown();
    }

    switch (MAREA_SemaMacroValueOf(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name))) {
    case MAREA_SEMA_MACRO_VALUE_NONE:
        return MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_NONE, MAREA_NULLPTR, 0);
    case MAREA_SEMA_MACRO_VALUE_STRING:
        return MAREA_G_ShapePrimitive("string");
    case MAREA_SEMA_MACRO_VALUE_UINTEGER_32:
        return MAREA_G_ShapePrimitive("uinteger32");
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
        return MAREA_G_ShapeOfDeclared(context, expression);
    case MAREA_AST_KIND_MEMBER_ACCESS:
    case MAREA_AST_KIND_INDEX_ACCESS:
        return MAREA_G_ShapeOfDeclared(context, expression);
    case MAREA_AST_KIND_LITERAL_VALUE:
        return MAREA_G_ShapeOfDeclared(context, expression);
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_ShapeOfCall(context, expression);
    case MAREA_AST_KIND_MACRO_CALL:
        return MAREA_G_ShapeOfMacro(context, expression);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_ShapeOfBinary(context, expression, depth);
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        return MAREA_G_ShapeOfUnary(context, expression, depth);
    default:
        return unknown;
    }
}

// INFO: a value of a shape that is surely no function.
static MAREA_Boolean MAREA_CALL MAREA_G_IsNoFunction(const MAREA_G_Shape *shape)
{
    switch (shape->kind) {
    case MAREA_G_SHAPE_KIND_INTEGER_LITERAL:
    case MAREA_G_SHAPE_KIND_FLOAT_LITERAL:
    case MAREA_G_SHAPE_KIND_PRIMITIVE:
    case MAREA_G_SHAPE_KIND_STRUCTURE:
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_VIEW:
    case MAREA_G_SHAPE_KIND_NONE:
    case MAREA_G_SHAPE_KIND_REGISTERED:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: a value for a function type target. A bare name of a function of the file, not hidden by a local, must have the
// signature of the function type, and so must a value that has a written function type (FUNCTION_TYPE_MISMATCH). A value
// that is surely no function is TYPE_MISMATCH. Any other value is accepted.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFunctionTypeFit(MAREA_SemaContext *context, const MAREA_SemaWrittenType *target, const MAREA_AstNode *expression)
{
    if (MAREA_G_IsKind(expression, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *name = MAREA_AstNodeGetText(expression);

        const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(expression);

        const MAREA_AstNode *function = MAREA_G_FindDeclaration(context, expression, MAREA_AST_KIND_FUNCTION_DECLARATION);

        if ((function != MAREA_NULLPTR) && (MAREA_SemaLocalFind(context, name, LENGTH) == MAREA_NULLPTR)) {
            return (MAREA_SemaFunctionFitsFunctionType(context, function, target->type, &target->bindings) == MAREA_BOOLEAN_TRUE) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_FUNCTION_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "function signature does not fit the function type:", name, LENGTH);
        }
    }

    MAREA_SemaWrittenType value;

    if (MAREA_SemaWrittenTypeOf(context, expression, &value) == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaResolveWrittenType(context, &value);

        value.type = MAREA_SemaResolveFunctionType(context, value.type);

        if (value.type != MAREA_NULLPTR) {
            return (MAREA_SemaFunctionTypesFit(context, target, &value) == MAREA_BOOLEAN_TRUE) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_FUNCTION_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "the signature of the value does not fit the", "function type", strlen("function type"));
        }
    }

    const MAREA_G_Shape actual = MAREA_G_ShapeOf(context, expression, 0);

    if (MAREA_G_IsNoFunction(&actual) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", "function", strlen("function"));
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckWrittenFit(MAREA_SemaContext *context, const MAREA_SemaWrittenType *target, const MAREA_AstNode *expression)
{
    if ((context == MAREA_NULLPTR) || (target == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_SemaWrittenType resolved = *target;

    // INFO: a bare generic name is compared as its bound type, and not at all while nothing bound it.
    MAREA_SemaResolveWrittenType(context, &resolved);

    const MAREA_AstNode *type = resolved.type;

    if ((type == MAREA_NULLPTR) || (expression == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaWrittenType functionTarget = resolved;

    functionTarget.type = MAREA_SemaResolveFunctionType(context, type);

    if (functionTarget.type != MAREA_NULLPTR) {
        return MAREA_G_CheckFunctionTypeFit(context, &functionTarget, expression);
    }

    if (MAREA_SemaTypeArgumentsDiffer(context, &resolved, expression) == MAREA_BOOLEAN_TRUE) {
        const MAREA_AstNode *head = MAREA_SemaChildAt(MAREA_SemaChildAt(type, 0), 0);

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, MAREA_AstNodeGetPosition(expression), "type arguments differ from the declared type of", MAREA_AstNodeGetText(head), MAREA_AstNodeGetTextLength(head));
    }

    const MAREA_G_Shape shape = MAREA_G_ShapeOfType(context, type);

    if ((shape.kind != MAREA_G_SHAPE_KIND_PRIMITIVE) && (shape.kind != MAREA_G_SHAPE_KIND_VIEW)) {
        // INFO: a library type that a macro of a library gives is compared by its type arguments, the element type and the count.
        return MAREA_SemaCheckMacroResultFit(context, type, expression);
    }

    const MAREA_G_Shape actual = MAREA_G_ShapeOf(context, expression, 0);

    if (MAREA_G_Fits(&shape, &actual) == MAREA_BOOLEAN_TRUE) {
        // INFO: a division of the lowest signed value by -1 is checked against the declared type that holds it.
        return MAREA_SemaCheckSignedDivision(context, type, expression);
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "value does not fit the type", shape.name, shape.length);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckExpressionFit(MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *expression)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_SemaWrittenType TARGET = { .type = type, .bindings = { .count = 0 } };

    return MAREA_SemaCheckWrittenFit(context, &TARGET, expression);
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

    // INFO: a host signature may name a structure of the unit, and a value of that structure fits its own name.
    if ((actual.kind == MAREA_G_SHAPE_KIND_STRUCTURE) && (MAREA_G_SameName(&target, &actual) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a host signature may name a view, a raw pointer or a function type, and sema does not compare the text of that name.
    if (((actual.kind == MAREA_G_SHAPE_KIND_VIEW) || (actual.kind == MAREA_G_SHAPE_KIND_FUNCTION)) && (MAREA_SemaIsPrimitiveTypeName(primitiveName, primitiveLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    // INFO: a number literal is a value of a primitive only, never of a type that a signature names.
    const MAREA_Boolean NUMBER_LITERAL = ((actual.kind == MAREA_G_SHAPE_KIND_INTEGER_LITERAL) || (actual.kind == MAREA_G_SHAPE_KIND_FLOAT_LITERAL));
    const MAREA_Boolean OTHER_NAME = (NUMBER_LITERAL && (MAREA_SemaIsPrimitiveTypeName(primitiveName, primitiveLength) == MAREA_BOOLEAN_FALSE));

    if ((OTHER_NAME == MAREA_BOOLEAN_FALSE) && (MAREA_G_Fits(&target, &actual) == MAREA_BOOLEAN_TRUE)) {
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
    const MAREA_G_Shape shape = MAREA_G_ShapeOfType(context, type);

    return MAREA_G_PrimitiveOfShape(&shape, outName, outLength);
}

MAREA_Boolean MAREA_CALL MAREA_SemaPrimitiveOfExpression(const MAREA_SemaContext *context, const MAREA_AstNode *expression, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    const MAREA_G_Shape shape = MAREA_G_ShapeOf(context, expression, 0);

    return MAREA_G_PrimitiveOfShape(&shape, outName, outLength);
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsSurelyNotBoolean(const MAREA_SemaContext *context, const MAREA_AstNode *expression, const MAREA_Character8 **outName, MAREA_IntegerSize *outLength)
{
    const MAREA_G_Shape shape = MAREA_G_ShapeOf(context, expression, 0);

    *outName = (shape.name == MAREA_NULLPTR) ? "another type" : shape.name;
    *outLength = (shape.name == MAREA_NULLPTR) ? strlen("another type") : shape.length;

    switch (shape.kind) {
    case MAREA_G_SHAPE_KIND_PRIMITIVE:
        return (MAREA_G_TextIs(shape.name, shape.length, "boolean") == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
    // INFO: an unknown shape, a generic parameter and a library type may still be a boolean.
    case MAREA_G_SHAPE_KIND_UNKNOWN:
    case MAREA_G_SHAPE_KIND_GENERIC:
    case MAREA_G_SHAPE_KIND_LIBRARY:
        return MAREA_BOOLEAN_FALSE;
    default:
        return MAREA_BOOLEAN_TRUE;
    }
}

MAREA_Boolean MAREA_CALL MAREA_SemaIsSurelyNotPrintable(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    const MAREA_G_Shape shape = MAREA_G_ShapeOf(context, expression, 0);

    switch (shape.kind) {
    case MAREA_G_SHAPE_KIND_STRUCTURE:
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_BUILTIN:
    case MAREA_G_SHAPE_KIND_NONE:
    case MAREA_G_SHAPE_KIND_REGISTERED:
    case MAREA_G_SHAPE_KIND_VIEW:
    case MAREA_G_SHAPE_KIND_FUNCTION:
        return MAREA_BOOLEAN_TRUE;
    // INFO: a literal and a primitive print, and an unknown shape, a generic parameter and a library type may print.
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

// INFO: a registered type has an array shape when it has an index or gives a partition part; an owner holds a value that sema does not unwrap, so it is accepted.
static MAREA_Boolean MAREA_CALL MAREA_G_IsRegisteredNotArray(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    MAREA_SemaWrittenType written;

    if (MAREA_SemaWrittenTypeOf(context, expression, &written) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_SemaResolveWrittenType(context, &written);

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if (MAREA_SemaDescriptorOfType(context, written.type, &descriptor) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((descriptor->hasIndexProtocol == MAREA_BOOLEAN_FALSE) && ((descriptor->roles & (MAREA_TYPE_ROLE_PARTITION_PART | MAREA_TYPE_ROLE_OWNER)) == MAREA_TYPE_ROLE_NONE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_SemaExpressionIsKnownNotArray(const MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    const MAREA_G_Shape shape = MAREA_G_ShapeOf(context, expression, 0);

    // INFO: a type that sema cannot trace to a registered descriptor is accepted, so a shape is never guessed.
    switch (shape.kind) {
    case MAREA_G_SHAPE_KIND_INTEGER_LITERAL:
    case MAREA_G_SHAPE_KIND_FLOAT_LITERAL:
    case MAREA_G_SHAPE_KIND_PRIMITIVE:
    case MAREA_G_SHAPE_KIND_STRUCTURE:
    case MAREA_G_SHAPE_KIND_ENUM:
    case MAREA_G_SHAPE_KIND_FUNCTION:
        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_G_IsRegisteredNotArray(context, expression);
    }
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

// INFO: an opaque structure has no known layout. Only a bare name, or one in parentheses, whose written type stands for an opaque declaration, of this file or of a FOUND namespace, is classified.
static MAREA_Boolean MAREA_CALL MAREA_G_IsOpaqueType(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    MAREA_SemaTypeTarget target = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    return ((MAREA_SemaFindTypeTarget(context, type, MAREA_NULLPTR, &target) == MAREA_BOOLEAN_TRUE) && (MAREA_G_IsKind(target.declaration, MAREA_AST_KIND_OPAQUE_DECLARATION) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
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

// INFO: no contract has an operator, so no bound gives one: an operator with a generic side is an error in the definition.
static MAREA_Boolean MAREA_CALL MAREA_G_HasGenericSide(const MAREA_G_Shape *left, const MAREA_G_Shape *right)
{
    return ((left->kind == MAREA_G_SHAPE_KIND_GENERIC) || (right->kind == MAREA_G_SHAPE_KIND_GENERIC)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static const MAREA_Character8 *MAREA_CALL MAREA_G_OperatorText(MAREA_TokenKind operatorKind)
{
    switch (operatorKind) {
    case MAREA_TOKEN_KIND_PLUS:
        return "+";
    case MAREA_TOKEN_KIND_MINUS:
        return "-";
    case MAREA_TOKEN_KIND_STAR:
        return "*";
    case MAREA_TOKEN_KIND_SLASH:
        return "/";
    case MAREA_TOKEN_KIND_PERCENT:
        return "%";
    case MAREA_TOKEN_KIND_STAR_STAR:
        return "**";
    case MAREA_TOKEN_KIND_PLUS_PERCENT:
        return "+%";
    case MAREA_TOKEN_KIND_MINUS_PERCENT:
        return "-%";
    case MAREA_TOKEN_KIND_STAR_PERCENT:
        return "*%";
    case MAREA_TOKEN_KIND_PLUS_PIPE:
        return "+|";
    case MAREA_TOKEN_KIND_MINUS_PIPE:
        return "-|";
    case MAREA_TOKEN_KIND_STAR_PIPE:
        return "*|";
    case MAREA_TOKEN_KIND_AMPERSAND:
        return "&";
    case MAREA_TOKEN_KIND_PIPE:
        return "|";
    case MAREA_TOKEN_KIND_CARET:
        return "^";
    case MAREA_TOKEN_KIND_SHIFT_LEFT:
        return "<<";
    case MAREA_TOKEN_KIND_SHIFT_RIGHT:
        return ">>";
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
        return "==";
    case MAREA_TOKEN_KIND_BANG_EQUAL:
        return "!=";
    case MAREA_TOKEN_KIND_LESS:
        return "<";
    case MAREA_TOKEN_KIND_LESS_EQUAL:
        return "<=";
    case MAREA_TOKEN_KIND_GREATER:
        return ">";
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return ">=";
    default:
        return MAREA_NULLPTR;
    }
}

// INFO: tells whether a value surely does not fit the type of an operator parameter. A shape that sema does not know fits.
static MAREA_Boolean MAREA_CALL MAREA_G_IsWrongOperand(const MAREA_G_Shape *target, const MAREA_G_Shape *actual)
{
    if ((target->kind == MAREA_G_SHAPE_KIND_PRIMITIVE) || (target->kind == MAREA_G_SHAPE_KIND_VIEW)) {
        return (MAREA_G_Fits(target, actual) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
    }

    if ((target->kind != MAREA_G_SHAPE_KIND_STRUCTURE) && (target->kind != MAREA_G_SHAPE_KIND_ENUM)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_G_IsComparable(actual) == MAREA_BOOLEAN_TRUE) {
        return MAREA_BOOLEAN_TRUE;
    }

    return (((actual->kind == MAREA_G_SHAPE_KIND_STRUCTURE) || (actual->kind == MAREA_G_SHAPE_KIND_ENUM)) && (actual->declaration != target->declaration)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: an operator of a structure or an enum is the function that an implementation marks with it, so the right side must fit its second parameter. A structure with none declared raises UNDECLARED_OPERATOR, because the built-in operand rules never judge a structure either; an enum with none is silent, since sema never compares an enum type and its variants compare by ordinal with no declared operator needed. With more than one, sema does not choose and accepts. outDeclared says that such a function, or the absence of one on a structure, already governs the operator, so no built-in operand rule applies.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckDeclaredOperator(MAREA_SemaContext *context, const MAREA_AstNode *expression, const MAREA_G_Shape *left, const MAREA_G_Shape *right, MAREA_Boolean *outDeclared)
{
    *outDeclared = MAREA_BOOLEAN_FALSE;

    const MAREA_Character8 *token = MAREA_G_OperatorText(MAREA_AstNodeGetOperator(expression));

    if (token == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaTypeTarget valueTarget = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    const MAREA_SemaExcept TARGET_RESULT = MAREA_SemaFindValueTarget(context, MAREA_SemaChildAt(expression, 0), &valueTarget);

    if (TARGET_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return TARGET_RESULT;
    }

    // INFO: a qualified type name of a FOUND namespace classifies as a library shape, never as a structure or an
    // enum shape, so that case is read from the resolved value target instead of the left shape.
    MAREA_G_Shape effective = *left;

    const MAREA_AstNode *importedRoot = MAREA_NULLPTR;

    if ((valueTarget.root != MAREA_NULLPTR) && (MAREA_G_IsKind(valueTarget.declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE)) {
        effective.kind = MAREA_G_SHAPE_KIND_STRUCTURE;
        effective.declaration = valueTarget.declaration;
        importedRoot = valueTarget.root;
    } else if ((valueTarget.root != MAREA_NULLPTR) && (MAREA_G_IsKind(valueTarget.declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_TRUE)) {
        effective.kind = MAREA_G_SHAPE_KIND_ENUM;
        effective.declaration = valueTarget.declaration;
        importedRoot = valueTarget.root;
    }

    if (((effective.kind != MAREA_G_SHAPE_KIND_STRUCTURE) && (effective.kind != MAREA_G_SHAPE_KIND_ENUM)) || (effective.declaration == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_IntegerSize count = 0;

    // INFO: a structure or an enum found through a FOUND namespace keeps its implementations in that namespace's own tree, not in context->root.
    const MAREA_AstNode *function = (importedRoot != MAREA_NULLPTR) ? MAREA_SemaFindOperatorFunctionInTree(importedRoot, effective.declaration, token, strlen(token), &count) : MAREA_SemaFindOperatorFunction(context, effective.declaration, token, strlen(token), &count);

    *outDeclared = (count > 0) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    // ERROR: a structure with no implementation that declares this operator token has nothing to apply it, and the built-in operand rules never judge a structure either. An enum is excluded: sema never compares an enum type (its variants compare by ordinal with no declared operator needed), so a zero count there is not a defect.
    if ((count == 0) && (effective.kind == MAREA_G_SHAPE_KIND_STRUCTURE)) {
        *outDeclared = MAREA_BOOLEAN_TRUE;

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_OPERATOR, MAREA_AstNodeGetPosition(expression), "no implementation declares the operator", token, strlen(token));
    }

    const MAREA_AstNode *parameters = (count == 1) ? MAREA_G_FindChild(function, MAREA_AST_KIND_PARAMETER_LIST) : MAREA_NULLPTR;

    if ((parameters == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(parameters) != 2)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *type = MAREA_SemaChildAt(MAREA_SemaChildAt(parameters, 1), 1);

    const MAREA_AstNode *segment = MAREA_SemaChildAt(MAREA_SemaChildAt(type, 0), 0);

    // INFO: Self is the type of the left side, as the function is a member of that type.
    const MAREA_Boolean IS_SELF = ((segment != MAREA_NULLPTR) && (MAREA_G_TextIs(MAREA_AstNodeGetText(segment), MAREA_AstNodeGetTextLength(segment), "Self") == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_G_Shape target = (IS_SELF == MAREA_BOOLEAN_TRUE) ? effective : MAREA_G_ShapeOfType(context, type);

    if (MAREA_G_IsWrongOperand(&target, right) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "the right operand does not fit the declared operator", token, strlen(token));
}

// INFO: an index on a structure is the function that an implementation marks with "[]". A structure of another namespace keeps that function in its own tree.
MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckIndexOperator(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *base = MAREA_SemaChildAt(node, 0);

    MAREA_SemaTypeTarget target = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    const MAREA_SemaExcept RESULT = MAREA_SemaFindValueTarget(context, base, &target);

    if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return RESULT;
    }

    const MAREA_Character8 *token = "[]";

    MAREA_IntegerSize count = 0;

    if ((target.root != MAREA_NULLPTR) && (MAREA_G_IsKind(target.declaration, MAREA_AST_KIND_STRUCTURE_DECLARATION) == MAREA_BOOLEAN_TRUE)) {
        count = (MAREA_SemaTreeDeclaresOperator(target.root, target.declaration, token, strlen(token)) == MAREA_BOOLEAN_TRUE) ? MAREA_INTEGER_SIZE(1) : MAREA_INTEGER_SIZE(0);
    } else {
        const MAREA_G_Shape shape = MAREA_G_ShapeOf(context, base, 0);

        if ((shape.kind != MAREA_G_SHAPE_KIND_STRUCTURE) || (shape.declaration == MAREA_NULLPTR)) {
            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        MAREA_IGNORE(MAREA_SemaFindOperatorFunction(context, shape.declaration, token, strlen(token), &count));
    }

    return (count > 0) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_OPERATOR, MAREA_AstNodeGetPosition(node), "no implementation declares the operator", token, strlen(token));
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

    MAREA_Boolean isTuple = MAREA_BOOLEAN_FALSE;

    const MAREA_SemaExcept TUPLE_RESULT = MAREA_SemaCheckTupleOperands(context, expression, &isTuple);

    if ((TUPLE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (isTuple == MAREA_BOOLEAN_TRUE)) {
        return TUPLE_RESULT;
    }

    const MAREA_G_Shape left = MAREA_G_ShapeOf(context, leftNode, 0);

    const MAREA_G_Shape right = MAREA_G_ShapeOf(context, rightNode, 0);

    const MAREA_TokenKind operatorKind = MAREA_AstNodeGetOperator(expression);

    if (MAREA_G_HasGenericSide(&left, &right) == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_BOUND_OPERATION_NOT_GIVEN, MAREA_AstNodeGetPosition(expression), "a generic value has no operator without a bound");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_SemaExcept result = MAREA_G_CheckPair(context, MAREA_AstNodeGetPosition(expression), operatorKind, &left, &right, "operands of a binary operator differ in type, one is");

    if ((result != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_G_OperandsAgree(&left, &right) == MAREA_BOOLEAN_FALSE)) {
        return result;
    }

    MAREA_Boolean declared = MAREA_BOOLEAN_FALSE;

    const MAREA_SemaExcept DECLARED = MAREA_G_CheckDeclaredOperator(context, expression, &left, &right, &declared);

    if ((DECLARED != MAREA_SEMA_EXCEPT_SUCCESS) || (declared == MAREA_BOOLEAN_TRUE)) {
        return DECLARED;
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

    if (MAREA_G_HasGenericSide(&target, &value) == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_BOUND_OPERATION_NOT_GIVEN, MAREA_AstNodeGetPosition(assignment), "a generic value has no operator without a bound");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

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

    const MAREA_G_Shape target = MAREA_G_ShapeOfType(context, targetType);

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

// INFO: size_of, align_of and convert are the built-in calls and Self is the type of an implementation block. c is no built-in name: it is a reserved first segment of a full path.
static MAREA_Boolean MAREA_CALL MAREA_G_IsBuiltInName(const MAREA_AstNode *name)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(name);

    const MAREA_IntegerSize length = MAREA_AstNodeGetTextLength(name);

    return ((MAREA_G_TextIs(text, length, "convert") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "size_of") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "align_of") == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextIs(text, length, "Self") == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsDeclaredName(const MAREA_SemaContext *context, const MAREA_AstNode *name)
{
    if ((MAREA_G_FindLocal(context, name) != MAREA_NULLPTR) || (MAREA_SemaNameTableFind(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) != MAREA_NULLPTR) || (MAREA_SemaIsGenericInScope(context, MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name)) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_BOOLEAN_TRUE;
    }

    return ((MAREA_G_IsBuiltInName(name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsNamespaceRoot(context, name) == MAREA_BOOLEAN_TRUE) || (MAREA_G_FindImplementationConstant(context, name) != MAREA_NULLPTR)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: a chain whose first name no declaration of the file binds is a full path into another namespace. A cover must reach it: with one the chain is accepted whole, without one it is UNIMPORTED_NAMESPACE, and a chain that no resolver knows is left to the check of the name.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFullPath(MAREA_SemaContext *context, const MAREA_AstNode *chain, MAREA_Boolean *outHandled)
{
    *outHandled = MAREA_BOOLEAN_FALSE;

    const MAREA_Character8 *text = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_AstNode *head = MAREA_NULLPTR;

    const MAREA_SemaExcept JOIN_RESULT = MAREA_SemaChainJoin(context, chain, &text, &length, &head);

    if ((JOIN_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (text == MAREA_NULLPTR) || (head == chain) || (MAREA_G_IsDeclaredName(context, head) == MAREA_BOOLEAN_TRUE)) {
        return JOIN_RESULT;
    }

    MAREA_SemaPathStatus status = MAREA_SEMA_PATH_STATUS_NOT_A_PATH;

    const MAREA_SemaExcept CLASSIFY_RESULT = MAREA_SemaClassifyPath(context, text, length, MAREA_BOOLEAN_TRUE, &status, MAREA_NULLPTR, MAREA_NULLPTR);

    if (CLASSIFY_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
        return CLASSIFY_RESULT;
    }

    if (status == MAREA_SEMA_PATH_STATUS_COVERED) {
        *outHandled = MAREA_BOOLEAN_TRUE;

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    if (status == MAREA_SEMA_PATH_STATUS_UNIMPORTED) {
        *outHandled = MAREA_BOOLEAN_TRUE;

        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNIMPORTED_NAMESPACE, MAREA_AstNodeGetPosition(head), "a namespace that no import or require covers:", text, length);
    }

    // INFO: a chain under a path that an import or a require names, with no resolver that knows it, is the fault of that declaration, which already reported it.
    if ((status == MAREA_SEMA_PATH_STATUS_UNKNOWN) && (MAREA_SemaPathIsCovered(context, text, length) == MAREA_BOOLEAN_TRUE)) {
        *outHandled = MAREA_BOOLEAN_TRUE;
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
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

// INFO: a call of a function of this file with no result has no value. The callee must be a bare name that no local hides and that the name table binds to a plain function with no result type. A method call and an imported call are skipped, sema cannot see their result.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckUsedAsValue(MAREA_SemaContext *context, const MAREA_AstNode *expression)
{
    const MAREA_AstNode *call = expression;

    for (MAREA_Uinteger32 depth = 0; (MAREA_G_IsKind(call, MAREA_AST_KIND_PARENTHESIZED_EXPRESSION) == MAREA_BOOLEAN_TRUE) && (depth < MAREA_G_DEPTH_LIMIT); ++depth) {
        call = MAREA_SemaChildAt(call, 0);
    }

    if (MAREA_G_IsKind(call, MAREA_AST_KIND_CALL_EXPRESSION) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    if ((MAREA_G_IsKind(callee, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_FALSE) || (MAREA_G_FindLocal(context, callee) != MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *function = MAREA_G_FindDeclaration(context, callee, MAREA_AST_KIND_FUNCTION_DECLARATION);

    if ((function == MAREA_NULLPTR) || (MAREA_SemaReturnType(function) != MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_RESULT_USE, MAREA_AstNodeGetPosition(call), "a call with no result is used as a value:", MAREA_AstNodeGetText(callee), MAREA_AstNodeGetTextLength(callee));
}

// INFO: the value slots of an operator and of a call: each operand, each argument.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckUsedAsValueOfChildren(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(node); ++index) {
        const MAREA_SemaExcept result = MAREA_G_CheckUsedAsValue(context, MAREA_SemaChildAt(node, index));

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckValue(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Uinteger32 depth, MAREA_Boolean checkNames);

// INFO: gives the declaration of a structure of this file that a type names, or null for any other type (an enum, a library type, a generic parameter, another namespace).
static const MAREA_AstNode *MAREA_CALL MAREA_G_StructureOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if ((type == MAREA_NULLPTR) || (MAREA_SemaClassifyType(context, type, &declaration, &name, &length) != MAREA_SEMA_TYPE_CLASS_USER) || (MAREA_AstNodeGetKind(declaration) != MAREA_AST_KIND_STRUCTURE_DECLARATION)) {
        return MAREA_NULLPTR;
    }

    return declaration;
}

// INFO: gives the FIELD_DECLARATION of a structure that has the name, or null.
static const MAREA_AstNode *MAREA_CALL MAREA_G_StructureField(const MAREA_AstNode *structure, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(structure); ++index) {
        const MAREA_AstNode *field = MAREA_SemaChildAt(structure, index);

        const MAREA_AstNode *fieldName = ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_FIELD_DECLARATION)) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

        if ((fieldName != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName), name, length) == MAREA_BOOLEAN_TRUE)) {
            return field;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: a literal of a structure of this file has its keys and values compared with the fields: a key that no field has is UNKNOWN_FIELD, a key seen before is DUPLICATE_MEMBER and a value is fitted to the declared type. A field that no entry names is MISSING_FIELD, one report per literal for the first such field in declaration order, checked after the entries.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckLiteralFields(MAREA_SemaContext *context, const MAREA_AstNode *literal)
{
    const MAREA_AstNode *structure = MAREA_G_StructureOfType(context, MAREA_SemaChildAt(literal, 0));

    if (structure == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(literal); ++index) {
        const MAREA_AstNode *entry = MAREA_SemaChildAt(literal, index);

        const MAREA_AstNode *key = ((entry != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry) == MAREA_AST_KIND_FIELD_ENTRY)) ? MAREA_SemaChildAt(entry, 0) : MAREA_NULLPTR;

        if ((key == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(key) != MAREA_AST_KIND_IDENTIFIER)) {
            continue;
        }

        const MAREA_AstNode *field = MAREA_G_StructureField(structure, MAREA_AstNodeGetText(key), MAREA_AstNodeGetTextLength(key));

        if (field == MAREA_NULLPTR) {
            const MAREA_SemaExcept result = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_FIELD, MAREA_AstNodeGetPosition(entry), "structure has no field", MAREA_AstNodeGetText(key), MAREA_AstNodeGetTextLength(key));

            if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                return result;
            }

            continue;
        }

        for (MAREA_IntegerSize before = 1; before < index; ++before) {
            const MAREA_AstNode *other = MAREA_SemaChildAt(literal, before);

            const MAREA_AstNode *otherKey = ((other != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(other) == MAREA_AST_KIND_FIELD_ENTRY)) ? MAREA_SemaChildAt(other, 0) : MAREA_NULLPTR;

            if ((otherKey != MAREA_NULLPTR) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(otherKey), MAREA_AstNodeGetTextLength(otherKey), MAREA_AstNodeGetText(key), MAREA_AstNodeGetTextLength(key)) == MAREA_BOOLEAN_TRUE)) {
                const MAREA_SemaExcept result = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_DUPLICATE_MEMBER, MAREA_AstNodeGetPosition(entry), "field is given twice", MAREA_AstNodeGetText(key), MAREA_AstNodeGetTextLength(key));

                if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
                    return result;
                }

                break;
            }
        }

        const MAREA_SemaExcept fit = MAREA_SemaCheckExpressionFit(context, MAREA_SemaChildAt(field, (MAREA_AstNodeGetChildCount(field) - 1)), MAREA_SemaChildAt(entry, 1));

        if (fit != MAREA_SEMA_EXCEPT_SUCCESS) {
            return fit;
        }
    }

    // INFO: a generic structure is skipped, because its fields are not seen through the type arguments.
    if (MAREA_G_FindChild(structure, MAREA_AST_KIND_GENERIC_PARAMETERS) != MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(structure); ++index) {
        const MAREA_AstNode *field = MAREA_SemaChildAt(structure, index);

        const MAREA_AstNode *fieldName = ((field != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(field) == MAREA_AST_KIND_FIELD_DECLARATION)) ? MAREA_G_FindChild(field, MAREA_AST_KIND_IDENTIFIER) : MAREA_NULLPTR;

        if (fieldName == MAREA_NULLPTR) {
            continue;
        }

        MAREA_Boolean named = MAREA_BOOLEAN_FALSE;

        for (MAREA_IntegerSize at = 1; at < MAREA_AstNodeGetChildCount(literal); ++at) {
            const MAREA_AstNode *entry = MAREA_SemaChildAt(literal, at);

            const MAREA_AstNode *key = ((entry != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(entry) == MAREA_AST_KIND_FIELD_ENTRY)) ? MAREA_SemaChildAt(entry, 0) : MAREA_NULLPTR;

            if ((key != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(key) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(key), MAREA_AstNodeGetTextLength(key), MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName)) == MAREA_BOOLEAN_TRUE)) {
                named = MAREA_BOOLEAN_TRUE;

                break;
            }
        }

        if (named == MAREA_BOOLEAN_FALSE) {
            return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_MISSING_FIELD, MAREA_AstNodeGetPosition(literal), "struct literal misses a field", MAREA_AstNodeGetText(fieldName), MAREA_AstNodeGetTextLength(fieldName));
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a field read or write on a structure of this file must name a field. A method callee is a MEMBER_ACCESS too, so the call case does not give its callee to this check.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckFieldName(MAREA_SemaContext *context, const MAREA_AstNode *access)
{
    const MAREA_AstNode *structure = MAREA_G_StructureOfType(context, MAREA_SemaInferType(context, MAREA_SemaChildAt(access, 0)));

    const MAREA_AstNode *member = MAREA_SemaChildAt(access, 1);

    if ((structure == MAREA_NULLPTR) || (member == MAREA_NULLPTR) || (MAREA_G_StructureField(structure, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) != MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_FIELD, MAREA_AstNodeGetPosition(access), "structure has no field", MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));
}

// INFO: true when an implementation block of the tree of the enum declares a function or a constant with the name.
static MAREA_Boolean MAREA_CALL MAREA_G_EnumHasImplementedMember(const MAREA_AstNode *root, const MAREA_AstNode *enumeration, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    const MAREA_AstNode *enumerationName = MAREA_G_FindChild(enumeration, MAREA_AST_KIND_IDENTIFIER);

    for (MAREA_IntegerSize block = 0; ((enumerationName != MAREA_NULLPTR) && (block < MAREA_AstNodeGetChildCount(root))); ++block) {
        const MAREA_AstNode *implementation = MAREA_SemaChildAt(root, block);

        const MAREA_AstNode *implemented = (MAREA_G_IsKind(implementation, MAREA_AST_KIND_IMPLEMENTATION_DECLARATION) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FindChild(implementation, MAREA_AST_KIND_NAMED_TYPE) : MAREA_NULLPTR;

        const MAREA_AstNode *implementedName = (implemented != MAREA_NULLPTR) ? MAREA_SemaChildAt(MAREA_SemaChildAt(implemented, 0), 0) : MAREA_NULLPTR;

        if ((implementedName == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(implementedName), MAREA_AstNodeGetTextLength(implementedName), MAREA_AstNodeGetText(enumerationName), MAREA_AstNodeGetTextLength(enumerationName)) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(implementation); ++index) {
            const MAREA_AstNode *member = MAREA_SemaChildAt(implementation, index);

            const MAREA_Character8 *text = MAREA_NULLPTR;

            MAREA_IntegerSize textLength = 0;

            if (((MAREA_G_IsKind(member, MAREA_AST_KIND_FUNCTION_DECLARATION) == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsKind(member, MAREA_AST_KIND_CONSTANT_DECLARATION) == MAREA_BOOLEAN_TRUE)) && (MAREA_SemaDeclarationName(member, &text, &textLength) == MAREA_BOOLEAN_TRUE) && (MAREA_SemaTextEquals(text, textLength, name, length) == MAREA_BOOLEAN_TRUE)) {
                return MAREA_BOOLEAN_TRUE;
            }
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: Enum.Name must name a variant, or a function or a constant that an implementation block gives the enum. The enum may be of this file, of a FOUND namespace or imported, and its blocks are in its own tree. A local that hides the enum name is not checked. A method callee is a MEMBER_ACCESS too, so the call case does not give its callee to this check.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckEnumVariant(MAREA_SemaContext *context, const MAREA_AstNode *access)
{
    const MAREA_AstNode *base = MAREA_SemaChildAt(access, 0);

    const MAREA_AstNode *member = MAREA_SemaChildAt(access, 1);

    if (member == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaTypeTarget target = { .type = MAREA_NULLPTR, .declaration = MAREA_NULLPTR, .root = MAREA_NULLPTR };

    const MAREA_SemaExcept TARGET_RESULT = MAREA_SemaFindNamedTarget(context, base, &target);

    if ((TARGET_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (MAREA_G_IsKind(target.declaration, MAREA_AST_KIND_ENUM_DECLARATION) == MAREA_BOOLEAN_FALSE)) {
        return TARGET_RESULT;
    }

    const MAREA_AstNode *root = (target.root != MAREA_NULLPTR) ? target.root : context->root;

    if ((MAREA_SemaEnumHasVariant(target.declaration, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE) || (MAREA_G_EnumHasImplementedMember(root, target.declaration, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ENUM_VARIANT, MAREA_AstNodeGetPosition(access), "enum has no variant:", MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member));
}

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

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_SemaExcept USED = MAREA_G_CheckUsedAsValueOfChildren(context, expression);

        return (USED != MAREA_SEMA_EXCEPT_SUCCESS) ? USED : MAREA_G_CheckOperands(context, expression);
    }
    case MAREA_AST_KIND_CALL_EXPRESSION:
    {
        // INFO: a MEMBER_ACCESS callee is a method, so only its base is checked and its member is not a field.
        const MAREA_AstNode *callee = MAREA_SemaChildAt(expression, 0);

        const MAREA_Boolean METHOD_CALLEE = ((callee != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(callee) == MAREA_AST_KIND_MEMBER_ACCESS) && (MAREA_G_IsOpaqueName(context, MAREA_SemaChildAt(callee, 0)) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

        MAREA_Boolean fullPath = MAREA_BOOLEAN_FALSE;

        MAREA_SemaExcept result = MAREA_SEMA_EXCEPT_SUCCESS;

        if ((checkNames == MAREA_BOOLEAN_TRUE) && (callee != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(callee) == MAREA_AST_KIND_MEMBER_ACCESS)) {
            result = MAREA_G_CheckFullPath(context, callee, &fullPath);
        }

        if ((result == MAREA_SEMA_EXCEPT_SUCCESS) && (fullPath == MAREA_BOOLEAN_FALSE)) {
            result = (METHOD_CALLEE == MAREA_BOOLEAN_TRUE) ? MAREA_G_CheckValue(context, MAREA_SemaChildAt(callee, 0), (depth + 1), checkNames) : MAREA_G_CheckValue(context, callee, (depth + 1), checkNames);
        }

        if ((result == MAREA_SEMA_EXCEPT_SUCCESS) && (MAREA_AstNodeGetChildCount(expression) > 1)) {
            result = MAREA_G_CheckValueOfChildren(context, expression, 1, depth, checkNames);
        }

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }

        const MAREA_AstNode *arguments = MAREA_G_FindChild(expression, MAREA_AST_KIND_ARGUMENT_LIST);

        const MAREA_SemaExcept USED = (arguments == MAREA_NULLPTR) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_CheckUsedAsValueOfChildren(context, arguments);

        return (USED != MAREA_SEMA_EXCEPT_SUCCESS) ? USED : MAREA_G_CheckConvert(context, expression);
    }
    case MAREA_AST_KIND_UNARY_EXPRESSION:
    {
        const MAREA_SemaExcept result = MAREA_G_CheckValueOfChildren(context, expression, 0, depth, checkNames);

        if (result != MAREA_SEMA_EXCEPT_SUCCESS) {
            return result;
        }

        // INFO: spawn starts the call as a worker, so the call of a function with no result is its operand and not a value.
        const MAREA_SemaExcept USED = (MAREA_AstNodeGetOperator(expression) == MAREA_TOKEN_KIND_KEYWORD_SPAWN) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_CheckUsedAsValue(context, MAREA_SemaChildAt(expression, 0));

        if (USED != MAREA_SEMA_EXCEPT_SUCCESS) {
            return USED;
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

        if (checkNames == MAREA_BOOLEAN_TRUE) {
            MAREA_Boolean fullPath = MAREA_BOOLEAN_FALSE;

            const MAREA_SemaExcept PATH_RESULT = MAREA_G_CheckFullPath(context, expression, &fullPath);

            if ((PATH_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) || (fullPath == MAREA_BOOLEAN_TRUE)) {
                // INFO: a full path names the enum with every segment, so its variant is read here.
                return (PATH_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) ? PATH_RESULT : MAREA_G_CheckEnumVariant(context, expression);
            }
        }

        if (MAREA_G_IsOpaqueName(context, base) == MAREA_BOOLEAN_TRUE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE, MAREA_AstNodeGetPosition(expression), "an opaque type has no fields");

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_SemaExcept baseResult = MAREA_G_CheckValue(context, base, (depth + 1), checkNames);

        if (baseResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return baseResult;
        }

        const MAREA_SemaExcept FIELD = MAREA_G_CheckFieldName(context, expression);

        return (FIELD != MAREA_SEMA_EXCEPT_SUCCESS) ? FIELD : MAREA_G_CheckEnumVariant(context, expression);
    }
    case MAREA_AST_KIND_LITERAL_VALUE:
        if (MAREA_G_IsOpaqueType(context, MAREA_SemaChildAt(expression, 0)) == MAREA_BOOLEAN_TRUE) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_OPAQUE_TYPE_MISUSE, MAREA_AstNodeGetPosition(expression), "an opaque type cannot be built with a literal");

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        {
            const MAREA_SemaExcept result = MAREA_G_CheckValueOfChildren(context, expression, 1, depth, checkNames);

            return (result != MAREA_SEMA_EXCEPT_SUCCESS) ? result : MAREA_G_CheckLiteralFields(context, expression);
        }
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

// INFO: Enum.Name of a variant of an enum of this file is a value of that enum.
static MAREA_G_Shape MAREA_CALL MAREA_G_ShapeOfCase(const MAREA_SemaContext *context, const MAREA_AstNode *value)
{
    const MAREA_AstNode *base = (MAREA_G_IsKind(value, MAREA_AST_KIND_MEMBER_ACCESS) == MAREA_BOOLEAN_TRUE) ? MAREA_SemaChildAt(value, 0) : MAREA_NULLPTR;

    const MAREA_AstNode *member = (base == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_SemaChildAt(value, 1);

    if ((member != MAREA_NULLPTR) && (MAREA_G_IsKind(base, MAREA_AST_KIND_IDENTIFIER) == MAREA_BOOLEAN_TRUE) && (MAREA_G_FindLocal(context, base) == MAREA_NULLPTR)) {
        const MAREA_AstNode *enumeration = MAREA_G_FindDeclaration(context, base, MAREA_AST_KIND_ENUM_DECLARATION);

        if ((enumeration != MAREA_NULLPTR) && (MAREA_SemaEnumHasVariant(enumeration, MAREA_AstNodeGetText(member), MAREA_AstNodeGetTextLength(member)) == MAREA_BOOLEAN_TRUE)) {
            MAREA_G_Shape shape = MAREA_G_ShapeMake(MAREA_G_SHAPE_KIND_ENUM, MAREA_AstNodeGetText(base), MAREA_AstNodeGetTextLength(base));

            shape.declaration = enumeration;

            return shape;
        }
    }

    return MAREA_G_ShapeOf(context, value, 0);
}

// INFO: each value of a case has the type of the switch value. Only a switch over a primitive or an enum is judged.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckCases(MAREA_SemaContext *context, const MAREA_AstNode *statement)
{
    const MAREA_G_Shape target = MAREA_G_ShapeOf(context, MAREA_SemaChildAt(statement, 0), 0);

    if ((target.kind != MAREA_G_SHAPE_KIND_PRIMITIVE) && (target.kind != MAREA_G_SHAPE_KIND_ENUM)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 1; index < MAREA_AstNodeGetChildCount(statement); ++index) {
        const MAREA_AstNode *arm = MAREA_SemaChildAt(statement, index);

        if (MAREA_G_IsKind(arm, MAREA_AST_KIND_SWITCH_ARM) == MAREA_BOOLEAN_FALSE) {
            continue;
        }

        // INFO: the last child of an arm is its block.
        for (MAREA_IntegerSize valueIndex = 0; (valueIndex + 1) < MAREA_AstNodeGetChildCount(arm); ++valueIndex) {
            const MAREA_AstNode *value = MAREA_SemaChildAt(arm, valueIndex);

            const MAREA_G_Shape actual = MAREA_G_ShapeOfCase(context, value);

            const MAREA_Boolean ENUM_FOR_PRIMITIVE = ((target.kind == MAREA_G_SHAPE_KIND_PRIMITIVE) && (actual.kind == MAREA_G_SHAPE_KIND_ENUM)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

            if ((ENUM_FOR_PRIMITIVE == MAREA_BOOLEAN_FALSE) && (MAREA_G_IsWrongOperand(&target, &actual) == MAREA_BOOLEAN_FALSE)) {
                continue;
            }

            const MAREA_SemaExcept RESULT = MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(value), "a case value does not fit the type of the switch value", target.name, target.length);

            if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
                return RESULT;
            }
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: a value that is known and is no number.
static MAREA_Boolean MAREA_CALL MAREA_G_IsNoNumber(const MAREA_G_Shape *shape)
{
    if ((shape->kind == MAREA_G_SHAPE_KIND_UNKNOWN) || (shape->kind == MAREA_G_SHAPE_KIND_GENERIC) || (shape->kind == MAREA_G_SHAPE_KIND_LIBRARY)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return (MAREA_G_IsNumber(shape) == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE;
}

// INFO: the two bounds of a for range are numbers of one type, which is the type of the loop name.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckRange(MAREA_SemaContext *context, const MAREA_AstNode *first, const MAREA_AstNode *last)
{
    const MAREA_G_Shape lower = MAREA_G_ShapeOf(context, first, 0);

    const MAREA_G_Shape upper = MAREA_G_ShapeOf(context, last, 0);

    const MAREA_AstNode *wrong = (MAREA_G_IsNoNumber(&lower) == MAREA_BOOLEAN_TRUE) ? first : ((MAREA_G_IsNoNumber(&upper) == MAREA_BOOLEAN_TRUE) ? last : MAREA_NULLPTR);

    if ((wrong == MAREA_NULLPTR) && (MAREA_G_OperandsAgree(&lower, &upper) == MAREA_BOOLEAN_FALSE)) {
        wrong = last;
    }

    if (wrong == MAREA_NULLPTR) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(wrong), "the bounds of a for range need one number type");

    return MAREA_SEMA_EXCEPT_SUCCESS;
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
        // INFO: the raw pointer role check runs once per written NAMED_TYPE (MAREA_SemaCheckNamedType,
        // MAREASemaType.c), which this local's own type node reaches through the recursive visit below;
        // no separate call is needed here.
        const MAREA_SemaExcept initializerResult = MAREA_SemaCheckLocalInitializer(context, node);

        if (initializerResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return initializerResult;
        }

        const MAREA_SemaExcept valueResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 2), 0, MAREA_BOOLEAN_TRUE);

        return (valueResult != MAREA_SEMA_EXCEPT_SUCCESS) ? valueResult : MAREA_G_CheckUsedAsValue(context, MAREA_SemaChildAt(node, 2));
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

        if (firstResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return firstResult;
        }

        const MAREA_SemaExcept lastResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 2), 0, MAREA_BOOLEAN_TRUE);

        return (lastResult != MAREA_SEMA_EXCEPT_SUCCESS) ? lastResult : MAREA_G_CheckRange(context, MAREA_SemaChildAt(node, 1), MAREA_SemaChildAt(node, 2));
    }
    case MAREA_AST_KIND_FOR_EACH_STATEMENT:
    {
        // INFO: the children are the variables, the collection and the body, so the collection is the child before the last.
        const MAREA_IntegerSize count = MAREA_AstNodeGetChildCount(node);

        const MAREA_SemaExcept VALUE_RESULT = (count < 3) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, (count - 2)), 0, MAREA_BOOLEAN_TRUE);

        return (VALUE_RESULT != MAREA_SEMA_EXCEPT_SUCCESS) ? VALUE_RESULT : MAREA_SemaCheckTupleForEach(context, node);
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
    case MAREA_AST_KIND_DO_WHILE_STATEMENT:
    {
        // INFO: the condition is the second child, after the body. This check runs before the body is visited, so a name that the body declares is not yet a local and the condition cannot read it.
        const MAREA_SemaExcept namesResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 1), 0, MAREA_BOOLEAN_TRUE);

        return (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) ? namesResult : MAREA_SemaCheckCondition(context, MAREA_SemaChildAt(node, 1));
    }
    case MAREA_AST_KIND_EXPRESSION_STATEMENT:
        return MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);
    case MAREA_AST_KIND_SWITCH_STATEMENT:
    {
        const MAREA_SemaExcept namesResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);

        return (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) ? namesResult : MAREA_G_CheckCases(context, node);
    }
    case MAREA_AST_KIND_RETURN_STATEMENT:
    {
        const MAREA_SemaExcept namesResult = MAREA_G_CheckValue(context, MAREA_SemaChildAt(node, 0), 0, MAREA_BOOLEAN_TRUE);

        if (namesResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return namesResult;
        }

        // INFO: a function with no result has no value to return, and a returned call is a value slot.
        if ((context->function != MAREA_NULLPTR) && (MAREA_SemaReturnType(context->function) == MAREA_NULLPTR)) {
            const MAREA_AstNode *name = MAREA_SemaChildAt(context->function, 0);

            return (MAREA_SemaChildAt(node, 0) == MAREA_NULLPTR) ? MAREA_SEMA_EXCEPT_SUCCESS : MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_RESULT_USE, MAREA_AstNodeGetPosition(node), "a function with no result cannot return a value:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
        }

        const MAREA_SemaExcept usedResult = MAREA_G_CheckUsedAsValue(context, MAREA_SemaChildAt(node, 0));

        if (usedResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return usedResult;
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

        const MAREA_SemaExcept usedResult = MAREA_G_CheckUsedAsValue(context, MAREA_SemaChildAt(node, 1));

        if (usedResult != MAREA_SEMA_EXCEPT_SUCCESS) {
            return usedResult;
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
