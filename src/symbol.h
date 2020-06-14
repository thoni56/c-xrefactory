#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/* Dependencies: */
#include "head.h"
#include "position.h"
#include "type.h"
#include "storage.h"



/* ****************************************************************** */
/*              symbol definition item in symbol table                */

typedef struct symbolBits {
    bool			isRecord			: 1;  /* whether struct record */
    bool			isExplicitlyImported: 1;  /* whether not imported by * import */
    Access			access				: 12; /* java access bits */
    bool			javaSourceIsLoaded	: 1;  /* is jsl source file loaded ? */
    bool			javaFileIsLoaded	: 1;  /* is class file loaded ? */

    enum type		symType				: SYMTYPES_LN;
    /* can be Default/Struct/Union/Enum/Label/Keyword/Macro/Package */
    enum storage	storage				: STORAGES_LN;
    unsigned		npointers			: 4; /*tmp. stored #of dcl. ptrs*/
} S_symbolBits;

typedef struct symbol {
    char					*name;
    char					*linkName;		/* fully qualified name for cx */
    struct position			pos;			/* definition position for most syms;
                                               import position for imported classes!
                                            */
    struct symbolBits bits;
    union {
        struct typeModifier		*type;		/* if symType == TypeDefault */
        struct symStructSpec	*s;			/* if symType == Struct/Union */
        struct symbolList		*enums;		/* if symType == Enum */
        struct macroBody		*mbody;     /* if symType == Macro ! can be NULL! */
        int						labelIndex;	/* break/continue label index */
        int						keyWordVal; /* if symType == Keyword */
    } u;
    struct symbol               *next;      /* next table item with the same hash */
} Symbol;

typedef struct symbolList {
    struct symbol        *d;
    struct symbolList    *next;
} SymbolList;



/* Functions: */

/* NOTE These will not fill bits-field, has to be done after allocation */
extern Symbol *newSymbol(char *name, char *linkName, struct position pos);
extern Symbol *newSymbolAsCopyOf(Symbol *original);
extern Symbol *newSymbolAsKeyword(char *name, char *linkName, struct position pos,
                                  int keyWordVal);
extern Symbol *newSymbolAsType(char *name, char *linkName, struct position pos,
                               struct typeModifier *type);
extern Symbol *newSymbolAsEnum(char *name, char *linkName, struct position pos,
                               struct symbolList *enums);
extern Symbol *newSymbolAsLabel(char *name, char *linkName, struct position pos,
                                int labelIndex);
extern void fillSymbol(Symbol *symbol, char *name, char *linkName, struct position pos);
extern void fillSymbolWithType(Symbol *symbol, char *name, char *linkName,
                               struct position pos, struct typeModifier *type);
extern void fillSymbolWithLabel(Symbol *symbol, char *name, char *linkName,
                                struct position pos, int labelIndex);
extern void fillSymbolWithStruct(Symbol *symbol, char *name, char *linkName,
                                 struct position pos, struct symStructSpec *structSpec);

extern void fillSymbolBits(S_symbolBits *bits, unsigned accessFlags, unsigned symType,
                           unsigned storage);

#endif
