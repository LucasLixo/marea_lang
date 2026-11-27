// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaCcDriver.h"

#include <stdlib.h>
#include <string.h>

#include "marea/base/MAREAArena.h"
#include "marea/base/MAREADiagnostic.h"
#include "marea/lex/MAREALexer.h"
#include "marea/parse/MAREAParser.h"
#include "marea/sema/MAREASemaImportResolver.h"
#include "marea/sema/MAREASemaTypedAst.h"
#include "marea/ir/MAREAIr.h"
#include "marea/cgen/MAREACgen.h"

#include "MareaDiagnosticPrinter.h"
#include "MareaFileIo.h"
#include "MareaImportResolver.h"

/* Appended to the generated C text for MAREA_CC_EMIT_EXECUTABLE: cgen never writes "main" and the
 * language has no fixed entry-point convention of its own (see the header's own doc comment), so
 * this only makes the linker happy for the demo binary. */
static const char MAREA_CC_STUB_MAIN[] = "\nint main(void)\n{\n    return 0;\n}\n";

/* Returns a heap-allocated copy of the directory part of path (everything before the last '/'), or
 * "." when path has no '/'. The caller frees the result. Mirrors MareaBcDriverDirectoryOf. */
static char *MareaCcDriverDirectoryOf(const char *path)
{
    const char *lastSlash;
    MAREA_IntegerSize length;
    char *directory;

    lastSlash = strrchr(path, '/');

    if (lastSlash == NULL) {
        directory = (char *)malloc(2);

        if (directory != NULL) {
            directory[0] = '.';
            directory[1] = '\0';
        }

        return directory;
    }

    length = (MAREA_IntegerSize)(lastSlash - path);
    directory = (char *)malloc(length + 1);

    if (directory != NULL) {
        memcpy(directory, path, length);
        directory[length] = '\0';
    }

    return directory;
}

/* Lexes source (already read from inputPath) with the real file path set on the context, so a
 * diagnostic's own position and the namespace-matches-file-path check both see the real path instead
 * of the lexer's own "<source>" default. Mirrors MareaBcDriverLexFile. */
static MAREA_LexExcept MareaCcDriverLexFile(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, const char *inputPath, const MAREA_Character8 *source, MAREA_IntegerSize sourceLength, MAREA_TokenList **outTokens)
{
    MAREA_LexContext *lexContext;

    if (MAREA_LexContextCreate(arena, sink, &lexContext) != MAREA_LEX_EXCEPT_SUCCESS) {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_LexContextSetFileName(lexContext, (const MAREA_Character8 *)inputPath) != MAREA_LEX_EXCEPT_SUCCESS) {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    return MAREA_LexTokenize(lexContext, source, sourceLength, outTokens);
}

/* The lex/parse/sema/ir pipeline that both MareaCcDriverGenerateC and a future caller run. Fills
 * *outIrModule and leaves the arena and sink the caller gave it holding every diagnostic. Returns
 * MAREA_BOOLEAN_FALSE as soon as one stage's own error count is not 0, after printing that stage's
 * diagnostics. */
static MAREA_Boolean MareaCcDriverLowerToIr(const char *inputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, MAREA_Arena *arena, MAREA_DiagnosticSink *sink, FILE *errorStream, const MAREA_IrNode **outIrModule)
{
    const MAREA_Character8 *source;
    MAREA_IntegerSize sourceLength;
    MAREA_TokenList *tokens;
    MAREA_ParseContext *parseContext;
    const MAREA_AstNode *root;
    MAREA_TypeRegistry *registry;
    MareaImportResolverState resolverState;
    MAREA_SemaImportResolver *resolver;
    MAREA_SemaContext *semaContext;
    const MAREA_SemaTypedNode *typedRoot;
    MAREA_IrContext *irContext;
    char *primaryDirectory;

    if (!MareaFileIoReadAll(arena, inputPath, &source, &sourceLength)) {
        fprintf(errorStream, "error: cannot read '%s'\n", inputPath);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MareaCcDriverLexFile(arena, sink, inputPath, source, sourceLength, &tokens) != MAREA_LEX_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0) {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_ParseContextCreate(arena, sink, &parseContext) != MAREA_PARSE_EXCEPT_SUCCESS || MAREA_ParseCompilationUnit(parseContext, tokens, &root) != MAREA_PARSE_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0) {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_TypeRegistryCreate(arena, libraryMask, &registry) != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS) {
        fprintf(errorStream, "error: cannot create the type registry\n");
        return MAREA_BOOLEAN_FALSE;
    }

    primaryDirectory = MareaCcDriverDirectoryOf(inputPath);
    MareaImportResolverInit(&resolverState, arena, sink, includeDirectories, includeDirectoryCount, (const MAREA_Character8 *)primaryDirectory);

    if (MAREA_SemaImportResolverCreate(arena, MareaImportResolverResolve, &resolverState, &resolver) != MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS) {
        free(primaryDirectory);
        fprintf(errorStream, "error: cannot create the import resolver\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaContextCreate(arena, sink, resolver, &semaContext) != MAREA_SEMA_EXCEPT_SUCCESS) {
        free(primaryDirectory);
        fprintf(errorStream, "error: cannot create the sema context\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaContextSetTypeRegistry(semaContext, registry) != MAREA_SEMA_EXCEPT_SUCCESS || MAREA_SemaContextSetTarget(semaContext, target) != MAREA_SEMA_EXCEPT_SUCCESS || MAREA_SemaContextSetSettings(semaContext, &settings) != MAREA_SEMA_EXCEPT_SUCCESS) {
        free(primaryDirectory);
        fprintf(errorStream, "error: cannot configure the sema context\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaCheckCompilationUnit(semaContext, root) != MAREA_SEMA_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0) {
        free(primaryDirectory);
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaBuildTypedAst(semaContext, root, &typedRoot) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0) {
        free(primaryDirectory);
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    free(primaryDirectory);

    if (MAREA_IrContextCreate(arena, sink, &irContext) != MAREA_IR_EXCEPT_SUCCESS || MAREA_IrLowerCompilationUnit(irContext, typedRoot, outIrModule) != MAREA_IR_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0) {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

/* Lowers inputPath to IR then runs cgen, leaving the generated text in outText/outLength (owned by
 * arena). Shared by MareaCcDriverGenerateC and MareaCcDriverCompile. */
static MAREA_Boolean MareaCcDriverGenerateText(const char *inputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, MAREA_Arena *arena, MAREA_DiagnosticSink *sink, FILE *errorStream, const MAREA_Character8 **outText, MAREA_IntegerSize *outLength)
{
    const MAREA_IrNode *irModule;
    MAREA_CgenContext *cgenContext;

    if (!MareaCcDriverLowerToIr(inputPath, libraryMask, settings, target, includeDirectories, includeDirectoryCount, arena, sink, errorStream, &irModule)) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_CgenContextCreate(arena, sink, &cgenContext) != MAREA_CGEN_EXCEPT_SUCCESS) {
        fprintf(errorStream, "error: cannot create the cgen context\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_CgenGenerateModule(cgenContext, irModule, outText, outLength) != MAREA_CGEN_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0) {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MareaCcDriverGenerateC(const char *inputPath, const char *outputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_Character8 *text;
    MAREA_IntegerSize textLength;
    MAREA_Boolean result;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS) {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
        MAREA_ArenaDestroy(arena);
        fprintf(errorStream, "error: cannot create the diagnostic sink\n");
        return MAREA_BOOLEAN_FALSE;
    }

    result = MAREA_BOOLEAN_FALSE;

    if (MareaCcDriverGenerateText(inputPath, libraryMask, settings, target, includeDirectories, includeDirectoryCount, arena, sink, errorStream, &text, &textLength)) {
        if (!MareaFileIoWriteAll(outputPath, text, textLength)) {
            fprintf(errorStream, "error: cannot write '%s'\n", outputPath);
        } else {
            result = MAREA_BOOLEAN_TRUE;
        }
    }

    MAREA_ArenaDestroy(arena);
    return result;
}

MAREA_Boolean MareaCcDriverDetectCompiler(char **outPath)
{
    static const char *const CANDIDATES[] = { "clang", "gcc" };
    const char *pathEnvironment;
    MAREA_IntegerSize candidateIndex;

    pathEnvironment = getenv("PATH");

    if (pathEnvironment == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    for (candidateIndex = 0; candidateIndex < (MAREA_IntegerSize)MAREA_ARRAY_SIZE(CANDIDATES); candidateIndex = candidateIndex + 1) {
        char *pathCopy;
        char *directory;

        pathCopy = (char *)malloc(strlen(pathEnvironment) + 1);

        if (pathCopy == NULL) {
            return MAREA_BOOLEAN_FALSE;
        }

        memcpy(pathCopy, pathEnvironment, strlen(pathEnvironment) + 1);
        directory = strtok(pathCopy, ":");

        while (directory != NULL) {
            char candidatePath[4096];

            if (((size_t)snprintf(candidatePath, sizeof(candidatePath), "%s/%s", directory, CANDIDATES[candidateIndex])) < sizeof(candidatePath)) {
                FILE *probe = fopen(candidatePath, "rb");

                if (probe != NULL) {
                    fclose(probe);
                    free(pathCopy);
                    *outPath = (char *)malloc(strlen(candidatePath) + 1);

                    if (*outPath == NULL) {
                        return MAREA_BOOLEAN_FALSE;
                    }

                    memcpy(*outPath, candidatePath, strlen(candidatePath) + 1);
                    return MAREA_BOOLEAN_TRUE;
                }
            }

            directory = strtok(NULL, ":");
        }

        free(pathCopy);
    }

    return MAREA_BOOLEAN_FALSE;
}

MAREA_Boolean MareaCcDriverCompile(const char *inputPath, const char *outputPath, MareaCcEmitMode emit, const char *ccOverride, const char *cflags, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *errorStream)
{
    if (emit == MAREA_CC_EMIT_C) {
        return MareaCcDriverGenerateC(inputPath, outputPath, libraryMask, settings, target, includeDirectories, includeDirectoryCount, errorStream);
    }

    {
        MAREA_Arena *arena;
        MAREA_DiagnosticSink *sink;
        const MAREA_Character8 *text;
        MAREA_IntegerSize textLength;
        char temporaryPath[4096];
        char *detectedCompiler;
        const char *compilerPath;
        char command[8192];
        int compilerResult;
        MAREA_Boolean result;
        FILE *temporaryFile;

        if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS) {
            fprintf(errorStream, "error: cannot create the arena\n");
            return MAREA_BOOLEAN_FALSE;
        }

        if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS) {
            MAREA_ArenaDestroy(arena);
            fprintf(errorStream, "error: cannot create the diagnostic sink\n");
            return MAREA_BOOLEAN_FALSE;
        }

        if (!MareaCcDriverGenerateText(inputPath, libraryMask, settings, target, includeDirectories, includeDirectoryCount, arena, sink, errorStream, &text, &textLength)) {
            MAREA_ArenaDestroy(arena);
            return MAREA_BOOLEAN_FALSE;
        }

        snprintf(temporaryPath, sizeof(temporaryPath), "%s.marea-cc-tmp.c", outputPath);

        temporaryFile = fopen(temporaryPath, "wb");

        if (temporaryFile == NULL) {
            fprintf(errorStream, "error: cannot write '%s'\n", temporaryPath);
            MAREA_ArenaDestroy(arena);
            return MAREA_BOOLEAN_FALSE;
        }

        fwrite(text, 1, (size_t)textLength, temporaryFile);

        if (emit == MAREA_CC_EMIT_EXECUTABLE) {
            fwrite(MAREA_CC_STUB_MAIN, 1, sizeof(MAREA_CC_STUB_MAIN) - 1, temporaryFile);
        }

        fclose(temporaryFile);
        MAREA_ArenaDestroy(arena);

        detectedCompiler = NULL;

        if (ccOverride != NULL) {
            compilerPath = ccOverride;
        } else if (MareaCcDriverDetectCompiler(&detectedCompiler)) {
            compilerPath = detectedCompiler;
        } else {
            fprintf(errorStream, "error: no C compiler found on PATH (tried clang, gcc); use --cc to override\n");
            remove(temporaryPath);
            return MAREA_BOOLEAN_FALSE;
        }

        snprintf(
            command,
            sizeof(command),
            "\"%s\" -std=c11 %s %s \"%s\" -o \"%s\" %s",
            compilerPath,
            emit == MAREA_CC_EMIT_OBJECT ? "-c" : "",
            cflags != NULL ? cflags : "",
            temporaryPath,
            outputPath,
            /* INFO: the generated C of a function that uses "**" or "%" on a float calls the libc pow/fmod,
             * so the executable mode needs libm on the link line; -c (object) mode never links. */
            emit == MAREA_CC_EMIT_OBJECT ? "" : "-lm");

        compilerResult = system(command);

        free(detectedCompiler);
        remove(temporaryPath);

        if (compilerResult != 0) {
            fprintf(errorStream, "error: the C compiler failed (exit %d)\n", compilerResult);
            return MAREA_BOOLEAN_FALSE;
        }

        result = MAREA_BOOLEAN_TRUE;
        return result;
    }
}

void MareaCcDriverListSettings(MAREA_SemaSettings settings, FILE *outStream)
{
    MAREA_IntegerSize index;

    for (index = 0; index < settings.count; index = index + 1) {
        fprintf(outStream, "%s=%s\n", settings.pairs[index].name, settings.pairs[index].value);
    }
}
