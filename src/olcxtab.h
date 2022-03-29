#ifndef OLCXTAB_H_INCLUDED
#define OLCXTAB_H_INCLUDED

#include "proto.h"

typedef struct SessionData {
    char                        *name;
    struct olcxReferencesStack	browserStack;
    struct olcxReferencesStack	completionsStack;
    struct olcxReferencesStack	retrieverStack;
    struct classTreeData		classTree;
    struct SessionData			*next;
} SessionData;


extern SessionData *currentUserData;

#endif
