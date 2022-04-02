#define _SYMTAB_
#include "symboltable.h"

#include "hash.h"

#include "memory.h"


#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (e1->bits.symbolType==e2->bits.symbolType && strcmp(e1->name,e2->name)==0)

SymbolTable *symbolTable;

#include "hashlist.tc"


void initSymbolTable(int size) {
    symbolTable = StackMemoryAlloc(SymbolTable);
    symbolTableInit(symbolTable, size);
}


int addToSymbolTable(Symbol *symbol) {
    return symbolTableAdd(symbolTable, symbol);
}


Symbol *getSymbol(int index) {
    return symbolTable->tab[index];
}


int getNextExistingSymbol(int index) {
    for (int i = index; i < symbolTable->size; i++)
        if (symbolTable->tab[i] != NULL)
            return i;
    return -1;
}

/* Java uses JavaStat.locals as a symboltable ("trail"?) so must use symtab argument */
void addSymbolNoTrail(SymbolTable *symtab, Symbol *symbol) {
    int i;
    Symbol *memb;

    //assert(symtab == symbolTable);
    symbolTableIsMember(symtab, symbol, &i, &memb);
    symbolTableSet(symtab, symbol, i);
}
