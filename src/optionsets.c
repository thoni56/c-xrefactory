#include "optionsets.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "constants.h"
#include "fileio.h"
#include "list.h"
#include "options.h"


OptionSets makeOptionSets(void) {
    OptionSets d;

    for (size_t i = 0; i < sizeof(d.set) / sizeof(d.set[0]); i++)
        d.set[i] = NULL;
    return d;
}

/* Pass number for a section we refuse to collect options from */
#define PASS_IGNORED (-1)

void readOptionSets(FILE *file, OptionSets *resultingDeltas) {
    int charsRead;
    char optionsText[MAX_OPTION_LEN];

    int passN = 0;
    int ch = getOptionFromFile(file, optionsText, &charsRead);
    while (ch != EOF) {
        if (optionsText[0] == '[')
            ; /* Skip section/project markers */
        else if (strncmp(optionsText, "-pass", 5) == 0 && isdigit(optionsText[5])) {
            passN = atoi(&optionsText[5]);
            if (passN > MAX_PASS_COUNT) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "pass number in '%s' is higher than the maximum of %d, section ignored",
                        optionsText, MAX_PASS_COUNT);
                errorMessage(ERR_ST, tmpBuff);
                passN = PASS_IGNORED;
            }
        } else if (passN != PASS_IGNORED) {
            assert(passN >= 0 && passN <= MAX_PASS_COUNT);
            LIST_APPEND(StringList, resultingDeltas->set[passN], newStringList(optionsText, NULL));
        }
        ch = getOptionFromFile(file, optionsText, &charsRead);
    }
}

void readOptionSetsFromFile(char *fileName, OptionSets *resultingOptionSets) {
    FILE *file = openFile(fileName, "r");
    if (file == NULL)
        return;
    readOptionSets(file, resultingOptionSets);
    closeFile(file);
}

ArgumentsVector argsFromOptionList(StringList *options, Memory *memory) {
    ArgumentsVector args = {.argc = 1, .argv = NULL};

    int argCount;
    LIST_LEN(argCount, StringList, options);

    args.argv = memoryAlloc(memory, (argCount + 1) * sizeof(args.argv[0]));
    for (StringList *o = options; o != NULL; o = o->next) {
        args.argv[args.argc] = memoryAlloc(memory, strlen(o->string) + 1);
        strcpy(args.argv[args.argc], o->string);
        args.argc++;
    }

    return args;
}

void applyOptionSet(StringList *optionList) {
    if (optionList == NULL)
        return;
    ArgumentsVector args = argsFromOptionList(optionList, &options.memory);
    processOptions(args, PROCESS_FILE_ARGUMENTS_NO);
}
