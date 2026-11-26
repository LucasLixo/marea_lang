// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/vm/MAREAVmInternal.h"

#include "marea/base/MAREAHostContext.h"
#include "marea/base/MAREAHostFunction.h"
#include "marea/internal/base/MAREAHostContextInternal.h"

#include <stdlib.h>
#include <string.h>

// INFO: a trap that a host function raises is the vm trap of the same name.
MAREA_STATIC_ASSERT((MAREA_Uinteger8)MAREA_HOST_TRAP_OUT_OF_MEMORY == (MAREA_Uinteger8)MAREA_BYTECODE_TRAP_CODE_OUT_OF_MEMORY, "A host trap must be the vm trap of the same name:");

// INFO: a register holds an address as the bit pattern of a pointer, the same rule the composite opcodes follow.
static MAREA_Null MAREA_CALL MAREA_G_VmPointerOf(MAREA_Uinteger64 bits)
{
    MAREA_Null pointer = MAREA_NULLPTR;

    memcpy(&pointer, &bits, sizeof(pointer));

    return pointer;
}

static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmBitsOf(const MAREA_Null pointer)
{
    MAREA_Uinteger64 bits = MAREA_UINTEGER_64(0);

    memcpy(&bits, &pointer, sizeof(pointer));

    return bits;
}

static MAREA_Boolean MAREA_CALL MAREA_G_VmNameIs(const MAREA_Character8 *text, MAREA_IntegerSize length, const MAREA_Character8 *expected)
{
    return ((strlen(expected) == length) && (memcmp(text, expected, length) == 0));
}

// INFO: a C function writes 8 bytes for a scalar; the register keeps them normalized to the declared type, as an
// arithmetic opcode does. The type table names a primitive, and the name says how to read the bits.
static MAREA_Uinteger64 MAREA_CALL MAREA_G_VmNormalizeScalar(const MAREA_Character8 *name, MAREA_IntegerSize length, MAREA_Uinteger64 bits)
{
    if (MAREA_G_VmNameIs(name, length, "boolean")) {
        return ((bits != MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0));
    }

    if (MAREA_G_VmNameIs(name, length, "sinteger8")) {
        return (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger8)bits;
    }

    if (MAREA_G_VmNameIs(name, length, "sinteger16")) {
        return (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger16)bits;
    }

    if (MAREA_G_VmNameIs(name, length, "sinteger32")) {
        return (MAREA_Uinteger64)(MAREA_Sinteger64)(MAREA_Sinteger32)bits;
    }

    if (MAREA_G_VmNameIs(name, length, "uinteger8")) {
        return (bits & MAREA_UINTEGER_64(0xFF));
    }

    if (MAREA_G_VmNameIs(name, length, "uinteger16")) {
        return (bits & MAREA_UINTEGER_64(0xFFFF));
    }

    if (MAREA_G_VmNameIs(name, length, "uinteger32")) {
        return (bits & MAREA_UINTEGER_64(0xFFFFFFFF));
    }

    if (MAREA_G_VmNameIs(name, length, "float32")) {
        MAREA_Float64 wide = MAREA_FLOAT_64(0.0);

        memcpy(&wide, &bits, sizeof(wide));

        wide = (MAREA_Float64)((MAREA_Float32)wide);

        MAREA_Uinteger64 rounded = MAREA_UINTEGER_64(0);

        memcpy(&rounded, &wide, sizeof(rounded));

        return rounded;
    }

    return bits;
}

// INFO: the name of an import is the namespace and the name of the registration, joined by the last dot.
static MAREA_HostFunction MAREA_CALL MAREA_G_VmFindHostFunction(const MAREA_VmContext *context, const MAREA_Character8 *name, MAREA_IntegerSize length)
{
    MAREA_IntegerSize dot = length;

    while ((dot > MAREA_INTEGER_SIZE(0)) && (name[(dot - MAREA_INTEGER_SIZE(1))] != '.')) {
        dot -= MAREA_INTEGER_SIZE(1);
    }

    if (dot == MAREA_INTEGER_SIZE(0)) {
        return MAREA_NULLPTR;
    }

    return MAREA_TypeRegistryFindFunction(context->registry, name, (dot - MAREA_INTEGER_SIZE(1)), &name[dot], (length - dot));
}

// INFO: the allocator of the vm keeps the address that malloc gave in the word before the aligned block, so any
// power of two works and free needs no size.
static MAREA_Null MAREA_CALL MAREA_G_VmDefaultAllocate(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
{
    const MAREA_IntegerSize EXTRA = ((alignment - MAREA_INTEGER_SIZE(1)) + sizeof(MAREA_Null));

    if (size > (MAREA_INTEGER_SIZE_MAXIMUM - EXTRA)) {
        return MAREA_NULLPTR;
    }

    MAREA_Byte *raw = malloc((size + EXTRA));

    if (raw == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    MAREA_Byte *block = (raw + sizeof(MAREA_Null));

    const MAREA_IntegerSize MISALIGNMENT = (MAREA_IntegerSize)(MAREA_G_VmBitsOf(block) & (MAREA_Uinteger64)(alignment - MAREA_INTEGER_SIZE(1)));

    if (MISALIGNMENT != MAREA_INTEGER_SIZE(0)) {
        block += (alignment - MISALIGNMENT);
    }

    const MAREA_Null KEPT = raw;

    memcpy((block - sizeof(KEPT)), &KEPT, sizeof(KEPT));

    return block;
}

static void MAREA_CALL MAREA_G_VmDefaultFree(MAREA_Null address)
{
    MAREA_Null raw = MAREA_NULLPTR;

    memcpy(&raw, (((MAREA_Byte *)address) - sizeof(raw)), sizeof(raw));

    free(raw);
}

// INFO: the two hooks come from one allocator, so a block is never freed by another one than gave it.
static MAREA_VmExcept MAREA_CALL MAREA_G_VmAllocator(const MAREA_VmContext *context, MAREA_RuntimeAllocateFunction *outAllocate, MAREA_RuntimeFreeFunction *outFree)
{
    const MAREA_Null ALLOCATE = MAREA_TypeRegistryFindRuntimeFunction(context->registry, "alloc", MAREA_INTEGER_SIZE(5));
    const MAREA_Null RELEASE = MAREA_TypeRegistryFindRuntimeFunction(context->registry, "free", MAREA_INTEGER_SIZE(4));

    *outAllocate = &MAREA_G_VmDefaultAllocate;
    *outFree = &MAREA_G_VmDefaultFree;

    if ((ALLOCATE == MAREA_NULLPTR) && (RELEASE == MAREA_NULLPTR)) {
        return MAREA_VM_EXCEPT_SUCCESS;
    }

    if ((ALLOCATE == MAREA_NULLPTR) || (RELEASE == MAREA_NULLPTR)) {
        return MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR;
    }

    memcpy(outAllocate, &ALLOCATE, sizeof(*outAllocate));
    memcpy(outFree, &RELEASE, sizeof(*outFree));

    return MAREA_VM_EXCEPT_SUCCESS;
}

// INFO: with one replaced hook both stay null, so the context reports the fault only when the function allocates.
static MAREA_HostContext MAREA_CALL MAREA_G_VmHostContext(const MAREA_VmContext *context)
{
    MAREA_HostContext host = { .allocate = MAREA_NULLPTR, .release = MAREA_NULLPTR, .trap = MAREA_HOST_TRAP_NONE, .unpaired = MAREA_BOOLEAN_FALSE };

    MAREA_RuntimeAllocateFunction allocate = MAREA_NULLPTR;
    MAREA_RuntimeFreeFunction release = MAREA_NULLPTR;

    if (MAREA_G_VmAllocator(context, &allocate, &release) == MAREA_VM_EXCEPT_SUCCESS) {
        host.allocate = allocate;
        host.release = release;
    }

    return host;
}

MAREA_VmExcept MAREA_CALL MAREA_VmCallHost(MAREA_VmContext *context, const MAREA_BytecodeModule *module, MAREA_Uinteger32 functionIndex, const MAREA_BytecodeFunctionInfo *info, MAREA_Uinteger64 *window, MAREA_Uinteger32 argumentCount, MAREA_VmResult *outResult)
{
    outResult->value = MAREA_UINTEGER_64(0);
    outResult->trap = MAREA_BYTECODE_TRAP_CODE_NONE;
    outResult->trapped = MAREA_BOOLEAN_FALSE;

    if ((argumentCount != info->parameterCount) || (argumentCount > MAREA_VM_HOST_PARAMETER_LIMIT)) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    const MAREA_Character8 *name = MAREA_NULLPTR;
    MAREA_IntegerSize length = MAREA_INTEGER_SIZE(0);

    if (MAREA_BytecodeModuleGetString(module, info->name, &name, &length) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    const MAREA_HostFunction FOUND = MAREA_G_VmFindHostFunction(context, name, length);

    if (FOUND == MAREA_NULLPTR) {
        outResult->trap = MAREA_BYTECODE_TRAP_CODE_HOST_FUNCTION_MISSING;
        outResult->trapped = MAREA_BOOLEAN_TRUE;

        return MAREA_VM_EXCEPT_SUCCESS;
    }

    MAREA_Null arguments[MAREA_VM_HOST_PARAMETER_LIMIT];

    for (MAREA_Uinteger32 index = MAREA_UINTEGER_32(0); index < argumentCount; index += MAREA_UINTEGER_32(1)) {
        MAREA_BytecodeParameterInfo parameter = { 0 };

        if (MAREA_BytecodeModuleGetParameter(module, functionIndex, index, &parameter) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        MAREA_Uinteger64 *slot = &window[(index + MAREA_UINTEGER_32(1))];

        if ((parameter.flags & MAREA_BYTECODE_PARAMETER_FLAG_BORROW) == MAREA_UINTEGER_32(0)) {
            arguments[index] = slot;

            continue;
        }

        arguments[index] = MAREA_G_VmPointerOf(*slot);

        if (arguments[index] == MAREA_NULLPTR) {
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }
    }

    MAREA_Uinteger64 scalar = MAREA_UINTEGER_64(0);
    MAREA_Null result = MAREA_NULLPTR;
    const MAREA_Character8 *scalarName = MAREA_NULLPTR;
    MAREA_IntegerSize scalarLength = MAREA_INTEGER_SIZE(0);

    if (info->result != MAREA_BYTECODE_NONE) {
        MAREA_BytecodeTypeInfo type = { 0 };

        if (MAREA_BytecodeModuleGetType(module, info->result, &type) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
            return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
        }

        if (type.kind == MAREA_BYTECODE_TYPE_KIND_STRUCTURE) {
            result = MAREA_G_VmPointerOf(window[0]);

            if (result == MAREA_NULLPTR) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }
        } else {
            if (MAREA_BytecodeModuleGetString(module, type.name, &scalarName, &scalarLength) != MAREA_BYTECODE_EXCEPT_SUCCESS) {
                return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
            }

            result = &scalar;
        }
    }

    MAREA_HostContext host = MAREA_G_VmHostContext(context);

    FOUND(&host, arguments, result);

    if (host.unpaired) {
        return MAREA_VM_EXCEPT_ERROR_UNPAIRED_ALLOCATOR;
    }

    if (host.trap != MAREA_HOST_TRAP_NONE) {
        outResult->trap = (MAREA_BytecodeTrapCode)host.trap;
        outResult->trapped = MAREA_BOOLEAN_TRUE;

        return MAREA_VM_EXCEPT_SUCCESS;
    }

    if (scalarName != MAREA_NULLPTR) {
        window[0] = MAREA_G_VmNormalizeScalar(scalarName, scalarLength, scalar);
    }

    outResult->value = window[0];

    return MAREA_VM_EXCEPT_SUCCESS;
}

MAREA_VmExcept MAREA_CALL MAREA_VmAllocate(const MAREA_VmContext *context, MAREA_Uinteger64 size, MAREA_Uinteger32 alignment, MAREA_Uinteger64 *outAddress, MAREA_VmResult *outResult)
{
    if ((alignment == MAREA_UINTEGER_32(0)) || ((alignment & (alignment - MAREA_UINTEGER_32(1))) != MAREA_UINTEGER_32(0))) {
        return MAREA_VM_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    MAREA_RuntimeAllocateFunction allocate = MAREA_NULLPTR;
    MAREA_RuntimeFreeFunction release = MAREA_NULLPTR;

    const MAREA_VmExcept PAIRED = MAREA_G_VmAllocator(context, &allocate, &release);

    if (PAIRED != MAREA_VM_EXCEPT_SUCCESS) {
        return PAIRED;
    }

    const MAREA_Uinteger64 ASKED = ((size == MAREA_UINTEGER_64(0)) ? MAREA_UINTEGER_64(1) : size);

    MAREA_Null block = MAREA_NULLPTR;

    if (ASKED <= (MAREA_Uinteger64)MAREA_INTEGER_SIZE_MAXIMUM) {
        block = allocate((MAREA_IntegerSize)ASKED, (MAREA_IntegerSize)alignment);
    }

    if (block == MAREA_NULLPTR) {
        outResult->value = MAREA_UINTEGER_64(0);
        outResult->trap = MAREA_BYTECODE_TRAP_CODE_OUT_OF_MEMORY;
        outResult->trapped = MAREA_BOOLEAN_TRUE;

        return MAREA_VM_EXCEPT_SUCCESS;
    }

    memset(block, 0, (MAREA_IntegerSize)ASKED);

    *outAddress = MAREA_G_VmBitsOf(block);

    return MAREA_VM_EXCEPT_SUCCESS;
}

MAREA_VmExcept MAREA_CALL MAREA_VmFree(const MAREA_VmContext *context, MAREA_Uinteger64 address)
{
    MAREA_RuntimeAllocateFunction allocate = MAREA_NULLPTR;
    MAREA_RuntimeFreeFunction release = MAREA_NULLPTR;

    const MAREA_VmExcept PAIRED = MAREA_G_VmAllocator(context, &allocate, &release);

    if (PAIRED != MAREA_VM_EXCEPT_SUCCESS) {
        return PAIRED;
    }

    if (address != MAREA_UINTEGER_64(0)) {
        release(MAREA_G_VmPointerOf(address));
    }

    return MAREA_VM_EXCEPT_SUCCESS;
}
