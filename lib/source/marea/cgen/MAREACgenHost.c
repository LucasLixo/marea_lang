// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

typedef struct MAREA_G_HostArgument
{
    const MAREA_Character8 *text;
    const MAREA_Character8 *place;
    const MAREA_Character8 *copy;
    MAREA_Uinteger32 type;
    MAREA_Uinteger8 padding[4];
} MAREA_G_HostArgument;

// INFO: a primitive passes by value and every composite value by address, which the typed C function declares as a pointer to its own structure.
static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteArgument(MAREA_CgenGenerator *generator, const MAREA_IrNode *argument, MAREA_G_HostArgument *outArgument)
{
    MAREA_CgenValue value = { 0 };

    MAREA_CGEN_TRY(MAREA_CgenWriteValue(generator, argument, MAREA_CGEN_KIND_NONE, &value));

    outArgument->text = "0";

    if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    outArgument->type = value.type;

    const MAREA_Uinteger8 CATEGORY = MAREA_CgenCategoryOf(generator, value.type);

    if ((CATEGORY == MAREA_CGEN_CATEGORY_STRUCTURE) || (CATEGORY == MAREA_CGEN_CATEGORY_ARRAY) || (MAREA_CgenIsAggregate(generator, value.type) == MAREA_BOOLEAN_TRUE)) {
        if (value.unaligned == MAREA_BOOLEAN_TRUE) {
            outArgument->place = value.text;

            MAREA_CgenSnapshot(generator, &value);

            outArgument->copy = value.text;
        }

        outArgument->text = MAREA_CGEN_JOIN(generator, "(void *)&(", value.text, ")");

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    // INFO: sema types a call only from the signature of the host, so a value with no type is never guessed.
    if (MAREA_IrNodeGetResolvedType(argument) == MAREA_NULLPTR) {
        return MAREA_CgenGeneratorFail(generator, argument, MAREA_CGEN_DIAGNOSTIC_CODE_UNTYPED_OPERAND, "An argument of a host call has no type.");
    }

    if (MAREA_CgenIsValueType(generator, value.type) == MAREA_BOOLEAN_FALSE) {
        return MAREA_CgenGeneratorFail(generator, argument, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "An argument of a host call has a type that is not a value here.");
    }

    if (value.stable == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenSnapshot(generator, &value);
    }

    outArgument->text = value.text;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

// INFO: a name with no C function is the trap of the vm for a function that no host registered, raised after the arguments run.
static void MAREA_CALL MAREA_G_WriteMissing(MAREA_CgenGenerator *generator, const MAREA_G_HostArgument *arguments, MAREA_IntegerSize count, MAREA_Uinteger32 result, MAREA_CgenValue *outValue)
{
    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < count; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_WRITE(generator->buffer, "(void)(", arguments[index].text, ");\n");
    }

    if (result != MAREA_CGEN_KIND_NONE) {
        MAREA_CgenZeroTemporary(generator, result, outValue);

        MAREA_CGEN_WRITE(generator->buffer, "(void)(", outValue->text, ");\n");
    }

    MAREA_CgenWriteTrap(generator, "MAREA_RT_TRAP_HOST_FUNCTION_MISSING");
}

// INFO: the context lives for one call, on the stack, with the allocator hooks of the caller of cgen.
static const MAREA_Character8 *MAREA_CALL MAREA_G_WriteContext(MAREA_CgenGenerator *generator)
{
    const MAREA_CgenContext *context = generator->context;

    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, "MAREA_HostContext ", name, ";\nif (MAREA_HostContextInitialize(&", name, ", ", ((context->allocate == MAREA_NULLPTR) ? "NULL" : context->allocate), ", ", ((context->release == MAREA_NULLPTR) ? "NULL" : context->release), ") != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {\n__builtin_trap();\n}\n");

    generator->usesContext = MAREA_BOOLEAN_TRUE;

    return name;
}

static const MAREA_Character8 *MAREA_CALL MAREA_G_JoinArgument(MAREA_CgenGenerator *generator, const MAREA_Character8 *list, const MAREA_Character8 *argument)
{
    return (list == MAREA_NULLPTR) ? argument : MAREA_CGEN_JOIN(generator, list, ", ", argument);
}

static const MAREA_Character8 *MAREA_CALL MAREA_G_ArgumentList(MAREA_CgenGenerator *generator, const MAREA_G_HostArgument *arguments, MAREA_IntegerSize count, MAREA_IntegerSize contextPosition, const MAREA_Character8 *contextName, const MAREA_Character8 *result)
{
    const MAREA_Character8 *list = MAREA_NULLPTR;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index <= count; index = (index + MAREA_INTEGER_SIZE(1))) {
        if ((contextName != MAREA_NULLPTR) && (index == contextPosition)) {
            list = MAREA_G_JoinArgument(generator, list, MAREA_CGEN_JOIN(generator, "&", contextName));
        }

        if (index < count) {
            list = MAREA_G_JoinArgument(generator, list, arguments[index].text);
        }
    }

    if (result != MAREA_NULLPTR) {
        list = MAREA_G_JoinArgument(generator, list, MAREA_CGEN_JOIN(generator, "(void *)&", result));
    }

    return (list == MAREA_NULLPTR) ? "" : list;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenWriteHostCall(MAREA_CgenGenerator *generator, const MAREA_IrNode *node, const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Boolean valueUsed, MAREA_CgenValue *outValue)
{
    const MAREA_Character8 *resolved = MAREA_IrNodeGetResolvedType(node);

    const MAREA_Boolean RESULT = ((resolved != MAREA_NULLPTR) && (MAREA_CgenSameText(resolved, MAREA_IrNodeGetResolvedTypeLength(node), "void", MAREA_INTEGER_SIZE(4)) == MAREA_BOOLEAN_FALSE));

    if ((RESULT == MAREA_BOOLEAN_FALSE) && (valueUsed == MAREA_BOOLEAN_TRUE)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNTYPED_OPERAND, "A host call that is used as a value has no type.");
    }

    MAREA_Uinteger32 type = MAREA_CGEN_KIND_NONE;

    if (RESULT == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_TRY(MAREA_CgenTypeOfNode(generator, node, &type));

        if (MAREA_CgenIsValueType(generator, type) == MAREA_BOOLEAN_FALSE) {
            return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A host call has a result type that cgen does not write.");
        }
    }

    MAREA_CgenHostFunction host = { .symbol = MAREA_NULLPTR, .symbolLength = MAREA_INTEGER_SIZE(0), .contextPosition = MAREA_CGEN_HOST_NO_CONTEXT };

    const MAREA_CgenContext *context = generator->context;

    const MAREA_Boolean FOUND = ((context->resolve != MAREA_NULLPTR) && (context->resolve(context->userData, name, length, &host) == MAREA_BOOLEAN_TRUE) && (host.symbol != MAREA_NULLPTR));

    const MAREA_IrNode *list = MAREA_CgenChildAt(node, MAREA_INTEGER_SIZE(1));

    const MAREA_IntegerSize COUNT = (list == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(list);

    MAREA_Null memory = MAREA_NULLPTR;

    MAREA_CGEN_TRY(MAREA_CgenAllocate(generator, (COUNT * sizeof(MAREA_G_HostArgument)), MAREA_ALIGNOF(MAREA_G_HostArgument), &memory));

    MAREA_G_HostArgument *arguments = memory;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_G_WriteArgument(generator, MAREA_CgenChildAt(list, index), &arguments[index]));

        if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
            return MAREA_CGEN_EXCEPT_SUCCESS;
        }
    }

    if (FOUND == MAREA_BOOLEAN_FALSE) {
        MAREA_G_WriteMissing(generator, arguments, COUNT, type, outValue);

        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    if ((host.contextPosition != MAREA_CGEN_HOST_NO_CONTEXT) && (host.contextPosition > COUNT)) {
        return MAREA_CgenGeneratorFail(generator, node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "The context of a C function is past its parameters.");
    }

    const MAREA_Character8 *symbol = MAREA_CgenCopyText(generator, host.symbol, host.symbolLength);

    const MAREA_Character8 *contextName = (host.contextPosition == MAREA_CGEN_HOST_NO_CONTEXT) ? MAREA_NULLPTR : MAREA_G_WriteContext(generator);

    const MAREA_Boolean AGGREGATE = ((RESULT == MAREA_BOOLEAN_TRUE) && (MAREA_CgenIsAggregate(generator, type) == MAREA_BOOLEAN_TRUE));

    if (AGGREGATE == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenZeroTemporary(generator, type, outValue);
    }

    const MAREA_Character8 *call = MAREA_CGEN_JOIN(generator, symbol, "(", MAREA_G_ArgumentList(generator, arguments, COUNT, host.contextPosition, contextName, ((AGGREGATE == MAREA_BOOLEAN_TRUE) ? outValue->text : MAREA_NULLPTR)), ")");

    if ((RESULT == MAREA_BOOLEAN_FALSE) || (AGGREGATE == MAREA_BOOLEAN_TRUE)) {
        MAREA_CGEN_WRITE(generator->buffer, call, ";\n");
    } else if (valueUsed == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenBind(generator, type, MAREA_CGEN_JOIN(generator, "(", MAREA_CgenTypeName(generator, type), ")", call), outValue);
    } else {
        MAREA_CGEN_WRITE(generator->buffer, "(void)", call, ";\n");
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        if (arguments[index].copy != MAREA_NULLPTR) {
            MAREA_CgenStore(generator, arguments[index].place, arguments[index].type, arguments[index].copy);
        }
    }

    // INFO: a trap that the C function raised stops the run when it returns, and its result is dropped.
    if (contextName != MAREA_NULLPTR) {
        MAREA_CGEN_WRITE(generator->buffer, "if (MAREA_HostContextGetTrap(&", contextName, ") != MAREA_HOST_TRAP_NONE) {\n*marea_trap = (marea_rt_trap)MAREA_HostContextGetTrap(&", contextName, ");\n");
        MAREA_CgenWriteZeroReturn(generator);
        MAREA_CgenWriteText(generator->buffer, "}\n");
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
