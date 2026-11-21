#include "tests/dependencies.hpp"

#include <cstddef>

TEST(MAREASourcePosition, FieldsKeepTheirOrder)
{
    GTEST_COUT << "Validating MAREA_SourcePosition keeps its fields in descending size order with no gaps." << std::endl;

    constexpr std::size_t FILE_OFFSET{ offsetof(MAREA_SourcePosition, file) };
    constexpr std::size_t OFFSET_OFFSET{ offsetof(MAREA_SourcePosition, offset) };
    constexpr std::size_t LINE_OFFSET{ offsetof(MAREA_SourcePosition, line) };
    constexpr std::size_t COLUMN_OFFSET{ offsetof(MAREA_SourcePosition, column) };
    constexpr std::size_t LENGTH_OFFSET{ offsetof(MAREA_SourcePosition, length) };

    EXPECT_LT(FILE_OFFSET, OFFSET_OFFSET) << "file must come before offset.";

    EXPECT_LT(OFFSET_OFFSET, LINE_OFFSET) << "offset must come before line.";

    EXPECT_LT(LINE_OFFSET, COLUMN_OFFSET) << "line must come before column.";

    EXPECT_LT(COLUMN_OFFSET, LENGTH_OFFSET) << "column must come before length.";

    constexpr std::size_t FIELD_SIZE_SUM{ sizeof(MAREA_SourcePosition::file) + sizeof(MAREA_SourcePosition::offset) + sizeof(MAREA_SourcePosition::line) + sizeof(MAREA_SourcePosition::column) + sizeof(MAREA_SourcePosition::length) };

    EXPECT_GE(sizeof(MAREA_SourcePosition), FIELD_SIZE_SUM) << "sizeof must be at least the sum of the field sizes.";
}
