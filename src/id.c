#include "id.h"

#include <stdio.h>
#include "stackmemory.h"


static void fillId(Id *id, char *name, Symbol *symbol, Position position) {
    id->name     = name;
    id->symbol   = symbol;
    id->position = position;
    id->next     = NULL;
}

Id makeId(char *name, Symbol *symbol, Position position) {
    Id id;
    fillId(&id, name, symbol, position);
    return id;
}

static void fillIdList(IdList *idList, Id id, char *fqtname, Type nameType, IdList *next) {
    idList->id       = id;
    idList->fqtname  = fqtname;
    idList->nameType = nameType;
    idList->next     = next;
}

IdList makeIdList(Id id, char *fname, Type nameType, IdList *next) {
    IdList l;
    fillIdList(&l, id, fname, nameType, next);
    return l;
}
