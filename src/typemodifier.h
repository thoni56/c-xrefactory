#ifndef _TYPEMODIFIERS_H_
#define _TYPEMODIFIERS_H_

#include "types.h"

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


extern S_typeModifier *newFunctionTypeModifier(void);

#endif /* _TYPEMODIFIERS_H_ */
