#ifndef _ID_H_
#define _ID_H_

#include "type.h"
#include "symbol.h"
#include "position.h"


typedef struct id {
    char *name;
    struct symbol *sd;
    struct position	p;
    struct id *next;
} S_id;

typedef struct idList {
    struct id id;
    char *fname;                /* fqt name for java */
    enum type nameType;             /* type of name segment for java */
    struct idList *next;
} S_idList;


extern void fillId(S_id *id, char *name, Symbol *symbol, S_position position);

extern void fillIdList(S_idList *idList, S_id id, char *fname, Type nameType, S_idList *next);
extern void fillfIdList(S_idList *idList, char *name, Symbol *symbol, S_position position,
                         char *fname, Type nameType, S_idList *next);

#endif
