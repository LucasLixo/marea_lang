// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREACCHARACTER_H)
    #define MAREA_INCLUDE_BASE_MAREACCHARACTER_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief True for a letter (a to z, A to Z) or a decimal digit (0 to 9), ASCII only, independent of the locale.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsAlphanumeric(MAREA_Character8 value);

    /**
     * @brief True for a letter (a to z, A to Z), ASCII only, independent of the locale.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsAlphabetic(MAREA_Character8 value);

    /**
     * @brief True for a space or a horizontal tab only.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsBlank(MAREA_Character8 value);

    /**
     * @brief True for a control byte: 0 to 31, or 127.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsControl(MAREA_Character8 value);

    /**
     * @brief True for a decimal digit (0 to 9) only.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsDecimal(MAREA_Character8 value);

    /**
     * @brief True for a printable byte that is not a space: 33 to 126.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsGraphical(MAREA_Character8 value);

    /**
     * @brief True for a lowercase letter (a to z) only, ASCII only, independent of the locale.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsLowercase(MAREA_Character8 value);

    /**
     * @brief True for a printable byte, the space included: 32 to 126.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsPrintable(MAREA_Character8 value);

    /**
     * @brief True for a graphical byte that is not a letter or a digit.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsPunctuation(MAREA_Character8 value);

    /**
     * @brief True for an ASCII white space byte: space, horizontal tab, line feed, vertical tab, form
     * feed or carriage return.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsWhiteSpace(MAREA_Character8 value);

    /**
     * @brief True for an uppercase letter (A to Z) only, ASCII only, independent of the locale.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsUppercase(MAREA_Character8 value);

    /**
     * @brief True for a hexadecimal digit: 0 to 9, a to f or A to F.
     * @param value Byte to classify.
     * @return MAREA_BOOLEAN_TRUE or MAREA_BOOLEAN_FALSE.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Boolean MAREA_CALL MAREA_CCharacterIsHexadecimal(MAREA_Character8 value);

    /**
     * @brief Lowers an uppercase letter (A to Z) to its lowercase form; every other byte comes back
     * unchanged, ASCII only, independent of the locale.
     * @param value Byte to transform.
     * @return The lowered byte, or value itself when it is not an uppercase letter.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Character8 MAREA_CALL MAREA_CCharacterToLowercase(MAREA_Character8 value);

    /**
     * @brief Raises a lowercase letter (a to z) to its uppercase form; every other byte comes back
     * unchanged, ASCII only, independent of the locale.
     * @param value Byte to transform.
     * @return The raised byte, or value itself when it is not a lowercase letter.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Character8 MAREA_CALL MAREA_CCharacterToUppercase(MAREA_Character8 value);

    /**
     * @brief Registers the library c.character: the fourteen ASCII classification and case functions
     * above, each taking a character by value.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the c.character library is off in the mask
     * of the registry, which then holds none of its functions; otherwise the result of the registry
     * operation that failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_CCharacterRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREACCHARACTER_H */
