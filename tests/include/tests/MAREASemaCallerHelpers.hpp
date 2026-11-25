// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREASEMACALLERHELPERS_HPP)
    #define MAREA_TESTS_MAREASEMACALLERHELPERS_HPP

    #include "tests/MAREASemaHelpers.hpp"

    #include <map>
    #include <utility>

// INFO: the three plain structs that the caller gives to sema (target, settings, include resolver), built from
// owned strings so a test writes only the values. A field that a test leaves empty is passed as "not answered".
struct MAREA_CallerInputs
{
    bool hasTarget{ false };
    std::string architecture;
    std::string operatingSystem;
    std::string compiler;
    std::string endianness;
    std::vector<std::string> features;
    MAREA_Uinteger32 pointerWidth{ 0 };

    bool hasSettings{ false };
    std::vector<std::pair<std::string, std::string>> settings;

    bool hasIncludes{ false };
    std::map<std::string, MAREA_IntegerSize> files;

    MAREA_LibraryMask libraries{ MAREA_LIBRARY_MASK_DEFAULT };

    // INFO: a target of the common x86_64 Linux build with gcc.
    static MAREA_CallerInputs MAREA_linuxX86()
    {
        MAREA_CallerInputs inputs;

        inputs.hasTarget = true;
        inputs.architecture = "x86_64";
        inputs.operatingSystem = "linux";
        inputs.compiler = "gcc";
        inputs.endianness = "little";
        inputs.features = { "sse2", "avx2" };
        inputs.pointerWidth = 64;

        return inputs;
    }

    static MAREA_Boolean MAREA_CALL MAREA_includeSize(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_IntegerSize *outSize)
    {
        const auto *files{ static_cast<const std::map<std::string, MAREA_IntegerSize> *>(userData) };

        const auto FOUND{ files->find(std::string(path, pathLength)) };

        if (FOUND == files->end()) {
            return MAREA_BOOLEAN_FALSE;
        }

        *outSize = FOUND->second;

        return MAREA_BOOLEAN_TRUE;
    }
};

// INFO: one sema run with the inputs given to the context. Every namespace of the c and marea roots and of demo is an
// external library, and the registry is the fixture one, built with the library mask of the inputs.
struct MAREA_CallerRun
{
    MAREA_SemaFixture fixture{ [](const std::string &text, const MAREA_AstNode **) {
        const bool EXTERNAL{ (text.rfind("marea.", 0) == 0) || (text.rfind("c.", 0) == 0) };

        return EXTERNAL ? MAREA_SEMA_IMPORT_RESULT_EXTERNAL : MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;
    } };
    MAREA_SemaTarget target{};
    MAREA_SemaSettings settings{};
    std::vector<MAREA_SemaSetting> pairs;
    std::vector<const MAREA_Character8 *> features;
    MAREA_SemaIncludeResolver includes{};
    MAREA_SemaExcept except{ MAREA_SEMA_EXCEPT_SUCCESS };

    MAREA_CallerRun(const std::string &program, const MAREA_CallerInputs &inputs)
    {
        MAREA_IGNORE(MAREA_SemaContextSetTypeRegistry(fixture.context, MAREA_buildLibraryRegistry(fixture.parser.lexer.arena, inputs.libraries)));

        if (inputs.hasTarget) {
            target.architecture = inputs.architecture.empty() ? nullptr : inputs.architecture.c_str();
            target.operatingSystem = inputs.operatingSystem.empty() ? nullptr : inputs.operatingSystem.c_str();
            target.compiler = inputs.compiler.empty() ? nullptr : inputs.compiler.c_str();
            target.endianness = inputs.endianness.empty() ? nullptr : inputs.endianness.c_str();

            for (const std::string &feature : inputs.features) {
                features.push_back(feature.c_str());
            }

            target.features = features.empty() ? nullptr : features.data();
            target.featureCount = features.size();
            target.pointerWidth = inputs.pointerWidth;

            MAREA_IGNORE(MAREA_SemaContextSetTarget(fixture.context, &target));
        }

        if (inputs.hasSettings) {
            for (const auto &pair : inputs.settings) {
                pairs.push_back({ pair.first.c_str(), pair.second.c_str() });
            }

            settings.pairs = pairs.empty() ? nullptr : pairs.data();
            settings.count = pairs.size();

            MAREA_IGNORE(MAREA_SemaContextSetSettings(fixture.context, &settings));
        }

        if (inputs.hasIncludes) {
            includes.size = &MAREA_CallerInputs::MAREA_includeSize;
            includes.userData = const_cast<std::map<std::string, MAREA_IntegerSize> *>(&inputs.files);

            MAREA_IGNORE(MAREA_SemaContextSetIncludeResolver(fixture.context, &includes));
        }

        except = fixture.MAREA_sema(program, MAREA_pathOf(program));
    }

    // INFO: the file path that the namespace line of the program asks for, so the namespace rule of the path raises nothing: "namespace marea.math;" is the file marea/math.marea.
    static std::string MAREA_pathOf(const std::string &program)
    {
        const std::string KEYWORD{ "namespace " };

        const std::size_t
END
{ program.find(';') };

if ((program.rfind(KEYWORD, 0) != 0) || (END == std::string::npos)) {
    return "main.marea";
}

std::string path{ program.substr(KEYWORD.size(), END - KEYWORD.size()) };

for (char &letter : path) {
    letter = (letter == '.') ? '/' : letter;
}

return path + ".marea";
}

std::vector<MAREA_Uinteger32> MAREA_codes() const
{
    if (except != MAREA_SEMA_EXCEPT_SUCCESS) {
        return { 0 };
    }

    return fixture.MAREA_semaCodes();
}

std::vector<std::string> MAREA_messages() const
{
    std::vector<std::string> messages;

    for (MAREA_Uinteger32 index = 0; index < static_cast<MAREA_Uinteger32>(MAREA_DiagnosticSinkGetCount(fixture.parser.lexer.sink)); ++index) {
        const MAREA_Diagnostic *diagnostic{ nullptr };

        if ((MAREA_DiagnosticSinkGetAt(fixture.parser.lexer.sink, index, &diagnostic) == MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) && (MAREA_DiagnosticGetCode(diagnostic) >= 3000)) {
            messages.emplace_back(MAREA_DiagnosticGetMessage(diagnostic));
        }
    }

    return messages;
}
}
;

inline std::vector<MAREA_Uinteger32> MAREA_codesWith(const std::string &program, const MAREA_CallerInputs &inputs = MAREA_CallerInputs{})
{
    MAREA_CallerRun run(program, inputs);

    return run.MAREA_codes();
}

#endif
