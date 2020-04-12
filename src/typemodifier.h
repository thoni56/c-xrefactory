#ifndef _TYPEMODIFIER_H_
#define _TYPEMODIFIER_H_

#include "types.h"
#include "symbol.h"

typedef struct typeModifier {
    enum type                kind;
    union typeModifierUnion {
        struct functionTypeModifier {           /* LAN_C/CPP Function */
            struct symbol     *args;
            struct symbol     **thisFunList;    /* only for LAN_CPP overloaded */
        } f;
        struct methodTypeModifier {             /* LAN_JAVA Function/Method */
            char              *signature;       /* NOTE: kind=TypeFunction */
            struct symbolList *exceptions;
        } m;
        struct symbol         *t;               /* Struct/Union/Enum */
    } u;
    struct symbol             *typedefSymbol;   /* the typedef symbol (if any) */
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
extern S_typeModifierList *newTypeModifierList(S_typeModifier *d);

/* And here are some fill/init functions if you need them, e.g. if you allocate elsewhere */
extern void initTypeModifier(S_typeModifier *typeModifier, Type kind);
extern void initTypeModifierAsStructUnionOrEnum(S_typeModifier *typeModifier, Type kind, Symbol *symbol, Symbol *typedefSymbol, S_typeModifier *next);
extern void initTypeModifierAsFunction(S_typeModifier *typeModifier, Symbol *args, Symbol **overloadFunctionList, Symbol *typedefSymbol, S_typeModifier *next);
extern void initTypeModifierAsMethod(S_typeModifier *typeModifier, char *signature, SymbolList *exceptions, Symbol *typedefSymbol, S_typeModifier *next);
extern void initTypeModifierAsPointer(S_typeModifier *typeModifier, S_typeModifier *next);
extern void initTypeModifierAsArray(S_typeModifier *typeModifier,Symbol *typedefSymbol, S_typeModifier *next);

extern void initFunctionTypeModifier(struct functionTypeModifier *modifier, Symbol *args);

#endif /* _TYPEMODIFIER_H_ */
