#ifndef _JSLTYPETAB_H_
#define _JSLTYPETAB_H_

#include "proto.h"

#define HASH_TAB_TYPE struct jslTypeTab
#define HASH_ELEM_TYPE S_jslSymbolList
#define HASH_FUN_PREFIX jslTypeTab

#include "hashlist.th"

#ifndef _JSLTYPETAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#endif
