// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAHostContext.h"

#include <stdlib.h>
#include <string.h>

// INFO: the default allocator keeps the address that malloc gave in the word before the aligned block, so any power of two works and free needs no size.
static MAREA_Null MAREA_CALL MAREA_G_HostContextDefaultAllocate(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
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

    MAREA_Uinteger64 address = MAREA_UINTEGER_64(0);

    memcpy(&address, &block, sizeof(block));

    const MAREA_IntegerSize MISALIGNMENT = (MAREA_IntegerSize)(address & (MAREA_Uinteger64)(alignment - MAREA_INTEGER_SIZE(1)));

    if (MISALIGNMENT != MAREA_INTEGER_SIZE(0)) {
        block += (alignment - MISALIGNMENT);
    }

    const MAREA_Null KEPT = raw;

    memcpy((block - sizeof(KEPT)), &KEPT, sizeof(KEPT));

    return block;
}

static void MAREA_CALL MAREA_G_HostContextDefaultFree(MAREA_Null address)
{
    MAREA_Null raw = MAREA_NULLPTR;

    memcpy(&raw, (((MAREA_Byte *)address) - sizeof(raw)), sizeof(raw));

    free(raw);
}

// INFO: the two hooks come from one allocator, so one of them alone gives and takes no block.
static MAREA_Boolean MAREA_CALL MAREA_G_HostContextIsPaired(MAREA_HostContext *context)
{
    if ((context->allocate != MAREA_NULLPTR) && (context->release != MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    context->unpaired = MAREA_BOOLEAN_TRUE;

    return MAREA_BOOLEAN_FALSE;
}

MAREA_HostContextExcept MAREA_CALL MAREA_HostContextInitialize(MAREA_HostContext *context, MAREA_RuntimeAllocateFunction allocate, MAREA_RuntimeFreeFunction release)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((allocate == MAREA_NULLPTR) != (release == MAREA_NULLPTR)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR;
    }

    memset(context, 0, sizeof(*context));

    context->allocate = ((allocate == MAREA_NULLPTR) ? &MAREA_G_HostContextDefaultAllocate : allocate);
    context->release = ((release == MAREA_NULLPTR) ? &MAREA_G_HostContextDefaultFree : release);
    context->trap = MAREA_HOST_TRAP_NONE;
    context->unpaired = MAREA_BOOLEAN_FALSE;

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostContextExcept MAREA_CALL MAREA_HostContextAllocate(MAREA_HostContext *context, MAREA_IntegerSize size, MAREA_IntegerSize alignment, MAREA_Null *outBlock)
{
    if (outBlock == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    *outBlock = MAREA_NULLPTR;

    if (context == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((alignment == MAREA_INTEGER_SIZE(0)) || ((alignment & (alignment - MAREA_INTEGER_SIZE(1))) != MAREA_INTEGER_SIZE(0))) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    if (!MAREA_G_HostContextIsPaired(context)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR;
    }

    const MAREA_IntegerSize ASKED = ((size == MAREA_INTEGER_SIZE(0)) ? MAREA_INTEGER_SIZE(1) : size);

    const MAREA_Null BLOCK = context->allocate(ASKED, alignment);

    if (BLOCK == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    memset(BLOCK, 0, ASKED);

    *outBlock = BLOCK;

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

void MAREA_CALL MAREA_HostContextFree(MAREA_HostContext *context, MAREA_Null address)
{
    if (context == MAREA_NULLPTR) {
        return;
    }

    if (!MAREA_G_HostContextIsPaired(context)) {
        return;
    }

    if (address != MAREA_NULLPTR) {
        context->release(address);
    }
}

void MAREA_CALL MAREA_HostContextRaiseTrap(MAREA_HostContext *context, MAREA_HostTrap trap)
{
    if (context == MAREA_NULLPTR) {
        return;
    }

    if ((trap == MAREA_HOST_TRAP_OUT_OF_MEMORY) && (context->trap == MAREA_HOST_TRAP_NONE)) {
        context->trap = trap;
    }
}

MAREA_HostTrap MAREA_CALL MAREA_HostContextGetTrap(const MAREA_HostContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_HOST_TRAP_NONE;
    }

    return context->trap;
}
