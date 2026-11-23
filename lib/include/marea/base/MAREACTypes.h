// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACTYPES_H)
    #define MAREA_INCLUDE_BASE_MAREACTYPES_H

    #include "marea/base/MAREAMacros.h"

    #if defined(__has_include) && !defined(MAREA_HAS_STDBOOL_H)
        #if __has_include(<stdbool.h>)
            #define MAREA_HAS_STDBOOL_H
        #endif
    #endif

    #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || (defined(_MSC_VER) && (_MSC_VER >= 1910)) || defined(MAREA_HAS_STDBOOL_H)
MAREA_BEGIN_IGNORE_PRAGMA_PACK

        #include <stdbool.h>

MAREA_END_IGNORE_PRAGMA
    #elif !defined(__bool_true_false_are_defined) && !defined(bool)
        // INFO: a pre-C99 compiler with no stdbool.h - emulate bool/true/false.
        #define bool                          unsigned char
        #define false                         0
        #define true                          1
        #define __bool_true_false_are_defined 1
    #endif

typedef bool MAREA_Boolean;

    #define MAREA_BOOLEAN_TRUE  true
    #define MAREA_BOOLEAN_FALSE false

MAREA_BEGIN_IGNORE_PRAGMA_PACK

    #include <stdint.h>
    #include <float.h>
    #include <stddef.h>
    #include <limits.h>

MAREA_END_IGNORE_PRAGMA

typedef int8_t MAREA_Sinteger8;
typedef int_least8_t MAREA_SintegerLeast8;
typedef int_fast8_t MAREA_SintegerFast8;

    #define MAREA_SINTEGER_8(value) INT8_C(value)

    #if defined(INT8_MAX)
        #define MAREA_SINTEGER_8_MAXIMUM INT8_MAX
    #else
        #define MAREA_SINTEGER_8_MAXIMUM ((MAREA_Sinteger8)0x7F)
    #endif

    #if defined(INT8_MIN)
        #define MAREA_SINTEGER_8_MINIMUM INT8_MIN
    #else
        #define MAREA_SINTEGER_8_MINIMUM ((MAREA_Sinteger8)(~0x7F))
    #endif

typedef int16_t MAREA_Sinteger16;
typedef int_least16_t MAREA_SintegerLeast16;
typedef int_fast16_t MAREA_SintegerFast16;

    #define MAREA_SINTEGER_16(value) INT16_C(value)

    #if defined(INT16_MAX)
        #define MAREA_SINTEGER_16_MAXIMUM INT16_MAX
    #else
        #define MAREA_SINTEGER_16_MAXIMUM ((MAREA_Sinteger16)0x7FFF)
    #endif

    #if defined(INT16_MIN)
        #define MAREA_SINTEGER_16_MINIMUM INT16_MIN
    #else
        #define MAREA_SINTEGER_16_MINIMUM ((MAREA_Sinteger16)(~0x7FFF))
    #endif

typedef int32_t MAREA_Sinteger32;
typedef int_least32_t MAREA_SintegerLeast32;
typedef int_fast32_t MAREA_SintegerFast32;

    #define MAREA_SINTEGER_32(value) INT32_C(value)

    #if defined(INT32_MAX)
        #define MAREA_SINTEGER_32_MAXIMUM INT32_MAX
    #else
        #define MAREA_SINTEGER_32_MAXIMUM ((MAREA_Sinteger32)0x7FFFFFFF)
    #endif

    #if defined(INT32_MIN)
        #define MAREA_SINTEGER_32_MINIMUM INT32_MIN
    #else
        #define MAREA_SINTEGER_32_MINIMUM ((MAREA_Sinteger32)(~0x7FFFFFFF))
    #endif

typedef int64_t MAREA_Sinteger64;
typedef int_least64_t MAREA_SintegerLeast64;
typedef int_fast64_t MAREA_SintegerFast64;

    #define MAREA_SINTEGER_64(value) INT64_C(value)

    #if defined(INT64_MAX)
        #define MAREA_SINTEGER_64_MAXIMUM INT64_MAX
    #else
        #define MAREA_SINTEGER_64_MAXIMUM ((MAREA_Sinteger64)0x7FFFFFFFFFFFFFFFLL)
    #endif

    #if defined(INT64_MIN)
        #define MAREA_SINTEGER_64_MINIMUM INT64_MIN
    #else
        #define MAREA_SINTEGER_64_MINIMUM (~((MAREA_Sinteger64)0x7FFFFFFFFFFFFFFFLL))
    #endif

typedef uint8_t MAREA_Uinteger8;
typedef uint_least8_t MAREA_UintegerLeast8;
typedef uint_fast8_t MAREA_UintegerFast8;

    #define MAREA_UINTEGER_8(value) UINT8_C(value)

    #if defined(UINT8_MAX)
        #define MAREA_UINTEGER_8_MAXIMUM UINT8_MAX
    #else
        #define MAREA_UINTEGER_8_MAXIMUM ((MAREA_Uinteger8)0xFF)
    #endif

    #define MAREA_UINTEGER_8_MINIMUM 0

typedef uint16_t MAREA_Uinteger16;
typedef uint_least16_t MAREA_UintegerLeast16;
typedef uint_fast16_t MAREA_UintegerFast16;

    #define MAREA_UINTEGER_16(value) UINT16_C(value)

    #if defined(UINT16_MAX)
        #define MAREA_UINTEGER_16_MAXIMUM UINT16_MAX
    #else
        #define MAREA_UINTEGER_16_MAXIMUM ((MAREA_Uinteger16)0xFFFF)
    #endif

    #define MAREA_UINTEGER_16_MINIMUM 0

typedef uint32_t MAREA_Uinteger32;
typedef uint_least32_t MAREA_UintegerLeast32;
typedef uint_fast32_t MAREA_UintegerFast32;

    #define MAREA_UINTEGER_32(value) UINT32_C(value)

    #if defined(UINT32_MAX)
        #define MAREA_UINTEGER_32_MAXIMUM UINT32_MAX
    #else
        #define MAREA_UINTEGER_32_MAXIMUM ((MAREA_Uinteger32)0xFFFFFFFFu)
    #endif

    #define MAREA_UINTEGER_32_MINIMUM 0

typedef uint64_t MAREA_Uinteger64;
typedef uint_least64_t MAREA_UintegerLeast64;
typedef uint_fast64_t MAREA_UintegerFast64;

    #define MAREA_UINTEGER_64(value) UINT64_C(value)

    #if defined(UINT64_MAX)
        #define MAREA_UINTEGER_64_MAXIMUM UINT64_MAX
    #else
        #define MAREA_UINTEGER_64_MAXIMUM ((MAREA_Uinteger64)0xFFFFFFFFFFFFFFFFULL)
    #endif

    #define MAREA_UINTEGER_64_MINIMUM 0

typedef MAREA_Uinteger32 MAREA_Hexadecimal;

    #define MAREA_HEXADECIMAL(value) (0x##value)

typedef uint8_t MAREA_Byte;
typedef uint16_t MAREA_Word;
typedef uint32_t MAREA_DoubleWord;
typedef uint64_t MAREA_QuadWord;

    #define MAREA_BINARY(value) 0b##value
    #define MAREA_BYTE(value)   0x##value

typedef size_t MAREA_IntegerSize;

    #if SIZE_MAX == UINT32_MAX
typedef uint_least32_t MAREA_IntegerSizeLeast;
typedef uint_fast32_t MAREA_IntegerSizeFast;
    #elif SIZE_MAX == UINT64_MAX
typedef uint_least64_t MAREA_IntegerSizeLeast;
typedef uint_fast64_t MAREA_IntegerSizeFast;
    #endif

    #define MAREA_INTEGER_SIZE(value)  ((MAREA_IntegerSize)(value))
    #define MAREA_INTEGER_SIZE_MAXIMUM SIZE_MAX
    #define MAREA_INTEGER_SIZE_MINIMUM 0

typedef MAREA_IntegerSize MAREA_IntegerID;

    #define MAREA_INTEGER_ID(value)  ((MAREA_IntegerSize)(value))
    #define MAREA_INTEGER_ID_MAXIMUM SIZE_MAX
    #define MAREA_INTEGER_ID_MINIMUM 0

    #define MAREA_NULL    0
    #define MAREA_NULLPTR ((void *)0)

typedef void *MAREA_Null;

typedef float MAREA_Float32;
typedef double MAREA_Float64;
typedef long double MAREA_Float80;

    #define MAREA_FLOAT_32(value)  ((MAREA_Float32)(value))
    #define MAREA_FLOAT_32_MAXIMUM FLT_MAX
    #define MAREA_FLOAT_32_MINIMUM (-FLT_MAX)

    #if defined(FLT_EPSILON)
        #define MAREA_FLOAT_32_EPSILON FLT_EPSILON
    #else
        // INFO: fallback value matches the IEEE-754 single-precision epsilon when <float.h> does not define FLT_EPSILON.
        #define MAREA_FLOAT_32_EPSILON 1.1920928955078125e-07F
    #endif

    #define MAREA_FLOAT_64(value)  ((MAREA_Float64)(value))
    #define MAREA_FLOAT_64_MAXIMUM DBL_MAX
    #define MAREA_FLOAT_64_MINIMUM (-DBL_MAX)

    #define MAREA_FLOAT_80(value)  ((MAREA_Float80)(value##L))
    #define MAREA_FLOAT_80_MAXIMUM LDBL_MAX
    #define MAREA_FLOAT_80_MINIMUM (-LDBL_MAX)

    #if SIZE_MAX == UINT32_MAX
typedef MAREA_Float32 MAREA_FloatSize;
        #define MAREA_FLOAT_SIZE(value)  MAREA_FLOAT_32(value)
        #define MAREA_FLOAT_SIZE_MAXIMUM MAREA_FLOAT_32_MAXIMUM
        #define MAREA_FLOAT_SIZE_MINIMUM MAREA_FLOAT_32_MINIMUM
    #elif SIZE_MAX == UINT64_MAX
typedef MAREA_Float64 MAREA_FloatSize;
        #define MAREA_FLOAT_SIZE(value)  MAREA_FLOAT_64(value)
        #define MAREA_FLOAT_SIZE_MAXIMUM MAREA_FLOAT_64_MAXIMUM
        #define MAREA_FLOAT_SIZE_MINIMUM MAREA_FLOAT_64_MINIMUM
    #endif

    #if SIZE_MAX == UINT64_MAX
        #define MAREA_PADDING(bytes64, bytes32) MAREA_Uinteger8 padding[bytes64];
        #define MAREA_PADDING_64(bytes)         MAREA_Uinteger8 padding[bytes];
        #define MAREA_PADDING_32(bytes)
    #elif SIZE_MAX == UINT32_MAX
        #define MAREA_PADDING(bytes64, bytes32) MAREA_Uinteger8 padding[bytes32];
        #define MAREA_PADDING_64(bytes)
        #define MAREA_PADDING_32(bytes) MAREA_Uinteger8 padding[bytes];
    #endif

MAREA_BEGIN_IGNORE_PRAGMA_PACK

    #include <inttypes.h>

MAREA_END_IGNORE_PRAGMA

    #if SIZE_MAX == UINT32_MAX
        #define MAREA_PRINT_PREFIX_MAX
    #elif SIZE_MAX == UINT64_MAX
        #define MAREA_PRINT_PREFIX_MAX "z"
    #endif

    #define MAREA_PRINT_DECIMAL_SINTEGER_8           PRId8
    #define MAREA_PRINT_INTEGER_SINTEGER_8           PRIi8
    #define MAREA_PRINT_HEXADECIMAL_SINTEGER_8       PRIx8
    #define MAREA_PRINT_HEXADECIMAL_UPPER_SINTEGER_8 PRIX8
    #define MAREA_PRINT_UNSIGNED_UINTEGER_8          PRIu8
    #define MAREA_PRINT_HEXADECIMAL_UINTEGER_8       PRIx8
    #define MAREA_PRINT_HEXADECIMAL_UPPER_UINTEGER_8 PRIX8
    #define MAREA_PRINT_OCTAL_UINTEGER_8             PRIo8

    #define MAREA_PRINT_DECIMAL_SINTEGER_16           PRId16
    #define MAREA_PRINT_INTEGER_SINTEGER_16           PRIi16
    #define MAREA_PRINT_HEXADECIMAL_SINTEGER_16       PRIx16
    #define MAREA_PRINT_HEXADECIMAL_UPPER_SINTEGER_16 PRIX16
    #define MAREA_PRINT_UNSIGNED_UINTEGER_16          PRIu16
    #define MAREA_PRINT_HEXADECIMAL_UINTEGER_16       PRIx16
    #define MAREA_PRINT_HEXADECIMAL_UPPER_UINTEGER_16 PRIX16
    #define MAREA_PRINT_OCTAL_UINTEGER_16             PRIo16

    #define MAREA_PRINT_DECIMAL_SINTEGER_32           PRId32
    #define MAREA_PRINT_INTEGER_SINTEGER_32           PRIi32
    #define MAREA_PRINT_HEXADECIMAL_SINTEGER_32       PRIx32
    #define MAREA_PRINT_HEXADECIMAL_UPPER_SINTEGER_32 PRIX32
    #define MAREA_PRINT_UNSIGNED_UINTEGER_32          PRIu32
    #define MAREA_PRINT_HEXADECIMAL_UINTEGER_32       PRIx32
    #define MAREA_PRINT_HEXADECIMAL_UPPER_UINTEGER_32 PRIX32
    #define MAREA_PRINT_OCTAL_UINTEGER_32             PRIo32

    #define MAREA_PRINT_DECIMAL_SINTEGER_64           PRId64
    #define MAREA_PRINT_INTEGER_SINTEGER_64           PRIi64
    #define MAREA_PRINT_HEXADECIMAL_SINTEGER_64       PRIx64
    #define MAREA_PRINT_HEXADECIMAL_UPPER_SINTEGER_64 PRIX64
    #define MAREA_PRINT_UNSIGNED_UINTEGER_64          PRIu64
    #define MAREA_PRINT_HEXADECIMAL_UINTEGER_64       PRIx64
    #define MAREA_PRINT_HEXADECIMAL_UPPER_UINTEGER_64 PRIX64
    #define MAREA_PRINT_OCTAL_UINTEGER_64             PRIo64

    #define MAREA_PRINT_UNSIGNED_BYTE          "hhu"
    #define MAREA_PRINT_HEXADECIMAL_BYTE       "hhx"
    #define MAREA_PRINT_HEXADECIMAL_UPPER_BYTE "hhX"
    #define MAREA_PRINT_OCTAL_BYTE             "hho"

    #define MAREA_PRINT_DECIMAL_INTEGER_SIZE           MAREA_PRINT_PREFIX_MAX "d"
    #define MAREA_PRINT_UNSIGNED_INTEGER_SIZE          MAREA_PRINT_PREFIX_MAX "u"
    #define MAREA_PRINT_HEXADECIMAL_INTEGER_SIZE       MAREA_PRINT_PREFIX_MAX "x"
    #define MAREA_PRINT_HEXADECIMAL_UPPER_INTEGER_SIZE MAREA_PRINT_PREFIX_MAX "X"
    #define MAREA_PRINT_OCTAL_INTEGER_SIZE             MAREA_PRINT_PREFIX_MAX "o"

    #define MAREA_PRINT_DECIMAL_INTEGER_ID           MAREA_PRINT_PREFIX_MAX "d"
    #define MAREA_PRINT_UNSIGNED_INTEGER_ID          MAREA_PRINT_PREFIX_MAX "u"
    #define MAREA_PRINT_HEXADECIMAL_INTEGER_ID       MAREA_PRINT_PREFIX_MAX "x"
    #define MAREA_PRINT_HEXADECIMAL_UPPER_INTEGER_ID MAREA_PRINT_PREFIX_MAX "X"
    #define MAREA_PRINT_OCTAL_INTEGER_ID             MAREA_PRINT_PREFIX_MAX "o"

    #define MAREA_PRINT_FIXED_FLOAT_32       "f"
    #define MAREA_PRINT_EXPONENTIAL_FLOAT_32 "e"
    #define MAREA_PRINT_GENERAL_FLOAT_32     "g"
    #define MAREA_PRINT_HEXADECIMAL_FLOAT_32 "a"

    #define MAREA_PRINT_FIXED_FLOAT_64       "lf"
    #define MAREA_PRINT_EXPONENTIAL_FLOAT_64 "le"
    #define MAREA_PRINT_GENERAL_FLOAT_64     "lg"
    #define MAREA_PRINT_HEXADECIMAL_FLOAT_64 "la"

    #if SIZE_MAX == UINT32_MAX
        #define MAREA_PRINT_FIXED_FLOAT_SIZE       MAREA_PRINT_FIXED_FLOAT_32
        #define MAREA_PRINT_EXPONENTIAL_FLOAT_SIZE MAREA_PRINT_EXPONENTIAL_FLOAT_32
        #define MAREA_PRINT_GENERAL_FLOAT_SIZE     MAREA_PRINT_GENERAL_FLOAT_32
        #define MAREA_PRINT_HEXADECIMAL_FLOAT_SIZE MAREA_PRINT_HEXADECIMAL_FLOAT_32
    #elif SIZE_MAX == UINT64_MAX
        #define MAREA_PRINT_FIXED_FLOAT_SIZE       MAREA_PRINT_FIXED_FLOAT_64
        #define MAREA_PRINT_EXPONENTIAL_FLOAT_SIZE MAREA_PRINT_EXPONENTIAL_FLOAT_64
        #define MAREA_PRINT_GENERAL_FLOAT_SIZE     MAREA_PRINT_GENERAL_FLOAT_64
        #define MAREA_PRINT_HEXADECIMAL_FLOAT_SIZE MAREA_PRINT_HEXADECIMAL_FLOAT_64
    #endif

    #define MAREA_PRINT_POINTER_POINTER     "p"
    #define MAREA_PRINT_CHARACTER_CHARACTER "c"
    #define MAREA_PRINT_STRING_STRING       "s"

    #if defined(SETTING_USE_SHORT_LIFE_BLOCK)
        #define MAREA_SHORT_LIFE_BLOCK for (MAREA_Boolean once = MAREA_BOOLEAN_TRUE; once; once = MAREA_BOOLEAN_FALSE)
    #else
        #define MAREA_SHORT_LIFE_BLOCK
    #endif

typedef char MAREA_Character8;
typedef wchar_t MAREA_CharacterWide;
typedef uint16_t MAREA_Character16;
typedef uint32_t MAREA_Character32;

    #define MAREA_CHARACTER_8(text)    text
    #define MAREA_CHARACTER_WIDE(text) L##text
    #define MAREA_CHARACTER_16(text)   u##text
    #define MAREA_CHARACTER_32(text)   U##text

#endif /* MAREA_INCLUDE_BASE_MAREACTYPES_H */
