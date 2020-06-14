#ifndef _JSLTYPETAB_H_
#define _JSLTYPETAB_H_

#include "proto.h"

#define HASH_TAB_NAME jslTypeTab
#define HASH_TAB_TYPE JslTypeTab
#define HASH_ELEM_TYPE JslSymbolList

#include "hashlist-new.th"

#ifndef _IN_JSLTYPETAB_C_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
