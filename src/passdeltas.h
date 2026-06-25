#pragma once

#include "stringlist.h"

#define MAX_PASS_COUNT 9

typedef struct {
    StringList *delta[MAX_PASS_COUNT + 1];
} PassDeltas;
