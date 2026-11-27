// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

// INFO: a tuple is a generated structure, so its ownership and its passing are those of a structure. This file writes
// what a structure does not have: the constant index, == and != and swap.

static MAREA_CgenValue MAREA_CALL MAREA_G_ElementOf(MAREA_CgenGenerator *generator, const MAREA_CgenValue *base, MAREA_IntegerSize position)
{
    const MAREA_CgenField *field = &MAREA_CgenTypeAt(generator, base->type)->fields[position];

    const MAREA_Boolean PACKED = MAREA_CgenTypeAt(generator, base->type)->packed;

    return (MAREA_CgenValue){
        .text = MAREA_CGEN_JOIN(generator, "(", base->text, ").", MAREA_CgenMangled(generator, "f_", field->name, field->nameLength)),
        .type = field->type,
        .stable = MAREA_BOOLEAN_FALSE,
        .unaligned = ((base->unaligned == MAREA_BOOLEAN_TRUE) || (PACKED == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE,
    };
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTupleIndex(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, MAREA_CgenValue *outPlace)
{
    const MAREA_IrNode *key = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    MAREA_Uinteger64 position = MAREA_UINTEGER_64(0);

    if ((MAREA_IrNodeGetKind(key) != MAREA_IR_KIND_INTEGER_LITERAL) || (MAREA_CgenParseInteger(MAREA_IrNodeGetText(key), MAREA_IrNodeGetTextLength(key), &position) == MAREA_BOOLEAN_FALSE) || (position >= MAREA_CgenTypeAt(generator, base->type)->memberCount)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "The index of a tuple is an integer literal below its arity.");
    }

    *outPlace = MAREA_G_ElementOf(generator, base, (MAREA_IntegerSize)position);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: clears the result when one element differs. A float that is not a number differs from itself, as the vm answers.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteEquality(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *left, const MAREA_CgenValue *right, const MAREA_Character8 *result)
{
    const MAREA_IntegerSize COUNT = MAREA_CgenTypeAt(generator, left->type)->memberCount;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenValue ONE = MAREA_G_ElementOf(generator, left, index);
        const MAREA_CgenValue OTHER = MAREA_G_ElementOf(generator, right, index);

        if (MAREA_CgenIsTuple(generator, ONE.type) == MAREA_BOOLEAN_TRUE) {
            MAREA_CGEN_TRY(MAREA_G_WriteEquality(generator, node, &ONE, &OTHER, result));

            continue;
        }

        const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, ONE.type);

        if (CATEGORY == MAREA_CGEN_CATEGORY_ENUM) {
            const MAREA_Character8 *tag = ((MAREA_CgenIsAggregate(generator, ONE.type) == MAREA_BOOLEAN_TRUE) ? ".marea_tag" : "");

            MAREA_CGEN_WRITE(generator->buffer, result, " = (", result, " && ((", ONE.text, ")", tag, " == (", OTHER.text, ")", tag, "));\n");

            continue;
        }

        if (CATEGORY != MAREA_CGEN_CATEGORY_SCALAR) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An element of a tuple has no == that is written.");
        }

        MAREA_CGEN_WRITE(generator->buffer, result, " = (", result, " && marea_rt_eq_", MAREA_CgenKindSuffix(ONE.type), "(", ONE.text, ", ", OTHER.text, "));\n");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTupleComparison(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 type, MAREA_CgenValue *outValue)
{
    const MAREA_TokenKind OPERATOR = MAREA_IrNodeGetOperator(node);

    if ((OPERATOR != MAREA_TOKEN_KIND_EQUAL_EQUAL) && (OPERATOR != MAREA_TOKEN_KIND_BANG_EQUAL)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A tuple has no order.");
    }

    MAREA_CgenValue left = { 0 };
    MAREA_CgenValue right = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0)), type, &left));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    // INFO: operands run in the order of the source, so the left one is held before the right one runs.
    MAREA_CgenSnapshot(generator, &left);

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), type, &right));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((left.type != type) || (right.type != type)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A comparison needs two tuples of one type.");
    }

    MAREA_CgenValue result = { 0 };

    MAREA_CgenBind(generator, MAREA_CGEN_KIND_BOOLEAN, "true", &result);

    MAREA_CGEN_TRY(MAREA_G_WriteEquality(generator, node, &left, &right, result.text));

    if (OPERATOR == MAREA_TOKEN_KIND_EQUAL_EQUAL) {
        *outValue = result;
    } else {
        MAREA_CgenBind(generator, MAREA_CGEN_KIND_BOOLEAN, MAREA_CGEN_JOIN(generator, "!(", result.text, ")"), outValue);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteTupleCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue)
{
    MAREA_IGNORE(outValue);

    if ((MAREA_CgenTextIs(member, "swap") == MAREA_BOOLEAN_FALSE) || (arguments == MAREA_NULLPTR) || (MAREA_IrNodeGetChildCount(arguments) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "swap is the only method of a tuple.");
    }

    MAREA_CgenValue other = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), receiver->type, &other));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (other.type != receiver->type) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "swap needs two tuples of one type.");
    }

    MAREA_CgenValue held = *receiver;

    held.stable = MAREA_BOOLEAN_FALSE;

    MAREA_CgenSnapshot(generator, &held);
    MAREA_CgenStore(generator, receiver->text, receiver->type, other.text);
    MAREA_CgenStore(generator, other.text, other.type, held.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
