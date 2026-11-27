// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_MAREABC_MAREABCDRIVER_H)
    #define MAREA_APPS_MAREABC_MAREABCDRIVER_H

    #include <stdio.h>

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/sema/MAREASema.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief Lexes, parses, checks, lowers and generates bytecode for inputPath, writing the serialized
     * module to outputPath. Every diagnostic of a failing stage is written to errorStream; the pipeline
     * stops at the first stage whose own error count is not 0.
     * @param inputPath Path of the ".marea" source file. Must not be null.
     * @param outputPath Path of the ".mbc" file to write. Must not be null.
     * @param libraryMask Library mask given to MAREA_TypeRegistryCreate.
     * @param settings Build settings given to MAREA_SemaContextSetSettings.
     * @param target Target description given to MAREA_SemaContextSetTarget. Must not be null.
     * @param includeDirectories Import search directories, in order. May be null when
     * includeDirectoryCount is 0.
     * @param includeDirectoryCount Number of entries in includeDirectories.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaBcDriverCompile(const char *inputPath, const char *outputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *errorStream);

    /**
     * @brief Assembles a ".mbca" text file into a ".mbc" binary module.
     * @param inputPath Path of the ".mbca" text file. Must not be null.
     * @param outputPath Path of the ".mbc" file to write. Must not be null.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaBcDriverAssemble(const char *inputPath, const char *outputPath, FILE *errorStream);

    /**
     * @brief Disassembles a ".mbc" binary module into a ".mbca" text file.
     * @param inputPath Path of the ".mbc" binary file. Must not be null.
     * @param outputPath Path of the ".mbca" file to write. Must not be null.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaBcDriverDisassemble(const char *inputPath, const char *outputPath, FILE *errorStream);

    /**
     * @brief Lexes inputPath and prints one line per token (index, kind, line:column, text) to outStream.
     * @param inputPath Path of the ".marea" source file. Must not be null.
     * @param outStream Stream the listing is printed to. Must not be null.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaBcDriverDumpTokens(const char *inputPath, FILE *outStream, FILE *errorStream);

    /**
     * @brief Lexes and parses inputPath and prints the AST's own text form (MAREA_AstDump) to outStream.
     * @param inputPath Path of the ".marea" source file. Must not be null.
     * @param outStream Stream the dump is printed to. Must not be null.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaBcDriverDumpAst(const char *inputPath, FILE *outStream, FILE *errorStream);

    /**
     * @brief Lexes, parses, checks and lowers inputPath and prints the IR module's own text form
     * (MAREA_IrModuleDump) to outStream.
     * @param inputPath Path of the ".marea" source file. Must not be null.
     * @param libraryMask Library mask given to MAREA_TypeRegistryCreate.
     * @param target Target description given to MAREA_SemaContextSetTarget. Must not be null.
     * @param includeDirectories Import search directories, in order. May be null when
     * includeDirectoryCount is 0.
     * @param includeDirectoryCount Number of entries in includeDirectories.
     * @param outStream Stream the dump is printed to. Must not be null.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaBcDriverDumpIr(const char *inputPath, MAREA_LibraryMask libraryMask, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *outStream, FILE *errorStream);

    /**
     * @brief Lists every name=value pair of settings to outStream, one per line. This is the close,
     * honest approximation the apply's own text allows: the typed-AST accessor API does not yet expose
     * reading a build_value! call site back out, so this lists every setting name actually given on the
     * command line instead of every build_value! call the compiled unit makes.
     * @param settings Settings to list.
     * @param outStream Stream the listing is printed to. Must not be null.
     */
    void MareaBcDriverListSettings(MAREA_SemaSettings settings, FILE * outStream);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_MAREABC_MAREABCDRIVER_H */
