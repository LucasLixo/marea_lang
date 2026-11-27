// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAMathLibrary.h"

#include "tests/MAREAShippedLibraries.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> MAREA_mathNames()
    {
        const std::vector<std::string> FLOAT_OPERATIONS{
        "add",
        "subtract",
        "multiply",
        "divide",
        "scale",
        "dot",
        "length",
        "distance",
        "normalize",
        "mix",
        "minimum",
        "maximum",
        "clamp",
        "absolute",
        "sine",
        "cosine",
        "tangent",
        "arc_sine",
        "arc_cosine",
        "arc_tangent",
        "radians",
        "degrees",
        "exponential",
        "logarithm",
        "square_root",
        "inverse_square_root",
        "power"
        };

        const std::vector<std::string> INTEGER_OPERATIONS{
        "add",
        "subtract",
        "multiply",
        "divide",
        "scale",
        "dot",
        "minimum",
        "maximum",
        "clamp",
        "absolute"
        };

        std::vector<std::string> names{ "cross_vector3_float32", "cross_vector3_sinteger32" };

        for (const char *size : { "2", "3", "4" }) {
            for (const std::string &operation : FLOAT_OPERATIONS) {
                names.push_back(operation + "_vector" + size + "_float32");
            }

            for (const std::string &operation : INTEGER_OPERATIONS) {
                names.push_back(operation + "_vector" + size + "_sinteger32");
            }
        }

        return names;
    }

    MAREA_HostFunction MAREA_findMath(const MAREA_ShippedRegistry &registry, const std::string &name)
    {
        return MAREA_TypeRegistryFindFunction(registry.registry, "marea.math", MAREA_INTEGER_SIZE(10), name.c_str(), name.size());
    }
} // namespace

TEST(MAREAMathLibrary, FlagIsItsOwnBitOfTheDefaultMask)
{
    GTEST_COUT << "Validating marea.math has its own library bit, on by default." << std::endl;

    EXPECT_EQ(static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MATH), MAREA_UINTEGER_32(1) << 8) << "marea.math takes bit 8.";

    EXPECT_NE((MAREA_LIBRARY_MASK_DEFAULT & MAREA_LIBRARY_FLAG_MAREA_MATH), 0U) << "The default mask must turn marea.math on.";
}

TEST(MAREAMathLibrary, RegisterGivesEveryVectorOperation)
{
    GTEST_COUT << "Validating each vector operation is registered under marea.math." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const std::vector<std::string> NAMES{ MAREA_mathNames() };

    ASSERT_EQ(NAMES.size(), 113U) << "The vector inventory has 113 operations.";

    for (const std::string &name : NAMES) {
        EXPECT_NE(MAREA_findMath(registry, name), nullptr) << "marea.math must register " << name;
    }
}

TEST(MAREAMathLibrary, RegisterWithTheFlagOffGivesNoName)
{
    GTEST_COUT << "Validating a mask without marea.math knows no vector operation." << std::endl;

    MAREA_ShippedRegistry registry(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MATH));

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "A library that is off is no error.";

    for (const std::string &name : MAREA_mathNames()) {
        EXPECT_EQ(MAREA_findMath(registry, name), nullptr) << "A library that is off must not register " << name;
    }
}

TEST(MAREAMathLibrary, RegisterRejectsANullRegistry)
{
    GTEST_COUT << "Validating the registration reports a null registry." << std::endl;

    EXPECT_EQ(MAREA_MathLibraryRegister(nullptr), MAREA_TYPE_REGISTRY_EXCEPT_ERROR_NULL_POINTER) << "A null registry is an error.";
}

TEST(MAREAMathLibrary, WrapperReadsBorrowedVectorsAndWritesAStructureResult)
{
    GTEST_COUT << "Validating the registered wrapper of a vector addition." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_HostFunction ADD{ MAREA_findMath(registry, "add_vector3_float32") };

    ASSERT_NE(ADD, nullptr) << "The addition must be registered.";

    std::array<float, 4> left{ 1.0F, 2.0F, 3.0F, 0.0F };
    std::array<float, 4> right{ 4.0F, 5.0F, 6.0F, 0.0F };
    std::array<float, 4> sum{};
    std::array<MAREA_Null, 2> arguments{ left.data(), right.data() };
    MAREA_HostContext *context{ nullptr };

    ADD(context, arguments.data(), sum.data());

    EXPECT_EQ(sum, (std::array<float, 4>{ 5.0F, 7.0F, 9.0F, 0.0F })) << "The wrapper must write the sum.";
}

TEST(MAREAMathLibrary, WrapperReadsScalarsAndWritesScalarResults)
{
    GTEST_COUT << "Validating the registered wrappers of the dot product and of the scale." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_HostFunction DOT{ MAREA_findMath(registry, "dot_vector3_float32") };

    const MAREA_HostFunction SCALE{ MAREA_findMath(registry, "scale_vector4_sinteger32") };

    ASSERT_NE(DOT, nullptr) << "The dot product must be registered.";

    ASSERT_NE(SCALE, nullptr) << "The scale must be registered.";

    std::array<float, 4> left{ 1.0F, 2.0F, 3.0F, 0.0F };
    std::array<float, 4> right{ 4.0F, 5.0F, 6.0F, 0.0F };
    std::array<MAREA_Null, 2> dotArguments{ left.data(), right.data() };
    std::uint64_t slot{ 0 };

    DOT(nullptr, dotArguments.data(), &slot);

    double wide{ 0.0 };

    std::memcpy(&wide, &slot, sizeof(wide));

    EXPECT_EQ(wide, 32.0) << "A float32 result is stored in the register form.";

    std::array<std::int32_t, 4> value{ 1, -2, 3, -4 };
    std::uint64_t factor{ 3 };
    std::array<std::int32_t, 4> scaled{};
    std::array<MAREA_Null, 2> scaleArguments{ value.data(), &factor };

    SCALE(nullptr, scaleArguments.data(), scaled.data());

    EXPECT_EQ(scaled, (std::array<std::int32_t, 4>{ 3, -6, 9, -12 })) << "The wrapper must read the scalar factor.";
}
