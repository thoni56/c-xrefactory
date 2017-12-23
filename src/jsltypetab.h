#ifndef JSLTYPETAB_H
#define JSLTYPETAB_H

#define HASH_TAB_TYPE struct jslTypeTab
#define HASH_ELEM_TYPE S_jslSymbolList
#define HASH_FUN_PREFIX jslTypeTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#endif
