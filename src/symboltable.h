#ifndef SYMTAB_H_INCLUDED
#define SYMTAB_H_INCLUDED

/* An instance of hashlist */

#include "symbol.h"

#define HASH_TAB_NAME symbolTable
#define HASH_TAB_TYPE SymbolTable
#define HASH_ELEM_TYPE Symbol


#include "hashlist.th"


extern SymbolTable *symbolTable;

#ifndef IN_SYMBOLTABLE_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void initSymbolTable(int size);
extern int  addToSymbolTable(Symbol *symbol);
extern void addSymbolToTable(SymbolTable *table, Symbol *symbol);
extern Symbol *getSymbol(int index);
extern int getNextExistingSymbol(int index);

#endif
