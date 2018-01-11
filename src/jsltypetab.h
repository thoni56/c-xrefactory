#ifndef _JSLTYPETAB_H_
#define _JSLTYPETAB_H_

#include "proto.h"

#define HASH_TAB_NAME jslTypeTab
#define HASH_ELEM_TYPE S_jslSymbolList

#include "hashlist.th"

#ifndef _JSLTYPETAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
