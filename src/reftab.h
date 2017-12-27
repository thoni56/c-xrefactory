#ifndef REFTAB_H
#define REFTAB_H

#include "proto.h"

#define HASH_TAB_TYPE struct refTab
#define HASH_ELEM_TYPE S_symbolRefItem
#define HASH_FUN_PREFIX refTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#endif
