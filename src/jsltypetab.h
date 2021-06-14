#ifndef JSLTYPETAB_H_INCLUDED
#define JSLTYPETAB_H_INCLUDED

#include "proto.h"

#define HASH_TAB_NAME jslTypeTab
#define HASH_TAB_TYPE JslTypeTab
#define HASH_ELEM_TYPE JslSymbolList

#include "hashlist.th"

#ifndef IN_JSLTYPETAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

#endif
