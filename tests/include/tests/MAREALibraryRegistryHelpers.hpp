// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREALIBRARYREGISTRYHELPERS_HPP)
    #define MAREA_TESTS_MAREALIBRARYREGISTRYHELPERS_HPP

    #include "tests/dependencies.hpp"

// INFO: the examples and the language catalogue write the old built-in names under the neutral fixture
// root demo (never c or marea: those two already carry other, unrelated pre-existing meanings in this
// suite, such as MAREASemaOwnership.c's own "a library storage type is written c.X" rule, which this
// must not disturb). This registers a fixture descriptor per name, through the internal entry (a
// shipped library), with the roles and the ownership class the old hard-coded rule gave it, so removing
// that rule from sema changes nothing an example or a catalogue item observes.
struct MAREA_LibraryDescriptorEntry
{
    MAREA_LibraryFlag owner;
    MAREA_TypeDescriptor descriptor;
};

inline const MAREA_LibraryDescriptorEntry *MAREA_libraryDescriptors(MAREA_IntegerSize *outCount)
{
    static const MAREA_LibraryDescriptorEntry ENTRIES[] = {
        { MAREA_LIBRARY_FLAG_MAREA_VECTOR, { "demo", MAREA_INTEGER_SIZE(4), "table", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(2), MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, false } },
        { MAREA_LIBRARY_FLAG_MAREA_ARRAY, { "demo", MAREA_INTEGER_SIZE(4), "array", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(2), MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_COPY, true } },
        { MAREA_LIBRARY_FLAG_MAREA_VECTOR, { "demo", MAREA_INTEGER_SIZE(4), "vector", MAREA_INTEGER_SIZE(6), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, true } },
        { MAREA_LIBRARY_FLAG_C_MEMORY, { "demo", MAREA_INTEGER_SIZE(4), "slice", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(1), (MAREA_TypeRole)(MAREA_TYPE_ROLE_VIEW | MAREA_TYPE_ROLE_PARTITION_PART), MAREA_TYPE_OWNERSHIP_CLASS_COPY, true } },
        { MAREA_LIBRARY_FLAG_C_MEMORY, { "demo", MAREA_INTEGER_SIZE(4), "pointer", MAREA_INTEGER_SIZE(7), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_RAW_POINTER, MAREA_TYPE_OWNERSHIP_CLASS_COPY, false } },
        { MAREA_LIBRARY_FLAG_C_MEMORY, { "demo", MAREA_INTEGER_SIZE(4), "owned", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_OWNER, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, false } },
        { MAREA_LIBRARY_FLAG_C_THREADS, { "demo", MAREA_INTEGER_SIZE(4), "task", MAREA_INTEGER_SIZE(4), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_SPAWN_HANDLE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, false } },
        { MAREA_LIBRARY_FLAG_MAREA_ARRAY, { "marea.array", MAREA_INTEGER_SIZE(11), "Array", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(2), MAREA_TYPE_ROLE_NONE, MAREA_TYPE_OWNERSHIP_CLASS_COPY, true } },
        { MAREA_LIBRARY_FLAG_C_MEMORY, { "c.memory", MAREA_INTEGER_SIZE(8), "Slice", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(1), (MAREA_TypeRole)(MAREA_TYPE_ROLE_VIEW | MAREA_TYPE_ROLE_PARTITION_PART), MAREA_TYPE_OWNERSHIP_CLASS_COPY, true } },
        { MAREA_LIBRARY_FLAG_C_MEMORY, { "c.memory", MAREA_INTEGER_SIZE(8), "Pointer", MAREA_INTEGER_SIZE(7), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_RAW_POINTER, MAREA_TYPE_OWNERSHIP_CLASS_COPY, false } },
        { MAREA_LIBRARY_FLAG_C_MEMORY, { "c.memory", MAREA_INTEGER_SIZE(8), "Owned", MAREA_INTEGER_SIZE(5), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_OWNER, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, false } },
        { MAREA_LIBRARY_FLAG_C_THREADS, { "c.threads", MAREA_INTEGER_SIZE(9), "Task", MAREA_INTEGER_SIZE(4), MAREA_INTEGER_SIZE(1), MAREA_TYPE_ROLE_SPAWN_HANDLE, MAREA_TYPE_OWNERSHIP_CLASS_MOVE, false } },
    };

    *outCount = MAREA_ARRAY_SIZE(ENTRIES);

    return ENTRIES;
}

// INFO: the one fixture macro, include_bytes, registered under the namespace of each fixture array type, the
// way the real macro belongs to marea.array. It asks the include resolver of the caller for the size of the
// file, so it never reads the disk.
inline MAREA_Boolean MAREA_CALL MAREA_expandFixtureIncludeBytes(const MAREA_MacroExpandRequest *request, MAREA_IntegerSize *outCount)
{
    if (request->includeSize == nullptr) {
        return MAREA_BOOLEAN_FALSE;
    }

    return request->includeSize(request->includeUserData, request->text, request->textLength, outCount);
}

struct MAREA_LibraryMacroEntry
{
    MAREA_LibraryFlag owner;
    MAREA_MacroDescriptor descriptor;
};

inline const MAREA_LibraryMacroEntry *MAREA_libraryMacros(MAREA_IntegerSize *outCount)
{
    static const MAREA_MacroArgumentKind ARGUMENTS[]{ MAREA_MACRO_ARGUMENT_KIND_STRING_LITERAL };

    MAREA_IntegerSize typeCount{ 0 };

    const MAREA_LibraryDescriptorEntry *types{ MAREA_libraryDescriptors(&typeCount) };

    static const MAREA_LibraryMacroEntry ENTRIES[] = {
        { MAREA_LIBRARY_FLAG_MAREA_ARRAY, { "demo.array", MAREA_INTEGER_SIZE(10), "include_bytes", MAREA_INTEGER_SIZE(13), ARGUMENTS, MAREA_INTEGER_SIZE(1), &types[1].descriptor, "uinteger8", MAREA_INTEGER_SIZE(9), &MAREA_expandFixtureIncludeBytes } },
        { MAREA_LIBRARY_FLAG_MAREA_ARRAY, { "marea.array", MAREA_INTEGER_SIZE(11), "include_bytes", MAREA_INTEGER_SIZE(13), ARGUMENTS, MAREA_INTEGER_SIZE(1), &types[7].descriptor, "uinteger8", MAREA_INTEGER_SIZE(9), &MAREA_expandFixtureIncludeBytes } },
    };

    *outCount = MAREA_ARRAY_SIZE(ENTRIES);

    return ENTRIES;
}

inline MAREA_TypeRegistry *MAREA_buildLibraryRegistry(MAREA_Arena *arena, MAREA_LibraryMask mask = MAREA_LIBRARY_MASK_DEFAULT)
{
    MAREA_TypeRegistry *registry{ nullptr };

    MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, mask, &registry));

    MAREA_IntegerSize count{ 0 };

    const MAREA_LibraryDescriptorEntry *entries{ MAREA_libraryDescriptors(&count) };

    for (MAREA_IntegerSize index{ 0 }; index < count; ++index) {
        MAREA_IGNORE(MAREA_TypeRegistryRegisterInternal(registry, entries[index].owner, &entries[index].descriptor));
    }

    MAREA_IntegerSize macroCount{ 0 };

    const MAREA_LibraryMacroEntry *macros{ MAREA_libraryMacros(&macroCount) };

    for (MAREA_IntegerSize index{ 0 }; index < macroCount; ++index) {
        MAREA_IGNORE(MAREA_TypeRegistryRegisterInternalMacro(registry, macros[index].owner, &macros[index].descriptor));
    }

    return registry;
}

#endif
