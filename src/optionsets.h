#pragma once

#include <stdio.h>

#include "argumentsvector.h"
#include "memory.h"
#include "stringlist.h"


#define MAX_PASS_COUNT 9

typedef struct {
    StringList *set[MAX_PASS_COUNT + 1];
} OptionSets;

extern OptionSets makeOptionSets(void);
extern void readOptionSets(FILE *file, OptionSets *resultingDeltas);

extern void applyOptionSet(StringList *optionList);

extern ArgumentsVector argsFromOptionList(StringList *options, Memory *memory);

extern void readOptionSetsFromFile(char *fileName, OptionSets *resultingOptionSets);
