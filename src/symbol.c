#include "symbol.h"

#include "access.h"
#include "memory.h"
#include "storage.h"
#include "type.h"


/* Symbol bits */

SymbolBits makeSymbolBits(Access access, Type symbolType, Storage storage) {
    return (SymbolBits){.access = access, .type = symbolType, .storage = storage};
}

void fillSymbol(Symbol *s, char *name, char *linkName, Position pos) {
    s->name = name;
    s->linkName = linkName;
    s->pos = pos;
    s->u.typeModifier = NULL;
    s->next = NULL;
    s->bits.isExplicitlyImported = false;
    s->bits.javaSourceIsLoaded = false;
    s->bits.javaClassIsLoaded = false;
    s->bits.access = AccessDefault;
    s->bits.type = TypeDefault;
    s->bits.storage = StorageDefault;
    s->bits.npointers = 0;
}

Symbol makeSymbol(char *name, char *linkName, Position pos) {
    Symbol symbol;
    fillSymbol(&symbol, name, linkName, pos);
    return symbol;
}

Symbol makeSymbolWithBits(char *name, char *linkName, Position pos, Access access, Type type, Storage storage) {
    Symbol symbol;
    fillSymbol(&symbol, name, linkName, pos);
    symbol.bits.access = access;
    symbol.bits.type = type;
    symbol.bits.storage = storage;
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
