// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

// INFO: the runtime preamble every generated module opens with: the trap code and one wraparound, trapping
// division/remainder and range-checked shift helper per integer kind, matching the vm's defined outcomes
// (docs/vm/marea-vm.md) without ever relying on C undefined behavior for overflow, a zero divisor, the signed
// minimum divided by -1 or a shift past the width of the kind.
static const MAREA_Character8 MAREA_G_PREAMBLE[] =
    "#include <stdint.h>\n"
    "#include <stdbool.h>\n"
    "typedef enum marea_rt_trap { MAREA_RT_TRAP_NONE = 0, MAREA_RT_TRAP_DIVISION_BY_ZERO = 1, MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE = 2 } marea_rt_trap;\n"
    "#define MAREA_RT_ARITH(NAME, T, U) \\\n"
    "static inline __attribute__((unused)) T marea_rt_add_##NAME(T a, T b) { return (T)(U)((U)a + (U)b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_sub_##NAME(T a, T b) { return (T)(U)((U)a - (U)b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_mul_##NAME(T a, T b) { return (T)(U)((U)a * (U)b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_neg_##NAME(T a) { return (T)(U)((U)0 - (U)a); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_not_##NAME(T a) { return (T)(U)(~(U)a); }\n"
    "MAREA_RT_ARITH(s8,int8_t,uint8_t)\n"
    "MAREA_RT_ARITH(s16,int16_t,uint16_t)\n"
    "MAREA_RT_ARITH(s32,int32_t,uint32_t)\n"
    "MAREA_RT_ARITH(s64,int64_t,uint64_t)\n"
    "MAREA_RT_ARITH(u8,uint8_t,uint8_t)\n"
    "MAREA_RT_ARITH(u16,uint16_t,uint16_t)\n"
    "MAREA_RT_ARITH(u32,uint32_t,uint32_t)\n"
    "MAREA_RT_ARITH(u64,uint64_t,uint64_t)\n"
    "#undef MAREA_RT_ARITH\n"
    "#define MAREA_RT_SDIV(NAME, T, U) \\\n"
    "static inline __attribute__((unused)) T marea_rt_div_##NAME(T a, T b, marea_rt_trap *trap) { if (b == (T)0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return (T)0; } if (b == (T)-1) { return (T)(U)((U)0 - (U)a); } return (T)(a / b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_rem_##NAME(T a, T b, marea_rt_trap *trap) { if (b == (T)0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return (T)0; } if (b == (T)-1) { return (T)0; } return (T)(a % b); }\n"
    "MAREA_RT_SDIV(s8,int8_t,uint8_t)\n"
    "MAREA_RT_SDIV(s16,int16_t,uint16_t)\n"
    "MAREA_RT_SDIV(s32,int32_t,uint32_t)\n"
    "MAREA_RT_SDIV(s64,int64_t,uint64_t)\n"
    "#undef MAREA_RT_SDIV\n"
    "#define MAREA_RT_UDIV(NAME, T) \\\n"
    "static inline __attribute__((unused)) T marea_rt_div_##NAME(T a, T b, marea_rt_trap *trap) { if (b == (T)0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return (T)0; } return (T)(a / b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_rem_##NAME(T a, T b, marea_rt_trap *trap) { if (b == (T)0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return (T)0; } return (T)(a % b); }\n"
    "MAREA_RT_UDIV(u8,uint8_t)\n"
    "MAREA_RT_UDIV(u16,uint16_t)\n"
    "MAREA_RT_UDIV(u32,uint32_t)\n"
    "MAREA_RT_UDIV(u64,uint64_t)\n"
    "#undef MAREA_RT_UDIV\n"
    "#define MAREA_RT_SSHIFT(NAME, T, U, WIDTH) \\\n"
    "static inline __attribute__((unused)) T marea_rt_shl_##NAME(T a, T b, marea_rt_trap *trap) { if ((b < (T)0) || (b >= (T)WIDTH)) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return (T)0; } return (T)(U)((U)a << (U)b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_shr_##NAME(T a, T b, marea_rt_trap *trap) { if ((b < (T)0) || (b >= (T)WIDTH)) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return (T)0; } return (T)(a >> b); }\n"
    "MAREA_RT_SSHIFT(s8,int8_t,uint8_t,8)\n"
    "MAREA_RT_SSHIFT(s16,int16_t,uint16_t,16)\n"
    "MAREA_RT_SSHIFT(s32,int32_t,uint32_t,32)\n"
    "MAREA_RT_SSHIFT(s64,int64_t,uint64_t,64)\n"
    "#undef MAREA_RT_SSHIFT\n"
    "#define MAREA_RT_USHIFT(NAME, T, WIDTH) \\\n"
    "static inline __attribute__((unused)) T marea_rt_shl_##NAME(T a, T b, marea_rt_trap *trap) { if (b >= (T)WIDTH) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return (T)0; } return (T)(a << b); } \\\n"
    "static inline __attribute__((unused)) T marea_rt_shr_##NAME(T a, T b, marea_rt_trap *trap) { if (b >= (T)WIDTH) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return (T)0; } return (T)(a >> b); }\n"
    "MAREA_RT_USHIFT(u8,uint8_t,8)\n"
    "MAREA_RT_USHIFT(u16,uint16_t,16)\n"
    "MAREA_RT_USHIFT(u32,uint32_t,32)\n"
    "MAREA_RT_USHIFT(u64,uint64_t,64)\n"
    "#undef MAREA_RT_USHIFT\n";

typedef struct MAREA_CgenContext
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
} MAREA_CgenContext;

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextCreate(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, MAREA_CgenContext **outContext)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (sink == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outContext == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_CgenContext), MAREA_ALIGNOF(MAREA_CgenContext), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_CgenContext *context = memory;

    context->arena = arena;
    context->sink = sink;

    *outContext = context;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetTypeRegistry(MAREA_CgenContext *context, const MAREA_TypeRegistry *registry)
{
    MAREA_IGNORE(context);

    MAREA_IGNORE(registry);

    return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
}

const MAREA_TypeRegistry *MAREA_CALL MAREA_CgenContextGetTypeRegistry(const MAREA_CgenContext *context)
{
    MAREA_IGNORE(context);

    return MAREA_NULLPTR;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetHostResolver(MAREA_CgenContext *context, MAREA_CgenHostResolveFunction resolve, MAREA_Null userData)
{
    MAREA_IGNORE(context);

    MAREA_IGNORE(resolve);

    MAREA_IGNORE(userData);

    return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextAddInclude(MAREA_CgenContext *context, const MAREA_Character8 *header, MAREA_IntegerSize length)
{
    MAREA_IGNORE(context);

    MAREA_IGNORE(header);

    MAREA_IGNORE(length);

    return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenContextSetAllocator(MAREA_CgenContext *context, const MAREA_Character8 *allocate, MAREA_IntegerSize allocateLength, const MAREA_Character8 *release, MAREA_IntegerSize releaseLength)
{
    MAREA_IGNORE(context);

    MAREA_IGNORE(allocate);

    MAREA_IGNORE(allocateLength);

    MAREA_IGNORE(release);

    MAREA_IGNORE(releaseLength);

    return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
}

MAREA_Arena *MAREA_CALL MAREA_CgenContextGetArena(const MAREA_CgenContext *context)
{
    return (context == MAREA_NULLPTR) ? MAREA_NULLPTR : context->arena;
}

MAREA_DiagnosticSink *MAREA_CALL MAREA_CgenContextGetDiagnostics(const MAREA_CgenContext *context)
{
    return (context == MAREA_NULLPTR) ? MAREA_NULLPTR : context->sink;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_Collect(MAREA_CgenGenerator *generator, const MAREA_IrNode *module)
{
    const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(module);

    MAREA_IntegerSize functionCount = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(module, index, &child));

        if (MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_FUNCTION_DECLARATION) {
            functionCount = (functionCount + MAREA_INTEGER_SIZE(1));
        }
    }

    if (functionCount == MAREA_INTEGER_SIZE(0)) {
        return MAREA_CGEN_EXCEPT_SUCCESS;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(generator->arena, (functionCount * sizeof(MAREA_CgenFunction)), MAREA_ALIGNOF(MAREA_CgenFunction), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    generator->functions = memory;

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_IrNode *child = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(module, index, &child));

        if (MAREA_IrNodeGetKind(child) != MAREA_IR_KIND_FUNCTION_DECLARATION) {
            continue;
        }

        const MAREA_IrNode *parameterList = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(child, MAREA_INTEGER_SIZE(0), &parameterList));

        const MAREA_IntegerSize PARAMETER_COUNT = (parameterList == MAREA_NULLPTR) ? MAREA_INTEGER_SIZE(0) : MAREA_IrNodeGetChildCount(parameterList);

        MAREA_Uinteger8 *parameterKinds = MAREA_NULLPTR;

        if (PARAMETER_COUNT > MAREA_INTEGER_SIZE(0)) {
            MAREA_Null kindMemory = MAREA_NULLPTR;

            if (MAREA_ArenaAllocate(generator->arena, (PARAMETER_COUNT * sizeof(MAREA_Uinteger8)), MAREA_ALIGNOF(MAREA_Uinteger8), &kindMemory) != MAREA_ARENA_EXCEPT_SUCCESS) {
                return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
            }

            parameterKinds = kindMemory;

            for (MAREA_IntegerSize parameterIndex = MAREA_INTEGER_SIZE(0); parameterIndex < PARAMETER_COUNT; parameterIndex = (parameterIndex + MAREA_INTEGER_SIZE(1))) {
                const MAREA_IrNode *parameter = MAREA_NULLPTR;

                MAREA_IGNORE(MAREA_IrNodeGetChildAt(parameterList, parameterIndex, &parameter));

                parameterKinds[parameterIndex] = MAREA_CgenKindOfNode(parameter);
            }
        }

        generator->functions[generator->functionCount].node = child;
        generator->functions[generator->functionCount].name = MAREA_IrNodeGetText(child);
        generator->functions[generator->functionCount].nameLength = MAREA_IrNodeGetTextLength(child);
        generator->functions[generator->functionCount].parameterKinds = parameterKinds;
        generator->functions[generator->functionCount].parameterCount = (MAREA_Uinteger32)PARAMETER_COUNT;

        const MAREA_Character8 *resolved = MAREA_IrNodeGetResolvedType(child);

        generator->functions[generator->functionCount].resultKind = (resolved == MAREA_NULLPTR) ? MAREA_CGEN_KIND_VOID : MAREA_CgenKindOfNode(child);

        generator->functionCount = (generator->functionCount + MAREA_INTEGER_SIZE(1));
    }

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteSignature(MAREA_CgenGenerator *generator, const MAREA_CgenFunction *entry)
{
    const MAREA_IrNode *parameterList = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(entry->node, MAREA_INTEGER_SIZE(0), &parameterList));

    MAREA_CgenWriteCType(generator->buffer, entry->resultKind);
    MAREA_CgenWriteText(generator->buffer, " ");
    MAREA_CgenWriteMangled(generator->buffer, "f_", entry->name, entry->nameLength);
    MAREA_CgenWriteText(generator->buffer, "(");

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < entry->parameterCount; index = (index + MAREA_UINTEGER_32(1))) {
        const MAREA_IrNode *parameter = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(parameterList, index, &parameter));

        MAREA_CgenWriteCType(generator->buffer, entry->parameterKinds[index]);
        MAREA_CgenWriteText(generator->buffer, " ");
        MAREA_CgenWriteMangled(generator->buffer, "v_", MAREA_IrNodeGetText(parameter), MAREA_IrNodeGetTextLength(parameter));
        MAREA_CgenWriteText(generator->buffer, ",");
    }

    MAREA_CgenWriteText(generator->buffer, "marea_rt_trap *marea_trap)");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

static MAREA_CgenExcept MAREA_CALL MAREA_G_WriteFunction(MAREA_CgenGenerator *generator, const MAREA_CgenFunction *entry)
{
    generator->functionFailed = MAREA_BOOLEAN_FALSE;
    generator->symbolCount = MAREA_INTEGER_SIZE(0);
    generator->currentResultKind = entry->resultKind;

    if ((entry->resultKind >= MAREA_CGEN_KIND_NUMBER_COUNT) && (entry->resultKind != MAREA_CGEN_KIND_BOOLEAN) && (entry->resultKind != MAREA_CGEN_KIND_VOID)) {
        return MAREA_CgenGeneratorFail(generator, entry->node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A function has a result type that is not a scalar.");
    }

    MAREA_CGEN_TRY(MAREA_G_WriteSignature(generator, entry));

    const MAREA_IrNode *parameterList = MAREA_NULLPTR;
    const MAREA_IrNode *body = MAREA_NULLPTR;

    MAREA_IGNORE(MAREA_IrNodeGetChildAt(entry->node, MAREA_INTEGER_SIZE(0), &parameterList));
    MAREA_IGNORE(MAREA_IrNodeGetChildAt(entry->node, MAREA_INTEGER_SIZE(1), &body));

    MAREA_CgenWriteText(generator->buffer, "{\n");

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < entry->parameterCount; index = (index + MAREA_UINTEGER_32(1))) {
        if ((entry->parameterKinds[index] >= MAREA_CGEN_KIND_NUMBER_COUNT) && (entry->parameterKinds[index] != MAREA_CGEN_KIND_BOOLEAN)) {
            MAREA_CGEN_TRY(MAREA_CgenGeneratorFail(generator, entry->node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, "A parameter has a type that is not a scalar."));

            continue;
        }

        const MAREA_IrNode *parameter = MAREA_NULLPTR;

        MAREA_IGNORE(MAREA_IrNodeGetChildAt(parameterList, index, &parameter));

        MAREA_CGEN_TRY(MAREA_CgenPushSymbol(generator, MAREA_IrNodeGetText(parameter), MAREA_IrNodeGetTextLength(parameter), entry->parameterKinds[index]));
    }

    if ((body == MAREA_NULLPTR) || (MAREA_IrNodeGetKind(body) != MAREA_IR_KIND_BLOCK)) {
        MAREA_CGEN_TRY(MAREA_CgenGeneratorFail(generator, entry->node, MAREA_CGEN_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, "A function has no body."));
    } else {
        const MAREA_IntegerSize COUNT = MAREA_IrNodeGetChildCount(body);

        for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < COUNT; index = (index + MAREA_INTEGER_SIZE(1))) {
            const MAREA_IrNode *statement = MAREA_NULLPTR;

            MAREA_IGNORE(MAREA_IrNodeGetChildAt(body, index, &statement));

            MAREA_CGEN_TRY(MAREA_CgenWriteStatement(generator, statement));

            if (generator->functionFailed == MAREA_BOOLEAN_TRUE) {
                break;
            }

            const MAREA_IrKind KIND = MAREA_IrNodeGetKind(statement);

            if ((KIND == MAREA_IR_KIND_LOCAL_DECLARATION) || (KIND == MAREA_IR_KIND_ASSIGNMENT) || (KIND == MAREA_IR_KIND_EXPRESSION_STATEMENT) || ((KIND == MAREA_IR_KIND_RETURN_STATEMENT) && (MAREA_IrNodeGetChildCount(statement) > MAREA_INTEGER_SIZE(0)))) {
                MAREA_CgenWriteTrapGuard(generator);
            }
        }
    }

    // INFO: a trailing fallback return keeps -Wreturn-type quiet; sema already requires every path of a
    // function with a result type to return, so this line is unreachable in a well-typed unit.
    if (entry->resultKind != MAREA_CGEN_KIND_VOID) {
        MAREA_CgenWriteText(generator->buffer, "return ");
        MAREA_CgenWriteDefaultValue(generator->buffer, entry->resultKind);
        MAREA_CgenWriteText(generator->buffer, ";\n");
    }

    MAREA_CgenWriteText(generator->buffer, "}\n");

    return MAREA_CGEN_EXCEPT_SUCCESS;
}

MAREA_CgenExcept MAREA_CALL MAREA_CgenGenerateModule(MAREA_CgenContext *context, const MAREA_IrNode *module, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (module == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outText == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outLength == MAREA_NULLPTR) {
        return MAREA_CGEN_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_IrNodeGetKind(module) != MAREA_IR_KIND_MODULE) {
        return MAREA_CGEN_EXCEPT_ERROR_BAD_ROOT;
    }

    MAREA_CgenBuffer buffer = { .arena = context->arena, .data = MAREA_NULLPTR, .length = MAREA_INTEGER_SIZE(0), .capacity = MAREA_INTEGER_SIZE(0), .failed = MAREA_BOOLEAN_FALSE };

    MAREA_CgenGenerator generator = {
        .context = context,
        .arena = context->arena,
        .sink = context->sink,
        .buffer = &buffer,
        .namespaceText = MAREA_IrNodeGetText(module),
        .namespaceLength = MAREA_IrNodeGetTextLength(module),
        .functions = MAREA_NULLPTR,
        .functionCount = MAREA_INTEGER_SIZE(0),
        .symbols = MAREA_NULLPTR,
        .symbolCount = MAREA_INTEGER_SIZE(0),
        .symbolCapacity = MAREA_INTEGER_SIZE(0),
        .currentResultKind = MAREA_CGEN_KIND_VOID,
        .functionFailed = MAREA_BOOLEAN_FALSE,
    };

    MAREA_CGEN_TRY(MAREA_G_Collect(&generator, module));

    MAREA_CgenWrite(&buffer, MAREA_G_PREAMBLE, (MAREA_IntegerSize)(sizeof(MAREA_G_PREAMBLE) - MAREA_INTEGER_SIZE(1)));

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator.functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_G_WriteSignature(&generator, &generator.functions[index]));

        MAREA_CgenWriteText(&buffer, ";\n");
    }

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < generator.functionCount; index = (index + MAREA_INTEGER_SIZE(1))) {
        MAREA_CGEN_TRY(MAREA_G_WriteFunction(&generator, &generator.functions[index]));
    }

    if (buffer.failed == MAREA_BOOLEAN_TRUE) {
        return MAREA_CGEN_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    buffer.data[buffer.length] = '\0';

    *outText = buffer.data;
    *outLength = buffer.length;

    return MAREA_CGEN_EXCEPT_SUCCESS;
}
