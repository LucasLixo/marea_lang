// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaCallerHelpers.hpp"

namespace
{
    using Codes = std::vector<MAREA_Uinteger32>;

    constexpr MAREA_Uinteger32 UNKNOWN_KEY{ MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_ATTRIBUTE_KEY };
    constexpr MAREA_Uinteger32 INVALID_VALUE{ MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_ATTRIBUTE_VALUE };
    constexpr MAREA_Uinteger32 CONFLICTING{ MAREA_SEMA_DIAGNOSTIC_CODE_CONFLICTING_ATTRIBUTES };
    constexpr MAREA_Uinteger32 MISPLACED{ MAREA_SEMA_DIAGNOSTIC_CODE_MISPLACED_ATTRIBUTE };
    constexpr MAREA_Uinteger32 UNAVAILABLE{ MAREA_SEMA_DIAGNOSTIC_CODE_BACKEND_UNAVAILABLE };
    constexpr MAREA_Uinteger32 NOT_ALLOWED{ MAREA_SEMA_DIAGNOSTIC_CODE_ATTRIBUTE_NOT_ALLOWED_HERE };
    // INFO: a file under the root marea is always a RESERVED_NAMESPACE_ROOT for the project rule, so a program of marea.math carries that code too; the shipped file of the library is not checked as a project file yet.
    constexpr MAREA_Uinteger32 RESERVED_ROOT{ MAREA_SEMA_DIAGNOSTIC_CODE_RESERVED_NAMESPACE_ROOT };

    std::string MAREA_onFunction(const std::string &attribute)
    {
        return "namespace main;\n#[" + attribute + "]\nfunction run()\nend\n";
    }

    std::string MAREA_onFile(const std::string &attribute)
    {
        return "namespace main;\n#![" + attribute + "]\nfunction run()\nend\n";
    }

    std::string MAREA_math(const std::string &attribute)
    {
        return "namespace marea.math;\n#![" + attribute + "]\n";
    }

    MAREA_CallerInputs MAREA_target(const std::string &architecture, const std::vector<std::string> &features, const std::string &compiler = "gcc")
    {
        MAREA_CallerInputs inputs;

        inputs.hasTarget = true;
        inputs.architecture = architecture;
        inputs.operatingSystem = "linux";
        inputs.compiler = compiler;
        inputs.endianness = "little";
        inputs.features = features;
        inputs.pointerWidth = 64;

        return inputs;
    }
} // namespace

TEST(MAREASemaWhen, APredicateThatHoldsAppliesItsAttributes)
{
    GTEST_COUT << "Validating a when item is accepted whether its predicate holds on the target or not." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"x86_64\", inline = never)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"aarch64\", inline = never)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(operating_system = \"linux\", inline = never, build = release)"), LINUX), Codes{});
}

TEST(MAREASemaWhen, EveryPredicateNameIsKnown)
{
    GTEST_COUT << "Validating the six predicate names, with a string value or an integer value as each one needs." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    for (const char *predicate : { "architecture = \"x86_64\"", "operating_system = \"linux\"", "feature = \"avx2\"", "compiler = \"gcc\"", "pointer_width = 64", "endianness = \"little\"" }) {
        EXPECT_EQ(MAREA_codesWith(MAREA_onFunction(std::string("when(") + predicate + ", inline = never)"), LINUX), Codes{}) << "The predicate " << predicate << " must be accepted.";
    }
}

TEST(MAREASemaWhen, TheCombinatorsAreAccepted)
{
    GTEST_COUT << "Validating all, any and not over predicates are accepted." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(all(architecture = \"x86_64\", feature = \"sse2\"), inline = never)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(any(architecture = \"arm\", feature = \"sse2\"), inline = never)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(not(feature = \"neon\"), inline = never)"), LINUX), Codes{});
}

TEST(MAREASemaWhen, AnUnknownPredicateNameIsReportedOncePerPredicate)
{
    GTEST_COUT << "Validating a predicate name that the plan does not list raises INVALID_ATTRIBUTE_VALUE, once for each such predicate." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(colour = \"red\", inline = never)")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(all(colour = \"red\", shape = \"round\"), inline = never)")), (Codes{ INVALID_VALUE, INVALID_VALUE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(not(colour = \"red\"), inline = never)")), Codes{ INVALID_VALUE });
}

TEST(MAREASemaWhen, APredicateValueOfTheWrongKindIsInvalid)
{
    GTEST_COUT << "Validating pointer_width needs an integer and every other predicate needs a string." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(pointer_width = \"64\", inline = never)")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = 64, inline = never)")), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(pointer_width = 64, inline = never)")), Codes{});
}

TEST(MAREASemaWhen, TheListedAttributesAreCheckedWhateverThePredicateSays)
{
    GTEST_COUT << "Validating the attributes inside a when item are checked for key, value and place even when the predicate is false." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"aarch64\", frobnicate = true)"), LINUX), Codes{ UNKNOWN_KEY });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"aarch64\", inline = sometimes)"), LINUX), Codes{ INVALID_VALUE });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"aarch64\", align = 16)"), LINUX), Codes{ MISPLACED });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"x86_64\", align = 16)"), LINUX), Codes{ MISPLACED });
}

TEST(MAREASemaWhen, AWhenThatHoldsOverridesThePlainValueOfTheSameKey)
{
    GTEST_COUT << "Validating a plain key and a holding when on the same key are not in conflict: the when wins." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("inline = force, when(architecture = \"x86_64\", inline = never)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"x86_64\", inline = never), inline = force"), LINUX), Codes{});
}

TEST(MAREASemaWhen, TwoWhenThatHoldOnOneKeyConflict)
{
    GTEST_COUT << "Validating two when items that hold and give the same key raise CONFLICTING_ATTRIBUTES, so order never decides." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"x86_64\", inline = never), when(operating_system = \"linux\", inline = force)"), LINUX), Codes{ CONFLICTING });
}

TEST(MAREASemaWhen, TwoWhenOfWhichOnlyOneHoldsDoNotConflict)
{
    GTEST_COUT << "Validating two when items on one key are fine when at most one of them holds." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"aarch64\", inline = never), when(operating_system = \"linux\", inline = force)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"aarch64\", inline = never), when(operating_system = \"macos\", inline = force)"), LINUX), Codes{});
}

TEST(MAREASemaWhen, WithNoTargetEveryPredicateIsFalse)
{
    GTEST_COUT << "Validating a context with no target answers no predicate, so two when on one key never conflict." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"x86_64\", inline = never), when(operating_system = \"linux\", inline = force)")), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(not(architecture = \"x86_64\"), inline = never), when(not(operating_system = \"linux\"), inline = force)")), Codes{ CONFLICTING });
}

TEST(MAREASemaWhen, APredicateTheTargetDoesNotAnswerIsFalse)
{
    GTEST_COUT << "Validating a field that the target leaves empty is not answered, so its predicate is false." << std::endl;

    MAREA_CallerInputs partial;

    partial.hasTarget = true;
    partial.architecture = "x86_64";

    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(compiler = \"gcc\", inline = never), when(architecture = \"x86_64\", inline = force)"), partial), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(pointer_width = 64, inline = never), when(architecture = \"x86_64\", inline = force)"), partial), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(feature = \"sse2\", inline = never), when(architecture = \"x86_64\", inline = force)"), partial), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("when(architecture = \"x86_64\", inline = never), when(architecture = \"x86_64\", inline = force)"), partial), Codes{ CONFLICTING });
}

TEST(MAREASemaWhen, EachPredicateIsJudgedAgainstItsFieldOfTheTarget)
{
    GTEST_COUT << "Validating each predicate name reads its own field: the second when holds, so the first must not." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    const std::vector<std::pair<std::string, std::string>> FALSE_AND_TRUE{
        { "architecture = \"riscv64\"", "architecture = \"x86_64\"" },
        { "operating_system = \"windows\"", "operating_system = \"linux\"" },
        { "feature = \"neon\"", "feature = \"avx2\"" },
        { "compiler = \"clang\"", "compiler = \"gcc\"" },
        { "pointer_width = 32", "pointer_width = 64" },
        { "endianness = \"big\"", "endianness = \"little\"" },
    };

    for (const auto &pair : FALSE_AND_TRUE) {
        const std::string PROGRAM{ MAREA_onFunction("when(" + pair.first + ", inline = never), when(" + pair.second + ", inline = force), when(" + pair.second + ", inline = auto)") };

        EXPECT_EQ(MAREA_codesWith(PROGRAM, LINUX), Codes{ CONFLICTING }) << "The predicate '" << pair.second << "' must hold twice.";

        const std::string FALSE_TWICE{ MAREA_onFunction("when(" + pair.first + ", inline = never), when(" + pair.first + ", inline = force)") };

        EXPECT_EQ(MAREA_codesWith(FALSE_TWICE, LINUX), Codes{}) << "The predicate '" << pair.first << "' must not hold.";
    }
}

TEST(MAREASemaWhen, TheCombinatorsAreEvaluated)
{
    GTEST_COUT << "Validating all, any and not give the boolean they stand for on the target." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    const auto HOLDS{ [&LINUX](const std::string &predicate) {
        return MAREA_codesWith(MAREA_onFunction("when(" + predicate + ", inline = never), when(" + predicate + ", inline = force)"), LINUX) == Codes{ CONFLICTING };
    } };

    EXPECT_TRUE(HOLDS("all(architecture = \"x86_64\", feature = \"sse2\")"));
    EXPECT_FALSE(HOLDS("all(architecture = \"x86_64\", feature = \"neon\")"));
    EXPECT_TRUE(HOLDS("any(architecture = \"arm\", feature = \"sse2\")"));
    EXPECT_FALSE(HOLDS("any(architecture = \"arm\", feature = \"neon\")"));
    EXPECT_TRUE(HOLDS("not(feature = \"neon\")"));
    EXPECT_FALSE(HOLDS("not(feature = \"sse2\")"));
    EXPECT_TRUE(HOLDS("all(not(architecture = \"arm\"), any(compiler = \"clang\", compiler = \"gcc\"))"));
}

TEST(MAREASemaWhen, AWhenOnTheFileIsAccepted)
{
    GTEST_COUT << "Validating a when item is allowed wherever an attribute list is, the file list included." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    EXPECT_EQ(MAREA_codesWith(MAREA_onFile("when(operating_system = \"linux\", visibility = public)"), LINUX), Codes{});
    EXPECT_EQ(MAREA_codesWith(MAREA_onFile("when(operating_system = \"linux\", align = 16)"), LINUX), Codes{ MISPLACED });
}

TEST(MAREASemaBackend, AnyBackendMemberIsAKnownValue)
{
    GTEST_COUT << "Validating the four members of the backend key and a wrong member in marea.math." << std::endl;

    const MAREA_CallerInputs LINUX{ MAREA_CallerInputs::MAREA_linuxX86() };

    for (const char *member : { "auto", "scalar", "vector_extension", "intrinsics" }) {
        EXPECT_EQ(MAREA_codesWith(MAREA_math(std::string("backend = ") + member), LINUX), Codes{ RESERVED_ROOT }) << "The member " << member << " must be accepted on a target that gives it.";
    }

    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = fast"), LINUX), (Codes{ RESERVED_ROOT, INVALID_VALUE }));
}

TEST(MAREASemaBackend, TheBackendKeyIsAllowedOnlyInMareaMath)
{
    GTEST_COUT << "Validating backend in a file whose namespace is not marea.math or below raises ATTRIBUTE_NOT_ALLOWED_HERE, on the file and on a function." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_onFile("backend = auto")), Codes{ NOT_ALLOWED });
    EXPECT_EQ(MAREA_codesWith(MAREA_onFunction("backend = scalar")), Codes{ NOT_ALLOWED });
    EXPECT_EQ(MAREA_codesWith("namespace mathlike;\n#![backend = auto]\n"), Codes{ NOT_ALLOWED });
    EXPECT_EQ(MAREA_codesWith("namespace marea.mathx;\n#![backend = auto]\n"), (Codes{ RESERVED_ROOT, NOT_ALLOWED }));
    EXPECT_EQ(MAREA_codesWith("namespace marea;\n#![backend = auto]\n"), (Codes{ RESERVED_ROOT, NOT_ALLOWED }));
}

TEST(MAREASemaBackend, TheNamespacesBelowMareaMathAreAllowed)
{
    GTEST_COUT << "Validating a file below marea.math may write backend, and so may a function of it." << std::endl;

    EXPECT_EQ(MAREA_codesWith("namespace marea.math.vector;\n#![backend = scalar]\n"), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith("namespace marea.math;\n#[backend = scalar]\nfunction run()\nend\n"), (Codes{ RESERVED_ROOT }));
}

TEST(MAREASemaBackend, BackendStandsOnlyOnAFileOrAFunction)
{
    GTEST_COUT << "Validating backend on a structure of marea.math is MISPLACED_ATTRIBUTE by the matrix." << std::endl;

    EXPECT_EQ(MAREA_codesWith("namespace marea.math;\n#[backend = auto]\nstructure Vector\n    x: float32;\nend\n"), (Codes{ RESERVED_ROOT, MISPLACED }));
}

TEST(MAREASemaBackend, WithNoTargetAutoAndScalarAreTheOnlyAvailableBackends)
{
    GTEST_COUT << "Validating a host-neutral context: auto resolves to scalar, an explicit vector_extension or intrinsics is BACKEND_UNAVAILABLE." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = auto")), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = scalar")), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = vector_extension")), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics")), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
}

TEST(MAREASemaBackend, AnExplicitBackendNeedsWhatTheTargetGives)
{
    GTEST_COUT << "Validating the availability table: x86_64 with sse2 and aarch64 with neon give the vector backends, others do not." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("x86_64", { "sse2" })), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = vector_extension"), MAREA_target("x86_64", { "sse2" })), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("x86_64", {})), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("aarch64", { "neon" })), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = vector_extension"), MAREA_target("aarch64", {})), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("aarch64", {})), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("riscv64", { "sse2" })), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = vector_extension"), MAREA_target("riscv64", {})), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("x86_64", { "sse2" }, "msvc")), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = intrinsics"), MAREA_target("x86_64", { "sse2" }, "clang")), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(MAREA_math("backend = scalar"), MAREA_target("riscv64", {}, "msvc")), (Codes{ RESERVED_ROOT }));
}

TEST(MAREASemaBackend, ABackendThatAFunctionPinsIsCheckedToo)
{
    GTEST_COUT << "Validating the attribute of a function wins over the file one and is checked for availability on its own." << std::endl;

    const std::string PROGRAM{ "namespace marea.math;\n#![backend = scalar]\n#[backend = intrinsics]\nfunction run()\nend\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_target("riscv64", {})), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_target("x86_64", { "sse2" })), (Codes{ RESERVED_ROOT }));
}

TEST(MAREASemaBackend, ABackendInAWhenThatHoldsIsChecked)
{
    GTEST_COUT << "Validating the backend of a when item is judged for availability only when the predicate holds on the target." << std::endl;

    const std::string PROGRAM{ MAREA_math("when(architecture = \"riscv64\", backend = intrinsics)") };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_target("riscv64", {})), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_target("x86_64", { "sse2" })), (Codes{ RESERVED_ROOT }));
}

TEST(MAREASemaBackend, AWhenThatHoldsReplacesThePlainBackend)
{
    GTEST_COUT << "Validating the plain backend is replaced by a holding when, so the unavailable plain value is not the one judged." << std::endl;

    const std::string PROGRAM{ "namespace marea.math;\n#![backend = intrinsics, when(architecture = \"riscv64\", backend = scalar)]\n" };

    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_target("riscv64", {})), (Codes{ RESERVED_ROOT }));
    EXPECT_EQ(MAREA_codesWith(PROGRAM, MAREA_target("arm", {})), (Codes{ RESERVED_ROOT, UNAVAILABLE }));
}

TEST(MAREASemaBackend, ABackendInAWhenOutsideMareaMathIsNotAllowed)
{
    GTEST_COUT << "Validating the namespace limit of backend holds inside a when item too." << std::endl;

    EXPECT_EQ(MAREA_codesWith(MAREA_onFile("when(architecture = \"x86_64\", backend = scalar)"), MAREA_CallerInputs::MAREA_linuxX86()), Codes{ NOT_ALLOWED });
}

TEST(MAREASemaBackend, TheMessageNamesTheBackendAndTheTarget)
{
    GTEST_COUT << "Validating the BACKEND_UNAVAILABLE message names the backend and the architecture and the compiler of the target." << std::endl;

    MAREA_CallerRun run(MAREA_math("backend = intrinsics"), MAREA_target("riscv64", {}));

    const std::vector<std::string> MESSAGES{ run.MAREA_messages() };

    ASSERT_EQ(MESSAGES.size(), 2U) << "The project rule and the backend rule each raise one diagnostic.";
    EXPECT_NE(MESSAGES[1].find("intrinsics"), std::string::npos) << "The message must name the backend.";
    EXPECT_NE(MESSAGES[1].find("riscv64"), std::string::npos) << "The message must name the architecture.";
    EXPECT_NE(MESSAGES[1].find("gcc"), std::string::npos) << "The message must name the compiler.";
}
