#include "symbol.h"

#include "access.h"
#include "memory.h"
#include "storage.h"
#include "type.h"


void fillSymbol(Symbol *s, char *name, char *linkName, Position pos) {
    s->name = name;
    s->linkName = linkName;
    s->pos = pos;
    s->u.typeModifier = NULL;
    s->next = NULL;
    s->isExplicitlyImported = false;
    s->javaSourceIsLoaded = false;
    s->javaClassIsLoaded = false;
    s->access = AccessDefault;
    s->type = TypeDefault;
    s->storage = StorageDefault;
    s->npointers = 0;
}

Symbol makeSymbol(char *name, char *linkName, Position pos) {
    Symbol symbol;
    fillSymbol(&symbol, name, linkName, pos);
    return symbol;
}

Symbol makeSymbolWithBits(char *name, char *linkName, Position pos, Access access, Type type, Storage storage) {
    Symbol symbol;
    fillSymbol(&symbol, name, linkName, pos);
    symbol.access = access;
    symbol.type = type;
    symbol.storage = storage;
    return symbol;
}

void fillSymbolWithTypeModifier(Symbol *symbol, char *name, char *linkName, Position pos, struct typeModifier *typeModifier) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.typeModifier = typeModifier;
}

void fillSymbolWithStruct(Symbol *symbol, char *name, char *linkName, Position pos, struct symStructSpec *structSpec) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.structSpec = structSpec;
}

void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName, Position pos, int labelIndex) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.labelIndex = labelIndex;
}

Symbol *newSymbol(char *name, char *linkName, Position pos) {
    Symbol *s;
    s = StackMemoryAlloc(Symbol);
    *s = makeSymbol(name, linkName, pos);
    return s;
}

Symbol *newSymbolAsCopyOf(Symbol *original) {
    Symbol *s;
    s = StackMemoryAlloc(Symbol);
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
