// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>

namespace
{
    struct MAREA_CoverageItem
    {
        std::string itemClass;
        std::string area;
        std::string id;
        std::string source;
    };

    struct MAREA_CodeExemption
    {
        int code;
        const char *reason;
    };

    struct MAREA_ExampleExemption
    {
        const char *path;
        const char *reason;
    };

    struct MAREA_MissingRule
    {
        const char *name;
        const char *issue;
        const char *how;
    };

    // INFO: a retired code keeps its number and is never reused. The header of its stage must not define it.
    const std::vector<MAREA_CodeExemption> RETIRED_CODES{
        { 3023, "the fixed main signature check was retired, the number stays reserved" },
        { 4001, "fork_join became a lowered construct in the ir" },
        { 4002, "switch became a lowered construct in the ir" },
        { 4003, "for each became a lowered construct in the ir" },
    };

    // INFO: a live code that no source program can trigger yet. The Gaps section of the language doc names each one.
    const std::vector<MAREA_CodeExemption> EXEMPT_CODES{
        { 4004, "the catalogue runner runs lex, parse and sema only, so no item reaches the ir; remove this entry when a runner makes the ir run" },
        { 4005, "the catalogue runner runs lex, parse and sema only, so no item reaches the ir; remove this entry when a runner makes the ir run" },
        { 4006, "the catalogue runner runs lex, parse and sema only, so no item reaches the ir; remove this entry when a runner makes the ir run" },
    };

    // INFO: an example that has no accept item. Each reason is checked by hand and is named in the Gaps section.
    const std::vector<MAREA_ExampleExemption> EXEMPT_EXAMPLES{
        { "examples/basics/ownership.marea", "writes a parameter with the default borrow, which settled rule 2 (read-only default) forbids (issues #203, #214)" },
        { "examples/interop/gpu_handles.marea", "writes a parameter with the default borrow, which settled rule 2 forbids (issues #203, #214)" },
        { "examples/parallel/scaling.marea", "writes a slice parameter with the default borrow, which settled rule 2 forbids (issues #203, #214)" },
        { "examples/types/methods.marea", "a method writes self with the default borrow, which settled rule 2 forbids (issues #203, #214)" },
        { "examples/macros/builtin.marea", "imports the project namespace attributes.layout, and an item is one file (issue #250)" },
        { "examples/main.marea", "imports eight project namespaces, and an item is one file (issue #250)" },
        { "examples/namespaces/consumer.marea", "imports the project namespace namespaces.library, and an item is one file (issue #250)" },
    };

    // INFO: the rules that the settled rules show as missing. Each one names where it is kept:
    // "gap" means a known_gap item with the issue exists, "reject" means the codes in how have a reject item, and
    // "none" means no item can show it today, so the Gaps section of the doc must name the issue.
    const std::vector<MAREA_MissingRule> MISSING_RULES{
        { "use of a moved value", "198", "gap" },
        { "use of an uninitialized value", "177", "gap" },
        { "write through a read-only borrow", "204", "gap" },
        { "literal out of range", "181", "gap" },
        { "constant division by zero", "183", "gap" },
        { "shift count too large", "184", "gap" },
        { "recursive type by value", "221", "gap" },
        { "cyclic alias", "221", "gap" },
        { "cyclic constant", "249", "gap" },
        { "overlapping implementation", "225", "gap" },
        { "unreachable switch arm", "0", "reject 3025 2010" },
        { "import cycle", "250", "none" },
        { "private access across namespaces", "250", "none" },
    };

    std::string MAREA_trim(const std::string &text)
    {
        const std::string SPACES{ " \t\r\n" };

        const std::size_t FIRST{ text.find_first_not_of(SPACES) };

        if (FIRST == std::string::npos) {
            return "";
        }

        return text.substr(FIRST, text.find_last_not_of(SPACES) - FIRST + 1);
    }

    std::vector<std::string> MAREA_lines(const std::string &text)
    {
        std::vector<std::string> result;

        std::istringstream stream(text);

        std::string line;

        while (std::getline(stream, line)) {
            result.push_back(MAREA_trim(line));
        }

        return result;
    }

    std::string MAREA_firstLine(const std::string &text)
    {
        const std::vector<std::string> LINES{ MAREA_lines(text) };

        return LINES.empty() ? "" : LINES[0];
    }

    std::string MAREA_readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    std::filesystem::path MAREA_root()
    {
        return std::filesystem::path(MAREA_TESTS_LANGUAGE_DIRECTORY).parent_path().parent_path();
    }

    std::vector<MAREA_CoverageItem> MAREA_collectItems()
    {
        std::vector<MAREA_CoverageItem> result;

        const std::filesystem::path ROOT{ MAREA_TESTS_LANGUAGE_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (!entry.is_regular_file() || (entry.path().extension() != ".marea")) {
                continue;
            }

            const std::filesystem::path RELATIVE{ std::filesystem::relative(entry.path(), ROOT) };

            result.push_back({ RELATIVE.begin()->string(), RELATIVE.parent_path().filename().string(), entry.path().stem().string(), MAREA_readFile(entry.path()) });
        }

        std::sort(result.begin(), result.end(), [](const MAREA_CoverageItem &left, const MAREA_CoverageItem &right) { return left.id < right.id; });

        return result;
    }

    std::vector<std::string> MAREA_collectExamples()
    {
        std::vector<std::string> result;

        const std::filesystem::path ROOT{ MAREA_TESTS_EXAMPLES_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".marea")) {
                result.push_back("examples/" + std::filesystem::relative(entry.path(), ROOT).generic_string());
            }
        }

        std::sort(result.begin(), result.end());

        return result;
    }

    // INFO: the table rows of the doc, one entry per line that starts with "| L-", id to the whole line.
    std::map<std::string, std::string> MAREA_documentRows(const std::string &document)
    {
        std::map<std::string, std::string> result;

        const std::regex ROW("^\\| (L-[A-Z0-9_]+-[0-9]{3}) \\|.*$");

        for (const std::string &line : MAREA_lines(document)) {
            std::smatch match;

            if (std::regex_match(line, match, ROW)) {
                result[match[1].str()] = line;
            }
        }

        return result;
    }

    // INFO: the text of one "## <title>" section, up to the next "## " heading.
    std::string MAREA_section(const std::string &document, const std::string &title)
    {
        std::string result;

        bool inside{ false };

        for (const std::string &line : MAREA_lines(document)) {
            if (line.compare(0, 3, "## ") == 0) {
                inside = (line == "## " + title);

                continue;
            }

            if (inside) {
                result += line + "\n";
            }
        }

        return result;
    }

    std::vector<std::string> MAREA_checkRowsAndFiles(const std::vector<MAREA_CoverageItem> &items, const std::string &document)
    {
        std::vector<std::string> failures;

        const std::map<std::string, std::string> ROWS{ MAREA_documentRows(document) };

        std::set<std::string> ids;

        for (const MAREA_CoverageItem &item : items) {
            ids.insert(item.id);

            if (ROWS.find(item.id) == ROWS.end()) {
                failures.push_back(item.id + ": the file has no row in the doc");
            }
        }

        for (const auto &row : ROWS) {
            if (ids.find(row.first) == ids.end()) {
                failures.push_back(row.first + ": the doc row has no file");
            }
        }

        return failures;
    }

    bool MAREA_hasExemption(const std::vector<MAREA_ExampleExemption> &exemptions, const std::string &path)
    {
        return std::any_of(exemptions.begin(), exemptions.end(), [&path](const MAREA_ExampleExemption &exemption) { return path == exemption.path; });
    }

    std::vector<std::string> MAREA_checkExamples(const std::vector<std::string> &examples, const std::vector<MAREA_CoverageItem> &items, const std::string &document, const std::vector<MAREA_ExampleExemption> &exemptions)
    {
        std::vector<std::string> failures;

        const std::map<std::string, std::string> ROWS{ MAREA_documentRows(document) };

        std::set<std::string> covered;

        for (const std::string &example : examples) {
            for (const MAREA_CoverageItem &item : items) {
                if (item.itemClass != "accept") {
                    continue;
                }

                const auto ROW{ ROWS.find(item.id) };

                const bool IN_ROW{ (ROW != ROWS.end()) && (ROW->second.find("`" + example + "`") != std::string::npos) };

                const std::vector<std::string> LINES{ MAREA_lines(item.source) };

                const bool IN_HEADER{ std::find(LINES.begin(), LINES.end(), "-- example: " + example) != LINES.end() };

                if (IN_ROW || IN_HEADER) {
                    covered.insert(example);
                }
            }
        }

        for (const std::string &example : examples) {
            const bool EXEMPT{ MAREA_hasExemption(exemptions, example) };

            if ((covered.find(example) == covered.end()) && !EXEMPT) {
                failures.push_back(example + ": no accept item names this example and it is not in the exemption list");
            }

            if ((covered.find(example) != covered.end()) && EXEMPT) {
                failures.push_back(example + ": the example has an accept item, remove it from the exemption list");
            }
        }

        for (const MAREA_ExampleExemption &exemption : exemptions) {
            if (std::find(examples.begin(), examples.end(), std::string(exemption.path)) == examples.end()) {
                failures.push_back(std::string(exemption.path) + ": the exemption names a file that does not exist");
            }
        }

        return failures;
    }

    std::map<int, std::string> MAREA_liveCodes(const std::vector<std::string> &headerTexts)
    {
        std::map<int, std::string> result;

        const std::regex CODE("MAREA_[A-Z]+_DIAGNOSTIC_CODE_([A-Z0-9_]+) = MAREA_UINTEGER_32\\(([0-9]{4})\\)");

        for (const std::string &text : headerTexts) {
            for (std::sregex_iterator iterator(text.begin(), text.end(), CODE), end; iterator != end; ++iterator) {
                result[std::stoi((*iterator)[2].str())] = (*iterator)[1].str();
            }
        }

        return result;
    }

    std::set<int> MAREA_codesOfHeaders(const std::vector<MAREA_CoverageItem> &items, const std::string &itemClass)
    {
        std::set<int> result;

        const std::regex NUMBER("[0-9]{4}");

        for (const MAREA_CoverageItem &item : items) {
            if (item.itemClass != itemClass) {
                continue;
            }

            const std::vector<std::string> LINES{ MAREA_lines(item.source) };

            std::string body{ LINES.empty() ? "" : LINES[0] };

            body = std::regex_replace(body, std::regex("#[0-9]+|@[0-9]+"), "");

            for (std::sregex_iterator iterator(body.begin(), body.end(), NUMBER), end; iterator != end; ++iterator) {
                result.insert(std::stoi(iterator->str()));
            }
        }

        return result;
    }

    std::vector<std::string> MAREA_checkCodes(const std::vector<std::string> &headerTexts, const std::vector<MAREA_CoverageItem> &items, const std::vector<MAREA_CodeExemption> &exemptions)
    {
        std::vector<std::string> failures;

        const std::map<int, std::string> LIVE{ MAREA_liveCodes(headerTexts) };

        const std::set<int> REJECTED{ MAREA_codesOfHeaders(items, "reject") };

        for (const auto &code : LIVE) {
            const bool EXEMPT{ std::any_of(exemptions.begin(), exemptions.end(), [&code](const MAREA_CodeExemption &exemption) { return exemption.code == code.first; }) };

            const bool HAS_REJECT{ REJECTED.find(code.first) != REJECTED.end() };

            if (!HAS_REJECT && !EXEMPT) {
                failures.push_back(std::to_string(code.first) + " " + code.second + ": no reject item names this code and it is not in the exemption list");
            }

            if (HAS_REJECT && EXEMPT) {
                failures.push_back(std::to_string(code.first) + " " + code.second + ": the code has a reject item, remove it from the exemption list");
            }
        }

        for (const MAREA_CodeExemption &exemption : exemptions) {
            if (LIVE.find(exemption.code) == LIVE.end()) {
                failures.push_back(std::to_string(exemption.code) + ": the exemption names a code that is not live");
            }
        }

        for (const MAREA_CodeExemption &retired : RETIRED_CODES) {
            if (LIVE.find(retired.code) != LIVE.end()) {
                failures.push_back(std::to_string(retired.code) + ": a retired code is defined in a header again");
            }

            if (REJECTED.find(retired.code) != REJECTED.end()) {
                failures.push_back(std::to_string(retired.code) + ": a reject item names a retired code");
            }
        }

        for (const int CODE : MAREA_codesOfHeaders(items, "known_gap")) {
            if (LIVE.find(CODE) == LIVE.end()) {
                failures.push_back(std::to_string(CODE) + ": a known_gap item names a code that does not exist");
            }
        }

        return failures;
    }

    std::vector<std::string> MAREA_checkMissingRules(const std::vector<MAREA_MissingRule> &rules, const std::vector<MAREA_CoverageItem> &items, const std::string &document)
    {
        std::vector<std::string> failures;

        const std::string GAPS{ MAREA_section(document, "Gaps") };

        const std::set<int> REJECTED{ MAREA_codesOfHeaders(items, "reject") };

        for (const MAREA_MissingRule &rule : rules) {
            const std::string HOW{ rule.how };

            if (HOW == "gap") {
                const std::string WANTED{ "-- gap: #" + std::string(rule.issue) + " " };

                const bool FOUND{ std::any_of(items.begin(), items.end(), [&WANTED](const MAREA_CoverageItem &item) { return (item.itemClass == "known_gap") && (MAREA_firstLine(item.source).compare(0, WANTED.size(), WANTED) == 0); }) };

                if (!FOUND) {
                    failures.push_back(std::string(rule.name) + ": no known_gap item has the issue #" + rule.issue);
                }
            } else if (HOW == "none") {
                if (GAPS.find("#" + std::string(rule.issue)) == std::string::npos) {
                    failures.push_back(std::string(rule.name) + ": the Gaps section does not name the issue #" + rule.issue);
                }
            } else {
                std::istringstream stream(HOW.substr(7));

                int code{ 0 };

                while (stream >> code) {
                    if (REJECTED.find(code) == REJECTED.end()) {
                        failures.push_back(std::string(rule.name) + ": no reject item names the code " + std::to_string(code));
                    }
                }
            }
        }

        return failures;
    }

    std::vector<std::string> MAREA_checkGapsNames(const std::vector<MAREA_CodeExemption> &codes, const std::vector<MAREA_ExampleExemption> &examples, const std::string &document)
    {
        std::vector<std::string> failures;

        const std::string GAPS{ MAREA_section(document, "Gaps") };

        for (const MAREA_CodeExemption &code : codes) {
            if (GAPS.find(std::to_string(code.code)) == std::string::npos) {
                failures.push_back(std::to_string(code.code) + ": the Gaps section does not name the exempt code");
            }
        }

        for (const MAREA_ExampleExemption &example : examples) {
            if (GAPS.find(example.path) == std::string::npos) {
                failures.push_back(std::string(example.path) + ": the Gaps section does not name the exempt example");
            }
        }

        return failures;
    }

    // INFO: the Counts table is "| area | accept | reject | known_gap |", one row per area and a "total" row.
    std::vector<std::string> MAREA_checkCounts(const std::vector<MAREA_CoverageItem> &items, const std::string &document)
    {
        std::vector<std::string> failures;

        std::map<std::string, std::map<std::string, int>> actual;

        for (const MAREA_CoverageItem &item : items) {
            ++actual[item.area][item.itemClass];

            ++actual["total"][item.itemClass];
        }

        const std::regex ROW("^\\| ([a-z_]+) \\| ([0-9]+) \\| ([0-9]+) \\| ([0-9]+) \\|$");

        std::set<std::string> seen;

        for (const std::string &line : MAREA_lines(MAREA_section(document, "Counts"))) {
            std::smatch match;

            if (!std::regex_match(line, match, ROW)) {
                continue;
            }

            const std::string AREA{ match[1].str() };

            seen.insert(AREA);

            const std::vector<std::string> CLASSES{ "accept", "reject", "known_gap" };

            for (std::size_t index{ 0 }; index < CLASSES.size(); ++index) {
                const int DOCUMENTED{ std::stoi(match[2 + index].str()) };

                const int ACTUAL{ actual[AREA][CLASSES[index]] };

                if (DOCUMENTED != ACTUAL) {
                    failures.push_back(AREA + " " + CLASSES[index] + ": the doc says " + std::to_string(DOCUMENTED) + ", the files are " + std::to_string(ACTUAL));
                }
            }
        }

        for (const auto &area : actual) {
            if (seen.find(area.first) == seen.end()) {
                failures.push_back(area.first + ": the Counts table has no row");
            }
        }

        return failures;
    }

    std::string MAREA_joinLines(const std::vector<std::string> &failures)
    {
        std::string result;

        for (const std::string &failure : failures) {
            result += failure + "\n";
        }

        return result;
    }

    std::vector<std::string> MAREA_readHeaders()
    {
        const std::filesystem::path INCLUDE{ MAREA_root() / "lib" / "include" / "marea" };

        return {
            MAREA_readFile(INCLUDE / "lex" / "MAREALexer.h"),
            MAREA_readFile(INCLUDE / "parse" / "MAREAParser.h"),
            MAREA_readFile(INCLUDE / "sema" / "MAREASema.h"),
            MAREA_readFile(INCLUDE / "ir" / "MAREAIr.h"),
        };
    }

    std::string MAREA_readDocument()
    {
        return MAREA_readFile(MAREA_root() / "docs" / "language" / "marea-language.md");
    }

    const std::string OK_BODY{ "-- Copyright\n\nnamespace main;\n" };

    const std::string SEED_DOCUMENT{ "## Gaps\n\nThe codes 4004 and #250 and examples/a.marea.\n\n## Counts\n\n| area | accept | reject | known_gap |\n|---|---|---|---|\n| seed | 1 | 1 | 1 |\n| total | 1 | 1 | 1 |\n" };

    const std::vector<MAREA_CoverageItem> SEED_ITEMS{
        { "accept", "seed", "L-SEED-001", "-- expect: ok\n-- example: examples/a.marea\n" + OK_BODY },
        { "reject", "seed", "L-SEED-002", "-- expect: 3004 @4\n" + OK_BODY },
        { "known_gap", "seed", "L-SEED-003", "-- gap: #12 3011\n-- rule: reject\n" + OK_BODY },
    };

    const std::string SEED_HEADER{ "MAREA_SEMA_DIAGNOSTIC_CODE_UNDECLARED_NAME = MAREA_UINTEGER_32(3004),\nMAREA_SEMA_DIAGNOSTIC_CODE_TYPE_MISMATCH = MAREA_UINTEGER_32(3011),\n" };

    bool MAREA_named(const std::vector<std::string> &failures, const std::string &name)
    {
        return !failures.empty() && std::all_of(failures.begin(), failures.end(), [&name](const std::string &failure) { return failure.compare(0, name.size(), name) == 0; });
    }
} // namespace

TEST(MAREALanguageCoverage, EveryExampleHasAnAcceptItemOrAnExemption)
{
    GTEST_COUT << "Validating that every examples/**/*.marea file has an accept item or a named exemption." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkExamples(MAREA_collectExamples(), MAREA_collectItems(), MAREA_readDocument(), EXEMPT_EXAMPLES) };

    EXPECT_TRUE(FAILURES.empty()) << "Every example must be covered:\n"
                                  << MAREA_joinLines(FAILURES);
}

TEST(MAREALanguageCoverage, EveryDocumentRowHasAFileAndEveryFileHasARow)
{
    GTEST_COUT << "Validating that the doc rows and the item files match one to one." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkRowsAndFiles(MAREA_collectItems(), MAREA_readDocument()) };

    EXPECT_TRUE(FAILURES.empty()) << "Rows and files must match:\n"
                                  << MAREA_joinLines(FAILURES);
}

TEST(MAREALanguageCoverage, EveryLiveDiagnosticCodeHasARejectItemOrAnExemption)
{
    GTEST_COUT << "Validating that every live diagnostic code has a reject item or a named exemption." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkCodes(MAREA_readHeaders(), MAREA_collectItems(), EXEMPT_CODES) };

    EXPECT_TRUE(FAILURES.empty()) << "Every live code must be covered:\n"
                                  << MAREA_joinLines(FAILURES);
}

TEST(MAREALanguageCoverage, MissingRulesAreKeptAsGapsOrNamedInTheGapsSection)
{
    GTEST_COUT << "Validating that each rule the language still lacks has its known_gap item, reject codes or a named issue." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkMissingRules(MISSING_RULES, MAREA_collectItems(), MAREA_readDocument()) };

    EXPECT_TRUE(FAILURES.empty()) << "Every missing rule must be kept:\n"
                                  << MAREA_joinLines(FAILURES);
}

TEST(MAREALanguageCoverage, GapsSectionNamesEveryExemption)
{
    GTEST_COUT << "Validating that the Gaps section names every exempt code and every exempt example." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkGapsNames(EXEMPT_CODES, EXEMPT_EXAMPLES, MAREA_readDocument()) };

    EXPECT_TRUE(FAILURES.empty()) << "The Gaps section must name every exemption:\n"
                                  << MAREA_joinLines(FAILURES);
}

TEST(MAREALanguageCoverage, CountsTableEqualsTheFilesOnDisk)
{
    GTEST_COUT << "Validating that the Counts table of the doc equals the item files per area and class." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkCounts(MAREA_collectItems(), MAREA_readDocument()) };

    EXPECT_TRUE(FAILURES.empty()) << "The counts must match:\n"
                                  << MAREA_joinLines(FAILURES);
}

TEST(MAREALanguageCoverage, ExemptionsAreUnique)
{
    GTEST_COUT << "Validating that the exemption lists hold no repeated entry." << std::endl;

    std::set<int> codes;

    for (const MAREA_CodeExemption &exemption : EXEMPT_CODES) {
        EXPECT_TRUE(codes.insert(exemption.code).second) << "Repeated code " << exemption.code;
    }

    std::set<std::string> paths;

    for (const MAREA_ExampleExemption &exemption : EXEMPT_EXAMPLES) {
        EXPECT_TRUE(paths.insert(exemption.path).second) << "Repeated example " << exemption.path;
    }
}

TEST(MAREALanguageCoverage, ExampleCheckFailsForAnUncoveredAndAStaleExample)
{
    GTEST_COUT << "Validating the example check fails for a missing accept item and for a stale exemption." << std::endl;

    EXPECT_TRUE(MAREA_checkExamples({ "examples/a.marea" }, SEED_ITEMS, SEED_DOCUMENT, {}).empty()) << "A header that names the example must cover it.";

    const std::string ROW_DOCUMENT{ "| L-SEED-001 | the example `examples/b.marea` | ok |\n" };

    const std::vector<MAREA_CoverageItem> NO_HEADER{ { "accept", "seed", "L-SEED-001", "-- expect: ok\n" + OK_BODY } };

    EXPECT_TRUE(MAREA_checkExamples({ "examples/b.marea" }, NO_HEADER, ROW_DOCUMENT, {}).empty()) << "A doc row that names the example must cover it.";

    EXPECT_TRUE(MAREA_named(MAREA_checkExamples({ "examples/c.marea" }, SEED_ITEMS, SEED_DOCUMENT, {}), "examples/c.marea")) << "An example with no item must fail.";

    EXPECT_TRUE(MAREA_named(MAREA_checkExamples({ "examples/a.marea" }, SEED_ITEMS, SEED_DOCUMENT, { { "examples/a.marea", "stale" } }), "examples/a.marea")) << "An exemption of a covered example must fail.";

    EXPECT_TRUE(MAREA_named(MAREA_checkExamples({ "examples/a.marea" }, SEED_ITEMS, SEED_DOCUMENT, { { "examples/z.marea", "missing" } }), "examples/z.marea")) << "An exemption of a missing file must fail.";

    const std::vector<MAREA_CoverageItem> REJECT_ONLY{ { "reject", "seed", "L-SEED-002", "-- expect: 3004 @4\n-- example: examples/a.marea\n" + OK_BODY } };

    EXPECT_FALSE(MAREA_checkExamples({ "examples/a.marea" }, REJECT_ONLY, SEED_DOCUMENT, {}).empty()) << "A reject item does not cover an example.";
}

TEST(MAREALanguageCoverage, RowCheckFailsForARowWithNoFileAndAFileWithNoRow)
{
    GTEST_COUT << "Validating the row check fails in both directions." << std::endl;

    const std::string DOCUMENT{ "| L-SEED-001 | a | ok |\n| L-SEED-002 | b | 3004 @4 |\n| L-SEED-003 | c | gap |\n" };

    EXPECT_TRUE(MAREA_checkRowsAndFiles(SEED_ITEMS, DOCUMENT).empty()) << "Matching rows and files must pass.";

    EXPECT_TRUE(MAREA_named(MAREA_checkRowsAndFiles(SEED_ITEMS, "| L-SEED-001 | a | ok |\n| L-SEED-002 | b | x |\n"), "L-SEED-003")) << "A file with no row must fail.";

    EXPECT_TRUE(MAREA_named(MAREA_checkRowsAndFiles(SEED_ITEMS, DOCUMENT + "| L-SEED-009 | d | ok |\n"), "L-SEED-009")) << "A row with no file must fail.";

    EXPECT_TRUE(MAREA_named(MAREA_checkRowsAndFiles(SEED_ITEMS, "| L-SEED-001 | a | ok |\n| L-SEED-002 | b | x |\n| L-SEED-0030 | c | gap |\n"), "L-SEED-003")) << "A renamed row must fail.";
}

TEST(MAREALanguageCoverage, CodeCheckFailsForACodeWithNoRejectItemAndAGapCodeThatDoesNotExist)
{
    GTEST_COUT << "Validating the code check fails for a live code with no reject item and for an unknown gap code." << std::endl;

    EXPECT_TRUE(MAREA_checkCodes({ SEED_HEADER }, SEED_ITEMS, { { 3011, "no source program reaches it" } }).empty()) << "A reject item or an exemption must cover each code.";

    EXPECT_TRUE(MAREA_named(MAREA_checkCodes({ SEED_HEADER }, SEED_ITEMS, {}), "3011")) << "A live code with no reject item and no exemption must fail.";

    EXPECT_FALSE(MAREA_checkCodes({ SEED_HEADER }, SEED_ITEMS, { { 3004, "stale" }, { 3011, "x" } }).empty()) << "An exemption of a code that has a reject item must fail.";

    const std::vector<MAREA_CoverageItem> BAD_GAP{ SEED_ITEMS[1], { "known_gap", "seed", "L-SEED-003", "-- gap: #12 3999\n-- rule: reject\n" + OK_BODY } };

    EXPECT_TRUE(MAREA_named(MAREA_checkCodes({ SEED_HEADER }, BAD_GAP, { { 3011, "x" } }), "3999")) << "A known_gap code that does not exist must fail.";

    EXPECT_FALSE(MAREA_checkCodes({ SEED_HEADER + "MAREA_SEMA_DIAGNOSTIC_CODE_OLD = MAREA_UINTEGER_32(3023),\n" }, SEED_ITEMS, { { 3011, "x" } }).empty()) << "A retired code defined again must fail.";
}

TEST(MAREALanguageCoverage, MissingRuleCheckAndGapsNamesAndCountsFail)
{
    GTEST_COUT << "Validating the missing rule check, the Gaps names check and the counts check fail when they must." << std::endl;

    EXPECT_TRUE(MAREA_checkMissingRules({ { "a", "12", "gap" }, { "b", "250", "none" }, { "c", "0", "reject 3004" } }, SEED_ITEMS, SEED_DOCUMENT).empty()) << "Kept rules must pass.";

    EXPECT_TRUE(MAREA_named(MAREA_checkMissingRules({ { "a", "13", "gap" } }, SEED_ITEMS, SEED_DOCUMENT), "a")) << "A gap rule with no item must fail.";

    EXPECT_TRUE(MAREA_named(MAREA_checkMissingRules({ { "b", "251", "none" } }, SEED_ITEMS, SEED_DOCUMENT), "b")) << "A rule that the Gaps section does not name must fail.";

    EXPECT_TRUE(MAREA_named(MAREA_checkMissingRules({ { "c", "0", "reject 3025" } }, SEED_ITEMS, SEED_DOCUMENT), "c")) << "A reject code with no item must fail.";

    EXPECT_TRUE(MAREA_checkGapsNames({ { 4004, "x" } }, { { "examples/a.marea", "x" } }, SEED_DOCUMENT).empty()) << "Names in the Gaps section must pass.";

    EXPECT_EQ(2U, MAREA_checkGapsNames({ { 4005, "x" } }, { { "examples/q.marea", "x" } }, SEED_DOCUMENT).size()) << "A name that the Gaps section lacks must fail.";

    EXPECT_TRUE(MAREA_checkCounts(SEED_ITEMS, SEED_DOCUMENT).empty()) << "Matching counts must pass.";

    EXPECT_FALSE(MAREA_checkCounts(SEED_ITEMS, "## Counts\n\n| seed | 2 | 1 | 1 |\n| total | 1 | 1 | 1 |\n").empty()) << "A wrong count must fail.";

    EXPECT_FALSE(MAREA_checkCounts(SEED_ITEMS, "## Counts\n\n| total | 1 | 1 | 1 |\n").empty()) << "An area with no row must fail.";
}
