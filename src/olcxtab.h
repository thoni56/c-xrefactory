#ifndef OLCXTAB_H_INCLUDED
#define OLCXTAB_H_INCLUDED

/* olcxTab - an instance of hashlist */

#include "proto.h"

typedef struct SessionData {
    char                        *name;
    struct olcxReferencesStack	browserStack;
    struct olcxReferencesStack	completionsStack;
    struct olcxReferencesStack	retrieverStack;
    struct classTreeData		classTree;
    struct SessionData			*next;
} SessionData;

#define HASH_TAB_NAME olcxTab
#define HASH_TAB_TYPE OlcxTab
#define HASH_ELEM_TYPE SessionData

#include "hashlist.th"

#ifndef IN_OLCXTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern SessionData *currentUserData;
extern OlcxTab s_olcxTab;

#endif
