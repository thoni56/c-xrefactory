#include "id.h"

#include <stdio.h>


Id makeId(char *name, Symbol *symbol, Position position) {
    Id id;
    id.name     = name;
    id.symbol   = symbol;
    id.position = position;
    id.next     = NULL;
    return id;
}

IdList makeIdList(Id id, IdList *next) {
    IdList l;
    l.id = id;
    l.next = next;
    return l;
}
