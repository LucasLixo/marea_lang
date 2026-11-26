// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREAArrayLibrary.h"

static const MAREA_TypeDescriptor MAREA_G_ARRAY = {
    .namespaceText = "marea.array",
    .namespaceLength = MAREA_INTEGER_SIZE(11),
    .name = "Array",
    .nameLength = MAREA_INTEGER_SIZE(5),
    .typeParameterCount = MAREA_INTEGER_SIZE(2),
    .roles = MAREA_TYPE_ROLE_NONE,
    .ownershipClass = MAREA_TYPE_OWNERSHIP_CLASS_COPY,
    .hasIndexProtocol = MAREA_BOOLEAN_TRUE,
};

static MAREA_Boolean MAREA_CALL MAREA_G_IsSeparator(MAREA_Character8 symbol)
{
    return ((symbol == '/') || (symbol == '\\')) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: sema does not know the project root, so every form that can leave the directory of the source file upward
// is refused: an absolute path, a drive letter and a ".." segment (decision D12.1).
static MAREA_Boolean MAREA_CALL MAREA_G_StaysInside(const MAREA_Character8 *path, MAREA_IntegerSize length)
{
    if ((length == MAREA_INTEGER_SIZE(0)) || (MAREA_G_IsSeparator(path[0]) == MAREA_BOOLEAN_TRUE) || ((length > MAREA_INTEGER_SIZE(1)) && (path[1] == ':'))) {
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_IntegerSize start = MAREA_INTEGER_SIZE(0);

    for (MAREA_IntegerSize index = MAREA_INTEGER_SIZE(0); index <= length; ++index) {
        if ((index < length) && (MAREA_G_IsSeparator(path[index]) == MAREA_BOOLEAN_FALSE)) {
            continue;
        }

        if (((index - start) == MAREA_INTEGER_SIZE(2)) && (path[start] == '.') && (path[start + 1] == '.')) {
            return MAREA_BOOLEAN_FALSE;
        }

        start = (index + MAREA_INTEGER_SIZE(1));
    }

    return MAREA_BOOLEAN_TRUE;
}

static MAREA_Boolean MAREA_CALL MAREA_G_ExpandIncludeBytes(const MAREA_MacroExpandRequest *request, MAREA_IntegerSize *outCount)
{
    if ((request->includeSize == MAREA_NULLPTR) || (MAREA_G_StaysInside(request->text, request->textLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_BOOLEAN_FALSE;
    }

    return request->includeSize(request->includeUserData, request->text, request->textLength, outCount);
}

static const MAREA_MacroArgumentKind MAREA_G_INCLUDE_BYTES_ARGUMENTS[] = { MAREA_MACRO_ARGUMENT_KIND_STRING_LITERAL };

static const MAREA_MacroDescriptor MAREA_G_INCLUDE_BYTES = {
    .namespaceText = "marea.array",
    .namespaceLength = MAREA_INTEGER_SIZE(11),
    .name = "include_bytes",
    .nameLength = MAREA_INTEGER_SIZE(13),
    .argumentKinds = MAREA_G_INCLUDE_BYTES_ARGUMENTS,
    .argumentCount = MAREA_INTEGER_SIZE(1),
    .resultType = &MAREA_G_ARRAY,
    .resultElementText = "uinteger8",
    .resultElementLength = MAREA_INTEGER_SIZE(9),
    .expand = &MAREA_G_ExpandIncludeBytes,
};

MAREA_TypeRegistryExcept MAREA_CALL MAREA_ArrayLibraryRegister(MAREA_TypeRegistry *registry)
{
    const MAREA_TypeRegistryExcept TYPE_RESULT = MAREA_TypeRegistryRegisterInternal(registry, MAREA_LIBRARY_FLAG_MAREA_ARRAY, &MAREA_G_ARRAY);

    if (TYPE_RESULT != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        return TYPE_RESULT;
    }

    return MAREA_TypeRegistryRegisterInternalMacro(registry, MAREA_LIBRARY_FLAG_MAREA_ARRAY, &MAREA_G_INCLUDE_BYTES);
}
