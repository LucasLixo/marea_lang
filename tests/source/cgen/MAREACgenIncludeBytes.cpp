// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAIncludedFiles.hpp"
#include "tests/MAREAParityHelpers.hpp"

#include <string>
#include <vector>

namespace
{
    const std::string MAREA_PRELUDE{ "namespace main; require marea.array; " };

    // INFO: zero bytes inside and at both ends, and the highest byte.
    const std::string MAREA_BYTES{ std::string("\0\1\0\xff\0", 5) };

    std::size_t MAREA_count(const std::string &text, const std::string &part)
    {
        std::size_t count{ 0 };

        for (std::size_t at{ text.find(part) }; at != std::string::npos; at = text.find(part, at + 1)) {
            ++count;
        }

        return count;
    }
} // namespace

TEST(MAREACgenIncludeBytes, BothBackendsReadTheBytesOfTheFile)
{
    GTEST_COUT << "Validating a constant, a local and two uses of one file read the same bytes in both backends." << std::endl;

    MAREA_IncludedFiles files;

    files.MAREA_add("data with \" and space.bin", "unused");
    files.MAREA_add("zero bytes.bin", MAREA_BYTES);

    const std::string SOURCE{ MAREA_PRELUDE + "constant DATA: marea.array.Array<uinteger8, 5> = marea.array.include_bytes!(\"zero bytes.bin\"); function sum(): uinteger32 local total: uinteger32 = 0; for each value in DATA do total = (total * 256) + convert<uinteger32>(value); end return total; end function at(index: uinteger64): uinteger8 return DATA[index]; end function again(): uinteger64 local data: marea.array.Array<uinteger8, 5> = marea.array.include_bytes!(\"zero bytes.bin\"); return convert<uinteger64>(data[3]) + data.size(); end" };

    const std::vector<MAREA_ParityCall> CALLS{
        MAREA_parityCall("sum", "uinteger32"),
        MAREA_parityCall("at", "uinteger8", { MAREA_parityUnsigned("uinteger64", 3) }),
        MAREA_parityCall("at", "uinteger8", { MAREA_parityUnsigned("uinteger64", 4) }),
        MAREA_parityCall("at", "uinteger8", { MAREA_parityUnsigned("uinteger64", 5) }),
        MAREA_parityCall("again", "uinteger64"),
    };

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, CALLS, nullptr, &files.resolver) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 0x0100FF00) << "The five bytes in order, the first one shifted out.";
    EXPECT_EQ(MAREA_parityValue(RESULT.generated[1]), 255) << "The fourth byte.";
    EXPECT_EQ(MAREA_parityValue(RESULT.generated[2]), 0) << "The last byte is a zero of the file.";
    EXPECT_EQ(RESULT.generated[3].trap, 8) << "No byte past the file.";
    EXPECT_EQ(MAREA_parityValue(RESULT.generated[4]), 260) << "The second use reads the same file.";

    EXPECT_EQ(MAREA_count(RESULT.cText, ".incbin"), 1U) << "One file is embedded once.";

    EXPECT_NE(RESULT.cText.find("#if defined(__EMSCRIPTEN__)"), std::string::npos) << "Emscripten loads the file instead.";
}

TEST(MAREACgenIncludeBytes, AnEmptyFileIsAnArrayOfNoByte)
{
    GTEST_COUT << "Validating an empty file gives an array of count zero, and a file with a mark and a space in its name is embedded." << std::endl;

    MAREA_IncludedFiles files;

    files.MAREA_add("empty.bin", "");
    files.MAREA_add("odd \" name.bin", "AB");

    const std::string SOURCE{ MAREA_PRELUDE + "constant NONE: marea.array.Array<uinteger8, 0> = marea.array.include_bytes!(\"empty.bin\"); constant ODD: marea.array.Array<uinteger8, 2> = marea.array.include_bytes!(\"odd \\\" name.bin\"); function f(): uinteger64 local count: uinteger64 = 0; for each value in NONE do count += 1; end return count + NONE.size(); end function g(): uinteger8 return ODD[1]; end" };

    // INFO: a path with a quote needs the escape of the language in the source, so the resolver sees it as written.
    files.entries["odd \\\" name.bin"] = files.entries["odd \" name.bin"];

    const MAREA_ParityResult RESULT{ MAREA_parityCheck(SOURCE, { MAREA_parityCall("f", "uinteger64"), MAREA_parityCall("g", "uinteger8") }, nullptr, &files.resolver) };

    ASSERT_TRUE(RESULT.failures.empty()) << RESULT.MAREA_failures();

    EXPECT_EQ(MAREA_parityValue(RESULT.generated[0]), 0) << "No byte.";
    EXPECT_EQ(MAREA_parityValue(RESULT.generated[1]), 'B') << "The second byte.";
}

TEST(MAREACgenIncludeBytes, ALoadThatDiffersFromTheSizeIsAnError)
{
    GTEST_COUT << "Validating the bytes that the caller gives must be as many as the size that it answered." << std::endl;

    MAREA_IncludedFiles files;

    files.MAREA_add("data.bin", "ABC");

    files.shortLoad = true;

    MAREA_IrFixture fixture(MAREA_parityLibraries());

    MAREA_IGNORE(MAREA_SemaContextSetIncludeResolver(fixture.sema.context, &files.resolver));
    MAREA_IGNORE(fixture.sema.MAREA_sema(MAREA_PRELUDE + "function f() marea.array.include_bytes!(\"data.bin\"); end", "main.marea"));

    EXPECT_EQ(fixture.sema.MAREA_semaCodes(), (std::vector<MAREA_Uinteger32>{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT })) << "The call cannot expand.";
}
