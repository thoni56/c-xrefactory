#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* symTab - an instance of hashlist */

#include "symbol.h"

#define HASH_TAB_NAME symbolTable
#define HASH_TAB_TYPE SymbolTable
#define HASH_ELEM_TYPE Symbol


#include "hashlist.th"

/*
  run the following command in shell to see the expanded declarations of hashlist

  gcc -E symtab.h

*/

extern SymbolTable *s_symbolTable;

#ifndef _SYMTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
