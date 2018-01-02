#include "generate_str_fill.h"

#include <stdio.h>

typedef struct typeModifiers {
    short                     m;
    union typeModifUnion {
        struct funTypeModif {                /* LAN_C/CPP Function */
            struct symbol     *args;
            struct symbol     **thisFunList; /* only for LAN_CPP overloaded */
        } f;
        //struct symbol       *args;         /* LAN_C Function - why not used? */
        struct methodTypeModif {             /* LAN_JAVA Function/Method */
            char              *sig;
            struct symbolList *exceptions;
        } m;
        //char                *sig;          /* LAN_JAVA Function */
        struct symbol         *t;            /* Struct/Union/Enum */
    } u;
    struct symbol             *typedefin;  /* the typedef symbol (if any) */
    struct typeModifiers      *next;
} S_typeModifiers;

static void f(void) {
    S_typeModifiers *s;
    FILLF_typeModifiers(s, 0, f ,(NULL,NULL), NULL, NULL);
}
