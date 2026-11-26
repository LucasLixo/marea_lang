// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

typedef struct MAREA_G_Operator
{
    const MAREA_Character8 *helper;
    const MAREA_Character8 *floatSymbol;
    MAREA_TokenKind token;
    MAREA_TokenKind compound;
    MAREA_Boolean traps;
    MAREA_Uinteger8 padding[3];
} MAREA_G_Operator;

// INFO: a wrapping form is the plain operator, which wraps already. A float has no saturating arithmetic.
static const MAREA_G_Operator MAREA_G_OPERATORS[] = {
    { .helper = "add", .floatSymbol = "+", .token = MAREA_TOKEN_KIND_PLUS, .compound = MAREA_TOKEN_KIND_PLUS_EQUAL },
    { .helper = "add", .floatSymbol = "+", .token = MAREA_TOKEN_KIND_PLUS_PERCENT, .compound = MAREA_TOKEN_KIND_ERROR },
    { .helper = "sadd", .floatSymbol = "+", .token = MAREA_TOKEN_KIND_PLUS_PIPE, .compound = MAREA_TOKEN_KIND_ERROR },
    { .helper = "sub", .floatSymbol = "-", .token = MAREA_TOKEN_KIND_MINUS, .compound = MAREA_TOKEN_KIND_MINUS_EQUAL },
    { .helper = "sub", .floatSymbol = "-", .token = MAREA_TOKEN_KIND_MINUS_PERCENT, .compound = MAREA_TOKEN_KIND_ERROR },
    { .helper = "ssub", .floatSymbol = "-", .token = MAREA_TOKEN_KIND_MINUS_PIPE, .compound = MAREA_TOKEN_KIND_ERROR },
    { .helper = "mul", .floatSymbol = "*", .token = MAREA_TOKEN_KIND_STAR, .compound = MAREA_TOKEN_KIND_STAR_EQUAL },
    { .helper = "mul", .floatSymbol = "*", .token = MAREA_TOKEN_KIND_STAR_PERCENT, .compound = MAREA_TOKEN_KIND_ERROR },
    { .helper = "smul", .floatSymbol = "*", .token = MAREA_TOKEN_KIND_STAR_PIPE, .compound = MAREA_TOKEN_KIND_ERROR },
    { .helper = "div", .floatSymbol = "/", .token = MAREA_TOKEN_KIND_SLASH, .compound = MAREA_TOKEN_KIND_SLASH_EQUAL, .traps = MAREA_BOOLEAN_TRUE },
    { .helper = "rem", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_PERCENT, .compound = MAREA_TOKEN_KIND_PERCENT_EQUAL, .traps = MAREA_BOOLEAN_TRUE },
    { .helper = "pow", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_STAR_STAR, .compound = MAREA_TOKEN_KIND_STAR_STAR_EQUAL, .traps = MAREA_BOOLEAN_TRUE },
    { .helper = "shl", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_SHIFT_LEFT, .compound = MAREA_TOKEN_KIND_SHIFT_LEFT_EQUAL, .traps = MAREA_BOOLEAN_TRUE },
    { .helper = "shr", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_SHIFT_RIGHT, .compound = MAREA_TOKEN_KIND_SHIFT_RIGHT_EQUAL, .traps = MAREA_BOOLEAN_TRUE },
    { .helper = "and", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_AMPERSAND, .compound = MAREA_TOKEN_KIND_AMPERSAND_EQUAL },
    { .helper = "or", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_PIPE, .compound = MAREA_TOKEN_KIND_PIPE_EQUAL },
    { .helper = "xor", .floatSymbol = MAREA_NULLPTR, .token = MAREA_TOKEN_KIND_CARET, .compound = MAREA_TOKEN_KIND_CARET_EQUAL },
};

MAREA_TokenKind MAREA_CALL MAREA_CgenPlainOperator(MAREA_TokenKind compound)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < (MAREA_IntegerSize)MAREA_ARRAY_SIZE(MAREA_G_OPERATORS); index = (index + MAREA_INTEGER_SIZE(1))) {
        if ((compound != MAREA_TOKEN_KIND_ERROR) && (MAREA_G_OPERATORS[index].compound == compound)) {
            return MAREA_G_OPERATORS[index].token;
        }
    }

    return MAREA_TOKEN_KIND_ERROR;
}

static const MAREA_G_Operator *MAREA_CALL MAREA_G_FindOperator(MAREA_TokenKind token)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < (MAREA_IntegerSize)MAREA_ARRAY_SIZE(MAREA_G_OPERATORS); index = (index + MAREA_INTEGER_SIZE(1))) {
        if (MAREA_G_OPERATORS[index].token == token) {
            return &MAREA_G_OPERATORS[index];
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsShift(MAREA_TokenKind token)
{
    return ((token == MAREA_TOKEN_KIND_SHIFT_LEFT) || (token == MAREA_TOKEN_KIND_SHIFT_RIGHT));
}

// INFO: a float operation runs in its own width, and pow and fmod run in double, as in the vm.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteFloatOperator(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_G_Operator *row, const MAREA_CgenValue *left, const MAREA_CgenValue *right, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 TYPE = left->type;

    if (row->floatSymbol != MAREA_NULLPTR) {
        MAREA_CgenBind(generator, TYPE, MAREA_CGEN_JOIN(generator, "(", left->text, ") ", row->floatSymbol, " (", right->text, ")"), outValue);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((row->token != MAREA_TOKEN_KIND_PERCENT) && (row->token != MAREA_TOKEN_KIND_STAR_STAR)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A float has no bit operator.");
    }

    generator->usesMath = MAREA_BOOLEAN_TRUE;

    MAREA_CgenBind(generator, TYPE, MAREA_CGEN_JOIN(generator, "(", MAREA_CgenTypeName(generator, TYPE), ")", ((row->token == MAREA_TOKEN_KIND_PERCENT) ? "fmod" : "pow"), "((double)(", left->text, "), (double)(", right->text, "))"), outValue);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteOperator(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_TokenKind operatorKind, const MAREA_CgenValue *left, const MAREA_CgenValue *right, MAREA_CgenValue *outValue)
{
    const MAREA_G_Operator *row = MAREA_G_FindOperator(operatorKind);

    if (row == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "An operator is not written.");
    }

    const MAREA_Uinteger32 TYPE = left->type;
    const MAREA_Boolean SHIFT = MAREA_G_IsShift(operatorKind);
    const MAREA_Boolean TYPED = (SHIFT == MAREA_BOOLEAN_TRUE) ? ((MAREA_CgenIsInteger(TYPE) == MAREA_BOOLEAN_TRUE) && (MAREA_CgenIsInteger(right->type) == MAREA_BOOLEAN_TRUE)) : ((MAREA_CgenIsNumber(TYPE) == MAREA_BOOLEAN_TRUE) && (right->type == TYPE));

    if (TYPED == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An operator needs two numbers of one type.");
    }

    if (MAREA_CgenIsFloat(TYPE) == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_WriteFloatOperator(generator, node, row, left, right, outValue);
    }

    const MAREA_Character8 *helper = MAREA_CGEN_JOIN(generator, "marea_rt_", row->helper, "_", MAREA_CgenKindSuffix(TYPE), "(", left->text, ", ");

    if (SHIFT == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenBind(generator, TYPE, MAREA_CGEN_JOIN(generator, helper, "(uint64_t)(", right->text, "), marea_trap)"), outValue);
    } else {
        MAREA_CgenBind(generator, TYPE, MAREA_CGEN_JOIN(generator, helper, right->text, ((row->traps == MAREA_BOOLEAN_TRUE) ? ", marea_trap)" : ")")), outValue);
    }

    if (row->traps == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteTrapGuard(generator);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: an operand of a comparison of enums is the tag, read where the operand is written. A variant written by name compares its number and builds no value.
static MAREA_CgenExcept MAREA_CALL MAREA_G_ComparisonOperand(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 hint, MAREA_Uinteger32 *outEnum, MAREA_CgenValue *outValue)
{
    *outEnum = MAREA_CGEN_KIND_NONE;

    if (MAREA_IrNodeGetKind(node) == MAREA_IR_KIND_MEMBER_ACCESS) {
        MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

        MAREA_CGEN_TRY(MAREA_CgenQualifierType(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), &type));

        const MAREA_IrNode *member = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

        MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0);

        if (MAREA_CgenFindVariant(generator, type, MAREA_IrNodeGetText(member), MAREA_IrNodeGetTextLength(member), &variant) == MAREA_BOOLEAN_TRUE) {
            *outEnum = type;

            outValue->type = MAREA_CgenTypeAt(generator, type)->element;
            outValue->text = MAREA_CGEN_JOIN(generator, "((", MAREA_CgenTypeName(generator, outValue->type), ")", MAREA_CgenNumber(generator, variant), ")");
            outValue->stable = MAREA_BOOLEAN_TRUE;
            outValue->unaligned = MAREA_BOOLEAN_FALSE;

            return MAREA_CGEN_EXCEPT_SUCCESS;
        }
    }

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, node, hint, outValue));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, outValue->type);

    if (CATEGORY == MAREA_CGEN_CATEGORY_ENUM) {
        *outEnum = outValue->type;

        const MAREA_Character8 *tag = (MAREA_CgenIsAggregate(generator, outValue->type) == MAREA_BOOLEAN_TRUE) ? MAREA_CGEN_JOIN(generator, "(", outValue->text, ").marea_tag") : outValue->text;

        MAREA_CgenBind(generator, MAREA_CgenTypeAt(generator, outValue->type)->element, tag, outValue);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (CATEGORY != MAREA_CGEN_CATEGORY_SCALAR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A comparison needs two scalar values or two enum values.");
    }

    if (outValue->stable == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenSnapshot(generator, outValue);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static const MAREA_Character8 *MAREA_CALL MAREA_G_ComparisonName(MAREA_TokenKind token)
{
    switch (token) {
    case MAREA_TOKEN_KIND_EQUAL_EQUAL:
        return "eq";
    case MAREA_TOKEN_KIND_BANG_EQUAL:
        return "ne";
    case MAREA_TOKEN_KIND_LESS:
        return "lt";
    case MAREA_TOKEN_KIND_LESS_EQUAL:
        return "le";
    case MAREA_TOKEN_KIND_GREATER:
        return "gt";
    case MAREA_TOKEN_KIND_GREATER_EQUAL:
        return "ge";
    default:
        return MAREA_NULLPTR;
    }
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteComparison(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue)
{
    MAREA_CgenValue left = { 0 };
    MAREA_CgenValue right = { 0 };

    MAREA_Uinteger32 leftEnum = MAREA_CGEN_KIND_NONE;
    MAREA_Uinteger32 rightEnum = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_G_ComparisonOperand(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_NONE, &leftEnum, &left));

    const MAREA_Uinteger32 HINT = (leftEnum == MAREA_CGEN_KIND_NONE) ? left.type : (MAREA_Uinteger32)MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_G_ComparisonOperand(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), HINT, &rightEnum, &right));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *name = MAREA_G_ComparisonName(MAREA_IrNodeGetOperator(node));

    const MAREA_Boolean ORDERED = ((name != MAREA_NULLPTR) && (name[0] != 'e') && (name[0] != 'n'));

    if ((name == MAREA_NULLPTR) || (leftEnum != rightEnum) || (left.type != right.type) || ((left.type == MAREA_CGEN_KIND_BOOLEAN) && (ORDERED == MAREA_BOOLEAN_TRUE))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A comparison of these two values is not written.");
    }

    MAREA_CgenBind(generator, MAREA_CGEN_KIND_BOOLEAN, MAREA_CGEN_JOIN(generator, "marea_rt_", name, "_", MAREA_CgenKindSuffix(left.type), "(", left.text, ", ", right.text, ")"), outValue);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the right operand runs inside a block that only runs when the left one does not decide.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteLogical(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean isAnd, MAREA_CgenValue *outValue)
{
    MAREA_CgenValue left = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_BOOLEAN, &left));

    MAREA_CgenBind(generator, MAREA_CGEN_KIND_BOOLEAN, left.text, outValue);

    MAREA_CGEN_WRITE(generator->buffer, ((isAnd == MAREA_BOOLEAN_TRUE) ? "if (" : "if (!"), outValue->text, ") {\n");

    MAREA_CgenValue right = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), MAREA_CGEN_KIND_BOOLEAN, &right));

    MAREA_CGEN_WRITE(generator->buffer, outValue->text, " = ", right.text, ";\n}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteBinary(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    if (MAREA_IrNodeGetChildCount(node) != MAREA_INTEGER_SIZE(2)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A binary expression does not have two operands.");
    }

    if ((OPERATOR == MAREA_TOKEN_KIND_KEYWORD_AND) || (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_OR)) {
        return MAREA_G_WriteLogical(generator, node, (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_AND), outValue);
    }

    if (MAREA_G_ComparisonName(OPERATOR) != MAREA_NULLPTR) {
        return MAREA_CgenWriteComparison(generator, node, outValue);
    }

    MAREA_CgenValue left = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), expected, &left));

    // INFO: operands run in the order of the source, so the left one is held before the right one runs.
    if ((generator->functionFailed == MAREA_BOOLEAN_FALSE) && (left.stable == MAREA_BOOLEAN_FALSE) && (MAREA_CgenIsAggregate(generator, left.type) == MAREA_BOOLEAN_FALSE)) {
        MAREA_CgenSnapshot(generator, &left);
    }

    MAREA_CgenValue right = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), ((MAREA_G_IsShift(OPERATOR) == MAREA_BOOLEAN_TRUE) ? (MAREA_Uinteger32)MAREA_CGEN_KIND_NONE : left.type), &right));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    return MAREA_CgenWriteOperator(generator, node, OPERATOR, &left, &right, outValue);
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteMove(MAREA_CgenGenerator *generator, const MAREA_IrNode *operand, MAREA_CgenValue *outValue)
{
    MAREA_CgenValue place = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, operand, MAREA_CGEN_KIND_NONE, &place));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    *outValue = place;

    if (MAREA_CgenIsAggregate(generator, place.type) == MAREA_BOOLEAN_FALSE) {
        if (place.stable == MAREA_BOOLEAN_FALSE) {
            MAREA_CgenSnapshot(generator, outValue);
        }

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    // INFO: a moved composite value leaves zeros behind, as in the vm.
    MAREA_CgenSnapshot(generator, outValue);

    MAREA_CgenValue zero = { 0 };

    MAREA_CgenZeroTemporary(generator, place.type, &zero);
    MAREA_CgenStore(generator, place.text, place.type, zero.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteNegation(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *operand, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 OWN = MAREA_CgenKindFromText(MAREA_IrNodeGetResolvedType(node), MAREA_IrNodeGetResolvedTypeLength(node));

    const MAREA_Uinteger32 USED = (MAREA_CgenIsNumber(OWN) == MAREA_BOOLEAN_TRUE) ? OWN : expected;

    if ((MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_INTEGER_LITERAL) || (MAREA_IrNodeGetKind(operand) == MAREA_IR_KIND_FLOAT_LITERAL)) {
        return MAREA_CgenWriteNumberLiteral(generator, operand, MAREA_BOOLEAN_TRUE, USED, outValue);
    }

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, operand, USED, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenIsNumber(value.type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A negation needs a number.");
    }

    if (MAREA_CgenIsFloat(value.type) == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenBind(generator, value.type, MAREA_CGEN_JOIN(generator, "-(", value.text, ")"), outValue);
    } else {
        MAREA_CgenBind(generator, value.type, MAREA_CGEN_JOIN(generator, "marea_rt_neg_", MAREA_CgenKindSuffix(value.type), "(", value.text, ")"), outValue);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteUnary(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 expected, MAREA_CgenValue *outValue)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);
    const MAREA_IrNode *operand = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));

    if (operand == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A unary expression has no operand.");
    }

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_MOVE) {
        return MAREA_G_WriteMove(generator, operand, outValue);
    }

    if (OPERATOR == MAREA_TOKEN_KIND_MINUS) {
        return MAREA_G_WriteNegation(generator, node, operand, expected, outValue);
    }

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_SPAWN) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "The handle of a spawn has no form yet.");
    }

    MAREA_CgenValue value = { 0 };

    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_NOT) {
        MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, operand, MAREA_CGEN_KIND_BOOLEAN, &value));

        MAREA_CgenBind(generator, MAREA_CGEN_KIND_BOOLEAN, MAREA_CGEN_JOIN(generator, "!(", value.text, ")"), outValue);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, operand, expected, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    // INFO: a copy is a new value that no later change of its source reaches.
    if (OPERATOR == MAREA_TOKEN_KIND_KEYWORD_COPY) {
        *outValue = value;

        if ((value.stable == MAREA_BOOLEAN_FALSE) || (MAREA_CgenIsAggregate(generator, value.type) == MAREA_BOOLEAN_TRUE)) {
            MAREA_CgenSnapshot(generator, outValue);
        }

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((OPERATOR != MAREA_TOKEN_KIND_TILDE) || (MAREA_CgenIsInteger(value.type) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A unary operator of this value is not written.");
    }

    MAREA_CgenBind(generator, value.type, MAREA_CGEN_JOIN(generator, "marea_rt_not_", MAREA_CgenKindSuffix(value.type), "(", value.text, ")"), outValue);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteConvert(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenValue *outValue)
{
    const MAREA_IrNode *target = MAREA_CgenChildAt(MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *arguments = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    if ((target == MAREA_NULLPTR) || (arguments == MAREA_NULLPTR) || (MAREA_IrNodeGetChildCount(arguments) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A convert has not one type and one value.");
    }

    const MAREA_Uinteger32 TARGET = MAREA_CgenKindFromText(MAREA_IrNodeGetText(target), MAREA_IrNodeGetTextLength(target));

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), MAREA_CGEN_KIND_NONE, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Uinteger32 SOURCE = value.type;

    if ((MAREA_CgenIsNumber(TARGET) == MAREA_BOOLEAN_FALSE) || (MAREA_CgenIsNumber(SOURCE) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A convert takes a number and gives a number.");
    }

    if (SOURCE == TARGET) {
        *outValue = value;

        if (value.stable == MAREA_BOOLEAN_FALSE) {
            MAREA_CgenSnapshot(generator, outValue);
        }

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *cType = MAREA_CgenTypeName(generator, TARGET);

    if (MAREA_CgenIsFloat(TARGET) == MAREA_BOOLEAN_TRUE) {
        // INFO: the vm converts through double, which a 64-bit integer can round twice on the way to float32.
        const MAREA_Character8 *wide = (MAREA_CgenIsFloat(SOURCE) == MAREA_BOOLEAN_TRUE) ? "" : ((MAREA_CgenIsSigned(SOURCE) == MAREA_BOOLEAN_TRUE) ? "(double)(int64_t)" : "(double)(uint64_t)");

        MAREA_CgenBind(generator, TARGET, MAREA_CGEN_JOIN(generator, "(", cType, ")", wide, "(", value.text, ")"), outValue);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Character8 *form = (MAREA_CgenIsFloat(SOURCE) == MAREA_BOOLEAN_TRUE) ? "marea_rt_fi_" : ((MAREA_CgenIsSigned(SOURCE) == MAREA_BOOLEAN_TRUE) ? "marea_rt_si_" : "marea_rt_ui_");
    const MAREA_Character8 *cast = (MAREA_CgenIsFloat(SOURCE) == MAREA_BOOLEAN_TRUE) ? "(double)(" : ((MAREA_CgenIsSigned(SOURCE) == MAREA_BOOLEAN_TRUE) ? "(int64_t)(" : "(uint64_t)(");

    MAREA_CgenBind(generator, TARGET, MAREA_CGEN_JOIN(generator, form, MAREA_CgenKindSuffix(TARGET), "(", cast, value.text, "), marea_trap)"), outValue);
    MAREA_CgenWriteTrapGuard(generator);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
