#ifndef SYMBOL_H_INCLUDED
#define SYMBOL_H_INCLUDED

/* Dependencies: */
#include "access.h"
#include "category.h"
#include "position.h"
#include "scope.h"
#include "storage.h"
#include "type.h"


/* ****************************************************************** */
/*              symbol definition item in symbol table                */

typedef struct symbol {
    char    *name;
    char    *linkName; /* fully qualified name for cx */
    Position pos;      /* definition position for most syms;
                          import position for imported classes! */
    unsigned npointers : 4; /* tmp. stored #of dcl. ptrs */
    Access   access : 12;              /* java access bits */
    Storage  storage : STORAGES_LN;
    Type     type : SYMTYPES_LN;
    /* can be Default/Struct/Union/Enum/Label/Keyword/Macro/Package */
    union {
        struct typeModifier  *typeModifier; /* if bits.symbolType == TypeDefault */
        struct symStructSpec *structSpec;   /* if bits.symbolType == Struct/Union */
        struct symbolList    *enums;        /* if bits.symbolType == Enum */
        struct macroBody     *mbody;        /* if bits.symbolType == Macro, can be NULL! */
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
/* They all allocate in SM memory... */
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

extern void fillSymbol(Symbol *symbol, char *name, char *linkName, Position pos);
extern void fillSymbolWithTypeModifier(Symbol *symbol, char *name, char *linkName,
                               Position pos, struct typeModifier *typeModifier);
extern void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName,
                                Position pos, int labelIndex);

/* Create and return a symbol structure... */
extern Symbol makeSymbol(char *name, char *linkName, Position pos);
extern Symbol makeSymbolWithBits(char *name, char *linkName, Position pos, Access access, Type type,
                                 Storage storage);

extern void getSymbolCxrefProperties(Symbol *symbol, ReferenceCategory *categoryP, ReferenceScope *scopeP, Storage *storageP);

#endif
