#include "symbol.h"

#include "memory.h"


void fillSymbolBits(SymbolBits *bits, Access access, Type symType, Storage storage) {
    memset(bits, 0, sizeof(SymbolBits));
    bits->access = access;
    bits->symbolType = symType;
    bits->storage = storage;
}

void fillSymbol(Symbol *s, char *name, char *linkName, struct position pos) {
    s->name = name;
    s->linkName = linkName;
    s->pos = pos;
    s->u.type = NULL;
    s->next = NULL;
    fillSymbolBits(&s->bits, AccessDefault, TypeDefault, StorageDefault);
}


void fillSymbolWithType(Symbol *symbol, char *name, char *linkName, struct position pos, struct typeModifier *type) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.type = type;
}

void fillSymbolWithStruct(Symbol *symbol, char *name, char *linkName, struct position pos, struct symStructSpec *structSpec) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.s = structSpec;
}

void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName, struct position pos, int labelIndex) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.labelIndex = labelIndex;
}

Symbol *newSymbol(char *name, char *linkName, struct position pos) {
    Symbol *s;
    s = StackMemoryAlloc(Symbol);
    fillSymbol(s, name, linkName, pos);
    return s;
}

Symbol *newSymbolAsCopyOf(Symbol *original) {
    Symbol *s;
    s = StackMemoryAlloc(Symbol);
    *s = *original;
    return s;
}

Symbol *newSymbolAsKeyword(char *name, char *linkName, struct position pos, int keyWordVal) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.keyword = keyWordVal;
    return s;
}

Symbol *newSymbolAsType(char *name, char *linkName, struct position pos, struct typeModifier *type) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.type = type;
    return s;
}

Symbol *newSymbolAsEnum(char *name, char *linkName, struct position pos, struct symbolList *enums) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.enums = enums;
    return s;
}

Symbol *newSymbolAsLabel(char *name, char *linkName, struct position pos, int labelIndex) {
    Symbol *s = newSymbol(name, linkName, pos);
    s->u.labelIndex = labelIndex;
    return s;
}
