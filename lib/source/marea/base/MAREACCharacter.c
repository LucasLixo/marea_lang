// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREACCharacter.h"

#include "marea/base/MAREAHostFunction.h"

// INFO: every range below is an explicit ASCII byte range, never a <ctype.h> call, so the result is
// the same whatever the C locale of the caller.

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsAlphabetic(MAREA_Character8 value)
{
    return (((value >= 'a') && (value <= 'z')) || ((value >= 'A') && (value <= 'Z'))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsDecimal(MAREA_Character8 value)
{
    return ((value >= '0') && (value <= '9')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsAlphanumeric(MAREA_Character8 value)
{
    return ((MAREA_CCharacterIsAlphabetic(value) == MAREA_BOOLEAN_TRUE) || (MAREA_CCharacterIsDecimal(value) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsBlank(MAREA_Character8 value)
{
    return ((value == ' ') || (value == '\t')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsControl(MAREA_Character8 value)
{
    return (((value >= 0) && (value <= 31)) || (value == 127)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsGraphical(MAREA_Character8 value)
{
    return ((value >= 33) && (value <= 126)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsLowercase(MAREA_Character8 value)
{
    return ((value >= 'a') && (value <= 'z')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsPrintable(MAREA_Character8 value)
{
    return ((value >= 32) && (value <= 126)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsPunctuation(MAREA_Character8 value)
{
    return ((MAREA_CCharacterIsGraphical(value) == MAREA_BOOLEAN_TRUE) && (MAREA_CCharacterIsAlphanumeric(value) == MAREA_BOOLEAN_FALSE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsWhiteSpace(MAREA_Character8 value)
{
    return ((value == ' ') || (value == '\t') || (value == '\n') || (value == '\v') || (value == '\f') || (value == '\r')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsUppercase(MAREA_Character8 value)
{
    return ((value >= 'A') && (value <= 'Z')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MAREA_CALL MAREA_CCharacterIsHexadecimal(MAREA_Character8 value)
{
    return ((MAREA_CCharacterIsDecimal(value) == MAREA_BOOLEAN_TRUE) || ((value >= 'a') && (value <= 'f')) || ((value >= 'A') && (value <= 'F'))) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

MAREA_Character8 MAREA_CALL MAREA_CCharacterToLowercase(MAREA_Character8 value)
{
    return (MAREA_CCharacterIsUppercase(value) == MAREA_BOOLEAN_TRUE) ? (MAREA_Character8)((value - 'A') + 'a') : value;
}

MAREA_Character8 MAREA_CALL MAREA_CCharacterToUppercase(MAREA_Character8 value)
{
    return (MAREA_CCharacterIsLowercase(value) == MAREA_BOOLEAN_TRUE) ? (MAREA_Character8)((value - 'a') + 'A') : value;
}

MAREA_HOST_FUNCTION(MAREA_CCharacterIsAlphanumeric, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsAlphabetic, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsBlank, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsControl, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsDecimal, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsGraphical, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsLowercase, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsPrintable, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsPunctuation, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsWhiteSpace, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsUppercase, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterIsHexadecimal, MAREA_HOST_RETURNS(boolean), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterToLowercase, MAREA_HOST_RETURNS(character), MAREA_HOST_VALUE(character));
MAREA_HOST_FUNCTION(MAREA_CCharacterToUppercase, MAREA_HOST_RETURNS(character), MAREA_HOST_VALUE(character));

MAREA_TypeRegistryExcept MAREA_CALL MAREA_CCharacterRegister(MAREA_TypeRegistry *registry)
{
    static const MAREA_Character8 NAMESPACE_TEXT[] = "c.character";
    static const MAREA_IntegerSize NAMESPACE_LENGTH = MAREA_INTEGER_SIZE(11);

    MAREA_TypeRegistryExcept except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_alphanumeric", MAREA_INTEGER_SIZE(15), MAREA_CCharacterIsAlphanumeric);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_alphabetic", MAREA_INTEGER_SIZE(13), MAREA_CCharacterIsAlphabetic);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_blank", MAREA_INTEGER_SIZE(8), MAREA_CCharacterIsBlank);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_control", MAREA_INTEGER_SIZE(10), MAREA_CCharacterIsControl);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_decimal", MAREA_INTEGER_SIZE(10), MAREA_CCharacterIsDecimal);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_graphical", MAREA_INTEGER_SIZE(12), MAREA_CCharacterIsGraphical);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_lowercase", MAREA_INTEGER_SIZE(12), MAREA_CCharacterIsLowercase);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_printable", MAREA_INTEGER_SIZE(12), MAREA_CCharacterIsPrintable);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_punctuation", MAREA_INTEGER_SIZE(14), MAREA_CCharacterIsPunctuation);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_white_space", MAREA_INTEGER_SIZE(14), MAREA_CCharacterIsWhiteSpace);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_uppercase", MAREA_INTEGER_SIZE(12), MAREA_CCharacterIsUppercase);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "is_hexadecimal", MAREA_INTEGER_SIZE(14), MAREA_CCharacterIsHexadecimal);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    except = MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "to_lowercase", MAREA_INTEGER_SIZE(12), MAREA_CCharacterToLowercase);

    if (except != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return except;
    }

    return MAREA_REGISTER_FUNCTION(registry, MAREA_LIBRARY_FLAG_C_CHARACTER, NAMESPACE_TEXT, NAMESPACE_LENGTH, "to_uppercase", MAREA_INTEGER_SIZE(12), MAREA_CCharacterToUppercase);
}
