// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/cgen/MAREACgenInternal.h"

#include <string.h>

// INFO: the runtime of the generated program. It gives each operation the outcome of the vm (docs/vm/marea-vm.md) with no C undefined behavior: wrapping arithmetic through uint64_t, the same traps and the same trap codes. It is split into parts so that no string literal passes the length that C11 asks a compiler to take.
static const MAREA_Character8 MAREA_G_INCLUDES[] =
    "#include <stdint.h>\n"
    "#include <stdbool.h>\n"
    "#include <stddef.h>\n"
    "#include <string.h>\n";

static const MAREA_Character8 MAREA_G_TRAPS[] =
    "typedef enum marea_rt_trap {\n"
    "    MAREA_RT_TRAP_NONE = 0,\n"
    "    MAREA_RT_TRAP_DIVISION_BY_ZERO = 1,\n"
    "    MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE = 2,\n"
    "    MAREA_RT_TRAP_NEGATIVE_EXPONENT = 3,\n"
    "    MAREA_RT_TRAP_CONVERT_OUT_OF_RANGE = 4,\n"
    "    MAREA_RT_TRAP_UNREACHABLE = 5,\n"
    "    MAREA_RT_TRAP_MISSING_RETURN = 6,\n"
    "    MAREA_RT_TRAP_STACK_OVERFLOW = 7,\n"
    "    MAREA_RT_TRAP_INDEX_OUT_OF_BOUNDS = 8,\n"
    "    MAREA_RT_TRAP_OUT_OF_MEMORY = 9,\n"
    "    MAREA_RT_TRAP_HOST_FUNCTION_MISSING = 10,\n"
    "    MAREA_RT_TRAP_INVALID_PARTITION_COUNT = 11\n"
    "} marea_rt_trap;\n"
    "#define MAREA_RT_HELPER static inline __attribute__((unused))\n";

static const MAREA_Character8 MAREA_G_COMPARISONS[] =
    "#define MAREA_RT_ORDER(NAME, T) \\\n"
    "MAREA_RT_HELPER bool marea_rt_eq_##NAME(T a, T b) { return a == b; } \\\n"
    "MAREA_RT_HELPER bool marea_rt_ne_##NAME(T a, T b) { return a != b; } \\\n"
    "MAREA_RT_HELPER bool marea_rt_lt_##NAME(T a, T b) { return a < b; } \\\n"
    "MAREA_RT_HELPER bool marea_rt_le_##NAME(T a, T b) { return a <= b; } \\\n"
    "MAREA_RT_HELPER bool marea_rt_gt_##NAME(T a, T b) { return a > b; } \\\n"
    "MAREA_RT_HELPER bool marea_rt_ge_##NAME(T a, T b) { return a >= b; }\n"
    "MAREA_RT_ORDER(s8, int8_t)\n"
    "MAREA_RT_ORDER(s16, int16_t)\n"
    "MAREA_RT_ORDER(s32, int32_t)\n"
    "MAREA_RT_ORDER(s64, int64_t)\n"
    "MAREA_RT_ORDER(u8, uint8_t)\n"
    "MAREA_RT_ORDER(u16, uint16_t)\n"
    "MAREA_RT_ORDER(u32, uint32_t)\n"
    "MAREA_RT_ORDER(u64, uint64_t)\n"
    "MAREA_RT_ORDER(f32, float)\n"
    "MAREA_RT_ORDER(f64, double)\n"
    "#undef MAREA_RT_ORDER\n"
    "MAREA_RT_HELPER bool marea_rt_eq_bool(bool a, bool b) { return a == b; }\n"
    "MAREA_RT_HELPER bool marea_rt_ne_bool(bool a, bool b) { return a != b; }\n";

static const MAREA_Character8 MAREA_G_WRAPPING[] =
    "#define MAREA_RT_WRAP(NAME, T) \\\n"
    "MAREA_RT_HELPER T marea_rt_add_##NAME(T a, T b) { return (T)((uint64_t)a + (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_sub_##NAME(T a, T b) { return (T)((uint64_t)a - (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_mul_##NAME(T a, T b) { return (T)((uint64_t)a * (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_neg_##NAME(T a) { return (T)((uint64_t)0 - (uint64_t)a); } \\\n"
    "MAREA_RT_HELPER T marea_rt_not_##NAME(T a) { return (T)(~(uint64_t)a); } \\\n"
    "MAREA_RT_HELPER T marea_rt_and_##NAME(T a, T b) { return (T)((uint64_t)a & (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_or_##NAME(T a, T b) { return (T)((uint64_t)a | (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_xor_##NAME(T a, T b) { return (T)((uint64_t)a ^ (uint64_t)b); }\n"
    "MAREA_RT_WRAP(s8, int8_t)\n"
    "MAREA_RT_WRAP(s16, int16_t)\n"
    "MAREA_RT_WRAP(s32, int32_t)\n"
    "MAREA_RT_WRAP(s64, int64_t)\n"
    "MAREA_RT_WRAP(u8, uint8_t)\n"
    "MAREA_RT_WRAP(u16, uint16_t)\n"
    "MAREA_RT_WRAP(u32, uint32_t)\n"
    "MAREA_RT_WRAP(u64, uint64_t)\n"
    "#undef MAREA_RT_WRAP\n"
    "MAREA_RT_HELPER uint64_t marea_rt_power(uint64_t base, uint64_t exponent) {\n"
    "    uint64_t result = 1u;\n"
    "    while (exponent > 0u) {\n"
    "        if ((exponent & 1u) != 0u) { result *= base; }\n"
    "        base *= base;\n"
    "        exponent >>= 1u;\n"
    "    }\n"
    "    return result;\n"
    "}\n";

static const MAREA_Character8 MAREA_G_SIGNED[] =
    "#define MAREA_RT_SIGNED(NAME, T, LOWEST, HIGHEST) \\\n"
    "MAREA_RT_HELPER T marea_rt_div_##NAME(T a, T b, marea_rt_trap *trap) { if (b == 0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return 0; } if (b == -1) { return marea_rt_neg_##NAME(a); } return (T)(a / b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_rem_##NAME(T a, T b, marea_rt_trap *trap) { if (b == 0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return 0; } if (b == -1) { return 0; } return (T)(a % b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_pow_##NAME(T a, T b, marea_rt_trap *trap) { if (b < 0) { *trap = MAREA_RT_TRAP_NEGATIVE_EXPONENT; return 0; } return (T)marea_rt_power((uint64_t)a, (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_shl_##NAME(T a, uint64_t count, marea_rt_trap *trap) { if (count >= (sizeof(T) * 8u)) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return 0; } return (T)((uint64_t)a << count); } \\\n"
    "MAREA_RT_HELPER T marea_rt_shr_##NAME(T a, uint64_t count, marea_rt_trap *trap) { if (count >= (sizeof(T) * 8u)) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return 0; } return (T)((int64_t)a >> count); } \\\n"
    "MAREA_RT_HELPER T marea_rt_sadd_##NAME(T a, T b) { T r; if (__builtin_add_overflow(a, b, &r)) { return (b > 0) ? HIGHEST : LOWEST; } return r; } \\\n"
    "MAREA_RT_HELPER T marea_rt_ssub_##NAME(T a, T b) { T r; if (__builtin_sub_overflow(a, b, &r)) { return (b < 0) ? HIGHEST : LOWEST; } return r; } \\\n"
    "MAREA_RT_HELPER T marea_rt_smul_##NAME(T a, T b) { T r; if (__builtin_mul_overflow(a, b, &r)) { return ((a < 0) != (b < 0)) ? LOWEST : HIGHEST; } return r; }\n"
    "MAREA_RT_SIGNED(s8, int8_t, INT8_MIN, INT8_MAX)\n"
    "MAREA_RT_SIGNED(s16, int16_t, INT16_MIN, INT16_MAX)\n"
    "MAREA_RT_SIGNED(s32, int32_t, INT32_MIN, INT32_MAX)\n"
    "MAREA_RT_SIGNED(s64, int64_t, INT64_MIN, INT64_MAX)\n"
    "#undef MAREA_RT_SIGNED\n";

static const MAREA_Character8 MAREA_G_UNSIGNED[] =
    "#define MAREA_RT_UNSIGNED(NAME, T, HIGHEST) \\\n"
    "MAREA_RT_HELPER T marea_rt_div_##NAME(T a, T b, marea_rt_trap *trap) { if (b == 0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return 0; } return (T)(a / b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_rem_##NAME(T a, T b, marea_rt_trap *trap) { if (b == 0) { *trap = MAREA_RT_TRAP_DIVISION_BY_ZERO; return 0; } return (T)(a % b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_pow_##NAME(T a, T b, marea_rt_trap *trap) { (void)trap; return (T)marea_rt_power((uint64_t)a, (uint64_t)b); } \\\n"
    "MAREA_RT_HELPER T marea_rt_shl_##NAME(T a, uint64_t count, marea_rt_trap *trap) { if (count >= (sizeof(T) * 8u)) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return 0; } return (T)((uint64_t)a << count); } \\\n"
    "MAREA_RT_HELPER T marea_rt_shr_##NAME(T a, uint64_t count, marea_rt_trap *trap) { if (count >= (sizeof(T) * 8u)) { *trap = MAREA_RT_TRAP_SHIFT_OUT_OF_RANGE; return 0; } return (T)((uint64_t)a >> count); } \\\n"
    "MAREA_RT_HELPER T marea_rt_sadd_##NAME(T a, T b) { T r; if (__builtin_add_overflow(a, b, &r)) { return HIGHEST; } return r; } \\\n"
    "MAREA_RT_HELPER T marea_rt_ssub_##NAME(T a, T b) { T r; if (__builtin_sub_overflow(a, b, &r)) { return 0; } return r; } \\\n"
    "MAREA_RT_HELPER T marea_rt_smul_##NAME(T a, T b) { T r; if (__builtin_mul_overflow(a, b, &r)) { return HIGHEST; } return r; }\n"
    "MAREA_RT_UNSIGNED(u8, uint8_t, UINT8_MAX)\n"
    "MAREA_RT_UNSIGNED(u16, uint16_t, UINT16_MAX)\n"
    "MAREA_RT_UNSIGNED(u32, uint32_t, UINT32_MAX)\n"
    "MAREA_RT_UNSIGNED(u64, uint64_t, UINT64_MAX)\n"
    "#undef MAREA_RT_UNSIGNED\n";

static const MAREA_Character8 MAREA_G_CONVERT_SIGNED[] =
    "#define MAREA_RT_OUT_OF_RANGE(CONDITION) if (CONDITION) { *trap = MAREA_RT_TRAP_CONVERT_OUT_OF_RANGE; return 0; }\n"
    "#define MAREA_RT_TO_SIGNED(NAME, T, LOWEST, HIGHEST) \\\n"
    "MAREA_RT_HELPER T marea_rt_si_##NAME(int64_t v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE((v < (int64_t)LOWEST) || (v > (int64_t)HIGHEST)) return (T)v; } \\\n"
    "MAREA_RT_HELPER T marea_rt_ui_##NAME(uint64_t v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(v > (uint64_t)HIGHEST) return (T)v; } \\\n"
    "MAREA_RT_HELPER T marea_rt_fi_##NAME(double v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(__builtin_isnan(v) || (v < (double)LOWEST) || (v > (double)HIGHEST)) return (T)v; }\n"
    "MAREA_RT_TO_SIGNED(s8, int8_t, INT8_MIN, INT8_MAX)\n"
    "MAREA_RT_TO_SIGNED(s16, int16_t, INT16_MIN, INT16_MAX)\n"
    "MAREA_RT_TO_SIGNED(s32, int32_t, INT32_MIN, INT32_MAX)\n"
    "#undef MAREA_RT_TO_SIGNED\n"
    "MAREA_RT_HELPER int64_t marea_rt_si_s64(int64_t v, marea_rt_trap *trap) { (void)trap; return v; }\n"
    "MAREA_RT_HELPER int64_t marea_rt_ui_s64(uint64_t v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(v > (uint64_t)INT64_MAX) return (int64_t)v; }\n"
    "MAREA_RT_HELPER int64_t marea_rt_fi_s64(double v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(__builtin_isnan(v) || (v < -0x1p+63) || (v >= 0x1p+63)) return (int64_t)v; }\n";

static const MAREA_Character8 MAREA_G_CONVERT_UNSIGNED[] =
    "#define MAREA_RT_TO_UNSIGNED(NAME, T, HIGHEST) \\\n"
    "MAREA_RT_HELPER T marea_rt_si_##NAME(int64_t v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE((v < 0) || ((uint64_t)v > (uint64_t)HIGHEST)) return (T)v; } \\\n"
    "MAREA_RT_HELPER T marea_rt_ui_##NAME(uint64_t v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(v > (uint64_t)HIGHEST) return (T)v; } \\\n"
    "MAREA_RT_HELPER T marea_rt_fi_##NAME(double v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(__builtin_isnan(v) || (v < 0.0) || (v > (double)HIGHEST)) return (T)v; }\n"
    "MAREA_RT_TO_UNSIGNED(u8, uint8_t, UINT8_MAX)\n"
    "MAREA_RT_TO_UNSIGNED(u16, uint16_t, UINT16_MAX)\n"
    "MAREA_RT_TO_UNSIGNED(u32, uint32_t, UINT32_MAX)\n"
    "#undef MAREA_RT_TO_UNSIGNED\n"
    "MAREA_RT_HELPER uint64_t marea_rt_si_u64(int64_t v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(v < 0) return (uint64_t)v; }\n"
    "MAREA_RT_HELPER uint64_t marea_rt_ui_u64(uint64_t v, marea_rt_trap *trap) { (void)trap; return v; }\n"
    "MAREA_RT_HELPER uint64_t marea_rt_fi_u64(double v, marea_rt_trap *trap) { MAREA_RT_OUT_OF_RANGE(__builtin_isnan(v) || (v < 0.0) || (v >= 0x1p+64)) return (uint64_t)v; }\n"
    "#undef MAREA_RT_OUT_OF_RANGE\n";

void MAREA_CALL MAREA_CgenWritePreamble(MAREA_CgenBuffer *buffer, MAREA_Boolean math, MAREA_Boolean packed)
{
    MAREA_CgenWriteText(buffer, MAREA_G_INCLUDES);

    if (math == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(buffer, "#include <math.h>\n");
    }

    // INFO: a packed type reads and writes its members in place, and cgen never passes the address of one on.
    if (packed == MAREA_BOOLEAN_TRUE) {
        MAREA_CgenWriteText(buffer, "#pragma GCC diagnostic ignored \"-Waddress-of-packed-member\"\n#if !defined(__clang__)\n#pragma GCC diagnostic ignored \"-Wpacked-not-aligned\"\n#endif\n");
    }

    MAREA_CgenWriteText(buffer, MAREA_G_TRAPS);
    MAREA_CgenWriteText(buffer, MAREA_G_COMPARISONS);
    MAREA_CgenWriteText(buffer, MAREA_G_WRAPPING);
    MAREA_CgenWriteText(buffer, MAREA_G_SIGNED);
    MAREA_CgenWriteText(buffer, MAREA_G_UNSIGNED);
    MAREA_CgenWriteText(buffer, MAREA_G_CONVERT_SIGNED);
    MAREA_CgenWriteText(buffer, MAREA_G_CONVERT_UNSIGNED);
}

void MAREA_CALL MAREA_CgenWriteZeroReturn(MAREA_CgenGenerator *generator)
{
    const MAREA_Uinteger32 RESULT = ((generator->current == MAREA_NULLPTR) ? (MAREA_Uinteger32)MAREA_CGEN_KIND_VOID : generator->current->result);

    if (RESULT == MAREA_CGEN_KIND_VOID) {
        MAREA_CgenWriteText(generator->buffer, "return;\n");
    } else if (MAREA_CgenIsAggregate(generator, RESULT) == MAREA_BOOLEAN_TRUE) {
        MAREA_CGEN_WRITE(generator->buffer, "{\n", MAREA_CgenTypeName(generator, RESULT), " marea_none;\nmemset(&marea_none, 0, sizeof(marea_none));\nreturn marea_none;\n}\n");
    } else {
        MAREA_CgenWriteText(generator->buffer, "return 0;\n");
    }
}

void MAREA_CALL MAREA_CgenWriteTrapGuard(MAREA_CgenGenerator *generator)
{
    MAREA_CgenWriteText(generator->buffer, "if (*marea_trap != MAREA_RT_TRAP_NONE) {\n");
    MAREA_CgenWriteZeroReturn(generator);
    MAREA_CgenWriteText(generator->buffer, "}\n");
}

void MAREA_CALL MAREA_CgenWriteTrap(MAREA_CgenGenerator *generator, const MAREA_Character8 *trap)
{
    MAREA_CGEN_WRITE(generator->buffer, "*marea_trap = ", trap, ";\n");
    MAREA_CgenWriteZeroReturn(generator);
}

static void MAREA_CALL MAREA_G_SetStable(MAREA_CgenValue *outValue, const MAREA_Character8 *text, MAREA_Uinteger32 type)
{
    outValue->text = text;
    outValue->type = type;
    outValue->stable = MAREA_BOOLEAN_TRUE;
    outValue->unaligned = MAREA_BOOLEAN_FALSE;
}

void MAREA_CALL MAREA_CgenBind(MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, const MAREA_Character8 *expression, MAREA_CgenValue *outValue)
{
    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, MAREA_CgenTypeName(generator, type), " ", name, " = ", expression, ";\n");

    MAREA_G_SetStable(outValue, name, type);
}

void MAREA_CALL MAREA_CgenZeroTemporary(MAREA_CgenGenerator *generator, MAREA_Uinteger32 type, MAREA_CgenValue *outValue)
{
    if (MAREA_CgenIsAggregate(generator, type) == MAREA_BOOLEAN_FALSE) {
        MAREA_CgenBind(generator, type, "0", outValue);

        return;
    }

    const MAREA_Character8 *name = MAREA_CgenTemporaryName(generator);

    MAREA_CGEN_WRITE(generator->buffer, MAREA_CgenTypeName(generator, type), " ", name, ";\nmemset(&", name, ", 0, sizeof(", name, "));\n");

    MAREA_G_SetStable(outValue, name, type);
}

// INFO: C copies a structure by assignment, also from a member of a packed one, and assigning storage to itself would only raise a warning.
void MAREA_CALL MAREA_CgenStore(MAREA_CgenGenerator *generator, const MAREA_Character8 *target, MAREA_Uinteger32 type, const MAREA_Character8 *value)
{
    MAREA_IGNORE(type);

    if (MAREA_CgenSameText(target, (MAREA_IntegerSize)strlen(target), value, (MAREA_IntegerSize)strlen(value)) == MAREA_BOOLEAN_TRUE) {
        return;
    }

    MAREA_CGEN_WRITE(generator->buffer, target, " = ", value, ";\n");
}

void MAREA_CALL MAREA_CgenSnapshot(MAREA_CgenGenerator *generator, MAREA_CgenValue *ioValue)
{
    MAREA_CgenBind(generator, ioValue->type, ioValue->text, ioValue);
}
