// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "MareaLibraryMask.h"

TEST(MareaLibraryMask, NullTextGivesDefaultMask)
{
    MAREA_LibraryMask mask{ 0 };

    ASSERT_TRUE(MareaLibraryMaskParse(nullptr, &mask));
    ASSERT_EQ(mask, MAREA_LIBRARY_MASK_DEFAULT);
}

TEST(MareaLibraryMask, EmptyTextGivesDefaultMask)
{
    MAREA_LibraryMask mask{ 0 };

    ASSERT_TRUE(MareaLibraryMaskParse("", &mask));
    ASSERT_EQ(mask, MAREA_LIBRARY_MASK_DEFAULT);
}

TEST(MareaLibraryMask, ParsesOneKnownName)
{
    MAREA_LibraryMask mask{ 0 };

    ASSERT_TRUE(MareaLibraryMaskParse("marea.array", &mask));
    ASSERT_EQ(mask, static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_ARRAY));
}

TEST(MareaLibraryMask, ParsesCommaSeparatedNames)
{
    MAREA_LibraryMask mask{ 0 };

    ASSERT_TRUE(MareaLibraryMaskParse("marea.array,c.memory,c.threads", &mask));
    ASSERT_EQ(
        mask,
        static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_ARRAY | MAREA_LIBRARY_FLAG_C_MEMORY | MAREA_LIBRARY_FLAG_C_THREADS));
}

TEST(MareaLibraryMask, RejectsUnknownName)
{
    MAREA_LibraryMask mask{ 0 };

    ASSERT_FALSE(MareaLibraryMaskParse("bogus.lib", &mask));
}

TEST(MareaLibraryMask, RejectsNullOutput)
{
    ASSERT_FALSE(MareaLibraryMaskParse("marea.array", nullptr));
}
