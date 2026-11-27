// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREASHIPPEDLIBRARIES_HPP)
    #define MAREA_TESTS_MAREASHIPPEDLIBRARIES_HPP

    #include "tests/dependencies.hpp"

    #include "marea/base/MAREATypeRegistry.h"

    // INFO: the header of a container library that does not exist yet makes its registration answer false: the tests that
    // need it fail with a message and every other test still builds. The change that writes a header removes its guard.
    #include "marea/base/MAREAArrayLibrary.h"

    #include "marea/base/MAREAVectorLibrary.h"

    #if __has_include("marea/base/MAREATupleLibrary.h")
        #include "marea/base/MAREATupleLibrary.h"
        #define MAREA_TESTS_HAS_TUPLE_LIBRARY 1
    #endif

inline bool MAREA_registerArrayLibrary(MAREA_TypeRegistry *registry)
{
    return MAREA_ArrayLibraryRegister(registry) == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
}

inline bool MAREA_registerVectorLibrary(MAREA_TypeRegistry *registry)
{
    return MAREA_VectorLibraryRegister(registry) == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
}

inline bool MAREA_registerTupleLibrary(MAREA_TypeRegistry *registry)
{
    #if defined(MAREA_TESTS_HAS_TUPLE_LIBRARY)
    return MAREA_TupleLibraryRegister(registry) == MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
    #else
    MAREA_IGNORE(registry);

    return false;
    #endif
}

// INFO: the registry of the shipped container libraries alone, with the mask that the test gives.
struct MAREA_ShippedRegistry
{
    MAREA_Arena *arena{ nullptr };
    MAREA_TypeRegistry *registry{ nullptr };

    explicit MAREA_ShippedRegistry(MAREA_LibraryMask mask = MAREA_LIBRARY_MASK_DEFAULT)
    {
        MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
        MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, mask, &registry));
    }

    MAREA_ShippedRegistry(const MAREA_ShippedRegistry &) = delete;
    MAREA_ShippedRegistry &operator=(const MAREA_ShippedRegistry &) = delete;

    ~MAREA_ShippedRegistry()
    {
        MAREA_ArenaDestroy(arena);
    }

    const MAREA_TypeDescriptor *MAREA_find(const std::string &namespaceText, const std::string &name) const
    {
        return MAREA_TypeRegistryFind(registry, namespaceText.c_str(), namespaceText.size(), name.c_str(), name.size());
    }
};

#endif
