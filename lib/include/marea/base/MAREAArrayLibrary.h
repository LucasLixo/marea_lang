// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREAARRAYLIBRARY_H)
    #define MAREA_INCLUDE_BASE_MAREAARRAYLIBRARY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Registers the library marea.array: the type marea.array.Array<Element, Count> and the compile-time
     * macro include_bytes. The type is an inline run of Count elements that is held by value. The compiler
     * implements its members and its loops, because a generic type cannot be a C export. The macro takes one
     * string literal, a path relative to the source file, and gives an Array of uinteger8 whose count is the size
     * of the file. It refuses a path that can leave the project root: an absolute path, a drive letter or a ".."
     * segment. It learns the size only from the include function of the caller and never reads the disk.
     * @param registry Registry to change. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, also when the array library is off in the mask of the registry,
     * which then holds neither the type nor the macro; otherwise the result of the registry operation that
     * failed.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_ArrayLibraryRegister(MAREA_TypeRegistry * registry);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREAARRAYLIBRARY_H */
