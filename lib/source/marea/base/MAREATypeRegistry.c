// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/base/MAREATypeRegistry.h"

#include <string.h>

// INFO: one registered type descriptor.
typedef struct MAREA_G_TypeEntry
{
    const MAREA_TypeDescriptor *descriptor;
} MAREA_G_TypeEntry;

// INFO: one registered C function, under its namespace and its name.
typedef struct MAREA_G_FunctionEntry
{
    const MAREA_Character8 *namespaceText;
    const MAREA_Character8 *name;
    MAREA_Null functionPointer;
    MAREA_IntegerSize namespaceLength;
    MAREA_IntegerSize nameLength;
} MAREA_G_FunctionEntry;

// INFO: one declared dependency: owner may register through the internal entry only when every
// library of dependsOn is on.
typedef struct MAREA_G_DependencyEntry
{
    MAREA_LibraryFlag owner;
    MAREA_LibraryMask dependsOn;
} MAREA_G_DependencyEntry;

// INFO: one runtime hook a host may replace, and its current implementation.
typedef struct MAREA_G_RuntimeHookEntry
{
    const MAREA_Character8 *name;
    MAREA_IntegerSize nameLength;
    MAREA_Null functionPointer;
} MAREA_G_RuntimeHookEntry;

// INFO: the runtime hooks the registry declares; a host may replace one through
// MAREA_TypeRegistryReplaceRuntimeFunction, never by registering a new name under c or marea.
static const MAREA_Character8 *const MAREA_G_RUNTIME_HOOK_NAMES[] = { "alloc", "free" };

struct MAREA_TypeRegistry
{
    MAREA_Arena *arena;
    MAREA_G_TypeEntry *types;
    MAREA_G_FunctionEntry *functions;
    MAREA_G_DependencyEntry *dependencies;
    MAREA_G_RuntimeHookEntry *hooks;
    MAREA_IntegerSize typeCount;
    MAREA_IntegerSize typeCapacity;
    MAREA_IntegerSize functionCount;
    MAREA_IntegerSize functionCapacity;
    MAREA_IntegerSize dependencyCount;
    MAREA_IntegerSize dependencyCapacity;
    MAREA_IntegerSize hookCount;
    MAREA_IntegerSize hookCapacity;
    MAREA_LibraryMask enabledMask;
};

static MAREA_Boolean MAREA_CALL MAREA_G_TextEquals(const MAREA_Character8 *left, MAREA_IntegerSize leftLength, const MAREA_Character8 *right, MAREA_IntegerSize rightLength)
{
    return ((leftLength == rightLength) && (memcmp(left, right, leftLength) == 0)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: the first dotted segment of namespaceText, the part before the first '.' or the whole text when
// it has none.
static MAREA_IntegerSize MAREA_CALL MAREA_G_FirstSegmentLength(const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength)
{
    for (MAREA_IntegerSize index = 0; index < namespaceLength; ++index) {
        if (namespaceText[index] == '.') {
            return index;
        }
    }

    return namespaceLength;
}

// INFO: c and marea are the two roots the language reserves for the libraries it ships; the public
// entry never registers a name under either one.
static MAREA_Boolean MAREA_CALL MAREA_G_IsReservedRoot(const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength)
{
    const MAREA_IntegerSize FIRST_LENGTH = MAREA_G_FirstSegmentLength(namespaceText, namespaceLength);

    return ((MAREA_G_TextEquals(namespaceText, FIRST_LENGTH, "c", MAREA_INTEGER_SIZE(1)) == MAREA_BOOLEAN_TRUE) || (MAREA_G_TextEquals(namespaceText, FIRST_LENGTH, "marea", MAREA_INTEGER_SIZE(5)) == MAREA_BOOLEAN_TRUE)) ? MAREA_BOOLEAN_TRUE : MAREA_BOOLEAN_FALSE;
}

// INFO: every dependency declared for owner, combined with bitwise or; a library with no declared
// dependency needs none.
static MAREA_LibraryMask MAREA_CALL MAREA_G_DependenciesOf(const MAREA_TypeRegistry *registry, MAREA_LibraryFlag owner)
{
    MAREA_LibraryMask required = MAREA_LIBRARY_FLAG_NONE;

    for (MAREA_IntegerSize index = 0; index < registry->dependencyCount; ++index) {
        if (registry->dependencies[index].owner == owner) {
            required |= registry->dependencies[index].dependsOn;
        }
    }

    return required;
}

MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryCreate(MAREA_Arena *arena, MAREA_LibraryMask enabledMask, MAREA_TypeRegistry **outRegistry)
{
    if ((arena == MAREA_NULLPTR) || (outRegistry == MAREA_NULLPTR)) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_TypeRegistry), MAREA_ALIGNOF(MAREA_TypeRegistry), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_TypeRegistry *registry = memory;

    registry->arena = arena;
    registry->types = MAREA_NULLPTR;
    registry->functions = MAREA_NULLPTR;
    registry->dependencies = MAREA_NULLPTR;
    registry->hooks = MAREA_NULLPTR;
    registry->typeCount = 0;
    registry->typeCapacity = 0;
    registry->functionCount = 0;
    registry->functionCapacity = 0;
    registry->dependencyCount = 0;
    registry->dependencyCapacity = 0;
    registry->hookCount = 0;
    registry->hookCapacity = 0;
    registry->enabledMask = enabledMask;

    *outRegistry = registry;

    return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
}

MAREA_LibraryMask MAREA_CALL MAREA_TypeRegistryGetEnabledMask(const MAREA_TypeRegistry *registry)
{
    return (registry == MAREA_NULLPTR) ? MAREA_LIBRARY_FLAG_NONE : registry->enabledMask;
}

// INFO: generic grow-by-doubling append, the same idiom MAREA_SemaTableAppend uses, specialized once
// per table through a macro so every table keeps its own element type.
#define MAREA_G_APPEND(registry, entries, count, capacity, elementType, value, outExcept)                                                                 \
    do {                                                                                                                                                   \
        if ((count) == (capacity)) {                                                                                                                       \
            const MAREA_IntegerSize MAREA_G_NEW_CAPACITY = ((capacity) == 0) ? MAREA_INTEGER_SIZE(8) : ((capacity) * MAREA_INTEGER_SIZE(2));               \
            MAREA_Null MAREA_G_MEMORY = MAREA_NULLPTR;                                                                                                      \
            if (MAREA_ArenaAllocate((registry)->arena, (MAREA_G_NEW_CAPACITY * sizeof(elementType)), MAREA_ALIGNOF(elementType), &MAREA_G_MEMORY) != MAREA_ARENA_EXCEPT_SUCCESS) { \
                *(outExcept) = MAREA_TYPE_REGISTRY_EXCEPT_ERROR_OUT_OF_MEMORY;                                                                              \
                break;                                                                                                                                      \
            }                                                                                                                                                \
            elementType *MAREA_G_NEW_ENTRIES = MAREA_G_MEMORY;                                                                                              \
            if ((count) > 0) {                                                                                                                              \
                memcpy(MAREA_G_NEW_ENTRIES, (entries), ((count) * sizeof(elementType)));                                                                   \
            }                                                                                                                                                \
            (entries) = MAREA_G_NEW_ENTRIES;                                                                                                                 \
            (capacity) = MAREA_G_NEW_CAPACITY;                                                                                                               \
        }                                                                                                                                                    \
        (entries)[(count)] = (value);                                                                                                                       \
        (count) += 1;                                                                                                                                        \
        *(outExcept) = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;                                                                                                   \
    } while (MAREA_BOOLEAN_FALSE)

MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryDeclareDependency(MAREA_TypeRegistry *registry, MAREA_LibraryFlag owner, MAREA_LibraryMask dependsOn)
{
    if (registry == MAREA_NULLPTR) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    const MAREA_G_DependencyEntry ENTRY = { .owner = owner, .dependsOn = dependsOn };

    MAREA_G_APPEND(registry, registry->dependencies, registry->dependencyCount, registry->dependencyCapacity, MAREA_G_DependencyEntry, ENTRY, &except);

    return except;
}

// INFO: a library whose bit is off in the enabled mask registers nothing: a no-op success, so the
// caller never has to ask the mask itself before registering.
MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterInternal(MAREA_TypeRegistry *registry, MAREA_LibraryFlag owner, const MAREA_TypeDescriptor *descriptor)
{
    if ((registry == MAREA_NULLPTR) || (descriptor == MAREA_NULLPTR)) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((registry->enabledMask & owner) == MAREA_LIBRARY_FLAG_NONE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
    }

    const MAREA_LibraryMask REQUIRED = MAREA_G_DependenciesOf(registry, owner);

    if ((REQUIRED & ~registry->enabledMask) != MAREA_LIBRARY_FLAG_NONE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY;
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    const MAREA_G_TypeEntry ENTRY = { .descriptor = descriptor };

    MAREA_G_APPEND(registry, registry->types, registry->typeCount, registry->typeCapacity, MAREA_G_TypeEntry, ENTRY, &except);

    return except;
}

// INFO: the public entry, used by a user or a host, never registers a name under the reserved roots
// c or marea: only the internal entry, used by a shipped library, may.
MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterPublic(MAREA_TypeRegistry *registry, const MAREA_TypeDescriptor *descriptor)
{
    if ((registry == MAREA_NULLPTR) || (descriptor == MAREA_NULLPTR)) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_G_IsReservedRoot(descriptor->namespaceText, descriptor->namespaceLength) == MAREA_BOOLEAN_TRUE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT;
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    const MAREA_G_TypeEntry ENTRY = { .descriptor = descriptor };

    MAREA_G_APPEND(registry, registry->types, registry->typeCount, registry->typeCapacity, MAREA_G_TypeEntry, ENTRY, &except);

    return except;
}

const MAREA_TypeDescriptor *MAREA_CALL MAREA_TypeRegistryFind(const MAREA_TypeRegistry *registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if (registry == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < registry->typeCount; ++index) {
        const MAREA_TypeDescriptor *descriptor = registry->types[index].descriptor;

        if ((MAREA_G_TextEquals(descriptor->namespaceText, descriptor->namespaceLength, namespaceText, namespaceLength) == MAREA_BOOLEAN_TRUE) && (MAREA_G_TextEquals(descriptor->name, descriptor->nameLength, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return descriptor;
        }
    }

    return MAREA_NULLPTR;
}

MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterInternalFunction(MAREA_TypeRegistry *registry, MAREA_LibraryFlag owner, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_Null functionPointer)
{
    if ((registry == MAREA_NULLPTR) || (namespaceText == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (functionPointer == MAREA_NULLPTR)) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    if ((registry->enabledMask & owner) == MAREA_LIBRARY_FLAG_NONE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
    }

    const MAREA_LibraryMask REQUIRED = MAREA_G_DependenciesOf(registry, owner);

    if ((REQUIRED & ~registry->enabledMask) != MAREA_LIBRARY_FLAG_NONE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY;
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    const MAREA_G_FunctionEntry ENTRY = { .namespaceText = namespaceText, .name = name, .functionPointer = functionPointer, .namespaceLength = namespaceLength, .nameLength = nameLength };

    MAREA_G_APPEND(registry, registry->functions, registry->functionCount, registry->functionCapacity, MAREA_G_FunctionEntry, ENTRY, &except);

    return except;
}

// INFO: the reserved-root rejection, the same as MAREA_TypeRegistryRegisterPublic above.
MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterPublicFunction(MAREA_TypeRegistry *registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_Null functionPointer)
{
    if ((registry == MAREA_NULLPTR) || (namespaceText == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (functionPointer == MAREA_NULLPTR)) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_G_IsReservedRoot(namespaceText, namespaceLength) == MAREA_BOOLEAN_TRUE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT;
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    const MAREA_G_FunctionEntry ENTRY = { .namespaceText = namespaceText, .name = name, .functionPointer = functionPointer, .namespaceLength = namespaceLength, .nameLength = nameLength };

    MAREA_G_APPEND(registry, registry->functions, registry->functionCount, registry->functionCapacity, MAREA_G_FunctionEntry, ENTRY, &except);

    return except;
}

MAREA_Null MAREA_CALL MAREA_TypeRegistryFindFunction(const MAREA_TypeRegistry *registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if (registry == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < registry->functionCount; ++index) {
        const MAREA_G_FunctionEntry *entry = &registry->functions[index];

        if ((MAREA_G_TextEquals(entry->namespaceText, entry->namespaceLength, namespaceText, namespaceLength) == MAREA_BOOLEAN_TRUE) && (MAREA_G_TextEquals(entry->name, entry->nameLength, name, nameLength) == MAREA_BOOLEAN_TRUE)) {
            return entry->functionPointer;
        }
    }

    return MAREA_NULLPTR;
}

static MAREA_Boolean MAREA_CALL MAREA_G_IsRuntimeHookName(const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    for (MAREA_IntegerSize index = 0; index < MAREA_ARRAY_SIZE(MAREA_G_RUNTIME_HOOK_NAMES); ++index) {
        if (MAREA_G_TextEquals(name, nameLength, MAREA_G_RUNTIME_HOOK_NAMES[index], (MAREA_IntegerSize)strlen(MAREA_G_RUNTIME_HOOK_NAMES[index])) == MAREA_BOOLEAN_TRUE) {
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

// INFO: the runtime-hook allow list: a name MAREA_G_IsRuntimeHookName does not know is rejected, so a
// host never replaces an arbitrary name under the guise of a hook.
MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryReplaceRuntimeFunction(MAREA_TypeRegistry *registry, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_Null functionPointer)
{
    if ((registry == MAREA_NULLPTR) || (name == MAREA_NULLPTR) || (functionPointer == MAREA_NULLPTR)) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_G_IsRuntimeHookName(name, nameLength) == MAREA_BOOLEAN_FALSE) {
        return MAREA_TYPE_REGISTRY_EXCEPT_ERROR_UNKNOWN_RUNTIME_FUNCTION;
    }

    for (MAREA_IntegerSize index = 0; index < registry->hookCount; ++index) {
        if (MAREA_G_TextEquals(registry->hooks[index].name, registry->hooks[index].nameLength, name, nameLength) == MAREA_BOOLEAN_TRUE) {
            registry->hooks[index].functionPointer = functionPointer;

            return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;
        }
    }

    MAREA_TypeRegistryExcept except = MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS;

    const MAREA_G_RuntimeHookEntry ENTRY = { .name = name, .nameLength = nameLength, .functionPointer = functionPointer };

    MAREA_G_APPEND(registry, registry->hooks, registry->hookCount, registry->hookCapacity, MAREA_G_RuntimeHookEntry, ENTRY, &except);

    return except;
}

MAREA_Null MAREA_CALL MAREA_TypeRegistryFindRuntimeFunction(const MAREA_TypeRegistry *registry, const MAREA_Character8 *name, MAREA_IntegerSize nameLength)
{
    if ((registry == MAREA_NULLPTR) || (MAREA_G_IsRuntimeHookName(name, nameLength) == MAREA_BOOLEAN_FALSE)) {
        return MAREA_NULLPTR;
    }

    for (MAREA_IntegerSize index = 0; index < registry->hookCount; ++index) {
        if (MAREA_G_TextEquals(registry->hooks[index].name, registry->hooks[index].nameLength, name, nameLength) == MAREA_BOOLEAN_TRUE) {
            return registry->hooks[index].functionPointer;
        }
    }

    return MAREA_NULLPTR;
}
