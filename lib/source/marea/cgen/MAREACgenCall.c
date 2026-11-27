// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

// INFO: a borrow of storage inside a packed type passes a copy, since C asks the address of a structure to be aligned; a write borrow copies the value back after the call.
typedef struct MAREA_G_CopyBack
{
    const MAREA_Character8 *place;
    const MAREA_Character8 *copy;
} MAREA_G_CopyBack;

// INFO: a failed callee leaves its caller out too, so the written program links.
static MAREA_CgenExcept MAREA_CALL MAREA_G_RecordCallee(MAREA_CgenGenerator *generator, const MAREA_CgenFunction *callee)
{
    MAREA_CgenFunction *caller = generator->current;

    MAREA_Null items = caller->callees;

    MAREA_CGEN_TRY(MAREA_CgenGrow(generator, &items, &caller->calleeCapacity, caller->calleeCount, sizeof(MAREA_IntegerSize), MAREA_ALIGNOF(MAREA_IntegerSize)));

    caller->callees = items;
    caller->callees[caller->calleeCount] = (MAREA_IntegerSize)(callee - generator->functions);
    caller->calleeCount = (caller->calleeCount + MAREA_INTEGER_SIZE(1));

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteArgument(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_CgenParameter *parameter, const MAREA_CgenValue *receiver, const MAREA_IrNode *argument, MAREA_G_CopyBack *copyBack, const MAREA_Character8 **outText)
{
    MAREA_CgenValue value = { 0 };

    if (receiver != MAREA_NULLPTR) {
        value = *receiver;
    } else if (parameter->passing == MAREA_CGEN_PASSING_VALUE) {
        MAREA_CGEN_TRY(MAREA_CgenWriteTyped(generator, argument, parameter->type, &value));
    } else {
        MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, argument, parameter->type, &value));
    }

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        *outText = "0";

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (value.type != parameter->type) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An argument has another type than its parameter.");
    }

    if (parameter->passing == MAREA_CGEN_PASSING_VALUE) {
        // INFO: a value passes as it is where it is written, whatever a later argument changes.
        if ((value.stable == MAREA_BOOLEAN_FALSE) || (MAREA_CgenIsAggregate(generator, value.type) == MAREA_BOOLEAN_TRUE)) {
            MAREA_CgenSnapshot(generator, &value);
        }

        *outText = value.text;

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (value.unaligned == MAREA_BOOLEAN_TRUE) {
        const MAREA_Character8 *place = value.text;

        MAREA_CgenSnapshot(generator, &value);

        if (parameter->passing == MAREA_CGEN_PASSING_MUTABLE_BORROW) {
            copyBack->place = place;
            copyBack->copy = value.text;
        }
    }

    *outText = MAREA_CGEN_JOIN(generator, "&(", value.text, ")");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteUnitCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_CgenFunction *function, const MAREA_CgenValue *receiver, const MAREA_IrNode *arguments, MAREA_CgenValue *outValue)
{
    outValue->text = "0";
    outValue->type = MAREA_CGEN_KIND_VOID;
    outValue->stable = MAREA_BOOLEAN_TRUE;
    outValue->unaligned = MAREA_BOOLEAN_FALSE;

    if (function->supported == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call names a function that cgen does not write.");
    }

    const MAREA_IntegerSize GIVEN = (arguments == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(arguments);
    const MAREA_IntegerSize OFFSET = (receiver == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_INTEGER_SIZE(1);
    const MAREA_IntegerSize TOTAL = (GIVEN + OFFSET);

    if ((TOTAL != function->parameterCount) || ((receiver != MAREA_NULLPTR) && (MAREA_CgenTextIs(function->parameters[0].node, "self") == MAREA_BOOLEAN_FALSE))) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call does not give one value for each parameter.");
    }

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (TOTAL * sizeof(const MAREA_Character8 *)), MAREA_ALIGNOF(const MAREA_Character8 *), &memory));

    const MAREA_Character8 **texts = memory;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (TOTAL * sizeof(MAREA_G_CopyBack)), MAREA_ALIGNOF(MAREA_G_CopyBack), &memory));

    MAREA_G_CopyBack *copies = memory;

    MAREA_CGEN_TRY(MAREA_G_RecordCallee(generator, function));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < TOTAL; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_CgenValue *given = ((receiver != MAREA_NULLPTR) && (index == MAREA_INTEGER_SIZE(0))) ? receiver : MAREA_NULLPTR;

        const MAREA_IrNode *argument = (given == MAREA_NULLPTR) ? MAREA_CgenChildAt(arguments, (index - OFFSET)) : MAREA_NULLPTR;

        MAREA_CGEN_TRY(MAREA_G_WriteArgument(generator, node, &function->parameters[index], given, argument, &copies[index], &texts[index]));

        if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }
    }

    const MAREA_Character8 *call = MAREA_CGEN_JOIN(generator, function->cName, "(");

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < TOTAL; index = (index + MAREA_INTEGER_SIZE(1))) {
        call = MAREA_CGEN_JOIN(generator, call, texts[index], ", ");
    }

    call = MAREA_CGEN_JOIN(generator, call, "marea_trap)");

    if (function->result == MAREA_CGEN_KIND_VOID) {
        MAREA_CGEN_WRITE(generator->buffer, call, ";\n");
    } else {
        MAREA_CgenBind(generator, function->result, call, outValue);
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < TOTAL; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (copies[index].place != MAREA_NULLPTR) {
            MAREA_CgenStore(generator, copies[index].place, function->parameters[index].type, copies[index].copy);
        }
    }

    MAREA_CgenWriteTrapGuard(generator);

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WritePlainCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *callee, const MAREA_IrNode *arguments, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue)
{
    const MAREA_Character8 *text = MAREA_IrNodeGetText(callee);
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(callee);

    if (MAREA_CgenFindSymbol(generator, text, LENGTH) != MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A call through a function value is not written.");
    }

    MAREA_CgenFunction *function = MAREA_CgenFindFunction(generator, text, LENGTH);

    if (function != MAREA_NULLPTR) {
        return MAREA_CgenWriteUnitCall(generator, node, function, MAREA_NULLPTR, arguments, outValue);
    }

    // INFO: a member that an alias binds reaches here as its full dotted path, which no local name can be.
    if ((MAREA_IrNodeIsExternalCall(node) == MAREA_BOOLEAN_TRUE) && (memchr(text, '.', LENGTH) != MAREA_NULLPTR)) {
        return MAREA_CgenWriteHostCall(generator, node, text, LENGTH, valueUsed, outValue);
    }

    return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A call names no function of this unit and no name that the host registered.");
}

// INFO: a call through a member names a variant or a function of a type, a function of a namespace, or a method of a value.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteMemberCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_IrNode *callee, const MAREA_IrNode *arguments, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue)
{
    const MAREA_IrNode *base = MAREA_CgenChildAt(callee, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *member = MAREA_CgenChildAt(callee, MAREA_INTEGER_SIZE(1));

    const MAREA_Character8 *name = MAREA_IrNodeGetText(member);
    const MAREA_IntegerSize LENGTH = MAREA_IrNodeGetTextLength(member);

    // INFO: a static call on a written type is one of an array, since a generic type of the unit is not instantiated.
    if (MAREA_IrNodeGetKind(base) == MAREA_IR_KIND_TYPE_NAME) {
        return MAREA_CgenWriteArrayFilled(generator, node, arguments, outValue);
    }

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    MAREA_CGEN_TRY(MAREA_CgenQualifierType(generator, base, &type));

    if (type != MAREA_CGEN_KIND_NONE) {
        MAREA_IntegerSize variant = MAREA_INTEGER_SIZE(0);

        if (MAREA_CgenFindVariant(generator, type, name, LENGTH, &variant) == MAREA_BOOLEAN_TRUE) {
            return MAREA_CgenWriteVariant(generator, node, type, variant, arguments, outValue);
        }

        const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, type);

        MAREA_CgenFunction *function = MAREA_CgenFindMember(generator, entry->name, entry->nameLength, name, LENGTH);

        if (function != MAREA_NULLPTR) {
            return MAREA_CgenWriteUnitCall(generator, node, function, MAREA_NULLPTR, arguments, outValue);
        }

        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A type has no variant and no function of this name.");
    }

    MAREA_IntegerSize fullLength = MAREA_INTEGER_SIZE(0);

    const MAREA_Character8 *full = MAREA_CgenQualifiedName(generator, callee, &fullLength);

    if (full != MAREA_NULLPTR) {
        MAREA_CgenFunction *function = MAREA_CgenFindFunction(generator, full, fullLength);

        if (function != MAREA_NULLPTR) {
            return MAREA_CgenWriteUnitCall(generator, node, function, MAREA_NULLPTR, arguments, outValue);
        }

        if (MAREA_IrNodeIsExternalCall(node) == MAREA_BOOLEAN_TRUE) {
            return MAREA_CgenWriteHostCall(generator, node, full, fullLength, valueUsed, outValue);
        }

        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNRESOLVED_NAME, "A call names no function of this unit and no name that the host registered.");
    }

    MAREA_CgenValue receiver = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWritePlace(generator, base, MAREA_CGEN_KIND_NONE, &receiver));

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if (MAREA_CgenCategoryOf(generator, receiver.type) == MAREA_CGEN_CATEGORY_ARRAY) {
        return MAREA_CgenWriteArrayCall(generator, node, &receiver, member, arguments, outValue);
    }

    const MAREA_CgenType *entry = MAREA_CgenTypeAt(generator, receiver.type);

    MAREA_CgenFunction *method = (entry == MAREA_NULLPTR) ? MAREA_NULLPTR : MAREA_CgenFindMember(generator, entry->name, entry->nameLength, name, LENGTH);

    if (method == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A method of this value is not written.");
    }

    return MAREA_CgenWriteUnitCall(generator, node, method, &receiver, arguments, outValue);
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue)
{
    const MAREA_IrNode *callee = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(0));
    const MAREA_IrNode *arguments = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    outValue->text = "0";
    outValue->type = MAREA_CGEN_KIND_VOID;
    outValue->stable = MAREA_BOOLEAN_TRUE;
    outValue->unaligned = MAREA_BOOLEAN_FALSE;

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    switch (MAREA_IrNodeGetKind(callee)) {
    case MAREA_IR_KIND_BUILTIN_NAME:
        if (MAREA_CgenTextIs(callee, "convert") == MAREA_BOOLEAN_TRUE) {
            return MAREA_CgenWriteConvert(generator, node, outValue);
        }

        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A built-in call other than convert is not written.");
    case MAREA_IR_KIND_IDENTIFIER:
        return MAREA_G_WritePlainCall(generator, node, callee, arguments, valueUsed, outValue);
    case MAREA_IR_KIND_MEMBER_ACCESS:
        return MAREA_G_WriteMemberCall(generator, node, callee, arguments, valueUsed, outValue);
    default:
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A call of this shape is not written.");
    }
}
