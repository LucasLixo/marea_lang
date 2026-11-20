#include "tests/dependencies.hpp"

TEST(MAREAVersion, GetVersion)
{
    GTEST_COUT << "Validating MAREA_GetVersion returns the concatenated MAREA_VERSION string." << std::endl;

    const MAREA_Character8 *version{ MAREA_GetVersion() };

    ASSERT_NE(version, MAREA_NULLPTR) << "MAREA_GetVersion must never return null.";

    EXPECT_STREQ(version, MAREA_VERSION) << "MAREA_GetVersion must return the MAREA_VERSION string.";
}
