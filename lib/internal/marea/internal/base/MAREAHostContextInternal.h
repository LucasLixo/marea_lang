// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_INTERNAL_BASE_MAREAHOSTCONTEXTINTERNAL_H)
    #define MAREA_INTERNAL_BASE_MAREAHOSTCONTEXTINTERNAL_H

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREAMacros.h"
    #include "marea/base/MAREAHostContext.h"
    #include "marea/base/MAREATypeRegistry.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief The body of a host context. A runtime makes one on its own stack for each call of a C function: the
     * two hooks of its allocator (both null when a host replaced only one), no trap and unpaired false. After the
     * call it reads trap and unpaired.
     */
    struct MAREA_HostContext
    {
        MAREA_RuntimeAllocateFunction allocate;
        MAREA_RuntimeFreeFunction release;
        MAREA_HostTrap trap;
        MAREA_Boolean unpaired;
    };

MAREA_END_EXTERN_C

#endif /* MAREA_INTERNAL_BASE_MAREAHOSTCONTEXTINTERNAL_H */
