// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "marea/base/MAREAHostContext.h"

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

    std::vector<std::string> MAREA_matrixNames()
    {
        std::vector<std::string> names{
            "translate_matrix4x4_float32",
            "scale_matrix4x4_float32",
            "rotate_matrix4x4_float32",
            "look_at_matrix4x4_float32",
            "perspective_matrix4x4_float32",
            "orthographic_matrix4x4_float32",
            "frustum_matrix4x4_float32"
        };

        for (const char *operation : { "identity", "from_axis_angle", "from_euler", "to_matrix4x4", "multiply", "conjugate", "normalize", "dot", "length", "mix", "spherical_mix" }) {
            names.push_back(std::string(operation) + "_quaternion_float32");
        }

        names.emplace_back("rotate_vector3_quaternion_float32");

        for (const std::string &size : { std::string("2"), std::string("3"), std::string("4") }) {
            const std::string MATRIX{ "_matrix" + size + "x" + size };

            for (const char *lane : { "_float32", "_sinteger32" }) {
                names.push_back("identity" + MATRIX + lane);
                names.push_back("transpose" + MATRIX + lane);
                names.push_back("multiply" + MATRIX + lane);
                names.push_back("multiply" + MATRIX + "_vector" + size + lane);
            }

            names.push_back("determinant" + MATRIX + "_float32");
            names.push_back("inverse" + MATRIX + "_float32");
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

TEST(MAREAMathLibrary, WrapperOfAnIntegerDivisionRaisesTheTrap)
{
    GTEST_COUT << "Validating the registered integer division raises the trap through its context." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_HostFunction DIVIDE{ MAREA_findMath(registry, "divide_vector2_sinteger32") };

    ASSERT_NE(DIVIDE, nullptr) << "The division must be registered.";

    std::array<std::int32_t, 4> left{ 6, 8, 0, 0 };
    std::array<std::int32_t, 4> right{ 2, 0, 0, 0 };
    std::array<std::int32_t, 4> quotient{};
    std::array<MAREA_Null, 2> arguments{ left.data(), right.data() };
    MAREA_HostContext context{};

    ASSERT_EQ(MAREA_HostContextInitialize(&context, nullptr, nullptr), MAREA_HOST_CONTEXT_EXCEPT_SUCCESS) << "The context must be made.";

    DIVIDE(&context, arguments.data(), quotient.data());

    EXPECT_EQ(MAREA_HostContextGetTrap(&context), MAREA_HOST_TRAP_DIVISION_BY_ZERO) << "A zero divisor must raise the trap.";
}

TEST(MAREAMathLibrary, RegisterGivesEveryMatrixAndQuaternionOperation)
{
    GTEST_COUT << "Validating each matrix and quaternion operation is registered under marea.math." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const std::vector<std::string> NAMES{ MAREA_matrixNames() };

    ASSERT_EQ(NAMES.size(), 49U) << "The matrix and quaternion inventory has 49 operations.";

    for (const std::string &name : NAMES) {
        EXPECT_NE(MAREA_findMath(registry, name), nullptr) << "marea.math must register " << name;
    }
}

TEST(MAREAMathLibrary, RegisterWithTheFlagOffGivesNoMatrixOrQuaternionName)
{
    GTEST_COUT << "Validating a mask without marea.math knows no matrix or quaternion operation." << std::endl;

    MAREA_ShippedRegistry registry(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MATH));

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "A library that is off is no error.";

    for (const std::string &name : MAREA_matrixNames()) {
        EXPECT_EQ(MAREA_findMath(registry, name), nullptr) << "A library that is off must not register " << name;
    }
}

TEST(MAREAMathLibrary, RegisterNeedsNoOtherLibrary)
{
    GTEST_COUT << "Validating marea.math gives every name when each other library is off." << std::endl;

    MAREA_ShippedRegistry registry(static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MATH));

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "marea.math declares no dependency.";

    std::vector<std::string> names{ MAREA_mathNames() };

    const std::vector<std::string> MATRIX_NAMES{ MAREA_matrixNames() };

    names.insert(names.end(), MATRIX_NAMES.begin(), MATRIX_NAMES.end());

    for (const std::string &name : names) {
        EXPECT_NE(MAREA_findMath(registry, name), nullptr) << "marea.math alone must register " << name;
    }
}

TEST(MAREAMathLibrary, WrapperOfAMatrixProductAndOfADeterminant)
{
    GTEST_COUT << "Validating the registered wrappers of a matrix product and of a determinant." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_HostFunction MULTIPLY{ MAREA_findMath(registry, "multiply_matrix2x2_float32") };

    const MAREA_HostFunction DETERMINANT{ MAREA_findMath(registry, "determinant_matrix2x2_float32") };

    ASSERT_NE(MULTIPLY, nullptr) << "The product must be registered.";

    ASSERT_NE(DETERMINANT, nullptr) << "The determinant must be registered.";

    alignas(16) std::array<float, 8> left{ 1.0F, 2.0F, 0.0F, 0.0F, 3.0F, 4.0F, 0.0F, 0.0F };
    alignas(16) std::array<float, 8> right{ 5.0F, 6.0F, 0.0F, 0.0F, 7.0F, 8.0F, 0.0F, 0.0F };
    alignas(16) std::array<float, 8> product{};
    std::array<MAREA_Null, 2> arguments{ left.data(), right.data() };

    MULTIPLY(nullptr, arguments.data(), product.data());

    EXPECT_EQ(product, (std::array<float, 8>{ 23.0F, 34.0F, 0.0F, 0.0F, 31.0F, 46.0F, 0.0F, 0.0F })) << "The wrapper must write the product.";

    std::uint64_t slot{ 0 };

    DETERMINANT(nullptr, arguments.data(), &slot);

    double wide{ 0.0 };

    std::memcpy(&wide, &slot, sizeof(wide));

    EXPECT_EQ(wide, -2.0) << "A float32 result is stored in the register form.";
}

TEST(MAREAMathLibrary, WrapperOfAnOperationWithNoParameterAndOfAQuaternionProduct)
{
    GTEST_COUT << "Validating the registered wrappers of the identity and of the product of quaternions." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_HostFunction IDENTITY{ MAREA_findMath(registry, "identity_quaternion_float32") };

    const MAREA_HostFunction MULTIPLY{ MAREA_findMath(registry, "multiply_quaternion_float32") };

    ASSERT_NE(IDENTITY, nullptr) << "The identity must be registered.";

    ASSERT_NE(MULTIPLY, nullptr) << "The product must be registered.";

    alignas(16) std::array<float, 4> identity{};

    IDENTITY(nullptr, nullptr, identity.data());

    EXPECT_EQ(identity, (std::array<float, 4>{ 0.0F, 0.0F, 0.0F, 1.0F })) << "The wrapper must write the identity.";

    alignas(16) std::array<float, 4> value{ 1.0F, 0.0F, 0.0F, 0.0F };
    alignas(16) std::array<float, 4> product{};
    std::array<MAREA_Null, 2> arguments{ value.data(), identity.data() };

    MULTIPLY(nullptr, arguments.data(), product.data());

    EXPECT_EQ(product, value) << "The wrapper must write the product.";
}

TEST(MAREAMathLibrary, WrapperReadsTheScalarsOfTheOrthographicMatrix)
{
    GTEST_COUT << "Validating the registered wrapper of an operation with six scalar arguments." << std::endl;

    MAREA_ShippedRegistry registry;

    ASSERT_EQ(MAREA_MathLibraryRegister(registry.registry), MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) << "The registration must succeed.";

    const MAREA_HostFunction ORTHOGRAPHIC{ MAREA_findMath(registry, "orthographic_matrix4x4_float32") };

    ASSERT_NE(ORTHOGRAPHIC, nullptr) << "The orthographic matrix must be registered.";

    std::array<double, 6> bounds{ 0.0, 4.0, 0.0, 2.0, 1.0, 3.0 };
    std::array<MAREA_Null, 6> arguments{ &bounds[0], &bounds[1], &bounds[2], &bounds[3], &bounds[4], &bounds[5] };
    alignas(16) std::array<float, 16> matrix{};

    ORTHOGRAPHIC(nullptr, arguments.data(), matrix.data());

    EXPECT_EQ(matrix, (std::array<float, 16>{ 0.5F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, -0.5F, 0.0F, -1.0F, -1.0F, -0.5F, 1.0F })) << "The wrapper must read each scalar as a float32.";
}
