#include "tests/dependencies.hpp"

#include "marea/base/MAREAMacros.h"

#include <cstdio>
#include <cstring>

TEST(MAREASettingsHeader, VersionPartsAreWellFormed)
{
    GTEST_COUT << "Validating the version macros are non-negative and the build counter has started." << std::endl;

    EXPECT_GE(MAREA_VERSION_MAJOR, 0) << "MAREA_VERSION_MAJOR must not be negative.";

    EXPECT_GE(MAREA_VERSION_MINOR, 0) << "MAREA_VERSION_MINOR must not be negative.";

    EXPECT_GE(MAREA_VERSION_PATCH, 0) << "MAREA_VERSION_PATCH must not be negative.";

    EXPECT_GE(MAREA_VERSION_BUILD, 1) << "MAREA_VERSION_BUILD is a monotonic counter that starts at 1.";
}

TEST(MAREASettingsHeader, VersionStringIsDotJoined)
{
    GTEST_COUT << "Validating MAREA_VERSION is the dot-joined concatenation of its four parts." << std::endl;

    char expected[32];

    MAREA_IGNORE(std::snprintf(expected, sizeof(expected), "%d.%d.%d.%d", MAREA_VERSION_MAJOR, MAREA_VERSION_MINOR, MAREA_VERSION_PATCH, MAREA_VERSION_BUILD));

    EXPECT_STREQ(MAREA_VERSION, expected) << "MAREA_VERSION must equal major.minor.patch.build.";
}

TEST(MAREASettingsHeader, MetadataStringsAreNotEmpty)
{
    GTEST_COUT << "Validating MAREA_TITLE, MAREA_LABEL and MAREA_PACKAGE are non-empty strings." << std::endl;

    EXPECT_GT(std::strlen(MAREA_TITLE), 0u) << "MAREA_TITLE must not be empty.";

    EXPECT_GT(std::strlen(MAREA_LABEL), 0u) << "MAREA_LABEL must not be empty.";

    EXPECT_GT(std::strlen(MAREA_PACKAGE), 0u) << "MAREA_PACKAGE must not be empty.";
}

TEST(MAREASettingsHeader, BuildDebugMirrorsTheSetting)
{
    GTEST_COUT << "Validating MAREA_BUILD_DEBUG mirrors SETTING_BUILD_DEBUG from the generated header." << std::endl;

#if defined(SETTING_BUILD_DEBUG)
    EXPECT_EQ(MAREA_BUILD_DEBUG, 1) << "MAREA_BUILD_DEBUG must be 1 when SETTING_BUILD_DEBUG is defined.";
#else
    EXPECT_EQ(MAREA_BUILD_DEBUG, 0) << "MAREA_BUILD_DEBUG must be 0 when SETTING_BUILD_DEBUG is not defined.";
#endif
}
