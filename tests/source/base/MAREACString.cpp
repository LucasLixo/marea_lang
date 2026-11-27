// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.
// Reference: library/alloctests/tests/string.rs (test_push_str, test_add_assign, test_push,
// test_str_clear) and library/alloctests/tests/str.rs (utf8_chars), adapted to the c.string
// functions of 22_C_LIBRARIES.md section 6 this apply implements: length, append, clear, compare.
// string.rs's test_push_bytes (raw byte-vector access), test_pop/test_split_off_* (character
// iteration and splitting) and test_str_truncate* (truncate, out of this apply's scope) are
// skipped (logged): they need a mechanism (byte-vector handle, character iteration, truncate)
// this apply does not implement. str.rs carries no dedicated ordering/compare case for a string;
// compare's cases below come from the spec (22_C_LIBRARIES.md D4) instead.

#include "tests/dependencies.hpp"

#include "marea/base/MAREACString.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace
{
    // A host allocator that always fails, to prove append's failed-grow path leaves the target
    // string unchanged and raises the out-of-memory trap. string.rs's reference corpus has no
    // equivalent (Rust's global allocator is not swappable per call in the adapted corpus); this
    // case comes from the spec instead (22_C_LIBRARIES.md section 6, append).
    MAREA_Null MAREA_CALL MAREA_failingAllocate(MAREA_IntegerSize size, MAREA_IntegerSize alignment)
    {
        (void)size;

        (void)alignment;

        return nullptr;
    }

    void MAREA_CALL MAREA_failingFree(MAREA_Null address)
    {
        (void)address;
    }

    struct MAREA_CStringFixture
    {
        MAREA_Arena *arena{ nullptr };
        MAREA_TypeRegistry *registry{ nullptr };

        explicit MAREA_CStringFixture(MAREA_LibraryMask mask = MAREA_LIBRARY_MASK_DEFAULT)
        {
            MAREA_IGNORE(MAREA_ArenaCreate(MAREA_INTEGER_SIZE(0), &arena));
            MAREA_IGNORE(MAREA_TypeRegistryCreate(arena, mask, &registry));
            MAREA_IGNORE(MAREA_CStringRegister(registry));
        }

        MAREA_CStringFixture(const MAREA_CStringFixture &) = delete;
        MAREA_CStringFixture &operator=(const MAREA_CStringFixture &) = delete;

        ~MAREA_CStringFixture()
        {
            MAREA_ArenaDestroy(arena);
        }

        MAREA_HostFunction MAREA_find(const char *name)
        {
            return MAREA_TypeRegistryFindFunction(registry, "c.string", MAREA_INTEGER_SIZE(8), name, static_cast<MAREA_IntegerSize>(std::strlen(name)));
        }
    };

    MAREA_String MAREA_stringOf(MAREA_HostContext &context, const char *text)
    {
        MAREA_String value{};

        MAREA_IntegerSize length{ static_cast<MAREA_IntegerSize>(std::strlen(text)) };

        if (length > 0) {
            MAREA_IGNORE(MAREA_HostContextAllocate(&context, length, MAREA_INTEGER_SIZE(1), reinterpret_cast<MAREA_Null *>(&value.data)));

            std::memcpy(value.data, text, static_cast<std::size_t>(length));
        }

        value.lengthInBytes = length;

        value.capacityInBytes = length;

        return value;
    }

    std::uint64_t MAREA_length(MAREA_HostContext &context, MAREA_HostFunction function, MAREA_String *value)
    {
        std::array<MAREA_Null, 33> arguments{};

        arguments[0] = value;

        std::uint64_t result{ 0 };

        function(&context, arguments.data(), &result);

        return result;
    }

    void MAREA_append(MAREA_HostContext &context, MAREA_HostFunction function, MAREA_String *target, MAREA_String *suffix)
    {
        std::array<MAREA_Null, 33> arguments{};

        arguments[0] = target;

        arguments[1] = suffix;

        function(&context, arguments.data(), nullptr);
    }

    void MAREA_clear(MAREA_HostContext &context, MAREA_HostFunction function, MAREA_String *target)
    {
        std::array<MAREA_Null, 33> arguments{};

        arguments[0] = target;

        function(&context, arguments.data(), nullptr);
    }

    std::int32_t MAREA_compare(MAREA_HostContext &context, MAREA_HostFunction function, MAREA_String *left, MAREA_String *right)
    {
        std::array<MAREA_Null, 33> arguments{};

        arguments[0] = left;

        arguments[1] = right;

        // The wrapper always writes the full 8-byte register form (MAREA_HostDetailStore), sign
        // extended for a signed scalar, so the result slot must be 8 bytes wide even though
        // sinteger32 is a 4-byte type.
        std::uint64_t bits{ 0 };

        function(&context, arguments.data(), &bits);

        return static_cast<std::int32_t>(static_cast<std::int64_t>(bits));
    }

    std::string MAREA_toText(const MAREA_String &value)
    {
        return std::string(value.data, static_cast<std::size_t>(value.lengthInBytes));
    }
} // namespace

TEST(MAREACString, ZeroInitializedStringIsTheValidEmptyString)
{
    GTEST_COUT << "Validating a zero-initialized MAREA_String is the empty string, length zero." << std::endl;

    MAREA_String value{};

    EXPECT_EQ(value.lengthInBytes, 0) << "A zero-initialized string has no bytes.";

    EXPECT_EQ(value.capacityInBytes, 0) << "A zero-initialized string has no storage.";
}

TEST(MAREACString, LengthCountsUtf8ScalarValuesNotRawBytes)
{
    GTEST_COUT << "Validating length counts scalar values by decoding lead bytes over a mix of one-, two-, "
                  "three- and four-byte UTF-8 sequences (library/alloctests/tests/str.rs's utf8_chars: "
                  "s.len() == 10 bytes, s.chars().count() == 4 scalar values for 'e', the two-byte "
                  "e-acute, the three-byte euro sign and the four-byte U+10000)."
               << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    // "e" (1 byte) + U+00E9 (2 bytes) + U+20AC (3 bytes) + U+10000 (4 bytes) = 10 bytes, 4 scalar values.
    const char bytes[]{ 'e', '\xc3', '\xa9', '\xe2', '\x82', '\xac', '\xf0', '\x90', '\x80', '\x80' };

    MAREA_String value{};

    MAREA_IGNORE(MAREA_HostContextAllocate(&context, MAREA_INTEGER_SIZE(10), MAREA_INTEGER_SIZE(1), reinterpret_cast<MAREA_Null *>(&value.data)));

    std::memcpy(value.data, bytes, sizeof(bytes));

    value.lengthInBytes = MAREA_INTEGER_SIZE(10);

    value.capacityInBytes = MAREA_INTEGER_SIZE(10);

    MAREA_CStringFixture fixture;

    EXPECT_EQ(MAREA_length(context, fixture.MAREA_find("length"), &value), 4U) << "Four scalar values, not ten bytes.";

    MAREA_HostContextFree(&context, value.data);
}

TEST(MAREACString, LengthOfTheEmptyStringIsZero)
{
    GTEST_COUT << "Validating length of the zero-initialized empty string is zero." << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    MAREA_String value{};

    MAREA_CStringFixture fixture;

    EXPECT_EQ(MAREA_length(context, fixture.MAREA_find("length"), &value), 0U) << "The empty string has no scalar values.";
}

TEST(MAREACString, AppendGrowsAnEmptyStringAndConcatenates)
{
    GTEST_COUT << "Validating append (string.rs's test_push_str/test_add_assign: push_str(''), push_str('abc') "
                  "then a longer multi-byte suffix) grows an empty string's storage and concatenates its bytes."
               << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    MAREA_String target{};

    MAREA_CStringFixture fixture;

    MAREA_HostFunction append{ fixture.MAREA_find("append") };

    MAREA_String emptySuffix{ MAREA_stringOf(context, "") };

    MAREA_append(context, append, &target, &emptySuffix);

    EXPECT_EQ(target.lengthInBytes, 0) << "Appending the empty suffix changes nothing.";

    MAREA_String abc{ MAREA_stringOf(context, "abc") };

    MAREA_append(context, append, &target, &abc);

    EXPECT_EQ(MAREA_toText(target), "abc") << "The target now holds the suffix.";

    MAREA_String more{ MAREA_stringOf(context, "defghijkl") };

    MAREA_append(context, append, &target, &more);

    EXPECT_EQ(MAREA_toText(target), "abcdefghijkl") << "A second append concatenates again.";

    MAREA_HostContextFree(&context, target.data);

    MAREA_HostContextFree(&context, emptySuffix.data);

    MAREA_HostContextFree(&context, abc.data);

    MAREA_HostContextFree(&context, more.data);
}

TEST(MAREACString, AppendGrowsCapacityByMaxOfFourOrTwiceCapacity)
{
    GTEST_COUT << "Validating append's growth sequence is max(4, 2 * capacity), the same rule marea.vector "
                  "reuses (DECISIONS rows 24, 41, 46): 0 -> 4 -> 8 -> 16 as the target's length crosses each "
                  "threshold."
               << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    MAREA_String target{};

    MAREA_CStringFixture fixture;

    MAREA_HostFunction append{ fixture.MAREA_find("append") };

    MAREA_String one{ MAREA_stringOf(context, "a") };

    MAREA_append(context, append, &target, &one);

    EXPECT_EQ(target.capacityInBytes, 4) << "The first grow from zero capacity is max(4, 0) = 4.";

    MAREA_String four{ MAREA_stringOf(context, "bcde") };

    MAREA_append(context, append, &target, &four);

    EXPECT_EQ(target.lengthInBytes, 5) << "5 bytes now held.";

    EXPECT_EQ(target.capacityInBytes, 8) << "5 bytes needs more than 4, so capacity doubles to max(4, 2 * 4) = 8.";

    MAREA_String five{ MAREA_stringOf(context, "fghij") };

    MAREA_append(context, append, &target, &five);

    EXPECT_EQ(target.lengthInBytes, 10) << "10 bytes now held.";

    EXPECT_EQ(target.capacityInBytes, 16) << "10 bytes needs more than 8, so capacity doubles again to max(4, 2 * 8) = 16.";

    MAREA_HostContextFree(&context, target.data);

    MAREA_HostContextFree(&context, one.data);

    MAREA_HostContextFree(&context, four.data);

    MAREA_HostContextFree(&context, five.data);
}

TEST(MAREACString, ClearSetsLengthToZeroAndKeepsCapacity)
{
    GTEST_COUT << "Validating clear (string.rs's test_str_clear) sets length to zero and keeps capacity, on "
                  "an empty and on a non-empty string."
               << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    MAREA_String target{};

    MAREA_CStringFixture fixture;

    MAREA_HostFunction clear{ fixture.MAREA_find("clear") };

    MAREA_clear(context, clear, &target);

    EXPECT_EQ(target.lengthInBytes, 0) << "Clearing an already-empty string stays at zero.";

    EXPECT_EQ(target.capacityInBytes, 0) << "No storage was ever allocated.";

    MAREA_HostFunction append{ fixture.MAREA_find("append") };

    MAREA_String abc{ MAREA_stringOf(context, "abc") };

    MAREA_append(context, append, &target, &abc);

    MAREA_IntegerSize capacityBeforeClear{ target.capacityInBytes };

    MAREA_clear(context, clear, &target);

    EXPECT_EQ(target.lengthInBytes, 0) << "A non-empty string becomes the empty string.";

    EXPECT_EQ(target.capacityInBytes, capacityBeforeClear) << "The storage capacity is kept, not released.";

    MAREA_HostContextFree(&context, target.data);

    MAREA_HostContextFree(&context, abc.data);
}

TEST(MAREACString, CompareGivesTheSignOfTheByteOrderComparison)
{
    GTEST_COUT << "Validating compare returns negative, zero or positive by the byte order of the two "
                  "strings, including a prefix relationship (22_C_LIBRARIES.md D4)."
               << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    MAREA_CStringFixture fixture;

    MAREA_HostFunction compare{ fixture.MAREA_find("compare") };

    MAREA_String abc{ MAREA_stringOf(context, "abc") };

    MAREA_String abcCopy{ MAREA_stringOf(context, "abc") };

    MAREA_String abd{ MAREA_stringOf(context, "abd") };

    MAREA_String ab{ MAREA_stringOf(context, "ab") };

    EXPECT_EQ(MAREA_compare(context, compare, &abc, &abcCopy), 0) << "Equal byte sequences compare equal.";

    EXPECT_LT(MAREA_compare(context, compare, &abc, &abd), 0) << "abc sorts before abd.";

    EXPECT_GT(MAREA_compare(context, compare, &abd, &abc), 0) << "abd sorts after abc.";

    EXPECT_LT(MAREA_compare(context, compare, &ab, &abc), 0) << "A prefix sorts before the longer string it prefixes.";

    EXPECT_GT(MAREA_compare(context, compare, &abc, &ab), 0) << "The longer string sorts after its own prefix.";

    MAREA_HostContextFree(&context, abc.data);

    MAREA_HostContextFree(&context, abcCopy.data);

    MAREA_HostContextFree(&context, abd.data);

    MAREA_HostContextFree(&context, ab.data);
}

TEST(MAREACString, AppendRaisesOutOfMemoryAndLeavesTheTargetUnchangedWhenTheHostAllocatorFails)
{
    GTEST_COUT << "Validating append raises MAREA_HOST_TRAP_OUT_OF_MEMORY and leaves the target string "
                  "unchanged when the host allocator cannot grow its storage."
               << std::endl;

    MAREA_HostContext context{};

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, nullptr, nullptr));

    MAREA_IGNORE(MAREA_HostContextInitialize(&context, MAREA_failingAllocate, MAREA_failingFree));

    MAREA_String target{};

    MAREA_CStringFixture fixture;

    MAREA_HostFunction append{ fixture.MAREA_find("append") };

    MAREA_String abc{};

    abc.data = const_cast<char *>("abc");

    abc.lengthInBytes = MAREA_INTEGER_SIZE(3);

    abc.capacityInBytes = MAREA_INTEGER_SIZE(3);

    MAREA_append(context, append, &target, &abc);

    EXPECT_EQ(context.trap, MAREA_HOST_TRAP_OUT_OF_MEMORY) << "The failed grow raises the out-of-memory trap.";

    EXPECT_EQ(target.lengthInBytes, 0) << "The target is unchanged: still the empty string.";

    EXPECT_EQ(target.capacityInBytes, 0) << "No storage was ever attached to the target.";
}

TEST(MAREACString, ARegistryWithTheCStringBitOffRegistersNoFunctionOfThisLibrary)
{
    GTEST_COUT << "Validating MAREA_TypeRegistryRegisterInternalFunction's no-op contract: a disabled library "
                  "bit leaves every function of c.string unregistered."
               << std::endl;

    MAREA_CStringFixture fixture{ MAREA_LIBRARY_MASK_DEFAULT & ~MAREA_LIBRARY_FLAG_C_STRING };

    EXPECT_EQ(fixture.MAREA_find("length"), nullptr) << "The library bit was off, so nothing was registered.";
}
