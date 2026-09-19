#ifndef ID_H_INCLUDED
#define ID_H_INCLUDED

#include "symbol.h"
#include "position.h"


typedef struct Id {
    char *name;
    struct symbol *symbol;
    struct position position;
    struct Id *next;
} Id;

typedef struct idList {
    Id             id;
    struct idList *next;
} IdList;

extern Id makeId(char *name, Symbol *symbol, Position position);
extern IdList makeIdList(Id id, IdList *next);

#endif
