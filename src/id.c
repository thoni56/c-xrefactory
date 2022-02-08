#include "id.h"

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

void fillfIdList(IdList *idList, char *name, Symbol *symbol, Position position, char *fqtname, Type nameType,
                 IdList *next) {
    idList->id.name     = name;
    idList->id.symbol   = symbol;
    idList->id.position = position;
    idList->id.next     = NULL;
    idList->fqtname     = fqtname;
    idList->nameType    = nameType;
    idList->next        = next;
}

Id *newCopyOfId(Id *id) {
    Id *copy = StackMemoryAlloc(Id);
    *(copy)  = *(id);
    return copy;
}
