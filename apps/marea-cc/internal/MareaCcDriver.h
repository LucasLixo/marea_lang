// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#if !defined(MAREA_APPS_MAREACC_MAREACCDRIVER_H)
    #define MAREA_APPS_MAREACC_MAREACCDRIVER_H

    #include <stdio.h>

    #include "marea/base/MAREACTypes.h"
    #include "marea/base/MAREATypeRegistry.h"
    #include "marea/sema/MAREASema.h"

MAREA_BEGIN_EXTERN_C

    /**
     * @brief What marea-cc writes out, past the generated C11 text.
     */
    typedef enum MareaCcEmitMode
    {
        MAREA_CC_EMIT_C = 0,
        MAREA_CC_EMIT_OBJECT = 1,
        MAREA_CC_EMIT_EXECUTABLE = 2,
    } MareaCcEmitMode;

    /**
     * @brief Lexes, parses, checks, lowers and runs cgen on inputPath, writing the generated C11 text
     * to outputPath. Every diagnostic of a failing stage is written to errorStream; the pipeline stops
     * at the first stage whose own error count is not 0.
     * @param inputPath Path of the ".marea" source file. Must not be null.
     * @param outputPath Path of the ".c" file to write. Must not be null.
     * @param libraryMask Library mask given to MAREA_TypeRegistryCreate.
     * @param settings Build settings given to MAREA_SemaContextSetSettings.
     * @param target Target description given to MAREA_SemaContextSetTarget. Must not be null.
     * @param includeDirectories Import search directories, in order. May be null when
     * includeDirectoryCount is 0.
     * @param includeDirectoryCount Number of entries in includeDirectories.
     * @param errorStream Stream every diagnostic is printed to. Must not be null.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaCcDriverGenerateC(const char *inputPath, const char *outputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *errorStream);

    /**
     * @brief Drives the full marea-cc pipeline: lex through cgen, then, for emit modes other than
     * MAREA_CC_EMIT_C, a system C compiler invoked as a subprocess to produce an object file or a
     * linked executable. For MAREA_CC_EMIT_EXECUTABLE the generated text gets a trivial "int
     * main(void) { return 0; }" appended first, since cgen never writes one and the language has no
     * fixed entry-point convention of its own (AGENTS.md, "Entry point") - this only makes the linker
     * happy for the demo binary, it does not define the language's own entry point.
     * @param inputPath Path of the ".marea" source file. Must not be null.
     * @param outputPath Path of the file to write (".c", object or executable, per emit). Must not be
     * null.
     * @param emit What to write.
     * @param ccOverride Path of the C compiler to use, or null to detect clang then gcc on PATH.
     * @param cflags Extra flags appended to the compiler invocation, or null for none.
     * @param libraryMask Library mask given to MAREA_TypeRegistryCreate.
     * @param settings Build settings given to MAREA_SemaContextSetSettings.
     * @param target Target description given to MAREA_SemaContextSetTarget. Must not be null.
     * @param includeDirectories Import search directories, in order. May be null when
     * includeDirectoryCount is 0.
     * @param includeDirectoryCount Number of entries in includeDirectories.
     * @param errorStream Stream every diagnostic, and every C compiler failure, is printed to. Must
     * not be null. A C compiler failure is reported as a plain error line, never as a
     * MAREA_Diagnostic, since the C compiler is outside every stage's diagnostic model.
     * @return MAREA_BOOLEAN_TRUE on success, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaCcDriverCompile(const char *inputPath, const char *outputPath, MareaCcEmitMode emit, const char *ccOverride, const char *cflags, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *errorStream);

    /**
     * @brief Searches PATH for clang, then for gcc, the same two-step order
     * tests/CMakeLists.txt uses for its own cgen parity tests.
     * @param outPath Receives a heap-allocated copy of the full path. The caller frees it. Left
     * unchanged when no compiler is found.
     * @return MAREA_BOOLEAN_TRUE when a compiler was found, MAREA_BOOLEAN_FALSE otherwise.
     */
    MAREA_Boolean MareaCcDriverDetectCompiler(char **outPath);

    /**
     * @brief Lists every name=value pair of settings to outStream, one per line - the same close,
     * honest approximation MareaBcDriverListSettings documents.
     * @param settings Settings to list.
     * @param outStream Stream the listing is printed to. Must not be null.
     */
    void MareaCcDriverListSettings(MAREA_SemaSettings settings, FILE * outStream);

MAREA_END_EXTERN_C

#endif /* MAREA_APPS_MAREACC_MAREACCDRIVER_H */
