// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

// INFO: the members, the literal and the index of a vector. The storage of a vector is changed only by the runtime
// of marea/base/MAREAVectorRuntime.h, which the vm calls too; the elements are written here.

static void MAREA_CALL MAREA_G_SetResult(MAREA_CgenValue *value, const MAREA_Character8 *text, MAREA_Uinteger32 type)
{
    value->text = text;
    value->type = type;
    value->stable = MAREA_BOOLEAN_TRUE;
    value->unaligned = MAREA_BOOLEAN_FALSE;
}

// INFO: an element of no size has no storage of its own, so every one of them is the same zeroed byte of the function.
const MAREA_Character8 *MAREA_CALL MAREA_CgenElementText(MAREA_CgenGenerator *generator, const MAREA_Character8 *vector, MAREA_Uinteger32 element, const MAREA_Character8 *index)
{
    const MAREA_Character8 *type = MAREA_CgenTypeName(generator, element);

    if (MAREA_CgenSizeOf(generator, element) == MAREA_UINTEGER_64(0)) {
        const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

        MAREA_CGEN_WRITE(generator->prelude, type, " ", name, ";\nmemset(&", name, ", 0, sizeof(", name, "));\n");
        MAREA_CGEN_WRITE(generator->buffer, "(void)(", index, ");\n");

        return name;
    }

    return MAREA_CGEN_JOIN(generator, "(((", type, " *)(uintptr_t)(", vector, ").data)[", index, "])");
}

const MAREA_Character8 *MAREA_CALL MAREA_CgenStrideText(MAREA_CgenGenerator *generator, MAREA_Uinteger32 element)
{
    return MAREA_CGEN_JOIN(generator, MAREA_CgenNumber(generator, MAREA_CgenSizeOf(generator, element)), "ULL, ", MAREA_CgenNumber(generator, MAREA_CgenAlignmentOf(generator, element)), "ULL");
}

void MAREA_CALL MAREA_CgenWriteMemoryGuard(MAREA_CgenGenerator *generator, const MAREA_Character8 *context)
{
    MAREA_CGEN_WRITE(generator->buffer, "if (MAREA_HostContextGetTrap(&", context, ") != MAREA_HOST_TRAP_NONE) {\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_OUT_OF_MEMORY");
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

static void MAREA_CALL MAREA_G_WriteIndexGuard(MAREA_CgenGenerator *generator, const MAREA_Character8 *condition)
{
    MAREA_CGEN_WRITE(generator->buffer, "if (", condition, ") {\n");
    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_INDEX_OUT_OF_BOUNDS");
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

// INFO: a count or an index goes into a temporary as an unsigned 64-bit value, so a negative one is past every size.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteNumber(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 **outName)
{
    MAREA_CgenValue value = { 0 };

    *outName = "0u";

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, node, MAREA_CGEN_KIND_UINTEGER_64, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenIsInteger(value.type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A count or an index of a vector is an integer.");
    }

    *outName = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "uint64_t ", *outName, " = (uint64_t)(", value.text, ");\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the value is held before the vector changes, since it may be one of its own elements.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteStored(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 element, MAREA_CgenValue *outValue)
{
    MAREA_CGEN_TRY(MAREA_CgenWriteOwned(generator, node, element, outValue));

    if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenSnapshot(generator, outValue);
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVectorIndex(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *base, MAREA_CgenValue *outPlace)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, base->type)->element;

    const MAREA_Character8 *index = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_G_WriteNumber(generator, MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1)), &index));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_G_WriteIndexGuard(generator, MAREA_CGEN_JOIN(generator, index, " >= (", base->text, ").size"));

    MAREA_G_SetResult(outPlace, MAREA_CgenElementText(generator, base->text, ELEMENT, index), ELEMENT);

    outPlace->stable = MAREA_BOOLEAN_FALSE;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVectorLiteral(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Uinteger32 type, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, type)->element;
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(node);

    MAREA_CgenValue made = { 0 };

    MAREA_CgenZeroTemporary(generator, type, &made);

    if (COUNT > MAREA_INTEGER_SIZE(0)) {
        MAREA_CgenWriteText(generator->buffer, "{\n");

        const MAREA_Character8 *context = MAREA_CgenWriteContext(generator);

        MAREA_CGEN_WRITE(generator->buffer, "MAREA_VectorReserve(&", context, ", &", made.text, ", ", MAREA_CgenNumber(generator, COUNT), "ULL, ", MAREA_CgenStrideText(generator, ELEMENT), ");\n");

        MAREA_CgenWriteMemoryGuard(generator, context);
        MAREA_CgenWriteText(generator->buffer, "}\n");
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); (index < COUNT) && (generator->functionFailed == MAREA_BOOLEAN_FALSE); index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *entry = MAREA_CgenChildAt(node, index);

        if (MAREA_IrNodeGetKind(entry) != MAREA_IR_KIND_VALUE_ENTRY) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A literal of a vector holds only values.");
        }

        MAREA_CgenValue value = { 0 };

        MAREA_CGEN_TRY(MAREA_G_WriteStored(generator, MAREA_CgenChildAt(entry, MAREA_INTEGER_SIZE(0)), ELEMENT, &value));

        const MAREA_Character8 *position = MAREA_CgenNumber(generator, index);

        MAREA_CgenStore(generator, MAREA_CgenElementText(generator, made.text, ELEMENT, position), ELEMENT, value.text);
        MAREA_CGEN_WRITE(generator->buffer, made.text, ".size = ", position, "ULL + 1u;\n");
    }

    generator->usesVector = MAREA_BOOLEAN_TRUE;

    *outValue = made;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the element at an index leaves the vector and its owner is the result; with no owner it is destroyed here.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteTaken(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_Character8 *index, MAREA_Boolean isUsed, MAREA_Boolean isErase, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    MAREA_CgenValue taken = { 0 };

    MAREA_G_SetResult(&taken, MAREA_CgenElementText(generator, receiver->text, ELEMENT, index), ELEMENT);

    MAREA_CgenSnapshot(generator, &taken);

    if (isErase == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_WRITE(generator->buffer, "MAREA_VectorClose(&(", receiver->text, "), ", index, ", ", MAREA_CgenNumber(generator, MAREA_CgenSizeOf(generator, ELEMENT)), "ULL);\n");
    } else {
        MAREA_CGEN_WRITE(generator->buffer, "(", receiver->text, ").size = ", index, ";\n");
    }

    if (isUsed == MAREA_BOOLEAN_TRUE) {
        *outValue = taken;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CGEN_WRITE(generator->buffer, "(void)", taken.text, ";\n");

    return MAREA_CgenWriteDrop(generator, node, taken.text, ELEMENT, MAREA_BOOLEAN_TRUE);
}

// INFO: destroys the elements from the last one down to a size.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteTruncate(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_Character8 *size)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    MAREA_CGEN_WRITE(generator->buffer, "while ((", receiver->text, ").size > ", size, ") {\n(", receiver->text, ").size -= 1u;\n");

    if (MAREA_CgenElementNeedsDrop(generator, ELEMENT) == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_TRY(MAREA_CgenWriteDrop(generator, node, MAREA_CgenElementText(generator, receiver->text, ELEMENT, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size")), ELEMENT, MAREA_BOOLEAN_TRUE));
    }

    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteAdd(MAREA_CgenGenerator *generator, const MAREA_CgenValue *receiver, const MAREA_IrNode *arguments, MAREA_Boolean isInsert)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    const MAREA_Character8 *index = MAREA_NULLPTR;

    if (isInsert == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_TRY(MAREA_G_WriteNumber(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), &index));
    }

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_G_WriteStored(generator, MAREA_CgenChildAt(arguments, (isInsert == MAREA_BOOLEAN_TRUE) ? MAREA_INTEGER_SIZE(1) : MAREA_INTEGER_SIZE(0)), ELEMENT, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (isInsert == MAREA_BOOLEAN_TRUE) {
        MAREA_G_WriteIndexGuard(generator, MAREA_CGEN_JOIN(generator, index, " > (", receiver->text, ").size"));
    } else {
        index = MAREA_CgenTemporaryName(generator);

        MAREA_CGEN_WRITE(generator->buffer, "uint64_t ", index, " = (", receiver->text, ").size;\n");
    }

    MAREA_CgenWriteText(generator->buffer, "{\n");

    const MAREA_Character8 *context = MAREA_CgenWriteContext(generator);

    MAREA_CGEN_WRITE(generator->buffer, "MAREA_VectorOpen(&", context, ", &(", receiver->text, "), ", index, ", ", MAREA_CgenStrideText(generator, ELEMENT), ");\n");

    MAREA_CgenWriteMemoryGuard(generator, context);
    MAREA_CgenWriteText(generator->buffer, "}\n");

    MAREA_CgenStore(generator, MAREA_CgenElementText(generator, receiver->text, ELEMENT, index), ELEMENT, value.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: reserve, shrink_to_fit and the growth of resize ask the runtime, which may refuse.
static void MAREA_CALL MAREA_G_WriteStorageCall(MAREA_CgenGenerator *generator, const MAREA_Character8 *function, const MAREA_CgenValue *receiver, const MAREA_Character8 *count)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    MAREA_CgenWriteText(generator->buffer, "{\n");

    const MAREA_Character8 *context = MAREA_CgenWriteContext(generator);

    MAREA_CGEN_WRITE(generator->buffer, function, "(&", context, ", &(", receiver->text, "), ", ((count == MAREA_NULLPTR) ? "" : count), ((count == MAREA_NULLPTR) ? "" : ", "), MAREA_CgenStrideText(generator, ELEMENT), ");\n");

    MAREA_CgenWriteMemoryGuard(generator, context);
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

// INFO: every element up to count becomes the value; fill destroys the old ones, a grown element has none.
static void MAREA_CALL MAREA_G_WriteRepeat(MAREA_CgenGenerator *generator, const MAREA_CgenValue *receiver, const MAREA_Character8 *first, const MAREA_Character8 *last, const MAREA_Character8 *value)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    const MAREA_Character8 *index = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "for (uint64_t ", index, " = ", first, "; ", index, " < ", last, "; ", index, " += 1u) {\n");
    MAREA_CgenStore(generator, MAREA_CgenElementText(generator, receiver->text, ELEMENT, index), ELEMENT, value);
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteResize(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *arguments)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    const MAREA_Character8 *count = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_G_WriteNumber(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), &count));

    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_G_WriteStored(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(1)), ELEMENT, &value));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_CGEN_TRY(MAREA_G_WriteTruncate(generator, node, receiver, count));

    MAREA_G_WriteStorageCall(generator, "MAREA_VectorGrow", receiver, count);

    MAREA_G_WriteRepeat(generator, receiver, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size"), count, value.text);

    MAREA_CGEN_WRITE(generator->buffer, "(", receiver->text, ").size = ", count, ";\n");

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
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A swap takes a vector of the same type.");
    }

    MAREA_CgenValue held = *receiver;

    MAREA_CgenSnapshot(generator, &held);

    MAREA_CgenStore(generator, receiver->text, receiver->type, other.text);
    MAREA_CgenStore(generator, other.text, receiver->type, held.text);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: the members that give a value: the two counts, the empty test, the largest size and the two ends.
static MAREA_Boolean MAREA_CALL MAREA_G_WriteQuery(MAREA_CgenGenerator *generator, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    if ((MAREA_CgenTextIs(member, "size") == MAREA_BOOLEAN_TRUE) || (MAREA_CgenTextIs(member, "capacity") == MAREA_BOOLEAN_TRUE)) {
        MAREA_CgenBind(generator, MAREA_CGEN_KIND_UINTEGER_64, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").", ((MAREA_CgenTextIs(member, "size") == MAREA_BOOLEAN_TRUE) ? "size" : "capacity")), outValue);
    } else if (MAREA_CgenTextIs(member, "empty") == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenBind(generator, MAREA_CGEN_KIND_BOOLEAN, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size == 0u"), outValue);
    } else if (MAREA_CgenTextIs(member, "max_size") == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenBind(generator, MAREA_CGEN_KIND_UINTEGER_64, MAREA_CGEN_JOIN(generator, "MAREA_VectorMaxSize(", MAREA_CgenNumber(generator, MAREA_CgenSizeOf(generator, ELEMENT)), "ULL)"), outValue);
    } else if ((MAREA_CgenTextIs(member, "front") == MAREA_BOOLEAN_TRUE) || (MAREA_CgenTextIs(member, "back") == MAREA_BOOLEAN_TRUE)) {
        MAREA_G_WriteIndexGuard(generator, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size == 0u"));

        const MAREA_Character8 *index = (MAREA_CgenTextIs(member, "front") == MAREA_BOOLEAN_TRUE) ? "0u" : MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size - 1u");

        // INFO: the result is a copy of the element, so a later write of the vector does not change it.
        MAREA_G_SetResult(outValue, MAREA_CgenElementText(generator, receiver->text, ELEMENT, index), ELEMENT);

        MAREA_CgenSnapshot(generator, outValue);
    } else {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteVectorCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenValue *receiver, const MAREA_IrNode *member, const MAREA_IrNode *arguments, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue)
{
    const MAREA_Uinteger32 ELEMENT = MAREA_CgenTypeAt(generator, receiver->type)->element;

    generator->usesVector = MAREA_BOOLEAN_TRUE;

    MAREA_G_SetResult(outValue, "0", MAREA_CGEN_KIND_VOID);

    if (MAREA_G_WriteQuery(generator, receiver, member, outValue) == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenTextIs(member, "pop") == MAREA_BOOLEAN_TRUE) {
        MAREA_G_WriteIndexGuard(generator, MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size == 0u"));

        const MAREA_Character8 *last = MAREA_CgenTemporaryName(generator);

        MAREA_CGEN_WRITE(generator->buffer, "uint64_t ", last, " = (", receiver->text, ").size - 1u;\n");

        return MAREA_G_WriteTaken(generator, node, receiver, last, valueUsed, MAREA_BOOLEAN_FALSE, outValue);
    }

    if (MAREA_CgenTextIs(member, "erase") == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *index = MAREA_NULLPTR;

        MAREA_CGEN_TRY(MAREA_G_WriteNumber(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), &index));

        MAREA_G_WriteIndexGuard(generator, MAREA_CGEN_JOIN(generator, index, " >= (", receiver->text, ").size"));

        return MAREA_G_WriteTaken(generator, node, receiver, index, valueUsed, MAREA_BOOLEAN_TRUE, outValue);
    }

    if ((MAREA_CgenTextIs(member, "append") == MAREA_BOOLEAN_TRUE) || (MAREA_CgenTextIs(member, "insert") == MAREA_BOOLEAN_TRUE)) {
        return MAREA_G_WriteAdd(generator, receiver, arguments, MAREA_CgenTextIs(member, "insert"));
    }

    if (MAREA_CgenTextIs(member, "reserve") == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *count = MAREA_NULLPTR;

        MAREA_CGEN_TRY(MAREA_G_WriteNumber(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), &count));

        MAREA_G_WriteStorageCall(generator, "MAREA_VectorReserve", receiver, count);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenTextIs(member, "shrink_to_fit") == MAREA_BOOLEAN_TRUE) {
        MAREA_G_WriteStorageCall(generator, "MAREA_VectorShrink", receiver, MAREA_NULLPTR);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenTextIs(member, "clear") == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_WriteTruncate(generator, node, receiver, "0u");
    }

    if (MAREA_CgenTextIs(member, "resize") == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_WriteResize(generator, node, receiver, arguments);
    }

    if (MAREA_CgenTextIs(member, "fill") == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenValue value = { 0 };

        MAREA_CGEN_TRY(MAREA_G_WriteStored(generator, MAREA_CgenChildAt(arguments, MAREA_INTEGER_SIZE(0)), ELEMENT, &value));

        if (generator->functionFailed == MAREA_BOOLEAN_FALSE) {
            MAREA_G_WriteRepeat(generator, receiver, "0u", MAREA_CGEN_JOIN(generator, "(", receiver->text, ").size"), value.text);
        }

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenTextIs(member, "swap") == MAREA_BOOLEAN_TRUE) {
        return MAREA_G_WriteSwap(generator, node, receiver, arguments);
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A member of a vector is not written.");
}
