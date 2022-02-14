#ifndef ID_H_INCLUDED
#define ID_H_INCLUDED

#include "type.h"
#include "symbol.h"
#include "position.h"

typedef struct id {
    char *          name;
    struct symbol * symbol;
    struct position position;
    struct id *     next;
} Id;

typedef struct idList {
    Id      id;
    char *         fqtname;  /* fqt name for java */
    enum type      nameType; /* type of name segment for java */
    struct idList *next;
} IdList;


extern void fillId(Id *id, char *name, Symbol *symbol, Position position);

extern void fillIdList(IdList *idList, Id id, char *fname, Type nameType, IdList *next);
extern void fillfIdList(IdList *idList, char *name, Symbol *symbol, Position position, char *fname, Type nameType,
                        IdList *next);
extern Id *newCopyOfId(Id *id);

#endif
