#pragma once

#include "stringlist.h"

#define MAX_PASS_COUNT 9

typedef struct {
    StringList *set[MAX_PASS_COUNT + 1];
} OptionSets;
