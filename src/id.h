#ifndef ID_H_INCLUDED
#define ID_H_INCLUDED

#include "type.h"
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
    char          *fqtname;  /* fqt name for java */
    Type           nameType; /* type of name segment for java */
    struct idList *next;
} IdList;

extern Id makeId(char *name, Symbol *symbol, Position position);
extern IdList makeIdList(Id id, char *fname, Type nameType, IdList *next);

#endif
