#ifndef _REFTAB_H_
#define _REFTAB_H_

#include "proto.h"

#define HASH_TAB_NAME refTab
#define HASH_ELEM_TYPE S_symbolRefItem

#include "hashlist.th"

/* TODO: this is somewhat irregular naming, "cx" or not? */
extern S_refTab s_cxrefTab;

#ifndef _REFTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
