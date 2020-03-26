#ifndef _OLCXTAB_H_
#define _OLCXTAB_H_

/* olcxTab - an instance of hashlist */

#include "proto.h"

typedef struct userOlcx {
    char                        *name;
    struct olcxReferencesStack	browserStack;
    struct olcxReferencesStack	completionsStack;
    struct olcxReferencesStack	retrieverStack;
    struct classTreeData		classTree;
    struct userOlcx				*next;
} S_userOlcx;

#define HASH_TAB_NAME olcxTab
#define HASH_ELEM_TYPE S_userOlcx

#include "hashlist.th"

#ifndef _OLCXTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

extern S_userOlcx *s_olcxCurrentUser;
extern S_olcxTab s_olcxTab;

#endif
