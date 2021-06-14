#ifndef SYMTAB_H_INCLUDED
#define SYMTAB_H_INCLUDED

/* symTab - an instance of hashlist */

#include "symbol.h"

#define HASH_TAB_NAME symbolTable
#define HASH_TAB_TYPE SymbolTable
#define HASH_ELEM_TYPE Symbol


#include "hashlist.th"


extern SymbolTable *s_symbolTable;

#ifndef _SYMTAB_
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

#endif
