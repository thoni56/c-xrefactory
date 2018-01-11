#ifndef _MATAB_H_
#define _MATAB_H_

/* Macro argument table - instance of hashtable */

#include "proto.h"

#define HASH_TAB_TYPE struct maTab
#define HASH_ELEM_TYPE S_macroArgTabElem
#define HASH_FUN_PREFIX maTab

#include "hashtab.th"

extern struct maTab s_maTab;

#ifndef _MATAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#endif
