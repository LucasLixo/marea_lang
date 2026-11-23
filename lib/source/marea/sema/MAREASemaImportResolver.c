// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "marea/internal/sema/MAREASemaInternal.h"

MAREA_SemaImportResolverExcept MAREA_CALL MAREA_SemaImportResolverCreate(MAREA_Arena *arena, MAREA_SemaImportResolveFunction resolve, MAREA_Null userData, MAREA_SemaImportResolver **outResolver)
{
    if (arena == MAREA_NULLPTR) {
        return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER;
    }

    if (resolve == MAREA_NULLPTR) {
        return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER;
    }

    if (outResolver == MAREA_NULLPTR) {
        return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER;
    }

    MAREA_Null memory = MAREA_NULLPTR;

    if (MAREA_ArenaAllocate(arena, sizeof(MAREA_SemaImportResolver), MAREA_ALIGNOF(MAREA_SemaImportResolver), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_OUT_OF_MEMORY;
    }

    MAREA_SemaImportResolver *resolver = memory;

    resolver->resolve = resolve;
    resolver->signature = MAREA_NULLPTR;
    resolver->userData = userData;

    *outResolver = resolver;

    return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS;
}

MAREA_SemaImportResolverExcept MAREA_CALL MAREA_SemaImportResolverSetSignatureFunction(MAREA_SemaImportResolver *resolver, MAREA_SemaImportSignatureFunction signature)
{
    if (resolver == MAREA_NULLPTR) {
        return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_ERROR_NULL_POINTER;
    }

    resolver->signature = signature;

    return MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS;
}

MAREA_Null MAREA_CALL MAREA_SemaImportResolverGetUserData(const MAREA_SemaImportResolver *resolver)
{
    if (resolver == MAREA_NULLPTR) {
        return MAREA_NULLPTR;
    }

    return resolver->userData;
}
