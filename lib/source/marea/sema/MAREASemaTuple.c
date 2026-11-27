// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

// INFO: how deep a tuple of tuples, or an alias, is followed.
#define MAREA_G_TUPLE_DEPTH_LIMIT MAREA_UINTEGER_32(16)

// INFO: a tuple is an anonymous structure: the stages read the shape that the descriptor gives, never the name.
MAREA_Boolean MAREA_CALL MAREA_SemaTupleOfType(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode **outArguments)
{
    if (outArguments != MAREA_NULLPTR) {
        *outArguments = MAREA_NULLPTR;
    }

    const MAREA_TypeDescriptor *descriptor = MAREA_NULLPTR;

    if ((MAREA_SemaDescriptorOfType(context, type, &descriptor) == MAREA_BOOLEAN_FALSE) || !MAREA_TYPE_DESCRIPTOR_IS_TUPLE(descriptor)) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (MAREA_IntegerSize index = 0; (outArguments != MAREA_NULLPTR) && (index < MAREA_AstNodeGetChildCount(type)); ++index) {
        const MAREA_AstNode *child = MAREA_SemaChildAt(type, index);

        if ((child != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(child) == MAREA_AST_KIND_TYPE_ARGUMENTS)) {
            *outArguments = child;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}

const MAREA_AstNode *MAREA_CALL MAREA_SemaTupleElement(const MAREA_SemaContext *context, const MAREA_AstNode *type, const MAREA_AstNode *key)
{
    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    MAREA_Uinteger64 position = 0;

    if ((MAREA_SemaTupleOfType(context, type, &arguments) == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR) || (MAREA_SemaReadIntegerLiteral(key, &position) == MAREA_BOOLEAN_FALSE) || (position >= MAREA_AstNodeGetChildCount(arguments))) {
        return MAREA_NULLPTR;
    }

    return MAREA_SemaChildAt(arguments, (MAREA_IntegerSize)position);
}

// INFO: the tuple that a value has, with the bindings of its frame. The outWritten node is null when sema does not know it.
static MAREA_Boolean MAREA_CALL MAREA_G_TupleOfValue(const MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_SemaWrittenType *outWritten, const MAREA_AstNode **outArguments)
{
    if (outArguments != MAREA_NULLPTR) {
        *outArguments = MAREA_NULLPTR;
    }

    if (MAREA_SemaWrittenTypeOf(context, expression, outWritten) == MAREA_BOOLEAN_FALSE) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_SemaResolveWrittenType(context, outWritten);

    return MAREA_SemaTupleOfType(context, outWritten->type, outArguments);
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckTupleType(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    if (MAREA_SemaTupleOfType(context, node, &arguments) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize COUNT = (arguments == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(arguments);

    MAREA_Boolean badArguments = ((COUNT < MAREA_TYPE_TUPLE_MINIMUM_ARITY) || (COUNT > MAREA_TYPE_TUPLE_MAXIMUM_ARITY)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((argument != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(argument) == MAREA_AST_KIND_INTEGER_LITERAL)) {
            badArguments = MAREA_BOOLEAN_TRUE;
        }
    }

    if (badArguments == MAREA_BOOLEAN_TRUE) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, MAREA_AstNodeGetPosition(node), "a tuple takes from 2 to 16 types");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < COUNT; ++index) {
        const MAREA_AstNode *argument = MAREA_SemaChildAt(arguments, index);

        if ((argument != MAREA_NULLPTR) && (MAREA_SemaHasKnownLayout(context, argument, MAREA_TYPE_ROLE_NONE) == MAREA_BOOLEAN_FALSE)) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_ELEMENT_WITHOUT_MEASURED_LAYOUT, MAREA_AstNodeGetPosition(argument), "an element type with no measured layout cannot be held by value");
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckTupleIndex(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    if ((MAREA_G_TupleOfValue(context, MAREA_SemaChildAt(node, 0), &written, &arguments) == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_AstNode *key = MAREA_SemaChildAt(node, 1);

    MAREA_Uinteger64 position = 0;

    const MAREA_Boolean IS_LITERAL = MAREA_SemaReadIntegerLiteral(key, &position);

    // INFO: a negated literal is a constant that is below zero.
    const MAREA_Boolean IS_NEGATIVE = ((key != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(key) == MAREA_AST_KIND_UNARY_EXPRESSION) && (MAREA_AstNodeGetOperator(key) == MAREA_TOKEN_KIND_MINUS) && (MAREA_SemaReadIntegerLiteral(MAREA_SemaChildAt(key, 0), &position) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IS_LITERAL == MAREA_BOOLEAN_FALSE) && (IS_NEGATIVE == MAREA_BOOLEAN_FALSE)) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_TUPLE_INDEX_NOT_CONSTANT, MAREA_AstNodeGetPosition(node), "the index of a tuple is an integer literal");
    } else if ((IS_NEGATIVE == MAREA_BOOLEAN_TRUE) || (position >= MAREA_AstNodeGetChildCount(arguments))) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_CONSTANT_INDEX_OUT_OF_RANGE, MAREA_AstNodeGetPosition(node), "a constant index is not below the arity of the tuple");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: one value of a literal against its element: the type, the range of a number and copy or move for an owner.
static MAREA_SemaExcept MAREA_CALL MAREA_G_CheckElementValue(MAREA_SemaContext *context, const MAREA_SemaWrittenType *written, const MAREA_AstNode *element, const MAREA_AstNode *value)
{
    MAREA_SemaWrittenType target = *written;

    target.type = element;

    MAREA_SemaExcept result = MAREA_SemaCheckWrittenFit(context, &target, value);

    if (result == MAREA_SEMA_EXCEPT_SUCCESS) {
        result = MAREA_SemaCheckLiteralRange(context, element, value);
    }

    const MAREA_AstKind KIND = (value == MAREA_NULLPTR) ? MAREA_AST_KIND_ERROR_NODE : MAREA_AstNodeGetKind(value);

    const MAREA_Boolean EXISTING = ((KIND == MAREA_AST_KIND_IDENTIFIER) || (KIND == MAREA_AST_KIND_MEMBER_ACCESS) || (KIND == MAREA_AST_KIND_INDEX_ACCESS)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((result == MAREA_SEMA_EXCEPT_SUCCESS) && (EXISTING == MAREA_BOOLEAN_TRUE) && (written->bindings.count == 0) && (MAREA_SemaTypeIsByValue(context, element, MAREA_BOOLEAN_TRUE) == MAREA_BOOLEAN_FALSE)) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE, MAREA_AstNodeGetPosition(value), "an existing value that is stored needs copy or move");
    }

    return result;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckTupleLiteral(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_SemaWrittenType written = { .type = MAREA_SemaChildAt(node, 0), .bindings = { .count = 0 } };

    MAREA_SemaResolveWrittenType(context, &written);

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    if ((MAREA_SemaTupleOfType(context, written.type, &arguments) == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_IntegerSize ARITY = MAREA_AstNodeGetChildCount(arguments);

    const MAREA_IntegerSize GIVEN = MAREA_AstNodeGetChildCount(node) - 1;

    if ((GIVEN != 0) && (GIVEN != ARITY)) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_LITERAL_COUNT_MISMATCH, MAREA_AstNodeGetPosition(node), "the values of a tuple literal differ from the arity of the tuple");

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    for (MAREA_IntegerSize index = 0; index < GIVEN; ++index) {
        const MAREA_AstNode *entry = MAREA_SemaChildAt(node, (index + 1));

        if ((entry == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry) != MAREA_AST_KIND_VALUE_ENTRY)) {
            MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(node), "a tuple literal holds its values in order, with no name and no index");

            return MAREA_SEMA_EXCEPT_SUCCESS;
        }

        const MAREA_SemaExcept RESULT = MAREA_G_CheckElementValue(context, &written, MAREA_SemaChildAt(arguments, index), MAREA_SemaChildAt(entry, 0));

        if (RESULT != MAREA_SEMA_EXCEPT_SUCCESS) {
            return RESULT;
        }
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

// INFO: two tuple types are one type when they have one arity and no element differs.
static MAREA_Boolean MAREA_CALL MAREA_G_SameTuple(const MAREA_SemaContext *context, const MAREA_SemaWrittenType *written, const MAREA_AstNode *arguments, const MAREA_AstNode *other)
{
    MAREA_SemaWrittenType otherWritten = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    const MAREA_AstNode *otherArguments = MAREA_NULLPTR;

    if ((MAREA_G_TupleOfValue(context, other, &otherWritten, &otherArguments) == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR) || (otherArguments == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return ((MAREA_AstNodeGetChildCount(arguments) == MAREA_AstNodeGetChildCount(otherArguments)) && (MAREA_SemaTypeArgumentsDiffer(context, written, other) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckTupleCall(MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Boolean *outHandled)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR) || (outHandled == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    *outHandled = MAREA_BOOLEAN_FALSE;

    const MAREA_AstNode *callee = MAREA_SemaChildAt(node, 0);

    const MAREA_AstNode *receiver = MAREA_SemaChildAt(callee, 0);

    const MAREA_AstNode *name = MAREA_SemaChildAt(callee, 1);

    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    if ((receiver == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || ((MAREA_SemaTupleOfType(context, receiver, MAREA_NULLPTR) == MAREA_BOOLEAN_FALSE) && (MAREA_G_TupleOfValue(context, receiver, &written, &arguments) == MAREA_BOOLEAN_FALSE))) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    *outHandled = MAREA_BOOLEAN_TRUE;

    // INFO: a tuple type has no static function, and a value has swap alone.
    if ((written.type == MAREA_NULLPTR) || (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), "swap", strlen("swap")) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_TYPE_FUNCTION, MAREA_AstNodeGetPosition(node), "type has no function:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    const MAREA_AstNode *given = MAREA_SemaChildAt(node, 1);

    if (((given == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(given)) != 1) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_ARGUMENT_COUNT_MISMATCH, MAREA_AstNodeGetPosition(node), "wrong number of arguments in the call of", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    const MAREA_SemaExcept WRITABLE = MAREA_SemaCheckWritablePlace(context, receiver, name);

    if (WRITABLE != MAREA_SEMA_EXCEPT_SUCCESS) {
        return WRITABLE;
    }

    const MAREA_AstNode *other = MAREA_SemaChildAt(given, 0);

    if (MAREA_G_SameTuple(context, &written, arguments, other) == MAREA_BOOLEAN_FALSE) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH, MAREA_AstNodeGetPosition(other), "the two tuples of a swap differ in type:", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    if (MAREA_SemaPlacesAlias(receiver, other) == MAREA_BOOLEAN_TRUE) {
        return MAREA_SemaReportNamed(context, MAREA_SEMA_DIAGNOSTIC_CODE_EXCLUSIVE_BORROW_CONFLICT, MAREA_AstNodeGetPosition(node), "two write borrows of one value are passed to", MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name));
    }

    return MAREA_SemaCheckWritablePlace(context, other, name);
}

// INFO: == and != are given by a primitive, an enum and a tuple of these. A type that sema cannot see is accepted.
static MAREA_Boolean MAREA_CALL MAREA_G_IsEquatable(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_Uinteger32 depth)
{
    const MAREA_AstNode *declaration = MAREA_NULLPTR;

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    switch (MAREA_SemaClassifyType(context, type, &declaration, &name, &length)) {
    case MAREA_SEMA_TYPE_CLASS_PRIMITIVE:
    case MAREA_SEMA_TYPE_CLASS_EXTERNAL:
        return MAREA_BOOLEAN_TRUE;
    case MAREA_SEMA_TYPE_CLASS_USER:
        return (MAREA_AstNodeGetKind(declaration) == MAREA_AST_KIND_ENUM_DECLARATION) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
    case MAREA_SEMA_TYPE_CLASS_TUPLE:
        if ((depth >= MAREA_G_TUPLE_DEPTH_LIMIT) || (MAREA_SemaTupleOfType(context, type, &arguments) == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR)) {
            return MAREA_BOOLEAN_TRUE;
        }

        for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(arguments); ++index) {
            if (MAREA_G_IsEquatable(context, MAREA_SemaChildAt(arguments, index), (depth + 1)) == MAREA_BOOLEAN_FALSE) {
                return MAREA_BOOLEAN_FALSE;
            }
        }

        return MAREA_BOOLEAN_TRUE;
    default:
        return MAREA_BOOLEAN_FALSE;
    }
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckTupleOperands(MAREA_SemaContext *context, const MAREA_AstNode *expression, MAREA_Boolean *outHandled)
{
    if ((context == MAREA_NULLPTR) || (expression == MAREA_NULLPTR) || (outHandled == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *leftNode = MAREA_SemaChildAt(expression, 0);

    const MAREA_AstNode *rightNode = MAREA_SemaChildAt(expression, 1);

    MAREA_SemaWrittenType left = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    MAREA_SemaWrittenType right = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    const MAREA_AstNode *leftArguments = MAREA_NULLPTR;

    const MAREA_AstNode *rightArguments = MAREA_NULLPTR;

    const MAREA_Boolean LEFT = MAREA_G_TupleOfValue(context, leftNode, &left, &leftArguments);

    const MAREA_Boolean RIGHT = MAREA_G_TupleOfValue(context, rightNode, &right, &rightArguments);

    *outHandled = ((LEFT == MAREA_BOOLEAN_TRUE) || (RIGHT == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (*outHandled == MAREA_BOOLEAN_FALSE) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(expression);

    const MAREA_AstNode *tuple = (LEFT == MAREA_BOOLEAN_TRUE) ? left.type : right.type;

    MAREA_Boolean accepted = ((OPERATOR == MAREA_TOKEN_KIND_EQUAL_EQUAL) || (OPERATOR == MAREA_TOKEN_KIND_BANG_EQUAL)) ? MAREA_G_IsEquatable(context, tuple, 0) : MAREA_BOOLEAN_FALSE;

    if ((accepted == MAREA_BOOLEAN_TRUE) && (LEFT == MAREA_BOOLEAN_TRUE) && (RIGHT == MAREA_BOOLEAN_TRUE)) {
        accepted = MAREA_G_SameTuple(context, &left, leftArguments, rightNode);
    }

    if (accepted == MAREA_BOOLEAN_FALSE) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH, MAREA_AstNodeGetPosition(expression), "a tuple has only == and != with a tuple of its type, when each element has them");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckTupleForEach(MAREA_SemaContext *context, const MAREA_AstNode *node)
{
    if ((context == MAREA_NULLPTR) || (node == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(node);

    MAREA_SemaWrittenType written = { .type = MAREA_NULLPTR, .bindings = { .count = 0 } };

    if ((COUNT >= 3) && (MAREA_G_TupleOfValue(context, MAREA_SemaChildAt(node, (COUNT - 2)), &written, MAREA_NULLPTR) == MAREA_BOOLEAN_TRUE)) {
        MAREA_SemaReport(context, MAREA_SEMA_DIAGNOSTIC_CODE_NOT_ITERABLE, MAREA_AstNodeGetPosition(node), "a tuple is not iterable, because its elements differ in type");
    }

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_Boolean MAREA_CALL MAREA_SemaTupleIsCopyable(const MAREA_SemaContext *context, const MAREA_AstNode *type)
{
    const MAREA_AstNode *arguments = MAREA_NULLPTR;

    if ((MAREA_SemaTupleOfType(context, type, &arguments) == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_AstNodeGetChildCount(arguments); ++index) {
        if (MAREA_SemaIsCopyable(context, MAREA_SemaChildAt(arguments, index)) == MAREA_BOOLEAN_FALSE) {
            return MAREA_BOOLEAN_FALSE;
        }
    }

    return MAREA_BOOLEAN_TRUE;
}
