// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIncludedFiles.hpp"
#include "tests/MAREAIrHelpers.hpp"
#include "tests/MAREAParityHelpers.hpp"

#include <string>

TEST(MAREAIrIncludedBytes, TheBytesAndThePathAreKeptInTheIr)
{
    GTEST_COUT << "Validating include_bytes lowers to INCLUDED_BYTES, with the bytes as hexadecimal text and the path as its child." << std::endl;

    EXPECT_STREQ(MAREA_IrKindGetName(MAREA_IR_KIND_INCLUDED_BYTES), "INCLUDED_BYTES") << "The name must be stable.";

    MAREA_IncludedFiles files;

    files.MAREA_add("data.bin", std::string("\x0a\0\xff", 3));

    MAREA_IrFixture fixture(MAREA_parityLibraries());

    MAREA_IGNORE(MAREA_SemaContextSetIncludeResolver(fixture.sema.context, &files.resolver));

    ASSERT_EQ(fixture.MAREA_lower("namespace main; require marea.array; constant DATA: marea.array.Array<uinteger8, 3> = marea.array.include_bytes!(\"data.bin\");"), MAREA_IR_EXCEPT_SUCCESS) << "The unit must lower.";

    const MAREA_IrNode *value{ MAREA_irChild(MAREA_irChild(fixture.module, 0), 0) };

    ASSERT_EQ(MAREA_IrNodeGetKind(value), MAREA_IR_KIND_INCLUDED_BYTES) << "The value is the included file.";

    EXPECT_EQ(MAREA_irText(value), "0a00ff") << "Two digits for each byte.";
    EXPECT_EQ(std::string(MAREA_IrNodeGetResolvedType(value), MAREA_IrNodeGetResolvedTypeLength(value)), "marea.array.Array<uinteger8,3>") << "The array that holds the bytes.";
    EXPECT_EQ(MAREA_irText(MAREA_irChild(value, 0)), files.entries["data.bin"].path) << "The path that the resolver gave.";
}
