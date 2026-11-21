// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INCLUDE_SEMA_MAREASEMAIMPORTRESOLVER_H)
    #define MAREA_INCLUDE_SEMA_MAREASEMAIMPORTRESOLVER_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/parse/MAREAAst.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Answer of an import resolver for one namespace.
     */
    typedef enum MAREA_SemaImportResult
    {
        MAREA_SEMA_IMPORT_RESULT_NOT_FOUND = MAREA_UINTEGER_8(0),
        MAREA_SEMA_IMPORT_RESULT_FOUND = MAREA_UINTEGER_8(1),
        MAREA_SEMA_IMPORT_RESULT_EXTERNAL = MAREA_UINTEGER_8(2),
    } MAREA_SemaImportResult;

    /**
     * @brief Result of an import resolver operation.
     */
    typedef enum MAREA_SemaImportResolverExcept
    {
        MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS = MAREA_UINTEGER_8(0),
        MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER = MAREA_UINTEGER_8(1),
        MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_OUT_OF_MEMORY = MAREA_UINTEGER_8(2),
    } MAREA_SemaImportResolverExcept;

    /**
     * @brief Function the caller supplies to answer an import. Sema never reads the disk: this function
     * is the only way it learns about another namespace.
     * @param userData Pointer given to MAREA_SemaImportResolverCreate.
     * @param namespaceText Dotted namespace text, not NUL terminated.
     * @param namespaceLength Length of namespaceText in bytes.
     * @param outResult Receives NOT_FOUND, FOUND (the namespace has a tree) or EXTERNAL (it exists, but sema
     * has no tree for it).
     * @param outRoot Receives the COMPILATION_UNIT of the namespace. Read only when *outResult is FOUND.
     */
    typedef void(MAREA_CALL * MAREA_SemaImportResolveFunction)(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot);

    /**
     * @brief Opaque import resolver: a resolve function with the user data it needs.
     */
    typedef struct MAREA_SemaImportResolver MAREA_SemaImportResolver;

    /**
     * @brief Creates an import resolver in the arena.
     * @param arena Arena that owns the resolver. Must not be null.
     * @param resolve Function that answers an import. Must not be null.
     * @param userData Pointer handed to resolve on every call. May be null.
     * @param outResolver Receives the resolver. Must not be null.
     * @return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS, ERROR_NULL_POINTER when arena, resolve or outResolver
     * is null, or ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaImportResolverExcept MAREA_CALL MAREA_SemaImportResolverCreate(MAREA_Arena * arena, MAREA_SemaImportResolveFunction resolve, MAREA_Null userData, MAREA_SemaImportResolver * *outResolver);

    /**
     * @brief Returns the user data the resolver was created with.
     * @param resolver Resolver to query. Null returns null.
     * @return The user data, or null when resolver is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Null MAREA_CALL MAREA_SemaImportResolverGetUserData(const MAREA_SemaImportResolver *resolver);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_SEMA_MAREASEMAIMPORTRESOLVER_H */
