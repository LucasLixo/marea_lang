// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    const std::string MAREA_SHAPES{ "namespace main; enum Shape : uinteger8 Circle(radius: float64 = 1.0); Rect(width: float64, height: float64 = 2.0); Empty; end function area(shape: Shape): float64 switch shape case Shape.Circle then return shape.radius * shape.radius * 3.0; case Shape.Rect then return shape.width * shape.height; case Shape.Empty then return 0.0; end end " };
} // namespace

TEST(MAREACgenEnum, FieldlessEnumsCompareSwitchAndCrossFunctions)
{
    GTEST_COUT << "Validating an enum with no data: variants, comparisons, switch, parameters, results and methods." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; enum Color Red; Green; Blue; end implementation Color function is_red(self: Color): boolean return self == Color.Red; end end function name(c: Color): sinteger32 switch c case Color.Red then return 1; case Color.Green, Color.Blue then return 2; end end function next(c: Color): Color switch c case Color.Red then return Color.Green; case Color.Green then return Color.Blue; case Color.Blue then return Color.Red; end end function f(): sinteger32 local c: Color = Color.Green; local d: Color = next(c); local r: sinteger32 = name(d) * 100; if d != Color.Red and not d.is_red() then r += 10; end if next(d).is_red() then r += 1; end return r; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 211) << "d is Blue and next(d) is Red.";
}

TEST(MAREACgenEnum, TheTagTypeOfSemaIsTheStorageOfTheEnum)
{
    GTEST_COUT << "Validating an enum with a written tag type keeps its size inside a structure, which C asserts." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; enum Level : uinteger8 Low; High; end enum Big : sinteger64 First; Second; end structure S level: Level; big: Big; other: Level; end function f(): sinteger32 local s: S = S{ level = Level.High, big = Big.Second, other = Level.Low }; local r: sinteger32 = 0; if s.level == Level.High then r += 1; end if s.big == Big.Second then r += 10; end if s.other != Level.Low then r += 100; end return r; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 11) << "The first two tests hold.";

    EXPECT_NE(RESULT.cText.find("_Static_assert(sizeof(marea_s_S) == 24"), std::string::npos) << "The size of sema must be asserted.";
}

TEST(MAREACgenEnum, VariantsWithDataAreBuiltReadAndSwitched)
{
    GTEST_COUT << "Validating the constructor of a variant fills the defaults and a switch reads the data of the variant." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_SHAPES + "function make(): Shape return Shape.Circle(2.0); end function f(): float64 local a: Shape = Shape.Circle(3.0); local b: Shape = Shape.Rect(4.0); local c: Shape = Shape.Empty; local d: Shape = Shape.Circle; a = b; return (area(a) * 1000.0) + (area(make()) * 100.0) + (area(c) * 10.0) + area(d); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityDouble(RESULT.MAREA_outcome("f()")), 9203.0) << "8, 12, 0 and 3.";
}

TEST(MAREACgenEnum, ComparisonsReadTheVariantAndNotItsData)
{
    GTEST_COUT << "Validating == of two enum values compares the variants, which is what a switch needs." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_SHAPES + "function f(): sinteger32 local a: Shape = Shape.Circle(1.0); local b: Shape = Shape.Circle(2.0); local c: Shape = Shape.Rect(1.0); local r: sinteger32 = 0; if a == b then r += 1; end if a != c then r += 10; end if c == Shape.Rect then r += 100; end return r; end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.MAREA_outcome("f()")), 111) << "The three tests hold.";
}

TEST(MAREACgenEnum, ReadingAFieldThatTheVariantLacksTraps)
{
    GTEST_COUT << "Validating a field of another variant is the unreachable trap, never a read of other bytes." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_SHAPES + "function f(kind: sinteger32): float64 local s: Shape = Shape.Empty; if kind == 1 then s = Shape.Circle(5.0); end return s.radius; end", { MAREA_parityCall("f", "float64", { MAREA_parityInt(1) }), MAREA_parityCall("f", "float64", { MAREA_parityInt(0) }) }) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityDouble(RESULT.generated[0]), 5.0) << "The circle has a radius.";

    EXPECT_EQ(RESULT.generated[1].trap, 5) << "The empty variant has no radius.";
}

TEST(MAREACgenEnum, TheDataOfAVariantIsReadThroughAMethod)
{
    GTEST_COUT << "Validating variants whose data have the same names, a constant of the type and a call on a variant." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated("namespace main; enum Planet Mercury(mass: float64 = 3.0, radius: float64 = 2.0); Earth(mass: float64 = 6.0, radius: float64 = 4.0); end implementation Planet constant G: float64 = 10.0; function gravity(self: Planet): float64 return Planet.G * self.mass / self.radius; end end function f(): float64 local p: Planet = Planet.Earth; return (p.gravity() * 100.0) + Planet.Mercury.gravity(); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityDouble(RESULT.MAREA_outcome("f()")), 1515.0) << "15 * 100 + 15.";
}

TEST(MAREACgenEnum, AnEnumWithDataIsAFieldOfAStructure)
{
    GTEST_COUT << "Validating a structure holds an enum with data at the offset of sema." << std::endl;

    const MAREA_ParityResult RESULT{ MAREA_parityGenerated(MAREA_SHAPES + "structure Item shape: Shape; count: sinteger32; end function f(): float64 local item: Item = Item{ shape = Shape.Rect(3.0, 5.0), count = 2 }; local other: Item = copy item; item.shape = Shape.Empty; return (area(other.shape) * 10.0) + area(item.shape); end", {}) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityDouble(RESULT.MAREA_outcome("f()")), 150.0) << "The copy keeps the rectangle.";
}
