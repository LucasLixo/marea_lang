// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREAShippedLibraries.hpp"

#include <string>

namespace
{
    MAREA_Boolean MAREA_CALL MAREA_sizeOfEveryFile(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_IntegerSize *outSize)
    {
        MAREA_IGNORE(userData);
        MAREA_IGNORE(path);
        MAREA_IGNORE(pathLength);

        *outSize = MAREA_INTEGER_SIZE(12);

        return MAREA_BOOLEAN_TRUE;
    }

    MAREA_Boolean MAREA_CALL MAREA_sizeOfNoFile(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_IntegerSize *outSize)
    {
        MAREA_IGNORE(userData);
        MAREA_IGNORE(path);
        MAREA_IGNORE(pathLength);
        MAREA_IGNORE(outSize);

        return MAREA_BOOLEAN_FALSE;
    }

    const MAREA_MacroDescriptor *MAREA_includeBytes(const MAREA_ShippedRegistry &shipped)
    {
        return MAREA_TypeRegistryFindMacro(shipped.registry, "marea.array", MAREA_INTEGER_SIZE(11), "include_bytes", MAREA_INTEGER_SIZE(13));
    }

    bool MAREA_expand(const MAREA_MacroDescriptor *macro, const std::string &path, MAREA_MacroIncludeSizeFunction includeSize, MAREA_IntegerSize &count)
    {
        const MAREA_MacroExpandRequest REQUEST{ path.c_str(), path.size(), includeSize, nullptr };

        return macro->expand(&REQUEST, &count) == MAREA_BOOLEAN_TRUE;
    }
} // namespace

TEST(MAREAArrayLibrary, ArrayLibraryRegisterGivesAnInlineRunOfTwoParameters)
{
    GTEST_COUT << "Validating the array library registers marea.array.Array as a copied, indexable run of an element and a count." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerArrayLibrary(SHIPPED.registry)) << "marea/base/MAREAArrayLibrary.h must declare the registration of the array library.";

    const MAREA_TypeDescriptor *DESCRIPTOR{ SHIPPED.MAREA_find("marea.array", "Array") };

    ASSERT_NE(DESCRIPTOR, nullptr) << "The type must be found by its namespace and name.";

    EXPECT_EQ(DESCRIPTOR->typeParameterCount, MAREA_INTEGER_SIZE(2)) << "An element type and a count.";

    EXPECT_EQ(DESCRIPTOR->roles, MAREA_TYPE_ROLE_NONE) << "An array has no role.";

    EXPECT_EQ(DESCRIPTOR->ownershipClass, MAREA_TYPE_OWNERSHIP_CLASS_COPY) << "An array is held by value.";

    EXPECT_EQ(DESCRIPTOR->hasIndexProtocol, MAREA_BOOLEAN_TRUE) << "An element is reached by its index.";
}

TEST(MAREAArrayLibrary, ArrayLibraryRegisterRegistersNothingWhenItsBitIsOff)
{
    GTEST_COUT << "Validating a registry with the array bit off holds no array type and no array macro." << std::endl;

    const MAREA_ShippedRegistry SHIPPED{ static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_MASK_DEFAULT & ~static_cast<MAREA_LibraryMask>(MAREA_LIBRARY_FLAG_MAREA_ARRAY)) };

    ASSERT_TRUE(MAREA_registerArrayLibrary(SHIPPED.registry)) << "A library that is off is a success that registers nothing.";

    EXPECT_EQ(SHIPPED.MAREA_find("marea.array", "Array"), nullptr) << "The type of a library that is off is unknown.";

    EXPECT_EQ(MAREA_includeBytes(SHIPPED), nullptr) << "The macro of a library that is off is unknown.";
}

TEST(MAREAArrayLibrary, ArrayLibraryRegisterRegistersTheIncludeBytesMacro)
{
    GTEST_COUT << "Validating include_bytes is a macro of marea.array that gives an array of uinteger8." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerArrayLibrary(SHIPPED.registry)) << "marea/base/MAREAArrayLibrary.h must declare the registration of the array library.";

    const MAREA_MacroDescriptor *MACRO{ MAREA_includeBytes(SHIPPED) };

    ASSERT_NE(MACRO, nullptr) << "The macro must be registered.";

    ASSERT_EQ(MACRO->argumentCount, MAREA_INTEGER_SIZE(1)) << "One argument, the path.";

    EXPECT_EQ(MACRO->argumentKinds[0], MAREA_MACRO_ARGUMENT_KIND_STRING_LITERAL) << "The path is a string literal.";

    EXPECT_EQ(MACRO->resultType, SHIPPED.MAREA_find("marea.array", "Array")) << "The result is the array type.";

    EXPECT_EQ(std::string(MACRO->resultElementText, MACRO->resultElementLength), "uinteger8") << "The elements are the bytes.";
}

TEST(MAREAArrayLibrary, IncludeBytesExpandsToTheSizeOfTheFile)
{
    GTEST_COUT << "Validating the expander takes the count from the include function and fails for a missing file." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerArrayLibrary(SHIPPED.registry)) << "marea/base/MAREAArrayLibrary.h must declare the registration of the array library.";

    const MAREA_MacroDescriptor *MACRO{ MAREA_includeBytes(SHIPPED) };

    ASSERT_NE(MACRO, nullptr) << "The macro must be registered.";

    MAREA_IntegerSize count{ 0 };

    ASSERT_TRUE(MAREA_expand(MACRO, "images/logo.bin", &MAREA_sizeOfEveryFile, count)) << "A file that the caller knows is expanded.";

    EXPECT_EQ(count, MAREA_INTEGER_SIZE(12)) << "The count is the size of the file.";

    EXPECT_FALSE(MAREA_expand(MACRO, "images/logo.bin", &MAREA_sizeOfNoFile, count)) << "A file that the caller does not know is an error.";

    EXPECT_FALSE(MAREA_expand(MACRO, "images/logo.bin", nullptr, count)) << "With no include function every file is missing.";
}

TEST(MAREAArrayLibrary, IncludeBytesRejectsAnAbsolutePath)
{
    GTEST_COUT << "Validating a path that leaves the project root is refused, even when the caller would answer its size." << std::endl;

    const MAREA_ShippedRegistry SHIPPED;

    ASSERT_TRUE(MAREA_registerArrayLibrary(SHIPPED.registry)) << "marea/base/MAREAArrayLibrary.h must declare the registration of the array library.";

    const MAREA_MacroDescriptor *MACRO{ MAREA_includeBytes(SHIPPED) };

    ASSERT_NE(MACRO, nullptr) << "The macro must be registered.";

    MAREA_IntegerSize count{ 0 };

    EXPECT_FALSE(MAREA_expand(MACRO, "/etc/passwd", &MAREA_sizeOfEveryFile, count)) << "An absolute path is outside the project root.";
}
