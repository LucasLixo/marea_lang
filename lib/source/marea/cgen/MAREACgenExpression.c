// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(MAREA_CGEN_LITERAL_LIMIT)
    #define MAREA_CGEN_LITERAL_LIMIT MAREA_INTEGER_SIZE(128)
#endif

static MAREA_Boolean MAREA_CALL MAREA_G_IsFloatKind(MAREA_Uinteger8 kind)
{
    return ((kind == MAREA_CGEN_KIND_FLOAT_32) || (kind == MAREA_CGEN_KIND_FLOAT_64));
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsNumberKind(MAREA_Uinteger8 kind)
{
    return (kind < MAREA_CGEN_KIND_NUMBER_COUNT);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsSignedKind(MAREA_Uinteger8 kind)
{
    return (kind < MAREA_CGEN_KIND_UINTEGER_8);
}

static MAREA_Boolean MAREA_CALL MAREA_G_ParseInteger(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger64 *outValue)
{
    MAREA_Uinteger64 base = MAREA_UINTEGER_64(10);
    MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0);

    if ((length > MAREA_INTEGER_SIZE(2)) && (text[0] == '0') && (text[1] == 'x')) {
        base = MAREA_UINTEGER_64(16);
        position = MAREA_INTEGER_SIZE(2);
    } else if ((length > MAREA_INTEGER_SIZE(2)) && (text[0] == '0') && (text[1] == 'b')) {
        base = MAREA_UINTEGER_64(2);
        position = MAREA_INTEGER_SIZE(2);
    }

    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);
    MAREA_Boolean seen = MAREA_BOOLEAN_FALSE;

    for (; position < length; position = (position + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Character8 SYMBOL = text[position];

        if (SYMBOL == '_') {
            continue;
        }

        MAREA_Uinteger64 digit = MAREA_UINTEGER_64(0);

        if ((SYMBOL >= '0') && (SYMBOL <= '9')) {
            digit = (MAREA_Uinteger64)(SYMBOL - '0');
        } else if ((SYMBOL >= 'a') && (SYMBOL <= 'f')) {
            digit = (MAREA_Uinteger64)((SYMBOL - 'a') + 10);
        } else if ((SYMBOL >= 'A') && (SYMBOL <= 'F')) {
            digit = (MAREA_Uinteger64)((SYMBOL - 'A') + 10);
        } else {
            return MAREA_BOOLEAN_FALSE;
        }

        if (digit >= base) {
            return MAREA_BOOLEAN_FALSE;
        }

        value = ((value * base) + digit);
        seen = MAREA_BOOLEAN_TRUE;
    }

    *outValue = value;

    return seen;
}

static MAREA_Boolean MAREA_CALL MAREA_G_ParseFloat(const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Float64 *outValue)
{
    MAREA_Character8 buffer[MAREA_CGEN_LITERAL_LIMIT];
    MAREA_IntegerSize used = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize position = MAREA_INTEGER_SIZE(0); position < length; position = (position + MAREA_INTEGER_SIZE(1))) {
        if (text[position] == '_') {
            continue;
        }

        if (used >= (MAREA_CGEN_LITERAL_LIMIT - MAREA_INTEGER_SIZE(1))) {
            return MAREA_BOOLEAN_FALSE;
        }

        buffer[used] = text[position];
        used = (used + MAREA_INTEGER_SIZE(1));
    }

    buffer[used] = '\0';

    if (used == MAREA_INTEGER_SIZE(0)) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Character8 *end = MAREA_NULLPTR;

    *outValue = strtod(buffer, &end);

    return (*end == '\0');
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_IrNode *node, const MAREA_Character8 *expected)
{
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(node);

    return (((MAREA_IntegerSize)strlen(expected) == LENGTH) && (memcmp(MAREA_IrNodeGetText(node), expected, LENGTH) == 0));
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteIntegerLiteral(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean negate, MAREA_Uinteger8 *outKind)
{
    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

    if (MAREA_G_ParseInteger(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), &value) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An integer literal cannot be read.");
    }

    MAREA_Uinteger8 kind = MAREA_CgenKindOfNode(node);

    if (MAREA_G_IsNumberKind(kind) == MAREA_BOOLEAN_FALSE) {
        kind = MAREA_CGEN_KIND_SINTEGER_32;
    }

    *outKind = kind;

    if (negate == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(generator->buffer, "-");
    }

    if (MAREA_G_IsFloatKind(kind) == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteNumber(generator->buffer, value);
        MAREA_CgenWriteText(generator->buffer, ".0");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CgenWriteText(generator->buffer, "(");
    MAREA_CgenWriteCType(generator->buffer, kind);
    MAREA_CgenWriteText(generator->buffer, ")");
    MAREA_CgenWriteNumber(generator->buffer, value);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteFloatLiteral(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean negate, MAREA_Uinteger8 *outKind)
{
    MAREA_Float64 value = MAREA_FLOAT_64(0.0);

    if (MAREA_G_ParseFloat(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), &value) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A float literal cannot be read.");
    }

    MAREA_Uinteger8 kind = MAREA_CgenKindOfNode(node);

    if (MAREA_G_IsFloatKind(kind) == MAREA_BOOLEAN_FALSE) {
        kind = MAREA_CGEN_KIND_FLOAT_64;
    }

    *outKind = kind;

    MAREA_Character8 text[64];

    const int WRITTEN = snprintf(text, sizeof(text), "%.17g", (negate == MAREA_BOOLEAN_TRUE) ? -value : value);

    if ((WRITTEN <= 0) || ((MAREA_IntegerSize)WRITTEN >= (MAREA_IntegerSize)sizeof(text))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A float literal cannot be written.");
    }

    MAREA_CgenWriteText(generator->buffer, text);

    if (kind == MAREA_CGEN_KIND_FLOAT_32) {
        MAREA_CgenWriteText(generator->buffer, "f");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteIdentifier(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind)
{
    const MAREA_CgenSymbol *symbol = MAREA_CgenFindSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

    if (symbol == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A name is not a local or a parameter of this function.");
    }

    *outKind = symbol->kind;

    MAREA_CgenWriteMangled(generator->buffer, "v_", symbol->name, symbol->length);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteArithmetic(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_TokenKind operator, const MAREA_IrNode *left, const MAREA_IrNode *right, MAREA_Uinteger8 *outKind)
{
    MAREA_Uinteger8 leftKind = MAREA_CGEN_KIND_NONE;
    MAREA_Uinteger8 rightKind = MAREA_CGEN_KIND_NONE;

    // INFO: a kind is read once from the left operand; sema already agreed both sides on one type.
    MAREA_CgenBuffer scratch = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };
    MAREA_CgenBuffer *savedBuffer = generator->buffer;

    generator->buffer = &scratch;
    const MAREA_CgenExcept LEFT_EXCEPT = MAREA_CgenWriteExpression(generator, left, &leftKind);
    generator->buffer = savedBuffer;

    MAREA_CGEN_TRY(LEFT_EXCEPT);

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        *outKind = MAREA_CGEN_KIND_NONE;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CgenBuffer scratchRight = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };

    generator->buffer = &scratchRight;
    const MAREA_CgenExcept RIGHT_EXCEPT = MAREA_CgenWriteExpression(generator, right, &rightKind);
    generator->buffer = savedBuffer;

    MAREA_CGEN_TRY(RIGHT_EXCEPT);

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        *outKind = MAREA_CGEN_KIND_NONE;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger8 KIND = (MAREA_G_IsNumberKind(leftKind) == MAREA_BOOLEAN_TRUE) ? leftKind : rightKind;

    if (MAREA_G_IsNumberKind(KIND) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An arithmetic operator needs two numbers.");
    }

    *outKind = KIND;

    const MAREA_Boolean FLOAT = MAREA_G_IsFloatKind(KIND);
    const MAREA_Boolean SIGNED = MAREA_G_IsSignedKind(KIND);

    if (FLOAT == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *symbol = "+";

        if (operator == MAREA_TOKEN_KIND_MINUS) {
            symbol = "-";
        } else if (operator == MAREA_TOKEN_KIND_STAR) {
            symbol = "*";
        } else if (operator == MAREA_TOKEN_KIND_SLASH) {
            symbol = "/";
        } else if (operator != MAREA_TOKEN_KIND_PLUS) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A float does not support this operator.");
        }

        MAREA_CgenWrite(generator->buffer, scratch.data, scratch.length);
        MAREA_CgenWriteText(generator->buffer, symbol);
        MAREA_CgenWrite(generator->buffer, scratchRight.data, scratchRight.length);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((operator == MAREA_TOKEN_KIND_AMPERSAND) || (operator == MAREA_TOKEN_KIND_PIPE) || (operator == MAREA_TOKEN_KIND_CARET)) {
        const MAREA_Character8 *symbol = (operator == MAREA_TOKEN_KIND_AMPERSAND) ? "&" : ((operator == MAREA_TOKEN_KIND_PIPE) ? "|" : "^");

        MAREA_CgenWrite(generator->buffer, scratch.data, scratch.length);
        MAREA_CgenWriteText(generator->buffer, symbol);
        MAREA_CgenWrite(generator->buffer, scratchRight.data, scratchRight.length);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean NEEDS_TRAP = ((operator == MAREA_TOKEN_KIND_SLASH) || (operator == MAREA_TOKEN_KIND_PERCENT) || (operator == MAREA_TOKEN_KIND_SHIFT_LEFT) || (operator == MAREA_TOKEN_KIND_SHIFT_RIGHT));

    const MAREA_Character8 *name = "add";

    if (operator == MAREA_TOKEN_KIND_MINUS) {
        name = "sub";
    } else if (operator == MAREA_TOKEN_KIND_STAR) {
        name = "mul";
    } else if (operator == MAREA_TOKEN_KIND_SLASH) {
        name = "div";
    } else if (operator == MAREA_TOKEN_KIND_PERCENT) {
        name = "rem";
    } else if (operator == MAREA_TOKEN_KIND_SHIFT_LEFT) {
        name = "shl";
    } else if (operator == MAREA_TOKEN_KIND_SHIFT_RIGHT) {
        name = "shr";
    } else if (operator != MAREA_TOKEN_KIND_PLUS) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An arithmetic operator is not lowered.");
    }

    MAREA_IGNORE(SIGNED);

    MAREA_CgenWriteText(generator->buffer, "marea_rt_");
    MAREA_CgenWriteText(generator->buffer, name);
    MAREA_CgenWriteText(generator->buffer, "_");
    MAREA_CgenWriteText(generator->buffer, MAREA_CgenKindSuffix(KIND));
    MAREA_CgenWriteText(generator->buffer, "(");
    MAREA_CgenWrite(generator->buffer, scratch.data, scratch.length);
    MAREA_CgenWriteText(generator->buffer, ",");
    MAREA_CgenWrite(generator->buffer, scratchRight.data, scratchRight.length);

    if (NEEDS_TRAP == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(generator->buffer, ",marea_trap");
    }

    MAREA_CgenWriteText(generator->buffer, ")");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsComparison(MAREA_TokenKind token)
{
    return ((token == MAREA_TOKEN_KIND_EQUAL_EQUAL) || (token == MAREA_TOKEN_KIND_BANG_EQUAL) || (token == MAREA_TOKEN_KIND_LESS) || (token == MAREA_TOKEN_KIND_LESS_EQUAL) || (token == MAREA_TOKEN_KIND_GREATER) || (token == MAREA_TOKEN_KIND_GREATER_EQUAL));
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteComparison(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_TokenKind operator, const MAREA_IrNode *left, const MAREA_IrNode *right, MAREA_Uinteger8 *outKind)
{
    MAREA_IGNORE(node);

    *outKind = MAREA_CGEN_KIND_BOOLEAN;

    const MAREA_Character8 *symbol = "==";

    if (operator == MAREA_TOKEN_KIND_BANG_EQUAL) {
        symbol = "!=";
    } else if (operator == MAREA_TOKEN_KIND_LESS) {
        symbol = "<";
    } else if (operator == MAREA_TOKEN_KIND_LESS_EQUAL) {
        symbol = "<=";
    } else if (operator == MAREA_TOKEN_KIND_GREATER) {
        symbol = ">";
    } else if (operator == MAREA_TOKEN_KIND_GREATER_EQUAL) {
        symbol = ">=";
    }

    MAREA_Uinteger8 leftKind = MAREA_CGEN_KIND_NONE;
    MAREA_Uinteger8 rightKind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, left, &leftKind));
    MAREA_CgenWriteText(generator->buffer, symbol);
    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, right, &rightKind));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteLogical(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean isAnd, const MAREA_IrNode *left, const MAREA_IrNode *right, MAREA_Uinteger8 *outKind)
{
    MAREA_IGNORE(node);

    *outKind = MAREA_CGEN_KIND_BOOLEAN;

    MAREA_Uinteger8 leftKind = MAREA_CGEN_KIND_NONE;
    MAREA_Uinteger8 rightKind = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, left, &leftKind));
    MAREA_CgenWriteText(generator->buffer, (isAnd == MAREA_BOOLEAN_TRUE) ? "&&" : "||");
    MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, right, &rightKind));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteBinary(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    if (MAREA_IrNodeGetChildCount(node) != MAREA_INTEGER_SIZE(2)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A binary expression does not have two operands.");
    }

    const MAREA_IrNode *left = MAREA_NULLPTR;
    const MAREA_IrNode *right = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &left));
    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(1), &right));

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_AND) {
        return MAREA_G_WriteLogical(generator, node, MAREA_BOOLEAN_TRUE, left, right, outKind);
    }

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_OR) {
        return MAREA_G_WriteLogical(generator, node, MAREA_BOOLEAN_FALSE, left, right, outKind);
    }

    if (MAREA_G_IsComparison(OPERATOR) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_WriteComparison(generator, node, OPERATOR, left, right, outKind);
    }

    return MAREA_G_WriteArithmetic(generator, node, OPERATOR, left, right, outKind);
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteUnary(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);
    const MAREA_IrNode *operand = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &operand));

    if (operand == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A unary expression has no operand.");
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_COPY) || (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_MOVE)) {
        return MAREA_CgenWriteExpression(generator, operand, outKind);
    }

    if (OPERATOR == MAREA_TOKEN_KIND_MINUS) {
        if (MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_INTEGER_LITERAL) {
            return MAREA_G_WriteIntegerLiteral(generator, operand, MAREA_BOOLEAN_TRUE, outKind);
        }

        if (MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_FLOAT_LITERAL) {
            return MAREA_G_WriteFloatLiteral(generator, operand, MAREA_BOOLEAN_TRUE, outKind);
        }

        MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

        MAREA_CgenBuffer scratch = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };
        MAREA_CgenBuffer *savedBuffer = generator->buffer;

        generator->buffer = &scratch;
        const MAREA_CgenExcept EXCEPT = MAREA_CgenWriteExpression(generator, operand, &kind);
        generator->buffer = savedBuffer;

        MAREA_CGEN_TRY(EXCEPT);

        if ((generator->functionFailed == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsNumberKind(kind) == MAREA_BOOLEAN_FALSE)) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A negation needs a number.");
        }

        *outKind = kind;

        if (MAREA_G_IsFloatKind(kind) == MAREA_BOOLEAN_TRUE) {
            MAREA_CgenWriteText(generator->buffer, "-");
            MAREA_CgenWrite(generator->buffer, scratch.data, scratch.length);

            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        MAREA_CgenWriteText(generator->buffer, "marea_rt_neg_");
        MAREA_CgenWriteText(generator->buffer, MAREA_CgenKindSuffix(kind));
        MAREA_CgenWriteText(generator->buffer, "(");
        MAREA_CgenWrite(generator->buffer, scratch.data, scratch.length);
        MAREA_CgenWriteText(generator->buffer, ")");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_NOT) {
        MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

        MAREA_CgenWriteText(generator->buffer, "!");
        MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, operand, &kind));

        *outKind = MAREA_CGEN_KIND_BOOLEAN;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (OPERATOR == MAREA_TOKEN_KIND_TILDE) {
        MAREA_Uinteger8 kind = MAREA_CGEN_KIND_NONE;

        MAREA_CgenBuffer scratch = { .arena = generator->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };
        MAREA_CgenBuffer *savedBuffer = generator->buffer;

        generator->buffer = &scratch;
        const MAREA_CgenExcept EXCEPT = MAREA_CgenWriteExpression(generator, operand, &kind);
        generator->buffer = savedBuffer;

        MAREA_CGEN_TRY(EXCEPT);

        if ((generator->functionFailed == MAREA_BOOLEAN_TRUE) || (MAREA_G_IsNumberKind(kind) == MAREA_BOOLEAN_FALSE) || (MAREA_G_IsFloatKind(kind) == MAREA_BOOLEAN_TRUE)) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A bit-not needs an integer.");
        }

        *outKind = kind;

        MAREA_CgenWriteText(generator->buffer, "marea_rt_not_");
        MAREA_CgenWriteText(generator->buffer, MAREA_CgenKindSuffix(kind));
        MAREA_CgenWriteText(generator->buffer, "(");
        MAREA_CgenWrite(generator->buffer, scratch.data, scratch.length);
        MAREA_CgenWriteText(generator->buffer, ")");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A unary operator is not lowered.");
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *callee = MAREA_NULLPTR;
    const MAREA_IrNode *arguments = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(0), &callee));
    MAREA_IGNORE(MAREA_IrNodeGetChildAt(node, MAREA_INTEGER_SIZE(1), &arguments));

    *outKind = MAREA_CGEN_KIND_VOID;

    if ((callee == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(callee) != MAREA_IR_KIND_IDENTIFIER)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call is not a plain call of a function of this unit yet.");
    }

    const MAREA_CgenFunction *function = MAREA_CgenFindFunction(generator, MAREA_IrNodeGetText(callee), MAREA_IrNodeGetTextLength(callee));

    if (function == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A call does not resolve to a function of this unit.");
    }

    const MAREA_IntegerSize COUNT = (arguments == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(arguments);

    if ((MAREA_Uinteger32)COUNT != function->parameterCount) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call does not give one argument for every parameter.");
    }

    *outKind = function->resultKind;

    MAREA_CgenWriteMangled(generator->buffer, "f_", function->name, function->nameLength);
    MAREA_CgenWriteText(generator->buffer, "(");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (index > MAREA_INTEGER_SIZE(0)) {
            MAREA_CgenWriteText(generator->buffer, ",");
        }

        const MAREA_IrNode *argument = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(arguments, index, &argument));

        MAREA_Uinteger8 argumentKind = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenWriteExpression(generator, argument, &argumentKind));
    }

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_CgenWriteText(generator->buffer, ",");
    }

    MAREA_CgenWriteText(generator->buffer, "marea_trap)");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteExpression(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 *outKind)
{
    *outKind = MAREA_CGEN_KIND_NONE;

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (node == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An expression is missing.");
    }

    MAREA_CgenWriteText(generator->buffer, "(");

    MAREA_CgenExcept except = MAREA_CGEN_EXCEPT_SUCCESS;

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_INTEGER_LITERAL:
        except = MAREA_G_WriteIntegerLiteral(generator, node, MAREA_BOOLEAN_FALSE, outKind);
        break;
    case MAREA_IR_KIND_FLOAT_LITERAL:
        except = MAREA_G_WriteFloatLiteral(generator, node, MAREA_BOOLEAN_FALSE, outKind);
        break;
    case MAREA_IR_KIND_BOOLEAN_LITERAL:
        *outKind = MAREA_CGEN_KIND_BOOLEAN;

        MAREA_CgenWriteText(generator->buffer, (MAREA_G_TextIs(node, "true") == MAREA_BOOLEAN_TRUE) ? "true" : "false");
        break;
    case MAREA_IR_KIND_IDENTIFIER:
        except = MAREA_G_WriteIdentifier(generator, node, outKind);
        break;
    case MAREA_IR_KIND_UNARY_EXPRESSION:
        except = MAREA_G_WriteUnary(generator, node, outKind);
        break;
    case MAREA_IR_KIND_BINARY_EXPRESSION:
        except = MAREA_G_WriteBinary(generator, node, outKind);
        break;
    case MAREA_IR_KIND_CALL_EXPRESSION:
        except = MAREA_G_WriteCall(generator, node, outKind);
        break;
    default:
        except = MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An expression is not generated yet.");
        break;
    }

    MAREA_CgenWriteText(generator->buffer, ")");

    return except;
}
