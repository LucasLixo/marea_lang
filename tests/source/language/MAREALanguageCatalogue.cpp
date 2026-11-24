// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "tests/dependencies.hpp"

#include "tests/MAREASemaHelpers.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>

namespace
{
    enum class MAREA_HeaderKind
    {
        OK,
        REJECT,
        GAP
    };

    struct MAREA_Expectation
    {
        MAREA_Uinteger32 code;
        MAREA_Uinteger32 line;
    };

    struct MAREA_Header
    {
        MAREA_HeaderKind kind{ MAREA_HeaderKind::OK };
        std::vector<MAREA_Expectation> expectations;
        std::string issue;
        std::string gapCode;
        std::string rule;
        std::string error;
    };

    struct MAREA_ItemFile
    {
        std::string relativePath;
        std::string source;
    };

    struct MAREA_RunResult
    {
        MAREA_SemaExcept except{ MAREA_SEMA_EXCEPT_SUCCESS };
        MAREA_IntegerSize errorCount{ 0 };
        std::vector<MAREA_Uinteger32> codes;
        std::vector<MAREA_Uinteger32> lines;
    };

    const std::vector<std::string> CLASSES{ "accept", "reject", "known_gap" };

    std::string MAREA_trim(const std::string &text)
    {
        const std::string SPACES{ " \t\r\n" };

        const std::size_t FIRST{ text.find_first_not_of(SPACES) };

        if (FIRST == std::string::npos) {
            return "";
        }

        return text.substr(FIRST, text.find_last_not_of(SPACES) - FIRST + 1);
    }

    std::vector<std::string> MAREA_splitLines(const std::string &text)
    {
        std::vector<std::string> result;

        std::istringstream stream(text);

        std::string line;

        while (std::getline(stream, line)) {
            result.push_back(MAREA_trim(line));
        }

        return result;
    }

    std::vector<std::string> MAREA_split(const std::string &text, char separator)
    {
        std::vector<std::string> result;

        std::istringstream stream(text);

        std::string part;

        while (std::getline(stream, part, separator)) {
            result.push_back(part);
        }

        return result;
    }

    bool MAREA_isKnownCode(MAREA_Uinteger32 code)
    {
        if ((code >= MAREA_LEX_DIAGNOSTIC_CODE_UNKNOWN_BYTE) && (code <= MAREA_LEX_DIAGNOSTIC_CODE_TOO_MANY_CHARACTERS)) {
            return true;
        }

        if ((code >= MAREA_PARSE_DIAGNOSTIC_CODE_EXPECTED_TOKEN) && (code <= MAREA_PARSE_DIAGNOSTIC_CODE_DEFAULT_ARM_NOT_LAST)) {
            return true;
        }

        if ((code == MAREA_SEMA_DIAGNOSTIC_CODE_NEGATE_UNSIGNED) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_SIGNED_DIVISION_OVERFLOW) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_BITWISE_OPERAND_NOT_INTEGER) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_LOGICAL_OPERAND_NOT_BOOLEAN) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_RAW_POINTER_OUTSIDE_UNSAFE) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_ASSIGNMENT_TO_CONSTANT) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_NON_BOOLEAN_CONDITION) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_CYCLIC_CONSTANT) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_LOCAL_WITHOUT_INITIAL_VALUE) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_UNKNOWN_MACRO) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_INVALID_MACRO_ARGUMENT) || (code == MAREA_SEMA_DIAGNOSTIC_CODE_COMPILE_TIME_ASSERTION_FAILED)) {
            return true;
        }

        return (code >= MAREA_SEMA_DIAGNOSTIC_CODE_UNRESOLVED_IMPORT) && (code <= MAREA_SEMA_DIAGNOSTIC_CODE_SHIFT_COUNT_TOO_LARGE) && (code != 3023);
    }

    MAREA_Header MAREA_parseHeader(const std::string &source)
    {
        MAREA_Header header;

        const std::vector<std::string> LINES{ MAREA_splitLines(source) };

        if (LINES.empty()) {
            header.error = "the file is empty, the first line must be '-- expect: ...' or '-- gap: ...'";

            return header;
        }

        const std::string &first{ LINES[0] };

        std::smatch match;

        if (first.compare(0, 11, "-- expect: ") == 0) {
            const std::string BODY{ MAREA_trim(first.substr(11)) };

            if (BODY == "ok") {
                header.kind = MAREA_HeaderKind::OK;
            } else {
                header.kind = MAREA_HeaderKind::REJECT;

                const std::regex ENTRY("^([0-9]{4})(?:[ ]*@([0-9]+))?$");

                for (const std::string &part : MAREA_split(BODY, ',')) {
                    const std::string ENTRY_TEXT{ MAREA_trim(part) };

                    if (!std::regex_match(ENTRY_TEXT, match, ENTRY)) {
                        header.error = "malformed expect entry '" + ENTRY_TEXT + "', the form is '<code> [@<line>]'";

                        return header;
                    }

                    const auto CODE{ static_cast<MAREA_Uinteger32>(std::stoul(match[1].str())) };

                    if (!MAREA_isKnownCode(CODE)) {
                        header.error = "unknown diagnostic code " + match[1].str();

                        return header;
                    }

                    header.expectations.push_back({ CODE, match[2].matched ? static_cast<MAREA_Uinteger32>(std::stoul(match[2].str())) : 0U });
                }
            }
        } else if (first.compare(0, 8, "-- gap: ") == 0) {
            header.kind = MAREA_HeaderKind::GAP;

            const std::regex GAP("^-- gap: #([0-9]+) ([0-9]{4}|NEW)$");

            if (!std::regex_match(first, match, GAP)) {
                header.error = "malformed gap line, the form is '-- gap: #<issue> <code or NEW>'";

                return header;
            }

            header.issue = match[1].str();

            header.gapCode = match[2].str();

            if ((header.issue == "0") || ((header.gapCode != "NEW") && !MAREA_isKnownCode(static_cast<MAREA_Uinteger32>(std::stoul(header.gapCode))))) {
                header.error = "the gap line needs an issue number above zero and a known code or NEW";

                return header;
            }
        } else {
            header.error = "the first line must be '-- expect: ok', '-- expect: <code> [@<line>]' or '-- gap: #<issue> <code or NEW>'";

            return header;
        }

        const std::string SECOND{ LINES.size() > 1 ? LINES[1] : "" };

        if (SECOND.compare(0, 8, "-- rule:") == 0) {
            if (header.kind != MAREA_HeaderKind::GAP) {
                header.error = "a '-- rule:' line belongs to a known_gap item only";

                return header;
            }

            if ((SECOND != "-- rule: reject") && (SECOND != "-- rule: accept")) {
                header.error = "malformed rule line, the form is '-- rule: reject' or '-- rule: accept'";

                return header;
            }

            header.rule = SECOND.substr(9);
        } else if (header.kind == MAREA_HeaderKind::GAP) {
            header.error = "a known_gap item needs '-- rule: reject' or '-- rule: accept' as its second line";
        }

        return header;
    }

    MAREA_HeaderKind MAREA_kindOfClass(const std::string &itemClass)
    {
        if (itemClass == "accept") {
            return MAREA_HeaderKind::OK;
        }

        return (itemClass == "reject") ? MAREA_HeaderKind::REJECT : MAREA_HeaderKind::GAP;
    }

    std::string MAREA_describe(const std::vector<MAREA_Uinteger32> &codes, const std::vector<MAREA_Uinteger32> &lines)
    {
        std::string result;

        for (std::size_t index{ 0 }; index < codes.size(); ++index) {
            result += (result.empty() ? "" : ", ") + std::to_string(codes[index]) + " @" + std::to_string(lines[index]);
        }

        return result.empty() ? "none" : result;
    }

    MAREA_SemaImportResult MAREA_resolveLibrary(const std::string &text)
    {
        const auto STARTS{ [&text](const std::string &prefix) { return text.compare(0, prefix.size(), prefix) == 0; } };

        const bool EXTERNAL{ STARTS("collections.") || STARTS("concurrency.") || STARTS("arenas.") || (text == "text") || STARTS("c.") };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    }

    MAREA_RunResult MAREA_run(const std::string &source)
    {
        MAREA_RunResult result;

        MAREA_SemaFixture fixture([](const std::string &text, const MAREA_AstNode **) { return MAREA_resolveLibrary(text); });

        result.except = fixture.MAREA_sema(source, "main.marea");

        result.errorCount = fixture.MAREA_errorCount();

        const MAREA_IntegerSize COUNT{ MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink) };

        for (MAREA_IntegerSize index{ 0 }; index < COUNT; ++index) {
            const MAREA_Diagnostic *diagnostic{ nullptr };

            MAREA_IGNORE(MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, index, &diagnostic));

            result.codes.push_back(MAREA_DiagnosticGetCode(diagnostic));

            result.lines.push_back(MAREA_DiagnosticGetPosition(diagnostic).line);
        }

        return result;
    }

    std::vector<std::string> MAREA_checkItem(const std::string &itemClass, const MAREA_ItemFile &item)
    {
        std::vector<std::string> failures;

        const std::string NAME{ item.relativePath };

        const MAREA_Header HEADER{ MAREA_parseHeader(item.source) };

        if (!HEADER.error.empty()) {
            failures.push_back(NAME + ": bad header: " + HEADER.error);

            return failures;
        }

        if (HEADER.kind != MAREA_kindOfClass(itemClass)) {
            failures.push_back(NAME + ": the header kind does not match the folder " + itemClass);

            return failures;
        }

        const MAREA_RunResult RESULT{ MAREA_run(item.source) };

        if (RESULT.except != MAREA_SEMA_EXCEPT_SUCCESS) {
            failures.push_back(NAME + ": the compiler stages did not run to the end");

            return failures;
        }

        const std::string ACTUAL{ MAREA_describe(RESULT.codes, RESULT.lines) };

        if (itemClass == "accept") {
            if (RESULT.errorCount != 0) {
                failures.push_back(NAME + ": an accept item must give no error, got " + ACTUAL);
            }

            return failures;
        }

        if (itemClass == "known_gap") {
            if (RESULT.errorCount != 0) {
                failures.push_back(NAME + ": a known_gap item must be accepted today, got " + ACTUAL + "; the gap is fixed, move the item to reject/ or accept/ and close issue #" + HEADER.issue);
            }

            return failures;
        }

        std::vector<MAREA_Uinteger32> wantedCodes;

        std::vector<MAREA_Uinteger32> wantedLines;

        for (const MAREA_Expectation &expectation : HEADER.expectations) {
            if (expectation.line == 0) {
                failures.push_back(NAME + ": a reject item must give '@<line>' for code " + std::to_string(expectation.code));
            }

            wantedCodes.push_back(expectation.code);

            wantedLines.push_back(expectation.line);
        }

        if (!failures.empty()) {
            return failures;
        }

        if (RESULT.codes != wantedCodes) {
            failures.push_back(NAME + ": the diagnostic codes differ, expected " + MAREA_describe(wantedCodes, wantedLines) + ", got " + ACTUAL);

            return failures;
        }

        if (RESULT.lines != wantedLines) {
            failures.push_back(NAME + ": a diagnostic is on another line, expected " + MAREA_describe(wantedCodes, wantedLines) + ", got " + ACTUAL);
        }

        return failures;
    }

    bool MAREA_hasDocumentRow(const std::string &document, const std::string &identifier)
    {
        for (const std::string &line : MAREA_splitLines(document)) {
            if (line.compare(0, identifier.size() + 4, "| " + identifier + " |") == 0) {
                return true;
            }
        }

        return false;
    }

    std::vector<std::string> MAREA_checkStructure(const std::vector<MAREA_ItemFile> &files, const std::string &document)
    {
        std::vector<std::string> failures;

        std::map<std::string, std::string> seen;

        const std::regex IDENTIFIER("^L-([A-Z][A-Z0-9_]*)-[0-9]{3}$");

        const std::regex AREA("^[a-z][a-z0-9_]*$");

        for (const MAREA_ItemFile &file : files) {
            const std::vector<std::string> PARTS{ MAREA_split(file.relativePath, '/') };

            if ((PARTS.size() < 2) || (std::find(CLASSES.begin(), CLASSES.end(), PARTS[0]) == CLASSES.end())) {
                failures.push_back(file.relativePath + ": a .marea file must sit under accept/, reject/ or known_gap/");

                continue;
            }

            if (PARTS.size() != 3) {
                failures.push_back(file.relativePath + ": the place of an item is <class>/<area>/<id>.marea");

                continue;
            }

            const std::string &fileName{ PARTS[2] };

            const std::string IDENTIFIER_TEXT{ fileName.substr(0, fileName.size() - std::string(".marea").size()) };

            std::smatch match;

            std::string upperArea{ PARTS[1] };

            std::transform(upperArea.begin(), upperArea.end(), upperArea.begin(), [](unsigned char letter) { return static_cast<char>(std::toupper(letter)); });

            if (!std::regex_match(PARTS[1], AREA) || !std::regex_match(IDENTIFIER_TEXT, match, IDENTIFIER) || (match[1].str() != upperArea)) {
                failures.push_back(file.relativePath + ": the file name must be L-<AREA>-<nnn>.marea, with AREA the upper case of the area folder " + PARTS[1]);

                continue;
            }

            const auto FOUND{ seen.find(IDENTIFIER_TEXT) };

            if (FOUND != seen.end()) {
                failures.push_back(file.relativePath + ": the item id " + IDENTIFIER_TEXT + " is already used by " + FOUND->second);
            } else {
                seen[IDENTIFIER_TEXT] = file.relativePath;
            }

            const MAREA_Header HEADER{ MAREA_parseHeader(file.source) };

            if (HEADER.error.empty() && (HEADER.kind != MAREA_kindOfClass(PARTS[0]))) {
                failures.push_back(file.relativePath + ": the header kind does not match the folder " + PARTS[0]);
            }

            if (!MAREA_hasDocumentRow(document, IDENTIFIER_TEXT)) {
                failures.push_back(file.relativePath + ": the catalogue tables of docs/language/marea-language.md have no row for " + IDENTIFIER_TEXT);
            }
        }

        return failures;
    }

    std::string MAREA_readFile(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);

        std::ostringstream buffer;

        buffer << stream.rdbuf();

        return buffer.str();
    }

    std::vector<MAREA_ItemFile> MAREA_collectItems()
    {
        std::vector<MAREA_ItemFile> result;

        const std::filesystem::path ROOT{ MAREA_TESTS_LANGUAGE_DIRECTORY };

        for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(ROOT)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".marea")) {
                result.push_back({ std::filesystem::relative(entry.path(), ROOT).generic_string(), MAREA_readFile(entry.path()) });
            }
        }

        std::sort(result.begin(), result.end(), [](const MAREA_ItemFile &left, const MAREA_ItemFile &right) { return left.relativePath < right.relativePath; });

        return result;
    }

    std::string MAREA_readDocument()
    {
        return MAREA_readFile(std::filesystem::path(MAREA_TESTS_LANGUAGE_DIRECTORY).parent_path().parent_path() / "docs" / "language" / "marea-language.md");
    }

    std::string MAREA_join(const std::vector<std::string> &lines)
    {
        std::string result;

        for (const std::string &line : lines) {
            result += line + "\n";
        }

        return result;
    }

    const std::string SIMPLE_PROGRAM{ "namespace main;\nfunction one(): sinteger32\n    return 1;\nend\n" };

    const std::string BAD_PROGRAM{ "namespace main;\nfunction one(): sinteger32\n    return missing;\nend\n" };

    bool MAREA_mentions(const std::vector<std::string> &failures, const std::string &name)
    {
        return !failures.empty() && std::all_of(failures.begin(), failures.end(), [&name](const std::string &failure) { return failure.compare(0, name.size(), name) == 0; });
    }
} // namespace

TEST(MAREALanguageCatalogue, EveryItemMatchesItsHeader)
{
    GTEST_COUT << "Validating every catalogue item against the result its first-line header declares." << std::endl;

    const std::vector<MAREA_ItemFile> ITEMS{ MAREA_collectItems() };

    std::map<std::string, int> perClass;

    std::string failures;

    for (const MAREA_ItemFile &item : ITEMS) {
        const std::string ITEM_CLASS{ item.relativePath.substr(0, item.relativePath.find('/')) };

        if (std::find(CLASSES.begin(), CLASSES.end(), ITEM_CLASS) == CLASSES.end()) {
            continue;
        }

        ++perClass[ITEM_CLASS];

        for (const std::string &failure : MAREA_checkItem(ITEM_CLASS, item)) {
            failures += failure + "\n";
        }
    }

    for (const std::string &itemClass : CLASSES) {
        EXPECT_GE(perClass[itemClass], 1) << "The class " << itemClass << " must hold at least one item.";
    }

    EXPECT_TRUE(failures.empty()) << "Every item must match its header:\n"
                                  << failures;
}

TEST(MAREALanguageCatalogue, StructureOfTheCatalogue)
{
    GTEST_COUT << "Validating the place, the name, the header kind, the id and the doc row of every catalogue item." << std::endl;

    const std::vector<std::string> FAILURES{ MAREA_checkStructure(MAREA_collectItems(), MAREA_readDocument()) };

    EXPECT_TRUE(FAILURES.empty()) << "The catalogue structure must hold:\n"
                                  << MAREA_join(FAILURES);
}

TEST(MAREALanguageCatalogue, AcceptItemPassesAndAnErroringProgramFails)
{
    GTEST_COUT << "Validating an accept item passes and one that gives an error fails with its file name." << std::endl;

    const MAREA_ItemFile GOOD{ "accept/seed/L-SEED-001.marea", "-- expect: ok\n" + SIMPLE_PROGRAM };

    EXPECT_TRUE(MAREA_checkItem("accept", GOOD).empty()) << "A clean program with '-- expect: ok' must pass.";

    const MAREA_ItemFile BAD{ "accept/seed/L-SEED-002.marea", "-- expect: ok\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("accept", BAD), BAD.relativePath)) << "An accept item with an error must fail and name its file.";
}

TEST(MAREALanguageCatalogue, RejectItemChecksTheCodeAndTheLine)
{
    GTEST_COUT << "Validating a reject item checks the exact codes and the line of each diagnostic." << std::endl;

    const MAREA_ItemFile GOOD{ "reject/seed/L-SEED-001.marea", "-- expect: 3004 @4\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_checkItem("reject", GOOD).empty()) << "The right code on the right line must pass.";

    const MAREA_ItemFile WRONG_CODE{ "reject/seed/L-SEED-002.marea", "-- expect: 3011 @4\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("reject", WRONG_CODE), WRONG_CODE.relativePath)) << "A wrong code must fail and name its file.";

    const MAREA_ItemFile EXTRA_CODE{ "reject/seed/L-SEED-003.marea", "-- expect: 3004 @4, 3004 @4\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("reject", EXTRA_CODE), EXTRA_CODE.relativePath)) << "A code list longer than the diagnostics must fail.";

    const MAREA_ItemFile WRONG_LINE{ "reject/seed/L-SEED-004.marea", "-- expect: 3004 @9\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("reject", WRONG_LINE), WRONG_LINE.relativePath)) << "A wrong line must fail and name its file.";

    const MAREA_ItemFile NO_LINE{ "reject/seed/L-SEED-005.marea", "-- expect: 3004\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("reject", NO_LINE), NO_LINE.relativePath)) << "A reject item without '@<line>' must fail.";

    const MAREA_ItemFile ACCEPTED{ "reject/seed/L-SEED-006.marea", "-- expect: 3004 @4\n" + SIMPLE_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("reject", ACCEPTED), ACCEPTED.relativePath)) << "A reject item that compiles clean must fail.";
}

TEST(MAREALanguageCatalogue, KnownGapItemMustBeAcceptedToday)
{
    GTEST_COUT << "Validating a known_gap item passes while accepted and fails once the program is rejected." << std::endl;

    const MAREA_ItemFile GOOD{ "known_gap/seed/L-SEED-001.marea", "-- gap: #12 NEW\n-- rule: reject\n" + SIMPLE_PROGRAM };

    EXPECT_TRUE(MAREA_checkItem("known_gap", GOOD).empty()) << "An accepted program with a full gap header must pass.";

    const MAREA_ItemFile FIXED{ "known_gap/seed/L-SEED-002.marea", "-- gap: #12 3004\n-- rule: reject\n" + BAD_PROGRAM };

    EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("known_gap", FIXED), FIXED.relativePath)) << "A known_gap program that is rejected must fail and name its file.";
}

TEST(MAREALanguageCatalogue, MalformedHeaderFailsWithTheFileName)
{
    GTEST_COUT << "Validating a missing or malformed header fails with the file name in the message." << std::endl;

    const std::vector<std::string> HEADERS{
        "",
        "// expect: ok\n",
        "-- expect: fine\n",
        "-- expect: 3004 @\n",
        "-- expect: 9999 @4\n",
        "-- expect: 3023 @4\n",
        "-- gap: NEW\n-- rule: reject\n",
        "-- gap: #12\n-- rule: reject\n",
        "-- gap: #12 NEW\n",
        "-- gap: #12 NEW\n-- rule: maybe\n",
        "-- gap: #0 NEW\n-- rule: reject\n",
        "-- expect: ok\n-- rule: reject\n",
    };

    for (const std::string &header : HEADERS) {
        const MAREA_ItemFile ITEM{ "accept/seed/L-SEED-001.marea", header + SIMPLE_PROGRAM };

        EXPECT_TRUE(MAREA_mentions(MAREA_checkItem("accept", ITEM), ITEM.relativePath)) << "The header must fail with the file name: " << header;
    }
}

TEST(MAREALanguageCatalogue, StructureCheckFailsForAMisplacedOrMisnamedItem)
{
    GTEST_COUT << "Validating the structure check fails for each placement rule it owns." << std::endl;

    const std::string DOCUMENT{ "| L-SEED-001 | seed | ok |\n| L-SEED-002 | seed | reject |\n" };

    const std::string OK_SOURCE{ "-- expect: ok\n" + SIMPLE_PROGRAM };

    EXPECT_TRUE(MAREA_checkStructure({ { "accept/seed/L-SEED-001.marea", OK_SOURCE } }, DOCUMENT).empty()) << "A well placed item with a row must pass.";

    const std::vector<std::pair<std::string, std::vector<MAREA_ItemFile>>> CASES{
        { "outside the class folders", { { "stray/L-SEED-001.marea", OK_SOURCE } } },
        { "no area folder", { { "accept/L-SEED-001.marea", OK_SOURCE } } },
        { "wrong file name", { { "accept/seed/first.marea", OK_SOURCE } } },
        { "area does not match the id", { { "accept/seed/L-OTHER-001.marea", OK_SOURCE } } },
        { "header kind does not match the folder", { { "reject/seed/L-SEED-001.marea", OK_SOURCE } } },
        { "duplicate id", { { "accept/seed/L-SEED-001.marea", OK_SOURCE }, { "known_gap/seed/L-SEED-001.marea", "-- gap: #12 NEW\n-- rule: reject\n" + SIMPLE_PROGRAM } } },
        { "no row in the doc", { { "accept/seed/L-SEED-003.marea", OK_SOURCE } } },
    };

    for (const auto &entry : CASES) {
        const std::vector<std::string> FAILURES{ MAREA_checkStructure(entry.second, DOCUMENT) };

        EXPECT_FALSE(FAILURES.empty()) << "The structure check must fail for: " << entry.first;

        for (const std::string &failure : FAILURES) {
            EXPECT_NE(failure.find(".marea"), std::string::npos) << "The failure must name the file for: " << entry.first;
        }
    }
}
