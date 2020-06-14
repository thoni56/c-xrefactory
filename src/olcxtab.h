#ifndef _OLCXTAB_H_
#define _OLCXTAB_H_

/* olcxTab - an instance of hashlist */

#include "proto.h"

typedef struct userOlcxData {
    char                        *name;
    struct olcxReferencesStack	browserStack;
    struct olcxReferencesStack	completionsStack;
    struct olcxReferencesStack	retrieverStack;
    struct classTreeData		classTree;
    struct userOlcxData			*next;
} UserOlcxData;

#define HASH_TAB_NAME olcxTab
#define HASH_TAB_TYPE OlcxTab
#define HASH_ELEM_TYPE UserOlcxData

#include "hashlist.th"

#ifndef _IN_OLCXTAB_C_
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern UserOlcxData *s_olcxCurrentUser;
extern OlcxTab s_olcxTab;

#endif
