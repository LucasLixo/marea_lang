// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

#include <string.h>

#define MAREA_G_EVALUATION_DEPTH_LIMIT MAREA_UINTEGER_32(32)

// INFO: the three answers of the evaluator, plus the integer kind of a value that is not a boolean.
typedef enum MAREA_G_ValueKind
{
    MAREA_G_VALUE_KIND_UNKNOWN = 0,
    MAREA_G_VALUE_KIND_BOOLEAN = 1,
    MAREA_G_VALUE_KIND_INTEGER = 2,
} MAREA_G_ValueKind;

typedef struct MAREA_G_Value
{
    MAREA_G_ValueKind kind;
    MAREA_Boolean boolean;
    MAREA_Sinteger64 integer;
} MAREA_G_Value;

static MAREA_G_Value MAREA_CALL MAREA_G_Unknown(void)
{
    return (MAREA_G_Value){ MAREA_G_VALUE_KIND_UNKNOWN, MAREA_BOOLEAN_FALSE, 0 };
}

static MAREA_G_Value MAREA_CALL MAREA_G_FromBoolean(MAREA_Boolean value)
{
    return (MAREA_G_Value){ MAREA_G_VALUE_KIND_BOOLEAN, value, 0 };
}

static MAREA_G_Value MAREA_CALL MAREA_G_FromInteger(MAREA_Sinteger64 value)
{
    return (MAREA_G_Value){ MAREA_G_VALUE_KIND_INTEGER, MAREA_BOOLEAN_FALSE, value };
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

static MAREA_G_Value MAREA_CALL MAREA_G_Evaluate(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth);

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

    return MAREA_G_Evaluate(context, MAREA_SemaChildAt(entry->declaration, 2), (depth + 1));
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

    if ((LEFT.kind != MAREA_G_VALUE_KIND_INTEGER) || (RIGHT.kind != MAREA_G_VALUE_KIND_INTEGER)) {
        return MAREA_G_Unknown();
    }

    const MAREA_G_Value COMPARED = MAREA_G_Compare(OPERATOR, LEFT.integer, RIGHT.integer);

    return (COMPARED.kind != MAREA_G_VALUE_KIND_UNKNOWN) ? COMPARED : MAREA_G_Arithmetic(OPERATOR, LEFT.integer, RIGHT.integer);
}

static MAREA_G_Value MAREA_CALL MAREA_G_EvaluateUnary(const MAREA_SemaContext *context, const MAREA_AstNode *node, MAREA_Uinteger32 depth)
{
    const MAREA_TokenKind OPERATOR = MAREA_AstNodeGetOperator(node);

    const MAREA_G_Value OPERAND = MAREA_G_Evaluate(context, MAREA_SemaChildAt(node, 0), (depth + 1));

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_NOT) && (OPERAND.kind == MAREA_G_VALUE_KIND_BOOLEAN)) {
        return MAREA_G_FromBoolean((OPERAND.boolean == MAREA_BOOLEAN_TRUE) ? MAREA_BOOLEAN_FALSE : MAREA_BOOLEAN_TRUE);
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_MINUS) && (OPERAND.kind == MAREA_G_VALUE_KIND_INTEGER) && (OPERAND.integer != INT64_MIN)) {
        return MAREA_G_FromInteger(-OPERAND.integer);
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
    case MAREA_AST_KIND_IDENTIFIER:
        return MAREA_G_EvaluateName(context, node, depth);
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
