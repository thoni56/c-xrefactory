#define _SYMTAB_
#include "symboltable.h"


#define HASH_FUN(element) hashFun(element->name)
#define HASH_ELEM_EQUAL(e1,e2) (e1->type==e2->type && strcmp(e1->name,e2->name)==0)

SymbolTable *symbolTable;

#include "hashlist.tc"


void initSymbolTable(int size) {
    symbolTable = stackMemoryAlloc(sizeof(SymbolTable));
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

void addSymbolToTable(SymbolTable *table, Symbol *symbol) {
    int i;

    symbolTableIsMember(table, symbol, &i, NULL);
    symbolTablePush(table, symbol, i);
}
