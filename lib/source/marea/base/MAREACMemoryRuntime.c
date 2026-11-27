// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACMemoryRuntime.h"

#include <stdint.h>
#include <string.h>

static MAREA_Byte *MAREA_CALL MAREA_G_BytesAt(MAREA_Uinteger64 address)
{
    return (MAREA_Byte *)(uintptr_t)address;
}

MAREA_Uinteger64 MAREA_CALL MAREA_CMemoryAllocate(MAREA_HostContext *context, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment)
{
    MAREA_Null block = MAREA_NULLPTR;

    // INFO: a number that the machine cannot hold is checked before the cast, which would cut it.
    if ((size > (MAREA_Uinteger64)MAREA_INTEGER_SIZE_MAXIMUM) || (alignment > (MAREA_Uinteger64)MAREA_INTEGER_SIZE_MAXIMUM) || (MAREA_HostContextAllocate(context, (MAREA_IntegerSize)size, (MAREA_IntegerSize)alignment, &block) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS)) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

        return MAREA_UINTEGER_64(0);
    }

    return (MAREA_Uinteger64)(uintptr_t)block;
}

void MAREA_CALL MAREA_CMemoryFree(MAREA_HostContext *context, MAREA_Uinteger64 address)
{
    MAREA_HostContextFree(context, MAREA_G_BytesAt(address));
}

void MAREA_CALL MAREA_CMemoryCopyBytes(MAREA_Uinteger64 destination, MAREA_Uinteger64 source, MAREA_Uinteger64 count)
{
    if (count > MAREA_UINTEGER_64(0)) {
        memmove(MAREA_G_BytesAt(destination), MAREA_G_BytesAt(source), (MAREA_IntegerSize)count);
    }
}

void MAREA_CALL MAREA_CMemoryFillBytes(MAREA_Uinteger64 destination, MAREA_Uinteger8 value, MAREA_Uinteger64 count)
{
    if (count > MAREA_UINTEGER_64(0)) {
        memset(MAREA_G_BytesAt(destination), (int)value, (MAREA_IntegerSize)count);
    }
}

MAREA_Uinteger64 MAREA_CALL MAREA_CMemoryOffset(MAREA_Uinteger64 address, MAREA_Sinteger64 count, MAREA_Uinteger64 stride)
{
    // INFO: unsigned arithmetic wraps, and the cast of a negative count keeps its two's complement bits.
    return (address + ((MAREA_Uinteger64)count * stride));
}

MAREA_Boolean MAREA_CALL MAREA_CMemoryIsNull(MAREA_Uinteger64 address)
{
    return (address == MAREA_UINTEGER_64(0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}
