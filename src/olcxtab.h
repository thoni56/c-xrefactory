#ifndef _OLCXTAB_H_
#define _OLCXTAB_H_

/* olcxTab - an instance of hashlist */

#include "proto.h"

#define HASH_TAB_NAME olcxTab
#define HASH_ELEM_TYPE S_userOlcx

#include "hashlist.th"

#ifndef _OLCXTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

extern S_olcxTab s_olcxTab;

#endif
