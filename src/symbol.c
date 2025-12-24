#include "symbol.h"

#include <stddef.h>

#include "visibility.h"
#include "stackmemory.h"
#include "scope.h"
#include "storage.h"
#include "type.h"


static void fillSymbol(Symbol *symbol, char *name, char *linkName, Position pos) {
    symbol->name = name;
    symbol->linkName = linkName;
    symbol->position = pos;
    symbol->typeModifier = NULL;
    symbol->next = NULL;
    symbol->type = TypeDefault;
    symbol->storage = StorageDefault;
    symbol->npointers = 0;
}

/* Storage = StorageDefault */
Symbol makeSymbol(char *name, Type type, Position pos) {
    Symbol symbol;
    fillSymbol(&symbol, name, name, pos);
    symbol.type = type;
    return symbol;
}

Symbol makeMacroSymbol(char *name, Position pos) {
    Symbol symbol = {.name = name, .linkName = name, .position = pos, .type = TypeMacro, .storage = StorageDefault};
    return symbol;
}

void fillSymbolWithTypeModifier(Symbol *symbol, char *name, char *linkName, Position pos,
                                struct typeModifier *typeModifier) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->typeModifier = typeModifier;
}

void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName, Position pos, int labelIndex) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->labelIndex = labelIndex;
}

/* Allocate and init a new Symbol in StackMemory */
Symbol *newSymbol(char *name, Position pos) {
    Symbol *s;
    s = stackMemoryAlloc(sizeof(Symbol));
    *s = makeSymbol(name, TypeDefault, pos);
    return s;
}

Symbol *newSymbolAsCopyOf(Symbol *original) {
    Symbol *s;
    s = stackMemoryAlloc(sizeof(Symbol));
    *s = *original;
    return s;
}

Symbol *newSymbolAsKeyword(char *name, char *linkName, Position pos, int keyWordVal) {
    Symbol *s = newSymbol(name, pos);
    s->keyword = keyWordVal;
    return s;
}

Symbol *newSymbolAsType(char *name, char *linkName, Position pos, struct typeModifier *type) {
    Symbol *s = newSymbol(name, pos);
    s->typeModifier = type;
    return s;
}

Symbol *newSymbolAsEnum(char *name, char *linkName, Position pos, struct symbolList *enums) {
    Symbol *s = newSymbol(name, pos);
    s->enums = enums;
    return s;
}

Symbol *newSymbolAsLabel(char *name, char *linkName, Position pos, int labelIndex) {
    Symbol *s = newSymbol(name, pos);
    s->labelIndex = labelIndex;
    return s;
}

void getSymbolCxrefProperties(Symbol *symbol, Visibility *visibilityP, Scope *scopeP,
                              Storage *storageP) {
    int visibility, scope, storage;

    visibility = VisibilityLocal; scope = AutoScope; storage=StorageAuto;
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
                visibility = VisibilityLocal;
            } else {
                visibility = VisibilityGlobal;
            }
            scope = GlobalScope;
        }
    }
    /* enumeration constants */
    if (symbol->type==TypeDefault && symbol->storage==StorageConstant) {
        visibility = VisibilityGlobal;  scope = GlobalScope; storage=StorageExtern;
    }
    /* struct, union, enum */
    if ((symbol->type==TypeStruct||symbol->type==TypeUnion||symbol->type==TypeEnum)){
        visibility = VisibilityGlobal;  scope = GlobalScope; storage=StorageExtern;
    }
    /* macros */
    if (symbol->type == TypeMacro) {
        visibility = VisibilityGlobal;  scope = GlobalScope; storage=StorageExtern;
    }
    if (symbol->type == TypeLabel) {
        visibility = VisibilityLocal; scope = FileScope; storage=StorageStatic;
    }
    if (symbol->type == TypeCppIfElse) {
        visibility = VisibilityLocal; scope = FileScope; storage=StorageStatic;
    }
    if (symbol->type == TypeCppInclude) {
        visibility = VisibilityGlobal; scope = GlobalScope; storage=StorageExtern;
    }
    if (symbol->type == TypeCppCollate) {
        visibility = VisibilityGlobal; scope = GlobalScope; storage=StorageExtern;
    }
    if (symbol->type == TypeYaccSymbol) {
        visibility = VisibilityLocal; scope = FileScope; storage=StorageStatic;
    }

    *visibilityP = visibility;
    *scopeP = scope;
    *storageP = storage;
}
