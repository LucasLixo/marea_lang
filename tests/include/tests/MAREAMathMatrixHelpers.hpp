// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAMATHMATRIXHELPERS_HPP)
    #define MAREA_TESTS_MAREAMATHMATRIXHELPERS_HPP

    #include "tests/MAREAMathVectorHelpers.hpp"

    #include <algorithm>

    #if defined(__clang__)
        #pragma STDC FP_CONTRACT OFF
    #endif

// INFO: a matrix is column major and each column takes four lanes, so the element (column, row) is at column * 4 + row
// for every size. A lane outside the size is padding.
using MAREA_MatrixLanes = std::array<float, 16>;

using MAREA_IntegerMatrixLanes = std::array<std::int32_t, 16>;

constexpr MAREA_MatrixLanes MAREA_MATRIX_IDENTITY{ 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F };

template <typename Lanes>
Lanes MAREA_columnsOf(std::size_t size, std::initializer_list<typename Lanes::value_type> values)
{
    Lanes result{};

    std::size_t index{ 0 };

    for (const typename Lanes::value_type value : values) {
        result[((index / size) * 4) + (index % size)] = value;

        ++index;
    }

    return result;
}

inline MAREA_MatrixLanes MAREA_matrixColumns(std::size_t size, std::initializer_list<float> values)
{
    return MAREA_columnsOf<MAREA_MatrixLanes>(size, values);
}

inline MAREA_IntegerMatrixLanes MAREA_integerColumns(std::size_t size, std::initializer_list<std::int32_t> values)
{
    return MAREA_columnsOf<MAREA_IntegerMatrixLanes>(size, values);
}

template <typename Matrix, typename Lanes>
Matrix MAREA_matrixOf(const Lanes &lanes)
{
    static_assert(sizeof(Matrix) <= sizeof(Lanes), "a matrix is at most sixteen lanes of four bytes");

    Matrix result;

    std::memcpy(&result, lanes.data(), sizeof(result));

    return result;
}

template <typename Lanes, typename Matrix>
Lanes MAREA_matrixLanesOf(const Matrix &matrix)
{
    static_assert(sizeof(Matrix) <= sizeof(Lanes), "a matrix is at most sixteen lanes of four bytes");

    Lanes result{};

    std::memcpy(result.data(), &matrix, sizeof(matrix));

    return result;
}

inline bool MAREA_sameFloat(float left, float right)
{
    if (std::isnan(left) || std::isnan(right)) {
        return std::isnan(left) && std::isnan(right);
    }

    return MAREA_floatBits(left) == MAREA_floatBits(right);
}

template <typename Matrix>
void MAREA_expectMatrixBits(const Matrix &result, const MAREA_MatrixLanes &expected, const char *label, std::size_t index = 0)
{
    const MAREA_MatrixLanes ACTUAL{ MAREA_matrixLanesOf<MAREA_MatrixLanes>(result) };

    for (std::size_t lane{ 0 }; lane < (sizeof(Matrix) / sizeof(float)); ++lane) {
        ASSERT_TRUE(MAREA_sameFloat(ACTUAL[lane], expected[lane])) << label << " case " << index << " lane " << lane << ": " << ACTUAL[lane] << " against " << expected[lane];
    }
}

template <typename Matrix>
void MAREA_expectMatrixNear(const Matrix &result, const MAREA_MatrixLanes &expected, float tolerance, const char *label)
{
    const MAREA_MatrixLanes ACTUAL{ MAREA_matrixLanesOf<MAREA_MatrixLanes>(result) };

    for (std::size_t lane{ 0 }; lane < (sizeof(Matrix) / sizeof(float)); ++lane) {
        EXPECT_NEAR(ACTUAL[lane], expected[lane], tolerance) << label << " lane " << lane;
    }
}

template <typename Matrix>
void MAREA_expectIntegerMatrix(const Matrix &result, const MAREA_IntegerMatrixLanes &expected, const char *label)
{
    const MAREA_IntegerMatrixLanes ACTUAL{ MAREA_matrixLanesOf<MAREA_IntegerMatrixLanes>(result) };

    for (std::size_t lane{ 0 }; lane < (sizeof(Matrix) / sizeof(std::int32_t)); ++lane) {
        EXPECT_EQ(ACTUAL[lane], expected[lane]) << label << " lane " << lane;
    }
}

inline MAREA_FloatLanes MAREA_referenceMultiplyVector(const MAREA_MatrixLanes &matrix, const MAREA_FloatLanes &vector)
{
    MAREA_FloatLanes result{};

    for (std::size_t row{ 0 }; row < 4; ++row) {
        float sum{ MAREA_product(matrix[row], vector[0]) };

        for (std::size_t column{ 1 }; column < 4; ++column) {
            sum += MAREA_product(matrix[(column * 4) + row], vector[column]);
        }

        result[row] = sum;
    }

    return result;
}

inline MAREA_MatrixLanes MAREA_referenceMultiply(const MAREA_MatrixLanes &left, const MAREA_MatrixLanes &right)
{
    MAREA_MatrixLanes result{};

    for (std::size_t column{ 0 }; column < 4; ++column) {
        const MAREA_FloatLanes OUTPUT{ MAREA_referenceMultiplyVector(left, { right[column * 4], right[(column * 4) + 1], right[(column * 4) + 2], right[(column * 4) + 3] }) };

        std::copy(OUTPUT.begin(), OUTPUT.end(), result.begin() + (column * 4));
    }

    return result;
}

inline MAREA_MatrixLanes MAREA_referenceTranspose(const MAREA_MatrixLanes &matrix)
{
    MAREA_MatrixLanes result{};

    for (std::size_t column{ 0 }; column < 4; ++column) {
        for (std::size_t row{ 0 }; row < 4; ++row) {
            result[(column * 4) + row] = matrix[(row * 4) + column];
        }
    }

    return result;
}

// INFO: a * b - c * d, with each product rounded on its own.
inline float MAREA_cofactor(const MAREA_MatrixLanes &matrix, std::size_t a, std::size_t b, std::size_t c, std::size_t d)
{
    return MAREA_product(matrix[a], matrix[b]) - MAREA_product(matrix[c], matrix[d]);
}

// INFO: a * x - b * y + c * z, from left to right.
inline float MAREA_expansion(float a, float x, float b, float y, float c, float z)
{
    return (MAREA_product(a, x) - MAREA_product(b, y)) + MAREA_product(c, z);
}

inline float MAREA_referenceDeterminant(const MAREA_MatrixLanes &m)
{
    const auto E{ [&m](std::size_t column, std::size_t row) { return m[(column * 4) + row]; } };

    const float S0{ MAREA_cofactor(m, 10, 15, 14, 11) };
    const float S1{ MAREA_cofactor(m, 9, 15, 13, 11) };
    const float S2{ MAREA_cofactor(m, 9, 14, 13, 10) };
    const float S3{ MAREA_cofactor(m, 8, 15, 12, 11) };
    const float S4{ MAREA_cofactor(m, 8, 14, 12, 10) };
    const float S5{ MAREA_cofactor(m, 8, 13, 12, 9) };
    const float C0{ MAREA_expansion(E(1, 1), S0, E(1, 2), S1, E(1, 3), S2) };
    const float C1{ MAREA_expansion(E(1, 0), S0, E(1, 2), S3, E(1, 3), S4) };
    const float C2{ MAREA_expansion(E(1, 0), S1, E(1, 1), S3, E(1, 3), S5) };
    const float C3{ MAREA_expansion(E(1, 0), S2, E(1, 1), S4, E(1, 2), S5) };

    return MAREA_expansion(E(0, 0), C0, E(0, 1), C1, E(0, 2), C2) - MAREA_product(E(0, 3), C3);
}

// INFO: the adjugate over the determinant. A determinant that is equal to zero gives the zero matrix.
inline MAREA_MatrixLanes MAREA_referenceInverse(const MAREA_MatrixLanes &m)
{
    const auto E{ [&m](std::size_t column, std::size_t row) { return m[(column * 4) + row]; } };

    const float C00{ MAREA_cofactor(m, 10, 15, 14, 11) };
    const float C02{ MAREA_cofactor(m, 6, 15, 14, 7) };
    const float C03{ MAREA_cofactor(m, 6, 11, 10, 7) };
    const float C04{ MAREA_cofactor(m, 9, 15, 13, 11) };
    const float C06{ MAREA_cofactor(m, 5, 15, 13, 7) };
    const float C07{ MAREA_cofactor(m, 5, 11, 9, 7) };
    const float C08{ MAREA_cofactor(m, 9, 14, 13, 10) };
    const float C10{ MAREA_cofactor(m, 5, 14, 13, 6) };
    const float C11{ MAREA_cofactor(m, 5, 10, 9, 6) };
    const float C12{ MAREA_cofactor(m, 8, 15, 12, 11) };
    const float C14{ MAREA_cofactor(m, 4, 15, 12, 7) };
    const float C15{ MAREA_cofactor(m, 4, 11, 8, 7) };
    const float C16{ MAREA_cofactor(m, 8, 14, 12, 10) };
    const float C18{ MAREA_cofactor(m, 4, 14, 12, 6) };
    const float C19{ MAREA_cofactor(m, 4, 10, 8, 6) };
    const float C20{ MAREA_cofactor(m, 8, 13, 12, 9) };
    const float C22{ MAREA_cofactor(m, 4, 13, 12, 5) };
    const float C23{ MAREA_cofactor(m, 4, 9, 8, 5) };

    MAREA_MatrixLanes inverse{
        MAREA_expansion(E(1, 1), C00, E(1, 2), C04, E(1, 3), C08),
        -MAREA_expansion(E(0, 1), C00, E(0, 2), C04, E(0, 3), C08),
        MAREA_expansion(E(0, 1), C02, E(0, 2), C06, E(0, 3), C10),
        -MAREA_expansion(E(0, 1), C03, E(0, 2), C07, E(0, 3), C11),
        -MAREA_expansion(E(1, 0), C00, E(1, 2), C12, E(1, 3), C16),
        MAREA_expansion(E(0, 0), C00, E(0, 2), C12, E(0, 3), C16),
        -MAREA_expansion(E(0, 0), C02, E(0, 2), C14, E(0, 3), C18),
        MAREA_expansion(E(0, 0), C03, E(0, 2), C15, E(0, 3), C19),
        MAREA_expansion(E(1, 0), C04, E(1, 1), C12, E(1, 3), C20),
        -MAREA_expansion(E(0, 0), C04, E(0, 1), C12, E(0, 3), C20),
        MAREA_expansion(E(0, 0), C06, E(0, 1), C14, E(0, 3), C22),
        -MAREA_expansion(E(0, 0), C07, E(0, 1), C15, E(0, 3), C23),
        -MAREA_expansion(E(1, 0), C08, E(1, 1), C16, E(1, 2), C20),
        MAREA_expansion(E(0, 0), C08, E(0, 1), C16, E(0, 2), C20),
        -MAREA_expansion(E(0, 0), C10, E(0, 1), C18, E(0, 2), C22),
        MAREA_expansion(E(0, 0), C11, E(0, 1), C19, E(0, 2), C23)
    };

    const float DETERMINANT{ ((MAREA_product(E(0, 0), inverse[0]) + MAREA_product(E(0, 1), inverse[4])) + MAREA_product(E(0, 2), inverse[8])) + MAREA_product(E(0, 3), inverse[12]) };

    if (DETERMINANT == 0.0F) {
        return MAREA_MatrixLanes{};
    }

    const float RECIPROCAL{ 1.0F / DETERMINANT };

    for (float &lane : inverse) {
        lane = lane * RECIPROCAL;
    }

    return inverse;
}

// INFO: identity, zero, singular, NaN and infinity carrying, negative zero, 64 integer valued and 2048 random matrices,
// all from fixed seeds.
inline const std::vector<MAREA_MatrixLanes> &MAREA_matrixCases()
{
    static const std::vector<MAREA_MatrixLanes> CASES{ [] {
        std::vector<MAREA_MatrixLanes> cases{ MAREA_MATRIX_IDENTITY, MAREA_MatrixLanes{} };

        MAREA_MatrixLanes singular{ MAREA_MATRIX_IDENTITY };

        singular[4] = 1.0F;
        singular[5] = 0.0F;
        cases.push_back(singular);

        MAREA_MatrixLanes repeated{};

        for (std::size_t lane{ 0 }; lane < 16; ++lane) {
            repeated[lane] = 1.0F + static_cast<float>(lane % 4);
        }

        cases.push_back(repeated);

        MAREA_MatrixLanes zeroColumn{ MAREA_MATRIX_IDENTITY };

        zeroColumn[10] = 0.0F;
        cases.push_back(zeroColumn);

        MAREA_MatrixLanes withNan{ MAREA_MATRIX_IDENTITY };

        withNan[5] = MAREA_MATH_NAN;
        cases.push_back(withNan);

        MAREA_MatrixLanes withNanCorner{ MAREA_MATRIX_IDENTITY };

        withNanCorner[15] = MAREA_MATH_NAN;
        cases.push_back(withNanCorner);

        MAREA_MatrixLanes withInfinity{ MAREA_MATRIX_IDENTITY };

        withInfinity[2] = MAREA_MATH_INFINITY;
        withInfinity[9] = -MAREA_MATH_INFINITY;
        cases.push_back(withInfinity);

        MAREA_MatrixLanes negativeZero{};

        negativeZero.fill(-0.0F);
        cases.push_back(negativeZero);

        MAREA_MathRandom integers{ 0x13579BDFU };

        for (std::size_t index{ 0 }; index < 64; ++index) {
            MAREA_MatrixLanes matrix{};

            for (float &lane : matrix) {
                lane = std::round(integers.MAREA_range(-3.0F, 3.0F));
            }

            cases.push_back(matrix);
        }

        MAREA_MathRandom random{ 0x0BADCAFEU };

        for (std::size_t index{ 0 }; index < 2048; ++index) {
            MAREA_MatrixLanes matrix{};

            for (float &lane : matrix) {
                lane = random.MAREA_range(-4.0F, 4.0F);
            }

            cases.push_back(matrix);
        }

        return cases;
    }() };

    return CASES;
}

inline const std::vector<MAREA_FloatLanes> &MAREA_matrixVectorCases()
{
    static const std::vector<MAREA_FloatLanes> CASES{ [] {
        std::vector<MAREA_FloatLanes> cases{
            { 0.0F, 0.0F, 0.0F, 0.0F },
            { 1.0F, 2.0F, 3.0F, 1.0F },
            { -0.0F, -0.0F, -0.0F, -0.0F },
            { MAREA_MATH_NAN, 1.0F, 1.0F, 1.0F },
            { MAREA_MATH_INFINITY, 0.0F, 1.0F, -1.0F },
        };

        MAREA_MathRandom random{ 0x2468ACE0U };

        for (std::size_t index{ 0 }; index < 2048; ++index) {
            MAREA_FloatLanes vector{};

            for (float &lane : vector) {
                lane = random.MAREA_range(-4.0F, 4.0F);
            }

            cases.push_back(vector);
        }

        return cases;
    }() };

    return CASES;
}

#endif
