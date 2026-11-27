// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAContainerHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    Codes MAREA_codesWithMemory(const std::string &body)
    {
        const MAREA_ShippedRegistry SHIPPED;

        MAREA_IGNORE(MAREA_registerCMemoryLibrary(SHIPPED.registry));
        MAREA_IGNORE(MAREA_registerMemoryLibrary(SHIPPED.registry));

        MAREA_SemaFixture fixture(MAREA_containerResolver());

        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, SHIPPED.registry));
        MAREA_IGNORE(fixture.MAREA_sema("namespace main; require marea.memory; structure Point x: sinteger32; end " + body));

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaMemoryLibrary, MemoryRequireIsUnresolvedWhenTheLibraryIsOff)
{
    GTEST_COUT << "Validating a require of marea.memory resolves from the registry only when the library is on." << std::endl;

    const MAREA_ShippedRegistry ON;
    const MAREA_ShippedRegistry OFF{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_MEMORY)) };

    ASSERT_TRUE(MAREA_registerCMemoryLibrary(ON.registry) && MAREA_registerMemoryLibrary(ON.registry)) << "Both registrations must succeed.";

    EXPECT_EQ(MAREA_requireCodes("marea.memory", "Owned", ON.registry), Codes{}) << "The library is on.";

    EXPECT_EQ(MAREA_requireCodes("marea.memory", "Owned", OFF.registry), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_REQUIRE }) << "The library is off.";
}

TEST(MAREASemaMemoryLibrary, AnOwnerOfTheLibraryIsMovedAndPeeled)
{
    GTEST_COUT << "Validating the shipped owner needs move or copy, and its member is read through it." << std::endl;

    EXPECT_EQ(MAREA_codesWithMemory("function f(p: marea.memory.Owned<Point>) local x: sinteger32 = p.x; local q: marea.memory.Owned<Point> = move p; end"), Codes{}) << "A member read and a move.";

    EXPECT_EQ(MAREA_codesWithMemory("function f(p: marea.memory.Owned<Point>) local q: marea.memory.Owned<Point> = p; end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE }) << "A name alone does not say copy or move.";
}

TEST(MAREASemaMemoryLibrary, AnArenaOfTheLibraryHasOneOwnerAndNoTypeArgument)
{
    GTEST_COUT << "Validating an arena is moved and takes no type argument." << std::endl;

    EXPECT_EQ(MAREA_codesWithMemory("function f(#[semantics = reference_mutable] a: marea.memory.Arena) local b: marea.memory.Arena = a; end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_NEEDS_COPY_OR_MOVE }) << "An arena is never copied by a name.";

    EXPECT_EQ(MAREA_codesWithMemory("function f(a: marea.memory.Arena<Point>) end"), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_GENERIC_ARGUMENT_MISMATCH }) << "No type argument.";
}
