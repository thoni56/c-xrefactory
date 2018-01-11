#ifndef _OLCXTAB_H_
#define _OLCXTAB_H_

/* olcxTab - an instance of hashlist */

#include "proto.h"

typedef struct olcxTab S_olcxTab;
#define HASH_TAB_TYPE struct olcxTab
#define HASH_ELEM_TYPE S_userOlcx
#define HASH_FUN_PREFIX olcxTab

#include "hashlist.th"

#ifndef _OLCXTAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

extern S_olcxTab s_olcxTab;

#endif
