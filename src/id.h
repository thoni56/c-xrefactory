#ifndef _ID_H_
#define _ID_H_

#include "symbol.h"
#include "position.h"


typedef struct id {
    char *name;
    struct symbol *sd;
    struct position	p;
    struct id *next;
} S_id;


extern void fill_id(S_id *id, char *name, Symbol *symbol, S_position position);

#endif
