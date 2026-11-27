// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.
// Reference: library/coretests/tests/ascii.rs assert_all!/assert_none! groups, adapted to the
// fourteen c.character functions of 22_C_LIBRARIES.md section 5. library/coretests/tests/char.rs's
// own is_lowercase/is_uppercase/is_whitespace/is_control cases are skipped (logged): they exercise
// full Unicode case and whitespace tables (letters such as 'o with diaeresis', 'sharp s'), while
// this library is ASCII only and locale independent per the spec, matching only ascii.rs's
// is_ascii_* group.

#include "tests/dependencies.hpp"

#include "marea/base/MAREACCharacter.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace
{
    struct MAREA_CharacterCallFixture
    {
        std::array<std::uint64_t, 33> slots{};
        std::array<MAREA_Null, 33> arguments{};
        MAREA_HostContext context{};
        std::uint64_t result{ 0 };

        MAREA_CharacterCallFixture()
        {
            for (std::size_t index{ 0 }; index < slots.size(); ++index) {
                arguments[index] = &slots[index];
            }
        }

        std::uint64_t MAREA_call(MAREA_HostFunction function, MAREA_Character8 value)
        {
            slots[0] = static_cast<std::uint64_t>(static_cast<unsigned char>(value));

            result = 0;

            function(&context, arguments.data(), &result);

            return result;
        }
    };

    struct MAREA_CCharacterFixture
    {
        MAREA_Arena *arena{ nullptr };
        MAREA_TypeRegistry *registry{ nullptr };

        MAREA_CCharacterFixture()
        {
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
            MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT, &registry));
            MAREA_IGNORE(MAREA_CCharacterRegister(registry));
        }

        MAREA_CCharacterFixture(const MAREA_CCharacterFixture &) = delete;
        MAREA_CCharacterFixture &operator=(const MAREA_CCharacterFixture &) = delete;

        ~MAREA_CCharacterFixture()
        {
            MAREA_ArenaDestroy(arena);
        }

        MAREA_HostFunction MAREA_find(const char *name)
        {
            return MAREA_TypeRegistryFindFunction(registry, "c.character", MAREA_INTEGER_SIZE(12), name, static_cast<MAREA_IntegerSize>(std::strlen(name)));
        }
    };

    // Every byte 0..255, split into the set a predicate must answer true for and the rest, which
    // must answer false; mirrors ascii.rs's assert_all!/assert_none! over the printable ranges.
    void MAREA_expect(MAREA_CharacterCallFixture &call, MAREA_HostFunction function, const char *trueFor, const char *label)
    {
        std::array<bool, 256> expected{};

        for (const char *cursor{ trueFor }; *cursor != '\0'; ++cursor) {
            expected[static_cast<unsigned char>(*cursor)] = true;
        }

        for (int value{ 0 }; value < 256; ++value) {
            EXPECT_EQ(call.MAREA_call(function, static_cast<MAREA_Character8>(value)), expected[static_cast<std::size_t>(value)] ? 1U : 0U) << label << " at byte " << value;
        }
    }
} // namespace

TEST(MAREACCharacter, IsAlphanumericIsTrueForALetterOrADecimalDigit)
{
    GTEST_COUT << "Validating is_alphanumeric is true for a letter or a decimal digit, ASCII only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_alphanumeric"), "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", "is_alphanumeric");
}

TEST(MAREACCharacter, IsAlphabeticIsTrueForALetter)
{
    GTEST_COUT << "Validating is_alphabetic is true for A to Z and a to z only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_alphabetic"), "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", "is_alphabetic");
}

TEST(MAREACCharacter, IsBlankIsTrueForSpaceAndHorizontalTab)
{
    GTEST_COUT << "Validating is_blank is true for space and horizontal tab only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_blank"), " \t", "is_blank");
}

TEST(MAREACCharacter, IsControlIsTrueForZeroToThirtyOneAndOneTwentySeven)
{
    GTEST_COUT << "Validating is_control is true for the 0..31 and 127 control bytes." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    std::string trueFor;

    for (int value{ 0 }; value <= 31; ++value) {
        trueFor.push_back(static_cast<char>(value));
    }

    trueFor.push_back(static_cast<char>(127));

    MAREA_expect(call, fixture.MAREA_find("is_control"), trueFor.c_str(), "is_control");
}

TEST(MAREACCharacter, IsDecimalIsTrueForZeroToNine)
{
    GTEST_COUT << "Validating is_decimal is true for 0 to 9 only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_decimal"), "0123456789", "is_decimal");
}

TEST(MAREACCharacter, IsGraphicalIsTruePrintableAndNotASpace)
{
    GTEST_COUT << "Validating is_graphical is true for 33 to 126 only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    std::string trueFor;

    for (int value{ 33 }; value <= 126; ++value) {
        trueFor.push_back(static_cast<char>(value));
    }

    MAREA_expect(call, fixture.MAREA_find("is_graphical"), trueFor.c_str(), "is_graphical");
}

TEST(MAREACCharacter, IsLowercaseIsTrueForATooZ)
{
    GTEST_COUT << "Validating is_lowercase is true for a to z only, ASCII only (char.rs's own case, which also "
                  "accepts a non-ASCII lower letter, is skipped: it needs a full Unicode case table)."
               << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_lowercase"), "abcdefghijklmnopqrstuvwxyz", "is_lowercase");
}

TEST(MAREACCharacter, IsPrintableIsTrueForThirtyTwoToOneTwentySix)
{
    GTEST_COUT << "Validating is_printable is true for 32 to 126 only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    std::string trueFor;

    for (int value{ 32 }; value <= 126; ++value) {
        trueFor.push_back(static_cast<char>(value));
    }

    MAREA_expect(call, fixture.MAREA_find("is_printable"), trueFor.c_str(), "is_printable");
}

TEST(MAREACCharacter, IsPunctuationIsTrueForGraphicalAndNotAlphanumeric)
{
    GTEST_COUT << "Validating is_punctuation is true for the graphical bytes that are not a letter or a digit." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_punctuation"), "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", "is_punctuation");
}

TEST(MAREACCharacter, IsWhiteSpaceIsTrueForTheFiveAsciiWhitespaceBytes)
{
    GTEST_COUT << "Validating is_white_space is true for space, tab, line feed, vertical tab, form feed and "
                  "carriage return only (char.rs's own test_is_whitespace, which also accepts the Unicode "
                  "space U+2007, is skipped: it needs a full Unicode whitespace table)."
               << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_white_space"), " \t\n\x0b\x0c\r", "is_white_space");
}

TEST(MAREACCharacter, IsUppercaseIsTrueForATooZ)
{
    GTEST_COUT << "Validating is_uppercase is true for A to Z only, ASCII only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_uppercase"), "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "is_uppercase");
}

TEST(MAREACCharacter, IsHexadecimalIsTrueForDecimalDigitsAndAToFBothCases)
{
    GTEST_COUT << "Validating is_hexadecimal is true for 0-9, a-f and A-F only." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_expect(call, fixture.MAREA_find("is_hexadecimal"), "0123456789abcdefABCDEF", "is_hexadecimal");
}

TEST(MAREACCharacter, ToLowercaseChangesALetterAndGivesEverythingElseBack)
{
    GTEST_COUT << "Validating to_lowercase lowers A to Z and gives every other byte back unchanged, including "
                  "a byte above 127."
               << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_HostFunction toLowercase{ fixture.MAREA_find("to_lowercase") };

    EXPECT_EQ(call.MAREA_call(toLowercase, 'A'), static_cast<std::uint64_t>('a')) << "A becomes a.";

    EXPECT_EQ(call.MAREA_call(toLowercase, 'Z'), static_cast<std::uint64_t>('z')) << "Z becomes z.";

    EXPECT_EQ(call.MAREA_call(toLowercase, 'a'), static_cast<std::uint64_t>('a')) << "a stays a.";

    EXPECT_EQ(call.MAREA_call(toLowercase, '0'), static_cast<std::uint64_t>('0')) << "A digit is unchanged.";

    EXPECT_EQ(call.MAREA_call(toLowercase, static_cast<MAREA_Character8>(200)), 200U) << "A byte above 127 gives itself back.";
}

TEST(MAREACCharacter, ToUppercaseChangesALetterAndGivesEverythingElseBack)
{
    GTEST_COUT << "Validating to_uppercase raises a to z and gives every other byte back unchanged, including "
                  "a byte above 127."
               << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    MAREA_HostFunction toUppercase{ fixture.MAREA_find("to_uppercase") };

    EXPECT_EQ(call.MAREA_call(toUppercase, 'a'), static_cast<std::uint64_t>('A')) << "a becomes A.";

    EXPECT_EQ(call.MAREA_call(toUppercase, 'z'), static_cast<std::uint64_t>('Z')) << "z becomes Z.";

    EXPECT_EQ(call.MAREA_call(toUppercase, 'A'), static_cast<std::uint64_t>('A')) << "A stays A.";

    EXPECT_EQ(call.MAREA_call(toUppercase, '9'), static_cast<std::uint64_t>('9')) << "A digit is unchanged.";

    EXPECT_EQ(call.MAREA_call(toUppercase, static_cast<MAREA_Character8>(200)), 200U) << "A byte above 127 gives itself back.";
}

TEST(MAREACCharacter, AByteAboveOneTwentySevenAnswersFalseToEveryClassification)
{
    GTEST_COUT << "Validating every is_* function answers false for a character above 127." << std::endl;

    MAREA_CCharacterFixture fixture;

    MAREA_CharacterCallFixture call;

    const char *names[]{
        "is_alphanumeric",
        "is_alphabetic",
        "is_blank",
        "is_control",
        "is_decimal",
        "is_graphical",
        "is_lowercase",
        "is_printable",
        "is_punctuation",
        "is_white_space",
        "is_uppercase",
        "is_hexadecimal",
    };

    for (const char *name : names) {
        EXPECT_EQ(call.MAREA_call(fixture.MAREA_find(name), static_cast<MAREA_Character8>(200)), 0U) << name << " must be false above 127.";
    }
}

TEST(MAREACCharacter, ARegistryWithTheCCharacterBitOffRegistersNoFunctionOfThisLibrary)
{
    GTEST_COUT << "Validating MAREA_TypeRegistryRegisterInternalFunction's no-op contract: a disabled library "
                  "bit leaves every function of c.character unregistered."
               << std::endl;

    MAREA_Arena *arena{ nullptr };

    MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));

    MAREA_TypeRegistry *registry{ nullptr };

    MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, MAREA_LIBRARY_MASK_DEFAULT & ~MAREA_LIBRARY_FLAG_C_CHARACTER, &registry));

    MAREA_IGNORE(MAREA_CCharacterRegister(registry));

    EXPECT_EQ(MAREA_TypeRegistryFindFunction(registry, "c.character", MAREA_INTEGER_SIZE(12), "is_alphabetic", MAREA_INTEGER_SIZE(13)), nullptr) << "The library bit was off, so nothing was registered.";

    MAREA_ArenaDestroy(arena);
}
