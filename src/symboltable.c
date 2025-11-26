#define IN_SYMBOLTABLE_C
#include "symboltable.h"
#include "log.h"
#include "memory.h"


#define HASH_FUN(element) hashFun(element->name)
#define HASH_ELEM_EQUAL(e1,e2) (e1->type==e2->type && strcmp(e1->name,e2->name)==0)

SymbolTable *symbolTable;

#include "hashlist.tc"


void initSymbolTable(int size) {
    symbolTable = stackMemoryAlloc(sizeof(SymbolTable));
    log_debug("initSymbolTable: symbolTable allocated at %p", symbolTable);
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

void recoverSymbolTableMemory(void) {
    /* Remove symbols that point to freed ppmMemory.
     * This is called after ppmMemory.index has been reset to clear file-local macros
     * (including header guards) while preserving predefined macros.
     */
    for (int i = 0; i < symbolTable->size; i++) {
        Symbol **pp = &symbolTable->tab[i];
        while (*pp != NULL) {
            if (ppmIsFreedPointer(*pp)) {
                /* Symbol itself points to freed memory - remove it */
                *pp = (*pp)->next;
                continue;
            }
            pp = &(*pp)->next;
        }
    }
}

