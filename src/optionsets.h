#pragma once

#include <stdio.h>

#include "argumentsvector.h"
#include "memory.h"
#include "stringlist.h"


#define MAX_PASS_COUNT 9

typedef struct {
    StringList *set[MAX_PASS_COUNT + 1];
} OptionSets;

/* What an option read from a config file is, as a pass marker */
typedef enum {
    NotAPassMarker,       /* an ordinary option, not a marker at all */
    WellFormedPassMarker, /* -passN with N in range; the number is returned */
    IllFormedPassMarker   /* meant as a marker but unusable; reported to the user */
} PassMarker;

/* Both config-file readers decide what a section belongs to with this, so that
 * '-passfoo' and '-pass12' mean the same thing to each of them. */
extern PassMarker readPassMarker(char *optionText, int *passNumber);

extern OptionSets makeOptionSets(void);
/* For a set that already owns its lists; makeOptionSets() is for a fresh one. */
extern void resetOptionSets(OptionSets *sets);
extern void readOptionSets(FILE *file, OptionSets *resultingDeltas);

extern void applyOptionSet(StringList *optionList);

extern ArgumentsVector argsFromOptionList(StringList *options, Memory *memory);

extern void readOptionSetsFromFile(char *fileName, OptionSets *resultingOptionSets);
