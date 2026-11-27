// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "MareaTargetDescription.h"

TEST(MareaTargetDescription, FillsEveryTextField)
{
    MAREA_SemaTarget target{};

    MareaTargetDescriptionDetectHost(&target);

    ASSERT_NE(target.architecture, nullptr);
    ASSERT_NE(target.operatingSystem, nullptr);
    ASSERT_NE(target.compiler, nullptr);
    ASSERT_NE(target.endianness, nullptr);
    ASSERT_EQ(target.features, nullptr);
    ASSERT_EQ(target.featureCount, 0);
}

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
