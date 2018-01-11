#ifndef _REFTAB_H_
#define _REFTAB_H_

#include "proto.h"

#define HASH_TAB_NAME refTab
#define HASH_TAB_TYPE struct HASH_TAB_NAME
#define HASH_ELEM_TYPE S_symbolRefItem
#define HASH_FUN_PREFIX HASH_TAB_NAME

#include "hashlist.th"

#ifndef _REFTAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#endif
