// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaBcDriver.h"

#include <stdlib.h>
#include <string.h>

#include "marea/base/MAREAArena.h"
#include "marea/base/MAREADiagnostic.h"
#include "marea/lex/MAREALexer.h"
#include "marea/lex/MAREAToken.h"
#include "marea/lex/MAREATokenKind.h"
#include "marea/parse/MAREAParser.h"
#include "marea/parse/MAREAAst.h"
#include "marea/sema/MAREASemaImportResolver.h"
#include "marea/sema/MAREASemaTypedAst.h"
#include "marea/ir/MAREAIr.h"
#include "marea/bytecode/MAREABytecode.h"
#include "marea/bytecode/MAREABytecodeFormat.h"
#include "marea/bytecode/MAREABytecodeGenerate.h"

#include "MareaDiagnosticPrinter.h"
#include "MareaFileIo.h"
#include "MareaImportResolver.h"

/* Returns a heap-allocated copy of the directory part of path (everything before the last '/'), or
 * "." when path has no '/'. The caller frees the result. */
static char *MareaBcDriverDirectoryOf(const char *path)
{
    const char *lastSlash;
    MAREA_IntegerSize length;
    char *directory;

    lastSlash = strrchr(path, '/');

    if (lastSlash == NULL)
    {
        directory = (char *)malloc(2);

        if (directory != NULL)
        {
            directory[0] = '.';
            directory[1] = '\0';
        }

        return directory;
    }

    length = (MAREA_IntegerSize)(lastSlash - path);
    directory = (char *)malloc(length + 1);

    if (directory != NULL)
    {
        memcpy(directory, path, length);
        directory[length] = '\0';
    }

    return directory;
}

/* Lexes source (already read from inputPath) with the real file path set on the context, so a
 * diagnostic's own position and the namespace-matches-file-path check both see the real path instead
 * of the lexer's own "<source>" default. */
static MAREA_LexExcept MareaBcDriverLexFile(MAREA_Arena *arena, MAREA_DiagnosticSink *sink, const char *inputPath, const MAREA_Character8 *source, MAREA_IntegerSize sourceLength, MAREA_TokenList **outTokens)
{
    MAREA_LexContext *lexContext;

    if (MAREA_LexContextCreate(arena, sink, &lexContext) != MAREA_LEX_EXCEPT_SUCCESS)
    {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    if (MAREA_LexContextSetFileName(lexContext, (const MAREA_Character8 *)inputPath) != MAREA_LEX_EXCEPT_SUCCESS)
    {
        return MAREA_LEX_EXCEPT_ERROR_NULL_POINTER;
    }

    return MAREA_LexTokenize(lexContext, source, sourceLength, outTokens);
}

/* The lex/parse/sema/ir pipeline both compile and dump-ir run. Fills *outIrModule and leaves the
 * arena and sink the caller gave it holding every diagnostic. Returns MAREA_BOOLEAN_FALSE as soon as
 * one stage's own error count is not 0, after printing that stage's diagnostics. */
static MAREA_Boolean MareaBcDriverLowerToIr(const char *inputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, MAREA_Arena *arena, MAREA_DiagnosticSink *sink, FILE *errorStream, const MAREA_IrNode **outIrModule)
{
    const MAREA_Character8 *source;
    MAREA_IntegerSize sourceLength;
    MAREA_LexContext *lexContext;
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

    if (!MareaFileIoReadAll(arena, inputPath, &source, &sourceLength))
    {
        fprintf(errorStream, "error: cannot read '%s'\n", inputPath);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MareaBcDriverLexFile(arena, sink, inputPath, source, sourceLength, &tokens) != MAREA_LEX_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_ParseContextCreate(arena, sink, &parseContext) != MAREA_PARSE_EXCEPT_SUCCESS || MAREA_ParseCompilationUnit(parseContext, tokens, &root) != MAREA_PARSE_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_TypeRegistryCreate(arena, libraryMask, &registry) != MAREA_TYPE_REGISTRY_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the type registry\n");
        return MAREA_BOOLEAN_FALSE;
    }

    primaryDirectory = MareaBcDriverDirectoryOf(inputPath);
    MareaImportResolverInit(&resolverState, arena, sink, includeDirectories, includeDirectoryCount, (const MAREA_Character8 *)primaryDirectory);

    if (MAREA_SemaImportResolverCreate(arena, MareaImportResolverResolve, &resolverState, &resolver) != MAREA_SEMA_IMPORT_RESOLVER_EXCEPT_SUCCESS)
    {
        free(primaryDirectory);
        fprintf(errorStream, "error: cannot create the import resolver\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaContextCreate(arena, sink, resolver, &semaContext) != MAREA_SEMA_EXCEPT_SUCCESS)
    {
        free(primaryDirectory);
        fprintf(errorStream, "error: cannot create the sema context\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaContextSetTypeRegistry(semaContext, registry) != MAREA_SEMA_EXCEPT_SUCCESS || MAREA_SemaContextSetTarget(semaContext, target) != MAREA_SEMA_EXCEPT_SUCCESS || MAREA_SemaContextSetSettings(semaContext, &settings) != MAREA_SEMA_EXCEPT_SUCCESS)
    {
        free(primaryDirectory);
        fprintf(errorStream, "error: cannot configure the sema context\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaCheckCompilationUnit(semaContext, root) != MAREA_SEMA_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        free(primaryDirectory);
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_SemaBuildTypedAst(semaContext, root, &typedRoot) != MAREA_SEMA_TYPED_AST_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        free(primaryDirectory);
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    free(primaryDirectory);

    if (MAREA_IrContextCreate(arena, sink, &irContext) != MAREA_IR_EXCEPT_SUCCESS || MAREA_IrLowerCompilationUnit(irContext, typedRoot, outIrModule) != MAREA_IR_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MareaBcDriverCompile(const char *inputPath, const char *outputPath, MAREA_LibraryMask libraryMask, MAREA_SemaSettings settings, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_IrNode *irModule;
    MAREA_BytecodeContext *bytecodeContext;
    MAREA_BytecodeModule *bytecodeModule;
    const MAREA_Byte *bytes;
    MAREA_IntegerSize byteSize;
    MAREA_Boolean succeeded;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS)
    {
        MAREA_ArenaDestroy(arena);
        fprintf(errorStream, "error: cannot create the diagnostic sink\n");
        return MAREA_BOOLEAN_FALSE;
    }

    succeeded = MareaBcDriverLowerToIr(inputPath, libraryMask, settings, target, includeDirectories, includeDirectoryCount, arena, sink, errorStream, &irModule);

    if (!succeeded)
    {
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_BytecodeContextCreate(arena, sink, &bytecodeContext) != MAREA_BYTECODE_EXCEPT_SUCCESS || MAREA_BytecodeGenerate(bytecodeContext, irModule, &bytecodeModule) != MAREA_BYTECODE_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_BytecodeSerialize(bytecodeModule, &bytes, &byteSize) != MAREA_BYTECODE_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot serialize the bytecode module\n");
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (!MareaFileIoWriteAll(outputPath, bytes, byteSize))
    {
        fprintf(errorStream, "error: cannot write '%s'\n", outputPath);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    MAREA_ArenaDestroy(arena);
    return MAREA_BOOLEAN_TRUE;
}

MAREA_Boolean MareaBcDriverAssemble(const char *inputPath, const char *outputPath, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_Character8 *text;
    MAREA_IntegerSize textLength;
    MAREA_BytecodeContext *bytecodeContext;
    MAREA_BytecodeModule *module;
    const MAREA_Byte *bytes;
    MAREA_IntegerSize byteSize;
    MAREA_Boolean result;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS)
    {
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (!MareaFileIoReadAll(arena, inputPath, &text, &textLength))
    {
        fprintf(errorStream, "error: cannot read '%s'\n", inputPath);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    result = MAREA_BOOLEAN_TRUE;

    if (MAREA_BytecodeContextCreate(arena, sink, &bytecodeContext) != MAREA_BYTECODE_EXCEPT_SUCCESS || MAREA_BytecodeAssemble(bytecodeContext, text, textLength, &module) != MAREA_BYTECODE_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        result = MAREA_BOOLEAN_FALSE;
    }
    else if (MAREA_BytecodeSerialize(module, &bytes, &byteSize) != MAREA_BYTECODE_EXCEPT_SUCCESS || !MareaFileIoWriteAll(outputPath, bytes, byteSize))
    {
        fprintf(errorStream, "error: cannot write '%s'\n", outputPath);
        result = MAREA_BOOLEAN_FALSE;
    }

    MAREA_ArenaDestroy(arena);
    return result;
}

MAREA_Boolean MareaBcDriverDisassemble(const char *inputPath, const char *outputPath, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_Character8 *bytes;
    MAREA_IntegerSize byteSize;
    MAREA_BytecodeContext *bytecodeContext;
    MAREA_BytecodeModule *module;
    const MAREA_Character8 *text;
    MAREA_IntegerSize textLength;
    MAREA_Boolean result;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS)
    {
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (!MareaFileIoReadAll(arena, inputPath, &bytes, &byteSize))
    {
        fprintf(errorStream, "error: cannot read '%s'\n", inputPath);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    result = MAREA_BOOLEAN_TRUE;

    if (MAREA_BytecodeContextCreate(arena, sink, &bytecodeContext) != MAREA_BYTECODE_EXCEPT_SUCCESS || MAREA_BytecodeDeserialize(bytecodeContext, (const MAREA_Byte *)bytes, byteSize, &module) != MAREA_BYTECODE_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        result = MAREA_BOOLEAN_FALSE;
    }
    else if (MAREA_BytecodeDisassemble(module, &text, &textLength) != MAREA_BYTECODE_EXCEPT_SUCCESS || !MareaFileIoWriteAll(outputPath, text, textLength))
    {
        fprintf(errorStream, "error: cannot write '%s'\n", outputPath);
        result = MAREA_BOOLEAN_FALSE;
    }

    MAREA_ArenaDestroy(arena);
    return result;
}

MAREA_Boolean MareaBcDriverDumpTokens(const char *inputPath, FILE *outStream, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_Character8 *source;
    MAREA_IntegerSize sourceLength;
    MAREA_LexContext *lexContext;
    MAREA_TokenList *tokens;
    MAREA_IntegerSize count;
    MAREA_IntegerSize index;
    MAREA_Boolean result;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS)
    {
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (!MareaFileIoReadAll(arena, inputPath, &source, &sourceLength))
    {
        fprintf(errorStream, "error: cannot read '%s'\n", inputPath);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (MareaBcDriverLexFile(arena, sink, inputPath, source, sourceLength, &tokens) != MAREA_LEX_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    result = MAREA_BOOLEAN_TRUE;
    count = MAREA_TokenListGetCount(tokens);

    for (index = 0; index < count; index = index + 1)
    {
        const MAREA_Token *token;
        MAREA_SourcePosition position;

        if (MAREA_TokenListGetAt(tokens, index, &token) != MAREA_LEX_EXCEPT_SUCCESS)
        {
            result = MAREA_BOOLEAN_FALSE;
            break;
        }

        position = MAREA_TokenGetPosition(token);

        fprintf(
            outStream,
            "%zu: kind=%d %u:%u text=\"%.*s\"\n",
            (size_t)index,
            (int)MAREA_TokenGetKind(token),
            (unsigned int)position.line,
            (unsigned int)position.column,
            (int)MAREA_TokenGetLength(token),
            MAREA_TokenGetText(token));
    }

    MAREA_ArenaDestroy(arena);
    return result;
}

MAREA_Boolean MareaBcDriverDumpAst(const char *inputPath, FILE *outStream, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_Character8 *source;
    MAREA_IntegerSize sourceLength;
    MAREA_LexContext *lexContext;
    MAREA_TokenList *tokens;
    MAREA_ParseContext *parseContext;
    const MAREA_AstNode *root;
    const MAREA_Character8 *text;
    MAREA_IntegerSize textLength;
    MAREA_Boolean result;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS)
    {
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    if (!MareaFileIoReadAll(arena, inputPath, &source, &sourceLength))
    {
        fprintf(errorStream, "error: cannot read '%s'\n", inputPath);
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    result = MAREA_BOOLEAN_TRUE;

    if (MareaBcDriverLexFile(arena, sink, inputPath, source, sourceLength, &tokens) != MAREA_LEX_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        result = MAREA_BOOLEAN_FALSE;
    }
    else if (MAREA_ParseContextCreate(arena, sink, &parseContext) != MAREA_PARSE_EXCEPT_SUCCESS || MAREA_ParseCompilationUnit(parseContext, tokens, &root) != MAREA_PARSE_EXCEPT_SUCCESS || MAREA_DiagnosticSinkGetErrorCount(sink) != 0)
    {
        MareaDiagnosticPrinterPrint(sink, errorStream);
        result = MAREA_BOOLEAN_FALSE;
    }
    else if (MAREA_AstDump(arena, root, &text, &textLength) != MAREA_AST_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot dump the AST\n");
        result = MAREA_BOOLEAN_FALSE;
    }
    else
    {
        fwrite(text, 1, (size_t)textLength, outStream);
    }

    MAREA_ArenaDestroy(arena);
    return result;
}

MAREA_Boolean MareaBcDriverDumpIr(const char *inputPath, MAREA_LibraryMask libraryMask, const MAREA_SemaTarget *target, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, FILE *outStream, FILE *errorStream)
{
    MAREA_Arena *arena;
    MAREA_DiagnosticSink *sink;
    const MAREA_IrNode *irModule;
    const MAREA_Character8 *text;
    MAREA_IntegerSize textLength;
    MAREA_Boolean result;
    MAREA_SemaSettings emptySettings;

    emptySettings.pairs = NULL;
    emptySettings.count = 0;

    if (MAREA_ArenaCreate(0, &arena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(errorStream, "error: cannot create the arena\n");
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_DiagnosticSinkCreate(arena, &sink) != MAREA_DIAGNOSTIC_EXCEPT_SUCCESS)
    {
        MAREA_ArenaDestroy(arena);
        return MAREA_BOOLEAN_FALSE;
    }

    result = MareaBcDriverLowerToIr(inputPath, libraryMask, emptySettings, target, includeDirectories, includeDirectoryCount, arena, sink, errorStream, &irModule);

    if (result)
    {
        if (MAREA_IrModuleDump(arena, irModule, &text, &textLength) != MAREA_IR_EXCEPT_SUCCESS)
        {
            fprintf(errorStream, "error: cannot dump the IR module\n");
            result = MAREA_BOOLEAN_FALSE;
        }
        else
        {
            fwrite(text, 1, (size_t)textLength, outStream);
        }
    }

    MAREA_ArenaDestroy(arena);
    return result;
}

void MareaBcDriverListSettings(MAREA_SemaSettings settings, FILE *outStream)
{
    MAREA_IntegerSize index;

    for (index = 0; index < settings.count; index = index + 1)
    {
        fprintf(outStream, "%s=%s\n", settings.pairs[index].name, settings.pairs[index].value);
    }
}
