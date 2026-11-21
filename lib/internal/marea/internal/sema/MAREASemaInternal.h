// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H)
    #define MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAArena.h"
    #include "marea/base/MAREADiagnostic.h"
    #include "marea/parse/MAREAAst.h"
    #include "marea/parse/MAREAAstKind.h"
    #include "marea/sema/MAREASema.h"
    #include "marea/sema/MAREASemaImportResolver.h"

MAREA_BEGIN_EXTERN_C

    struct MAREA_SemaImportResolver
    {
        MAREA_SemaImportResolveFunction resolve;
        MAREA_Null userData;
    };

    struct MAREA_SemaContext
    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        MAREA_SemaImportResolver *resolver;
    };

    /**
     * @brief Visits a node and every node below it, in source order. Returns at once on an ERROR_NODE:
     * no check and no diagnostic runs under it. Every check of the stage is called from here.
     * @param context Context of the run. Must not be null.
     * @param node Node to visit. A null node is ignored.
     * @return MAREA_SEMA_EXCEPT_SUCCESS, or the first exception a check returns.
     */
    MAREA_NODISCARD MAREA_SemaExcept MAREA_CALL MAREA_SemaVisit(MAREA_SemaContext * context, const MAREA_AstNode *node);

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_SEMA_MAREASEMAINTERNAL_H */
