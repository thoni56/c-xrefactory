#include "id.h"

#include <stdio.h>
#include "stackmemory.h"


void fillId(Id *id, char *name, Symbol *symbol, Position position) {
    id->name     = name;
    id->symbol   = symbol;
    id->position = position;
    id->next     = NULL;
}

void fillIdList(IdList *idList, Id id, char *fqtname, Type nameType, IdList *next) {
    idList->id       = id;
    idList->fqtname  = fqtname;
    idList->nameType = nameType;
    idList->next     = next;
}

Id *newCopyOfId(Id *id) {
    Id *copy = stackMemoryAlloc(sizeof(Id));
    *(copy)  = *(id);
    return copy;
}
