// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREACatalogueHelpers.hpp"
#include "tests/MAREAParityHelpers.hpp"

#include <algorithm>
#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace
{
    // INFO: an accept item that has a result and does not run on both backends is listed with the reason. Each reason
    // says the diagnostic of the vm and whether cgen lowers the item; both are checked, so a stale line fails.
    struct MAREA_Exclusion
    {
        std::string name;
        MAREA_Uinteger32 vmCode;
        bool cgenLowers;
        std::vector<std::string> items;
    };

    const std::vector<MAREA_Exclusion> &MAREA_exclusions()
    {
        static const std::vector<MAREA_Exclusion> EXCLUSIONS{
            { "the vm takes and returns no structure between functions of the unit", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, true, { "L-EXAMPLES-016", "L-FUNCTIONS-007", "L-FUNCTIONS-013", "L-FUNCTIONS-014", "L-FUNCTIONS-015", "L-MEMORY-001", "L-MEMORY-002", "L-MEMORY-003", "L-MEMORY-005", "L-MEMORY-015", "L-MEMORY-024", "L-MEMORY-027", "L-MEMORY-033", "L-OWNERSHIP-001", "L-OWNERSHIP-003", "L-OWNERSHIP-005", "L-OWNERSHIP-006", "L-OWNERSHIP-008", "L-OWNERSHIP-012", "L-OWNERSHIP-016", "L-OWNERSHIP-018", "L-OWNERSHIP-020", "L-OWNERSHIP-028", "L-OWNERSHIP-037", "L-OWNERSHIP-040", "L-OWNERSHIP-043", "L-OWNERSHIP-045", "L-OWNERSHIP-049", "L-OWNERSHIP-052", "L-OWNERSHIP-067", "L-OWNERSHIP-070", "L-OWNERSHIP-075", "L-OWNERSHIP-078", "L-OWNERSHIP-090", "L-OWNERSHIP-104", "L-OWNERSHIP-114", "L-OWNERSHIP-118", "L-OWNERSHIP-120", "L-OWNERSHIP-122", "L-OWNERSHIP-124", "L-OWNERSHIP-130", "L-OWNERSHIP-132", "L-OWNERSHIP-134", "L-OWNERSHIP-135", "L-OWNERSHIP-137", "L-TYPES-001", "L-TYPES-002", "L-TYPES-003", "L-TYPES-005", "L-TYPES-006", "L-TYPES-075", "L-TYPES-076", "L-TYPES-077", "L-TYPES-078", "L-TYPES-081" } },
            { "the vm holds no enum value", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, true, { "L-NAMESPACES-064", "L-OWNERSHIP-027", "L-TYPES-032", "L-TYPES-036", "L-TYPES-037", "L-TYPES-038", "L-TYPES-039", "L-TYPES-041", "L-TYPES-042" } },
            { "the vm runs no construct and no destroy operator", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, true, { "L-MEMORY-010", "L-MEMORY-011", "L-MEMORY-012", "L-MEMORY-013", "L-MEMORY-014", "L-MEMORY-016" } },
            { "the vm reads no constant of a structure type", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, true, { "L-OWNERSHIP-024" } },
            { "an operator or a method of a library array, with no library yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, false, { "L-EXAMPLES-003" } },
            { "size_of, which neither backend lowers yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_CONSTRUCT, false, { "L-ARRAY-010", "L-VECTOR-013", "L-TUPLE-008" } },
            { "a user operator that the ir lowers to no call, and a C function that no signature types", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-EXAMPLES-019" } },
            { "a with block, which neither backend writes yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-OWNERSHIP-022", "L-OWNERSHIP-087", "L-OWNERSHIP-099", "L-OWNERSHIP-147" } },
            { "a library type that no registry describes as an array, with no library yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-EXAMPLES-017", "L-EXAMPLES-018", "L-EXAMPLES-020", "L-INTEROP-005", "L-NAMESPACES-006", "L-OWNERSHIP-030", "L-OWNERSHIP-034", "L-OWNERSHIP-047", "L-OWNERSHIP-080", "L-OWNERSHIP-081", "L-OWNERSHIP-083", "L-OWNERSHIP-139", "L-OWNERSHIP-140", "L-OWNERSHIP-141", "L-OWNERSHIP-143", "L-OWNERSHIP-145", "L-PARALLEL-001", "L-PARALLEL-002", "L-PARALLEL-003", "L-PARALLEL-004", "L-PARALLEL-005", "L-PARALLEL-006", "L-PARALLEL-008", "L-PARALLEL-009", "L-PARALLEL-010", "L-PARALLEL-011", "L-PARALLEL-013", "L-PARALLEL-014", "L-PARALLEL-015", "L-PARALLEL-016", "L-PARALLEL-017", "L-PARALLEL-018", "L-PARALLEL-019", "L-PARALLEL-020", "L-PARALLEL-079", "L-TYPES-129" } },
            { "a string, which neither backend holds yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-BINDINGS-023", "L-BINDINGS-076", "L-EXAMPLES-006", "L-EXAMPLES-007", "L-EXAMPLES-009", "L-EXAMPLES-011", "L-EXAMPLES-012", "L-EXAMPLES-014", "L-EXAMPLES-015", "L-INTEROP-016", "L-NAMESPACES-013" } },
            { "a character, which neither backend holds yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-BINDINGS-024", "L-TYPES-064" } },
            { "a function type value, which neither backend holds yet", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-FUNCTIONS-004", "L-FUNCTIONS-016" } },
            { "a generic structure, which the ir does not instantiate", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNSUPPORTED_TYPE, false, { "L-TYPES-011" } },
            { "a call of a C function that no signature types and no host registers", MAREA_BYTECODE_DIAGNOSTIC_CODE_UNTYPED_OPERAND, false, { "L-INTEROP-001", "L-INTEROP-008", "L-NAMESPACES-009", "L-NAMESPACES-010" } },
        };

        return EXCLUSIONS;
    }

    const MAREA_Exclusion *MAREA_findExclusion(const std::string &item)
    {
        for (const MAREA_Exclusion &exclusion : MAREA_exclusions()) {
            if (std::find(exclusion.items.begin(), exclusion.items.end(), item) != exclusion.items.end()) {
                return &exclusion;
            }
        }

        return nullptr;
    }

    bool MAREA_isContainerItem(const std::string &relativePath)
    {
        return (relativePath.rfind("accept/array/", 0) == 0) || (relativePath.rfind("accept/vector/", 0) == 0) || (relativePath.rfind("accept/tuple/", 0) == 0);
    }

    bool MAREA_hasResultFunction(const MAREA_IrNode *module)
    {
        for (MAREA_IntegerSize index{ 0 }; index < MAREA_IrNodeGetChildCount(module); ++index) {
            const MAREA_IrNode *child{ MAREA_irChild(module, index) };

            if ((MAREA_IrNodeGetKind(child) == MAREA_IR_KIND_FUNCTION_DECLARATION) && !MAREA_parityResolvedType(child).empty()) {
                return true;
            }
        }

        return false;
    }

    // INFO: the compile and the run of one item, made on a thread: calls is empty when the C text is only compiled.
    struct MAREA_ParityJob
    {
        std::string item;
        std::string cText;
        std::vector<MAREA_ParityCall> calls;
        std::vector<MAREA_ParityOutcome> vm;
        std::string compileError;
        MAREA_ParityCRun run;
    };

    void MAREA_runJobs(std::vector<MAREA_ParityJob> &jobs)
    {
        MAREA_IGNORE(MAREA_parityCompilers());

        std::atomic<std::size_t> next{ 0 };

        const auto WORK{ [&jobs, &next] {
            for (std::size_t index{ next++ }; index < jobs.size(); index = next++) {
                if (jobs[index].calls.empty()) {
                    jobs[index].compileError = MAREA_parityCompileOnly(jobs[index].cText);
                } else {
                    jobs[index].run = MAREA_parityRunC(jobs[index].cText, jobs[index].calls);
                }
            }
        } };

        const unsigned COUNT{ std::clamp(std::thread::hardware_concurrency(), 1U, 8U) };

        std::vector<std::thread> workers;

        for (unsigned index{ 0 }; index < COUNT; ++index) {
            workers.emplace_back(WORK);
        }

        for (std::thread &worker : workers) {
            worker.join();
        }
    }
} // namespace

TEST(MAREALanguageParity, EveryAcceptItemWithAResultRunsOnBothBackendsOrIsExcluded)
{
    GTEST_COUT << "Validating each accept item that has a result gives the same answers in the vm and in the generated C." << std::endl;

    std::vector<MAREA_ParityJob> jobs;

    std::map<std::string, std::size_t> excludedByReason;

    std::size_t items{ 0 };
    std::size_t withResult{ 0 };
    std::size_t compared{ 0 };
    std::size_t excluded{ 0 };
    std::size_t calls{ 0 };

    std::string failures;

    std::set<std::string> seen;

    for (const MAREA_ItemFile &file : MAREA_collectItems()) {
        if (file.relativePath.rfind("accept/", 0) != 0) {
            continue;
        }

        ++items;

        const std::string ITEM{ std::filesystem::path(file.relativePath).stem().string() };

        MAREA_IrFixture fixture([](const std::string &text, const MAREA_AstNode **outRoot) { return MAREA_resolveLibrary(text, outRoot); });

        if (fixture.MAREA_lower(file.source) != MAREA_IR_EXCEPT_SUCCESS) {
            failures += ITEM + ": the item does not lower to the ir\n";

            continue;
        }

        // INFO: the ir does not lower a static_assert of the module, which sema has checked already.
        const std::vector<MAREA_Uinteger32> IR_CODES{ fixture.MAREA_irCodes() };

        if (MAREA_isContainerItem(file.relativePath) && std::any_of(IR_CODES.begin(), IR_CODES.end(), [](MAREA_Uinteger32 code) { return code != MAREA_IR_DIAGNOSTIC_CODE_UNSUPPORTED_MACRO; })) {
            failures += ITEM + ": the item raises an ir diagnostic\n";

            continue;
        }

        if (!MAREA_hasResultFunction(fixture.module)) {
            continue;
        }

        ++withResult;

        seen.insert(ITEM);

        const MAREA_ParityModules MODULES{ MAREA_parityGenerate(fixture, nullptr) };

        const MAREA_Exclusion *EXCLUSION{ MAREA_findExclusion(ITEM) };

        if (EXCLUSION != nullptr) {
            ++excluded;

            ++excludedByReason[EXCLUSION->name];

            if (MODULES.vmOk || (MODULES.vmCodes.count(EXCLUSION->vmCode) == 0)) {
                failures += ITEM + ": excluded as '" + EXCLUSION->name + "', but the vm gave " + (MODULES.vmOk ? std::string("a module") : MAREA_parityCodes(MODULES.vmCodes)) + "\n";
            }

            if (MODULES.cgenOk != EXCLUSION->cgenLowers) {
                failures += ITEM + ": excluded as '" + EXCLUSION->name + "', but cgen " + (MODULES.cgenOk ? std::string("lowers it") : "gave " + MAREA_parityCodes(MODULES.cgenCodes)) + "\n";
            }

            if (MODULES.cgenOk) {
                jobs.push_back({ ITEM, MODULES.cText, {}, {}, {}, {} });
            }

            continue;
        }

        if (!MODULES.vmOk || !MODULES.cgenOk) {
            failures += ITEM + ": not excluded, but the vm gave [" + MAREA_parityCodes(MODULES.vmCodes) + "] and cgen [" + MAREA_parityCodes(MODULES.cgenCodes) + "]\n";

            continue;
        }

        const std::vector<MAREA_ParityCall> CALLS{ MAREA_parityCalls(fixture.module) };

        // INFO: an item whose functions with a result all take a composite has no call to compare, so its C text is only compiled.
        if (CALLS.empty() && ((ITEM == "L-OWNERSHIP-010") || (ITEM == "L-OWNERSHIP-014") || (ITEM == "L-OWNERSHIP-031") || (ITEM == "L-OWNERSHIP-033") || (ITEM == "L-OWNERSHIP-085"))) {
            jobs.push_back({ ITEM, MODULES.cText, {}, {}, {}, {} });

            continue;
        }

        if (CALLS.empty()) {
            failures += ITEM + ": no function of the item takes only primitive values and returns one\n";

            continue;
        }

        ++compared;

        calls += CALLS.size();

        jobs.push_back({ ITEM, MODULES.cText, CALLS, MAREA_parityRunVm(fixture, MODULES.bytecode, CALLS, nullptr), {}, {} });
    }

    std::set<std::string> listed;

    for (const MAREA_Exclusion &exclusion : MAREA_exclusions()) {
        for (const std::string &item : exclusion.items) {
            if (!listed.insert(item).second) {
                failures += item + ": listed twice in the exclusion table\n";
            }

            if (seen.count(item) == 0) {
                failures += item + ": listed in the exclusion table, but it is no accept item with a result\n";
            }
        }
    }

    MAREA_runJobs(jobs);

    std::size_t compiledOnly{ 0 };

    for (const MAREA_ParityJob &job : jobs) {
        if (job.calls.empty()) {
            ++compiledOnly;

            if (!job.compileError.empty()) {
                failures += job.item + ": " + job.compileError + "\n";
            }

            continue;
        }

        if (!job.run.error.empty()) {
            failures += job.item + ": " + job.run.error + "\n";

            continue;
        }

        for (const std::string &difference : MAREA_parityDiffer(job.calls, job.vm, job.run.outcomes)) {
            failures += job.item + ": " + difference + "\n";
        }
    }

    GTEST_COUT << "Parity: " << items << " accept items, " << withResult << " with a function that has a result: " << compared << " compared on both backends (" << calls << " calls), " << excluded << " excluded, " << compiledOnly << " of them compiled from cgen alone." << std::endl;

    for (const auto &reason : excludedByReason) {
        GTEST_COUT << "Excluded " << reason.second << ": " << reason.first << std::endl;
    }

    EXPECT_GT(compared, 0U) << "Some item must be compared.";

    EXPECT_TRUE(failures.empty()) << "The two backends must agree:\n"
                                  << failures;
}

TEST(MAREALanguageParity, ADifferenceBetweenTheBackendsIsReported)
{
    GTEST_COUT << "Validating the comparison fails for a C program that answers other than the vm, with a value or a trap." << std::endl;

    MAREA_IrFixture vmSide;
    MAREA_IrFixture generatedSide;

    ASSERT_EQ(vmSide.MAREA_lower("namespace main; function f(a: sinteger32): sinteger32 return 10 / a; end function g(): sinteger32 return 1; end"), MAREA_IR_EXCEPT_SUCCESS) << "The program of the vm must lower.";

    ASSERT_EQ(generatedSide.MAREA_lower("namespace main; function f(a: sinteger32): sinteger32 return a; end function g(): sinteger32 return 2; end"), MAREA_IR_EXCEPT_SUCCESS) << "The other program must lower.";

    const MAREA_ParityModules VM{ MAREA_parityGenerate(vmSide, nullptr) };

    const MAREA_ParityModules OTHER{ MAREA_parityGenerate(generatedSide, nullptr) };

    ASSERT_TRUE(VM.vmOk && OTHER.cgenOk) << "Both programs must generate.";

    const std::vector<MAREA_ParityCall> CALLS{ MAREA_parityCall("f", "sinteger32", { MAREA_parityInt(0) }), MAREA_parityCall("g", "sinteger32") };

    const MAREA_ParityCRun RUN{ MAREA_parityRunC(OTHER.cText, CALLS) };

    ASSERT_TRUE(RUN.error.empty()) << RUN.error;

    const std::vector<std::string> DIFFERENCES{ MAREA_parityDiffer(CALLS, MAREA_parityRunVm(vmSide, VM.bytecode, CALLS, nullptr), RUN.outcomes) };

    ASSERT_EQ(DIFFERENCES.size(), 2U) << "The trap of the first call and the value of the second differ.";

    EXPECT_NE(DIFFERENCES[0].find("f("), std::string::npos) << "The failure names the call.";

    EXPECT_NE(DIFFERENCES[1].find("g()"), std::string::npos) << "The failure names the call.";
}
