// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_SHARED_MAREAIMPORTRESOLVER_H)
    #define MAREA_APPS_SHARED_MAREAIMPORTRESOLVER_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/sema/MAREASemaImportResolver.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief One namespace this resolver has already answered, cached by dotted name for the run.
     */
    typedef struct MareaImportResolverCacheEntry
    {
        const MAREA_Character8 *namespaceText;
        MAREA_IntegerSize namespaceLength;
        MAREA_SemaImportResult result;
        const MAREA_AstNode *root;
    } MareaImportResolverCacheEntry;

    /**
     * @brief State of the project-file import resolver: given a dotted namespace, it searches the
     * include directories (plus the directory of the file being compiled) for a matching ".marea"
     * file, lexes and parses it on first use, and caches the answer by namespace for the run. Sema
     * itself never reads the disk, so this is the tool's own job (apply marea-cc-and-bc-tools).
     */
    typedef struct MareaImportResolverState
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_Character8 *const *includeDirectories;
        MAREA_IntegerSize includeDirectoryCount;
        const MAREA_Character8 *primaryDirectory;
        MareaImportResolverCacheEntry *cache;
        MAREA_IntegerSize cacheCount;
        MAREA_IntegerSize cacheCapacity;
    } MareaImportResolverState;

    /**
     * @brief Initializes a resolver state.
     * @param state State to initialize. Must not be null.
     * @param arena Arena that owns the cache and every parsed tree. Must not be null.
     * @param sink Diagnostic sink the lex/parse of a resolved file reports to. Must not be null.
     * @param includeDirectories Directories searched in order, after primaryDirectory. May be null when
     * includeDirectoryCount is 0.
     * @param includeDirectoryCount Number of entries in includeDirectories.
     * @param primaryDirectory Directory of the file being compiled, searched first. May be null.
     */
    void MareaImportResolverInit(MareaImportResolverState * state, MAREA_Arena * arena, MAREA_DiagnosticSink * sink, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, const MAREA_Character8 *primaryDirectory);

    /**
     * @brief The resolve function itself, matching MAREA_SemaImportResolveFunction. userData must be a
     * MareaImportResolverState created with MareaImportResolverInit.
     */
    void MAREA_CALL MareaImportResolverResolve(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_SHARED_MAREAIMPORTRESOLVER_H */
