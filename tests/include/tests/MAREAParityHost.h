// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAPARITYHOST_H)
    #define MAREA_TESTS_MAREAPARITYHOST_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREACMemoryRuntime.h"
    #include "marea/base/MAREAHostContext.h"

    #include <stdlib.h>
    #include <string.h>

// INFO: the host functions of the parity tests are one header, so the vm and the generated C run the same code. Every
// function writes what it saw to one log, which the tests compare between the two backends.
    #define MAREA_PARITY_LOG_LIMIT 1024

typedef struct marea_parity_pair
{
    MAREA_Sinteger32 x;
    MAREA_Sinteger32 y;
} marea_parity_pair;

typedef struct marea_parity_wide
{
    MAREA_Float64 first;
    MAREA_Float64 second;
} marea_parity_wide;

typedef struct marea_parity_five
{
    MAREA_Sinteger64 values[5];
} marea_parity_five;

// INFO: the generated C program is one file. The C++ tests are many files that share the inline helpers of the parity
// tests, so they share one log: with one log per file, a helper could read another log than the host function writes.
    #if defined(__cplusplus)
inline MAREA_Sinteger64 marea_parity_log[MAREA_PARITY_LOG_LIMIT];
inline int marea_parity_log_count;
    #else
static MAREA_UNUSED MAREA_Sinteger64 marea_parity_log[MAREA_PARITY_LOG_LIMIT];
static MAREA_UNUSED int marea_parity_log_count;
    #endif

static inline void MAREA_CALL marea_parity_record(MAREA_Sinteger32 value)
{
    if (marea_parity_log_count < MAREA_PARITY_LOG_LIMIT) {
        marea_parity_log[marea_parity_log_count] = value;
        marea_parity_log_count += 1;
    }
}

static inline MAREA_Sinteger32 MAREA_CALL marea_parity_add(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return (left + right);
}

static inline MAREA_Sinteger32 MAREA_CALL marea_parity_subtract(MAREA_Sinteger32 left, MAREA_Sinteger32 right)
{
    return (left - right);
}

static inline MAREA_Float64 MAREA_CALL marea_parity_wide_sum(MAREA_Uinteger64 first, MAREA_Sinteger8 second, MAREA_Float32 third)
{
    return (((MAREA_Float64)first + (MAREA_Float64)second) + (MAREA_Float64)third);
}

static inline MAREA_Sinteger32 MAREA_CALL marea_parity_identity(MAREA_Sinteger32 value)
{
    return value;
}

static inline MAREA_Float64 MAREA_CALL marea_parity_third(void)
{
    return (1.0 / 3.0);
}

static inline MAREA_Boolean MAREA_CALL marea_parity_negate(MAREA_Boolean value)
{
    return !value;
}

static inline void MAREA_CALL marea_parity_sum(const marea_parity_pair *left, const marea_parity_pair *right, marea_parity_pair *outSum)
{
    outSum->x = (left->x + right->x);
    outSum->y = (left->y + right->y);
}

static inline MAREA_Sinteger32 MAREA_CALL marea_parity_square(const marea_parity_pair *value)
{
    return ((value->x * value->x) + (value->y * value->y));
}

static inline void MAREA_CALL marea_parity_bump(marea_parity_pair *value)
{
    value->x += 1;

    marea_parity_record(0);
}

static inline void MAREA_CALL marea_parity_shift(marea_parity_pair *value, MAREA_Sinteger32 by)
{
    value->x += by;
    value->y -= by;
}

static inline MAREA_Uinteger64 MAREA_CALL marea_parity_address(const marea_parity_pair *value)
{
    return (MAREA_Uinteger64)(uintptr_t)value;
}

static inline void MAREA_CALL marea_parity_swap(const marea_parity_wide *value, marea_parity_wide *outSwapped)
{
    outSwapped->first = value->second;
    outSwapped->second = value->first;
}

static inline void MAREA_CALL marea_parity_reverse(const marea_parity_five *value, marea_parity_five *outReversed)
{
    for (int index = 0; index < 5; index += 1) {
        outReversed->values[index] = value->values[(4 - index)];
    }
}

static inline MAREA_Sinteger32 MAREA_CALL marea_parity_twice(MAREA_Sinteger32 value)
{
    return (value * 2);
}

static inline MAREA_Boolean MAREA_CALL marea_parity_block(MAREA_HostContext *context, MAREA_Uinteger64 size)
{
    MAREA_Null block = MAREA_NULLPTR;

    if (MAREA_HostContextAllocate(context, (MAREA_IntegerSize)size, 16, &block) != MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) {
        MAREA_HostContextRaiseTrap(context, MAREA_HOST_TRAP_OUT_OF_MEMORY);

        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_Byte *bytes = (MAREA_Byte *)block;
    MAREA_Boolean zeroed = MAREA_BOOLEAN_TRUE;

    for (MAREA_Uinteger64 index = 0; index < size; index += 1) {
        if (bytes[index] != 0) {
            zeroed = MAREA_BOOLEAN_FALSE;
        }

        bytes[index] = 0xFF;
    }

    MAREA_HostContextFree(context, block);

    return zeroed;
}

    #define MAREA_PARITY_PARAMETER(index) MAREA_Sinteger32 p##index

static inline MAREA_Sinteger64 MAREA_CALL marea_parity_thirtytwo(MAREA_PARITY_PARAMETER(1), MAREA_PARITY_PARAMETER(2), MAREA_PARITY_PARAMETER(3), MAREA_PARITY_PARAMETER(4), MAREA_PARITY_PARAMETER(5), MAREA_PARITY_PARAMETER(6), MAREA_PARITY_PARAMETER(7), MAREA_PARITY_PARAMETER(8), MAREA_PARITY_PARAMETER(9), MAREA_PARITY_PARAMETER(10), MAREA_PARITY_PARAMETER(11), MAREA_PARITY_PARAMETER(12), MAREA_PARITY_PARAMETER(13), MAREA_PARITY_PARAMETER(14), MAREA_PARITY_PARAMETER(15), MAREA_PARITY_PARAMETER(16), MAREA_PARITY_PARAMETER(17), MAREA_PARITY_PARAMETER(18), MAREA_PARITY_PARAMETER(19), MAREA_PARITY_PARAMETER(20), MAREA_PARITY_PARAMETER(21), MAREA_PARITY_PARAMETER(22), MAREA_PARITY_PARAMETER(23), MAREA_PARITY_PARAMETER(24), MAREA_PARITY_PARAMETER(25), MAREA_PARITY_PARAMETER(26), MAREA_PARITY_PARAMETER(27), MAREA_PARITY_PARAMETER(28), MAREA_PARITY_PARAMETER(29), MAREA_PARITY_PARAMETER(30), MAREA_PARITY_PARAMETER(31), MAREA_PARITY_PARAMETER(32))
{
    const MAREA_Sinteger32 VALUES[32] = { p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32 };

    MAREA_Sinteger64 weighted = 0;

    for (int index = 0; index < 32; index += 1) {
        weighted += ((MAREA_Sinteger64)(index + 1) * VALUES[index]);
    }

    return weighted;
}

    #undef MAREA_PARITY_PARAMETER

// INFO: the two functions below replace the runtime hooks "alloc" and "free": they log each use of the allocator.
static inline MAREA_Null MAREA_CALL marea_parity_allocate(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
{
    marea_parity_record((MAREA_Sinteger32)(1000 + (MAREA_Sinteger32)size));

    const MAREA_IntegerSize ROUNDED = (((size + alignment) - 1) / alignment) * alignment;

    return aligned_alloc(alignment, ROUNDED);
}

static inline void MAREA_CALL marea_parity_release(MAREA_Null address)
{
    marea_parity_record(2000);

    free(address);
}

static inline MAREA_Null MAREA_CALL marea_parity_no_block(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
{
    MAREA_IGNORE(alignment);

    marea_parity_record((MAREA_Sinteger32)(3000 + (MAREA_Sinteger32)size));

    return MAREA_NULLPTR;
}

#endif /* MAREA_TESTS_MAREAPARITYHOST_H */
