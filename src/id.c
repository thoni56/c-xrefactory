#include "id.h"

void fill_id(S_id *id, char *name, Symbol *symbol, S_position position) {
    id->name = name;
    id->sd = symbol;
    id->p = position;
    id->next = NULL;
}

void fill_idList(S_idList *idList, S_id id, char *fname, Type nameType, S_idList *next) {
    idList->id = id;
    idList->fname = fname;
    idList->nameType = nameType;
    idList->next = next;
}

void fillf_idList(S_idList *idList, char *name, Symbol *symbol,
                  S_position position,
                  char *fname, Type nameType, S_idList *next) {
    idList->id.name = name;
    idList->id.sd = symbol;
    idList->id.p = position;
    idList->id.next = NULL;
    idList->fname = fname;
    idList->nameType = nameType;
    idList->next = next;
}
