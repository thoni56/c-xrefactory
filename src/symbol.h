#ifndef SYMBOL_H_INCLUDED
#define SYMBOL_H_INCLUDED

/* Dependencies: */
#include "visibility.h"
#include "position.h"
#include "scope.h"
#include "storage.h"
#include "type.h"


/* ****************************************************************** */
/*              symbol definition item in symbol table                */

typedef struct symbol {
    char    *name;
    char    *linkName; /* fully qualified name for cx */
    Position pos;      /* definition position for most syms */
    unsigned npointers : 4; /* tmp. stored # of dcl. ptrs */
    Storage  storage : STORAGES_LN;
    Type     type : SYMTYPES_LN;
    /* can be Default/Struct/Union/Enum/Label/Keyword/Macro */
    union {
        struct typeModifier  *typeModifier; /* if type == TypeDefault */
        struct structSpec    *structSpec;   /* if type == TypeStruct/TypeUnion */
        struct symbolList    *enums;        /* if type == TypeEnum */
        struct macroBody     *mbody;        /* if type == TypeMacro, can be NULL! */
        int                   labelIndex;   /* break/continue label index */
        int                   keyword;      /* if bits.symbolType == Keyword */
    } u;
    struct symbol *next; /* next table item with the same hash */
} Symbol;

typedef struct symbolList {
    struct symbol     *element;
    struct symbolList *next;
} SymbolList;

/* Functions: */

/* NOTE These will not fill bit-fields, has to be done after allocation */
/* They all allocate in StackMemory... */

extern Symbol *newSymbol(char *name, char *linkName, Position pos);
extern Symbol *newSymbolAsCopyOf(Symbol *original);
extern Symbol *newSymbolAsKeyword(char *name, char *linkName, Position pos,
                                  int keyWordVal);
extern Symbol *newSymbolAsType(char *name, char *linkName, Position pos,
                               struct typeModifier *type);
extern Symbol *newSymbolAsEnum(char *name, char *linkName, Position pos,
                               struct symbolList *enums);
extern Symbol *newSymbolAsLabel(char *name, char *linkName, Position pos,
                                int labelIndex);

extern void fillSymbolWithTypeModifier(Symbol *symbol, char *name, char *linkName,
                               Position pos, struct typeModifier *typeModifier);
extern void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName,
                                Position pos, int labelIndex);

/* Create and return a symbol structure... */
extern Symbol makeSymbol(char *name, char *linkName, Position pos);

extern void getSymbolCxrefProperties(Symbol *symbol, Visibility *categoryP, Scope *scopeP,
                                     Storage *storageP);

#endif
