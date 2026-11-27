// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACString.h"

#include "marea/base/MAREAHostFunction.h"

#include <string.h>

// INFO: a UTF-8 continuation byte matches 10xxxxxx; every other byte starts a new scalar value.
#define MAREA_C_STRING_CONTINUATION_MASK  MAREA_UINTEGER_8(0xC0)
#define MAREA_C_STRING_CONTINUATION_VALUE MAREA_UINTEGER_8(0x80)

// INFO: the growth rule marea.vector reuses: at least 4 bytes, doubled until the required length fits.
#define MAREA_C_STRING_MINIMUM_CAPACITY MAREA_INTEGER_SIZE(4)

MAREA_Uinteger64 MAREA_CALL MAREA_CStringLength(const MAREA_String *value)
{
    MAREA_Uinteger64 count = MAREA_UINTEGER_64(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index < value->lengthInBytes; index = (index + MAREA_INTEGER_SIZE(1))) {
        const MAREA_Uinteger8 BYTE = (MAREA_Uinteger8)value->data[index];

        if (((MAREA_Uinteger8)(BYTE & MAREA_C_STRING_CONTINUATION_MASK)) != MAREA_C_STRING_CONTINUATION_VALUE) {
            count = (count + MAREA_UINTEGER_64(1));
        }
    }

    return count;
}

void MAREA_CALL MAREA_CStringAppend(MAREA_HostContext *context, MAREA_String *target, const MAREA_String *suffix)
{
    const MAREA_IntegerSize REQUIRED_LENGTH = (target->lengthInBytes + suffix->lengthInBytes);

    if (REQUIRED_LENGTH > target->capacityInBytes) {
        MAREA_IntegerSize newCapacity = ((target->capacityInBytes < MAREA_C_STRING_MINIMUM_CAPACITY) ? MAREA_C_STRING_MINIMUM_CAPACITY : (target->capacityInBytes * MAREA_INTEGER_SIZE(2)));

        while (newCapacity < REQUIRED_LENGTH) {
            newCapacity = (newCapacity * MAREA_INTEGER_SIZE(2));
        }

        MAREA_Null newBlock = MAREA_NULLPTR;

        if (MAREA_HostContextAllocate(context, newCapacity, MAREA_INTEGER_SIZE(1), &newBlock) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
            MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

            return;
        }

        if (target->lengthInBytes > 0) {
            memcpy(newBlock, target->data, (size_t)target->lengthInBytes);
        }

        MAREA_HostContextFree(context, target->data);

        target->data = (MAREA_Character8 *)newBlock;

        target->capacityInBytes = newCapacity;
    }

    if (suffix->lengthInBytes > 0) {
        memcpy((target->data + target->lengthInBytes), suffix->data, (size_t)suffix->lengthInBytes);
    }

    target->lengthInBytes = REQUIRED_LENGTH;
}

void MAREA_CALL MAREA_CStringClear(MAREA_String *target)
{
    target->lengthInBytes = MAREA_INTEGER_SIZE(0);
}

MAREA_Sinteger32 MAREA_CALL MAREA_CStringCompare(const MAREA_String *left, const MAREA_String *right)
{
    const MAREA_IntegerSize MINIMUM_LENGTH = ((left->lengthInBytes < right->lengthInBytes) ? left->lengthInBytes : right->lengthInBytes);

    MAREA_Sinteger32 result = MAREA_SINTEGER_32(0);

    if (MINIMUM_LENGTH > 0) {
        result = (MAREA_Sinteger32)memcmp(left->data, right->data, (size_t)MINIMUM_LENGTH);
    }

    if (result != MAREA_SINTEGER_32(0)) {
        return result;
    }

    if (left->lengthInBytes < right->lengthInBytes) {
        return MAREA_SINTEGER_32(-1);
    }

    if (left->lengthInBytes > right->lengthInBytes) {
        return MAREA_SINTEGER_32(1);
    }

    return MAREA_SINTEGER_32(0);
}

MAREA_HOST_FUNCTION(MAREA_CStringLength, MAREA_HOST_RETURNS(uinteger64), MAREA_HOST_BORROW(MAREA_String));
MAREA_HOST_FUNCTION(MAREA_CStringAppend, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(MAREA_String), MAREA_HOST_BORROW(MAREA_String));
MAREA_HOST_FUNCTION(MAREA_CStringClear, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_BORROW_MUTABLE(MAREA_String));
MAREA_HOST_FUNCTION(MAREA_CStringCompare, MAREA_HOST_RETURNS(sinteger32), MAREA_HOST_BORROW(MAREA_String), MAREA_HOST_BORROW(MAREA_String));

MAREA_TypeRegistryExcept MAREA_CALL MAREA_CStringRegister(MAREA_TypeRegistry *registry)
{
    static const MAREA_Character8 NAMESPACE_TEXT[] = "c.string";
    static const MAREA_IntegerSize NAMESPACE_LENGTH = MAREA_INTEGER_SIZE(8);

    MAREA_TypeRegistryExcept except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_STRING, NAMESPACE_TEXT, NAMESPACE_LENGTH, "length", MAREA_INTEGER_SIZE(6), MAREA_CStringLength);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_STRING, NAMESPACE_TEXT, NAMESPACE_LENGTH, "append", MAREA_INTEGER_SIZE(6), MAREA_CStringAppend);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_STRING, NAMESPACE_TEXT, NAMESPACE_LENGTH, "clear", MAREA_INTEGER_SIZE(5), MAREA_CStringClear);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    return MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_STRING, NAMESPACE_TEXT, NAMESPACE_LENGTH, "compare", MAREA_INTEGER_SIZE(7), MAREA_CStringCompare);
}
