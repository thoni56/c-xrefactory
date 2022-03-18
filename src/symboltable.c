#define _SYMTAB_
#include "symboltable.h"

#include "hash.h"

#include "memory.h"


#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (e1->bits.symbolType==e2->bits.symbolType && strcmp(e1->name,e2->name)==0)

SymbolTable *symbolTable;

#include "hashlist.tc"


void initSymbolTable(void) {}


int getNextExistingSymbol(int index) {return -1;}
