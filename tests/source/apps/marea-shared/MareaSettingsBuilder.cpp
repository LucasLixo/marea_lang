// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "MareaSettingsBuilder.h"

namespace
{
    class MareaSettingsBuilderTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ASSERT_EQ(MAREA_ArenaCreate(0, &arena), MAREA_ARENA_EXCEPT_SUCCESS);
            MareaSettingsBuilderInit(&builder, arena);
        }

        void TearDown() override { MAREA_ArenaDestroy(arena); }

        MAREA_Arena *arena{ nullptr };
        MareaSettingsBuilder builder{};
    };
} // namespace

TEST_F(MareaSettingsBuilderTest, StartsEmpty)
{
    MAREA_SemaSettings settings = MareaSettingsBuilderBuild(&builder);

    ASSERT_EQ(settings.count, 0);
}

TEST_F(MareaSettingsBuilderTest, AddsOnePair)
{
    ASSERT_TRUE(MareaSettingsBuilderAddPair(&builder, reinterpret_cast<const MAREA_Character8 *>("DEBUG=true")));

    MAREA_SemaSettings settings = MareaSettingsBuilderBuild(&builder);

    ASSERT_EQ(settings.count, 1);
    ASSERT_STREQ(reinterpret_cast<const char *>(settings.pairs[0].name), "DEBUG");
    ASSERT_STREQ(reinterpret_cast<const char *>(settings.pairs[0].value), "true");
}

TEST_F(MareaSettingsBuilderTest, AddsManyPairsPastInitialCapacity)
{
    for (int index{ 0 }; index < 10; ++index) {
        std::string argument = "NAME" + std::to_string(index) + "=" + std::to_string(index);

        ASSERT_TRUE(MareaSettingsBuilderAddPair(&builder, reinterpret_cast<const MAREA_Character8 *>(argument.c_str())));
    }

    MAREA_SemaSettings settings = MareaSettingsBuilderBuild(&builder);

    ASSERT_EQ(settings.count, 10);
    ASSERT_STREQ(reinterpret_cast<const char *>(settings.pairs[9].name), "NAME9");
}

TEST_F(MareaSettingsBuilderTest, RejectsArgumentWithNoEquals)
{
    ASSERT_FALSE(MareaSettingsBuilderAddPair(&builder, reinterpret_cast<const MAREA_Character8 *>("NO_EQUALS_SIGN")));
}

TEST_F(MareaSettingsBuilderTest, RejectsNullArguments)
{
    ASSERT_FALSE(MareaSettingsBuilderAddPair(nullptr, reinterpret_cast<const MAREA_Character8 *>("A=1")));
    ASSERT_FALSE(MareaSettingsBuilderAddPair(&builder, nullptr));
}
