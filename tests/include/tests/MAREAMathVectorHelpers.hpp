// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAMATHVECTORHELPERS_HPP)
    #define MAREA_TESTS_MAREAMATHVECTORHELPERS_HPP

    #include "tests/dependencies.hpp"

    #include "marea/base/MAREAMathLibrary.h"

    #include <array>
    #include <cmath>
    #include <cstdint>
    #include <cstring>
    #include <limits>
    #include <utility>
    #include <vector>

constexpr std::uint32_t MAREA_MATH_MAXIMUM_ULP{ 2 };

constexpr float MAREA_MATH_NAN{ std::numeric_limits<float>::quiet_NaN() };

constexpr float MAREA_MATH_INFINITY{ std::numeric_limits<float>::infinity() };

constexpr std::int32_t MAREA_MATH_SMALLEST{ std::numeric_limits<std::int32_t>::min() };

constexpr std::int32_t MAREA_MATH_LARGEST{ std::numeric_limits<std::int32_t>::max() };

using MAREA_FloatLanes = std::array<float, 4>;

using MAREA_IntegerLanes = std::array<std::int32_t, 4>;

inline std::uint32_t MAREA_floatBits(float value)
{
    std::uint32_t bits{ 0 };

    std::memcpy(&bits, &value, sizeof(bits));

    return bits;
}

inline std::uint32_t MAREA_ulpDistance(float left, float right)
{
    if (std::isnan(left) || std::isnan(right)) {
        return (std::isnan(left) && std::isnan(right)) ? 0U : std::numeric_limits<std::uint32_t>::max();
    }

    const auto LINE{ [](float value) {
        const std::uint32_t BITS{ MAREA_floatBits(value) };

        const std::int64_t MAGNITUDE{ static_cast<std::int64_t>(BITS & 0x7FFFFFFFU) };

        return ((BITS & 0x80000000U) != 0U) ? -MAGNITUDE : MAGNITUDE;
    } };

    const std::int64_t DIFFERENCE{ LINE(left) - LINE(right) };

    return static_cast<std::uint32_t>((DIFFERENCE < 0) ? -DIFFERENCE : DIFFERENCE);
}

template <typename Vector, typename Lanes>
Vector MAREA_vectorOf(const Lanes &lanes, std::size_t count)
{
    static_assert(sizeof(Vector) == sizeof(Lanes), "a vector of four byte lanes is 16 bytes");

    Lanes active{};

    for (std::size_t lane{ 0 }; lane < count; ++lane) {
        active[lane] = lanes[lane];
    }

    Vector result;

    std::memcpy(&result, active.data(), sizeof(result));

    return result;
}

template <typename Vector>
Vector MAREA_garbage()
{
    Vector result;

    std::memset(&result, 0xAB, sizeof(result));

    return result;
}

template <typename Lanes, typename Vector>
Lanes MAREA_lanesOf(const Vector &vector)
{
    static_assert(sizeof(Vector) == sizeof(Lanes), "a vector of four byte lanes is 16 bytes");

    Lanes result{};

    std::memcpy(result.data(), &vector, sizeof(result));

    return result;
}

template <typename Vector>
void MAREA_expectFloats(const Vector &result, const MAREA_FloatLanes &expected, std::size_t count, const char *label, std::size_t index = 0)
{
    const MAREA_FloatLanes ACTUAL{ MAREA_lanesOf<MAREA_FloatLanes>(result) };

    for (std::size_t lane{ 0 }; lane < 4; ++lane) {
        if (lane < count) {
            EXPECT_LE(MAREA_ulpDistance(ACTUAL[lane], expected[lane]), MAREA_MATH_MAXIMUM_ULP) << label << " case " << index << " lane " << lane << ": " << ACTUAL[lane] << " against " << expected[lane];

        } else {
            EXPECT_EQ(MAREA_floatBits(ACTUAL[lane]), 0U) << label << " case " << index << ": padding lane " << lane << " must be zero.";
        }
    }
}

template <typename Vector>
void MAREA_expectFloatBits(const Vector &result, const MAREA_FloatLanes &expected, std::size_t count, const char *label)
{
    const MAREA_FloatLanes ACTUAL{ MAREA_lanesOf<MAREA_FloatLanes>(result) };

    for (std::size_t lane{ 0 }; lane < 4; ++lane) {
        const float WANTED{ (lane < count) ? expected[lane] : 0.0F };

        if (std::isnan(WANTED)) {
            EXPECT_TRUE(std::isnan(ACTUAL[lane])) << label << " lane " << lane << " must be a NaN.";

        } else {
            EXPECT_EQ(MAREA_floatBits(ACTUAL[lane]), MAREA_floatBits(WANTED)) << label << " lane " << lane << ": " << ACTUAL[lane] << " against " << WANTED;
        }
    }
}

template <typename Vector>
void MAREA_expectIntegers(const Vector &result, const MAREA_IntegerLanes &expected, std::size_t count, const char *label)
{
    const MAREA_IntegerLanes ACTUAL{ MAREA_lanesOf<MAREA_IntegerLanes>(result) };

    for (std::size_t lane{ 0 }; lane < 4; ++lane) {
        EXPECT_EQ(ACTUAL[lane], (lane < count) ? expected[lane] : 0) << label << " lane " << lane << " differs.";
    }
}

inline float MAREA_product(float left, float right)
{
    volatile float product = left * right;

    return product;
}

inline float MAREA_referenceDot(const MAREA_FloatLanes &left, const MAREA_FloatLanes &right, std::size_t count)
{
    float sum{ MAREA_product(left[0], right[0]) };

    for (std::size_t lane{ 1 }; lane < count; ++lane) {
        sum += MAREA_product(left[lane], right[lane]);
    }

    return sum;
}

inline float MAREA_referenceLength(const MAREA_FloatLanes &value, std::size_t count)
{
    return std::sqrt(MAREA_referenceDot(value, value, count));
}

inline float MAREA_referenceDistance(const MAREA_FloatLanes &left, const MAREA_FloatLanes &right, std::size_t count)
{
    MAREA_FloatLanes difference{};

    for (std::size_t lane{ 0 }; lane < count; ++lane) {
        difference[lane] = left[lane] - right[lane];
    }

    return MAREA_referenceLength(difference, count);
}

inline MAREA_FloatLanes MAREA_referenceNormalize(const MAREA_FloatLanes &value, std::size_t count)
{
    MAREA_FloatLanes result{};

    const float LENGTH{ MAREA_referenceLength(value, count) };

    if (LENGTH == 0.0F) {
        return result;
    }

    const float INVERSE{ 1.0F / LENGTH };

    for (std::size_t lane{ 0 }; lane < count; ++lane) {
        result[lane] = value[lane] * INVERSE;
    }

    return result;
}

inline MAREA_FloatLanes MAREA_referenceMix(const MAREA_FloatLanes &left, const MAREA_FloatLanes &right, float amount, std::size_t count)
{
    MAREA_FloatLanes result{};

    for (std::size_t lane{ 0 }; lane < count; ++lane) {
        result[lane] = left[lane] + MAREA_product(right[lane] - left[lane], amount);
    }

    return result;
}

inline MAREA_FloatLanes MAREA_referenceCross(const MAREA_FloatLanes &left, const MAREA_FloatLanes &right)
{
    return { MAREA_product(left[1], right[2]) - MAREA_product(left[2], right[1]), MAREA_product(left[2], right[0]) - MAREA_product(left[0], right[2]), MAREA_product(left[0], right[1]) - MAREA_product(left[1], right[0]), 0.0F };
}

struct MAREA_MathRandom
{
    std::uint32_t state;

    float MAREA_range(float low, float high)
    {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;

        const float UNIT{ static_cast<float>(state >> 8U) / static_cast<float>(1U << 24U) };

        return low + ((high - low) * UNIT);
    }
};

using MAREA_FloatPair = std::pair<MAREA_FloatLanes, MAREA_FloatLanes>;

inline const std::vector<MAREA_FloatPair> &MAREA_vectorPairCases()
{
    static const std::vector<MAREA_FloatPair> CASES{ [] {
        float tiny{ 0.0F };

        const std::uint32_t TINY_BITS{ 0x00001000U };

        std::memcpy(&tiny, &TINY_BITS, sizeof(tiny));

        const float SMALLEST{ std::numeric_limits<float>::min() };

        std::vector<MAREA_FloatPair> cases{
            { { 0.0F, 0.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 0.0F, 0.0F } },
            { { 1.0F, 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F, 0.0F } },
            { { -0.0F, -0.0F, -0.0F, -0.0F }, { 0.0F, 0.0F, 0.0F, 0.0F } },
            { { 1.0F, 2.0F, 3.0F, 4.0F }, { 4.0F, 3.0F, 2.0F, 1.0F } },
            { { tiny, tiny, tiny, tiny }, { tiny, -tiny, tiny, -tiny } },
            { { SMALLEST, SMALLEST, SMALLEST, SMALLEST }, { 1.0F, 1.0F, 1.0F, 1.0F } },
            { { 1.0e30F, 1.0e30F, 1.0e30F, 1.0e30F }, { -1.0e30F, 1.0e30F, -1.0e30F, 1.0e30F } },
            { { 1.0e-30F, 1.0e-30F, 1.0e-30F, 1.0e-30F }, { 1.0e-30F, 1.0e-30F, 1.0e-30F, 1.0e-30F } },
            { { MAREA_MATH_NAN, 1.0F, 1.0F, 1.0F }, { 1.0F, 1.0F, 1.0F, 1.0F } },
            { { MAREA_MATH_INFINITY, 0.0F, 1.0F, -1.0F }, { 1.0F, 1.0F, 1.0F, 1.0F } },
        };

        const auto APPEND{ [&cases](std::uint32_t seed, float low, float high) {
        MAREA_MathRandom random{ seed };

        for (std::size_t index{ 0 }; index < 2048; ++index) {
            MAREA_FloatPair pair{};

            for (std::size_t lane{ 0 }; lane < 4; ++lane) {
                pair.first[lane] = random.MAREA_range(low, high);
                pair.second[lane] = random.MAREA_range(low, high);
            }

            cases.push_back(pair);
        }
    } };

APPEND
(0x5EED0001U, 0.1F, 4.0F);
APPEND
(0x5EED0002U, -4.0F, 4.0F);

return cases;
}
()
}
;

return CASES;
}

inline float MAREA_mixParameter(std::size_t index)
{
    constexpr std::array<float, 6> PARAMETERS{ 0.25F, 0.5F, 0.0F, 1.0F, 1.5F, -0.5F };

    return PARAMETERS[index % PARAMETERS.size()];
}

template <typename Vector>
struct MAREA_MathFloatOperations
{
    using Binary = void(MAREA_CALL *)(const Vector *, const Vector *, Vector *);
    using Unary = void(MAREA_CALL *)(const Vector *, Vector *);

    std::size_t lanes;
    Binary add;
    Binary subtract;
    Binary multiply;
    Binary divide;
    void(MAREA_CALL *scale)(const Vector *, MAREA_Float32, Vector *);
    MAREA_Float32(MAREA_CALL *dot)(const Vector *, const Vector *);
    MAREA_Float32(MAREA_CALL *length)(const Vector *);
    MAREA_Float32(MAREA_CALL *distance)(const Vector *, const Vector *);
    Unary normalize;
    void(MAREA_CALL *mix)(const Vector *, const Vector *, MAREA_Float32, Vector *);
    Binary minimum;
    Binary maximum;
    void(MAREA_CALL *clamp)(const Vector *, const Vector *, const Vector *, Vector *);
    Unary absolute;
    Unary sine;
    Unary cosine;
    Unary tangent;
    Unary arcSine;
    Unary arcCosine;
    Unary arcTangent;
    Unary radians;
    Unary degrees;
    Unary exponential;
    Unary logarithm;
    Unary squareRoot;
    Unary inverseSquareRoot;
    Binary power;

    Vector MAREA_of(const MAREA_FloatLanes &values) const
    {
        return MAREA_vectorOf<Vector>(values, lanes);
    }

    Vector MAREA_unary(Unary operation, const MAREA_FloatLanes &value) const
    {
        const Vector INPUT{ MAREA_of(value) };

        Vector result{ MAREA_garbage<Vector>() };

        operation(&INPUT, &result);

        return result;
    }

    Vector MAREA_binary(Binary operation, const MAREA_FloatLanes &left, const MAREA_FloatLanes &right) const
    {
        const Vector LEFT{ MAREA_of(left) };

        const Vector RIGHT{ MAREA_of(right) };

        Vector result{ MAREA_garbage<Vector>() };

        operation(&LEFT, &RIGHT, &result);

        return result;
    }

    void MAREA_checkDot() const
    {
        std::size_t index{ 0 };

        for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
            const Vector LEFT{ MAREA_of(pair.first) };

            const Vector RIGHT{ MAREA_of(pair.second) };

            EXPECT_LE(MAREA_ulpDistance(dot(&LEFT, &RIGHT), MAREA_referenceDot(pair.first, pair.second, lanes)), MAREA_MATH_MAXIMUM_ULP) << "dot case " << index;

            ++index;
        }
    }

    void MAREA_checkLengthAndDistance() const
    {
        std::size_t index{ 0 };

        for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
            const Vector LEFT{ MAREA_of(pair.first) };

            const Vector RIGHT{ MAREA_of(pair.second) };

            EXPECT_LE(MAREA_ulpDistance(length(&LEFT), MAREA_referenceLength(pair.first, lanes)), MAREA_MATH_MAXIMUM_ULP) << "length case " << index;

            EXPECT_LE(MAREA_ulpDistance(distance(&LEFT, &RIGHT), MAREA_referenceDistance(pair.first, pair.second, lanes)), MAREA_MATH_MAXIMUM_ULP) << "distance case " << index;

            ++index;
        }

        const Vector ZERO{ MAREA_of({ 0.0F, 0.0F, 0.0F, 0.0F }) };

        EXPECT_EQ(MAREA_floatBits(distance(&ZERO, &ZERO)), 0U) << "The distance of a point to itself must be zero.";
    }

    void MAREA_checkNormalize() const
    {
        std::size_t index{ 0 };

        for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
            MAREA_expectFloats(MAREA_unary(normalize, pair.first), MAREA_referenceNormalize(pair.first, lanes), lanes, "normalize", index);

            ++index;
        }

        MAREA_expectFloatBits(MAREA_unary(normalize, { 0.0F, 0.0F, 0.0F, 0.0F }), { 0.0F, 0.0F, 0.0F, 0.0F }, lanes, "normalize of zero");
    }

    void MAREA_checkHugeLength() const
    {
        const Vector HUGE_VECTOR{ MAREA_of({ 1.0e30F, 1.0e30F, 1.0e30F, 1.0e30F }) };

        const Vector OPPOSITE{ MAREA_of({ -1.0e30F, -1.0e30F, -1.0e30F, -1.0e30F }) };

        EXPECT_EQ(length(&HUGE_VECTOR), MAREA_MATH_INFINITY) << "The square of a huge length must overflow to infinity.";

        EXPECT_EQ(distance(&HUGE_VECTOR, &OPPOSITE), MAREA_MATH_INFINITY) << "The square of a huge distance must overflow to infinity.";
    }

    void MAREA_checkMix() const
    {
        std::size_t index{ 0 };

        for (const MAREA_FloatPair &pair : MAREA_vectorPairCases()) {
            const Vector LEFT{ MAREA_of(pair.first) };

            const Vector RIGHT{ MAREA_of(pair.second) };

            Vector result{ MAREA_garbage<Vector>() };

            mix(&LEFT, &RIGHT, MAREA_mixParameter(index), &result);

            MAREA_expectFloats(result, MAREA_referenceMix(pair.first, pair.second, MAREA_mixParameter(index), lanes), lanes, "mix", index);

            ++index;
        }
    }

    void MAREA_checkArithmetic() const
    {
        const MAREA_FloatLanes A{ 1.0F, 2.0F, 3.0F, 4.0F };

        const MAREA_FloatLanes B{ 5.0F, 6.0F, 7.0F, 8.0F };

        MAREA_expectFloatBits(MAREA_binary(add, A, B), { 6.0F, 8.0F, 10.0F, 12.0F }, lanes, "add");

        MAREA_expectFloatBits(MAREA_binary(subtract, A, B), { -4.0F, -4.0F, -4.0F, -4.0F }, lanes, "subtract");

        MAREA_expectFloatBits(MAREA_binary(multiply, A, B), { 5.0F, 12.0F, 21.0F, 32.0F }, lanes, "multiply");

        MAREA_expectFloatBits(MAREA_binary(divide, A, B), { 1.0F / 5.0F, 2.0F / 6.0F, 3.0F / 7.0F, 4.0F / 8.0F }, lanes, "divide");

        const Vector VALUE{ MAREA_of(A) };

        Vector result{ MAREA_garbage<Vector>() };

        scale(&VALUE, 2.5F, &result);

        MAREA_expectFloatBits(result, { 2.5F, 5.0F, 7.5F, 10.0F }, lanes, "scale");
    }

    void MAREA_checkZeroByZero() const
    {
        const MAREA_FloatLanes RESULT{ MAREA_lanesOf<MAREA_FloatLanes>(MAREA_binary(divide, { 0.0F, 0.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 0.0F, 0.0F })) };

        for (std::size_t lane{ 0 }; lane < lanes; ++lane) {
            EXPECT_TRUE(std::isnan(RESULT[lane])) << "Zero by zero must be a NaN in lane " << lane;

            EXPECT_FALSE(std::signbit(RESULT[lane])) << "Zero by zero must have the same sign on every target in lane " << lane;
        }
    }

    void MAREA_checkMinimumMaximumAbsolute() const
    {
        const MAREA_FloatLanes A{ -3.0F, 7.0F, -0.5F, 2.0F };

        const MAREA_FloatLanes B{ 2.0F, 1.0F, 0.5F, -2.0F };

        MAREA_expectFloatBits(MAREA_binary(minimum, A, B), { -3.0F, 1.0F, -0.5F, -2.0F }, lanes, "minimum");

        MAREA_expectFloatBits(MAREA_binary(maximum, A, B), { 2.0F, 7.0F, 0.5F, 2.0F }, lanes, "maximum");

        MAREA_expectFloatBits(MAREA_binary(minimum, { MAREA_MATH_NAN, 1.0F, MAREA_MATH_NAN, 1.0F }, { 1.0F, MAREA_MATH_NAN, 1.0F, MAREA_MATH_NAN }), { 1.0F, MAREA_MATH_NAN, 1.0F, MAREA_MATH_NAN }, lanes, "minimum with a NaN");

        MAREA_expectFloatBits(MAREA_unary(absolute, { -3.0F, -0.0F, MAREA_MATH_NAN, -MAREA_MATH_INFINITY }), { 3.0F, 0.0F, MAREA_MATH_NAN, MAREA_MATH_INFINITY }, lanes, "absolute");
    }

    void MAREA_expectClamp(const MAREA_FloatLanes &value, const MAREA_FloatLanes &low, const MAREA_FloatLanes &high, const MAREA_FloatLanes &expected, const char *label) const
    {
        const Vector VALUE{ MAREA_of(value) };

        const Vector LOW{ MAREA_of(low) };

        const Vector HIGH{ MAREA_of(high) };

        Vector result{ MAREA_garbage<Vector>() };

        clamp(&VALUE, &LOW, &HIGH, &result);

        MAREA_expectFloatBits(result, expected, lanes, label);
    }

    void MAREA_checkClamp() const
    {
        MAREA_expectClamp({ -5.0F, 0.5F, 10.0F, 1.5F }, { 0.0F, 0.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F, 1.0F }, { 0.0F, 0.5F, 1.0F, 1.0F }, "clamp");

        MAREA_expectClamp({ 2.5F, 0.0F, 9.0F, 3.0F }, { 4.0F, 4.0F, 4.0F, 4.0F }, { 1.0F, 1.0F, 1.0F, 1.0F }, { 4.0F, 4.0F, 1.0F, 4.0F }, "clamp to an inverted range");

        MAREA_expectClamp({ 2.0F, 2.0F, MAREA_MATH_NAN, -0.0F }, { MAREA_MATH_NAN, 0.0F, 0.0F, 0.0F }, { MAREA_MATH_NAN, MAREA_MATH_NAN, 1.0F, -0.0F }, { 2.0F, 2.0F, MAREA_MATH_NAN, -0.0F }, "clamp with a NaN");
    }

    void MAREA_expectEachLane(Unary operation, const MAREA_FloatLanes &value, float (*reference)(float), const char *label) const
    {
        MAREA_FloatLanes expected{};

        for (std::size_t lane{ 0 }; lane < lanes; ++lane) {
            expected[lane] = reference(value[lane]);
        }

        MAREA_expectFloats(MAREA_unary(operation, value), expected, lanes, label);
    }

    void MAREA_checkTrigonometry() const
    {
        const MAREA_FloatLanes ANGLES{ 0.0F, 0.5F, -0.75F, 1.0F };

        MAREA_expectEachLane(sine, ANGLES, +[](float value) { return std::sin(value); }, "sine");

        MAREA_expectEachLane(cosine, ANGLES, +[](float value) { return std::cos(value); }, "cosine");

        MAREA_expectEachLane(tangent, ANGLES, +[](float value) { return std::tan(value); }, "tangent");

        MAREA_expectEachLane(arcSine, ANGLES, +[](float value) { return std::asin(value); }, "arc sine");

        MAREA_expectEachLane(arcCosine, ANGLES, +[](float value) { return std::acos(value); }, "arc cosine");

        MAREA_expectEachLane(arcTangent, ANGLES, +[](float value) { return std::atan(value); }, "arc tangent");

        MAREA_expectFloats(MAREA_unary(radians, { 180.0F, 90.0F, 0.0F, -180.0F }), { 3.14159265F, 1.57079633F, 0.0F, -3.14159265F }, lanes, "radians");

        MAREA_expectFloats(MAREA_unary(degrees, { 3.14159265F, 1.57079633F, 0.0F, -3.14159265F }), { 180.0F, 90.0F, 0.0F, -180.0F }, lanes, "degrees");
    }

    void MAREA_checkExponentialAndRoots() const
    {
        const MAREA_FloatLanes VALUES{ 1.0F, 4.0F, 9.0F, 0.25F };

        MAREA_expectEachLane(exponential, { 0.0F, 1.0F, -2.0F, 0.5F }, +[](float value) { return std::exp(value); }, "exponential");

        MAREA_expectEachLane(logarithm, VALUES, +[](float value) { return std::log(value); }, "logarithm");

        MAREA_expectEachLane(squareRoot, VALUES, +[](float value) { return std::sqrt(value); }, "square root");

        MAREA_expectEachLane(inverseSquareRoot, VALUES, +[](float value) { return 1.0F / std::sqrt(value); }, "inverse square root");

        MAREA_expectFloatBits(MAREA_unary(inverseSquareRoot, { 0.0F, -0.0F, 4.0F, 0.0F }), { 0.0F, 0.0F, 0.5F, 0.0F }, lanes, "inverse square root of zero");

        MAREA_expectFloats(MAREA_binary(power, { 2.0F, 9.0F, 3.0F, 5.0F }, { 10.0F, 0.5F, 2.0F, 0.0F }), { 1024.0F, 3.0F, 9.0F, 1.0F }, lanes, "power");
    }
};

template <typename Vector>
struct MAREA_MathIntegerOperations
{
    using Binary = void(MAREA_CALL *)(const Vector *, const Vector *, Vector *);

    std::size_t lanes;
    Binary add;
    Binary subtract;
    Binary multiply;
    Binary divide;
    void(MAREA_CALL *scale)(const Vector *, MAREA_Sinteger32, Vector *);
    MAREA_Sinteger32(MAREA_CALL *dot)(const Vector *, const Vector *);
    Binary minimum;
    Binary maximum;
    void(MAREA_CALL *clamp)(const Vector *, const Vector *, const Vector *, Vector *);
    void(MAREA_CALL *absolute)(const Vector *, Vector *);

    Vector MAREA_of(const MAREA_IntegerLanes &values) const
    {
        return MAREA_vectorOf<Vector>(values, lanes);
    }

    Vector MAREA_binary(Binary operation, const MAREA_IntegerLanes &left, const MAREA_IntegerLanes &right) const
    {
        const Vector LEFT{ MAREA_of(left) };

        const Vector RIGHT{ MAREA_of(right) };

        Vector result{ MAREA_garbage<Vector>() };

        operation(&LEFT, &RIGHT, &result);

        return result;
    }

    void MAREA_checkWrapping() const
    {
        const MAREA_IntegerLanes A{ MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST, -11, 9 };

        const MAREA_IntegerLanes B{ 1, 1, 8, -12 };

        MAREA_expectIntegers(MAREA_binary(add, A, B), { MAREA_MATH_SMALLEST, MAREA_MATH_SMALLEST + 1, -3, -3 }, lanes, "add");

        MAREA_expectIntegers(MAREA_binary(subtract, A, B), { MAREA_MATH_LARGEST - 1, MAREA_MATH_LARGEST, -19, 21 }, lanes, "subtract");

        const MAREA_IntegerLanes C{ MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST, -3, 4 };

        MAREA_expectIntegers(MAREA_binary(multiply, C, { 2, -1, 5, -6 }), { -2, MAREA_MATH_SMALLEST, -15, -24 }, lanes, "multiply");

        const Vector VALUE{ MAREA_of(C) };

        Vector result{ MAREA_garbage<Vector>() };

        scale(&VALUE, 2, &result);

        MAREA_expectIntegers(result, { -2, 0, -6, 8 }, lanes, "scale");
    }

    void MAREA_checkDot() const
    {
        const Vector LEFT{ MAREA_of({ MAREA_MATH_LARGEST, MAREA_MATH_SMALLEST, -3, 4 }) };

        const Vector RIGHT{ MAREA_of({ 2, 1, 5, -6 }) };

        const std::array<std::int32_t, 3> EXPECTED{ MAREA_MATH_LARGEST - 1, MAREA_MATH_LARGEST - 16, MAREA_MATH_LARGEST - 40 };

        EXPECT_EQ(dot(&LEFT, &RIGHT), EXPECTED[lanes - 2]) << "The dot product must wrap.";
    }

    void MAREA_checkMinimumMaximumClampAbsolute() const
    {
        const MAREA_IntegerLanes A{ -3, 7, 5, -9 };

        const MAREA_IntegerLanes B{ 2, 1, 5, 0 };

        MAREA_expectIntegers(MAREA_binary(minimum, A, B), { -3, 1, 5, -9 }, lanes, "minimum");

        MAREA_expectIntegers(MAREA_binary(maximum, A, B), { 2, 7, 5, 0 }, lanes, "maximum");

        const Vector VALUE{ MAREA_of({ -5, 0, 10, 2 }) };

        const Vector LOW{ MAREA_of({ 0, 0, 0, 4 }) };

        const Vector HIGH{ MAREA_of({ 1, 1, 1, 1 }) };

        Vector clamped{ MAREA_garbage<Vector>() };

        clamp(&VALUE, &LOW, &HIGH, &clamped);

        MAREA_expectIntegers(clamped, { 0, 0, 1, 4 }, lanes, "clamp");

        const Vector SIGNED{ MAREA_of({ MAREA_MATH_SMALLEST, -5, 7, -1 }) };

        Vector magnitude{ MAREA_garbage<Vector>() };

        absolute(&SIGNED, &magnitude);

        MAREA_expectIntegers(magnitude, { MAREA_MATH_SMALLEST, 5, 7, 1 }, lanes, "absolute");
    }
};

    #define MAREA_MATH_FLOAT_OPERATIONS(count)                                                                                                          \
        MAREA_MathFloatOperations<MAREA_MathVector##count##Float32>                                                                                     \
        {                                                                                                                                               \
            count, &MAREA_MathAddVector##count##Float32, &MAREA_MathSubtractVector##count##Float32, &MAREA_MathMultiplyVector##count##Float32,          \
                &MAREA_MathDivideVector##count##Float32, &MAREA_MathScaleVector##count##Float32, &MAREA_MathDotVector##count##Float32,                  \
                &MAREA_MathLengthVector##count##Float32, &MAREA_MathDistanceVector##count##Float32, &MAREA_MathNormalizeVector##count##Float32,         \
                &MAREA_MathMixVector##count##Float32, &MAREA_MathMinimumVector##count##Float32, &MAREA_MathMaximumVector##count##Float32,               \
                &MAREA_MathClampVector##count##Float32, &MAREA_MathAbsoluteVector##count##Float32, &MAREA_MathSineVector##count##Float32,               \
                &MAREA_MathCosineVector##count##Float32, &MAREA_MathTangentVector##count##Float32, &MAREA_MathArcSineVector##count##Float32,            \
                &MAREA_MathArcCosineVector##count##Float32, &MAREA_MathArcTangentVector##count##Float32, &MAREA_MathRadiansVector##count##Float32,      \
                &MAREA_MathDegreesVector##count##Float32, &MAREA_MathExponentialVector##count##Float32, &MAREA_MathLogarithmVector##count##Float32,     \
                &MAREA_MathSquareRootVector##count##Float32, &MAREA_MathInverseSquareRootVector##count##Float32, &MAREA_MathPowerVector##count##Float32 \
        }

    #define MAREA_MATH_INTEGER_OPERATIONS(count)                                                                                                        \
        MAREA_MathIntegerOperations<MAREA_MathVector##count##Sinteger32>                                                                                \
        {                                                                                                                                               \
            count, &MAREA_MathAddVector##count##Sinteger32, &MAREA_MathSubtractVector##count##Sinteger32, &MAREA_MathMultiplyVector##count##Sinteger32, \
                &MAREA_MathDivideVector##count##Sinteger32, &MAREA_MathScaleVector##count##Sinteger32, &MAREA_MathDotVector##count##Sinteger32,         \
                &MAREA_MathMinimumVector##count##Sinteger32, &MAREA_MathMaximumVector##count##Sinteger32, &MAREA_MathClampVector##count##Sinteger32,    \
                &MAREA_MathAbsoluteVector##count##Sinteger32                                                                                            \
        }

#endif
