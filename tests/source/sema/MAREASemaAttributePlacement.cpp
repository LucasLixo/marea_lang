// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaCallerHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 MISPLACED{ MAREA_SEMA_DIAGNOSTIC_CODE_MISPLACED_ATTRIBUTE };

    // INFO: the ten keys of the matrix, each with a value that is valid wherever the key may stand.
    struct MAREA_Key
    {
        const char *name;
        const char *text;
    };

    const std::vector<MAREA_Key> KEYS{
        { "visibility", "visibility = public" },
        { "inline", "inline = never" },
        { "build", "build = release" },
        { "layout", "layout = c" },
        { "align", "align = 16" },
        { "pack", "pack = 4" },
        { "move_only", "move_only = true" },
        { "operator", "operator = \"()\"" },
        { "export", "export = \"label\"" },
        { "semantics", "semantics = reference_read_only" },
    };

    // INFO: one row of the placement matrix: a program with the marker ATTRIBUTE where the attribute list goes, and the keys that the row allows.
    struct MAREA_Place
    {
        const char *name;
        std::string program;
        std::vector<std::string> allowed;
    };

    const std::string STRUCTURE{ "structure S\n    x: sinteger32;\nend\n" };

    const std::vector<MAREA_Place> PLACES{
        { "file", "namespace main;\n#![ATTRIBUTE]\n", { "visibility", "inline", "build", "layout" } },
        { "top-level function", "namespace main;\n#[ATTRIBUTE]\nfunction run()\nend\n", { "visibility", "inline", "build", "export" } },
        { "top-level constant", "namespace main;\n#[ATTRIBUTE]\nconstant LIMIT: sinteger32 = 1;\n", { "visibility" } },
        { "structure", "namespace main;\n#[ATTRIBUTE]\n" + STRUCTURE, { "visibility", "layout", "align", "pack", "move_only" } },
        { "opaque structure", "namespace main;\n#[ATTRIBUTE]\nopaque structure Handle;\n", { "visibility" } },
        { "enum", "namespace main;\n#[ATTRIBUTE]\nenum E\n    A;\nend\n", { "visibility", "layout", "align", "pack", "move_only" } },
        { "type alias", "namespace main;\n#[ATTRIBUTE]\ntype Count = sinteger32;\n", { "visibility" } },
        { "contract", "namespace main;\n#[ATTRIBUTE]\ncontract Shape\n    function area(): float64;\nend\n", { "visibility" } },
        { "implementation block", "namespace main;\n" + STRUCTURE + "#[ATTRIBUTE]\nimplementation S\n    function get(self: S): sinteger32\n        return 1;\n    end\nend\n", { "visibility" } },
        { "field of a structure", "namespace main;\nstructure S\n    #[ATTRIBUTE]\n    x: sinteger32;\nend\n", { "visibility", "align" } },
        { "function in an implementation", "namespace main;\n" + STRUCTURE + "implementation S\n    #[ATTRIBUTE]\n    function call(self: S)\n    end\nend\n", { "visibility", "inline", "build", "operator", "export" } },
        { "constant in an implementation", "namespace main;\n" + STRUCTURE + "implementation S\n    #[ATTRIBUTE]\n    constant LIMIT: sinteger32 = 1;\nend\n", { "visibility" } },
        { "parameter", "namespace main;\n" + STRUCTURE + "function run(#[ATTRIBUTE] value: S)\nend\n", { "semantics" } },
        { "macro statement", "namespace main;\n#[ATTRIBUTE]\nstatic_assert!(true, \"x\");\n", {} },
    };

    std::string MAREA_fill(const std::string &program, const std::string &attribute)
    {
        const std::size_t AT{ program.find("ATTRIBUTE") };

        return program.substr(0, AT) + attribute + program.substr(AT + 9);
    }

    bool MAREA_allows(const MAREA_Place &place, const std::string &key)
    {
        for (const std::string &allowed : place.allowed) {
            if (allowed == key) {
                return true;
            }
        }

        return false;
    }
} // namespace

TEST(MAREASemaAttributePlacement, EveryCellOfTheMatrixIsEnforced)
{
    GTEST_COUT << "Validating every cell of the placement matrix of the attribute map: a key where the matrix says yes raises nothing, and where it says no raises MISPLACED_ATTRIBUTE." << std::endl;

    for (const MAREA_Place &place : PLACES) {
        for (const MAREA_Key &key : KEYS) {
            const Codes EXPECTED{ MAREA_allows(place, key.name) ? Codes{} : Codes{ MISPLACED } };

            EXPECT_EQ(MAREA_codesWith(MAREA_fill(place.program, key.text)), EXPECTED) << "The key " << key.name << " at the place '" << place.name << "' must " << (MAREA_allows(place, key.name) ? "be accepted." : "raise MISPLACED_ATTRIBUTE.");
        }
    }
}

TEST(MAREASemaAttributePlacement, AMisplacedAttributeIsReportedOncePerItem)
{
    GTEST_COUT << "Validating two misplaced keys in one list raise two diagnostics, one per item, and a placed key beside them raises none." << std::endl;

    const std::string PROGRAM{ "namespace main;\n#[visibility = public, inline = force, build = release]\nconstant LIMIT: sinteger32 = 1;\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM), (Codes{ MISPLACED, MISPLACED }));
}

TEST(MAREASemaAttributePlacement, TheMessageNamesTheKey)
{
    GTEST_COUT << "Validating the MISPLACED_ATTRIBUTE message names the key that stands in the wrong place." << std::endl;

    MAREA_CallerRun run("namespace main;\n#[align = 16]\nfunction run()\nend\n", MAREA_CallerInputs{});

    const std::vector<std::string> MESSAGES{ run.MAREA_messages() };

    ASSERT_EQ(MESSAGES.size(), 1U) << "One diagnostic is expected.";
    EXPECT_NE(MESSAGES[0].find("'align'"), std::string::npos) << "The message must name the key.";
}

TEST(MAREASemaAttributePlacement, ABareBooleanKeyIsPlacedLikeAnyOther)
{
    GTEST_COUT << "Validating a boolean key written bare is checked for its place like a key with a value." << std::endl;

    EXPECT_EQ(MAREA_codesWith("namespace main;\n#[move_only]\nstructure S\n    x: sinteger32;\nend\n"), Codes{});
    EXPECT_EQ(MAREA_codesWith("namespace main;\n#[move_only]\nfunction run()\nend\n"), Codes{ MISPLACED });
}

TEST(MAREASemaAttributePlacement, AnExportOnAGenericFunctionIsMisplaced)
{
    GTEST_COUT << "Validating an export on a function with its own generic parameter is MISPLACED_ATTRIBUTE, because C has no generics." << std::endl;

    EXPECT_EQ(MAREA_codesWith("namespace main;\n#[export = \"label\"]\nfunction run<T>()\nend\n"), Codes{ MISPLACED });
    EXPECT_EQ(MAREA_codesWith("namespace main;\n#[export = \"label\"]\nfunction run()\nend\n"), Codes{});
}

TEST(MAREASemaAttributePlacement, AnExportInAGenericImplementationIsMisplaced)
{
    GTEST_COUT << "Validating an export on a function of a generic implementation block is MISPLACED_ATTRIBUTE." << std::endl;

    const std::string GENERIC{ "namespace main;\nstructure Box<T>\n    value: T;\nend\nimplementation<T> Box<T>\n    #[export = \"get\"]\n    function get(self: Box<T>)\n    end\nend\n" };

    EXPECT_EQ(MAREA_codesWith(GENERIC), Codes{ MISPLACED });

    const std::string CONCRETE{ "namespace main;\nstructure Box<T>\n    value: T;\nend\nimplementation Box<sinteger32>\n    #[export = \"get\"]\n    function get(self: Box<sinteger32>)\n    end\nend\n" };

    EXPECT_EQ(MAREA_codesWith(CONCRETE), Codes{});
}

TEST(MAREASemaAttributePlacement, TheExportLabelMustBeAValidCIdentifier)
{
    GTEST_COUT << "Validating an export label that is not a valid C identifier raises INVALID_ATTRIBUTE_VALUE." << std::endl;

    const std::string PREFIX{ "namespace main;\n#[export = " };

    const std::string SUFFIX{ "]\nfunction run()\nend\n" };

    EXPECT_EQ(MAREA_codesWith(PREFIX + "\"run_it\"" + SUFFIX), Codes{});
    EXPECT_EQ(MAREA_codesWith(PREFIX + "\"_run2\"" + SUFFIX), Codes{});
    EXPECT_EQ(MAREA_codesWith(PREFIX + "\"1run\"" + SUFFIX), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE });
    EXPECT_EQ(MAREA_codesWith(PREFIX + "\"run it\"" + SUFFIX), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE });
    EXPECT_EQ(MAREA_codesWith(PREFIX + "\"run-it\"" + SUFFIX), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE });
    EXPECT_EQ(MAREA_codesWith(PREFIX + "\"\"" + SUFFIX), Codes{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE });
}
