#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/* Dependencies: */
#include "head.h"
#include "position.h"


/* ****************************************************************** */
/*              symbol definition item in symbol table                */

typedef struct symbolBits {
    unsigned			record			: 1;  /* whether struct record */
    unsigned			isSingleImported: 1;  /* whether not imported by * import */
    unsigned			accessFlags		: 12; /* java access bits */
    unsigned			javaSourceLoaded: 1;  /* is jsl source file loaded ? */
    unsigned			javaFileLoaded	: 1;  /* is class file loaded ? */

    unsigned			symType			: SYMTYPES_LN;
    /* can be Default/Struct/Union/Enum/Label/Keyword/Macro/Package */
    unsigned			storage			: STORAGES_LN;
    unsigned			npointers		: 4; /*tmp. stored #of dcl. ptrs*/
} S_symbolBits;

typedef struct symbol {
    char					*name;
    char					*linkName;		/* fully qualified name for cx */
    struct position			pos;			/* definition position for most syms;
                                              import position for imported classes!
                                            */
    struct symbolBits bits;
    union defUnion {
        struct typeModifier		*type;		/* if symType == TypeDefault */
        struct symStructSpec	*s;			/* if symType == Struct/Union */
        struct symbolList			*enums;		/* if symType == Enum */
        struct macroBody			*mbody;     /* if symType == Macro ! can be NULL! */
        int							labn;		/* break/continue label index */
        int							keyWordVal; /* if symType == Keyword */
    } u;
    struct symbol                   *next;	/* next table item with the same hash */
} Symbol;

typedef struct symbolList {
    struct symbol        *d;
    struct symbolList    *next;
} SymbolList;



/* Functions: */

/* NOTE These will not fill bits-field, has to be done after allocation */
extern Symbol *newSymbol(char *name, char *linkName, struct position pos);
extern Symbol *newSymbolAsKeyword(char *name, char *linkName, struct position pos, int keyWordVal);
extern Symbol *newSymbolAsType(char *name, char *linkName, struct position pos, struct typeModifier *type);
extern Symbol *newSymbolAsEnum(char *name, char *linkName, struct position pos, struct symbolList *enums);
extern Symbol *newSymbolAsLabel(char *name, char *linkName, struct position pos, int labelIndex);
extern void fillSymbol(Symbol *symbol, char *name, char *linkName, struct position pos);
extern void fillSymbolWithType(Symbol *symbol, char *name, char *linkName, struct position pos, struct typeModifier *type);
extern void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName, struct position pos, int labelIndex);
extern void fillSymbolWithStruct(Symbol *symbol, char *name, char *linkName, struct position pos, struct symStructSpec *structSpec);

extern void fillSymbolBits(S_symbolBits *bits, unsigned accessFlags, unsigned symType, unsigned storage);

#endif
