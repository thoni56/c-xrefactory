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


PassMarker readPassMarker(char *optionText, int *passNumber) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (strncmp(optionText, "-pass", 5) != 0)
        return NotAPassMarker;

    char *digits = &optionText[5];
    if (digits[0] == '\0' || strspn(digits, "0123456789") != strlen(digits)) {
        sprintf(tmpBuff, "'%s' is not a pass marker of the form -passN, section ignored", optionText);
        errorMessage(ERR_ST, tmpBuff);
        return IllFormedPassMarker;
    }

    int passN = atoi(digits);
    if (passN > MAX_PASS_COUNT) {
        sprintf(tmpBuff, "pass number in '%s' is higher than the maximum of %d, section ignored",
                optionText, MAX_PASS_COUNT);
        errorMessage(ERR_ST, tmpBuff);
        return IllFormedPassMarker;
    }

    *passNumber = passN;
    return WellFormedPassMarker;
}

OptionSets makeOptionSets(void) {
    OptionSets d;

    for (size_t i = 0; i < sizeof(d.set) / sizeof(d.set[0]); i++)
        d.set[i] = NULL;
    return d;
}

void resetOptionSets(OptionSets *sets) {
    for (size_t i = 0; i < sizeof(sets->set) / sizeof(sets->set[0]); i++)
        freeStringList(sets->set[i]);
    *sets = makeOptionSets();
}

/* Pass number for a section we refuse to collect options from */
#define PASS_IGNORED (-1)

void readOptionSets(FILE *file, OptionSets *resultingDeltas) {
    int charsRead;
    char optionsText[MAX_OPTION_LEN];

    int passN = 0;
    int ch = getOptionFromFile(file, optionsText, &charsRead);
    while (ch != EOF) {
        int markerPass;
        PassMarker marker = readPassMarker(optionsText, &markerPass);

        if (optionsText[0] == '[')
            ; /* Skip section/project markers */
        else if (marker == WellFormedPassMarker)
            passN = markerPass;
        else if (marker == IllFormedPassMarker)
            passN = PASS_IGNORED; /* readPassMarker has told the user why */
        else if (passN != PASS_IGNORED) {
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
