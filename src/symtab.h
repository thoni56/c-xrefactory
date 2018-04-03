#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "proto.h"

#define HASH_TAB_NAME symTab
#define HASH_ELEM_TYPE S_symbol

#include "hashlist.th"

/*
  run the following command in shell to get the expanded declarations of hashlist

  gcc -E symtab.h

*/

#ifndef _SYMTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
