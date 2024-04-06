#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "editor.h"
#include "proto.h"

typedef struct pushAllInBetweenData {
    int lowestIndex;
    int highestIndex;
} PushRange; /* WTF is this, memory indices, but why
 "pushAllInBetween"?  Shared between refactory.c and cxref.c */


extern void refactory(void);

#endif
