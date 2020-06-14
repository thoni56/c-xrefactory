#define _SYMTAB_
#include "symboltable.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (e1->bits.symType==e2->bits.symType && strcmp(e1->name,e2->name)==0)

SymbolTable *s_symbolTable;

#include "hashlist.tc"
