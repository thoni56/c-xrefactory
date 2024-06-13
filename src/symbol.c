#include "symbol.h"

#include <stddef.h>

#include "category.h"
#include "stackmemory.h"
#include "scope.h"
#include "storage.h"
#include "type.h"


static void fillSymbol(Symbol *symbol, char *name, char *linkName, Position pos) {
    symbol->name = name;
    symbol->linkName = linkName;
    symbol->pos = pos;
    symbol->u.typeModifier = NULL;
    symbol->next = NULL;
    symbol->type = TypeDefault;
    symbol->storage = StorageDefault;
    symbol->npointers = 0;
}

Symbol makeSymbol(char *name, char *linkName, Position pos) {
    Symbol symbol;
    fillSymbol(&symbol, name, linkName, pos);
    return symbol;
}

void fillSymbolWithTypeModifier(Symbol *symbol, char *name, char *linkName, Position pos,
                                struct typeModifier *typeModifier) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.typeModifier = typeModifier;
}

void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName, Position pos, int labelIndex) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.labelIndex = labelIndex;
}

/* Allocate and init a new Symbol in StackMemory */
Symbol *newSymbol(char *name, char *linkName, Position pos) {
    Symbol *s;
    s = stackMemoryAlloc(sizeof(Symbol));
    *s = makeSymbol(name, linkName, pos);
    return s;
}

Symbol *newSymbolAsCopyOf(Symbol *original) {
    Symbol *s;
    s = stackMemoryAlloc(sizeof(Symbol));
    *s = *original;
    return s;
}

Symbol *newSymbolAsKeyword(char *name, char *linkName, Position pos, int keyWordVal) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.keyword = keyWordVal;
    return s;
}

Symbol *newSymbolAsType(char *name, char *linkName, Position pos, struct typeModifier *type) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.typeModifier = type;
    return s;
}

Symbol *newSymbolAsEnum(char *name, char *linkName, Position pos, struct symbolList *enums) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.enums = enums;
    return s;
}

Symbol *newSymbolAsLabel(char *name, char *linkName, Position pos, int labelIndex) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.labelIndex = labelIndex;
    return s;
}

void getSymbolCxrefProperties(Symbol *symbol, ReferenceCategory *categoryP, ReferenceScope *scopeP, Storage *storageP) {
    int category, scope, storage;

    category = CategoryLocal; scope = ScopeAuto; storage=StorageAuto;
    /* default */
    if (symbol->type==TypeDefault) {
        storage = symbol->storage;
        if (symbol->storage==StorageExtern
            || symbol->storage==StorageDefault
            || symbol->storage==StorageTypedef
            || symbol->storage==StorageStatic
            || symbol->storage==StorageThreadLocal
        ) {
            if (symbol->linkName[0]==' ' && symbol->linkName[1]==' ') {
                // a special symbol local linkname
                category = CategoryLocal;
            } else {
                category = CategoryGlobal;
            }
            scope = ScopeGlobal;
        }
    }
    /* enumeration constants */
    if (symbol->type==TypeDefault && symbol->storage==StorageConstant) {
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }
    /* struct, union, enum */
    if ((symbol->type==TypeStruct||symbol->type==TypeUnion||symbol->type==TypeEnum)){
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }
    /* macros */
    if (symbol->type == TypeMacro) {
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }
    if (symbol->type == TypeLabel) {
        category = CategoryLocal; scope = ScopeFile; storage=StorageStatic;
    }
    if (symbol->type == TypeCppIfElse) {
        category = CategoryLocal; scope = ScopeFile; storage=StorageStatic;
    }
    if (symbol->type == TypeCppInclude) {
        category = CategoryGlobal; scope = ScopeGlobal; storage=StorageExtern;
    }
    if (symbol->type == TypeCppCollate) {
        category = CategoryGlobal; scope = ScopeGlobal; storage=StorageExtern;
    }
    if (symbol->type == TypeYaccSymbol) {
        category = CategoryLocal; scope = ScopeFile; storage=StorageStatic;
    }

    *categoryP = category;
    *scopeP = scope;
    *storageP = storage;
}
