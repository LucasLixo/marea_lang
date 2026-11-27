// Copyright (c) 2026 Paulo Lucas Alves Franca
// Licensed under the MIT License. See LICENSE.txt for full terms.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "marea/base/MAREAArena.h"
#include "marea/base/MAREACTypes.h"
#include "marea/base/MAREATypeRegistry.h"
#include "marea/sema/MAREASema.h"

#include "MareaBcDriver.h"
#include "MareaLibraryMask.h"
#include "MareaSettingsBuilder.h"
#include "MareaTargetDescription.h"

static void MareaBcPrintUsage(void)
{
    fprintf(
        stderr,
        "usage: marea-bc <mode> [options]\n"
        "  compile <input.marea> -o <output.mbc>\n"
        "  assemble <input.mbca> -o <output.mbc>\n"
        "  disassemble <input.mbc> -o <output.mbca>\n"
        "  dump-tokens <input.marea>\n"
        "  dump-ast <input.marea>\n"
        "  dump-ir <input.marea>\n"
        "options:\n"
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
    const char *mode;
    const char *inputPath;
    const char *outputPath;
    const char *librariesText;
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

    if (argumentCount < 2)
    {
        MareaBcPrintUsage();
        return 1;
    }

    if (strcmp(arguments[1], "--help") == 0)
    {
        MareaBcPrintUsage();
        return 0;
    }

    if (strcmp(arguments[1], "--version") == 0)
    {
        printf("marea-bc (marea_lang)\n");
        return 0;
    }

    if (MAREA_ArenaCreate(0, &argumentArena) != MAREA_ARENA_EXCEPT_SUCCESS)
    {
        fprintf(stderr, "error: cannot create the argument arena\n");
        return 1;
    }

    MareaSettingsBuilderInit(&settingsBuilder, argumentArena);

    mode = NULL;
    inputPath = NULL;
    outputPath = NULL;
    librariesText = NULL;
    includeDirectoryCount = 0;
    featureCount = 0;
    listSettings = MAREA_BOOLEAN_FALSE;

    /* arguments[1] is the mode only when it is not itself a flag, so a flag-only invocation such as
     * "marea-bc --list-settings --settings NAME=value" works with no mode at all. */
    if (arguments[1][0] != '-')
    {
        mode = arguments[1];
    }

    MareaTargetDescriptionDetectHost(&target);

    for (index = mode != NULL ? 2 : 1; index < argumentCount; index = index + 1)
    {
        const char *argument = arguments[index];

        if (strcmp(argument, "-o") == 0 && index + 1 < argumentCount)
        {
            outputPath = arguments[index + 1];
            index = index + 1;
        }
        else if (strcmp(argument, "--libraries") == 0 && index + 1 < argumentCount)
        {
            librariesText = arguments[index + 1];
            index = index + 1;
        }
        else if (strcmp(argument, "--settings") == 0 && index + 1 < argumentCount)
        {
            if (!MareaSettingsBuilderAddPair(&settingsBuilder, (const MAREA_Character8 *)arguments[index + 1]))
            {
                fprintf(stderr, "error: malformed --settings '%s' (expected NAME=value)\n", arguments[index + 1]);
                MAREA_ArenaDestroy(argumentArena);
                return 1;
            }

            index = index + 1;
        }
        else if (strcmp(argument, "--include") == 0 && index + 1 < argumentCount && includeDirectoryCount < MAREA_ARRAY_SIZE(includeDirectories))
        {
            includeDirectories[includeDirectoryCount] = arguments[index + 1];
            includeDirectoryCount = includeDirectoryCount + 1;
            index = index + 1;
        }
        else if (strcmp(argument, "--target-architecture") == 0 && index + 1 < argumentCount)
        {
            target.architecture = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        }
        else if (strcmp(argument, "--target-os") == 0 && index + 1 < argumentCount)
        {
            target.operatingSystem = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        }
        else if (strcmp(argument, "--target-compiler") == 0 && index + 1 < argumentCount)
        {
            target.compiler = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        }
        else if (strcmp(argument, "--target-endianness") == 0 && index + 1 < argumentCount)
        {
            target.endianness = (const MAREA_Character8 *)arguments[index + 1];
            index = index + 1;
        }
        else if (strcmp(argument, "--target-pointer-width") == 0 && index + 1 < argumentCount)
        {
            target.pointerWidth = (MAREA_Uinteger32)atoi(arguments[index + 1]);
            index = index + 1;
        }
        else if (strcmp(argument, "--target-feature") == 0 && index + 1 < argumentCount && featureCount < MAREA_ARRAY_SIZE(features))
        {
            features[featureCount] = arguments[index + 1];
            featureCount = featureCount + 1;
            index = index + 1;
        }
        else if (strcmp(argument, "--list-settings") == 0)
        {
            listSettings = MAREA_BOOLEAN_TRUE;
        }
        else if (inputPath == NULL)
        {
            inputPath = argument;
        }
        else
        {
            fprintf(stderr, "error: unexpected argument '%s'\n", argument);
            MareaBcPrintUsage();
            MAREA_ArenaDestroy(argumentArena);
            return 1;
        }
    }

    target.features = (const MAREA_Character8 *const *)features;
    target.featureCount = featureCount;

    if (listSettings)
    {
        MareaBcDriverListSettings(MareaSettingsBuilderBuild(&settingsBuilder), stdout);
        MAREA_ArenaDestroy(argumentArena);
        return 0;
    }

    if (!MareaLibraryMaskParse((const MAREA_Character8 *)librariesText, &libraryMask))
    {
        fprintf(stderr, "error: --libraries names an unknown library in '%s'\n", librariesText);
        MAREA_ArenaDestroy(argumentArena);
        return 1;
    }

    if (mode == NULL)
    {
        fprintf(stderr, "error: missing mode\n");
        MareaBcPrintUsage();
        MAREA_ArenaDestroy(argumentArena);
        return 1;
    }

    if (inputPath == NULL)
    {
        fprintf(stderr, "error: missing input file\n");
        MareaBcPrintUsage();
        MAREA_ArenaDestroy(argumentArena);
        return 1;
    }

    {
        MAREA_Boolean succeeded = MAREA_BOOLEAN_FALSE;

        if (strcmp(mode, "compile") == 0)
        {
            if (outputPath == NULL)
            {
                fprintf(stderr, "error: compile needs -o <output.mbc>\n");
                MAREA_ArenaDestroy(argumentArena);
                return 1;
            }

            succeeded = MareaBcDriverCompile(inputPath, outputPath, libraryMask, MareaSettingsBuilderBuild(&settingsBuilder), &target, (const MAREA_Character8 *const *)includeDirectories, includeDirectoryCount, stderr);
        }
        else if (strcmp(mode, "assemble") == 0)
        {
            if (outputPath == NULL)
            {
                fprintf(stderr, "error: assemble needs -o <output.mbc>\n");
                MAREA_ArenaDestroy(argumentArena);
                return 1;
            }

            succeeded = MareaBcDriverAssemble(inputPath, outputPath, stderr);
        }
        else if (strcmp(mode, "disassemble") == 0)
        {
            if (outputPath == NULL)
            {
                fprintf(stderr, "error: disassemble needs -o <output.mbca>\n");
                MAREA_ArenaDestroy(argumentArena);
                return 1;
            }

            succeeded = MareaBcDriverDisassemble(inputPath, outputPath, stderr);
        }
        else if (strcmp(mode, "dump-tokens") == 0)
        {
            succeeded = MareaBcDriverDumpTokens(inputPath, stdout, stderr);
        }
        else if (strcmp(mode, "dump-ast") == 0)
        {
            succeeded = MareaBcDriverDumpAst(inputPath, stdout, stderr);
        }
        else if (strcmp(mode, "dump-ir") == 0)
        {
            succeeded = MareaBcDriverDumpIr(inputPath, libraryMask, &target, (const MAREA_Character8 *const *)includeDirectories, includeDirectoryCount, stdout, stderr);
        }
        else
        {
            fprintf(stderr, "error: unknown mode '%s'\n", mode);
            MareaBcPrintUsage();
            MAREA_ArenaDestroy(argumentArena);
            return 1;
        }

        MAREA_ArenaDestroy(argumentArena);
        return succeeded ? 0 : 1;
    }
}
