// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_BASE_MAREATYPEREGISTRY_H)
    #define MAREA_INCLUDE_BASE_MAREATYPEREGISTRY_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREAHostContext.h"
    #include "marea/base/MAREAHostFunction.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief One bit per library the language ships with its core. A caller combines bits with bitwise
     * or into a MAREA_LibraryMask that turns each library on or off for one registry; a library whose bit
     * is off registers no type and no function, so sema and the registry answer every name of it as
     * unknown. The host, the CMake functions and marea-cc or marea-bc pass the mask.
     */
    typedef enum MAREA_LibraryFlag
    {
        MAREA_LIBRARY_FLAG_NONE = MAREA_UINTEGER_32(0),
        MAREA_LIBRARY_FLAG_MAREA_ARRAY = (MAREA_UINTEGER_32(1) << 0),
        MAREA_LIBRARY_FLAG_MAREA_VECTOR = (MAREA_UINTEGER_32(1) << 1),
        MAREA_LIBRARY_FLAG_MAREA_TUPLE = (MAREA_UINTEGER_32(1) << 2),
        MAREA_LIBRARY_FLAG_C_MEMORY = (MAREA_UINTEGER_32(1) << 3),
        MAREA_LIBRARY_FLAG_C_THREADS = (MAREA_UINTEGER_32(1) << 4),
        MAREA_LIBRARY_FLAG_MAREA_MEMORY = (MAREA_UINTEGER_32(1) << 5),
    } MAREA_LibraryFlag;

    /**
     * @brief A mask of MAREA_LibraryFlag bits, combined with bitwise or.
     */
    typedef MAREA_Uinteger32 MAREA_LibraryMask;

    /**
     * @brief The default mask: every library the language ships is on.
     */
    #define MAREA_LIBRARY_MASK_DEFAULT ((MAREA_LibraryMask)(MAREA_LIBRARY_FLAG_MAREA_ARRAY | MAREA_LIBRARY_FLAG_MAREA_VECTOR | MAREA_LIBRARY_FLAG_MAREA_TUPLE | MAREA_LIBRARY_FLAG_C_MEMORY | MAREA_LIBRARY_FLAG_C_THREADS | MAREA_LIBRARY_FLAG_MAREA_MEMORY))

    /**
     * @brief The typeParameterCount of a type that takes a list of types, each with its own place: a tuple. The
     * language sets how many such a type takes, from MAREA_TYPE_TUPLE_MINIMUM_ARITY to MAREA_TYPE_TUPLE_MAXIMUM_ARITY.
     */
    #define MAREA_TYPE_PARAMETER_COUNT_VARIADIC ((MAREA_IntegerSize) - 1)

    #define MAREA_TYPE_TUPLE_MINIMUM_ARITY MAREA_INTEGER_SIZE(2)

    #define MAREA_TYPE_TUPLE_MAXIMUM_ARITY MAREA_INTEGER_SIZE(16)

    /**
     * @brief True for the descriptor of a tuple: a variadic parameter count, no role and no index protocol. The
     * stages read this shape and never the name of the type. The ownership class of such a descriptor is not read,
     * because the elements of each written type decide what can be copied.
     */
    #define MAREA_TYPE_DESCRIPTOR_IS_TUPLE(descriptor) (((descriptor)->typeParameterCount == MAREA_TYPE_PARAMETER_COUNT_VARIADIC) && ((descriptor)->roles == MAREA_TYPE_ROLE_NONE) && ((descriptor)->hasIndexProtocol == MAREA_BOOLEAN_FALSE))

    /**
     * @brief One role the language reads from a registered type without ever reading its name. A
     * descriptor may carry more than one role, combined with bitwise or.
     */
    typedef enum MAREA_TypeRole
    {
        MAREA_TYPE_ROLE_NONE = MAREA_UINTEGER_32(0),
        MAREA_TYPE_ROLE_PARTITION_PART = (MAREA_UINTEGER_32(1) << 0),
        MAREA_TYPE_ROLE_SPAWN_HANDLE = (MAREA_UINTEGER_32(1) << 1),
        MAREA_TYPE_ROLE_RAW_POINTER = (MAREA_UINTEGER_32(1) << 2),
        MAREA_TYPE_ROLE_OWNER = (MAREA_UINTEGER_32(1) << 3),
        MAREA_TYPE_ROLE_VIEW = (MAREA_UINTEGER_32(1) << 4),
    } MAREA_TypeRole;

    /**
     * @brief Whether a value of the type is held by copy (a plain value, or a copied handle such as a
     * view or a raw pointer) or by move (a heap owner that one name alone may hold at a time).
     */
    typedef enum MAREA_TypeOwnershipClass
    {
        MAREA_TYPE_OWNERSHIP_CLASS_COPY = MAREA_UINTEGER_8(0),
        MAREA_TYPE_OWNERSHIP_CLASS_MOVE = MAREA_UINTEGER_8(1),
    } MAREA_TypeOwnershipClass;

    /**
     * @brief One type a library registers: its namespace and name, its generic type-parameter count,
     * its roles, its ownership class, and whether its first type parameter is an element reached by
     * index (an array, a vector or a view, as opposed to a single wrapped value such as an owner, a
     * spawn handle or a raw pointer). A plain struct, caller owned, the same pattern as
     * MAREA_SemaSignature: every pointer must stay valid for as long as the registry may be asked again.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_TypeDescriptor
    {
        const MAREA_Character8 *namespaceText;
        MAREA_IntegerSize namespaceLength;
        const MAREA_Character8 *name;
        MAREA_IntegerSize nameLength;
        MAREA_IntegerSize typeParameterCount;
        MAREA_TypeRole roles;
        MAREA_TypeOwnershipClass ownershipClass;
        MAREA_Boolean hasIndexProtocol;
    } MAREA_TypeDescriptor;

    /**
     * @brief Function the caller of sema gives to a macro expander so it learns the size of a file without
     * reading the disk itself: the caller owns every read.
     * @param userData Pointer the caller keeps with the function.
     * @param path File path as written in the call, not NUL terminated.
     * @param pathLength Length of path in bytes.
     * @param outSize Receives the size of the file in bytes. Read only when the function returns true.
     * @return True when the file exists and its size is known, false when it does not.
     */
    typedef MAREA_Boolean(MAREA_CALL * MAREA_MacroIncludeSizeFunction)(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_IntegerSize *outSize);

    /**
     * @brief What a macro expander is given: the text of the one string literal argument without its
     * quotes, and the only way to ask about a file. A plain struct, read during one call.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_MacroExpandRequest
    {
        const MAREA_Character8 *text;
        MAREA_IntegerSize textLength;
        MAREA_MacroIncludeSizeFunction includeSize;
        MAREA_Null includeUserData;
    } MAREA_MacroExpandRequest;

    /**
     * @brief C11 function of a shipped library that expands one compile-time macro call.
     * @param request The argument text and the file-size function of the caller. Never null.
     * @param outCount Receives the count that completes the result type (a byte count for a file). Read only
     * when the function returns true.
     * @return True when the call is valid, false when it is not (for example a file that does not exist).
     */
    typedef MAREA_Boolean(MAREA_CALL * MAREA_MacroExpandFunction)(const MAREA_MacroExpandRequest *request, MAREA_IntegerSize *outCount);

    /**
     * @brief The kind of one argument of a registered macro, checked by sema before the expander runs.
     */
    typedef enum MAREA_MacroArgumentKind
    {
        MAREA_MACRO_ARGUMENT_KIND_STRING_LITERAL = MAREA_UINTEGER_8(0),
    } MAREA_MacroArgumentKind;

    /**
     * @brief One compile-time macro a shipped library registers: its namespace and name (the call is written
     * with the path of the library, as in marea.array.include_bytes!), the kind of each argument, the type
     * it gives (a registered type with an element type name and the count that the expander returns), and
     * the expander. A plain struct, caller owned, with the same lifetime rule as MAREA_TypeDescriptor.
     */
    typedef struct MAREA_ALIGNED(8) MAREA_MacroDescriptor
    {
        const MAREA_Character8 *namespaceText;
        MAREA_IntegerSize namespaceLength;
        const MAREA_Character8 *name;
        MAREA_IntegerSize nameLength;
        const MAREA_MacroArgumentKind *argumentKinds;
        MAREA_IntegerSize argumentCount;
        const MAREA_TypeDescriptor *resultType;
        const MAREA_Character8 *resultElementText;
        MAREA_IntegerSize resultElementLength;
        MAREA_MacroExpandFunction expand;
    } MAREA_MacroDescriptor;

    /**
     * @brief Result of a registry operation.
     */
    typedef enum MAREA_TypeRegistryExcept
    {
        MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_TYPE_REGISTRY_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
        MAREA_TYPE_REGISTRY_EXCEPT_ERROR_RESERVED_NAMESPACE_ROOT = MAREA_UINTEGER_8(3),
        MAREA_TYPE_REGISTRY_EXCEPT_ERROR_MISSING_DEPENDENCY = MAREA_UINTEGER_8(4),
        MAREA_TYPE_REGISTRY_EXCEPT_ERROR_UNKNOWN_RUNTIME_FUNCTION = MAREA_UINTEGER_8(5),
    } MAREA_TypeRegistryExcept;

    /**
     * @brief Opaque registry of the types and the functions a library registers, and the mask that
     * turns each shipped library on or off. The caller owns the registry and every descriptor it hands
     * in; the registry only keeps the pointers, never copies the text or the function.
     */
    typedef struct MAREA_TypeRegistry MAREA_TypeRegistry;

    /**
     * @brief Creates a registry with the given library mask. A library whose bit is off in enabledMask
     * registers nothing: MAREA_TypeRegistryRegisterInternal is then a no-op success for it.
     * @param arena Arena that owns the registry. Must not be null.
     * @param enabledMask Mask of MAREA_LibraryFlag bits that are on. MAREA_LIBRARY_MASK_DEFAULT turns on
     * every shipped library.
     * @param outRegistry Receives the registry. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER when arena or outRegistry is null,
     * or ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryCreate(MAREA_Arena * arena, MAREA_LibraryMask enabledMask, MAREA_TypeRegistry * *outRegistry);

    /**
     * @brief Returns the enabled mask the registry was created with.
     * @param registry Registry to query. Null returns MAREA_LIBRARY_FLAG_NONE.
     * @return The mask.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_LibraryMask MAREA_CALL MAREA_TypeRegistryGetEnabledMask(const MAREA_TypeRegistry *registry);

    /**
     * @brief Registers a dependency: owner may be registered through the internal entry only when every
     * library in dependsOn is on in the registry's enabled mask.
     * @param registry Registry to change. Must not be null.
     * @param owner Library the dependency belongs to; exactly one bit.
     * @param dependsOn Mask of the libraries owner depends on. MAREA_LIBRARY_FLAG_NONE means no
     * dependency.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when registry is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryDeclareDependency(MAREA_TypeRegistry * registry, MAREA_LibraryFlag owner, MAREA_LibraryMask dependsOn);

    /**
     * @brief Registers one type descriptor on behalf of a shipped library, through the internal entry.
     * May register under the reserved roots c and marea. A no-op success, nothing stored, when owner's
     * bit is off in the registry's enabled mask.
     * @param registry Registry to change. Must not be null.
     * @param owner Library that owns the descriptor; exactly one bit.
     * @param descriptor Descriptor to register. Must not be null; its pointers must outlive the
     * registry.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER when registry or descriptor is
     * null, ERROR_OUT_OF_MEMORY when the arena cannot grow, or ERROR_MISSING_DEPENDENCY when owner is on
     * but a library MAREA_TypeRegistryDeclareDependency named for it is off.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterInternal(MAREA_TypeRegistry * registry, MAREA_LibraryFlag owner, const MAREA_TypeDescriptor *descriptor);

    /**
     * @brief Registers one type descriptor on behalf of a user or a host, through the public entry.
     * @param registry Registry to change. Must not be null.
     * @param descriptor Descriptor to register. Must not be null; its pointers must outlive the
     * registry.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER when registry or descriptor is
     * null, ERROR_OUT_OF_MEMORY when the arena cannot grow, or ERROR_RESERVED_NAMESPACE_ROOT when the
     * first dotted segment of descriptor->namespaceText is c or marea.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterPublic(MAREA_TypeRegistry * registry, const MAREA_TypeDescriptor *descriptor);

    /**
     * @brief Finds a registered type descriptor by its namespace and its name.
     * @param registry Registry to query. Null returns null.
     * @param namespaceText Dotted namespace text, not NUL terminated.
     * @param namespaceLength Length of namespaceText in bytes.
     * @param name Type name, not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @return The descriptor, or null when none is registered under that namespace and name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_TypeDescriptor *MAREA_CALL MAREA_TypeRegistryFind(const MAREA_TypeRegistry *registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Registers one compile-time macro on behalf of a shipped library, through the internal entry.
     * There is no public entry: a user or a host never registers a macro, so the roots c and marea stay
     * the only ones that hold one. A no-op success, nothing stored, when owner's bit is off in the enabled
     * mask.
     * @param registry Registry to change. Must not be null.
     * @param owner Library that owns the macro; exactly one bit.
     * @param descriptor Descriptor to register. Must not be null; its pointers must outlive the registry.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER when registry, descriptor or its
     * expander is null, ERROR_OUT_OF_MEMORY, or ERROR_MISSING_DEPENDENCY, the same as
     * MAREA_TypeRegistryRegisterInternal.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterInternalMacro(MAREA_TypeRegistry * registry, MAREA_LibraryFlag owner, const MAREA_MacroDescriptor *descriptor);

    /**
     * @brief Finds a registered macro by the namespace of its library and its name.
     * @param registry Registry to query. Null returns null.
     * @param namespaceText Dotted namespace text, not NUL terminated.
     * @param namespaceLength Length of namespaceText in bytes.
     * @param name Macro name without the "!", not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @return The descriptor, or null when none is registered under that namespace and name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_MacroDescriptor *MAREA_CALL MAREA_TypeRegistryFindMacro(const MAREA_TypeRegistry *registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Finds the first registered type that holds a role, so that sema can write the type a role
     * names (the handle of a spawn, the part of a partition) without ever reading a type name.
     * @param registry Registry to ask. A null registry finds nothing.
     * @param role One role bit.
     * @return The descriptor, or null when no registered type holds the role.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL const MAREA_TypeDescriptor *MAREA_CALL MAREA_TypeRegistryFindByRole(const MAREA_TypeRegistry *registry, MAREA_TypeRole role);

    /**
     * @brief Registers one wrapper of a C function for a shipped library, through the internal entry. Called
     * through MAREA_REGISTER_FUNCTION.
     * @param registry Registry to change. Must not be null.
     * @param owner Library that owns the function; exactly one bit.
     * @param namespaceText Dotted namespace text, not NUL terminated.
     * @param namespaceLength Length of namespaceText in bytes.
     * @param name Function name, not NUL terminated.
     * @param nameLength Length of name in bytes.
     * @param function The wrapper that the runtime calls. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER, ERROR_OUT_OF_MEMORY, or
     * ERROR_MISSING_DEPENDENCY, the same as MAREA_TypeRegistryRegisterInternal.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterInternalFunction(MAREA_TypeRegistry * registry, MAREA_LibraryFlag owner, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_HostFunction function);

    /**
     * @brief Registers one wrapper of a C function for a user or a host, through the public entry. Called
     * through MAREA_REGISTER_FUNCTION.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER, ERROR_OUT_OF_MEMORY, or
     * ERROR_RESERVED_NAMESPACE_ROOT, the same as MAREA_TypeRegistryRegisterPublic.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryRegisterPublicFunction(MAREA_TypeRegistry * registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_HostFunction function);

    /**
     * @brief Finds the wrapper of a registered C function by its namespace and its name.
     * @param registry Registry to query. Null returns null.
     * @return The wrapper, or null when none is registered under that namespace and name.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_HostFunction MAREA_CALL MAREA_TypeRegistryFindFunction(const MAREA_TypeRegistry *registry, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Replaces one runtime function of the language itself, such as alloc or free: the one door
     * a host has into a reserved root, because it names no new member of c or marea, only the
     * implementation of a hook the runtime already declares.
     * @param registry Registry to change. Must not be null.
     * @param name Hook name, not NUL terminated (for example "alloc" or "free").
     * @param nameLength Length of name in bytes.
     * @param functionPointer The replacement C function. Must not be null.
     * @return MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS, ERROR_NULL_POINTER, or
     * ERROR_UNKNOWN_RUNTIME_FUNCTION when name is not a runtime hook the registry declares.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_TypeRegistryExcept MAREA_CALL MAREA_TypeRegistryReplaceRuntimeFunction(MAREA_TypeRegistry * registry, const MAREA_Character8 *name, MAREA_IntegerSize nameLength, MAREA_Null functionPointer);

    /**
     * @brief Reads back the replacement of a runtime hook. The registry holds no default of its own: the
     * runtime that calls the hook uses its own when the answer is null.
     * @param registry Registry to query. Null returns null.
     * @return The replacement, or null when none was given or name is not a runtime hook the registry
     * declares.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Null MAREA_CALL MAREA_TypeRegistryFindRuntimeFunction(const MAREA_TypeRegistry *registry, const MAREA_Character8 *name, MAREA_IntegerSize nameLength);

    /**
     * @brief Registers one C function for the language runtime: the one macro that exposes a function,
     * with no external generator and no hand-written glue on either side. function is the name of a typed C
     * function that has its binding declaration (MAREA_HOST_FUNCTION) in this file: the macro registers the
     * wrapper that the binding made, so a function with no binding does not compile. owner
     * MAREA_LIBRARY_FLAG_NONE registers through the public entry (a user or a host); any other owner registers
     * through the internal entry (a shipped library), the only path allowed to name a function under c or marea.
     */
    #define MAREA_REGISTER_FUNCTION(registry, owner, namespaceText, namespaceLength, name, nameLength, function)                                                                              \
        (((owner) == MAREA_LIBRARY_FLAG_NONE) ? MAREA_TypeRegistryRegisterPublicFunction((registry), (namespaceText), (namespaceLength), (name), (nameLength), &MAREA_HOST_WRAPPER(function)) \
                                              : MAREA_TypeRegistryRegisterInternalFunction((registry), (owner), (namespaceText), (namespaceLength), (name), (nameLength), &MAREA_HOST_WRAPPER(function)))

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_BASE_MAREATYPEREGISTRY_H */
