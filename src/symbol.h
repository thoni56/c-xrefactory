#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/* Dependencies: */
#include "head.h"
#include "position.h"

/* Types: */

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
        struct typeModifiers		*type;		/* if symType == TypeDefault */
        struct symStructSpecific	*s;			/* if symType == Struct/Union */
        struct symbolList			*enums;		/* if symType == Enum */
        struct macroBody			*mbody;     /* if symType == Macro ! can be NULL! */
        int							labn;		/* break/continue label index */
        int							keyWordVal; /* if symType == Keyword */
    } u;
    struct symbol                   *next;	/* next table item with the same hash */
} S_symbol;


/* Functions: */

/* NOTE These will not fill bits-field, has to be done after allocation */
extern S_symbol *newSymbol(char *name, char *linkName, struct position pos);
extern void fillSymbol(S_symbol *symbol, char *name, char *linkName, struct position pos);
extern S_symbol *newSymbolIsKeyword(char *name, char *linkName, struct position pos, int keyWordVal);
extern S_symbol *newSymbolIsType(char *name, char *linkName, struct position pos, struct typeModifiers *type);
extern void fillSymbolWithType(S_symbol *symbol, char *name, char *linkName, struct position pos, struct typeModifiers *type);
extern S_symbol *newSymbolIsEnum(char *name, char *linkName, struct position pos, struct symbolList *enums);
extern S_symbol *newSymbolIsLabel(char *name, char *linkName, struct position pos, int labelIndex);
extern void fillSymbolWithLabel(S_symbol *symbol, char *name, char *linkName, struct position pos, int labelIndex);
extern void fillSymbolWithStruct(S_symbol *symbol, char *name, char *linkName, struct position pos, struct symStructSpecific *structSpec);

#endif
