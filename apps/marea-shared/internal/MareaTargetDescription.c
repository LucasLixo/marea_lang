// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaTargetDescription.h"

static const MAREA_Character8 *MareaTargetDescriptionArchitecture(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "unknown";
#endif
}

static const MAREA_Character8 *MareaTargetDescriptionOperatingSystem(void)
{
#if defined(__linux__)
    return "linux";
#elif defined(__APPLE__)
    return "macos";
#elif defined(_WIN32)
    return "windows";
#else
    return "unknown";
#endif
}

static const MAREA_Character8 *MareaTargetDescriptionCompiler(void)
{
#if defined(__clang__)
    return "clang";
#elif defined(__GNUC__)
    return "gcc";
#elif defined(_MSC_VER)
    return "msvc";
#else
    return "unknown";
#endif
}

static const MAREA_Character8 *MareaTargetDescriptionEndianness(void)
{
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return "big";
#else
    return "little";
#endif
}

void MareaTargetDescriptionDetectHost(MAREA_SemaTarget *target)
{
    if (target == NULL) {
        return;
    }

    target->architecture = MareaTargetDescriptionArchitecture();
    target->operatingSystem = MareaTargetDescriptionOperatingSystem();
    target->compiler = MareaTargetDescriptionCompiler();
    target->endianness = MareaTargetDescriptionEndianness();
    target->features = NULL;
    target->featureCount = 0;
    target->pointerWidth = (MAREA_Uinteger32)(sizeof(void *) * 8);
}
