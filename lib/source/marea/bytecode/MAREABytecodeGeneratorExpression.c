// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/bytecode/MAREABytecodeGeneratorInternal.h"

#include <string.h>

static MAREA_Boolean MAREA_CALL MAREA_G_IsNumber(MAREA_Uinteger8 kind)
{
    return (kind < MAREA_BYTECODE_NUMBER_KIND_COUNT);
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsFloat(MAREA_Uinteger8 kind)
{
    return ((kind == MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) || (kind == MAREA_BYTECODE_NUMBER_KIND_FLOAT_64));
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsSigned(MAREA_Uinteger8 kind)
{
    return (kind < MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8);
}

// INFO: a register holds a narrow integer sign extended or zero extended, so this is the form of a constant.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_Normalize(MAREA_Uinteger8 kind, MAREA_Uinteger64 bits)
{
    switch (kind) {
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_8:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger8)bits;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_16:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger16)bits;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger32)bits;
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8:
        return (bits & MAREA_UINTEGER_64(0xFF));
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_16:
        return (bits & MAREA_UINTEGER_64(0xFFFF));
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_32:
        return (bits & MAREA_UINTEGER_64(0xFFFFFFFF));
    default:
        return bits;
    }
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_FloatBits(MAREA_Uinteger8 kind, MAREA_Float64 value)
{
    MAREA_Float64 stored = value;

    if (kind == MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) {
        stored = (MAREA_Float64)((MAREA_Float32)value);
    }

    MAREA_Uinteger64 bits = MAREA_UINTEGER_64(0);

    memcpy(&bits, &stored, sizeof(bits));

    return bits;
}

static MAREA_Uinteger8 MAREA_CALL MAREA_G_LiteralKind(const MAREA_IrNode *node, MAREA_Uinteger8 hint, MAREA_Uinteger8 fallback)
{
    const MAREA_Uinteger8 OWN = MAREA_BytecodeGeneratorKindOfNode(node);

    if (MAREA_G_IsNumber(OWN)) {
        return OWN;
    }

    if (MAREA_G_IsNumber(hint)) {
        return hint;
    }

    return fallback;
}

static MAREA_Boolean MAREA_CALL MAREA_G_TextIs(const MAREA_IrNode *node, const MAREA_Character8 *expected)
{
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(node);

    return ((strlen(expected) == LENGTH) && (memcmp(MAREA_IrNodeGetText(node), expected, LENGTH) == 0));
}

static const MAREA_IrNode *MAREA_CALL MAREA_G_FindConstant(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length)
{
    MAREA_IntegerSize skip = MAREA_INTEGER_SIZE(0);

    if ((generator->namespaceLength > MAREA_INTEGER_SIZE(0)) && (length > (generator->namespaceLength + MAREA_INTEGER_SIZE(1))) && (memcmp(text, generator->namespaceText, generator->namespaceLength) == 0) && (text[generator->namespaceLength] == '.')) {
        skip = (generator->namespaceLength + MAREA_INTEGER_SIZE(1));
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->constantCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_IrNode *constant = generator->constants[index];

        if ((MAREA_IrNodeGetTextLength(constant) == (length - skip)) && (memcmp(MAREA_IrNodeGetText(constant), &text[skip], (length - skip)) == 0)) {
            return constant;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerIdentifier(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_Character8 *text = MAREA_IrNodeGetText(node);
    const MAREA_IntegerSize length = MAREA_IrNodeGetTextLength(node);

    const MAREA_BytecodeGeneratorSymbol *symbol = MAREA_BytecodeGeneratorFindSymbol(generator, text, length);

    if (symbol != MAREA_NULLPTR) {
        if (symbol->kind == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "The part of a partition is not a value yet.");
        }

        *outKind = symbol->kind;

        if (symbol->registerIndex == destination) {
            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, destination, symbol->registerIndex, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    const MAREA_IrNode *constant = MAREA_G_FindConstant(generator, text, length);

    if (constant == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A name is not a local, a parameter or a constant of the module.");
    }

    const MAREA_IrNode *value = MAREA_BytecodeGeneratorChildAt(constant, MAREA_INTEGER_SIZE(0));
    const MAREA_Uinteger8 KIND = MAREA_BytecodeGeneratorKindOfNode(constant);

    if (KIND == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A constant has a type that is not a scalar.");
    }

    if ((value == MAREA_NULLPTR) || (generator->constantDepth >= MAREA_BYTECODE_GENERATOR_CONSTANT_DEPTH)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A constant has no value, or reads constants too deep.");
    }

    generator->constantDepth += MAREA_UINTEGER_32(1);

    const MAREA_BytecodeExcept EXCEPT = MAREA_BytecodeGeneratorLowerInto(generator, value, destination, KIND, outKind);

    generator->constantDepth -= MAREA_UINTEGER_32(1);

    return EXCEPT;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerIntegerLiteral(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Boolean negate, MAREA_Uinteger8 *outKind)
{
    MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

    if (!MAREA_BytecodeGeneratorParseInteger(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), &value)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An integer literal does not fit 64 bits.");
    }

    const MAREA_Uinteger8 KIND = MAREA_G_LiteralKind(node, hint, MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32);

    *outKind = KIND;

    if (MAREA_G_IsFloat(KIND)) {
        const MAREA_Float64 NUMBER = (MAREA_Float64)value;

        return MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, MAREA_G_FloatBits(KIND, (negate ? -NUMBER : NUMBER)));
    }

    return MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, MAREA_G_Normalize(KIND, (negate ? (MAREA_UINTEGER_64(0) - value) : value)));
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerFloatLiteral(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Boolean negate, MAREA_Uinteger8 *outKind)
{
    MAREA_Float64 number = MAREA_FLOAT_64(0.0);

    if (!MAREA_BytecodeGeneratorParseFloat(MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node), &number)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A float literal cannot be read.");
    }

    MAREA_Uinteger8 kind = MAREA_G_LiteralKind(node, hint, MAREA_BYTECODE_NUMBER_KIND_FLOAT_64);

    if (!MAREA_G_IsFloat(kind)) {
        kind = MAREA_BYTECODE_NUMBER_KIND_FLOAT_64;
    }

    *outKind = kind;

    return MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, MAREA_G_FloatBits(kind, (negate ? -number : number)));
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerUnary(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Uinteger8 *outKind)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);
    const MAREA_IrNode *operand = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    if (operand == MAREA_NULLPTR) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A unary expression has no operand.");
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_COPY) || (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_MOVE)) {
        return MAREA_BytecodeGeneratorLowerInto(generator, operand, destination, hint, outKind);
    }

    if (OPERATOR == MAREA_TOKEN_KIND_MINUS) {
        const MAREA_Uinteger8 OWN = MAREA_BytecodeGeneratorKindOfNode(node);
        const MAREA_Uinteger8 USED = (MAREA_G_IsNumber(OWN) ? OWN : hint);

        if (MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_INTEGER_LITERAL) {
            return MAREA_G_LowerIntegerLiteral(generator, operand, destination, USED, MAREA_BOOLEAN_TRUE, outKind);
        }

        if (MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_FLOAT_LITERAL) {
            return MAREA_G_LowerFloatLiteral(generator, operand, destination, USED, MAREA_BOOLEAN_TRUE, outKind);
        }
    }

    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, operand, destination, hint, &kind));

    *outKind = kind;

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_NOT) {
        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_NOT, destination, destination, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
    }

    if ((OPERATOR != MAREA_TOKEN_KIND_MINUS) && (OPERATOR != MAREA_TOKEN_KIND_TILDE)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A unary operator is not lowered.");
    }

    if (!MAREA_G_IsNumber(kind)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A unary operator needs a number.");
    }

    return MAREA_BytecodeGeneratorEmit(generator, ((OPERATOR == MAREA_TOKEN_KIND_MINUS) ? MAREA_BYTECODE_OPCODE_NEGATE : MAREA_BYTECODE_OPCODE_BIT_NOT), destination, destination, kind, MAREA_NULLPTR);
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerLogical(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Boolean isAnd)
{
    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)), destination, MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN, &kind));

    MAREA_Uinteger32 jump = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, (isAnd ? MAREA_BYTECODE_OPCODE_JUMP_IF_NOT : MAREA_BYTECODE_OPCODE_JUMP_IF), destination, MAREA_UINTEGER_32(0), MAREA_UINTEGER_32(0), &jump));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), destination, MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN, &kind));

    return MAREA_BytecodeGeneratorPatchJump(generator, node, jump, (MAREA_Uinteger32)generator->codeCount);
}

static MAREA_Boolean MAREA_CALL MAREA_G_ComparisonOpcode(MAREA_TokenKind token, MAREA_Uinteger8 kind, MAREA_Uinteger16 *outOpcode, MAREA_Boolean *outSwap)
{
    const MAREA_Boolean FLOAT = MAREA_G_IsFloat(kind);
    const MAREA_Boolean SIGNED = MAREA_G_IsSigned(kind);

    // INFO: a greater test is a less test with the two sides swapped, which is also right for a float with NaN.
    *outSwap = ((token == MAREA_TOKEN_KIND_GREATER) || (token == MAREA_TOKEN_KIND_GREATER_EQUAL));

    if (token == MAREA_TOKEN_KIND_EQUAL_EQUAL) {
        *outOpcode = (FLOAT ? MAREA_BYTECODE_OPCODE_EQUAL_FLOAT : MAREA_BYTECODE_OPCODE_EQUAL);

        return MAREA_BOOLEAN_TRUE;
    }

    if (token == MAREA_TOKEN_KIND_BANG_EQUAL) {
        *outOpcode = (FLOAT ? MAREA_BYTECODE_OPCODE_NOT_EQUAL_FLOAT : MAREA_BYTECODE_OPCODE_NOT_EQUAL);

        return MAREA_BOOLEAN_TRUE;
    }

    if ((token == MAREA_TOKEN_KIND_LESS) || (token == MAREA_TOKEN_KIND_GREATER)) {
        *outOpcode = (FLOAT ? MAREA_BYTECODE_OPCODE_LESS_FLOAT : (SIGNED ? MAREA_BYTECODE_OPCODE_LESS_SIGNED : MAREA_BYTECODE_OPCODE_LESS_UNSIGNED));

        return MAREA_BOOLEAN_TRUE;
    }

    if ((token == MAREA_TOKEN_KIND_LESS_EQUAL) || (token == MAREA_TOKEN_KIND_GREATER_EQUAL)) {
        *outOpcode = (FLOAT ? MAREA_BYTECODE_OPCODE_LESS_EQUAL_FLOAT : (SIGNED ? MAREA_BYTECODE_OPCODE_LESS_EQUAL_SIGNED : MAREA_BYTECODE_OPCODE_LESS_EQUAL_UNSIGNED));

        return MAREA_BOOLEAN_TRUE;
    }

    return MAREA_BOOLEAN_FALSE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsComparison(MAREA_TokenKind token)
{
    return ((token == MAREA_TOKEN_KIND_EQUAL_EQUAL) || (token == MAREA_TOKEN_KIND_BANG_EQUAL) || (token == MAREA_TOKEN_KIND_LESS) || (token == MAREA_TOKEN_KIND_LESS_EQUAL) || (token == MAREA_TOKEN_KIND_GREATER) || (token == MAREA_TOKEN_KIND_GREATER_EQUAL));
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerComparison(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger32 left = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 right = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 leftKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger8 rightKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)), MAREA_BYTECODE_GENERATOR_KIND_NONE, &left, &leftKind));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), leftKind, &right, &rightKind));

    *outKind = MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN;

    if (leftKind >= MAREA_BYTECODE_GENERATOR_KIND_VOID) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A comparison needs two scalar values.");
    }

    MAREA_Uinteger16 opcode = MAREA_BYTECODE_OPCODE_NOP;
    MAREA_Boolean swap = MAREA_BOOLEAN_FALSE;

    if (!MAREA_G_ComparisonOpcode(MAREA_IrNodeGetOperator(node), leftKind, &opcode, &swap)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A comparison operator is not lowered.");
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, opcode, destination, (swap ? right : left), (swap ? left : right), MAREA_NULLPTR));

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerBinary(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Uinteger8 *outKind)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    if (MAREA_IrNodeGetChildCount(node) != MAREA_INTEGER_SIZE(2)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A binary expression does not have two operands.");
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_AND) || (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_OR)) {
        *outKind = MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN;

        return MAREA_G_LowerLogical(generator, node, destination, (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_AND));
    }

    if (MAREA_G_IsComparison(OPERATOR)) {
        return MAREA_G_LowerComparison(generator, node, destination, outKind);
    }

    MAREA_Uinteger16 opcode = MAREA_BYTECODE_OPCODE_NOP;
    MAREA_Boolean threeRegisters = MAREA_BOOLEAN_FALSE;

    if (!MAREA_BytecodeGeneratorOperatorOpcode(OPERATOR, &opcode, &threeRegisters)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A binary operator is not lowered.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;
    const MAREA_Boolean SHIFT = ((opcode == MAREA_BYTECODE_OPCODE_SHIFT_LEFT) || (opcode == MAREA_BYTECODE_OPCODE_SHIFT_RIGHT));

    MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0)), destination, hint, &kind));

    MAREA_Uinteger32 right = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 rightKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1)), (SHIFT ? MAREA_BYTECODE_GENERATOR_KIND_NONE : kind), &right, &rightKind));

    *outKind = kind;

    if (!MAREA_G_IsNumber(kind)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A binary operator needs numbers.");
    }

    if (threeRegisters) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, opcode, destination, destination, right, MAREA_NULLPTR));
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, opcode, destination, right, kind, MAREA_NULLPTR));
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerConvert(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *callee = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *target = MAREA_BytecodeGeneratorChildAt(callee, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));
    const MAREA_IrNode *argument = MAREA_BytecodeGeneratorChildAt(arguments, MAREA_INTEGER_SIZE(0));

    if ((target == MAREA_NULLPTR) || (argument == MAREA_NULLPTR) || (MAREA_IrNodeGetChildCount(arguments) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A convert has not one type and one value.");
    }

    const MAREA_Uinteger8 TARGET = MAREA_BytecodeGeneratorKindFromText(MAREA_IrNodeGetText(target), MAREA_IrNodeGetTextLength(target));

    if (!MAREA_G_IsNumber(TARGET)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A convert target is not a number type.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger32 source = MAREA_UINTEGER_32(0);
    MAREA_Uinteger8 sourceKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerOperand(generator, argument, MAREA_BYTECODE_GENERATOR_KIND_NONE, &source, &sourceKind));

    *outKind = TARGET;

    if (!MAREA_G_IsNumber(sourceKind)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A convert source is not a number type.");
    }

    if (sourceKind == TARGET) {
        if (source != destination) {
            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, destination, source, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
        }
    } else {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_CONVERT, destination, source, ((MAREA_Uinteger32)sourceKind | ((MAREA_Uinteger32)TARGET << MAREA_UINTEGER_32(8))), MAREA_NULLPTR));
    }

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

// INFO: a function of this unit is known before any body is lowered (MAREA_G_Collect), and a plain call always
// reads its callee by the bare name sema left it, with the namespace of this unit skipped when it is written.
static const MAREA_BytecodeGeneratorFunction *MAREA_CALL MAREA_G_FindFunction(const MAREA_BytecodeGenerator *generator, const MAREA_Character8 *text, MAREA_IntegerSize length, MAREA_Uinteger32 *outIndex)
{
    MAREA_IntegerSize skip = MAREA_INTEGER_SIZE(0);

    if ((generator->namespaceLength > MAREA_INTEGER_SIZE(0)) && (length > (generator->namespaceLength + MAREA_INTEGER_SIZE(1))) && (memcmp(text, generator->namespaceText, generator->namespaceLength) == 0) && (text[generator->namespaceLength] == '.')) {
        skip = (generator->namespaceLength + MAREA_INTEGER_SIZE(1));
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator->functionCount; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_BytecodeGeneratorFunction *entry = &generator->functions[index];
        const MAREA_IntegerSize declaredLength = MAREA_IrNodeGetTextLength(entry->node);

        if ((declaredLength == (length - skip)) && (memcmp(MAREA_IrNodeGetText(entry->node), &text[skip], (length - skip)) == 0)) {
            *outIndex = (MAREA_Uinteger32)index;

            return entry;
        }
    }

    return MAREA_NULLPTR;
}

// INFO: the call window is 1 + parameterCount registers: the result lands in the first, the arguments fill the
// rest in order, and both ends free once the call and the move out of the window are emitted.
static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerPlainCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_BytecodeGeneratorFunction *callee, MAREA_Uinteger32 functionIndex, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(arguments);

    if ((MAREA_Uinteger32)COUNT != callee->parameterCount) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call does not give one argument for every parameter.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;
    MAREA_Uinteger32 window = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, (callee->parameterCount + MAREA_UINTEGER_32(1)), &window));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
        const MAREA_Uinteger8 PARAMETER_KIND = callee->parameterKinds[index];

        if (PARAMETER_KIND == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A call argument has a type that is not a scalar.");
        }

        MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, MAREA_BytecodeGeneratorChildAt(arguments, index), (window + MAREA_UINTEGER_32(1) + (MAREA_Uinteger32)index), PARAMETER_KIND, &kind));
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_CALL, window, functionIndex, callee->parameterCount, MAREA_NULLPTR));

    if ((callee->resultKind != MAREA_BYTECODE_GENERATOR_KIND_VOID) && (window != destination)) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_MOVE, destination, window, MAREA_UINTEGER_32(0), MAREA_NULLPTR));
    }

    *outKind = callee->resultKind;

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerUnitCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, const MAREA_BytecodeGeneratorFunction *function, MAREA_Uinteger32 functionIndex, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    if (function->resultKind == MAREA_BYTECODE_GENERATOR_KIND_NONE) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A call result has a type that is not a scalar.");
    }

    return MAREA_G_LowerPlainCall(generator, node, function, functionIndex, destination, outKind);
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerCall(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Boolean valueUsed, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *callee = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    *outKind = MAREA_BYTECODE_GENERATOR_KIND_VOID;

    if ((callee != MAREA_NULLPTR) && (MAREA_IrNodeGetKind(callee) == MAREA_IR_KIND_BUILTIN_NAME) && MAREA_G_TextIs(callee, "convert")) {
        return MAREA_G_LowerConvert(generator, node, destination, outKind);
    }

    if ((callee != MAREA_NULLPTR) && (MAREA_IrNodeGetKind(callee) == MAREA_IR_KIND_IDENTIFIER)) {
        MAREA_Uinteger32 functionIndex = MAREA_UINTEGER_32(0);
        const MAREA_BytecodeGeneratorFunction *function = MAREA_G_FindFunction(generator, MAREA_IrNodeGetText(callee), MAREA_IrNodeGetTextLength(callee), &functionIndex);

        if (function != MAREA_NULLPTR) {
            return MAREA_G_LowerUnitCall(generator, node, function, functionIndex, destination, outKind);
        }

        const MAREA_Character8 *text = MAREA_IrNodeGetText(callee);
        const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(callee);

        // INFO: an imported member reaches here as its full dotted path, which no local name can be.
        if ((MAREA_BytecodeGeneratorFindSymbol(generator, text, LENGTH) == MAREA_NULLPTR) && (memchr(text, '.', LENGTH) != MAREA_NULLPTR)) {
            return MAREA_BytecodeGeneratorLowerHostCall(generator, node, text, LENGTH, destination, valueUsed, outKind);
        }

        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A call does not resolve to a function of this unit.");
    }

    if ((callee != MAREA_NULLPTR) && (MAREA_IrNodeGetKind(callee) == MAREA_IR_KIND_MEMBER_ACCESS)) {
        const MAREA_Character8 *name = MAREA_NULLPTR;
        MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorHostName(generator, callee, &name, &length));

        if (name != MAREA_NULLPTR) {
            MAREA_Uinteger32 functionIndex = MAREA_UINTEGER_32(0);
            const MAREA_BytecodeGeneratorFunction *function = MAREA_G_FindFunction(generator, name, length, &functionIndex);

            if (function != MAREA_NULLPTR) {
                return MAREA_G_LowerUnitCall(generator, node, function, functionIndex, destination, outKind);
            }

            return MAREA_BytecodeGeneratorLowerHostCall(generator, node, name, length, destination, valueUsed, outKind);
        }
    }

    return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call is not lowered.");
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorAggregateAddress(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 *outRegister, MAREA_Uinteger32 *outAggregateIndex)
{
    *outRegister = MAREA_UINTEGER_32(0);
    *outAggregateIndex = MAREA_BYTECODE_NONE;

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_IDENTIFIER) {
        const MAREA_BytecodeGeneratorSymbol *symbol = MAREA_BytecodeGeneratorFindSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

        if ((symbol != MAREA_NULLPTR) && (symbol->kind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE)) {
            *outRegister = symbol->registerIndex;
            *outAggregateIndex = symbol->aggregateType;

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }

        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A name is not a structure local or a structure parameter.");
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS) {
        const MAREA_IrNode *base = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
        const MAREA_IrNode *member = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

        if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR)) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A member access has no base or no member.");
        }

        MAREA_Uinteger32 baseRegister = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 baseAggregate = MAREA_BYTECODE_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, base, &baseRegister, &baseAggregate));

        MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);
        const MAREA_IrNode *field = MAREA_NULLPTR;

        if (!MAREA_BytecodeGeneratorAggregateField(generator, baseAggregate, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member), &offset, &field)) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A structure has no field of this name.");
        }

        MAREA_Uinteger8 fieldKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
        MAREA_Uinteger32 fieldAggregate = MAREA_BYTECODE_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, field, &fieldKind, &fieldAggregate));

        if (fieldKind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A structure address needs a structure field.");
        }

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_FIELD, *outRegister, baseRegister, (MAREA_Uinteger32)offset, MAREA_NULLPTR));

        *outAggregateIndex = fieldAggregate;

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A structure address is not lowered from this expression yet.");
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorConstructAggregate(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destinationAddress, MAREA_Uinteger32 aggregateIndex)
{
    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    const MAREA_BytecodeGeneratorAggregate *aggregate = &generator->aggregates[aggregateIndex];

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_LITERAL_VALUE) {
        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, destinationAddress, (MAREA_Uinteger32)aggregate->size, MAREA_UINTEGER_32(0), MAREA_NULLPTR));

        const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);
        const MAREA_Uinteger32 MARK = generator->nextRegister;

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index += MAREA_INTEGER_SIZE(1)) {
            const MAREA_IrNode *entry = MAREA_BytecodeGeneratorChildAt(node, index);
            const MAREA_IrNode *fieldValue = MAREA_BytecodeGeneratorChildAt(entry, MAREA_INTEGER_SIZE(0));

            if ((entry == MAREA_NULLPTR) || (fieldValue == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(entry) != MAREA_IR_KIND_FIELD_ENTRY)) {
                return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A structure literal holds something that is not a named field.");
            }

            MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);
            const MAREA_IrNode *field = MAREA_NULLPTR;

            if (!MAREA_BytecodeGeneratorAggregateField(generator, aggregateIndex, MAREA_IrNodeGetText(entry), MAREA_IrNodeGetTextLength(entry), &offset, &field)) {
                return MAREA_BytecodeGeneratorFail(generator, entry, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A structure has no field of this name.");
            }

            MAREA_Uinteger8 fieldKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
            MAREA_Uinteger32 fieldAggregate = MAREA_BYTECODE_NONE;

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, field, &fieldKind, &fieldAggregate));

            MAREA_Uinteger32 fieldAddress = MAREA_UINTEGER_32(0);

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, entry, MAREA_UINTEGER_32(1), &fieldAddress));

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_FIELD, fieldAddress, destinationAddress, (MAREA_Uinteger32)offset, MAREA_NULLPTR));

            if (fieldKind == MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) {
                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorConstructAggregate(generator, fieldValue, fieldAddress, fieldAggregate));
            } else if (fieldKind < MAREA_BYTECODE_GENERATOR_KIND_VOID) {
                MAREA_Uinteger32 valueRegister = MAREA_UINTEGER_32(0);

                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, entry, MAREA_UINTEGER_32(1), &valueRegister));

                MAREA_Uinteger8 gotKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, fieldValue, valueRegister, fieldKind, &gotKind));

                const MAREA_Uinteger8 WIDTH_KIND = ((fieldKind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 : fieldKind);

                MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_STORE, fieldAddress, valueRegister, WIDTH_KIND, MAREA_NULLPTR));
            } else {
                return MAREA_BytecodeGeneratorFail(generator, field, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A field has a type that is not lowered yet.");
            }

            generator->nextRegister = MARK;
        }

        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_UNARY_EXPRESSION) {
        const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

        if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_MOVE) || (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_COPY)) {
            const MAREA_IrNode *operand = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

            if (operand == MAREA_NULLPTR) {
                return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A copy or a move has no operand.");
            }

            MAREA_Uinteger32 sourceRegister = MAREA_UINTEGER_32(0);
            MAREA_Uinteger32 sourceAggregate = MAREA_BYTECODE_NONE;

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, operand, &sourceRegister, &sourceAggregate));

            MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, destinationAddress, sourceRegister, (MAREA_Uinteger32)aggregate->size, MAREA_NULLPTR));

            if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_MOVE) {
                return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_ZERO, sourceRegister, (MAREA_Uinteger32)aggregate->size, MAREA_UINTEGER_32(0), MAREA_NULLPTR);
            }

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    if ((MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_IDENTIFIER) || (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS)) {
        MAREA_Uinteger32 sourceRegister = MAREA_UINTEGER_32(0);
        MAREA_Uinteger32 sourceAggregate = MAREA_BYTECODE_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, node, &sourceRegister, &sourceAggregate));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, destinationAddress, sourceRegister, (MAREA_Uinteger32)aggregate->size, MAREA_NULLPTR);
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_CALL_EXPRESSION) {
        const MAREA_Uinteger64 SIZE = aggregate->size;

        MAREA_Uinteger8 kind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
        MAREA_Uinteger32 resultAggregate = MAREA_BYTECODE_NONE;

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, node, &kind, &resultAggregate));

        // INFO: a call with no type is left to the lowering of the call, which names that fault.
        if ((MAREA_IrNodeGetResolvedType(node) != MAREA_NULLPTR) && ((kind != MAREA_BYTECODE_GENERATOR_KIND_AGGREGATE) || (resultAggregate != aggregateIndex))) {
            return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A call gives another type than the structure that is built.");
        }

        MAREA_Uinteger32 sourceRegister = MAREA_UINTEGER_32(0);

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &sourceRegister));

        MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorLowerInto(generator, node, sourceRegister, MAREA_BYTECODE_GENERATOR_KIND_NONE, &kind));

        return MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_COPY_BYTES, destinationAddress, sourceRegister, (MAREA_Uinteger32)SIZE, MAREA_NULLPTR);
    }

    return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A structure value is not lowered from this expression yet.");
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerMemberAccess(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *base = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *member = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(1));

    if ((base == MAREA_NULLPTR) || (member == MAREA_NULLPTR)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A member access has no base or no member.");
    }

    const MAREA_Uinteger32 MARK = generator->nextRegister;

    MAREA_Uinteger32 baseRegister = MAREA_UINTEGER_32(0);
    MAREA_Uinteger32 baseAggregate = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateAddress(generator, base, &baseRegister, &baseAggregate));

    MAREA_Uinteger64 offset = MAREA_UINTEGER_64(0);
    const MAREA_IrNode *field = MAREA_NULLPTR;

    if (!MAREA_BytecodeGeneratorAggregateField(generator, baseAggregate, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member), &offset, &field)) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A structure has no field of this name.");
    }

    MAREA_Uinteger8 fieldKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;
    MAREA_Uinteger32 fieldAggregate = MAREA_BYTECODE_NONE;

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAggregateKind(generator, field, &fieldKind, &fieldAggregate));

    if (fieldKind >= MAREA_BYTECODE_GENERATOR_KIND_VOID) {
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A nested structure field is not read this way yet.");
    }

    *outKind = fieldKind;

    const MAREA_Uinteger8 WIDTH_KIND = ((fieldKind == MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN) ? MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8 : fieldKind);

    MAREA_Uinteger32 addressRegister = MAREA_UINTEGER_32(0);

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), &addressRegister));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_FIELD, addressRegister, baseRegister, (MAREA_Uinteger32)offset, MAREA_NULLPTR));

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorEmit(generator, MAREA_BYTECODE_OPCODE_LOAD, destination, addressRegister, WIDTH_KIND, MAREA_NULLPTR));

    generator->nextRegister = MARK;

    return MAREA_BYTECODE_EXCEPT_SUCCESS;
}

static MAREA_BytecodeExcept MAREA_CALL MAREA_G_LowerMacro(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Uinteger8 *outKind)
{
    const MAREA_IrNode *arguments = MAREA_BytecodeGeneratorChildAt(node, MAREA_INTEGER_SIZE(0));

    if (MAREA_G_TextIs(node, "build_value") && (arguments != MAREA_NULLPTR) && (MAREA_IrNodeGetChildCount(arguments) == MAREA_INTEGER_SIZE(3))) {
        // INFO: this stage has no build settings, so the value of build_value! is its default.
        return MAREA_BytecodeGeneratorLowerInto(generator, MAREA_BytecodeGeneratorChildAt(arguments, MAREA_INTEGER_SIZE(2)), destination, hint, outKind);
    }

    return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A macro of a library is not lowered.");
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerInto(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 destination, MAREA_Uinteger8 hint, MAREA_Uinteger8 *outKind)
{
    *outKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    switch (MAREA_IrNodeGetKind(node)) {
    case MAREA_IR_KIND_INTEGER_LITERAL:
        return MAREA_G_LowerIntegerLiteral(generator, node, destination, hint, MAREA_BOOLEAN_FALSE, outKind);
    case MAREA_IR_KIND_FLOAT_LITERAL:
        return MAREA_G_LowerFloatLiteral(generator, node, destination, hint, MAREA_BOOLEAN_FALSE, outKind);
    case MAREA_IR_KIND_BOOLEAN_LITERAL:
        *outKind = MAREA_BYTECODE_GENERATOR_KIND_BOOLEAN;

        return MAREA_BytecodeGeneratorEmitConstant(generator, node, destination, (MAREA_G_TextIs(node, "true") ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0)));
    case MAREA_IR_KIND_IDENTIFIER:
        return MAREA_G_LowerIdentifier(generator, node, destination, outKind);
    case MAREA_IR_KIND_UNARY_EXPRESSION:
        return MAREA_G_LowerUnary(generator, node, destination, hint, outKind);
    case MAREA_IR_KIND_BINARY_EXPRESSION:
        return MAREA_G_LowerBinary(generator, node, destination, hint, outKind);
    case MAREA_IR_KIND_CALL_EXPRESSION:
        return MAREA_BytecodeGeneratorLowerCall(generator, node, destination, MAREA_BOOLEAN_TRUE, outKind);
    case MAREA_IR_KIND_MEMBER_ACCESS:
        return MAREA_G_LowerMemberAccess(generator, node, destination, outKind);
    case MAREA_IR_KIND_MACRO_CALL:
        return MAREA_G_LowerMacro(generator, node, destination, hint, outKind);
    case MAREA_IR_KIND_STRING_LITERAL:
    case MAREA_IR_KIND_CHARACTER_LITERAL:
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A string and a character are not lowered yet.");
    default:
        return MAREA_BytecodeGeneratorFail(generator, node, MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An expression is not lowered yet.");
    }
}

MAREA_BytecodeExcept MAREA_CALL MAREA_BytecodeGeneratorLowerOperand(MAREA_BytecodeGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger8 hint, MAREA_Uinteger32 *outRegister, MAREA_Uinteger8 *outKind)
{
    *outRegister = MAREA_UINTEGER_32(0);
    *outKind = MAREA_BYTECODE_GENERATOR_KIND_NONE;

    if (generator->functionFailed) {
        return MAREA_BYTECODE_EXCEPT_SUCCESS;
    }

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_IDENTIFIER) {
        const MAREA_BytecodeGeneratorSymbol *symbol = MAREA_BytecodeGeneratorFindSymbol(generator, MAREA_IrNodeGetText(node), MAREA_IrNodeGetTextLength(node));

        if ((symbol != MAREA_NULLPTR) && (symbol->kind != MAREA_BYTECODE_GENERATOR_KIND_NONE)) {
            *outRegister = symbol->registerIndex;
            *outKind = symbol->kind;

            return MAREA_BYTECODE_EXCEPT_SUCCESS;
        }
    }

    MAREA_BYTECODE_TRY(MAREA_BytecodeGeneratorAllocate(generator, node, MAREA_UINTEGER_32(1), outRegister));

    return MAREA_BytecodeGeneratorLowerInto(generator, node, *outRegister, hint, outKind);
}
