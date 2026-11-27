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

// INFO: one feature name per detected ISA extension, read by marea.math's backend/when checks
// (MAREA_G_TargetHasFeature, lib/source/marea/sema/MAREASemaAttribute.c). Static storage: the caller keeps the
// pointer only for the life of one target description, never past it.
static const MAREA_Character8 *const MAREA_G_TargetFeatureSse2[] = { "sse2" };
static const MAREA_Character8 *const MAREA_G_TargetFeatureNeon[] = { "neon" };

// INFO: sse2 is part of the x86_64 baseline, so the compiler always defines __SSE2__ there; _M_X64 covers MSVC,
// which gives no such macro.
static MAREA_Boolean MareaTargetDescriptionHasSse2(void)
{
#if defined(__SSE2__) || defined(_M_X64)
    return MAREA_BOOLEAN_TRUE;
#else
    return MAREA_BOOLEAN_FALSE;
#endif
}

static MAREA_Boolean MareaTargetDescriptionHasNeon(void)
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    return MAREA_BOOLEAN_TRUE;
#else
    return MAREA_BOOLEAN_FALSE;
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

    if (MareaTargetDescriptionHasSse2() == MAREA_BOOLEAN_TRUE) {
        target->features = MAREA_G_TargetFeatureSse2;
        target->featureCount = (MAREA_IntegerSize)(sizeof(MAREA_G_TargetFeatureSse2) / sizeof(MAREA_G_TargetFeatureSse2[0]));
    } else if (MareaTargetDescriptionHasNeon() == MAREA_BOOLEAN_TRUE) {
        target->features = MAREA_G_TargetFeatureNeon;
        target->featureCount = (MAREA_IntegerSize)(sizeof(MAREA_G_TargetFeatureNeon) / sizeof(MAREA_G_TargetFeatureNeon[0]));
    } else {
        target->features = NULL;
        target->featureCount = 0;
    }

    target->pointerWidth = (MAREA_Uinteger32)(sizeof(void *) * 8);
}
