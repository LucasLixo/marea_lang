// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

TEST(MAREACTypes, Float64LiteralKeepsDoublePrecision)
{
    GTEST_COUT << "Validating MAREA_FLOAT_64 builds a double literal, not a float one." << std::endl;

    EXPECT_EQ(sizeof(MAREA_FLOAT_64(0.1)), sizeof(double)) << "MAREA_FLOAT_64 must have the type double.";

    EXPECT_EQ(MAREA_FLOAT_64(0.1), 0.1) << "MAREA_FLOAT_64(0.1) must equal the double literal 0.1.";
}

TEST(MAREACTypes, Float80LiteralKeepsLongDoublePrecision)
{
    GTEST_COUT << "Validating MAREA_FLOAT_80 builds a long double literal, not a float one." << std::endl;

    EXPECT_EQ(sizeof(MAREA_FLOAT_80(0.1)), sizeof(long double)) << "MAREA_FLOAT_80 must have the type long double.";

    EXPECT_EQ(MAREA_FLOAT_80(0.1), 0.1L) << "MAREA_FLOAT_80(0.1) must equal the long double literal 0.1L.";
}

TEST(MAREACTypes, Float32LiteralIsFloat)
{
    GTEST_COUT << "Validating MAREA_FLOAT_32 builds a float literal." << std::endl;

    EXPECT_EQ(sizeof(MAREA_FLOAT_32(0.1)), sizeof(float)) << "MAREA_FLOAT_32 must have the type float.";
}

TEST(MAREACTypes, FloatLiteralsAcceptAnIntegerToken)
{
    GTEST_COUT << "Validating the float literal macros accept a token without a decimal point." << std::endl;

    EXPECT_EQ(sizeof(MAREA_FLOAT_32(1)), sizeof(float)) << "MAREA_FLOAT_32(1) must have the type float.";

    EXPECT_EQ(sizeof(MAREA_FLOAT_64(1)), sizeof(double)) << "MAREA_FLOAT_64(1) must have the type double.";

    EXPECT_EQ(sizeof(MAREA_FLOAT_80(1)), sizeof(long double)) << "MAREA_FLOAT_80(1) must have the type long double.";

    EXPECT_EQ(MAREA_FLOAT_32(2), 2.0F) << "MAREA_FLOAT_32(2) must equal 2.0F.";
}
