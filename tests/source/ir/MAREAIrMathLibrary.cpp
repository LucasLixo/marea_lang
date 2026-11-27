// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREACatalogueHelpers.hpp"
#include "tests/MAREAIrHelpers.hpp"

#include <vector>

TEST(MAREAIrMathLibrary, ShippedSourceLowersWithItsCallsIntoItsOwnNamespace)
{
    GTEST_COUT << "Validating the full path of the namespace of a file is no value in the ir." << std::endl;

    MAREA_IrFixture fixture;

    ASSERT_EQ(MAREA_SemaContextSetShippedLibrarySource(fixture.sema.context, MAREA_BOOLEAN_TRUE), MAREA_SEMA_EXCEPT_SUCCESS) << "The input must be set.";

    ASSERT_EQ(fixture.MAREA_lower(MAREA_readFile(MAREA_mathSourcePath()), "marea/math.marea"), MAREA_IR_EXCEPT_SUCCESS) << "The source must lower.";

    EXPECT_EQ(fixture.MAREA_irCodes(), std::vector<MAREA_Uinteger32>{}) << "The ir must raise nothing.";
}
