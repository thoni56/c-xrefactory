#ifndef SYMTAB_H
#define SYMTAB_H

typedef struct symTab S_symTab; /* TODO this should be done automatically by the .th ... */

#define HASH_TAB_TYPE struct symTab
#define HASH_ELEM_TYPE S_symbol
#define HASH_FUN_PREFIX symTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#endif
