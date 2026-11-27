// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_SHARED_MAREATARGETDESCRIPTION_H)
    #define MAREA_APPS_SHARED_MAREATARGETDESCRIPTION_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/sema/MAREASema.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Fills target with the host this tool itself was compiled for: architecture, operating
     * system, compiler, endianness and pointer width, detected once from the tool's own compile-time
     * predefined macros. features/featureCount are left empty: no feature is detected automatically,
     * only given through a repeatable --target-feature flag.
     * @param target Target to fill. Must not be null.
     */
    void MareaTargetDescriptionDetectHost(MAREA_SemaTarget *target);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_SHARED_MAREATARGETDESCRIPTION_H */
