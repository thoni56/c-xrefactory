#include "id.h"

void fillId(Id *id, char *name, Symbol *symbol, Position position) {
    id->name = name;
    id->sd = symbol;
    id->p = position;
    id->next = NULL;
}

void fillIdList(IdList *idList, Id id, char *fname, Type nameType, IdList *next) {
    idList->id = id;
    idList->fname = fname;
    idList->nameType = nameType;
    idList->next = next;
}

void fillfIdList(IdList *idList, char *name, Symbol *symbol,
                  Position position,
                  char *fname, Type nameType, IdList *next) {
    idList->id.name = name;
    idList->id.sd = symbol;
    idList->id.p = position;
    idList->id.next = NULL;
    idList->fname = fname;
    idList->nameType = nameType;
    idList->next = next;
}
