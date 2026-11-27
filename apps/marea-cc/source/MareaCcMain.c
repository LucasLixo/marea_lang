// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "marea/base/MAREAArena.h"
#include "marea/base/MAREACTypes.h"
#include "marea/base/MAREATypeRegistry.h"
#include "marea/sema/MAREASema.h"

#include "MareaCcDriver.h"
#include "MareaLibraryMask.h"
#include "MareaSettingsBuilder.h"
#include "MareaTargetDescription.h"

static void MareaCcPrintUsage(void)
{
    fprintf(
        stderr,
        "usage: marea-cc <input.marea> -o <output> [options]\n"
        "options:\n"
        "  --emit c|object|executable   what to write (default: executable)\n"
        "  --cc <path>                  C compiler to use (default: detect clang then gcc)\n"
        "  --cflags <flags>              extra flags given to the C compiler\n"
        "  --libraries <name,...>       enabled shipped libraries (default: every one)\n"
        "  --settings <NAME=value>      a build setting, repeatable\n"
        "  --include <dir>               an import search directory, repeatable\n"
        "  --target-architecture <text>\n"
        "  --target-os <text>\n"
        "  --target-compiler <text>\n"
        "  --target-endianness <text>\n"
        "  --target-pointer-width <n>\n"
        "  --target-feature <name>      repeatable\n"
        "  --list-settings               list every --settings pair given, then exit\n"
        "  --help, --version\n");
}

int main(int argumentCount, char **arguments)
{
    const char *inputPath;
    const char *outputPath;
    const char *librariesText;
    const char *ccOverride;
    const char *cflags;
    MareaCcEmitMode emit;
    MAREA_Arena *argumentArena;
    MareaSettingsBuilder settingsBuilder;
    const char *includeDirectories[64];
    MAREA_IntegerSize includeDirectoryCount;
    const char *features[64];
    MAREA_IntegerSize featureCount;
    MAREA_SemaTarget target;
    MAREA_LibraryMask libraryMask;
    int index;
    MAREA_Boolean listSettings;

    if (argumentCount < 2) {
        MareaCcPrintUsage();
        return 1;
    }

    if (strcmp(arguments[1], "--help") == 0) {
        MareaCcPrintUsage();
        return 0;
    }

    if (strcmp(arguments[1], "--version") == 0) {
        printf("marea-cc (marea_lang)\n");
        return 0;
    }

    if (MAREA_ArenaCreate(0, &argumentArena) != MAREA_ARENA_EXCEPT_SUCCESS) {
        fprintf(stderr, "error: cannot create the argument arena\n");
        return 1;
    }

    MareaSettingsBuilderInit(&settingsBuilder, argumentArena);

    inputPath = NULL;
    outputPath = NULL;
    librariesText = NULL;
    ccOverride = NULL;
    cflags = NULL;
    emit = MAREA_CC_EMIT_EXECUTABLE;
    includeDirectoryCount = 0;
    featureCount = 0;
    listSettings = MAREA_BOOLEAN_FALSE;

    MareaTargetDescriptionDetectHost(&target);

    for (index = 1; index < argumentCount; index = index + 1) {
        const char *argument = arguments[index];

        if (strcmp(argument, "-o") == 0 && index + 1 < argumentCount) {
            outputPath = arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--emit") == 0 && index + 1 < argumentCount) {
            if (strcmp(arguments[index + 1], "c") == 0) {
                emit = MAREA_CC_EMIT_C;
            } else if (strcmp(arguments[index + 1], "object") == 0) {
                emit = MAREA_CC_EMIT_OBJECT;
            } else if (strcmp(arguments[index + 1], "executable") == 0) {
                emit = MAREA_CC_EMIT_EXECUTABLE;
            } else {
                fprintf(stderr, "error: unknown --emit '%s' (expected c, object or executable)\n", arguments[index + 1]);
                MAREA_ArenaDestroy(argumentArena);
                return 1;
            }

            index = index + 1;
        } else if (strcmp(argument, "--cc") == 0 && index + 1 < argumentCount) {
            ccOverride = arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--cflags") == 0 && index + 1 < argumentCount) {
            cflags = arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--libraries") == 0 && index + 1 < argumentCount) {
            librariesText = arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--settings") == 0 && index + 1 < argumentCount) {
            if (!MareaSettingsBuilderAddPair(&settingsBuilder, (const MAREA_Character8 *)arguments[index + 1])) {
                fprintf(stderr, "error: malformed --settings '%s' (expected NAME=value)\n", arguments[index + 1]);
                MAREA_ArenaDestroy(argumentArena);
                return 1;
            }

            index = index + 1;
        } else if (strcmp(argument, "--include") == 0 && index + 1 < argumentCount && includeDirectoryCount < MAREA_ARRAY_SIZE(includeDirectories)) {
            includeDirectories[includeDirectoryCount] = arguments[index + 1];
            includeDirectoryCount = includeDirectoryCount + 1;
            index = index + 1;
        } else if (strcmp(argument, "--target-architecture") == 0 && index + 1 < argumentCount) {
            target.architecture = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--target-os") == 0 && index + 1 < argumentCount) {
            target.operatingSystem = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--target-compiler") == 0 && index + 1 < argumentCount) {
            target.compiler = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--target-endianness") == 0 && index + 1 < argumentCount) {
            target.endianness = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        } else if (strcmp(argument, "--target-pointer-width") == 0 && index + 1 < argumentCount) {
            target.pointerWidth = (MAREA_Uinteger32)atoi(arguments[index + 1]);
            index = index + 1;
        } else if (strcmp(argument, "--target-feature") == 0 && index + 1 < argumentCount && featureCount < MAREA_ARRAY_SIZE(features)) {
            features[featureCount] = arguments[index + 1];
            featureCount = featureCount + 1;
            index = index + 1;
        } else if (strcmp(argument, "--list-settings") == 0) {
            listSettings = MAREA_BOOLEAN_TRUE;
        } else if (inputPath == NULL && argument[0] != '-') {
            inputPath = argument;
        } else {
            fprintf(stderr, "error: unexpected argument '%s'\n", argument);
            MareaCcPrintUsage();
            MAREA_ArenaDestroy(argumentArena);
            return 1;
        }
    }

    target.features = (const MAREA_Character8 *const *)features;
    target.featureCount = featureCount;

    if (listSettings) {
        MareaCcDriverListSettings(MareaSettingsBuilderBuild(&settingsBuilder), stdout);
        MAREA_ArenaDestroy(argumentArena);
        return 0;
    }

    if (!MareaLibraryMaskParse((const MAREA_Character8 *)librariesText, &libraryMask)) {
        fprintf(stderr, "error: --libraries names an unknown library in '%s'\n", librariesText);
        MAREA_ArenaDestroy(argumentArena);
        return 1;
    }

    if (inputPath == NULL) {
        fprintf(stderr, "error: missing input file\n");
        MareaCcPrintUsage();
        MAREA_ArenaDestroy(argumentArena);
        return 1;
    }

    if (outputPath == NULL) {
        fprintf(stderr, "error: missing -o <output>\n");
        MareaCcPrintUsage();
        MAREA_ArenaDestroy(argumentArena);
        return 1;
    }

    {
        MAREA_Boolean succeeded = MareaCcDriverCompile(inputPath, outputPath, emit, ccOverride, cflags, libraryMask, MareaSettingsBuilderBuild(&settingsBuilder), &target, (const MAREA_Character8 *const *)includeDirectories, includeDirectoryCount, stderr);

        MAREA_ArenaDestroy(argumentArena);
        return succeeded ? 0 : 1;
    }
}
