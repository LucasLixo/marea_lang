// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_TESTS_MAREAINCLUDEDFILES_HPP)
    #define MAREA_TESTS_MAREAINCLUDEDFILES_HPP

    #include "marea/sema/MAREASema.h"

    #include "tests/MAREAScratchDirectory.hpp"

    #include <fstream>
    #include <map>
    #include <string>

// INFO: real files in a scratch directory and the include resolver of a caller that reads them. sizeError makes the load
// function answer one byte less than the size function.
struct MAREA_IncludedFiles
{
    struct MAREA_Entry
    {
        std::string path;
        std::string bytes;
    };

    MAREA_ScratchDirectory scratch;
    std::map<std::string, MAREA_Entry> entries;
    bool shortLoad{ false };
    MAREA_SemaIncludeResolver resolver{};

    MAREA_IncludedFiles()
    {
        resolver.size = &MAREA_IncludedFiles::MAREA_size;
        resolver.load = &MAREA_IncludedFiles::MAREA_load;
        resolver.userData = this;
    }

    MAREA_IncludedFiles(const MAREA_IncludedFiles &) = delete;
    MAREA_IncludedFiles &operator=(const MAREA_IncludedFiles &) = delete;

    void MAREA_add(const std::string &name, const std::string &bytes)
    {
        const std::string PATH{ scratch.MAREA_file(name) };

        std::ofstream(PATH, std::ios::binary) << bytes;

        entries[name] = { PATH, bytes };
    }

    static MAREA_Boolean MAREA_CALL MAREA_size(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_IntegerSize *outSize)
    {
        const auto *files{ static_cast<const MAREA_IncludedFiles *>(userData) };

        const auto FOUND{ files->entries.find(std::string(path, pathLength)) };

        if (FOUND == files->entries.end()) {
            return MAREA_BOOLEAN_FALSE;
        }

        *outSize = FOUND->second.bytes.size();

        return MAREA_BOOLEAN_TRUE;
    }

    static MAREA_Boolean MAREA_CALL MAREA_load(MAREA_Null userData, const MAREA_Character8 *path, MAREA_IntegerSize pathLength, MAREA_SemaIncludedFile *outFile)
    {
        const auto *files{ static_cast<const MAREA_IncludedFiles *>(userData) };

        const auto FOUND{ files->entries.find(std::string(path, pathLength)) };

        if (FOUND == files->entries.end()) {
            return MAREA_BOOLEAN_FALSE;
        }

        outFile->bytes = reinterpret_cast<const MAREA_Byte *>(FOUND->second.bytes.data());
        outFile->size = FOUND->second.bytes.size() - ((files->shortLoad && !FOUND->second.bytes.empty()) ? 1 : 0);
        outFile->path = FOUND->second.path.c_str();
        outFile->pathLength = FOUND->second.path.size();

        return MAREA_BOOLEAN_TRUE;
    }
};

#endif
