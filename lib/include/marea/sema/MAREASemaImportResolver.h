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
     * @brief One parameter of the signature of a member of an EXTERNAL namespace: its type in the
     * canonical type spelling sema itself writes (for example "float64", "string"), not NUL terminated.
     * The host owns the text and keeps it valid for as long as the resolver may answer with it.
     */
    typedef struct MAREA_SemaSignatureParameter
    {
        const MAREA_Character8 *typeText;
        MAREA_IntegerSize typeTextLength;
    } MAREA_SemaSignatureParameter;

    /**
     * @brief Signature of one member of an EXTERNAL namespace, as the host describes it: its parameter
     * types and its result type, both in the canonical type spelling (for example "float64", "string").
     * A plain struct, not opaque, because the host builds and owns every value: the array of parameters
     * and every text it points to must stay valid for as long as the resolver may be asked again, and
     * sema only reads them during one call, never copies or frees them.
     */
    typedef struct MAREA_SemaSignature
    {
        const MAREA_SemaSignatureParameter *parameters;
        MAREA_IntegerSize parameterCount;
        const MAREA_Character8 *resultTypeText;
        MAREA_IntegerSize resultTypeTextLength;
    } MAREA_SemaSignature;

    /**
     * @brief Function the caller optionally supplies to give the signature of one member of a namespace
     * it answers EXTERNAL for. Sema calls it only for a call whose callee names such a member, never for
     * a namespace the resolve function answers FOUND or NOT_FOUND for.
     * @param userData Pointer given to MAREA_SemaImportResolverCreate.
     * @param namespaceText Dotted namespace text, not NUL terminated (for example "c.math").
     * @param namespaceLength Length of namespaceText in bytes.
     * @param memberText Member name, not NUL terminated (for example "pow").
     * @param memberLength Length of memberText in bytes.
     * @param outResult Receives NOT_FOUND when the host does not know the member, or FOUND when
     * *outSignature is filled. EXTERNAL is never a meaningful answer here.
     * @param outSignature Receives the signature. Read only when *outResult is FOUND.
     */
    typedef void(MAREA_CALL * MAREA_SemaImportSignatureFunction)(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_Character8 *memberText, MAREA_IntegerSize memberLength, MAREA_SemaImportResult *outResult, const MAREA_SemaSignature **outSignature);

    /**
     * @brief Opaque import resolver: a resolve function with the user data it needs, and the optional
     * signature function set after creation.
     */
    typedef struct MAREA_SemaImportResolver MAREA_SemaImportResolver;

    /**
     * @brief Creates an import resolver in the arena, with no signature function.
     * @param arena Arena that owns the resolver. Must not be null.
     * @param resolve Function that answers an import. Must not be null.
     * @param userData Pointer handed to resolve on every call. May be null.
     * @param outResolver Receives the resolver. Must not be null.
     * @return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS, ERROR_NULL_POINTER when arena, resolve or outResolver
     * is null, or ERROR_OUT_OF_MEMORY when the arena cannot grow.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaImportResolverExcept MAREA_CALL MAREA_SemaImportResolverCreate(MAREA_Arena * arena, MAREA_SemaImportResolveFunction resolve, MAREA_Null userData, MAREA_SemaImportResolver * *outResolver);

    /**
     * @brief Sets or clears the optional signature function of a resolver, created with none.
     * @param resolver Resolver to change. Must not be null.
     * @param signature Function that answers the signature of a member of an EXTERNAL namespace, or null
     * to answer every member with no signature, as a resolver with none always does.
     * @return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS, or ERROR_NULL_POINTER when resolver is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_SemaImportResolverExcept MAREA_CALL MAREA_SemaImportResolverSetSignatureFunction(MAREA_SemaImportResolver * resolver, MAREA_SemaImportSignatureFunction signature);

    /**
     * @brief Returns the user data the resolver was created with.
     * @param resolver Resolver to query. Null returns null.
     * @return The user data, or null when resolver is null.
     */
    MAREA_NODISCARD MAREA_EXTERN_DECL MAREA_Null MAREA_CALL MAREA_SemaImportResolverGetUserData(const MAREA_SemaImportResolver *resolver);

MAREA_END_EXTERN_C

#endif /* MAREA_INCLUDE_SEMA_MAREASEMAIMPORTRESOLVER_H */
