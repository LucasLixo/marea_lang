// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/vm/MAREAVmInternal.h"

#include <math.h>
#include <string.h>

// INFO: a register holds 64 bits with no tag; the opcode and its kind operand say how to read it. Every helper
// below works on the raw 64 bits so a width narrower than 64 stays normalized the way MAREA_BytecodeGenerate
// already stores it (sign extended for signed, zero extended for unsigned, a float32 as a float64 bit pattern).

static MAREA_Boolean MAREA_CALL MAREA_G_VmIsFloatKind(MAREA_Uinteger8 kind)
{
    return ((kind == MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) || (kind == MAREA_BYTECODE_NUMBER_KIND_FLOAT_64));
}

static MAREA_Boolean MAREA_CALL MAREA_G_VmIsSignedKind(MAREA_Uinteger8 kind)
{
    return (kind < MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8);
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmNormalize(MAREA_Uinteger8 kind, MAREA_Uinteger64 bits)
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

static MAREA_Float64 MAREA_CALL MAREA_G_VmAsDouble(MAREA_Uinteger64 bits)
{
    MAREA_Float64 value = MAREA_FLOAT_64(0.0);

    memcpy(&value, &bits, sizeof(value));

    return value;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmFromDouble(MAREA_Uinteger8 kind, MAREA_Float64 value)
{
    MAREA_Float64 stored = value;

    if (kind == MAREA_BYTECODE_NUMBER_KIND_FLOAT_32) {
        stored = (MAREA_Float64)((MAREA_Float32)value);
    }

    MAREA_Uinteger64 bits = MAREA_UINTEGER_64(0);

    memcpy(&bits, &stored, sizeof(bits));

    return bits;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmKindMinimum(MAREA_Uinteger8 kind)
{
    switch (kind) {
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_8:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)MAREA_SINTEGER_8_MINIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_16:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)MAREA_SINTEGER_16_MINIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)MAREA_SINTEGER_32_MINIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64:
        return (MAREA_Uinteger64)MAREA_SINTEGER_64_MINIMUM;
    default:
        return MAREA_UINTEGER_64(0);
    }
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmKindMaximum(MAREA_Uinteger8 kind)
{
    switch (kind) {
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_8:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)MAREA_SINTEGER_8_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_16:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)MAREA_SINTEGER_16_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_32:
        return (MAREA_Uinteger64)(MAREA_Sinteger64)MAREA_SINTEGER_32_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64:
        return (MAREA_Uinteger64)MAREA_SINTEGER_64_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_8:
        return (MAREA_Uinteger64)MAREA_UINTEGER_8_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_16:
        return (MAREA_Uinteger64)MAREA_UINTEGER_16_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_32:
        return (MAREA_Uinteger64)MAREA_UINTEGER_32_MAXIMUM;
    case MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64:
    default:
        return MAREA_UINTEGER_64_MAXIMUM;
    }
}

// INFO: whether the integer bits of sourceKind, read the way normalize already stored them, hold a value the
// target integer kind can represent exactly. convert traps when this is false, instead of silently truncating.
static MAREA_Boolean MAREA_CALL MAREA_G_VmIntegerFits(MAREA_Uinteger8 sourceKind, MAREA_Uinteger8 targetKind, MAREA_Uinteger64 bits)
{
    const MAREA_Uinteger64 TARGET_MAXIMUM = MAREA_G_VmKindMaximum(targetKind);

    if (MAREA_G_VmIsSignedKind(targetKind)) {
        const MAREA_Sinteger64 TARGET_MINIMUM_SIGNED = (MAREA_Sinteger64)MAREA_G_VmKindMinimum(targetKind);
        const MAREA_Sinteger64 TARGET_MAXIMUM_SIGNED = (MAREA_Sinteger64)TARGET_MAXIMUM;

        if (MAREA_G_VmIsSignedKind(sourceKind)) {
            const MAREA_Sinteger64 VALUE = (MAREA_Sinteger64)bits;

            return ((VALUE >= TARGET_MINIMUM_SIGNED) && (VALUE <= TARGET_MAXIMUM_SIGNED));
        }

        return (bits <= (MAREA_Uinteger64)TARGET_MAXIMUM_SIGNED);
    }

    if (MAREA_G_VmIsSignedKind(sourceKind)) {
        const MAREA_Sinteger64 VALUE = (MAREA_Sinteger64)bits;

        return ((VALUE >= MAREA_SINTEGER_64(0)) && ((MAREA_Uinteger64)VALUE <= TARGET_MAXIMUM));
    }

    return (bits <= TARGET_MAXIMUM);
}

// INFO: a portable, UB-free 64x64 -> 128-bit unsigned multiply (schoolbook, 32-bit halves), used only to decide a
// saturating multiply of two sinteger64 or uinteger64 operands, the one case a width-promoted multiply cannot
// detect by staying inside a 64-bit integer.
static void MAREA_CALL MAREA_G_VmMultiplyWide(MAREA_Uinteger64 a, MAREA_Uinteger64 b, MAREA_Uinteger64 *outHigh, MAREA_Uinteger64 *outLow)
{
    const MAREA_Uinteger64 MASK = MAREA_UINTEGER_64(0xFFFFFFFF);
    const MAREA_Uinteger64 A0 = (a & MASK);
    const MAREA_Uinteger64 A1 = (a >> MAREA_UINTEGER_32(32));
    const MAREA_Uinteger64 B0 = (b & MASK);
    const MAREA_Uinteger64 B1 = (b >> MAREA_UINTEGER_32(32));

    const MAREA_Uinteger64 LOW_LOW = (A0 * B0);
    const MAREA_Uinteger64 HIGH_LOW = (A1 * B0);
    const MAREA_Uinteger64 LOW_HIGH = (A0 * B1);
    const MAREA_Uinteger64 HIGH_HIGH = (A1 * B1);

    const MAREA_Uinteger64 CROSS = ((LOW_LOW >> MAREA_UINTEGER_32(32)) + (HIGH_LOW & MASK) + (LOW_HIGH & MASK));

    *outLow = ((LOW_LOW & MASK) | ((CROSS & MASK) << MAREA_UINTEGER_32(32)));
    *outHigh = (HIGH_HIGH + (HIGH_LOW >> MAREA_UINTEGER_32(32)) + (LOW_HIGH >> MAREA_UINTEGER_32(32)) + (CROSS >> MAREA_UINTEGER_32(32)));
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmSaturatingMultiply(MAREA_Uinteger8 kind, MAREA_Uinteger64 a, MAREA_Uinteger64 b)
{
    if ((kind != MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64) && (kind != MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64)) {
        const MAREA_Sinteger64 WIDE_A = (MAREA_Sinteger64)a;
        const MAREA_Sinteger64 WIDE_B = (MAREA_Sinteger64)b;
        const MAREA_Sinteger64 PRODUCT = (WIDE_A * WIDE_B);
        const MAREA_Sinteger64 MINIMUM = (MAREA_Sinteger64)MAREA_G_VmKindMinimum(kind);
        const MAREA_Sinteger64 MAXIMUM = (MAREA_Sinteger64)MAREA_G_VmKindMaximum(kind);

        if (PRODUCT < MINIMUM) {
            return (MAREA_Uinteger64)MINIMUM;
        }

        if (PRODUCT > MAXIMUM) {
            return (MAREA_Uinteger64)MAXIMUM;
        }

        return (MAREA_Uinteger64)PRODUCT;
    }

    MAREA_Uinteger64 high = MAREA_UINTEGER_64(0);
    MAREA_Uinteger64 low = MAREA_UINTEGER_64(0);

    MAREA_G_VmMultiplyWide(a, b, &high, &low);

    if (kind == MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64) {
        return ((high != MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64_MAXIMUM : low);
    }

    // INFO: the unsigned 128-bit product of two bit patterns equals the signed product once the high half is
    // corrected for a negative operand (two's complement widening multiply, a well known portable technique).
    const MAREA_Boolean A_NEGATIVE = (((MAREA_Sinteger64)a) < MAREA_SINTEGER_64(0));
    const MAREA_Boolean B_NEGATIVE = (((MAREA_Sinteger64)b) < MAREA_SINTEGER_64(0));

    if (A_NEGATIVE) {
        high -= b;
    }

    if (B_NEGATIVE) {
        high -= a;
    }

    const MAREA_Boolean RESULT_NEGATIVE = (A_NEGATIVE != B_NEGATIVE);
    const MAREA_Boolean LOW_SIGN_BIT = (((MAREA_Sinteger64)low) < MAREA_SINTEGER_64(0));

    if ((!RESULT_NEGATIVE) && (high == MAREA_UINTEGER_64(0)) && (!LOW_SIGN_BIT)) {
        return low;
    }

    if (RESULT_NEGATIVE && (high == MAREA_UINTEGER_64_MAXIMUM) && LOW_SIGN_BIT) {
        return low;
    }

    return (RESULT_NEGATIVE ? MAREA_G_VmKindMinimum(kind) : MAREA_G_VmKindMaximum(kind));
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmSaturatingAdd(MAREA_Uinteger8 kind, MAREA_Uinteger64 a, MAREA_Uinteger64 b)
{
    if ((kind != MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64) && (kind != MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64)) {
        const MAREA_Sinteger64 SUM = ((MAREA_Sinteger64)a + (MAREA_Sinteger64)b);
        const MAREA_Sinteger64 MINIMUM = (MAREA_Sinteger64)MAREA_G_VmKindMinimum(kind);
        const MAREA_Sinteger64 MAXIMUM = (MAREA_Sinteger64)MAREA_G_VmKindMaximum(kind);

        if (SUM < MINIMUM) {
            return (MAREA_Uinteger64)MINIMUM;
        }

        if (SUM > MAXIMUM) {
            return (MAREA_Uinteger64)MAXIMUM;
        }

        return (MAREA_Uinteger64)SUM;
    }

    const MAREA_Uinteger64 WRAPPED = (a + b);

    if (kind == MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64) {
        return ((WRAPPED < a) ? MAREA_UINTEGER_64_MAXIMUM : WRAPPED);
    }

    const MAREA_Boolean A_NEGATIVE = (((MAREA_Sinteger64)a) < MAREA_SINTEGER_64(0));
    const MAREA_Boolean B_NEGATIVE = (((MAREA_Sinteger64)b) < MAREA_SINTEGER_64(0));
    const MAREA_Boolean SUM_NEGATIVE = (((MAREA_Sinteger64)WRAPPED) < MAREA_SINTEGER_64(0));

    if ((A_NEGATIVE == B_NEGATIVE) && (SUM_NEGATIVE != A_NEGATIVE)) {
        return (A_NEGATIVE ? MAREA_G_VmKindMinimum(kind) : MAREA_G_VmKindMaximum(kind));
    }

    return WRAPPED;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmSaturatingSubtract(MAREA_Uinteger8 kind, MAREA_Uinteger64 a, MAREA_Uinteger64 b)
{
    if ((kind != MAREA_BYTECODE_NUMBER_KIND_SINTEGER_64) && (kind != MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64)) {
        const MAREA_Sinteger64 DIFFERENCE = ((MAREA_Sinteger64)a - (MAREA_Sinteger64)b);
        const MAREA_Sinteger64 MINIMUM = (MAREA_Sinteger64)MAREA_G_VmKindMinimum(kind);
        const MAREA_Sinteger64 MAXIMUM = (MAREA_Sinteger64)MAREA_G_VmKindMaximum(kind);

        if (DIFFERENCE < MINIMUM) {
            return (MAREA_Uinteger64)MINIMUM;
        }

        if (DIFFERENCE > MAXIMUM) {
            return (MAREA_Uinteger64)MAXIMUM;
        }

        return (MAREA_Uinteger64)DIFFERENCE;
    }

    const MAREA_Uinteger64 WRAPPED = (a - b);

    if (kind == MAREA_BYTECODE_NUMBER_KIND_UINTEGER_64) {
        return ((a < b) ? MAREA_UINTEGER_64(0) : WRAPPED);
    }

    const MAREA_Boolean A_NEGATIVE = (((MAREA_Sinteger64)a) < MAREA_SINTEGER_64(0));
    const MAREA_Boolean B_NEGATIVE = (((MAREA_Sinteger64)b) < MAREA_SINTEGER_64(0));
    const MAREA_Boolean DIFFERENCE_NEGATIVE = (((MAREA_Sinteger64)WRAPPED) < MAREA_SINTEGER_64(0));

    if ((A_NEGATIVE != B_NEGATIVE) && (DIFFERENCE_NEGATIVE != A_NEGATIVE)) {
        return (A_NEGATIVE ? MAREA_G_VmKindMinimum(kind) : MAREA_G_VmKindMaximum(kind));
    }

    return WRAPPED;
}

// INFO: an integer power by repeated squaring, wrapping like a chain of plain multiplies (the opcode is not a
// saturating one), done on the magnitude so a negative signed base with an odd exponent gives a negative result.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmIntegerPower(MAREA_Boolean negativeBase, MAREA_Uinteger64 magnitude, MAREA_Uinteger64 exponent)
{
    MAREA_Uinteger64 result = MAREA_UINTEGER_64(1);
    MAREA_Uinteger64 base = magnitude;
    MAREA_Uinteger64 remaining = exponent;

    while (remaining > MAREA_UINTEGER_64(0)) {
        if ((remaining & MAREA_UINTEGER_64(1)) != MAREA_UINTEGER_64(0)) {
            result = (result * base);
        }

        base = (base * base);
        remaining >>= MAREA_UINTEGER_32(1);
    }

    if (negativeBase && ((exponent & MAREA_UINTEGER_64(1)) != MAREA_UINTEGER_64(0))) {
        return (MAREA_UINTEGER_64(0) - result);
    }

    return result;
}

typedef struct MAREA_VmFrame
{
    MAREA_Uinteger64 registers[MAREA_VM_FRAME_REGISTER_LIMIT];
} MAREA_VmFrame;

static MAREA_VmResult MAREA_CALL MAREA_G_VmTrapResult(MAREA_BytecodeTrapCode trap)
{
    MAREA_VmResult result = { 0 };

    result.value = MAREA_UINTEGER_64(0);
    result.trap = trap;
    result.trapped = MAREA_BOOLEAN_TRUE;

    return result;
}

static MAREA_VmResult MAREA_CALL MAREA_G_VmValueResult(MAREA_Uinteger64 value)
{
    MAREA_VmResult result = { 0 };

    result.value = value;
    result.trap = MAREA_BYTECODE_TRAP_CODE_NONE;
    result.trapped = MAREA_BOOLEAN_FALSE;

    return result;
}

MAREA_VmExcept MAREA_CALL MAREA_VmRun(MAREA_VmContext *context, const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_Uinteger64 *arguments, MAREA_Uinteger32 argumentCount, MAREA_Uinteger32 depth, MAREA_VmResult *outResult)
{
    // INFO: the context carries the arena and sink for a future gap (host calls, allocation); a run that stays
    // inside the scalar opcodes of this slice needs neither.
    MAREA_IGNORE(context);

    *outResult = MAREA_G_VmValueResult(MAREA_UINTEGER_64(0));

    if (depth > MAREA_VM_CALL_DEPTH_LIMIT) {
        *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_STACK_OVERFLOW);

        return MAREA_VM_EXCEPT_SUCCESS;
    }

    MAREA_BytecodeFunctionInfo info = { 0 };

    if (MAREA_BytecodeModuleGetFunction(module, functionIndex, &info) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    if (info.registerCount > MAREA_VM_FRAME_REGISTER_LIMIT) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_VmFrame frame;

    memset(frame.registers, 0, sizeof(frame.registers));

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < argumentCount; index += MAREA_UINTEGER_32(1)) {
        if (index >= MAREA_VM_FRAME_REGISTER_LIMIT) {
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        frame.registers[index] = arguments[index];
    }

    MAREA_Uinteger32 ip = MAREA_UINTEGER_32(0);

    for (;;) {
        if (ip >= info.instructionCount) {
            // INFO: control fell off the end of the code with no return, which MAREA_BytecodeGenerate never
            // emits (every path ends with return, return_void or trap), so this is a malformed module.
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        MAREA_BytecodeInstruction instruction = { 0 };

        if (MAREA_BytecodeModuleGetInstruction(module, functionIndex, ip, &instruction) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        const MAREA_Uinteger32 A = instruction.a;
        const MAREA_Uinteger32 B = instruction.b;
        const MAREA_Uinteger32 C = instruction.c;

        // INFO: a holds a register in every opcode but trap (its own trap code, always a small value), so this
        // one check covers it everywhere. b and c mean a register only for some opcodes (an offset, a data or
        // function index, a kind, or an immediate half elsewhere), so each such case checks the one it reads.
        if (A >= MAREA_VM_FRAME_REGISTER_LIMIT) {
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        MAREA_Boolean advance = MAREA_BOOLEAN_TRUE;

        switch (instruction.opcode) {
        case MAREA_BYTECODE_OPCODE_NOP:
            break;
        case MAREA_BYTECODE_OPCODE_RETURN_VOID:
            *outResult = MAREA_G_VmValueResult(MAREA_UINTEGER_64(0));

            return MAREA_VM_EXCEPT_SUCCESS;
        case MAREA_BYTECODE_OPCODE_RETURN:
            *outResult = MAREA_G_VmValueResult(frame.registers[A]);

            return MAREA_VM_EXCEPT_SUCCESS;
        case MAREA_BYTECODE_OPCODE_MOVE:
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            frame.registers[A] = frame.registers[B];
            break;
        case MAREA_BYTECODE_OPCODE_LOAD_IMMEDIATE:
        {
            const MAREA_Uinteger32 IMMEDIATE = ((B & MAREA_UINTEGER_32(0xFFFF)) | (C << MAREA_UINTEGER_32(16)));

            frame.registers[A] = (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger32)IMMEDIATE;

            break;
        }
        case MAREA_BYTECODE_OPCODE_LOAD_DATA:
        {
            const MAREA_Byte *bytes = MAREA_NULLPTR;
            MAREA_IntegerSize size = MAREA_INTEGER_SIZE(0);

            if ((MAREA_BytecodeModuleGetData(module, B, &bytes, &size) != MAREA_BYTECODE_EXCEPT_SUCCESS) || (size != MAREA_INTEGER_SIZE(8))) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            MAREA_Uinteger64 value = MAREA_UINTEGER_64(0);

            memcpy(&value, bytes, sizeof(value));

            frame.registers[A] = value;

            break;
        }
        case MAREA_BYTECODE_OPCODE_ADD:
        case MAREA_BYTECODE_OPCODE_SUB:
        case MAREA_BYTECODE_OPCODE_MUL:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 KIND = (MAREA_Uinteger8)C;

            if (MAREA_G_VmIsFloatKind(KIND)) {
                const MAREA_Float64 LEFT = MAREA_G_VmAsDouble(frame.registers[A]);
                const MAREA_Float64 RIGHT = MAREA_G_VmAsDouble(frame.registers[B]);
                MAREA_Float64 computed = MAREA_FLOAT_64(0.0);

                if (instruction.opcode == MAREA_BYTECODE_OPCODE_ADD) {
                    computed = (LEFT + RIGHT);
                } else if (instruction.opcode == MAREA_BYTECODE_OPCODE_SUB) {
                    computed = (LEFT - RIGHT);
                } else {
                    computed = (LEFT * RIGHT);
                }

                frame.registers[A] = MAREA_G_VmFromDouble(KIND, computed);
            } else {
                MAREA_Uinteger64 computed = MAREA_UINTEGER_64(0);

                if (instruction.opcode == MAREA_BYTECODE_OPCODE_ADD) {
                    computed = (frame.registers[A] + frame.registers[B]);
                } else if (instruction.opcode == MAREA_BYTECODE_OPCODE_SUB) {
                    computed = (frame.registers[A] - frame.registers[B]);
                } else {
                    computed = (frame.registers[A] * frame.registers[B]);
                }

                frame.registers[A] = MAREA_G_VmNormalize(KIND, computed);
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_DIV:
        case MAREA_BYTECODE_OPCODE_REM:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 KIND = (MAREA_Uinteger8)C;

            if (MAREA_G_VmIsFloatKind(KIND)) {
                const MAREA_Float64 LEFT = MAREA_G_VmAsDouble(frame.registers[A]);
                const MAREA_Float64 RIGHT = MAREA_G_VmAsDouble(frame.registers[B]);

                frame.registers[A] = MAREA_G_VmFromDouble(KIND, ((instruction.opcode == MAREA_BYTECODE_OPCODE_DIV) ? (LEFT / RIGHT) : fmod(LEFT, RIGHT)));
            } else if (MAREA_G_VmIsSignedKind(KIND)) {
                const MAREA_Sinteger64 LEFT = (MAREA_Sinteger64)frame.registers[A];
                const MAREA_Sinteger64 RIGHT = (MAREA_Sinteger64)frame.registers[B];

                if (RIGHT == MAREA_SINTEGER_64(0)) {
                    *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO);

                    return MAREA_VM_EXCEPT_SUCCESS;
                }

                if (instruction.opcode == MAREA_BYTECODE_OPCODE_DIV) {
                    // INFO: dividing the lowest value by -1 wraps back to the lowest value; negation through
                    // unsigned subtraction gives the same wrapped bits with no undefined behavior.
                    frame.registers[A] = MAREA_G_VmNormalize(KIND, ((RIGHT == MAREA_SINTEGER_64(-1)) ? (MAREA_UINTEGER_64(0) - (MAREA_Uinteger64)LEFT) : (MAREA_Uinteger64)(LEFT / RIGHT)));
                } else {
                    frame.registers[A] = MAREA_G_VmNormalize(KIND, ((RIGHT == MAREA_SINTEGER_64(-1)) ? MAREA_UINTEGER_64(0) : (MAREA_Uinteger64)(LEFT % RIGHT)));
                }
            } else {
                const MAREA_Uinteger64 LEFT = frame.registers[A];
                const MAREA_Uinteger64 RIGHT = frame.registers[B];

                if (RIGHT == MAREA_UINTEGER_64(0)) {
                    *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_DIVISION_BY_ZERO);

                    return MAREA_VM_EXCEPT_SUCCESS;
                }

                frame.registers[A] = MAREA_G_VmNormalize(KIND, ((instruction.opcode == MAREA_BYTECODE_OPCODE_DIV) ? (LEFT / RIGHT) : (LEFT % RIGHT)));
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_POW:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 KIND = (MAREA_Uinteger8)C;

            if (MAREA_G_VmIsFloatKind(KIND)) {
                frame.registers[A] = MAREA_G_VmFromDouble(KIND, pow(MAREA_G_VmAsDouble(frame.registers[A]), MAREA_G_VmAsDouble(frame.registers[B])));
            } else if (MAREA_G_VmIsSignedKind(KIND)) {
                const MAREA_Sinteger64 BASE = (MAREA_Sinteger64)frame.registers[A];
                const MAREA_Sinteger64 EXPONENT = (MAREA_Sinteger64)frame.registers[B];

                if (EXPONENT < MAREA_SINTEGER_64(0)) {
                    *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_NEGATIVE_EXPONENT);

                    return MAREA_VM_EXCEPT_SUCCESS;
                }

                const MAREA_Boolean NEGATIVE = (BASE < MAREA_SINTEGER_64(0));
                const MAREA_Uinteger64 MAGNITUDE = (NEGATIVE ? (MAREA_UINTEGER_64(0) - (MAREA_Uinteger64)BASE) : (MAREA_Uinteger64)BASE);

                frame.registers[A] = MAREA_G_VmNormalize(KIND, MAREA_G_VmIntegerPower(NEGATIVE, MAGNITUDE, (MAREA_Uinteger64)EXPONENT));
            } else {
                frame.registers[A] = MAREA_G_VmNormalize(KIND, MAREA_G_VmIntegerPower(MAREA_BOOLEAN_FALSE, frame.registers[A], frame.registers[B]));
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_SATURATING_ADD:
        case MAREA_BYTECODE_OPCODE_SATURATING_SUB:
        case MAREA_BYTECODE_OPCODE_SATURATING_MUL:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 KIND = (MAREA_Uinteger8)C;

            if (MAREA_G_VmIsFloatKind(KIND)) {
                const MAREA_Float64 LEFT = MAREA_G_VmAsDouble(frame.registers[A]);
                const MAREA_Float64 RIGHT = MAREA_G_VmAsDouble(frame.registers[B]);
                MAREA_Float64 computed = MAREA_FLOAT_64(0.0);

                if (instruction.opcode == MAREA_BYTECODE_OPCODE_SATURATING_ADD) {
                    computed = (LEFT + RIGHT);
                } else if (instruction.opcode == MAREA_BYTECODE_OPCODE_SATURATING_SUB) {
                    computed = (LEFT - RIGHT);
                } else {
                    computed = (LEFT * RIGHT);
                }

                frame.registers[A] = MAREA_G_VmFromDouble(KIND, computed);
            } else if (instruction.opcode == MAREA_BYTECODE_OPCODE_SATURATING_ADD) {
                frame.registers[A] = MAREA_G_VmSaturatingAdd(KIND, frame.registers[A], frame.registers[B]);
            } else if (instruction.opcode == MAREA_BYTECODE_OPCODE_SATURATING_SUB) {
                frame.registers[A] = MAREA_G_VmSaturatingSubtract(KIND, frame.registers[A], frame.registers[B]);
            } else {
                frame.registers[A] = MAREA_G_VmSaturatingMultiply(KIND, frame.registers[A], frame.registers[B]);
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_SHIFT_LEFT:
        case MAREA_BYTECODE_OPCODE_SHIFT_RIGHT:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 KIND = (MAREA_Uinteger8)C;

            if (KIND >= MAREA_BYTECODE_NUMBER_KIND_COUNT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger32 WIDTHS[MAREA_BYTECODE_NUMBER_KIND_COUNT] = { 8, 16, 32, 64, 8, 16, 32, 64, 0, 0 };
            const MAREA_Uinteger32 WIDTH = WIDTHS[KIND];
            const MAREA_Uinteger64 COUNT = frame.registers[B];

            if (COUNT >= (MAREA_Uinteger64)WIDTH) {
                *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_SHIFT_OUT_OF_RANGE);

                return MAREA_VM_EXCEPT_SUCCESS;
            }

            if (instruction.opcode == MAREA_BYTECODE_OPCODE_SHIFT_LEFT) {
                frame.registers[A] = MAREA_G_VmNormalize(KIND, (frame.registers[A] << COUNT));
            } else if (MAREA_G_VmIsSignedKind(KIND)) {
                frame.registers[A] = MAREA_G_VmNormalize(KIND, (MAREA_Uinteger64)(((MAREA_Sinteger64)frame.registers[A]) >> COUNT));
            } else {
                frame.registers[A] = MAREA_G_VmNormalize(KIND, (frame.registers[A] >> COUNT));
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_NEGATE:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 KIND = (MAREA_Uinteger8)C;

            if (MAREA_G_VmIsFloatKind(KIND)) {
                frame.registers[A] = MAREA_G_VmFromDouble(KIND, (-MAREA_G_VmAsDouble(frame.registers[B])));
            } else {
                frame.registers[A] = MAREA_G_VmNormalize(KIND, (MAREA_UINTEGER_64(0) - frame.registers[B]));
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_BIT_NOT:
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            frame.registers[A] = MAREA_G_VmNormalize((MAREA_Uinteger8)C, (~frame.registers[B]));

            break;
        case MAREA_BYTECODE_OPCODE_NOT:
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            frame.registers[A] = ((frame.registers[B] == MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));

            break;
        case MAREA_BYTECODE_OPCODE_CONVERT:
        {
            if (B >= MAREA_VM_FRAME_REGISTER_LIMIT) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            const MAREA_Uinteger8 SOURCE_KIND = (MAREA_Uinteger8)(C & MAREA_UINTEGER_32(0xFF));
            const MAREA_Uinteger8 TARGET_KIND = (MAREA_Uinteger8)(C >> MAREA_UINTEGER_32(8));
            const MAREA_Boolean SOURCE_FLOAT = MAREA_G_VmIsFloatKind(SOURCE_KIND);
            const MAREA_Boolean TARGET_FLOAT = MAREA_G_VmIsFloatKind(TARGET_KIND);

            if (SOURCE_FLOAT && TARGET_FLOAT) {
                frame.registers[A] = MAREA_G_VmFromDouble(TARGET_KIND, MAREA_G_VmAsDouble(frame.registers[B]));
            } else if (SOURCE_FLOAT) {
                const MAREA_Float64 VALUE = MAREA_G_VmAsDouble(frame.registers[B]);
                const MAREA_Float64 MINIMUM = (MAREA_Float64)(MAREA_Sinteger64)MAREA_G_VmKindMinimum(TARGET_KIND);
                const MAREA_Float64 MAXIMUM = MAREA_G_VmIsSignedKind(TARGET_KIND) ? (MAREA_Float64)(MAREA_Sinteger64)MAREA_G_VmKindMaximum(TARGET_KIND) : (MAREA_Float64)MAREA_G_VmKindMaximum(TARGET_KIND);

                if (isnan(VALUE) || (VALUE < MINIMUM) || (VALUE > MAXIMUM)) {
                    *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_CONVERT_OUT_OF_RANGE);

                    return MAREA_VM_EXCEPT_SUCCESS;
                }

                frame.registers[A] = MAREA_G_VmNormalize(TARGET_KIND, MAREA_G_VmIsSignedKind(TARGET_KIND) ? (MAREA_Uinteger64)(MAREA_Sinteger64)VALUE : (MAREA_Uinteger64)VALUE);
            } else if (TARGET_FLOAT) {
                const MAREA_Float64 VALUE = MAREA_G_VmIsSignedKind(SOURCE_KIND) ? (MAREA_Float64)(MAREA_Sinteger64)frame.registers[B] : (MAREA_Float64)frame.registers[B];

                frame.registers[A] = MAREA_G_VmFromDouble(TARGET_KIND, VALUE);
            } else {
                if (!MAREA_G_VmIntegerFits(SOURCE_KIND, TARGET_KIND, frame.registers[B])) {
                    *outResult = MAREA_G_VmTrapResult(MAREA_BYTECODE_TRAP_CODE_CONVERT_OUT_OF_RANGE);

                    return MAREA_VM_EXCEPT_SUCCESS;
                }

                frame.registers[A] = MAREA_G_VmNormalize(TARGET_KIND, frame.registers[B]);
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_BIT_AND:
        case MAREA_BYTECODE_OPCODE_BIT_OR:
        case MAREA_BYTECODE_OPCODE_BIT_XOR:
        case MAREA_BYTECODE_OPCODE_EQUAL:
        case MAREA_BYTECODE_OPCODE_NOT_EQUAL:
        case MAREA_BYTECODE_OPCODE_LESS_SIGNED:
        case MAREA_BYTECODE_OPCODE_LESS_EQUAL_SIGNED:
        case MAREA_BYTECODE_OPCODE_LESS_UNSIGNED:
        case MAREA_BYTECODE_OPCODE_LESS_EQUAL_UNSIGNED:
        case MAREA_BYTECODE_OPCODE_EQUAL_FLOAT:
        case MAREA_BYTECODE_OPCODE_NOT_EQUAL_FLOAT:
        case MAREA_BYTECODE_OPCODE_LESS_FLOAT:
        case MAREA_BYTECODE_OPCODE_LESS_EQUAL_FLOAT:
        {
            if ((B >= MAREA_VM_FRAME_REGISTER_LIMIT) || (C >= MAREA_VM_FRAME_REGISTER_LIMIT)) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            switch (instruction.opcode) {
            case MAREA_BYTECODE_OPCODE_BIT_AND:
                frame.registers[A] = (frame.registers[B] & frame.registers[C]);
                break;
            case MAREA_BYTECODE_OPCODE_BIT_OR:
                frame.registers[A] = (frame.registers[B] | frame.registers[C]);
                break;
            case MAREA_BYTECODE_OPCODE_BIT_XOR:
                frame.registers[A] = (frame.registers[B] ^ frame.registers[C]);
                break;
            case MAREA_BYTECODE_OPCODE_EQUAL:
                frame.registers[A] = ((frame.registers[B] == frame.registers[C]) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_NOT_EQUAL:
                frame.registers[A] = ((frame.registers[B] != frame.registers[C]) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_LESS_SIGNED:
                frame.registers[A] = ((((MAREA_Sinteger64)frame.registers[B]) < ((MAREA_Sinteger64)frame.registers[C])) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_LESS_EQUAL_SIGNED:
                frame.registers[A] = ((((MAREA_Sinteger64)frame.registers[B]) <= ((MAREA_Sinteger64)frame.registers[C])) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_LESS_UNSIGNED:
                frame.registers[A] = ((frame.registers[B] < frame.registers[C]) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_LESS_EQUAL_UNSIGNED:
                frame.registers[A] = ((frame.registers[B] <= frame.registers[C]) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_EQUAL_FLOAT:
                frame.registers[A] = ((MAREA_G_VmAsDouble(frame.registers[B]) == MAREA_G_VmAsDouble(frame.registers[C])) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_NOT_EQUAL_FLOAT:
                frame.registers[A] = ((MAREA_G_VmAsDouble(frame.registers[B]) != MAREA_G_VmAsDouble(frame.registers[C])) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            case MAREA_BYTECODE_OPCODE_LESS_FLOAT:
                frame.registers[A] = ((MAREA_G_VmAsDouble(frame.registers[B]) < MAREA_G_VmAsDouble(frame.registers[C])) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            default:
                frame.registers[A] = ((MAREA_G_VmAsDouble(frame.registers[B]) <= MAREA_G_VmAsDouble(frame.registers[C])) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
                break;
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_JUMP:
            ip = (MAREA_Uinteger32)((MAREA_Sinteger64)(ip + MAREA_UINTEGER_32(1)) + (MAREA_Sinteger64)(MAREA_Sinteger16)instruction.b);
            advance = MAREA_BOOLEAN_FALSE;

            break;
        case MAREA_BYTECODE_OPCODE_JUMP_IF:
        case MAREA_BYTECODE_OPCODE_JUMP_IF_NOT:
        {
            const MAREA_Boolean TAKE = ((instruction.opcode == MAREA_BYTECODE_OPCODE_JUMP_IF) ? (frame.registers[A] != MAREA_UINTEGER_64(0)) : (frame.registers[A] == MAREA_UINTEGER_64(0)));

            if (TAKE) {
                ip = (MAREA_Uinteger32)((MAREA_Sinteger64)(ip + MAREA_UINTEGER_32(1)) + (MAREA_Sinteger64)(MAREA_Sinteger16)instruction.b);
                advance = MAREA_BOOLEAN_FALSE;
            }

            break;
        }
        case MAREA_BYTECODE_OPCODE_CALL:
        {
            if (C > (MAREA_VM_FRAME_REGISTER_LIMIT - A)) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            MAREA_VmResult called = { 0 };

            const MAREA_VmExcept EXCEPT = MAREA_VmRun(context, module, B, &frame.registers[(A + MAREA_UINTEGER_32(1))], C, (depth + MAREA_UINTEGER_32(1)), &called);

            if (EXCEPT != MAREA_VM_EXCEPT_SUCCESS) {
                return EXCEPT;
            }

            if (called.trapped) {
                *outResult = called;

                return MAREA_VM_EXCEPT_SUCCESS;
            }

            frame.registers[A] = called.value;

            break;
        }
        case MAREA_BYTECODE_OPCODE_TRAP:
            *outResult = MAREA_G_VmTrapResult((MAREA_BytecodeTrapCode)A);

            return MAREA_VM_EXCEPT_SUCCESS;
        default:
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        if (advance) {
            ip += MAREA_UINTEGER_32(1);
        }
    }
}
