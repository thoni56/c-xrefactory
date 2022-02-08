#ifndef SYMBOL_H_INCLUDED
#define SYMBOL_H_INCLUDED

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

    enum type		symbolType			: SYMTYPES_LN;
    /* can be Default/Struct/Union/Enum/Label/Keyword/Macro/Package */
    enum storage	storage				: STORAGES_LN;
    unsigned		npointers			: 4; /*tmp. stored #of dcl. ptrs*/
} SymbolBits;

typedef struct symbol {
    char					*name;
    char					*linkName;		/* fully qualified name for cx */
    struct position			pos;			/* definition position for most syms;
                                               import position for imported classes! */
    struct symbolBits bits;
    union {
        struct typeModifier		*typeModifier; /* if bits.symbolType == TypeDefault */
        struct symStructSpec	*structSpec;   /* if bits.symbolType == Struct/Union */
        struct symbolList		*enums;		   /* if bits.symbolType == Enum */
        struct macroBody		*mbody;        /* if bits.symbolType == Macro, can be NULL! */
        int						labelIndex;	   /* break/continue label index */
        int						keyword;       /* if bits.symbolType == Keyword */
    } u;
    struct symbol               *next;         /* next table item with the same hash */
} Symbol;

typedef struct symbolList {
    struct symbol        *d;
    struct symbolList    *next;
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
extern void fillSymbolWithStruct(Symbol *symbol, char *name, char *linkName,
                                 Position pos, struct symStructSpec *structSpec);

extern void fillSymbolBits(SymbolBits *bits, unsigned accessFlags, unsigned symType,
                           unsigned storage);

extern Symbol makeSymbol(char *name, char *linkName, Position pos);
extern Symbol makeSymbolWithBits(char *name, char *linkName, Position pos,
                                 unsigned accessFlags, unsigned symbolType, unsigned storage);

extern SymbolBits makeSymbolBits(unsigned accessFlags, unsigned symbolType, unsigned storage);

#endif
