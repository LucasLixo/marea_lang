// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAHostContext.h"

#include "marea/internal/base/MAREAHostContextInternal.h"

#include <string.h>

// INFO: the two hooks come from one allocator, so one of them alone gives and takes no block.
static MAREA_Boolean MAREA_CALL MAREA_G_HostContextIsPaired(MAREA_HostContext *context)
{
    if ((context->allocate != MAREA_NULLPTR) && (context->release != MAREA_NULLPTR)) {
        return MAREA_BOOLEAN_TRUE;
    }

    context->unpaired = MAREA_BOOLEAN_TRUE;

    return MAREA_BOOLEAN_FALSE;
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

MAREA_HostContextExcept MAREA_CALL MAREA_HostContextFree(MAREA_HostContext *context, MAREA_Null address)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if (!MAREA_G_HostContextIsPaired(context)) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_UNPAIRED_ALLOCATOR;
    }

    if (address != MAREA_NULLPTR) {
        context->release(address);
    }

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostContextExcept MAREA_CALL MAREA_HostContextRaiseTrap(MAREA_HostContext *context, MAREA_HostTrap trap)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_NULL_POINTER;
    }

    if (trap != MAREA_HOST_TRAP_OUT_OF_MEMORY) {
        return MAREA_HOST_CONTEXT_EXCEPT_ERROR_OUT_OF_RANGE;
    }

    if (context->trap == MAREA_HOST_TRAP_NONE) {
        context->trap = trap;
    }

    return MAREA_HOST_CONTEXT_EXCEPT_SUCCESS;
}

MAREA_HostTrap MAREA_CALL MAREA_HostContextGetTrap(const MAREA_HostContext *context)
{
    if (context == MAREA_NULLPTR) {
        return MAREA_HOST_TRAP_NONE;
    }

    return context->trap;
}
