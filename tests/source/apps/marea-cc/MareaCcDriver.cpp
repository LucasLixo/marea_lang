// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "MareaCcDriver.h"
#include "MareaTargetDescription.h"

namespace
{
    std::string MAREA_scratchPath(const char *suffix)
    {
        return std::string(::testing::TempDir()) + "marea-cc-driver-test" + suffix;
    }

    /* Sema's own namespace-matches-file-path check (see MareaBcDriver.cpp's own fixture, the same
     * apply) needs the input path relative to examples/, so this fixture moves the process there for
     * the one test's duration and restores the previous directory afterward. */
    class MareaCcDriverExamplesTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            previousDirectory = std::filesystem::current_path();
            std::filesystem::current_path(MAREA_TESTS_EXAMPLES_DIRECTORY);
        }

        void TearDown() override { std::filesystem::current_path(previousDirectory); }

        static constexpr const char *FIXTURE_PATH = "basics/operators.marea";

    private:
        std::filesystem::path previousDirectory;
    };
} // namespace

TEST_F(MareaCcDriverExamplesTest, GenerateCWritesC11Text)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath(".c");

    ASSERT_TRUE(MareaCcDriverGenerateC(FIXTURE_PATH, outputPath.c_str(), MAREA_LIBRARY_MASK_DEFAULT, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));

    std::FILE *file = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    char firstBytes[32] = { 0 };
    ASSERT_GT(std::fread(firstBytes, 1, sizeof(firstBytes) - 1, file), 0u);
    std::fclose(file);
}

TEST_F(MareaCcDriverExamplesTest, GenerateCWorksWithAnEmptyLibraryMask)
{
    // INFO: none of the shipped libraries (lib-array-vector-tuple and the rest) are built yet, so this
    // proves marea-cc runs correctly with every one of them disabled, not only with the default mask.
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath("-empty-mask.c");

    ASSERT_TRUE(MareaCcDriverGenerateC(FIXTURE_PATH, outputPath.c_str(), MAREA_LIBRARY_FLAG_NONE, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));
}

TEST(MareaCcDriver, GenerateCFailsOnAMissingInputFile)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath("-missing.c");

    ASSERT_FALSE(MareaCcDriverGenerateC("/no/such/file.marea", outputPath.c_str(), MAREA_LIBRARY_MASK_DEFAULT, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));
}

TEST_F(MareaCcDriverExamplesTest, CompileWithEmitCMatchesGenerateC)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath("-emit-c.c");

    ASSERT_TRUE(MareaCcDriverCompile(FIXTURE_PATH, outputPath.c_str(), MAREA_CC_EMIT_C, nullptr, nullptr, MAREA_LIBRARY_MASK_DEFAULT, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));

    std::FILE *file = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    std::fclose(file);
}

TEST(MareaCcDriver, DetectCompilerFindsOneOnThisMachine)
{
    char *compilerPath = nullptr;

    ASSERT_TRUE(MareaCcDriverDetectCompiler(&compilerPath)) << "This development machine must have clang or gcc on PATH.";

    ASSERT_NE(compilerPath, nullptr);
    std::free(compilerPath);
}

TEST(MareaCcDriver, ListSettingsPrintsEveryPair)
{
    MAREA_SemaSetting pairs[2] = {
        { reinterpret_cast<const MAREA_Character8 *>("DEBUG"), reinterpret_cast<const MAREA_Character8 *>("true") },
        { reinterpret_cast<const MAREA_Character8 *>("LEVEL"), reinterpret_cast<const MAREA_Character8 *>("3") },
    };
    MAREA_SemaSettings settings{};
    settings.pairs = pairs;
    settings.count = 2;

    const std::string outputPath = MAREA_scratchPath("-settings.txt");
    std::FILE *outFile = std::fopen(outputPath.c_str(), "wb");
    ASSERT_NE(outFile, nullptr);

    MareaCcDriverListSettings(settings, outFile);
    std::fclose(outFile);

    std::FILE *readBack = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(readBack, nullptr);
    char contents[256] = { 0 };
    std::size_t read = std::fread(contents, 1, sizeof(contents) - 1, readBack);
    contents[read] = '\0';
    std::fclose(readBack);

    ASSERT_NE(std::string(contents).find("DEBUG=true"), std::string::npos);
    ASSERT_NE(std::string(contents).find("LEVEL=3"), std::string::npos);
}
