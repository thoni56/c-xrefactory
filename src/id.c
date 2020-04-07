#include "id.h"

void fill_id(S_id *id, char *name, Symbol *symbol, S_position position) {
    id->name = name;
    id->sd = symbol;
    id->p = position;
    id->next = NULL;
}
