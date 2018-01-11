#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "proto.h"

typedef struct symTab S_symTab;

#define HASH_TAB_TYPE struct symTab
#define HASH_ELEM_TYPE S_symbol
#define HASH_FUN_PREFIX symTab

#include "hashlist.th"

#ifndef _SYMTAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#endif
