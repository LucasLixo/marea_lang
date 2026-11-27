// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

static void MAREA_CALL MAREA_G_SetResult(MAREA_CgenValue *value, const MAREA_Character8 *text, MAREA_Uinteger32 type)
{
    value->text = text;
    value->type = type;
    value->stable = MAREA_BOOLEAN_TRUE;
    value->unaligned = MAREA_BOOLEAN_FALSE;
}

// INFO: a loop over the elements of an array; a count of zero writes none, since C has no use for a test that is never true.
static void MAREA_CALL MAREA_G_WriteFill(MAREA_CgenGenerator *generator, const MAREA_Character8 *array, MAREA_Uinteger32 type, const MAREA_Character8 *value)
{
    const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, type)->count;

    if (COUNT == MAREA_UINTEGER_64(0)) {
        return;
    }

    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = 0; ", index, " < ", MAREA_CgenNumber(generator, COUNT), "ULL; ", index, " += 1u) {\n");
    MAREA_CgenStore(generator, MAREA_CGEN_JOIN(generator, "(", array, ").items[", index, "]"), ELEMENT, value);
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

void MAREA_CALL MAREA_CgenWriteEmptyElement(MAREA_CgenGenerator *generator, MAREA_Uinteger32 element, MAREA_CgenValue *outPlace)
{
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_INDEX_OUT_OF_BOUNDS");

    // INFO: the trap leaves the function, so the zero value that stands for the element is never read.
    MAREA_CgenZeroTemporary(generator, element, outPlace);
}

// INFO: the value of a member argument is held before the array is written, since it may be one of its own elements.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteElementValue(MAREA_CgenGenerator *generator, const MAREA_IrNode *arguments, MAREA_Uinteger32 element, MAREA_CgenValue *outValue)
{
    MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), element, outValue));

    if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenSnapshot(generator, outValue);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteSwap(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *arguments)
{
    MAREA_CgenValue other = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), receiver->type, &other));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (other.type != receiver->type) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A swap takes an array of the same type.");
    }

    MAREA_CgenValue held = *receiver;

    MAREA_CgenSnapshot(generator, &held);

    MAREA_CgenStore(generator, receiver->text, receiver->type, other.text);
    MAREA_CgenStore(generator, other.text, receiver->type, held.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteArrayFilled(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue)
{
    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &type));

    if ((MAREA_CgenCategoryOf(generator, type) != MAREA_CGEN_CATEGORY_ARRAY) || (arguments == MAREA_NULLPTR) || (MAREA_IrNodeGetChildCount(arguments) != MAREA_INTEGER_SIZE(1))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "Only filled is a static function of an array.");
    }

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_G_WriteElementValue(generator, arguments, MAREA_CgenTypeAt(generator, type)->element, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CgenValue made = { 0 };

    MAREA_CgenZeroTemporary(generator, type, &made);

    MAREA_G_WriteFill(generator, made.text, type, value.text);

    *outValue = made;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteArrayCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger64 COUNT = MAREA_CgenTypeAt(generator, receiver->type)->count;
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    const MAREA_IntegerSize GIVEN = (arguments == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(arguments);

    if (((MAREA_CgenTextIs(member, "size") == MAREA_BOOLEAN_TRUE) || (MAREA_CgenTextIs(member, "max_size") == MAREA_BOOLEAN_TRUE)) && (GIVEN == MAREA_INTEGER_SIZE(0))) {
        MAREA_G_SetResult(outValue, MAREA_CGEN_JOIN(generator, "((uint64_t)", MAREA_CgenNumber(generator, COUNT), "ULL)"), MAREA_CGEN_KIND_UINTEGER_64);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((MAREA_CgenTextIs(member, "empty") == MAREA_BOOLEAN_TRUE) && (GIVEN == MAREA_INTEGER_SIZE(0))) {
        MAREA_G_SetResult(outValue, ((COUNT == MAREA_UINTEGER_64(0)) ? "true" : "false"), MAREA_CGEN_KIND_BOOLEAN);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    const MAREA_Boolean IS_BACK = MAREA_CgenTextIs(member, "back");

    if (((IS_BACK == MAREA_BOOLEAN_TRUE) || (MAREA_CgenTextIs(member, "front") == MAREA_BOOLEAN_TRUE)) && (GIVEN == MAREA_INTEGER_SIZE(0))) {
        if (COUNT == MAREA_UINTEGER_64(0)) {
            MAREA_CgenWriteEmptyElement(generator, ELEMENT, outValue);

            return MAREA_CGEN_EXCEPT_SUCCESS;
        }

        MAREA_G_SetResult(outValue, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").items[", MAREA_CgenNumber(generator, ((IS_BACK == MAREA_BOOLEAN_TRUE) ? (COUNT - MAREA_UINTEGER_64(1)) : MAREA_UINTEGER_64(0))), "ULL]"), ELEMENT);

        outValue->unaligned = receiver->unaligned;

        // INFO: the result is a copy of the element, so a later write of the array does not change it.
        MAREA_CgenSnapshot(generator, outValue);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_G_SetResult(outValue, "0", MAREA_CGEN_KIND_VOID);

    if ((MAREA_CgenTextIs(member, "fill") == MAREA_BOOLEAN_TRUE) && (GIVEN == MAREA_INTEGER_SIZE(1))) {
        MAREA_CgenValue value = { 0 };

        MAREA_CGEN_TRY(MAREA_G_WriteElementValue(generator, arguments, ELEMENT, &value));

        if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
            MAREA_G_WriteFill(generator, receiver->text, receiver->type, value.text);
        }

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((MAREA_CgenTextIs(member, "swap") == MAREA_BOOLEAN_TRUE) && (GIVEN == MAREA_INTEGER_SIZE(1))) {
        return MAREA_G_WriteSwap(generator, node, receiver, arguments);
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A member of an array is not written.");
}
