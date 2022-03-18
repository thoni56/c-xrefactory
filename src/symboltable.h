#ifndef SYMTAB_H_INCLUDED
#define SYMTAB_H_INCLUDED

/* An instance of hashlist */

#include "symbol.h"

#define HASH_TAB_NAME symbolTable
#define HASH_TAB_TYPE SymbolTable
#define HASH_ELEM_TYPE Symbol


#include "hashlist.th"


extern SymbolTable *symbolTable;

#ifndef _SYMTAB_
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void initSymbolTable(void);
extern int getNextExistingSymbol(int index);

#endif
