// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAMEMORYHELPERS_HPP)
    #define MAREA_TESTS_MAREAMEMORYHELPERS_HPP

    #include "marea/base/MAREAHostContext.h"

    #include <cstdint>
    #include <cstdlib>

// INFO: what the tests of the two memory runtimes share: a context of one call with the default allocator, with
// counting hooks or with hooks that give no block.
inline int MAREA_countedAllocations{ 0 };

inline int MAREA_countedReleases{ 0 };

inline MAREA_Null MAREA_CALL MAREA_countedAllocate(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
{
    ++MAREA_countedAllocations;

    return std::aligned_alloc(alignment, (((size + alignment) - 1) / alignment) * alignment);
}

inline void MAREA_CALL MAREA_countedRelease(MAREA_Null address)
{
    ++MAREA_countedReleases;

    std::free(address);
}

inline MAREA_Null MAREA_CALL MAREA_refusedAllocate(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
{
    MAREA_IGNORE(size);
    MAREA_IGNORE(alignment);

    return nullptr;
}

struct MAREA_MemoryRun
{
    MAREA_HostContext context{};

    MAREA_MemoryRun()
    {
        MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));
    }
};

struct MAREA_CountedRun
{
    MAREA_HostContext context{};

    MAREA_CountedRun()
    {
        MAREA_countedAllocations = 0;
        MAREA_countedReleases = 0;

        MAREA_IGNORE(MAREA_HostContextInitialize(&context, &MAREA_countedAllocate, &MAREA_countedRelease));
    }
};

struct MAREA_RefusingRun
{
    MAREA_HostContext context{};

    MAREA_RefusingRun()
    {
        MAREA_IGNORE(MAREA_HostContextInitialize(&context, &MAREA_refusedAllocate, &MAREA_countedRelease));
    }
};

inline std::uint64_t MAREA_addressOf(const void *pointer)
{
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(pointer));
}

inline std::uint8_t *MAREA_bytesAt(std::uint64_t address)
{
    return reinterpret_cast<std::uint8_t *>(static_cast<std::uintptr_t>(address));
}

#endif
