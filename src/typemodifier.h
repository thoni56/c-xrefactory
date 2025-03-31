#ifndef TYPEMODIFIER_H_INCLUDED
#define TYPEMODIFIER_H_INCLUDED

#include "type.h"
#include "symbol.h"

typedef struct typeModifier {
    enum type                type;
    union {
        struct symbol *args; /* Function - list of symbols*/
        struct symbol *typeSymbol;    /* Struct/Union/Enum */
    };
    struct symbol            *typedefSymbol;   /* the typedef symbol (if any) */
    struct typeModifier      *next;
} TypeModifier;


/* Allocates in stack space, aka XX-memory */
extern TypeModifier *newTypeModifier(Type kind, Symbol *typedefSymbol, TypeModifier *next);
extern TypeModifier *newSimpleTypeModifier(Type kind);
extern TypeModifier *newFunctionTypeModifier(Symbol *args, Symbol *typedefSymbol, TypeModifier *next);
extern TypeModifier *newPointerTypeModifier(TypeModifier *next);
extern TypeModifier *newArrayTypeModifier(void);
extern TypeModifier *newStructTypeModifier(Symbol *symbol);
extern TypeModifier *newEnumTypeModifier(Symbol *symbol);

/* And here are some fill/init functions if you need them, e.g. if you allocate elsewhere */
extern void initTypeModifier(TypeModifier *typeModifier, Type kind);
extern void initTypeModifierAsStructUnionOrEnum(TypeModifier *typeModifier, Type kind, Symbol *symbol,
                                                Symbol *typedefSymbol, TypeModifier *next);
extern void initTypeModifierAsPointer(TypeModifier *typeModifier, TypeModifier *next);
extern void initTypeModifierAsArray(TypeModifier *typeModifier,Symbol *typedefSymbol, TypeModifier *next);

extern void initFunctionTypeModifier(TypeModifier *modifier, Symbol *args);

extern TypeModifier *prependTypeModifierWith(TypeModifier *thisModifier, Type kind);

#endif
