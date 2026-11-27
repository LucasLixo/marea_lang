// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAContainerHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    Codes MAREA_codesWithCMemory(const std::string &body)
    {
        const MAREA_ShippedRegistry SHIPPED;

        MAREA_IGNORE(MAREA_registerCMemoryLibrary(SHIPPED.registry));

        MAREA_SemaFixture fixture(MAREA_containerResolver());

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, SHIPPED.registry));
        MAREA_IGNORE(fixture.MAREA_sema("namespace main; require c.memory; " + body));

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaCMemoryLibrary, CMemoryRequireIsUnresolvedWhenTheLibraryIsOff)
{
    GTEST_COUT << "Validating a require of c.memory resolves from the registry only when the library is on." << std::endl;

    const MAREA_ShippedRegistry ON;
    const MAREA_ShippedRegistry OFF{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_C_MEMORY)) };

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(ON.registry) && MAREA_registerCMemoryLibrary(OFF.registry)) << "Both registrations must succeed.";

    EXPECT_EQ(MAREA_requireCodes("c.memory", "Pointer", ON.registry), Codes{}) << "The library is on.";

    EXPECT_EQ(MAREA_requireCodes("c.memory", "Pointer", OFF.registry), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE }) << "The library is off.";
}

TEST(MAREASemaCMemoryLibrary, APointerOfTheLibraryIsLegalOnlyInsideUnsafe)
{
    GTEST_COUT << "Validating the shipped pointer type is rejected in a local, a parameter, a result and a field outside unsafe." << std::endl;

    const Codes OUTSIDE{ MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE };

    EXPECT_EQ(MAREA_codesWithCMemory("function f(p: c.memory.Pointer<uinteger8>) end"), OUTSIDE) << "A parameter.";

    EXPECT_EQ(MAREA_codesWithCMemory("structure Holder p: c.memory.Pointer<uinteger8>; end"), OUTSIDE) << "A field.";

    EXPECT_EQ(MAREA_codesWithCMemory("function f() unsafe local a: uinteger64 = 0; local p: c.memory.Pointer<uinteger8> = convert<c.memory.Pointer<uinteger8>>(a); local q: c.memory.Pointer<uinteger32> = convert<c.memory.Pointer<uinteger32>>(p); local back: uinteger64 = convert<uinteger64>(q); end end"), Codes{}) << "Inside unsafe, with the unchecked conversion to a pointer of another type and back.";

    EXPECT_EQ(MAREA_codesWithCMemory("function f() local a: uinteger64 = 0; local b: uinteger64 = convert<c.memory.Pointer<uinteger8>>(a); end").empty(), false) << "The unchecked conversion is rejected outside unsafe.";
}
