#ifndef _TYPEMODIFIER_H_
#define _TYPEMODIFIER_H_

#include "types.h"
#include "symbol.h"

typedef struct typeModifier {
    enum type                kind;
    union typeModifierUnion {
        struct functionTypeModifier {                /* LAN_C/CPP Function */
            struct symbol     *args;
            struct symbol     **thisFunList; /* only for LAN_CPP overloaded */
        } f;
        struct methodTypeModifier {             /* LAN_JAVA Function/Method */
            char              *sig;
            struct symbolList *exceptions;
        } m;
        struct symbol         *t;            /* Struct/Union/Enum */
    } u;
    struct symbol             *typedefSymbol;  /* the typedef symbol (if any) */
    struct typeModifier      *next;
} S_typeModifier;

typedef struct typeModifierList {
    struct typeModifier		*d;
    struct typeModifierList	*next;
} S_typeModifierList;


/* Allocates in stack space, aka XX-memory */
extern S_typeModifier *newTypeModifier(Type kind, Symbol *typedefSymbol, S_typeModifier *next);
extern S_typeModifier *newFunctionTypeModifier(Symbol *args, Symbol **overLoadList, Symbol *typedefSymbol, S_typeModifier *next);
extern S_typeModifier *newArrayTypeModifier(void);
extern S_typeModifier *newStructTypeModifier(void);

/* And here are some fill/init functions if you need them, e.g. if you allocate elsewhere */
extern void initTypeModifierAsStructUnionOrEnum(S_typeModifier *typeModifier, Type kind, Symbol *symbol, Symbol *typedefSymbol, S_typeModifier *next);

extern void initTypeModifierAsFunction(S_typeModifier *typeModifier, Symbol *args, Symbol **overloadFunctionList, Symbol *typedefSymbol, S_typeModifier *next);

extern void initTypeModifierAsPointer(S_typeModifier *typeModifier, S_typeModifier *next);
extern S_typeModifierList *newTypeModifierList(S_typeModifier *d);

#endif /* _TYPEMODIFIER_H_ */
