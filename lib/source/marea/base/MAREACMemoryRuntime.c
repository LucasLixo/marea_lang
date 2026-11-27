// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACMemoryRuntime.h"

// TODO: not implemented yet.
MAREA_Uinteger64 MAREA_CALL MAREA_CMemoryAllocate(MAREA_HostContext *context, MAREA_Uinteger64 size, MAREA_Uinteger64 alignment)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(size);
    MAREA_IGNORE(alignment);

    return MAREA_UINTEGER_64(0);
}

// TODO: not implemented yet.
void MAREA_CALL MAREA_CMemoryFree(MAREA_HostContext *context, MAREA_Uinteger64 address)
{
    MAREA_IGNORE(context);
    MAREA_IGNORE(address);
}

// TODO: not implemented yet.
void MAREA_CALL MAREA_CMemoryCopyBytes(MAREA_Uinteger64 destination, MAREA_Uinteger64 source, MAREA_Uinteger64 count)
{
    MAREA_IGNORE(destination);
    MAREA_IGNORE(source);
    MAREA_IGNORE(count);
}

// TODO: not implemented yet.
void MAREA_CALL MAREA_CMemoryFillBytes(MAREA_Uinteger64 destination, MAREA_Uinteger8 value, MAREA_Uinteger64 count)
{
    MAREA_IGNORE(destination);
    MAREA_IGNORE(value);
    MAREA_IGNORE(count);
}

// TODO: not implemented yet.
MAREA_Uinteger64 MAREA_CALL MAREA_CMemoryOffset(MAREA_Uinteger64 address, MAREA_Sinteger64 count, MAREA_Uinteger64 stride)
{
    MAREA_IGNORE(address);
    MAREA_IGNORE(count);
    MAREA_IGNORE(stride);

    return MAREA_UINTEGER_64(0);
}

// TODO: not implemented yet.
MAREA_Boolean MAREA_CALL MAREA_CMemoryIsNull(MAREA_Uinteger64 address)
{
    MAREA_IGNORE(address);

    return MAREA_BOOLEAN_FALSE;
}
