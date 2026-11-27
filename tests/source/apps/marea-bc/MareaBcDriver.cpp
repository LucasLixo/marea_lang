// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

#include "MareaBcDriver.h"
#include "MareaTargetDescription.h"

namespace
{
    std::string MAREA_scratchPath(const char *suffix)
    {
        return std::string(::testing::TempDir()) + "marea-bc-driver-test" + suffix;
    }

    /* Sema's own namespace-matches-file-path check (apply marea-cc-and-bc-tools' own "Do not touch":
     * sema is driven, never changed) compares the whole given path, end to end, against the dotted
     * namespace. examples/basics/operators.marea's own namespace is "basics.operators", so the input
     * path given to the driver must be exactly "basics/operators.marea", relative to examples/ - the
     * same shape a caller cd'd into examples/ would give. This fixture moves the process there for the
     * one test's duration and restores the previous directory afterward. */
    class MareaBcDriverExamplesTest : public ::testing::Test
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

TEST_F(MareaBcDriverExamplesTest, CompileProducesABytecodeModule)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath(".mbc");

    ASSERT_TRUE(MareaBcDriverCompile(FIXTURE_PATH, outputPath.c_str(), MAREA_LIBRARY_MASK_DEFAULT, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));

    std::FILE *file = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(file, nullptr);
    std::fclose(file);
}

TEST_F(MareaBcDriverExamplesTest, CompileWorksWithAnEmptyLibraryMask)
{
    // INFO: none of the shipped libraries (lib-array-vector-tuple and the rest) are built yet, so this
    // proves marea-bc runs correctly with every one of them disabled, not only with the default mask.
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath("-empty-mask.mbc");

    ASSERT_TRUE(MareaBcDriverCompile(FIXTURE_PATH, outputPath.c_str(), MAREA_LIBRARY_FLAG_NONE, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));
}

TEST(MareaBcDriver, CompileFailsOnAMissingInputFile)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath("-missing.mbc");

    ASSERT_FALSE(MareaBcDriverCompile("/no/such/file.marea", outputPath.c_str(), MAREA_LIBRARY_MASK_DEFAULT, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));
}

TEST_F(MareaBcDriverExamplesTest, DisassembleThenAssembleRoundTripsByteIdentically)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string modulePath = MAREA_scratchPath("-roundtrip.mbc");
    const std::string textPath = MAREA_scratchPath("-roundtrip.mbca");
    const std::string reassembledPath = MAREA_scratchPath("-roundtrip-2.mbc");

    ASSERT_TRUE(MareaBcDriverCompile(FIXTURE_PATH, modulePath.c_str(), MAREA_LIBRARY_MASK_DEFAULT, MAREA_SemaSettings{}, &target, nullptr, 0, stderr));
    ASSERT_TRUE(MareaBcDriverDisassemble(modulePath.c_str(), textPath.c_str(), stderr));
    ASSERT_TRUE(MareaBcDriverAssemble(textPath.c_str(), reassembledPath.c_str(), stderr));

    std::FILE *original = std::fopen(modulePath.c_str(), "rb");
    std::FILE *reassembled = std::fopen(reassembledPath.c_str(), "rb");
    ASSERT_NE(original, nullptr);
    ASSERT_NE(reassembled, nullptr);

    int originalByte;
    int reassembledByte;

    do {
        originalByte = std::fgetc(original);
        reassembledByte = std::fgetc(reassembled);
        ASSERT_EQ(originalByte, reassembledByte);
    } while (originalByte != EOF);

    std::fclose(original);
    std::fclose(reassembled);
}

TEST_F(MareaBcDriverExamplesTest, DumpTokensListsAtLeastOneToken)
{
    const std::string outputPath = MAREA_scratchPath("-tokens.txt");
    std::FILE *outFile = std::fopen(outputPath.c_str(), "wb");
    ASSERT_NE(outFile, nullptr);

    ASSERT_TRUE(MareaBcDriverDumpTokens(FIXTURE_PATH, outFile, stderr));
    std::fclose(outFile);

    std::FILE *readBack = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(readBack, nullptr);
    char firstLine[256] = { 0 };
    ASSERT_NE(std::fgets(firstLine, sizeof(firstLine), readBack), nullptr);
    std::fclose(readBack);

    ASSERT_NE(std::string(firstLine).find("kind="), std::string::npos);
}

TEST_F(MareaBcDriverExamplesTest, DumpAstPrintsTheCompilationUnitRoot)
{
    const std::string outputPath = MAREA_scratchPath("-ast.txt");
    std::FILE *outFile = std::fopen(outputPath.c_str(), "wb");
    ASSERT_NE(outFile, nullptr);

    ASSERT_TRUE(MareaBcDriverDumpAst(FIXTURE_PATH, outFile, stderr));
    std::fclose(outFile);

    std::FILE *readBack = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(readBack, nullptr);
    char firstLine[256] = { 0 };
    ASSERT_NE(std::fgets(firstLine, sizeof(firstLine), readBack), nullptr);
    std::fclose(readBack);

    ASSERT_NE(std::string(firstLine).find("COMPILATION_UNIT"), std::string::npos);
}

TEST_F(MareaBcDriverExamplesTest, DumpIrPrintsTheModuleHeader)
{
    MAREA_SemaTarget target{};
    MareaTargetDescriptionDetectHost(&target);

    const std::string outputPath = MAREA_scratchPath("-ir.txt");
    std::FILE *outFile = std::fopen(outputPath.c_str(), "wb");
    ASSERT_NE(outFile, nullptr);

    ASSERT_TRUE(MareaBcDriverDumpIr(FIXTURE_PATH, MAREA_LIBRARY_MASK_DEFAULT, &target, nullptr, 0, outFile, stderr));
    std::fclose(outFile);

    std::FILE *readBack = std::fopen(outputPath.c_str(), "rb");
    ASSERT_NE(readBack, nullptr);
    char firstLine[256] = { 0 };
    ASSERT_NE(std::fgets(firstLine, sizeof(firstLine), readBack), nullptr);
    std::fclose(readBack);

    ASSERT_NE(std::string(firstLine).find("MODULE"), std::string::npos);
}

TEST(MareaBcDriver, ListSettingsPrintsEveryPair)
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

    MareaBcDriverListSettings(settings, outFile);
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
