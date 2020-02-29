#include "symbol.h"

#include "misc.h"               /* For XX_ALLOC() */

void fillSymbol(S_symbol *s, char *name, char *linkName, struct position  pos) {
    s->name = name;
    s->linkName = linkName;
    s->pos = pos;
    /* s->bits is not assigned, all zeros? */
    s->u.type = NULL;
    s->next = NULL;
}


void fillSymbolWithType(S_symbol *symbol, char *name, char *linkName, struct position pos, struct typeModifiers *type) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.type = type;
}

void fillSymbolWithStruct(S_symbol *symbol, char *name, char *linkName, struct position pos, struct symStructSpec *structSpec) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.s = structSpec;
}

void fillSymbolWithLabel(S_symbol *symbol, char *name, char *linkName, struct position pos, int labelIndex) {
    fillSymbol(symbol, name, linkName, pos);
    symbol->u.labn = labelIndex;
}

S_symbol *newSymbol(char *name, char *linkName, struct position pos) {
    S_symbol *s;

    XX_ALLOC(s, S_symbol);
    fillSymbol(s, name, linkName, pos);

    return s;
}

S_symbol *newSymbolIsKeyword(char *name, char *linkName, struct position pos, int keyWordVal) {
    S_symbol *s = newSymbol(name, linkName, pos);
    s->u.keyWordVal = keyWordVal;
    return s;
}

S_symbol *newSymbolIsType(char *name, char *linkName, struct position pos, struct typeModifiers *type) {
    S_symbol *s = newSymbol(name, linkName, pos);
    s->u.type = type;
    return s;
}

S_symbol *newSymbolIsEnum(char *name, char *linkName, struct position pos, struct symbolList *enums) {
    S_symbol *s = newSymbol(name, linkName, pos);
    s->u.enums = enums;
    return s;
}

S_symbol *newSymbolIsLabel(char *name, char *linkName, struct position pos, int labelIndex) {
    S_symbol *s = newSymbol(name, linkName, pos);
    s->u.labn = labelIndex;
    return s;
}
