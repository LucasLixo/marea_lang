// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <stdlib.h>
#include <string.h>

#define MAREA_G_EVALUATION_DEPTH_LIMIT MAREA_UINTEGER_32(32)

#define MAREA_G_FLOAT_TEXT_LIMIT MAREA_INTEGER_SIZE(128)

// INFO: the answers of the evaluator: unknown, or a value of one of four kinds.
typedef enum MAREA_G_ValueKind
{
    MAREA_G_VALUE_KIND_UNKNOWN = 0,
    MAREA_G_VALUE_KIND_BOOLEAN = 1,
    MAREA_G_VALUE_KIND_INTEGER = 2,
    MAREA_G_VALUE_KIND_FLOAT = 3,
    MAREA_G_VALUE_KIND_STRING = 4,
} MAREA_G_ValueKind;

// INFO: bits is the width of the type of a number. Zero is a number with no type yet, which a literal gives.
typedef struct MAREA_G_Value
{
    MAREA_G_ValueKind kind;
    MAREA_Boolean boolean;
    MAREA_Sinteger64 integer;
    MAREA_Float64 floating;
    const MAREA_Character8 *text;
    MAREA_IntegerSize length;
    MAREA_Uinteger32 bits;
    MAREA_Boolean isSigned;
} MAREA_G_Value;

static MAREA_G_Value MAREA_CALL MAREA_G_Unknown(void)
{
    return (MAREA_G_Value){ .kind = MAREA_G_VALUE_KIND_UNKNOWN };
}

static MAREA_G_Value MAREA_CALL MAREA_G_FromBoolean(MAREA_Boolean value)
{
    return (MAREA_G_Value){ .kind = MAREA_G_VALUE_KIND_BOOLEAN, .boolean = value };
}

static MAREA_G_Value MAREA_CALL MAREA_G_FromInteger(MAREA_Sinteger64 value)
{
    return (MAREA_G_Value){ .kind = MAREA_G_VALUE_KIND_INTEGER, .integer = value, .isSigned = MAREA_BOOLEAN_TRUE };
}

// INFO: a value of bits 32 is a float32, so it is rounded to that width.
static MAREA_G_Value MAREA_CALL MAREA_G_FromFloat(MAREA_Float64 value, MAREA_Uinteger32 bits)
{
    return (MAREA_G_Value){ .kind = MAREA_G_VALUE_KIND_FLOAT, .floating = (bits == 32) ? (MAREA_Float64)(MAREA_Float32)value : value, .bits = bits };
}

// INFO: gives an integer the width of a type. A narrow width wraps. A 64-bit unsigned value is held as a signed one, so a negative value is not decided.
static MAREA_G_Value MAREA_CALL MAREA_G_Typed(MAREA_Sinteger64 value, MAREA_Uinteger32 bits, MAREA_Boolean isSigned)
{
    MAREA_Uinteger64 raw = (MAREA_Uinteger64)value;

    if ((bits > 0) && (bits < 64)) {
        const MAREA_Uinteger64 MASK = (((MAREA_Uinteger64)1 << bits) - 1);

        raw &= MASK;

        if ((isSigned == MAREA_BOOLEAN_TRUE) && ((raw >> (bits - 1)) != 0)) {
            raw |= ~MASK;
        }
    } else if ((isSigned == MAREA_BOOLEAN_FALSE) && (value < 0)) {
        return MAREA_G_Unknown();
    }

    return (MAREA_G_Value){ .kind = MAREA_G_VALUE_KIND_INTEGER, .integer = (MAREA_Sinteger64)raw, .bits = bits, .isSigned = isSigned };
}

// INFO: reads an integer literal (decimal, 0x or 0b, with "_" between digits). It gives false when the value does not fit a signed 64-bit integer.
static MAREA_Boolean MAREA_CALL MAREA_G_ReadInteger(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Sinteger64 *outValue)
{
    MAREA_Uinteger64 base = 10;

    MAREA_IntegerSize index = 0;

    if ((length > 2) && (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
        base = 16;
        index = 2;
    } else if ((length > 2) && (text[0] == '0') && ((text[1] == 'b') || (text[1] == 'B'))) {
        base = 2;
        index = 2;
    }

    MAREA_Uinteger64 value = 0;

    for (; index < length; ++index) {
        const MAREA_Character8 CHARACTER = text[index];

        MAREA_Uinteger64 digit = 0;

        if (CHARACTER == '_') {
            continue;
        }

        if ((CHARACTER >= '0') && (CHARACTER <= '9')) {
            digit = (MAREA_Uinteger64)(CHARACTER - '0');
        } else if ((CHARACTER >= 'a') && (CHARACTER <= 'f')) {
            digit = (MAREA_Uinteger64)(CHARACTER - 'a') + 10;
        } else if ((CHARACTER >= 'A') && (CHARACTER <= 'F')) {
            digit = (MAREA_Uinteger64)(CHARACTER - 'A') + 10;
        } else {
            return MAREA_BOOLEAN_FALSE;
        }

        if ((digit >= base) || (value > (((MAREA_Uinteger64)INT64_MAX - digit) / base))) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
    }

    *outValue = (MAREA_Sinteger64)value;

    return MAREA_BOOLEAN_TRUE;
}

// INFO: the product is built from the two magnitudes in an unsigned 64-bit integer, so no signed step can overflow. A product that does not fit a signed 64-bit integer gives unknown.
static MAREA_G_Value MAREA_CALL MAREA_G_Multiply(MAREA_Sinteger64 left, MAREA_Sinteger64 right)
{
    if ((left == 0) || (right == 0)) {
        return MAREA_G_FromInteger(0);
    }

    const MAREA_Uinteger64 LEFT_MAGNITUDE = (left < 0) ? (MAREA_Uinteger64)0 - (MAREA_Uinteger64)left : (MAREA_Uinteger64)left;

    const MAREA_Uinteger64 RIGHT_MAGNITUDE = (right < 0) ? (MAREA_Uinteger64)0 - (MAREA_Uinteger64)right : (MAREA_Uinteger64)right;

    if (LEFT_MAGNITUDE > (UINT64_MAX / RIGHT_MAGNITUDE)) {
        return MAREA_G_Unknown();
    }

    const MAREA_Uinteger64 MAGNITUDE = (LEFT_MAGNITUDE * RIGHT_MAGNITUDE);

    const MAREA_Boolean NEGATIVE = ((left < 0) != (right < 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (NEGATIVE == MAREA_BOOLEAN_TRUE) {
        // INFO: the magnitude 2^63 is the lowest signed value, so it fits only with a negative sign.
        if (MAGNITUDE > ((MAREA_Uinteger64)INT64_MAX + 1)) {
            return MAREA_G_Unknown();
        }

        return MAREA_G_FromInteger((MAGNITUDE == ((MAREA_Uinteger64)INT64_MAX + 1)) ? INT64_MIN : -(MAREA_Sinteger64)MAGNITUDE);
    }

    return (MAGNITUDE > (MAREA_Uinteger64)INT64_MAX) ? MAREA_G_Unknown() : MAREA_G_FromInteger((MAREA_Sinteger64)MAGNITUDE);
}

// INFO: one arithmetic step over 64-bit integers. Overflow, a division by zero and INT64_MIN / -1 give unknown, never a diagnostic: the rules of the constant arithmetic belong to other checks.
static MAREA_G_Value MAREA_CALL MAREA_G_Arithmetic(MAREA_TokenKind operatorKind, MAREA_Sinteger64 left, MAREA_Sinteger64 right)
{
    switch (operatorKind) {
    case MAREA_TOKEN_KIND_PLUS:
        if (((right > 0) && (left > (INT64_MAX - right))) || ((right < 0) && (left < (INT64_MIN - right)))) {
            return MAREA_G_Unknown();
        }

        return MAREA_G_FromInteger(left + right);
    case MAREA_TOKEN_KIND_MINUS:
        if (((right < 0) && (left > (INT64_MAX + right))) || ((right > 0) && (left < (INT64_MIN + right)))) {
            return MAREA_G_Unknown();
        }

        return MAREA_G_FromInteger(left - right);
    case MAREA_TOKEN_KIND_STAR:
        return MAREA_G_Multiply(left, right);
    case MAREA_TOKEN_KIND_SLASH:
    case MAREA_TOKEN_KIND_PERCENT:
        if ((right == 0) || ((left == INT64_MIN) && (right == -1))) {
            return MAREA_G_Unknown();
        }

        return MAREA_G_FromInteger((operatorKind == MAREA_TOKEN_KIND_SLASH) ? (left / right) : (left % right));
    default:
        return MAREA_G_Unknown();
    }
}

static MAREA_G_Value MAREA_CALL MAREA_G_Compare(MAREA_TokenKind operatorKind, MAREA_Sinteger64 left, MAREA_Sinteger64 right)
{
    switch (operatorKind) {
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
        return MAREA_G_FromBoolean((left == right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_BANG_EQUAL:
        return MAREA_G_FromBoolean((left != right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_LESS:
        return MAREA_G_FromBoolean((left < right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_LESS_EQUAL:
        return MAREA_G_FromBoolean((left <= right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_GREATER:
        return MAREA_G_FromBoolean((left > right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return MAREA_G_FromBoolean((left >= right) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    default:
        return MAREA_G_Unknown();
    }
}

// INFO: integer arithmetic in the width of a type. A 64-bit type and a number with no type keep the rule of G_Arithmetic: an overflow is unknown.
static MAREA_G_Value MAREA_CALL MAREA_G_TypedArithmetic(MAREA_TokenKind operatorKind, MAREA_G_Value left, MAREA_G_Value right)
{
    const MAREA_G_Value *typed = (left.bits > 0) ? &left : &right;

    if ((left.bits > 0) && (right.bits > 0) && ((left.bits != right.bits) || (left.isSigned != right.isSigned))) {
        return MAREA_G_Unknown();
    }

    if (typed->bits == 0) {
        return MAREA_G_Arithmetic(operatorKind, left.integer, right.integer);
    }

    if (typed->bits == 64) {
        const MAREA_G_Value RESULT = MAREA_G_Arithmetic(operatorKind, left.integer, right.integer);

        return (RESULT.kind == MAREA_G_VALUE_KIND_INTEGER) ? MAREA_G_Typed(RESULT.integer, typed->bits, typed->isSigned) : RESULT;
    }

    const MAREA_Uinteger64 LEFT = (MAREA_Uinteger64)left.integer;

    const MAREA_Uinteger64 RIGHT = (MAREA_Uinteger64)right.integer;

    switch (operatorKind) {
    case MAREA_TOKEN_KIND_PLUS:
        return MAREA_G_Typed((MAREA_Sinteger64)(LEFT + RIGHT), typed->bits, typed->isSigned);
    case MAREA_TOKEN_KIND_MINUS:
        return MAREA_G_Typed((MAREA_Sinteger64)(LEFT - RIGHT), typed->bits, typed->isSigned);
    case MAREA_TOKEN_KIND_STAR:
        return MAREA_G_Typed((MAREA_Sinteger64)(LEFT * RIGHT), typed->bits, typed->isSigned);
    default:
    {
        const MAREA_G_Value RESULT = MAREA_G_Arithmetic(operatorKind, left.integer, right.integer);

        return (RESULT.kind == MAREA_G_VALUE_KIND_INTEGER) ? MAREA_G_Typed(RESULT.integer, typed->bits, typed->isSigned) : RESULT;
    }
    }
}

static MAREA_G_Value MAREA_CALL MAREA_G_FloatBinary(MAREA_TokenKind operatorKind, MAREA_G_Value left, MAREA_G_Value right)
{
    const MAREA_Uinteger32 BITS = (left.bits == 32) || (right.bits == 32) ? 32 : ((left.bits == 64) || (right.bits == 64) ? 64 : 0);

    // INFO: an integer literal fits a float type, so it is read as a float.
    const MAREA_Float64 LEFT = MAREA_G_FromFloat((left.kind == MAREA_G_VALUE_KIND_FLOAT) ? left.floating : (MAREA_Float64)left.integer, BITS).floating;

    const MAREA_Float64 RIGHT = MAREA_G_FromFloat((right.kind == MAREA_G_VALUE_KIND_FLOAT) ? right.floating : (MAREA_Float64)right.integer, BITS).floating;

    switch (operatorKind) {
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
        return MAREA_G_FromBoolean((LEFT == RIGHT) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_BANG_EQUAL:
        return MAREA_G_FromBoolean((LEFT != RIGHT) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_LESS:
        return MAREA_G_FromBoolean((LEFT < RIGHT) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_LESS_EQUAL:
        return MAREA_G_FromBoolean((LEFT <= RIGHT) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_GREATER:
        return MAREA_G_FromBoolean((LEFT > RIGHT) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return MAREA_G_FromBoolean((LEFT >= RIGHT) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_TOKEN_KIND_PLUS:
        return MAREA_G_FromFloat(LEFT + RIGHT, BITS);
    case MAREA_TOKEN_KIND_MINUS:
        return MAREA_G_FromFloat(LEFT - RIGHT, BITS);
    case MAREA_TOKEN_KIND_STAR:
        return MAREA_G_FromFloat(LEFT * RIGHT, BITS);
    case MAREA_TOKEN_KIND_SLASH:
        return (RIGHT == 0.0) ? MAREA_G_Unknown() : MAREA_G_FromFloat(LEFT / RIGHT, BITS);
    default:
        return MAREA_G_Unknown();
    }
}

// INFO: only equality of two string literals folds. Two texts that differ and hold an escape may still be the same string, so they are not decided.
static MAREA_G_Value MAREA_CALL MAREA_G_StringBinary(MAREA_TokenKind operatorKind, MAREA_G_Value left, MAREA_G_Value right)
{
    if ((operatorKind != MAREA_TOKEN_KIND_EQUAL_EQUAL) && (operatorKind != MAREA_TOKEN_KIND_BANG_EQUAL)) {
        return MAREA_G_Unknown();
    }

    const MAREA_Boolean SAME = MAREA_SemaTextEquals(left.text, left.length, right.text, right.length);

    if ((SAME == MAREA_BOOLEAN_FALSE) && ((memchr(left.text, '\\', left.length) != MAREA_NULLPTR) || (memchr(right.text, '\\', right.length) != MAREA_NULLPTR))) {
        return MAREA_G_Unknown();
    }

    return MAREA_G_FromBoolean((SAME == ((operatorKind == MAREA_TOKEN_KIND_EQUAL_EQUAL) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
}

// INFO: a float literal with "_" between digits. A text that is too long for the buffer is not decided.
static MAREA_G_Value MAREA_CALL MAREA_G_ReadFloat(const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_Character8 buffer[MAREA_G_FLOAT_TEXT_LIMIT];

    MAREA_IntegerSize count = 0;

    for (MAREA_IntegerSize index = 0; index < length; ++index) {
        if (text[index] == '_') {
            continue;
        }

        if ((count + 1) >= MAREA_G_FLOAT_TEXT_LIMIT) {
            return MAREA_G_Unknown();
        }

        buffer[count] = text[index];
        ++count;
    }

    buffer[count] = '\0';

    MAREA_Character8 *end = MAREA_NULLPTR;

    const MAREA_Float64 VALUE = strtod(buffer, &end);

    return ((count > 0) && (end == (buffer + count))) ? MAREA_G_FromFloat(VALUE, 0) : MAREA_G_Unknown();
}

// INFO: the value of a constant has the declared type of the constant.
static MAREA_G_Value MAREA_CALL MAREA_G_AsDeclared(const MAREA_SemaContext *context, const MAREA_AstNode *type, MAREA_G_Value value)
{
    static const struct
    {
        const MAREA_Character8 *name;
        MAREA_Uinteger32 bits;
        MAREA_Boolean isSigned;
    } INTEGERS[] = {
        { "sinteger8", 8, MAREA_BOOLEAN_TRUE },
        { "sinteger16", 16, MAREA_BOOLEAN_TRUE },
        { "sinteger32", 32, MAREA_BOOLEAN_TRUE },
        { "sinteger64", 64, MAREA_BOOLEAN_TRUE },
        { "uinteger8", 8, MAREA_BOOLEAN_FALSE },
        { "uinteger16", 16, MAREA_BOOLEAN_FALSE },
        { "uinteger32", 32, MAREA_BOOLEAN_FALSE },
        { "uinteger64", 64, MAREA_BOOLEAN_FALSE },
    };

    const MAREA_Character8 *name = MAREA_NULLPTR;

    MAREA_IntegerSize length = 0;

    if (MAREA_SemaPrimitiveOfType(context, type, &name, &length) == MAREA_BOOLEAN_FALSE) {
        return value;
    }

    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(INTEGERS); ++index) {
        if (MAREA_SemaTextEquals(name, length, INTEGERS[index].name, strlen(INTEGERS[index].name)) == MAREA_BOOLEAN_TRUE) {
            return (value.kind == MAREA_G_VALUE_KIND_INTEGER) ? MAREA_G_Typed(value.integer, INTEGERS[index].bits, INTEGERS[index].isSigned) : value;
        }
    }

    const MAREA_Uinteger32 FLOAT_BITS = (MAREA_SemaTextEquals(name, length, "float32", 7) == MAREA_BOOLEAN_TRUE) ? 32 : ((MAREA_SemaTextEquals(name, length, "float64", 7) == MAREA_BOOLEAN_TRUE) ? 64 : 0);

    if ((FLOAT_BITS == 0) || ((value.kind != MAREA_G_VALUE_KIND_FLOAT) && (value.kind != MAREA_G_VALUE_KIND_INTEGER))) {
        return value;
    }

    return MAREA_G_FromFloat((value.kind == MAREA_G_VALUE_KIND_FLOAT) ? value.floating : (MAREA_Float64)value.integer, FLOAT_BITS);
}

// INFO: size_of<T>() and align_of<T>() are uinteger64 values of the layout record. A name that the unit binds makes the call a plain call.
static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateCall(const MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    const MAREA_AstNode *callee = MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *typeArguments = MAREA_SemaChildAt(call, 1);

    if ((callee == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(callee) != MAREA_AST_KIND_IDENTIFIER) || (typeArguments == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(typeArguments) != MAREA_AST_KIND_TYPE_ARGUMENTS) || (MAREA_AstNodeGetChildCount(typeArguments) != 1)) {
        return MAREA_G_Unknown();
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(callee);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(callee);

    const MAREA_Boolean IS_SIZE = MAREA_SemaTextEquals(text, LENGTH, "size_of", 7);

    if (((IS_SIZE == MAREA_BOOLEAN_FALSE) && (MAREA_SemaTextEquals(text, LENGTH, "align_of", 8) == MAREA_BOOLEAN_FALSE)) || (MAREA_SemaLocalFind(context, text, LENGTH) != MAREA_NULLPTR) || (MAREA_SemaNameTableFind(context, text, LENGTH) != MAREA_NULLPTR)) {
        return MAREA_G_Unknown();
    }

    MAREA_Uinteger64 size = 0;

    MAREA_Uinteger64 alignment = 0;

    if (MAREA_SemaMeasureWrittenType(context, MAREA_SemaChildAt(typeArguments, 0), &size, &alignment) == MAREA_BOOLEAN_FALSE) {
        return MAREA_G_Unknown();
    }

    return MAREA_G_Typed((MAREA_Sinteger64)((IS_SIZE == MAREA_BOOLEAN_TRUE) ? size : alignment), 64, MAREA_BOOLEAN_FALSE);
}

static MAREA_G_Value MAREA_CALL MAREA_G_Evaluate(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth);

// INFO: a build_value call is the value that the build gave when it gave one that sema reads (a boolean or an integer), and else its default, read one level deeper. A given text of another type, or a text that does not fit, is unknown.
static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateBuildValue(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    MAREA_Boolean boolean = MAREA_BOOLEAN_FALSE;

    MAREA_Sinteger64 integer = 0;

    switch (MAREA_SemaBuildValueGiven(context, node, &boolean, &integer)) {
    case MAREA_SEMA_BUILD_GIVEN_BOOLEAN:
        return MAREA_G_FromBoolean(boolean);
    case MAREA_SEMA_BUILD_GIVEN_INTEGER:
        return MAREA_G_FromInteger(integer);
    case MAREA_SEMA_BUILD_GIVEN_UNKNOWN:
        return MAREA_G_Unknown();
    case MAREA_SEMA_BUILD_GIVEN_NOT_GIVEN:
        break;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(node, 0);

    const MAREA_AstNode *list = MAREA_SemaChildAt(node, 1);

    const MAREA_Boolean IS_BUILD_VALUE = ((name != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(name) == MAREA_AST_KIND_IDENTIFIER) && (MAREA_SemaTextEquals(MAREA_AstNodeGetText(name), MAREA_AstNodeGetTextLength(name), "build_value", 11) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((IS_BUILD_VALUE == MAREA_BOOLEAN_FALSE) || (list == MAREA_NULLPTR) || (MAREA_AstNodeGetChildCount(list) != 3)) {
        return MAREA_G_Unknown();
    }

    return MAREA_G_Evaluate(context, MAREA_SemaChildAt(list, 2), (depth + 1));
}

// INFO: a name is a file constant only when no open scope declares a local or a parameter of the same name. The value of the constant is its initial value, read one level deeper so a cycle ends as unknown.
static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateName(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    const MAREA_Character8 *text = MAREA_AstNodeGetText(node);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(node);

    if (MAREA_SemaLocalFind(context, text, LENGTH) != MAREA_NULLPTR) {
        return MAREA_G_Unknown();
    }

    const MAREA_SemaName *entry = MAREA_SemaNameTableFind(context, text, LENGTH);

    if ((entry == MAREA_NULLPTR) || (entry->kind != MAREA_SEMA_NAME_KIND_DECLARATION) || (entry->declaration == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(entry->declaration) != MAREA_AST_KIND_CONSTANT_DECLARATION)) {
        return MAREA_G_Unknown();
    }

    const MAREA_IntegerSize COUNT = MAREA_AstNodeGetChildCount(entry->declaration);

    return MAREA_G_AsDeclared(context, MAREA_SemaChildAt(entry->declaration, (COUNT - 2)), MAREA_G_Evaluate(context, MAREA_SemaChildAt(entry->declaration, (COUNT - 1)), (depth + 1)));
}

static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateLogical(MAREA_TokenKind operatorKind, MAREA_G_Value left, MAREA_G_Value right)
{
    const MAREA_Boolean IS_AND = (operatorKind == MAREA_TOKEN_KIND_KEYWORD_AND) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    // INFO: a decided operand ends the question even when the other side is unknown: false and x is false, true or x is true.
    if ((left.kind == MAREA_G_VALUE_KIND_BOOLEAN) && (left.boolean == ((IS_AND == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE))) {
        return left;
    }

    if ((right.kind == MAREA_G_VALUE_KIND_BOOLEAN) && (right.boolean == ((IS_AND == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE))) {
        return right;
    }

    if ((left.kind == MAREA_G_VALUE_KIND_BOOLEAN) && (right.kind == MAREA_G_VALUE_KIND_BOOLEAN)) {
        return left;
    }

    return MAREA_G_Unknown();
}

static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateBinary(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(node);

    const MAREA_G_Value LEFT = MAREA_G_Evaluate(context, MAREA_SemaChildAt(node, 0), (depth + 1));

    const MAREA_G_Value RIGHT = MAREA_G_Evaluate(context, MAREA_SemaChildAt(node, 1), (depth + 1));

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_AND) || (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_OR)) {
        return MAREA_G_EvaluateLogical(OPERATOR, LEFT, RIGHT);
    }

    if ((LEFT.kind == MAREA_G_VALUE_KIND_BOOLEAN) && (RIGHT.kind == MAREA_G_VALUE_KIND_BOOLEAN)) {
        if (OPERATOR == MAREA_TOKEN_KIND_EQUAL_EQUAL) {
            return MAREA_G_FromBoolean((LEFT.boolean == RIGHT.boolean) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
        }

        if (OPERATOR == MAREA_TOKEN_KIND_BANG_EQUAL) {
            return MAREA_G_FromBoolean((LEFT.boolean != RIGHT.boolean) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
        }

        return MAREA_G_Unknown();
    }

    if ((LEFT.kind == MAREA_G_VALUE_KIND_STRING) && (RIGHT.kind == MAREA_G_VALUE_KIND_STRING)) {
        return MAREA_G_StringBinary(OPERATOR, LEFT, RIGHT);
    }

    const MAREA_Boolean LEFT_NUMBER = ((LEFT.kind == MAREA_G_VALUE_KIND_INTEGER) || (LEFT.kind == MAREA_G_VALUE_KIND_FLOAT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    const MAREA_Boolean RIGHT_NUMBER = ((RIGHT.kind == MAREA_G_VALUE_KIND_INTEGER) || (RIGHT.kind == MAREA_G_VALUE_KIND_FLOAT)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if ((LEFT_NUMBER == MAREA_BOOLEAN_FALSE) || (RIGHT_NUMBER == MAREA_BOOLEAN_FALSE)) {
        return MAREA_G_Unknown();
    }

    if ((LEFT.kind == MAREA_G_VALUE_KIND_FLOAT) || (RIGHT.kind == MAREA_G_VALUE_KIND_FLOAT)) {
        return MAREA_G_FloatBinary(OPERATOR, LEFT, RIGHT);
    }

    const MAREA_G_Value COMPARED = MAREA_G_Compare(OPERATOR, LEFT.integer, RIGHT.integer);

    return (COMPARED.kind != MAREA_G_VALUE_KIND_UNKNOWN) ? COMPARED : MAREA_G_TypedArithmetic(OPERATOR, LEFT, RIGHT);
}

static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateUnary(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(node);

    const MAREA_G_Value OPERAND = MAREA_G_Evaluate(context, MAREA_SemaChildAt(node, 0), (depth + 1));

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_NOT) && (OPERAND.kind == MAREA_G_VALUE_KIND_BOOLEAN)) {
        return MAREA_G_FromBoolean((OPERAND.boolean == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE);
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_MINUS) && (OPERAND.kind == MAREA_G_VALUE_KIND_INTEGER) && (OPERAND.integer != INT64_MIN)) {
        return (OPERAND.bits == 0) ? MAREA_G_FromInteger(-OPERAND.integer) : MAREA_G_Typed(-OPERAND.integer, OPERAND.bits, OPERAND.isSigned);
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_MINUS) && (OPERAND.kind == MAREA_G_VALUE_KIND_FLOAT)) {
        return MAREA_G_FromFloat(-OPERAND.floating, OPERAND.bits);
    }

    return MAREA_G_Unknown();
}

static MAREA_G_Value MAREA_CALL MAREA_G_Evaluate(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    if ((node == MAREA_NULLPTR) || (depth >= MAREA_G_EVALUATION_DEPTH_LIMIT)) {
        return MAREA_G_Unknown();
    }

    switch (MAREA_AstNodeGetKind(node)) {
    case MAREA_AST_KIND_BOOLEAN_LITERAL:
        return MAREA_G_FromBoolean((MAREA_AstNodeGetTextLength(node) == 4) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    case MAREA_AST_KIND_INTEGER_LITERAL:
    {
        MAREA_Sinteger64 value = 0;

        return (MAREA_G_ReadInteger(MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node), &value) == MAREA_BOOLEAN_TRUE) ? MAREA_G_FromInteger(value) : MAREA_G_Unknown();
    }
    case MAREA_AST_KIND_FLOAT_LITERAL:
        return MAREA_G_ReadFloat(MAREA_AstNodeGetText(node), MAREA_AstNodeGetTextLength(node));
    case MAREA_AST_KIND_STRING_LITERAL:
        return (MAREA_G_Value){ .kind = MAREA_G_VALUE_KIND_STRING, .text = MAREA_AstNodeGetText(node), .length = MAREA_AstNodeGetTextLength(node) };
    case MAREA_AST_KIND_CALL_EXPRESSION:
        return MAREA_G_EvaluateCall(context, node);
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_EvaluateName(context, node, depth);
    case MAREA_AST_KIND_MACRO_CALL:
        return MAREA_G_EvaluateBuildValue(context, node, depth);
    case MAREA_AST_KIND_PARENTHESIZED_EXPRESSION:
        return MAREA_G_Evaluate(context, MAREA_SemaChildAt(node, 0), (depth + 1));
    case MAREA_AST_KIND_UNARY_EXPRESSION:
        return MAREA_G_EvaluateUnary(context, node, depth);
    case MAREA_AST_KIND_BINARY_EXPRESSION:
        return MAREA_G_EvaluateBinary(context, node, depth);
    default:
        return MAREA_G_Unknown();
    }
}

// INFO: the message is the prefix, then ": ", then the text of the string literal without its quotes. A node that is not a string literal gives the prefix alone.
static MAREA_SemaExcept MAREA_CALL MAREA_G_ReportWithText(MAREA_SemaContext *context, MAREA_SemaDiagnosticCode code, MAREA_SourcePosition position, const MAREA_Character8 *prefix, const MAREA_AstNode *literal)
{
    const MAREA_IntegerSize PREFIX_LENGTH = strlen(prefix);

    const MAREA_Boolean HAS_TEXT = ((literal != MAREA_NULLPTR) && (MAREA_AstNodeGetKind(literal) == MAREA_AST_KIND_STRING_LITERAL) && (MAREA_AstNodeGetTextLength(literal) >= 2)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;

    if (HAS_TEXT == MAREA_BOOLEAN_FALSE) {
        MAREA_SemaReport(context, code, position, prefix);

        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = (MAREA_AstNodeGetText(literal) + 1);

    const MAREA_IntegerSize TEXT_LENGTH = (MAREA_AstNodeGetTextLength(literal) - 2);

    // ERROR: a total that does not fit in MAREA_IntegerSize would allocate a small block and then copy past it.
    if (TEXT_LENGTH > (MAREA_INTEGER_SIZE_MAXIMUM - PREFIX_LENGTH - MAREA_INTEGER_SIZE(3))) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(context->arena, (PREFIX_LENGTH + TEXT_LENGTH + MAREA_INTEGER_SIZE(3)), MAREA_ALIGNOF(MAREA_Character8), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_Character8 *message = memory;

    memcpy(message, prefix, PREFIX_LENGTH);
    message[PREFIX_LENGTH] = ':';
    message[(PREFIX_LENGTH + MAREA_INTEGER_SIZE(1))] = ' ';
    memcpy((message + PREFIX_LENGTH + MAREA_INTEGER_SIZE(2)), text, TEXT_LENGTH);
    message[(PREFIX_LENGTH + TEXT_LENGTH + MAREA_INTEGER_SIZE(2))] = '\0';

    MAREA_SemaReport(context, code, position, message);

    return MAREA_SEMA_EXCEPT_SUCCESS;
}

MAREA_SemaExcept MAREA_CALL MAREA_SemaCheckMacroEvaluation(MAREA_SemaContext *context, const MAREA_AstNode *call)
{
    if ((context == MAREA_NULLPTR) || (call == MAREA_NULLPTR)) {
        return MAREA_SEMA_EXCEPT_ERROR_NULL_POINTER;
    }

    const MAREA_AstNode *name = MAREA_SemaChildAt(call, 0);

    const MAREA_AstNode *arguments = MAREA_SemaChildAt(call, 1);

    if ((name == MAREA_NULLPTR) || (MAREA_AstNodeGetKind(name) != MAREA_AST_KIND_IDENTIFIER)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *text = MAREA_AstNodeGetText(name);

    const MAREA_IntegerSize LENGTH = MAREA_AstNodeGetTextLength(name);

    const MAREA_IntegerSize ARGUMENT_COUNT = (arguments == MAREA_NULLPTR) ? 0 : MAREA_AstNodeGetChildCount(arguments);

    // INFO: the call itself says "stop", so compile_error raises whatever its arguments are.
    if (MAREA_SemaTextEquals(text, LENGTH, "compile_error", MAREA_INTEGER_SIZE(13)) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_ReportWithText(context, MAREA_SEMA_DIAGNOSTIC_CODE_COMPILE_TIME_ASSERTION_FAILED, MAREA_AstNodeGetPosition(call), "compile_error", (ARGUMENT_COUNT > 0) ? MAREA_SemaChildAt(arguments, 0) : MAREA_NULLPTR);
    }

    // INFO: the argument count of static_assert is the shape rule of another check, so a call with the wrong count is skipped here.
    if ((MAREA_SemaTextEquals(text, LENGTH, "static_assert", MAREA_INTEGER_SIZE(13)) == MAREA_BOOLEAN_FALSE) || (ARGUMENT_COUNT != 2)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    const MAREA_G_Value CONDITION = MAREA_G_Evaluate(context, MAREA_SemaChildAt(arguments, 0), 0);

    if ((CONDITION.kind != MAREA_G_VALUE_KIND_BOOLEAN) || (CONDITION.boolean == MAREA_BOOLEAN_TRUE)) {
        return MAREA_SEMA_EXCEPT_SUCCESS;
    }

    return MAREA_G_ReportWithText(context, MAREA_SEMA_DIAGNOSTIC_CODE_COMPILE_TIME_ASSERTION_FAILED, MAREA_AstNodeGetPosition(call), "static assertion failed", MAREA_SemaChildAt(arguments, 1));
}
