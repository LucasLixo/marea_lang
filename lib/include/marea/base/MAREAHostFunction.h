// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAHOSTFUNCTION_H)
    #define MAREA_INCLUDE_BASE_MAREAHOSTFUNCTION_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"

    #include <string.h>

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The one shape of a C function that the language runtime calls. MAREA_HOST_FUNCTION makes a function
     * of this shape for a typed C function; nothing else needs to write one. context gives the allocator hooks and
     * the trap of the call. arguments holds one pointer for each parameter of the language function, in order. For
     * a borrowed value (a structure) it points at the bytes of the value itself, with the layout of the structure.
     * For a scalar it points at 8 bytes in the register form: an integer sign extended or zero extended to 64 bits,
     * a float32 or a float64 as a float64, a boolean as the 64-bit integer 0 or 1. result points at the storage of
     * the result: 8 zeroed bytes in the register form for a scalar result, or the zeroed bytes of the structure for
     * a structure result. result is null when the call has no result. result does not alias an argument. Every
     * pointer is valid only for the time of the call, and the function must not keep it.
     */
    typedef void(MAREA_CALL * MAREA_HostFunction)(MAREA_HostContext * context, const MAREA_Null *arguments, MAREA_Null result);

    /**
     * @brief Reads the 8 bytes of a scalar argument. For the code that MAREA_HOST_FUNCTION makes.
     * @param slot The 8 bytes of the argument. Must not be null.
     * @return The bits.
     */
    MAREA_FORCE_INLINE MAREA_Uinteger64 MAREA_CALL MAREA_HostDetailLoad(const MAREA_Null slot)
    {
        MAREA_Uinteger64 bits = MAREA_UINTEGER_64(0);

        memcpy(&bits, slot, sizeof(bits));

        return bits;
    }

    /**
     * @brief Writes the 8 bytes of a scalar result. For the code that MAREA_HOST_FUNCTION makes.
     * @param result The 8 bytes of the result, or null for a call that drops the result.
     * @param bits The bits in the register form.
     */
    MAREA_FORCE_INLINE void MAREA_CALL MAREA_HostDetailStore(MAREA_Null result, MAREA_Uinteger64 bits)
    {
        if (result != MAREA_NULLPTR) {
            memcpy(result, &bits, sizeof(bits));
        }
    }

    // INFO: the three templates below make the C type, the reader and the writer of one scalar of the language.
    #if !defined(MAREA_HOST_DETAIL_SIGNED)
        #define MAREA_HOST_DETAIL_SIGNED(scalar, type)                                                 \
            typedef type MAREA_HostType_##scalar;                                                      \
            MAREA_FORCE_INLINE type MAREA_CALL MAREA_HostRead_##scalar(const MAREA_Null slot)          \
            {                                                                                          \
                return (type)(MAREA_Sinteger64)MAREA_HostDetailLoad(slot);                             \
            }                                                                                          \
            MAREA_FORCE_INLINE void MAREA_CALL MAREA_HostWrite_##scalar(MAREA_Null result, type value) \
            {                                                                                          \
                MAREA_HostDetailStore(result, (MAREA_Uinteger64)(MAREA_Sinteger64)value);              \
            }
    #endif

    #if !defined(MAREA_HOST_DETAIL_UNSIGNED)
        #define MAREA_HOST_DETAIL_UNSIGNED(scalar, type)                                               \
            typedef type MAREA_HostType_##scalar;                                                      \
            MAREA_FORCE_INLINE type MAREA_CALL MAREA_HostRead_##scalar(const MAREA_Null slot)          \
            {                                                                                          \
                return (type)MAREA_HostDetailLoad(slot);                                               \
            }                                                                                          \
            MAREA_FORCE_INLINE void MAREA_CALL MAREA_HostWrite_##scalar(MAREA_Null result, type value) \
            {                                                                                          \
                MAREA_HostDetailStore(result, (MAREA_Uinteger64)value);                                \
            }
    #endif

    #if !defined(MAREA_HOST_DETAIL_FLOAT)
        #define MAREA_HOST_DETAIL_FLOAT(scalar, type)                                                  \
            typedef type MAREA_HostType_##scalar;                                                      \
            MAREA_FORCE_INLINE type MAREA_CALL MAREA_HostRead_##scalar(const MAREA_Null slot)          \
            {                                                                                          \
                const MAREA_Uinteger64 BITS = MAREA_HostDetailLoad(slot);                              \
                MAREA_Float64 wide = MAREA_FLOAT_64(0.0);                                              \
                memcpy(&wide, &BITS, sizeof(wide));                                                    \
                return (type)wide;                                                                     \
            }                                                                                          \
            MAREA_FORCE_INLINE void MAREA_CALL MAREA_HostWrite_##scalar(MAREA_Null result, type value) \
            {                                                                                          \
                const MAREA_Float64 WIDE = (MAREA_Float64)value;                                       \
                MAREA_Uinteger64 bits = MAREA_UINTEGER_64(0);                                          \
                memcpy(&bits, &WIDE, sizeof(bits));                                                    \
                MAREA_HostDetailStore(result, bits);                                                   \
            }
    #endif

    /**
     * @brief The C type of the scalar boolean of the language.
     */
    typedef MAREA_Boolean MAREA_HostType_boolean;

    /**
     * @brief Reads a boolean argument: any bit set is true. For the code that MAREA_HOST_FUNCTION makes.
     * @param slot The 8 bytes of the argument. Must not be null.
     * @return The value.
     */
    MAREA_FORCE_INLINE MAREA_Boolean MAREA_CALL MAREA_HostRead_boolean(const MAREA_Null slot)
    {
        return ((MAREA_HostDetailLoad(slot) != MAREA_UINTEGER_64(0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE);
    }

    /**
     * @brief Writes a boolean result as 0 or 1. For the code that MAREA_HOST_FUNCTION makes.
     * @param result The 8 bytes of the result, or null.
     * @param value The value.
     */
    MAREA_FORCE_INLINE void MAREA_CALL MAREA_HostWrite_boolean(MAREA_Null result, MAREA_Boolean value)
    {
        MAREA_HostDetailStore(result, (value ? MAREA_UINTEGER_64(1) : MAREA_UINTEGER_64(0)));
    }

    /**
     * @brief The C type, the reader and the writer of each other scalar of the language: MAREA_HostType_name,
     * MAREA_HostRead_name and MAREA_HostWrite_name, found by the language name. A reader gives the value of the
     * 8 bytes of an argument and a writer stores the 8 bytes of a result, or nothing for a null result.
     */
    MAREA_HOST_DETAIL_SIGNED(sinteger8, MAREA_Sinteger8)
    MAREA_HOST_DETAIL_SIGNED(sinteger16, MAREA_Sinteger16)
    MAREA_HOST_DETAIL_SIGNED(sinteger32, MAREA_Sinteger32)
    MAREA_HOST_DETAIL_SIGNED(sinteger64, MAREA_Sinteger64)
    MAREA_HOST_DETAIL_UNSIGNED(uinteger8, MAREA_Uinteger8)
    MAREA_HOST_DETAIL_UNSIGNED(uinteger16, MAREA_Uinteger16)
    MAREA_HOST_DETAIL_UNSIGNED(uinteger32, MAREA_Uinteger32)
    MAREA_HOST_DETAIL_UNSIGNED(uinteger64, MAREA_Uinteger64)
    MAREA_HOST_DETAIL_FLOAT(float32, MAREA_Float32)
    MAREA_HOST_DETAIL_FLOAT(float64, MAREA_Float64)

    /**
     * @brief The C type of the scalar character of the language.
     */
    typedef MAREA_Character8 MAREA_HostType_character;

    /**
     * @brief Reads a character argument: the low byte of the 8 bytes, zero extended while in register
     * form, independent of whether MAREA_Character8 is signed or unsigned on this target. For the code
     * that MAREA_HOST_FUNCTION makes.
     * @param slot The 8 bytes of the argument. Must not be null.
     * @return The value.
     */
    MAREA_FORCE_INLINE MAREA_Character8 MAREA_CALL MAREA_HostRead_character(const MAREA_Null slot)
    {
        return (MAREA_Character8)(MAREA_Uinteger8)MAREA_HostDetailLoad(slot);
    }

    /**
     * @brief Writes a character result zero extended to 8 bytes, never sign extended even when
     * MAREA_Character8 is a signed char on this target. For the code that MAREA_HOST_FUNCTION makes.
     * @param result The 8 bytes of the result, or null.
     * @param value The value.
     */
    MAREA_FORCE_INLINE void MAREA_CALL MAREA_HostWrite_character(MAREA_Null result, MAREA_Character8 value)
    {
        MAREA_HostDetailStore(result, (MAREA_Uinteger64)(MAREA_Uinteger8)value);
    }

    /**
     * @brief The most parameters of the language function that a binding takes: the parameter limit of the vm. The
     * context is not one of them.
     */
    #if !defined(MAREA_HOST_FUNCTION_PARAMETER_LIMIT)
        #define MAREA_HOST_FUNCTION_PARAMETER_LIMIT 32
    #endif

    /**
     * @brief A parameter of MAREA_HOST_FUNCTION: a primitive by value. scalar is the language name: boolean,
     * sinteger8 to sinteger64, uinteger8 to uinteger64, float32 or float64. The C parameter has the type of that
     * name (MAREA_Boolean, MAREA_Sinteger8, ... MAREA_Float64).
     */
    #if !defined(MAREA_HOST_VALUE)
        #define MAREA_HOST_VALUE(scalar) (mareaValue, scalar)
    #endif

    /**
     * @brief A parameter of MAREA_HOST_FUNCTION: a read-only borrow. The C parameter is const type *.
     */
    #if !defined(MAREA_HOST_BORROW)
        #define MAREA_HOST_BORROW(type) (mareaBorrow, type)
    #endif

    /**
     * @brief A parameter of MAREA_HOST_FUNCTION: a write borrow (mutable in the language). The C parameter is
     * type *.
     */
    #if !defined(MAREA_HOST_BORROW_MUTABLE)
        #define MAREA_HOST_BORROW_MUTABLE(type) (mareaMutable, type)
    #endif

    /**
     * @brief A parameter of MAREA_HOST_FUNCTION: the context of the call. The C parameter is MAREA_HostContext *.
     * It is not a parameter of the language function and takes no argument. A function that does not list it does
     * not get it.
     */
    #if !defined(MAREA_HOST_CONTEXT)
        #define MAREA_HOST_CONTEXT (mareaContextOfCall, mareaContextOfCall)
    #endif

    /**
     * @brief A result of MAREA_HOST_FUNCTION: no result. The C function returns void.
     */
    #if !defined(MAREA_HOST_RETURNS_NOTHING)
        #define MAREA_HOST_RETURNS_NOTHING (mareaNothing, mareaNothing)
    #endif

    /**
     * @brief A result of MAREA_HOST_FUNCTION: a primitive, which the C function returns by value. scalar is a name
     * of MAREA_HOST_VALUE.
     */
    #if !defined(MAREA_HOST_RETURNS)
        #define MAREA_HOST_RETURNS(scalar) (mareaScalar, scalar)
    #endif

    /**
     * @brief A result of MAREA_HOST_FUNCTION: a structure. The C function returns void and takes a last parameter
     * type * where it writes the result. The runtime gives zeroed bytes there.
     */
    #if !defined(MAREA_HOST_RETURNS_STRUCTURE)
        #define MAREA_HOST_RETURNS_STRUCTURE(type) (mareaStructure, type)
    #endif

    // INFO: from here to MAREA_HOST_WRAPPER, the preprocessor tools of the binding macro. A name that starts with MAREA_HOST_DETAIL_ is not part of the API.
    // INFO: MAREA_HOST_DETAIL_COUNT counts 1 to 34 arguments: the result and at most 33 items (the context and 32 parameters).
    #define MAREA_HOST_DETAIL_PICK(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, picked, ...) picked

    #define MAREA_HOST_DETAIL_COUNT(...) MAREA_HOST_DETAIL_PICK(__VA_ARGS__, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, ~)

    #define MAREA_HOST_DETAIL_SHAPE(...) MAREA_HOST_DETAIL_PICK(__VA_ARGS__, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaSome, mareaNone, ~)

    #define MAREA_HOST_DETAIL_FIRST(...) MAREA_HOST_DETAIL_FIRST_OF(__VA_ARGS__, ~)

    #define MAREA_HOST_DETAIL_FIRST_OF(first, ...) first

    // INFO: MAREA_HOST_DETAIL_EACH applies a macro to each item after the result, with no separator.
    #define MAREA_HOST_DETAIL_EACH_1(m, r)
    #define MAREA_HOST_DETAIL_EACH_2(m, r, p)       m(2, p)
    #define MAREA_HOST_DETAIL_EACH_3(m, r, p, ...)  m(3, p) MAREA_HOST_DETAIL_EACH_2(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_4(m, r, p, ...)  m(4, p) MAREA_HOST_DETAIL_EACH_3(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_5(m, r, p, ...)  m(5, p) MAREA_HOST_DETAIL_EACH_4(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_6(m, r, p, ...)  m(6, p) MAREA_HOST_DETAIL_EACH_5(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_7(m, r, p, ...)  m(7, p) MAREA_HOST_DETAIL_EACH_6(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_8(m, r, p, ...)  m(8, p) MAREA_HOST_DETAIL_EACH_7(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_9(m, r, p, ...)  m(9, p) MAREA_HOST_DETAIL_EACH_8(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_10(m, r, p, ...) m(10, p) MAREA_HOST_DETAIL_EACH_9(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_11(m, r, p, ...) m(11, p) MAREA_HOST_DETAIL_EACH_10(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_12(m, r, p, ...) m(12, p) MAREA_HOST_DETAIL_EACH_11(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_13(m, r, p, ...) m(13, p) MAREA_HOST_DETAIL_EACH_12(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_14(m, r, p, ...) m(14, p) MAREA_HOST_DETAIL_EACH_13(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_15(m, r, p, ...) m(15, p) MAREA_HOST_DETAIL_EACH_14(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_16(m, r, p, ...) m(16, p) MAREA_HOST_DETAIL_EACH_15(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_17(m, r, p, ...) m(17, p) MAREA_HOST_DETAIL_EACH_16(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_18(m, r, p, ...) m(18, p) MAREA_HOST_DETAIL_EACH_17(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_19(m, r, p, ...) m(19, p) MAREA_HOST_DETAIL_EACH_18(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_20(m, r, p, ...) m(20, p) MAREA_HOST_DETAIL_EACH_19(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_21(m, r, p, ...) m(21, p) MAREA_HOST_DETAIL_EACH_20(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_22(m, r, p, ...) m(22, p) MAREA_HOST_DETAIL_EACH_21(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_23(m, r, p, ...) m(23, p) MAREA_HOST_DETAIL_EACH_22(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_24(m, r, p, ...) m(24, p) MAREA_HOST_DETAIL_EACH_23(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_25(m, r, p, ...) m(25, p) MAREA_HOST_DETAIL_EACH_24(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_26(m, r, p, ...) m(26, p) MAREA_HOST_DETAIL_EACH_25(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_27(m, r, p, ...) m(27, p) MAREA_HOST_DETAIL_EACH_26(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_28(m, r, p, ...) m(28, p) MAREA_HOST_DETAIL_EACH_27(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_29(m, r, p, ...) m(29, p) MAREA_HOST_DETAIL_EACH_28(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_30(m, r, p, ...) m(30, p) MAREA_HOST_DETAIL_EACH_29(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_31(m, r, p, ...) m(31, p) MAREA_HOST_DETAIL_EACH_30(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_32(m, r, p, ...) m(32, p) MAREA_HOST_DETAIL_EACH_31(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_33(m, r, p, ...) m(33, p) MAREA_HOST_DETAIL_EACH_32(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH_34(m, r, p, ...) m(34, p) MAREA_HOST_DETAIL_EACH_33(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_EACH(m, ...)          MAREA_CONCATENATE(MAREA_HOST_DETAIL_EACH_, MAREA_HOST_DETAIL_COUNT(__VA_ARGS__))(m, __VA_ARGS__)

    // INFO: MAREA_HOST_DETAIL_LIST is MAREA_HOST_DETAIL_EACH with a comma between the items; it is used for one item or more.
    #define MAREA_HOST_DETAIL_LIST_2(m, r, p)       m(2, p)
    #define MAREA_HOST_DETAIL_LIST_3(m, r, p, ...)  m(3, p), MAREA_HOST_DETAIL_LIST_2(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_4(m, r, p, ...)  m(4, p), MAREA_HOST_DETAIL_LIST_3(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_5(m, r, p, ...)  m(5, p), MAREA_HOST_DETAIL_LIST_4(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_6(m, r, p, ...)  m(6, p), MAREA_HOST_DETAIL_LIST_5(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_7(m, r, p, ...)  m(7, p), MAREA_HOST_DETAIL_LIST_6(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_8(m, r, p, ...)  m(8, p), MAREA_HOST_DETAIL_LIST_7(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_9(m, r, p, ...)  m(9, p), MAREA_HOST_DETAIL_LIST_8(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_10(m, r, p, ...) m(10, p), MAREA_HOST_DETAIL_LIST_9(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_11(m, r, p, ...) m(11, p), MAREA_HOST_DETAIL_LIST_10(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_12(m, r, p, ...) m(12, p), MAREA_HOST_DETAIL_LIST_11(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_13(m, r, p, ...) m(13, p), MAREA_HOST_DETAIL_LIST_12(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_14(m, r, p, ...) m(14, p), MAREA_HOST_DETAIL_LIST_13(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_15(m, r, p, ...) m(15, p), MAREA_HOST_DETAIL_LIST_14(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_16(m, r, p, ...) m(16, p), MAREA_HOST_DETAIL_LIST_15(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_17(m, r, p, ...) m(17, p), MAREA_HOST_DETAIL_LIST_16(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_18(m, r, p, ...) m(18, p), MAREA_HOST_DETAIL_LIST_17(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_19(m, r, p, ...) m(19, p), MAREA_HOST_DETAIL_LIST_18(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_20(m, r, p, ...) m(20, p), MAREA_HOST_DETAIL_LIST_19(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_21(m, r, p, ...) m(21, p), MAREA_HOST_DETAIL_LIST_20(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_22(m, r, p, ...) m(22, p), MAREA_HOST_DETAIL_LIST_21(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_23(m, r, p, ...) m(23, p), MAREA_HOST_DETAIL_LIST_22(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_24(m, r, p, ...) m(24, p), MAREA_HOST_DETAIL_LIST_23(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_25(m, r, p, ...) m(25, p), MAREA_HOST_DETAIL_LIST_24(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_26(m, r, p, ...) m(26, p), MAREA_HOST_DETAIL_LIST_25(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_27(m, r, p, ...) m(27, p), MAREA_HOST_DETAIL_LIST_26(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_28(m, r, p, ...) m(28, p), MAREA_HOST_DETAIL_LIST_27(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_29(m, r, p, ...) m(29, p), MAREA_HOST_DETAIL_LIST_28(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_30(m, r, p, ...) m(30, p), MAREA_HOST_DETAIL_LIST_29(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_31(m, r, p, ...) m(31, p), MAREA_HOST_DETAIL_LIST_30(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_32(m, r, p, ...) m(32, p), MAREA_HOST_DETAIL_LIST_31(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_33(m, r, p, ...) m(33, p), MAREA_HOST_DETAIL_LIST_32(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST_34(m, r, p, ...) m(34, p), MAREA_HOST_DETAIL_LIST_33(m, r, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_LIST(m, ...)          MAREA_CONCATENATE(MAREA_HOST_DETAIL_LIST_, MAREA_HOST_DETAIL_COUNT(__VA_ARGS__))(m, __VA_ARGS__)

    // INFO: an item is a pair (kind, name). These open it and choose by the kind.
    #define MAREA_HOST_DETAIL_OPEN(kind, name) kind, name

    #define MAREA_HOST_DETAIL_TYPE_AT(at, item)             MAREA_HOST_DETAIL_TYPE_JOIN(MAREA_HOST_DETAIL_OPEN item)
    #define MAREA_HOST_DETAIL_TYPE_JOIN(...)                MAREA_HOST_DETAIL_TYPE_KIND(__VA_ARGS__)
    #define MAREA_HOST_DETAIL_TYPE_KIND(kind, name)         MAREA_HOST_DETAIL_TYPE_##kind(name)
    #define MAREA_HOST_DETAIL_TYPE_mareaValue(name)         MAREA_HostType_##name
    #define MAREA_HOST_DETAIL_TYPE_mareaBorrow(name)        const name *
    #define MAREA_HOST_DETAIL_TYPE_mareaMutable(name)       name *
    #define MAREA_HOST_DETAIL_TYPE_mareaContextOfCall(name) MAREA_HostContext *

    #define MAREA_HOST_DETAIL_LOCAL_AT(at, item)                 MAREA_HOST_DETAIL_LOCAL_JOIN(at, MAREA_HOST_DETAIL_OPEN item)
    #define MAREA_HOST_DETAIL_LOCAL_JOIN(...)                    MAREA_HOST_DETAIL_LOCAL_KIND(__VA_ARGS__)
    #define MAREA_HOST_DETAIL_LOCAL_KIND(at, kind, name)         MAREA_HOST_DETAIL_LOCAL_##kind(at, name)
    #define MAREA_HOST_DETAIL_LOCAL_mareaValue(at, name)         const MAREA_HostType_##name mareaAt##at = MAREA_HostRead_##name(mareaArguments[mareaNext++]);
    #define MAREA_HOST_DETAIL_LOCAL_mareaBorrow(at, name)        const name *const mareaAt##at = (const name *)mareaArguments[mareaNext++];
    #define MAREA_HOST_DETAIL_LOCAL_mareaMutable(at, name)       name *const mareaAt##at = (name *)mareaArguments[mareaNext++];
    #define MAREA_HOST_DETAIL_LOCAL_mareaContextOfCall(at, name) MAREA_HostContext *const mareaAt##at = mareaContext;

    #define MAREA_HOST_DETAIL_NAME_AT(at, item) mareaAt##at

    #define MAREA_HOST_DETAIL_ARGUMENT_AT(at, item)       MAREA_HOST_DETAIL_ARGUMENT_JOIN(MAREA_HOST_DETAIL_OPEN item)
    #define MAREA_HOST_DETAIL_ARGUMENT_JOIN(...)          MAREA_HOST_DETAIL_ARGUMENT_KIND(__VA_ARGS__)
    #define MAREA_HOST_DETAIL_ARGUMENT_KIND(kind, name)   MAREA_HOST_DETAIL_ARGUMENT_##kind
    #define MAREA_HOST_DETAIL_ARGUMENT_mareaValue         +1
    #define MAREA_HOST_DETAIL_ARGUMENT_mareaBorrow        +1
    #define MAREA_HOST_DETAIL_ARGUMENT_mareaMutable       +1
    #define MAREA_HOST_DETAIL_ARGUMENT_mareaContextOfCall +0

    #define MAREA_HOST_DETAIL_RETURN_mareaNothing(name)   void
    #define MAREA_HOST_DETAIL_RETURN_mareaScalar(name)    MAREA_HostType_##name
    #define MAREA_HOST_DETAIL_RETURN_mareaStructure(name) void

    // INFO: the parameter list of the C function, by the shape (mareaNone: no item) and the result kind.
    #define MAREA_HOST_DETAIL_PARAMETERS_mareaNone_mareaNothing(name, ...)   void
    #define MAREA_HOST_DETAIL_PARAMETERS_mareaNone_mareaScalar(name, ...)    void
    #define MAREA_HOST_DETAIL_PARAMETERS_mareaNone_mareaStructure(name, ...) name *
    #define MAREA_HOST_DETAIL_PARAMETERS_mareaSome_mareaNothing(name, ...)   MAREA_HOST_DETAIL_LIST(MAREA_HOST_DETAIL_TYPE_AT, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_PARAMETERS_mareaSome_mareaScalar(name, ...)    MAREA_HOST_DETAIL_LIST(MAREA_HOST_DETAIL_TYPE_AT, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_PARAMETERS_mareaSome_mareaStructure(name, ...) MAREA_HOST_DETAIL_LIST(MAREA_HOST_DETAIL_TYPE_AT, __VA_ARGS__), name *

    // INFO: a structure result of a call that drops it goes to a zeroed local of the wrapper.
    #define MAREA_HOST_DETAIL_OUT(name)                     \
        name mareaScratch;                                  \
        name *mareaOut = (name *)mareaResult;               \
        if (mareaOut == MAREA_NULLPTR) {                    \
            memset(&mareaScratch, 0, sizeof(mareaScratch)); \
            mareaOut = &mareaScratch;                       \
        }

    // INFO: the call of the C function, by the shape and the result kind.
    #define MAREA_HOST_DETAIL_CALL_mareaNone_mareaNothing(name, ...)   mareaTyped()
    #define MAREA_HOST_DETAIL_CALL_mareaNone_mareaScalar(name, ...)    MAREA_HostWrite_##name(mareaResult, mareaTyped())
    #define MAREA_HOST_DETAIL_CALL_mareaNone_mareaStructure(name, ...) MAREA_HOST_DETAIL_OUT(name) mareaTyped(mareaOut)
    #define MAREA_HOST_DETAIL_CALL_mareaSome_mareaNothing(name, ...)   mareaTyped(MAREA_HOST_DETAIL_LIST(MAREA_HOST_DETAIL_NAME_AT, __VA_ARGS__))
    #define MAREA_HOST_DETAIL_CALL_mareaSome_mareaScalar(name, ...)    MAREA_HostWrite_##name(mareaResult, mareaTyped(MAREA_HOST_DETAIL_LIST(MAREA_HOST_DETAIL_NAME_AT, __VA_ARGS__)))
    #define MAREA_HOST_DETAIL_CALL_mareaSome_mareaStructure(name, ...) MAREA_HOST_DETAIL_OUT(name) mareaTyped(MAREA_HOST_DETAIL_LIST(MAREA_HOST_DETAIL_NAME_AT, __VA_ARGS__), mareaOut)

    // INFO: C++ has no _Generic: there the initialization of the typed pointer in the wrapper is the check, and it is a hard error.
    #if defined(__cplusplus)
        #define MAREA_HOST_DETAIL_ASSERT(condition, message) static_assert(condition, message)
        #define MAREA_HOST_DETAIL_CHECK(function)
    #else
        #define MAREA_HOST_DETAIL_ASSERT(condition, message) MAREA_STATIC_ASSERT(condition, message)
        #define MAREA_HOST_DETAIL_CHECK(function)            MAREA_STATIC_ASSERT(_Generic(&(function), MAREA_HostSignature_##function: 1, default: 0), "MAREA_HOST_FUNCTION: the signature of the function does not match its description:");
    #endif

    #define MAREA_HOST_DETAIL_FUNCTION(function, shape, result, ...) MAREA_HOST_DETAIL_SPLIT(function, shape, MAREA_HOST_DETAIL_OPEN result, __VA_ARGS__)
    #define MAREA_HOST_DETAIL_SPLIT(...)                             MAREA_HOST_DETAIL_GENERATE(__VA_ARGS__)
    #define MAREA_HOST_DETAIL_GENERATE(function, shape, kind, name, ...)                                                                                              \
        typedef MAREA_HOST_DETAIL_RETURN_##kind(name)(MAREA_CALL * MAREA_HostSignature_##function)(MAREA_HOST_DETAIL_PARAMETERS_##shape##_##kind(name, __VA_ARGS__)); \
        MAREA_HOST_DETAIL_CHECK(function)                                                                                                                             \
        static MAREA_UNUSED void MAREA_CALL MAREA_HostFunction_##function(MAREA_HostContext *mareaContext, const MAREA_Null *mareaArguments, MAREA_Null mareaResult)  \
        {                                                                                                                                                             \
            const MAREA_HostSignature_##function mareaTyped = &(function);                                                                                            \
            MAREA_IntegerSize mareaNext = MAREA_INTEGER_SIZE(0);                                                                                                      \
            MAREA_HOST_DETAIL_EACH(MAREA_HOST_DETAIL_LOCAL_AT, __VA_ARGS__)                                                                                           \
            MAREA_HOST_DETAIL_CALL_##shape##_##kind(name, __VA_ARGS__);                                                                                               \
            MAREA_IGNORE(mareaContext);                                                                                                                               \
            MAREA_IGNORE(mareaArguments);                                                                                                                             \
            MAREA_IGNORE(mareaResult);                                                                                                                                \
            MAREA_IGNORE(mareaNext);                                                                                                                                  \
        }                                                                                                                                                             \
        MAREA_HOST_DETAIL_ASSERT((0 MAREA_HOST_DETAIL_EACH(MAREA_HOST_DETAIL_ARGUMENT_AT, __VA_ARGS__)) <= MAREA_HOST_FUNCTION_PARAMETER_LIMIT, "MAREA_HOST_FUNCTION: the vm calls a function with at most 32 parameters.")

    /**
     * @brief The name of the wrapper that MAREA_HOST_FUNCTION makes for a function, of the shape
     * MAREA_HostFunction. MAREA_REGISTER_FUNCTION uses it, so a function with no binding does not register.
     */
    #if !defined(MAREA_HOST_WRAPPER)
        #define MAREA_HOST_WRAPPER(function) MAREA_CONCATENATE(MAREA_HostFunction_, function)
    #endif

    /**
     * @brief The binding declaration of one typed C function: the file-scope macro that makes its wrapper. The
     * runtime calls the wrapper; a C caller calls the function itself. Write it after the declaration of the
     * function, with a semicolon, then register the function with MAREA_REGISTER_FUNCTION in the same file.
     *
     * function is the name of the function. The first argument after it describes the result:
     * MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_RETURNS(scalar) or MAREA_HOST_RETURNS_STRUCTURE(type). Each argument
     * after the result describes one parameter, in order: MAREA_HOST_VALUE(scalar), MAREA_HOST_BORROW(type),
     * MAREA_HOST_BORROW_MUTABLE(type) or MAREA_HOST_CONTEXT. The C signature must be exactly the one that these
     * describe, with MAREA_CALL: a type that differs, a missing or extra parameter, or a wrong result is a hard
     * compile error with every compiler, and so is a function with more than MAREA_HOST_FUNCTION_PARAMETER_LIMIT
     * parameters of the language.
     *
     * @code
     * static void MAREA_CALL demo_push(MAREA_HostContext *context, Bag *bag, MAREA_Sinteger32 value);
     * MAREA_HOST_FUNCTION(demo_push, MAREA_HOST_RETURNS_NOTHING, MAREA_HOST_CONTEXT, MAREA_HOST_BORROW_MUTABLE(Bag), MAREA_HOST_VALUE(sinteger32));
     * @endcode
     */
    #if !defined(MAREA_HOST_FUNCTION)
        #define MAREA_HOST_FUNCTION(function, ...) MAREA_HOST_DETAIL_FUNCTION(function, MAREA_HOST_DETAIL_SHAPE(__VA_ARGS__), MAREA_HOST_DETAIL_FIRST(__VA_ARGS__), __VA_ARGS__)
    #endif

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAHOSTFUNCTION_H */
