// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include "MareaImportResolver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "marea/lex/MAREALexer.h"
#include "marea/parse/MAREAParser.h"

void MareaImportResolverInit(MareaImportResolverState *state, MAREA_Arena *arena, MAREA_DiagnosticSink *sink, const MAREA_Character8 *const *includeDirectories, MAREA_IntegerSize includeDirectoryCount, const MAREA_Character8 *primaryDirectory)
{
    if (state == NULL) {
        return;
    }

    state->arena = arena;
    state->sink = sink;
    state->includeDirectories = includeDirectories;
    state->includeDirectoryCount = includeDirectoryCount;
    state->primaryDirectory = primaryDirectory;
    state->cache = NULL;
    state->cacheCount = 0;
    state->cacheCapacity = 0;
}

/* Builds "directory/first/second.marea" from a dotted namespace ("first.second"), or just
 * "first/second.marea" when directory is null or empty. Returns a heap-allocated, NUL-terminated
 * path the caller frees, or null on allocation failure. */
static char *MareaImportResolverBuildPath(const char *directory, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength)
{
    MAREA_IntegerSize directoryLength;
    MAREA_IntegerSize totalLength;
    char *path;
    MAREA_IntegerSize index;
    MAREA_IntegerSize cursor;

    directoryLength = directory != NULL ? (MAREA_IntegerSize)strlen(directory) : 0;
    totalLength = directoryLength + (directoryLength > 0 ? 1 : 0) + namespaceLength + strlen(".marea") + 1;

    path = (char *)malloc(totalLength);

    if (path == NULL) {
        return NULL;
    }

    cursor = 0;

    if (directoryLength > 0) {
        memcpy(path, directory, directoryLength);
        cursor = directoryLength;
        path[cursor] = '/';
        cursor = cursor + 1;
    }

    for (index = 0; index < namespaceLength; index = index + 1) {
        char character = (char)namespaceText[index];
        path[cursor + index] = (character == '.') ? '/' : character;
    }

    cursor = cursor + namespaceLength;
    memcpy(path + cursor, ".marea", strlen(".marea"));
    cursor = cursor + strlen(".marea");
    path[cursor] = '\0';

    return path;
}

/* Reads the whole content of path into the arena, NUL terminated. Returns null, with *outLength
 * unset, when the file does not exist or cannot be read. */
static const MAREA_Character8 *MareaImportResolverReadFile(MAREA_Arena *arena, const char *path, MAREA_IntegerSize *outLength)
{
    FILE *file;
    long size;
    MAREA_Null memory;

    file = fopen(path, "rb");

    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    size = ftell(file);

    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    if (MAREA_ArenaAllocate(arena, (MAREA_IntegerSize)size + 1, 1, &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        fclose(file);
        return NULL;
    }

    if (size > 0 && fread(memory, 1, (size_t)size, file) != (size_t)size) {
        fclose(file);
        return NULL;
    }

    fclose(file);
    ((char *)memory)[size] = '\0';
    *outLength = (MAREA_IntegerSize)size;

    return (const MAREA_Character8 *)memory;
}

static MAREA_Boolean MareaImportResolverFindCached(MareaImportResolverState *state, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MareaImportResolverCacheEntry *outEntry)
{
    MAREA_IntegerSize index;

    for (index = 0; index < state->cacheCount; index = index + 1) {
        if (state->cache[index].namespaceLength == namespaceLength && memcmp(state->cache[index].namespaceText, namespaceText, namespaceLength) == 0) {
            *outEntry = state->cache[index];
            return MAREA_BOOLEAN_TRUE;
        }
    }

    return MAREA_BOOLEAN_FALSE;
}

static void MareaImportResolverAppendCache(MareaImportResolverState *state, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult result, const MAREA_AstNode *root)
{
    MAREA_Null memory;
    MAREA_Character8 *namespaceCopy;

    if (state->cacheCount == state->cacheCapacity) {
        MAREA_IntegerSize newCapacity = state->cacheCapacity == 0 ? 4 : state->cacheCapacity * 2;

        if (MAREA_ArenaAllocate(state->arena, newCapacity * sizeof(MareaImportResolverCacheEntry), MAREA_ALIGNOF(MareaImportResolverCacheEntry), &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
            return;
        }

        if (state->cacheCount > 0) {
            memcpy(memory, state->cache, state->cacheCount * sizeof(MareaImportResolverCacheEntry));
        }

        state->cache = (MareaImportResolverCacheEntry *)memory;
        state->cacheCapacity = newCapacity;
    }

    if (MAREA_ArenaAllocate(state->arena, namespaceLength + 1, 1, &memory) != MAREA_ARENA_EXCEPT_SUCCESS) {
        return;
    }

    namespaceCopy = (MAREA_Character8 *)memory;
    memcpy(namespaceCopy, namespaceText, namespaceLength);
    namespaceCopy[namespaceLength] = '\0';

    state->cache[state->cacheCount].namespaceText = namespaceCopy;
    state->cache[state->cacheCount].namespaceLength = namespaceLength;
    state->cache[state->cacheCount].result = result;
    state->cache[state->cacheCount].root = root;
    state->cacheCount = state->cacheCount + 1;
}

/* Tries to lex and parse the ".marea" file this namespace maps to under one directory. Returns
 * MAREA_BOOLEAN_TRUE and fills *outRoot when the file exists and parses; MAREA_BOOLEAN_FALSE when the
 * file does not exist under this directory (not found is not an error). */
static MAREA_Boolean MareaImportResolverTryDirectory(MareaImportResolverState *state, const char *directory, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, const MAREA_AstNode **outRoot)
{
    char *path;
    const MAREA_Character8 *source;
    MAREA_IntegerSize sourceLength;
    MAREA_LexContext *lexContext;
    MAREA_TokenList *tokens;
    MAREA_ParseContext *parseContext;

    path = MareaImportResolverBuildPath(directory, namespaceText, namespaceLength);

    if (path == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    source = MareaImportResolverReadFile(state->arena, path, &sourceLength);
    free(path);

    if (source == NULL) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_LexContextCreate(state->arena, state->sink, &lexContext) != MAREA_LEX_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_LexTokenize(lexContext, source, sourceLength, &tokens) != MAREA_LEX_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_ParseContextCreate(state->arena, state->sink, &parseContext) != MAREA_PARSE_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    if (MAREA_ParseCompilationUnit(parseContext, tokens, outRoot) != MAREA_PARSE_EXCEPT_SUCCESS) {
        return MAREA_BOOLEAN_FALSE;
    }

    return MAREA_BOOLEAN_TRUE;
}

void MAREA_CALL MareaImportResolverResolve(MAREA_Null userData, const MAREA_Character8 *namespaceText, MAREA_IntegerSize namespaceLength, MAREA_SemaImportResult *outResult, const MAREA_AstNode **outRoot)
{
    MareaImportResolverState *state;
    MareaImportResolverCacheEntry cached;
    MAREA_IntegerSize directoryIndex;
    const MAREA_AstNode *root;

    state = (MareaImportResolverState *)userData;

    if (state == NULL || outResult == NULL) {
        return;
    }

    if (MareaImportResolverFindCached(state, namespaceText, namespaceLength, &cached)) {
        *outResult = cached.result;

        if (outRoot != NULL) {
            *outRoot = cached.root;
        }

        return;
    }

    root = NULL;

    if (state->primaryDirectory != NULL && MareaImportResolverTryDirectory(state, (const char *)state->primaryDirectory, namespaceText, namespaceLength, &root)) {
        *outResult = MAREA_SEMA_IMPORT_RESULT_FOUND;

        if (outRoot != NULL) {
            *outRoot = root;
        }

        MareaImportResolverAppendCache(state, namespaceText, namespaceLength, MAREA_SEMA_IMPORT_RESULT_FOUND, root);
        return;
    }

    for (directoryIndex = 0; directoryIndex < state->includeDirectoryCount; directoryIndex = directoryIndex + 1) {
        if (MareaImportResolverTryDirectory(state, (const char *)state->includeDirectories[directoryIndex], namespaceText, namespaceLength, &root)) {
            *outResult = MAREA_SEMA_IMPORT_RESULT_FOUND;

            if (outRoot != NULL) {
                *outRoot = root;
            }

            MareaImportResolverAppendCache(state, namespaceText, namespaceLength, MAREA_SEMA_IMPORT_RESULT_FOUND, root);
            return;
        }
    }

    *outResult = MAREA_SEMA_IMPORT_RESULT_NOT_FOUND;

    if (outRoot != NULL) {
        *outRoot = NULL;
    }

    MareaImportResolverAppendCache(state, namespaceText, namespaceLength, MAREA_SEMA_IMPORT_RESULT_NOT_FOUND, NULL);
}
