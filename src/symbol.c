#include "symbol.h"

#include "misc.h"               /* For XX_ALLOC() */

S_symbol *newSymbol(char *name, char *linkName, struct position pos, S_symbol *next) {
    S_symbol *s;
    XX_ALLOC(s, S_symbol);
    s->name = name;
    s->linkName = linkName;
    s->pos = pos;
    s->next = next;
    return s;
}

S_symbol *newSymbolKeyword(char *name, char *linkName, struct position pos, int keyWordVal, S_symbol *next) {
    S_symbol *s = newSymbol(name, linkName, pos, next);
    s->u.keyWordVal = keyWordVal;
    return s;
}
