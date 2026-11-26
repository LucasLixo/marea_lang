// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISMATCH{ MAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH };

    Codes MAREA_codesOf(const std::string &body)
    {
        MAREA_SemaFixture fixture([](const std::string &, const MAREA_AstNode **) { return MAREA_SEMA_IMPORT_RESULT_EXTERNAL; });

        if (fixture.MAREA_sema("namespace main; " + body) != MAREA_SEMA_EXCEPT_SUCCESS) {
            return { 0 };
        }

        for (MAREA_Uinteger32 code : fixture.parser.MAREA_codes()) {
            EXPECT_GE(code, 3000U) << "The program must lex and parse without a diagnostic: " << body;
        }

        return fixture.MAREA_semaCodes();
    }
} // namespace

TEST(MAREASemaKnownTypes, ViewAndRawPointerAreKnownShapes)
{
    GTEST_COUT << "Validating a slice and a raw pointer are known shapes, as a value and as a target." << std::endl;

    EXPECT_EQ(MAREA_codesOf("function g(s: demo.slice<float32>) local n: sinteger32 = s; end"), Codes{ MISMATCH }) << "A slice value for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("require vulkan; function g() unsafe local p: demo.pointer<uinteger8> = vulkan.take(); local n: uinteger64 = p; end end"), Codes{ MISMATCH }) << "A raw pointer value for a primitive must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function f(a: demo.slice<sinteger32>) end function g(s: string) f(s); end"), Codes{ MISMATCH }) << "A string for a slice parameter must raise TYPE_MISMATCH.";

    EXPECT_EQ(MAREA_codesOf("function g() unsafe local p: demo.pointer<uinteger8> = 5; end end"), Codes{ MISMATCH }) << "A number for a raw pointer must raise TYPE_MISMATCH.";

    EXPECT_TRUE(MAREA_codesOf("function f(a: demo.slice<sinteger32>) end function g(s: demo.slice<sinteger32>) f(s); local v: demo.slice<sinteger32> = s; end").empty()) << "A slice of the same element must raise nothing.";

    EXPECT_TRUE(MAREA_codesOf("require vulkan; function g() unsafe local p: demo.pointer<uinteger8> = vulkan.take(); local q: demo.pointer<uinteger8> = p; local w: uinteger64 = convert<uinteger64>(p); end end").empty()) << "A raw pointer of the same element and a conversion of it must raise nothing.";
}
