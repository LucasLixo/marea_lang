// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "MareaTargetDescription.h"

#include <cstring>

TEST(MareaTargetDescription, FillsEveryTextField)
{
    MAREA_SemaTarget target{};

    MareaTargetDescriptionDetectHost(&target);

    ASSERT_NE(target.architecture, nullptr);
    ASSERT_NE(target.operatingSystem, nullptr);
    ASSERT_NE(target.compiler, nullptr);
    ASSERT_NE(target.endianness, nullptr);
}

// INFO: this build machine is confirmed x86_64 with gcc or clang (see the apply report), so sse2 is real hardware
// and toolchain support, not a guess: marea.math's auto backend needs this feature present to pick intrinsics or
// vector_extension over scalar on this machine (MAREA_G_BackendNameAvailable, lib/source/marea/sema/MAREASemaAttribute.c).
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(_M_X64))
TEST(MareaTargetDescription, DetectsSse2OnX86_64WithGccOrClang)
{
    MAREA_SemaTarget target{};

    MareaTargetDescriptionDetectHost(&target);

    ASSERT_NE(target.features, nullptr);
    ASSERT_EQ(target.featureCount, 1);
    EXPECT_STREQ(target.features[0], "sse2");
}
#endif

// INFO: a target with neither selector keeps the empty feature list: no feature is invented for a target this
// function does not detect.
#if !((defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(_M_X64))) && !((defined(__GNUC__) || defined(__clang__)) && defined(__aarch64__))
TEST(MareaTargetDescription, LeavesFeaturesEmptyOnAnUndetectedTarget)
{
    MAREA_SemaTarget target{};

    MareaTargetDescriptionDetectHost(&target);

    ASSERT_EQ(target.features, nullptr);
    ASSERT_EQ(target.featureCount, 0);
}
#endif

TEST(MareaTargetDescription, PointerWidthMatchesThisProcess)
{
    MAREA_SemaTarget target{};

    MareaTargetDescriptionDetectHost(&target);

    ASSERT_EQ(target.pointerWidth, static_cast<MAREA_Uinteger32>(sizeof(void *) * 8));
}

TEST(MareaTargetDescription, NullTargetDoesNotCrash)
{
    MareaTargetDescriptionDetectHost(nullptr);
}
